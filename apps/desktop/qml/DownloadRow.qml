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
    readonly property string displayName: taskName.length > 0 ? taskName : "Unnamed task"
    readonly property string displayStatus: statusText.length > 0 ? statusText : stateText
    readonly property int progressPercent: Math.round(progress * 100)
    readonly property int downloadKiB: Math.round(downloadRate / 1024)
    readonly property int uploadKiB: Math.round(uploadRate / 1024)
    readonly property real remainingBytes: Math.max(0, totalBytes - downloadedBytes)
    readonly property real etaSeconds: downloadRate > 0 && remainingBytes > 0 ? remainingBytes / downloadRate : -1
    readonly property bool paused: stateText === "已暂停" || stateText === "Paused"

    signal openDetails(var item)
    signal pauseTask(string id)
    signal resumeTask(string id)
    signal removeTask(string id)

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

    function formatDuration(seconds) {
        if (!seconds || seconds < 0 || !isFinite(seconds)) {
            return "--"
        }

        seconds = Math.ceil(seconds)
        const days = Math.floor(seconds / 86400)
        seconds %= 86400
        const hours = Math.floor(seconds / 3600)
        seconds %= 3600
        const minutes = Math.floor(seconds / 60)

        if (days > 0) {
            return days + "d " + hours + "h"
        }
        if (hours > 0) {
            return hours + "h " + minutes + "m"
        }
        if (minutes > 0) {
            return minutes + "m"
        }
        return seconds + "s"
    }

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

    height: 118
    radius: AwaTheme.radiusMd
    clip: true
    color: selected ? "#f2f9ff" : hoverHandler.hovered ? "#fbfdff" : AwaTheme.surface
    border.width: 0

    Behavior on color { ColorAnimation { duration: 140 } }
    HoverHandler { id: hoverHandler }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: selected ? 5 : 3
        color: selected ? AwaTheme.primary : hoverHandler.hovered ? AwaTheme.borderStrong : AwaTheme.primarySoft
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            radius: 12
            color: AwaTheme.primaryPale
            border.width: 0
            Text {
                anchors.centerIn: parent
                text: "BT"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: "#dc2626"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 240
            Layout.preferredWidth: 640
            spacing: 8
            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.maximumWidth: parent.width
                text: rowRoot.displayName
                color: AwaTheme.ink
                font.pixelSize: 14
                font.weight: Font.DemiBold
                maximumLineCount: 1
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 10
                ProgressBar {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 96
                    from: 0
                    to: 1
                    value: rowRoot.progress
                    background: Rectangle {
                        implicitHeight: 8
                        radius: 4
                        color: "#fff3e6"
                        border.color: "#ffd7a8"
                    }
                    contentItem: Item {
                        implicitHeight: 8
                        Rectangle {
                            width: parent.width * rowRoot.progress
                            height: parent.height
                            radius: 4
                            color: "#f97316"
                        }
                    }
                }
                Text {
                    text: rowRoot.progressPercent + "%"
                    color: AwaTheme.inkSoft
                    font.pixelSize: 12
                    Layout.preferredWidth: 42
                    horizontalAlignment: Text.AlignRight
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.maximumWidth: parent.width
                text: rowRoot.formatBytes(rowRoot.downloadedBytes) + " / " + rowRoot.formatBytes(rowRoot.totalBytes) + " - " + rowRoot.displayStatus
                color: AwaTheme.muted
                font.pixelSize: 12
                maximumLineCount: 1
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 132
            Layout.minimumWidth: 132
            spacing: 6
            Text {
                Layout.fillWidth: true
                text: "↓ " + rowRoot.downloadKiB + " KiB/s"
                color: AwaTheme.primary
                font.pixelSize: 12
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: "↑ " + rowRoot.uploadKiB + " KiB/s"
                color: "#16a34a"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: rowRoot.stateText
                color: AwaTheme.muted
                font.pixelSize: 11
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: "ETA " + rowRoot.formatDuration(rowRoot.etaSeconds)
                color: AwaTheme.muted
                font.pixelSize: 11
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.preferredWidth: 118
            spacing: 4
            AcidToolButton {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                text: "||"
                enabled: rowRoot.downloadId.length > 0 && !rowRoot.paused
                ToolTip.visible: hovered
                ToolTip.text: "Pause"
                onClicked: rowRoot.pauseTask(rowRoot.downloadId)
            }
            AcidToolButton {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                text: "▶"
                tone: "primary"
                enabled: rowRoot.downloadId.length > 0 && rowRoot.paused
                ToolTip.visible: hovered
                ToolTip.text: "Resume"
                onClicked: rowRoot.resumeTask(rowRoot.downloadId)
            }
            AcidToolButton {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                text: "i"
                ToolTip.visible: hovered
                ToolTip.text: "Details"
                onClicked: rowRoot.openDetails(rowRoot.snapshot())
            }
        }
    }

    TapHandler {
        onTapped: rowRoot.openDetails(rowRoot.snapshot())
    }
}
