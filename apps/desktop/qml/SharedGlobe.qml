import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import AwaKurageDownloader 1.0

Item {
    id: root

    property real globeRadius: 185
    property vector3d localPoint: latLonToVector(31.2, 121.5, globeRadius + 8)
    property var gridPoints: buildGridPoints()
    property var ringDots: buildRingDots(28, 2.4)
    property real flowPhase: 0
    property quaternion globeRotation: Qt.quaternion(1, 0, 0, 0)
    property int flowsRevision: 0
    readonly property int flowLimit: 32
    readonly property int peerCount: {
        flowsRevision
        return downloadManager.sharedPeerFlows.count()
    }
    readonly property real totalDownloadRate: aggregateRate("download")
    readonly property real totalUploadRate: aggregateRate("upload")

    function formatBytes(bytes) {
        if (!bytes || bytes <= 0) {
            return "0 KiB/s"
        }
        const units = ["B/s", "KiB/s", "MiB/s", "GiB/s"]
        let value = bytes
        let unit = 0
        while (value >= 1024 && unit < units.length - 1) {
            value /= 1024
            unit += 1
        }
        return (unit === 0 ? Math.round(value).toString() : value.toFixed(value >= 100 ? 0 : value >= 10 ? 1 : 2)) + " " + units[unit]
    }

    function aggregateRate(direction) {
        flowsRevision
        let total = 0
        const count = downloadManager.sharedPeerFlows.count()
        for (let i = 0; i < count; ++i) {
            const flow = downloadManager.sharedPeerFlows.get(i)
            if (flow.direction === direction) {
                total += flow.rate || 0
            }
        }
        return total
    }

    function markFlowsChanged() {
        flowsRevision += 1
    }

    function buildGridPoints() {
        const points = []
        for (let lat = -75; lat <= 75; lat += 15) {
            const ring = Math.max(28, Math.round(112 * Math.cos(lat * Math.PI / 180)))
            for (let i = 0; i < ring; ++i) {
                const lon = i * 360 / ring
                const p = latLonToVector(lat, lon, globeRadius + 2.2)
                points.push({"x": p.x, "y": p.y, "z": p.z, "scale": 0.0068, "alpha": lat === 0 ? 0.52 : 0.34})
            }
        }
        for (let lonLine = -180; lonLine < 180; lonLine += 15) {
            for (let latLine = -82.5; latLine <= 82.5; latLine += 7.5) {
                const p = latLonToVector(latLine, lonLine, globeRadius + 2.4)
                points.push({"x": p.x, "y": p.y, "z": p.z, "scale": 0.0062, "alpha": lonLine % 45 === 0 ? 0.42 : 0.28})
            }
        }
        return points
    }

    function buildRingDots(count, radius) {
        const dots = []
        for (let i = 0; i < count; ++i) {
            const angle = i * Math.PI * 2 / count
            dots.push({"x": Math.cos(angle) * radius, "y": Math.sin(angle) * radius})
        }
        return dots
    }

    function latLonToVector(latitude, longitude, radius) {
        const lat = latitude * Math.PI / 180
        const lon = longitude * Math.PI / 180
        const cosLat = Math.cos(lat)
        return Qt.vector3d(
            radius * cosLat * Math.sin(lon),
            radius * Math.sin(lat),
            radius * cosLat * Math.cos(lon))
    }

    function normalizeVector(vector, radius) {
        const length = Math.sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
        if (length <= 0) {
            return Qt.vector3d(0, 0, radius)
        }
        const factor = radius / length
        return Qt.vector3d(vector.x * factor, vector.y * factor, vector.z * factor)
    }

    function dotVector(left, right) {
        return left.x * right.x + left.y * right.y + left.z * right.z
    }

    function crossVector(left, right) {
        return Qt.vector3d(
            left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x)
    }

    function trackballVector(x, y, width, height) {
        const size = Math.max(1, Math.min(width, height))
        const nx = (2 * x - width) / size
        const ny = (height - 2 * y) / size
        const lengthSquared = nx * nx + ny * ny
        if (lengthSquared <= 1) {
            return Qt.vector3d(nx, ny, Math.sqrt(1 - lengthSquared))
        }

        const length = Math.sqrt(lengthSquared)
        return Qt.vector3d(nx / length, ny / length, 0)
    }

    function multiplyQuaternion(left, right) {
        return Qt.quaternion(
            left.scalar * right.scalar - left.x * right.x - left.y * right.y - left.z * right.z,
            left.scalar * right.x + left.x * right.scalar + left.y * right.z - left.z * right.y,
            left.scalar * right.y - left.x * right.z + left.y * right.scalar + left.z * right.x,
            left.scalar * right.z + left.x * right.y - left.y * right.x + left.z * right.scalar)
    }

    function normalizeQuaternion(value) {
        const length = Math.sqrt(
            value.scalar * value.scalar +
            value.x * value.x +
            value.y * value.y +
            value.z * value.z)
        if (length <= 0) {
            return Qt.quaternion(1, 0, 0, 0)
        }
        return Qt.quaternion(value.scalar / length, value.x / length, value.y / length, value.z / length)
    }

    function rotateGlobeByDrag(fromVector, toVector) {
        const axis = crossVector(fromVector, toVector)
        const axisLength = Math.sqrt(dotVector(axis, axis))
        if (axisLength <= 0.0001) {
            return
        }

        const clampedDot = Math.max(-1, Math.min(1, dotVector(fromVector, toVector)))
        const angle = Math.acos(clampedDot) * 180 / Math.PI
        const delta = Quaternion.fromAxisAndAngle(
            Qt.vector3d(axis.x / axisLength, axis.y / axisLength, axis.z / axisLength),
            angle)
        globeRotation = normalizeQuaternion(multiplyQuaternion(delta, globeRotation))
    }

    function arcPosition(start, end, amount) {
        const lift = Math.sin(amount * Math.PI) * 46
        const mixed = Qt.vector3d(
            start.x * (1 - amount) + end.x * amount,
            start.y * (1 - amount) + end.y * amount,
            start.z * (1 - amount) + end.z * amount)
        return normalizeVector(mixed, globeRadius + 18 + lift)
    }

    function flowStart(direction, latitude, longitude) {
        const remote = latLonToVector(latitude || 0, longitude || 0, globeRadius + 8)
        return direction === "download" ? remote : localPoint
    }

    function flowEnd(direction, latitude, longitude) {
        const remote = latLonToVector(latitude || 0, longitude || 0, globeRadius + 8)
        return direction === "download" ? localPoint : remote
    }

    function flowColor(direction) {
        return direction === "download" ? "#e5484d" : "#22b56b"
    }

    function beadScale(rateValue, index) {
        const rate = Math.max(1, rateValue || 1)
        const weight = Math.min(0.035, 0.01 + Math.log(rate / 1024 + 1) / 300)
        const pulse = ((index / 10 - flowPhase + 1) % 1)
        return weight * (pulse > 0.72 ? 1.18 : 0.82)
    }

    Connections {
        target: downloadManager.sharedPeerFlows
        function onModelReset() { root.markFlowsChanged() }
        function onRowsInserted() { root.markFlowsChanged() }
        function onRowsRemoved() { root.markFlowsChanged() }
        function onDataChanged() { root.markFlowsChanged() }
    }

    NumberAnimation on flowPhase {
        from: 0
        to: 1
        duration: 1800
        loops: Animation.Infinite
    }

    function tangentA(normal) {
        const up = Math.abs(normal.y) > 0.86 ? Qt.vector3d(1, 0, 0) : Qt.vector3d(0, 1, 0)
        return normalizeVector(Qt.vector3d(
            up.y * normal.z - up.z * normal.y,
            up.z * normal.x - up.x * normal.z,
            up.x * normal.y - up.y * normal.x), 1)
    }

    function tangentB(normal, tangent) {
        return normalizeVector(Qt.vector3d(
            normal.y * tangent.z - normal.z * tangent.y,
            normal.z * tangent.x - normal.x * tangent.z,
            normal.x * tangent.y - normal.y * tangent.x), 1)
    }

    function ringDotPosition(center, dot) {
        const normal = normalizeVector(center, 1)
        const a = tangentA(normal)
        const b = tangentB(normal, a)
        return Qt.vector3d(
            center.x + a.x * dot.x + b.x * dot.y,
            center.y + a.y * dot.x + b.y * dot.y,
            center.z + a.z * dot.x + b.z * dot.y)
    }

    Rectangle {
        anchors.fill: parent
        color: AwaTheme.page
    }

    View3D {
        id: view
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: AwaTheme.page
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 88, 570)
            eulerRotation.x: -8
            clipFar: 1800
        }

        DirectionalLight {
            eulerRotation.x: -35
            eulerRotation.y: 35
            brightness: 1.2
            color: "#ffffff"
        }

        PointLight {
            position: Qt.vector3d(-260, 160, 220)
            brightness: 2.4
            color: "#bff1ff"
        }

        Node {
            id: globeNode
            rotation: root.globeRotation

            Model {
                source: "#Sphere"
                scale: Qt.vector3d(
                    (root.globeRadius + 0.9) / 50,
                    (root.globeRadius + 0.9) / 50,
                    (root.globeRadius + 0.9) / 50)
                castsShadows: false
                receivesShadows: false
                materials: PrincipledMaterial {
                    baseColor: "#243342"
                    baseColorMap: Texture {
                        source: appWorldDotsSource
                    }
                    alphaMode: PrincipledMaterial.Blend
                    lighting: PrincipledMaterial.NoLighting
                    opacity: 0.42
                    cullMode: Material.NoCulling
                    depthDrawMode: Material.NeverDepthDraw
                }
            }

            Repeater3D {
                model: root.gridPoints
                delegate: Model {
                    source: "#Sphere"
                    position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                    scale: Qt.vector3d(modelData.scale, modelData.scale, modelData.scale)
                    materials: PrincipledMaterial {
                        baseColor: Qt.rgba(0.28, 0.52, 0.72, modelData.alpha)
                        emissiveFactor: Qt.vector3d(0.06, 0.13, 0.18)
                        alphaMode: PrincipledMaterial.Blend
                        lighting: PrincipledMaterial.NoLighting
                        cullMode: Material.NoCulling
                    }
                }
            }

            Node {
                id: localRing
                property vector3d centerPoint: root.localPoint
                Repeater3D {
                    model: root.ringDots
                    delegate: Model {
                        source: "#Sphere"
                        position: root.ringDotPosition(localRing.centerPoint, modelData)
                        scale: Qt.vector3d(0.0065, 0.0065, 0.0065)
                        materials: PrincipledMaterial {
                            baseColor: "#1f7ae0"
                            emissiveFactor: Qt.vector3d(0.03, 0.12, 0.28)
                            lighting: PrincipledMaterial.NoLighting
                        }
                    }
                }
            }

            Repeater3D {
                model: downloadManager.sharedPeerFlows
                delegate: Node {
                    id: flowNode
                    visible: index < root.flowLimit
                    property string flowDirection: direction
                    property real flowRate: rate || 0
                    property real flowLatitude: latitude || 0
                    property real flowLongitude: longitude || 0
                    property vector3d startPoint: root.flowStart(flowDirection, flowLatitude, flowLongitude)
                    property vector3d endPoint: root.flowEnd(flowDirection, flowLatitude, flowLongitude)
                    property vector3d remotePoint: root.latLonToVector(flowLatitude, flowLongitude, root.globeRadius + 9)

                    Repeater3D {
                        model: flowNode.visible ? 10 : 0
                        delegate: Model {
                            source: "#Sphere"
                            readonly property real amount: (index + 1) / 11
                            position: root.arcPosition(flowNode.startPoint, flowNode.endPoint, amount)
                            scale: {
                                const s = root.beadScale(flowNode.flowRate, index)
                                return Qt.vector3d(s, s, s)
                            }
                            opacity: 0.18 + (((index / 10 + root.flowPhase) % 1) * 0.28)
                            materials: PrincipledMaterial {
                                baseColor: root.flowColor(flowNode.flowDirection)
                                emissiveFactor: flowNode.flowDirection === "download"
                                    ? Qt.vector3d(0.42, 0.04, 0.05)
                                    : Qt.vector3d(0.03, 0.36, 0.15)
                                alphaMode: PrincipledMaterial.Blend
                                lighting: PrincipledMaterial.NoLighting
                            }
                        }
                    }

                    Repeater3D {
                        model: flowNode.visible ? root.ringDots : []
                        delegate: Model {
                            source: "#Sphere"
                            position: root.ringDotPosition(flowNode.remotePoint, modelData)
                            scale: Qt.vector3d(0.007, 0.007, 0.007)
                            materials: PrincipledMaterial {
                                baseColor: root.flowColor(flowNode.flowDirection)
                                emissiveFactor: flowNode.flowDirection === "download"
                                    ? Qt.vector3d(0.38, 0.03, 0.04)
                                    : Qt.vector3d(0.03, 0.32, 0.13)
                                lighting: PrincipledMaterial.NoLighting
                            }
                        }
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            acceptedButtons: Qt.LeftButton

            property vector3d lastVector: Qt.vector3d(0, 0, 1)

            onPressed: mouse => {
                lastVector = root.trackballVector(mouse.x, mouse.y, width, height)
            }

            onPositionChanged: mouse => {
                if (!(pressedButtons & Qt.LeftButton)) {
                    return
                }

                const nextVector = root.trackballVector(mouse.x, mouse.y, width, height)
                root.rotateGlobeByDrag(lastVector, nextVector)
                lastVector = nextVector
            }
        }
    }

    Rectangle {
        z: 3
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 24
        width: Math.min(360, parent.width - 48)
        height: statsColumn.implicitHeight
        color: "transparent"
        border.width: 0

        ColumnLayout {
            id: statsColumn
            anchors.fill: parent
            spacing: 10

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 8
                columnSpacing: 18
                Text { text: I18n.tr("活跃流", "Active flows"); color: AwaTheme.muted; font.pixelSize: 12 }
                Text { text: root.peerCount; color: AwaTheme.ink; font.pixelSize: 12; font.weight: Font.DemiBold }
                Text { text: I18n.tr("上传", "Upload"); color: AwaTheme.muted; font.pixelSize: 12 }
                Text { text: root.formatBytes(root.totalUploadRate); color: "#168451"; font.pixelSize: 12; font.weight: Font.DemiBold }
                Text { text: I18n.tr("下载", "Download"); color: AwaTheme.muted; font.pixelSize: 12 }
                Text { text: root.formatBytes(root.totalDownloadRate); color: "#b4232b"; font.pixelSize: 12; font.weight: Font.DemiBold }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Rectangle { Layout.preferredWidth: 9; Layout.preferredHeight: 9; radius: 4; color: "#22b56b" }
                Text { text: I18n.tr("上传", "Upload"); color: AwaTheme.inkSoft; font.pixelSize: 11 }
                Rectangle { Layout.preferredWidth: 9; Layout.preferredHeight: 9; radius: 4; color: "#e5484d"; Layout.leftMargin: 8 }
                Text { text: I18n.tr("下载", "Download"); color: AwaTheme.inkSoft; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
            }
            Text {
                Layout.fillWidth: true
                text: I18n.tr("IP 区域来自资源目录 geoip.dat。", "IP regions come from resources/geoip.dat.")
                color: AwaTheme.muted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }
    }

    Rectangle {
        z: 3
        anchors.centerIn: parent
        anchors.verticalCenterOffset: Math.round(parent.height * 0.26)
        width: Math.min(360, parent.width - 48)
        height: emptyStateColumn.implicitHeight
        color: "transparent"
        border.width: 0
        visible: root.peerCount === 0

        ColumnLayout {
            id: emptyStateColumn
            anchors.fill: parent
            spacing: 6
            Text {
                Layout.fillWidth: true
                text: I18n.tr("暂无共享节点流量", "No sharing node traffic")
                color: AwaTheme.ink
                font.pixelSize: 16
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                Layout.fillWidth: true
                text: I18n.tr("开始下载或做种，并在资源目录提供 geoip.dat 后，已定位 peer 会显示在这里。", "Located peers appear here while downloading or seeding after resources/geoip.dat is provided.")
                color: AwaTheme.muted
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }
}
