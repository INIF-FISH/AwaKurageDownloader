import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AwaKurageDownloader 1.0

Rectangle {
    id: rowRoot
    property var itemData: ({})
    readonly property string downloadId: itemData.downloadId || ""
    readonly property string taskName: itemData.name || ""
    readonly property string stateText: itemData.stateText || ""
    readonly property int state: itemData.state === undefined ? -1 : itemData.state
    readonly property string statusText: itemData.statusText || ""
    readonly property real progress: itemData.progress || 0
    readonly property real downloadRate: itemData.downloadRate || 0
    readonly property real uploadRate: itemData.uploadRate || 0
    readonly property bool isComplete: itemData.isComplete === true
    readonly property real totalBytes: itemData.totalBytes || 0
    readonly property real downloadedBytes: itemData.downloadedBytes || 0
    readonly property int pieceCount: itemData.pieceCount || 0
    readonly property int completedPieces: itemData.completedPieces || 0
    readonly property string pieceMap: itemData.pieceMap || ""
    readonly property string savePath: itemData.savePath || ""
    property int rowIndex: -1
    property bool selected: false
    readonly property string displayName: taskName.length > 0 ? taskName : I18n.tr("未命名任务", "Unnamed task")
    readonly property string displayStatus: statusText.length > 0 ? I18n.dynamic(statusText) : localizedStateText()
    readonly property real displayProgress: Math.max(0, Math.min(1, progress))
    readonly property int progressPercent: Math.round(displayProgress * 100)
    readonly property int downloadKiB: Math.round(downloadRate / 1024)
    readonly property int uploadKiB: Math.round(uploadRate / 1024)
    readonly property real remainingBytes: Math.max(0, totalBytes - downloadedBytes)
    readonly property real etaSeconds: downloadRate > 0 && remainingBytes > 0 ? remainingBytes / downloadRate : -1
    readonly property bool completed: isComplete
    readonly property bool seeding: state === 4 || state === 7
    readonly property bool finished: state === 5
    readonly property bool deleted: state === 9
    readonly property bool terminal: state === 5 || state === 6 || deleted
    readonly property bool paused: state === 3 || state === 7
    readonly property bool waiting: state === 8
    readonly property color rowColor: selected
        ? (deleted ? "#fff1f2" : completed ? "#eefcf4" : waiting ? "#fff8ed" : "#f2f9ff")
        : hoverHandler.hovered
            ? (deleted ? "#fff7f7" : completed ? "#f6fdf8" : waiting ? "#fffbf4" : "#fbfdff")
            : (deleted ? "#fffafa" : completed ? "#f7fff9" : waiting ? "#fffaf0" : "#ffffff")
    readonly property color accentColor: selected
        ? (deleted ? "#dc2626" : finished ? "#16a34a" : seeding ? "#0f9fb8" : waiting ? "#f59e0b" : "#2d8df0")
        : hoverHandler.hovered
            ? (deleted ? "#ef4444" : finished ? "#22c55e" : seeding ? "#22cbd6" : waiting ? "#fbbf24" : "#80b8ee")
            : (deleted ? "#fecaca" : finished ? "#86efac" : seeding ? "#99f6e4" : completed ? "#b7ebc9" : waiting ? "#fde68a" : "#d9ecff")
    readonly property color badgeColor: deleted ? "#fff1f2" : finished ? "#ecfdf3" : seeding ? "#ecfeff" : waiting ? "#fffbeb" : "#eef7ff"
    readonly property color badgeTextColor: deleted ? "#b91c1c" : finished ? "#15803d" : seeding ? "#0f766e" : waiting ? "#b45309" : "#dc2626"
    readonly property color progressTrackColor: deleted ? "#fee2e2" : completed ? "#dcfce7" : waiting ? "#fef3c7" : "#fff3e6"
    readonly property color progressTrackBorderColor: deleted ? "#fecaca" : completed ? "#a7f3d0" : waiting ? "#fde68a" : "#ffd7a8"
    readonly property color progressFillColor: deleted ? "#ef4444" : finished ? "#22c55e" : seeding ? "#14b8a6" : waiting ? "#f59e0b" : "#f97316"

    signal openDetails(var item)
    signal pauseTask(string id)
    signal resumeTask(string id)
    signal openFolder(string id)
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

    function localizedStateText() {
        switch (state) {
        case 0: return I18n.tr("排队中", "Queued")
        case 1: return I18n.tr("获取元数据", "Fetching metadata")
        case 2: return I18n.tr("下载中", "Downloading")
        case 3: return I18n.tr("暂停下载", "Paused")
        case 4: return I18n.tr("做种中", "Seeding")
        case 5: return I18n.tr("已完成", "Finished")
        case 6: return I18n.tr("错误", "Error")
        case 7: return I18n.tr("暂停做种", "Paused seeding")
        case 8: return I18n.tr("等待下载", "Waiting")
        case 9: return I18n.tr("已删除", "Deleted")
        default: return stateText.length > 0 ? I18n.dynamic(stateText) : I18n.tr("未知", "Unknown")
        }
    }

    function snapshot() {
        return {
            "downloadId": downloadId,
            "rowIndex": rowIndex,
            "name": taskName,
            "state": state,
            "stateText": stateText,
            "statusText": statusText,
            "progress": progress,
            "totalBytes": totalBytes,
            "downloadedBytes": downloadedBytes,
            "downloadRate": downloadRate,
            "uploadRate": uploadRate,
            "isComplete": isComplete,
            "pieceCount": pieceCount,
            "completedPieces": completedPieces,
            "pieceMap": pieceMap,
            "savePath": savePath
        }
    }

    height: 102
    radius: AwaTheme.radiusMd
    clip: true
    color: rowColor
    border.width: 0

    Behavior on color { ColorAnimation { duration: 140 } }
    HoverHandler { id: hoverHandler }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: selected ? 5 : 3
        color: accentColor
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: 10
            color: badgeColor
            border.width: 0
            Text {
                anchors.centerIn: parent
                text: rowRoot.finished ? "OK" : rowRoot.seeding ? "SE" : rowRoot.waiting ? "WT" : "BT"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: badgeTextColor
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 240
            Layout.preferredWidth: 640
            spacing: 7
            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.maximumWidth: parent.width
                text: rowRoot.displayName
                color: AwaTheme.ink
                font.pixelSize: 13
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
                    value: rowRoot.displayProgress
                    background: Rectangle {
                        implicitHeight: 8
                        radius: 4
                        color: progressTrackColor
                        border.color: progressTrackBorderColor
                    }
                    contentItem: Item {
                        implicitHeight: 8
                        Rectangle {
                            width: parent.width * rowRoot.displayProgress
                            height: parent.height
                            radius: 4
                            color: progressFillColor
                        }
                    }
                }
                Text {
                    text: rowRoot.progressPercent + "%"
                    color: AwaTheme.inkSoft
                    font.pixelSize: 11
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
                font.pixelSize: 11
                maximumLineCount: 1
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 118
            Layout.minimumWidth: 118
            spacing: 5
            Text {
                Layout.fillWidth: true
                text: "↓ " + rowRoot.downloadKiB + " KiB/s"
                color: rowRoot.completed ? rowRoot.progressFillColor : AwaTheme.primary
                font.pixelSize: 11
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: "↑ " + rowRoot.uploadKiB + " KiB/s"
                color: rowRoot.finished ? "#15803d" : rowRoot.seeding ? "#0f766e" : "#16a34a"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: rowRoot.localizedStateText()
                color: AwaTheme.muted
                font.pixelSize: 10
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: I18n.tr("预计 ", "ETA ") + rowRoot.formatDuration(rowRoot.etaSeconds)
                color: AwaTheme.muted
                font.pixelSize: 10
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.preferredWidth: 104
            spacing: 4
            AcidToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                text: "||"
                visible: !rowRoot.terminal && !rowRoot.paused
                enabled: rowRoot.downloadId.length > 0 && !rowRoot.paused
                ToolTip.visible: hovered
                ToolTip.text: I18n.tr("暂停", "Pause")
                onClicked: rowRoot.pauseTask(rowRoot.downloadId)
            }
            AcidToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                text: "▶"
                tone: "primary"
                visible: !rowRoot.terminal && rowRoot.paused
                enabled: rowRoot.downloadId.length > 0 && rowRoot.paused
                ToolTip.visible: hovered
                ToolTip.text: I18n.tr("继续", "Resume")
                onClicked: rowRoot.resumeTask(rowRoot.downloadId)
            }
            AcidToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                text: "\uD83D\uDCC1"
                tone: "primary"
                visible: rowRoot.completed
                enabled: rowRoot.downloadId.length > 0 && rowRoot.savePath.length > 0
                ToolTip.visible: hovered
                ToolTip.text: I18n.tr("打开文件夹", "Open folder")
                onClicked: rowRoot.openFolder(rowRoot.downloadId)
            }
            AcidToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                text: "i"
                ToolTip.visible: hovered
                ToolTip.text: I18n.tr("详情", "Details")
                onClicked: rowRoot.openDetails(rowRoot.snapshot())
            }
        }
    }

    TapHandler {
        onTapped: rowRoot.openDetails(rowRoot.snapshot())
    }
}
