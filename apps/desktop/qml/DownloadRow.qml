import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: rowRoot
    property var itemData: ({})
    readonly property string downloadId: itemData.downloadId || ""
    readonly property string taskName: itemData.name || ""
    readonly property string stateText: itemData.stateText || ""
    readonly property string statusText: itemData.statusText || ""
    readonly property real progress: itemData.progress || 0
    readonly property real downloadRate: itemData.downloadRate || 0
    readonly property real uploadRate: itemData.uploadRate || 0
    readonly property real totalBytes: itemData.totalBytes || 0
    readonly property real downloadedBytes: itemData.downloadedBytes || 0
    readonly property int pieceCount: itemData.pieceCount || 0
    readonly property int completedPieces: itemData.completedPieces || 0
    readonly property string pieceMap: itemData.pieceMap || ""
    readonly property string savePath: itemData.savePath || ""
    property int rowIndex: -1
    property bool selected: false
    readonly property string displayName: taskName.length > 0 ? taskName : "未命名任务"
    readonly property string displayStatus: statusText.length > 0 ? statusText : stateText
    readonly property int progressPercent: Math.round(progress * 100)
    readonly property int downloadKiB: Math.round(downloadRate / 1024)
    readonly property int uploadKiB: Math.round(uploadRate / 1024)
    readonly property bool paused: stateText === "已暂停"

    function formatBytes(bytes) {
        if (!bytes || bytes <= 0) {
            return "-"
        }
        const units = ["B", "KiB", "MiB", "GiB", "TiB"]
        let value = bytes
        let unit = 0
        while (value >= 1024 && unit < units.length - 1) {
            value /= 1024
            unit += 1
        }
        return (unit === 0 ? Math.round(value).toString() : value.toFixed(value >= 100 ? 0 : value >= 10 ? 1 : 2)) + " " + units[unit]
    }

    signal openDetails(var item)
    signal pauseTask(string id)
    signal resumeTask(string id)
    signal removeTask(string id)

    height: 106
    radius: 8
    clip: true
    color: selected ? "#e0f2fe" : hoverHandler.hovered ? "#f8fafc" : "#ffffff"
    border.color: selected ? "#38bdf8" : "#e2e8f0"

    Behavior on color { ColorAnimation { duration: 140 } }
    HoverHandler { id: hoverHandler }

    function snapshot() {
        return {
            "downloadId": downloadId,
            "rowIndex": rowIndex,
            "name": taskName,
            "stateText": stateText,
            "statusText": statusText,
            "progress": progress,
            "totalBytes": totalBytes,
            "downloadedBytes": downloadedBytes,
            "downloadRate": downloadRate,
            "uploadRate": uploadRate,
            "pieceCount": pieceCount,
            "completedPieces": completedPieces,
            "pieceMap": pieceMap,
            "savePath": savePath
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 42
            Layout.preferredHeight: 42
            radius: 8
            color: "#f0fdfa"
            Text {
                anchors.centerIn: parent
                text: "BT"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: "#0f766e"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 180
            spacing: 7
            Text {
                Layout.fillWidth: true
                text: rowRoot.displayName
                color: "#0f172a"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: rowRoot.progress
                }
                Text {
                    text: rowRoot.progressPercent + "%"
                    color: "#475569"
                    font.pixelSize: 12
                    Layout.preferredWidth: 42
                    horizontalAlignment: Text.AlignRight
                }
            }
            Text {
                Layout.fillWidth: true
                text: rowRoot.formatBytes(rowRoot.downloadedBytes) + " / " + rowRoot.formatBytes(rowRoot.totalBytes) + " · " + rowRoot.displayStatus
                color: "#64748b"
                font.pixelSize: 12
                elide: Text.ElideRight
            }
            Item {
                id: rowPieceBar
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                clip: true
                readonly property int sampleCount: Math.min(rowRoot.pieceMap.length, 64)
                Row {
                    anchors.fill: parent
                    spacing: 1
                    Repeater {
                        model: rowPieceBar.sampleCount
                        Rectangle {
                            width: rowPieceBar.sampleCount > 0
                                ? Math.max(1, (rowPieceBar.width - (rowPieceBar.sampleCount - 1)) / rowPieceBar.sampleCount)
                                : 0
                            height: rowPieceBar.height
                            radius: 1
                            color: rowRoot.pieceMap.charAt(index) === "1" ? "#0f766e" : "#e2e8f0"
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 104
            Layout.minimumWidth: 104
            spacing: 4
            Text {
                Layout.fillWidth: true
                text: "↓ " + rowRoot.downloadKiB + " KiB/s"
                color: "#0f766e"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: "↑ " + rowRoot.uploadKiB + " KiB/s"
                color: "#64748b"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: rowRoot.stateText
                color: "#94a3b8"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.preferredWidth: 110
            spacing: 3
            ToolButton {
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
                text: "⏸"
                enabled: rowRoot.downloadId.length > 0 && !rowRoot.paused
                ToolTip.visible: hovered
                ToolTip.text: "暂停"
                onClicked: rowRoot.pauseTask(rowRoot.downloadId)
            }
            ToolButton {
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
                text: "▶"
                enabled: rowRoot.downloadId.length > 0 && rowRoot.paused
                ToolTip.visible: hovered
                ToolTip.text: "继续"
                onClicked: rowRoot.resumeTask(rowRoot.downloadId)
            }
            ToolButton {
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
                text: "⋯"
                ToolTip.visible: hovered
                ToolTip.text: "详情"
                onClicked: rowRoot.openDetails(rowRoot.snapshot())
            }
        }
    }

    TapHandler {
        onTapped: rowRoot.openDetails(rowRoot.snapshot())
    }
}
