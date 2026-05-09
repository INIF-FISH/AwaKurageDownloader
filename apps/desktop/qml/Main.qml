import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import AwaKurageDownloader 1.0

ApplicationWindow {
    id: window
    width: 1180
    height: 760
    minimumWidth: 920
    minimumHeight: 620
    visible: true
    title: "AwaKurageDownloader"
    color: AwaTheme.page

    onClosing: function(close) {
        close.accepted = !trayController.closeToTray(window)
    }

    property string selectedDownloadId: ""
    property var selectedDownload: ({})
    property string toastText: ""
    property string selectedLanguage: ""
    property bool completionSoundEnabled: true
    property int currentPage: 0
    property int downloadSectionTab: 0
    property int downloadsRevision: 0
    property int pieceMapRevision: 0
    property int downloadingCount: 0
    property int completedCount: 0
    property int downloadingUnseenCount: 0
    property int completedUnseenCount: 0
    property int selectedChokingAlgorithm: downloadManager.chokingAlgorithm
    property int selectedSeedChokingAlgorithm: downloadManager.seedChokingAlgorithm
    property bool downloadTabBadgesReady: false
    property var downloadTabSnapshot: ({})
    readonly property bool hasSelectedDownload: selectedDownloadId.length > 0
    readonly property int selectedState: selectedDownload.state === undefined ? -1 : selectedDownload.state
    readonly property bool selectedIsComplete: selectedDownload.isComplete === true
    readonly property string selectedStateText: selectedDownload.stateText || ""
    readonly property bool selectedIsPaused: selectedState === 3 || selectedState === 7
    readonly property bool selectedIsTerminal: selectedState === 5 || selectedState === 6
    readonly property bool selectedCanPause: hasSelectedDownload && !selectedIsPaused && !selectedIsTerminal
    readonly property bool selectedCanResume: hasSelectedDownload && selectedIsPaused
    readonly property var pageTitles: [
        I18n.tr("下载任务", "Downloads"),
        I18n.tr("RSS 订阅", "RSS"),
        I18n.tr("远程 API", "Remote API"),
        I18n.tr("设置", "Settings")
    ]

    readonly property color panelDivider: "#d8dee6"

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

    function positiveInt(text, fallback) {
        const parsed = parseInt(text)
        return !isNaN(parsed) && parsed >= 0 ? parsed : fallback
    }

    function selectDownload(id, fallback) {
        if ((!id || id.length === 0) && fallback && fallback.rowIndex >= 0) {
            const data = downloadManager.downloads.get(fallback.rowIndex)
            id = data.downloadId || ""
            fallback = data
        }
        if (!id || id.length === 0) {
            return
        }
        selectedDownloadId = id
        if (fallback) {
            selectedDownload = fallback
        }
        refreshSelectedDownload()
    }

    function refreshSelectedDownload() {
        if (selectedDownloadId.length === 0) {
            selectedDownload = ({})
            return
        }

        const data = downloadManager.downloads.getById(selectedDownloadId)
        if (data && data.downloadId) {
            selectedDownload = data
        }
    }

    function showToast(message) {
        toastText = I18n.dynamic(message)
        toastPopup.open()
    }

    function trackerSummary(workingCount, trackerCount) {
        if (I18n.japanese) {
            return (workingCount || 0) + " 稼働 / " + (trackerCount || 0) + " 合計"
        }
        return I18n.english
            ? ((workingCount || 0) + " working / " + (trackerCount || 0) + " total")
            : ((workingCount || 0) + " 可用 / " + (trackerCount || 0) + " 总数")
    }

    function piecesSummary(completedPieces, pieceCount) {
        if (I18n.japanese) {
            return (completedPieces || 0) + " / " + (pieceCount || 0) + " ピース"
        }
        return I18n.english
            ? ((completedPieces || 0) + " / " + (pieceCount || 0) + " pieces")
            : ((completedPieces || 0) + " / " + (pieceCount || 0) + " 个分块")
    }

    function isCompletedDownload(item) {
        return item && item.isComplete === true
    }

    function matchesDownloadSection(item) {
        if (!item || item.state === undefined) {
            return false
        }
        return downloadSectionTab === 0 ? !isCompletedDownload(item) : isCompletedDownload(item)
    }

    function filteredDownloadCount() {
        downloadsRevision
        let count = 0
        for (let i = 0; i < downloadManager.downloads.count(); ++i) {
            if (matchesDownloadSection(downloadManager.downloads.get(i))) {
                count += 1
            }
        }
        return count
    }

    function downloadSectionForItem(item) {
        if (!item || item.state === undefined) {
            return -1
        }
        return isCompletedDownload(item) ? 1 : 0
    }

    function markCurrentDownloadTabSeen() {
        if (currentPage !== 0) {
            return
        }
        if (downloadSectionTab === 0) {
            downloadingUnseenCount = 0
        } else {
            completedUnseenCount = 0
        }
    }

    function refreshDownloadTabBadges(initialLoad) {
        const nextSnapshot = ({})
        let nextDownloadingCount = 0
        let nextCompletedCount = 0
        let nextDownloadingUnseen = downloadingUnseenCount
        let nextCompletedUnseen = completedUnseenCount

        for (let i = 0; i < downloadManager.downloads.count(); ++i) {
            const item = downloadManager.downloads.get(i)
            if (!item || !item.downloadId) {
                continue
            }

            const section = downloadSectionForItem(item)
            nextSnapshot[item.downloadId] = section
            if (section === 0) {
                nextDownloadingCount += 1
            } else if (section === 1) {
                nextCompletedCount += 1
            }

            const hasPreviousSection = Object.prototype.hasOwnProperty.call(downloadTabSnapshot, item.downloadId)
            const previousSection = downloadTabSnapshot[item.downloadId]
            const shouldCountBadge = !initialLoad && downloadTabBadgesReady && section >= 0
                && (!hasPreviousSection || previousSection !== section)
            if (shouldCountBadge) {
                if (!(currentPage === 0 && downloadSectionTab === section)) {
                    if (section === 0) {
                        nextDownloadingUnseen += 1
                    } else {
                        nextCompletedUnseen += 1
                    }
                }
            }
        }

        downloadTabSnapshot = nextSnapshot
        downloadingCount = nextDownloadingCount
        completedCount = nextCompletedCount
        downloadingUnseenCount = nextDownloadingUnseen
        completedUnseenCount = nextCompletedUnseen
        markCurrentDownloadTabSeen()
    }

    function downloadTabLabel(baseText, count, unseenCount) {
        return unseenCount > 0
            ? baseText + " " + count + "  +" + unseenCount
            : baseText + " " + count
    }

    function roleListContains(roles, role) {
        if (!roles || roles.length === 0) {
            return true
        }
        for (let i = 0; i < roles.length; ++i) {
            if (roles[i] === role) {
                return true
            }
        }
        return false
    }

    function addMagnetFromDialog() {
        downloadManager.addMagnet(magnetInput.text, {"savePath": savePathInput.text})
        magnetInput.clear()
        magnetDialog.close()
    }

    component Panel: Rectangle {
        radius: AwaTheme.radiusLg
        color: AwaTheme.surface
        border.color: AwaTheme.border
        border.width: 1
    }

    component FieldBackground: Rectangle {
        radius: AwaTheme.radiusMd
        color: AwaTheme.surface
        border.color: AwaTheme.border
        border.width: 1
    }

    component SectionTitle: Text {
        color: AwaTheme.ink
        font.pixelSize: 17
        font.weight: Font.DemiBold
    }

    component LabelText: Text {
        color: AwaTheme.muted
        font.pixelSize: 13
    }

    Connections {
        target: downloadManager
        function onToastRequested(message) {
            showToast(message)
        }
    }

    Connections {
        target: downloadManager.downloads
        function onRowsInserted(parent, first, last) {
            downloadsRevision += 1
            pieceMapRevision += 1
            refreshSelectedDownload()
            refreshDownloadTabBadges(false)
        }
        function onRowsRemoved(parent, first, last) {
            downloadsRevision += 1
            pieceMapRevision += 1
            if (selectedDownloadId.length === 0) {
                refreshDownloadTabBadges(false)
                return
            }

            const row = downloadManager.downloads.indexOfId(selectedDownloadId)
            if (row >= 0) {
                refreshSelectedDownload()
            } else {
                selectedDownloadId = ""
                selectedDownload = ({})
            }
            refreshDownloadTabBadges(false)
        }
        function onDataChanged(topLeft, bottomRight, roles) {
            downloadsRevision += 1
            if (roleListContains(roles, downloadManager.downloads.pieceMapRole())) {
                pieceMapRevision += 1
            }
            refreshSelectedDownload()
            refreshDownloadTabBadges(false)
        }
        function onModelReset() {
            downloadsRevision += 1
            pieceMapRevision += 1
            refreshSelectedDownload()
            refreshDownloadTabBadges(false)
        }
    }

    Timer {
        id: downloadTabBadgeReadyTimer
        interval: 3000
        repeat: false
        onTriggered: {
            refreshDownloadTabBadges(true)
            downloadTabBadgesReady = true
        }
    }

    onCurrentPageChanged: markCurrentDownloadTabSeen()
    onDownloadSectionTabChanged: markCurrentDownloadTabSeen()

    Component.onCompleted: {
        selectedLanguage = settingsService.language()
        I18n.language = selectedLanguage
        completionSoundEnabled = settingsService.downloadCompletionSoundEnabled()
        refreshDownloadTabBadges(true)
        downloadTabBadgeReadyTimer.start()
    }

    FileDialog {
        id: torrentDialog
        title: I18n.tr("选择种子文件", "Select Torrent File")
        nameFilters: ["Torrent files (*.torrent)"]
        onAccepted: downloadManager.addTorrentFile(selectedFile.toString().replace("file:///", ""))
    }

    FolderDialog {
        id: folderDialog
        title: I18n.tr("选择默认保存目录", "Select Default Save Folder")
        onAccepted: {
            const path = selectedFolder.toString().replace("file:///", "")
            downloadManager.defaultSavePath = path
            settingsService.setDownloadDirectory(path)
            settingsPathInput.text = path
        }
    }

    Dialog {
        id: magnetDialog
        modal: true
        width: Math.min(620, window.width - 56)
        height: 360
        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        padding: 0
        closePolicy: Popup.CloseOnEscape
        Overlay.modal: Rectangle { color: "#660d3558" }

        background: Rectangle {
            radius: AwaTheme.radiusXl
            color: AwaTheme.surface
            border.color: AwaTheme.borderStrong
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 74
                radius: AwaTheme.radiusXl
                color: AwaTheme.surfaceSoft
                border.color: "transparent"
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: AwaTheme.radiusXl
                    color: parent.color
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 18
                    spacing: 12
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 14
                        color: AwaTheme.primary
                        Text {
                            anchors.centerIn: parent
                            text: "+"
                            color: "white"
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: I18n.tr("添加磁力链接", "Add Magnet Link"); color: AwaTheme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
                        Text { text: I18n.tr("粘贴 magnet URI，并确认保存目录", "Paste a magnet URI and confirm the save folder"); color: AwaTheme.muted; font.pixelSize: 12 }
                    }
                    AcidToolButton {
                        text: "x"
                        ToolTip.visible: hovered
                        ToolTip.text: I18n.tr("关闭", "Close")
                        onClicked: magnetDialog.close()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 24
                spacing: 14
                TextArea {
                    id: magnetInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 118
                    placeholderText: "magnet:?xt=urn:btih:..."
                    wrapMode: TextArea.WrapAnywhere
                    color: AwaTheme.ink
                    placeholderTextColor: AwaTheme.muted
                    selectedTextColor: "white"
                    selectionColor: AwaTheme.primary
                    background: FieldBackground {}
                }
                TextField {
                    id: savePathInput
                    Layout.fillWidth: true
                    text: downloadManager.defaultSavePath
                    placeholderText: I18n.tr("保存路径", "Save path")
                    color: AwaTheme.ink
                    placeholderTextColor: AwaTheme.muted
                    selectedTextColor: "white"
                    selectionColor: AwaTheme.primary
                    background: FieldBackground {}
                }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    AcidButton { text: I18n.tr("取消", "Cancel"); onClicked: magnetDialog.close() }
                    AcidButton { text: I18n.tr("添加任务", "Add Task"); tone: "primary"; onClicked: addMagnetFromDialog() }
                }
            }
        }
    }

    Dialog {
        id: removeConfirmDialog
        modal: true
        width: Math.min(460, window.width - 56)
        height: 220
        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        padding: 0
        closePolicy: Popup.CloseOnEscape
        Overlay.modal: Rectangle { color: "#660d3558" }

        background: Rectangle {
            radius: AwaTheme.radiusXl
            color: AwaTheme.surface
            border.color: AwaTheme.borderStrong
            border.width: 1
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            Text {
                Layout.fillWidth: true
                text: I18n.tr("确认移除任务", "Remove Task?")
                color: AwaTheme.ink
                font.pixelSize: 20
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: I18n.tr("任务会从列表中移除，已下载文件会保留。", "The task will be removed from the list. Downloaded files will be kept.")
                color: AwaTheme.muted
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                text: selectedDownload.name || selectedDownload.source || selectedDownloadId
                color: AwaTheme.inkSoft
                font.pixelSize: 12
                elide: Text.ElideMiddle
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                AcidButton {
                    text: I18n.tr("取消", "Cancel")
                    onClicked: removeConfirmDialog.close()
                }
                AcidButton {
                    text: I18n.tr("确认移除", "Remove")
                    tone: "danger"
                    onClicked: {
                        const id = selectedDownloadId
                        removeConfirmDialog.close()
                        if (id.length > 0) {
                            selectedDownloadId = ""
                            selectedDownload = ({})
                            downloadManager.remove(id, false)
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: toastPopup
        x: Math.round((window.width - width) / 2)
        y: 24
        width: Math.min(540, window.width - 48)
        height: 48
        modal: false
        closePolicy: Popup.NoAutoClose
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140 } }
        background: Rectangle {
            radius: AwaTheme.radiusLg
            color: "#f8fcff"
            border.color: AwaTheme.borderStrong
            border.width: 1
        }
        contentItem: RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10
            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: AwaTheme.primary
            }
            Text {
                Layout.fillWidth: true
                text: toastText
                color: AwaTheme.ink
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        Timer {
            running: toastPopup.opened
            interval: 2200
            onTriggered: toastPopup.close()
        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            for (let i = 0; i < drop.urls.length; ++i) {
                const path = drop.urls[i].toString().replace("file:///", "")
                if (path.toLowerCase().endsWith(".torrent")) {
                    downloadManager.addTorrentFile(path)
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 236
            Layout.fillHeight: true
            color: "#fafdff"
            border.width: 0

            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: 1
                color: panelDivider
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 18
                anchors.bottomMargin: 16
                spacing: 14

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 82
                        Layout.preferredHeight: 82
                        source: appLogoSource
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "AwaKurage"
                        color: AwaTheme.ink
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Text {
                        Layout.fillWidth: true
                        text: I18n.tr("BT下载器", "BT Downloader")
                        color: AwaTheme.muted
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SidebarButton { Layout.fillWidth: true; text: I18n.tr("下载任务", "Downloads"); iconText: "↓"; active: currentPage === 0; onClicked: currentPage = 0 }
                    SidebarButton { Layout.fillWidth: true; text: I18n.tr("RSS 订阅", "RSS"); iconText: "≋"; active: currentPage === 1; onClicked: currentPage = 1 }
                    SidebarButton { Layout.fillWidth: true; text: I18n.tr("远程 API", "Remote API"); iconText: "{}"; active: currentPage === 2; onClicked: currentPage = 2 }
                    SidebarButton { Layout.fillWidth: true; text: I18n.tr("设置", "Settings"); iconText: "⚙"; active: currentPage === 3; onClicked: currentPage = 3 }
                }

                Item { Layout.fillHeight: true }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 126
                    clip: true
                    Image {
                        source: appQPosterSource
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: -28
                        anchors.bottomMargin: -36
                        width: 124
                        height: 124
                        opacity: 0.36
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 5
                        Text { text: I18n.tr("本地 API", "Local API"); color: AwaTheme.ink; font.pixelSize: 12; font.weight: Font.DemiBold }
                        Text { width: parent.width; text: "127.0.0.1:" + apiServer.port; color: AwaTheme.inkSoft; font.pixelSize: 11; elide: Text.ElideRight }
                        Text { width: parent.width; text: "WebSocket: " + (apiServer.port + 1); color: AwaTheme.muted; font.pixelSize: 10; elide: Text.ElideRight }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: currentPage === 0 ? 96 : 74
                color: "#fafdff"
                border.width: 0

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: panelDivider
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: currentPage === 0 ? 6 : 2
                        Text { text: pageTitles[currentPage]; color: AwaTheme.ink; font.pixelSize: 22; font.weight: Font.DemiBold }
                        Rectangle {
                            visible: currentPage === 0
                            Layout.topMargin: 0
                            Layout.preferredHeight: 34
                            Layout.preferredWidth: 320
                            radius: AwaTheme.radiusMd
                            color: "#edf4fb"
                            border.color: "#d7e3ef"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 3
                                spacing: 0

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    radius: AwaTheme.radiusSm
                                    color: downloadSectionTab === 0 ? "#ffffff" : "transparent"
                                    border.width: downloadSectionTab === 0 ? 1 : 0
                                    border.color: "#c8d8e8"

                                    Text {
                                        anchors.centerIn: parent
                                        text: downloadTabLabel(I18n.tr("下载中", "Active"), downloadingCount, downloadingUnseenCount)
                                        color: downloadSectionTab === 0 ? AwaTheme.ink : AwaTheme.inkSoft
                                        font.pixelSize: 12
                                        font.weight: downloadSectionTab === 0 ? Font.DemiBold : Font.Medium
                                        elide: Text.ElideRight
                                    }

                                    TapHandler {
                                        onTapped: {
                                            downloadSectionTab = 0
                                            if (hasSelectedDownload && !matchesDownloadSection(selectedDownload)) {
                                                selectedDownloadId = ""
                                                selectedDownload = ({})
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    radius: AwaTheme.radiusSm
                                    color: downloadSectionTab === 1 ? "#ffffff" : "transparent"
                                    border.width: downloadSectionTab === 1 ? 1 : 0
                                    border.color: "#c8d8e8"

                                    Text {
                                        anchors.centerIn: parent
                                        text: downloadTabLabel(I18n.tr("已完成", "Completed"), completedCount, completedUnseenCount)
                                        color: downloadSectionTab === 1 ? AwaTheme.ink : AwaTheme.inkSoft
                                        font.pixelSize: 12
                                        font.weight: downloadSectionTab === 1 ? Font.DemiBold : Font.Medium
                                        elide: Text.ElideRight
                                    }

                                    TapHandler {
                                        onTapped: {
                                            downloadSectionTab = 1
                                            if (hasSelectedDownload && !matchesDownloadSection(selectedDownload)) {
                                                selectedDownloadId = ""
                                                selectedDownload = ({})
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        visible: currentPage === 0
                        spacing: 10

                        AcidButton {
                            text: I18n.tr("添加磁力", "Add Magnet")
                            tone: "primary"
                            onClicked: magnetDialog.open()
                        }
                        AcidButton {
                            text: I18n.tr("选择种子", "Open Torrent")
                            onClicked: torrentDialog.open()
                        }
                    }
                }
            }

            SplitView {
                visible: currentPage === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

                Rectangle {
                    SplitView.preferredWidth: 740
                    SplitView.minimumWidth: 560
                    color: AwaTheme.page

                    Image {
                        source: appPosterSource
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: -60
                        anchors.bottomMargin: -58
                        width: Math.min(parent.width * 0.58, parent.height * 0.86)
                        height: width
                        fillMode: Image.PreserveAspectFit
                        opacity: listView.count === 0 ? 0.82 : 0.18
                        smooth: true
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: AwaTheme.page
                        opacity: listView.count === 0 ? 0.18 : 0.68
                    }

                    ListView {
                        id: listView
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        anchors.topMargin: 12
                        anchors.bottomMargin: 18
                        spacing: 0
                        clip: true
                        model: downloadManager.downloads
                        displaced: Transition { NumberAnimation { properties: "x,y"; duration: 180; easing.type: Easing.OutCubic } }
                        add: Transition {
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 }
                            NumberAnimation { property: "y"; from: 18; duration: 180; easing.type: Easing.OutCubic }
                        }
                        delegate: Item {
                            width: ListView.view.width
                            height: matchesDownloadSection(itemData) ? 112 : 0
                            visible: height > 0
                            property var itemData: {
                                downloadsRevision
                                return downloadManager.downloads.get(index)
                            }

                            DownloadRow {
                                width: parent.width
                                height: 102
                                itemData: parent.itemData
                                rowIndex: index
                                selected: selectedDownloadId === itemData.downloadId
                                onOpenDetails: function(item) { selectDownload(item.downloadId, item) }
                                onPauseTask: function(id) { downloadManager.pause(id) }
                                onResumeTask: function(id) { downloadManager.resume(id) }
                                onOpenFolder: function(id) { downloadManager.openSavePath(id) }
                                onRemoveTask: function(id) { downloadManager.remove(id, false) }
                            }
                        }

                        Panel {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: Math.max(24, Math.round(parent.width * 0.09))
                            width: Math.min(420, parent.width - 48)
                            height: 204
                            visible: filteredDownloadCount() === 0
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 22
                                spacing: 12
                                Text { text: downloadSectionTab === 0 ? I18n.tr("暂无下载中的任务", "No Active Tasks") : I18n.tr("暂无已完成的任务", "No Completed Tasks"); color: AwaTheme.ink; font.pixelSize: 20; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                Text { Layout.fillWidth: true; text: downloadSectionTab === 0 ? I18n.tr("拖入 .torrent 文件，或添加磁力链接", "Drop a .torrent file here, or add a magnet link") : I18n.tr("完成后的任务会出现在这里", "Finished tasks will appear here"); color: AwaTheme.muted; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
                                RowLayout {
                                    visible: downloadSectionTab === 0
                                    Layout.alignment: Qt.AlignHCenter
                                    AcidButton { text: I18n.tr("添加磁力", "Add Magnet"); tone: "primary"; onClicked: magnetDialog.open() }
                                    AcidButton { text: I18n.tr("选择种子", "Open Torrent"); onClicked: torrentDialog.open() }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    id: detailPane
                    visible: hasSelectedDownload
                    SplitView.preferredWidth: 376
                    SplitView.minimumWidth: 316
                    color: "#fafdff"
                    border.width: 0

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: panelDivider
                    }

                    ScrollView {
                        id: detailScrollView
                        anchors.fill: parent
                        anchors.margins: 22
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ColumnLayout {
                            width: detailScrollView.availableWidth
                            spacing: 16
                            Text {
                                Layout.fillWidth: true
                                Layout.maximumWidth: parent.width
                                text: selectedDownload.name || I18n.tr("任务详情", "Task Details")
                                color: AwaTheme.ink
                                font.pixelSize: 19
                                font.weight: Font.DemiBold
                                wrapMode: Text.Wrap
                            }
                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 1
                            value: selectedDownload.progress || 0
                            background: Rectangle {
                                implicitHeight: 9
                                radius: 5
                                color: "#fff3e6"
                                border.color: "#ffd7a8"
                            }
                            contentItem: Item {
                                implicitHeight: 9
                                Rectangle {
                                    width: parent.width * (selectedDownload.progress || 0)
                                    height: parent.height
                                    radius: 5
                                    color: "#f97316"
                                }
                            }
                        }
                        GridLayout {
                            columns: 2
                            Layout.fillWidth: true
                            rowSpacing: 10
                            columnSpacing: 16
                            clip: true
                            LabelText { text: I18n.tr("状态", "State") }
                            Text { text: selectedDownload.stateText ? I18n.dynamic(selectedDownload.stateText) : "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("进展", "Progress") }
                            Text { text: selectedDownload.statusText ? I18n.dynamic(selectedDownload.statusText) : "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("连接", "Connection") }
                            Text { text: selectedDownload.connectionHealthText ? I18n.dynamic(selectedDownload.connectionHealthText) : "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("Peers / Seeds", "Peers / Seeds") }
                            Text { text: (selectedDownload.peerCount || 0) + " / " + (selectedDownload.seedCount || 0); color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("Tracker", "Tracker") }
                            Text { text: trackerSummary(selectedDownload.workingTrackerCount || 0, selectedDownload.trackerCount || 0); color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("DHT", "DHT") }
                            Text { text: selectedDownload.dhtStatusText ? I18n.dynamic(selectedDownload.dhtStatusText) : "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("保存", "Save Path") }
                            Text { text: selectedDownload.savePath || "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideMiddle; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("大小", "Size") }
                            Text { text: formatBytes(selectedDownload.downloadedBytes || 0) + " / " + formatBytes(selectedDownload.totalBytes || 0); color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("分块", "Pieces") }
                            Text { text: piecesSummary(selectedDownload.completedPieces || 0, selectedDownload.pieceCount || 0); color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: I18n.tr("下载", "Download") }
                            Text { text: Math.round((selectedDownload.downloadRate || 0) / 1024) + " KiB/s"; color: AwaTheme.primary; font.pixelSize: 12; font.weight: Font.DemiBold }
                            LabelText { text: I18n.tr("上传", "Upload") }
                            Text { text: Math.round((selectedDownload.uploadRate || 0) / 1024) + " KiB/s"; color: AwaTheme.ink; font.pixelSize: 12 }
                        }

                        Panel {
                            id: detailPieceBar
                            Layout.fillWidth: true
                            Layout.preferredHeight: 220
                            visible: hasSelectedDownload
                            clip: true
                            readonly property string fullPieceMap: {
                                pieceMapRevision
                                return selectedDownloadId.length > 0 ? downloadManager.downloads.pieceMapById(selectedDownloadId) : ""
                            }
                            readonly property int cellSize: 7
                            readonly property int cellGap: 2
                            readonly property int columnCount: Math.max(1, Math.floor((pieceGridViewport.width + cellGap) / (cellSize + cellGap)))
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        Layout.fillWidth: true
                                        text: I18n.tr("分块跟踪", "Piece Map")
                                        color: AwaTheme.ink
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        text: (selectedDownload.completedPieces || 0) + " / " + (selectedDownload.pieceCount || 0)
                                        color: AwaTheme.muted
                                        font.pixelSize: 12
                                    }
                                }
                                Item {
                                    id: pieceGridViewport
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 6
                                        color: "transparent"
                                        border.width: 0
                                    }

                                    Flickable {
                                        id: pieceGridFlick
                                        anchors.fill: parent
                                        clip: true
                                        flickableDirection: Flickable.VerticalFlick
                                        boundsBehavior: Flickable.StopAtBounds
                                        contentWidth: width
                                        contentHeight: pieceGridContent.height

                                        ScrollBar.vertical: ScrollBar {
                                            policy: ScrollBar.AlwaysOff
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.NoButton
                                            onWheel: function(wheel) {
                                                if (pieceGridFlick.contentHeight <= pieceGridFlick.height) {
                                                    wheel.accepted = true
                                                    return
                                                }

                                                const nextY = pieceGridFlick.contentY - wheel.angleDelta.y
                                                pieceGridFlick.contentY = Math.max(0,
                                                    Math.min(pieceGridFlick.contentHeight - pieceGridFlick.height, nextY))
                                                wheel.accepted = true
                                            }
                                        }

                                        Item {
                                            id: pieceGridContent
                                            width: pieceGridFlick.width
                                            height: Math.max(pieceGridFlick.height,
                                                detailPieceBar.fullPieceMap.length > 0
                                                    ? Math.ceil(detailPieceBar.fullPieceMap.length / detailPieceBar.columnCount)
                                                        * (detailPieceBar.cellSize + detailPieceBar.cellGap) + detailPieceBar.cellGap
                                                    : pieceGridFlick.height)
                                            visible: detailPieceBar.fullPieceMap.length > 0

                                            Canvas {
                                                id: pieceCanvas
                                                anchors.fill: parent
                                                onPaint: {
                                                    const ctx = getContext("2d")
                                                    ctx.clearRect(0, 0, width, height)
                                                    const map = detailPieceBar.fullPieceMap
                                                    const cell = detailPieceBar.cellSize
                                                    const gap = detailPieceBar.cellGap
                                                    const columns = detailPieceBar.columnCount
                                                    for (let i = 0; i < map.length; ++i) {
                                                        const x = (i % columns) * (cell + gap)
                                                        const y = Math.floor(i / columns) * (cell + gap)
                                                        const state = map.charAt(i)
                                                        ctx.fillStyle = state === "1" ? "#a9eec1" : state === "2" ? "#fff2a8" : state === "3" ? "#fecaca" : "#dceeff"
                                                        ctx.fillRect(x, y, cell, cell)
                                                    }
                                                }
                                                onWidthChanged: requestPaint()
                                                onHeightChanged: requestPaint()
                                            }

                                            Connections {
                                                target: detailPieceBar
                                                function onFullPieceMapChanged() { pieceCanvas.requestPaint() }
                                                function onColumnCountChanged() { pieceCanvas.requestPaint() }
                                            }
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        visible: detailPieceBar.fullPieceMap.length === 0
                                        text: I18n.tr("等待分块数据", "Waiting for piece data")
                                        color: AwaTheme.muted
                                        font.pixelSize: 11
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#a9eec1"; border.color: "#6fcf8d" }
                                    Text { text: I18n.tr("已完成", "Done"); color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#fff2a8"; border.color: "#e5c84c" }
                                    Text { text: I18n.tr("下载中", "Downloading"); color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#fecaca"; border.color: "#ef4444" }
                                    Text { text: I18n.tr("不良块", "Poor"); color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#dceeff"; border.color: AwaTheme.border }
                                    Text { text: I18n.tr("未完成", "Missing"); color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Item { Layout.fillWidth: true }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            AcidButton {
                                text: I18n.tr("暂停", "Pause")
                                Layout.fillWidth: true
                                enabled: selectedCanPause
                                onClicked: downloadManager.pause(selectedDownloadId)
                            }
                            AcidButton {
                                text: I18n.tr("继续", "Resume")
                                tone: "primary"
                                Layout.fillWidth: true
                                enabled: selectedCanResume
                                onClicked: downloadManager.resume(selectedDownloadId)
                            }
                        }
                        AcidButton {
                            Layout.fillWidth: true
                            text: I18n.tr("移除任务", "Remove Task")
                            tone: "danger"
                            enabled: selectedDownloadId.length > 0
                            onClicked: removeConfirmDialog.open()
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AwaTheme.border }

                        Text { Layout.fillWidth: true; text: I18n.tr("RSS 自动下载默认关闭", "RSS auto-download is off by default"); color: AwaTheme.muted; font.pixelSize: 12; elide: Text.ElideRight }
                        Switch {
                            Layout.fillWidth: true
                            text: I18n.tr("启用 RSS 自动匹配", "Enable RSS Auto Match")
                            checked: rssService.autoDownloadEnabled()
                            onToggled: rssService.setAutoDownloadEnabled(checked)
                        }
                        Item { Layout.preferredHeight: 1 }
                    }
                    }

                    AcidToolButton {
                        visible: hasSelectedDownload
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 18
                        anchors.rightMargin: 18
                        implicitWidth: 34
                        implicitHeight: 34
                        z: 2
                        tone: "ghost"
                        text: "X"
                        ToolTip.visible: hovered
                        ToolTip.text: I18n.tr("关闭详情", "Close Details")
                        onClicked: {
                            selectedDownloadId = ""
                            selectedDownload = ({})
                        }
                    }
                }
            }

            Rectangle {
                visible: currentPage === 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: AwaTheme.page

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: rssAddContent.implicitHeight + 38
                        Layout.minimumHeight: 264

                        ColumnLayout {
                            id: rssAddContent
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 13
                            SectionTitle { text: I18n.tr("添加 RSS 源", "Add RSS Feed") }
                            Text {
                                Layout.fillWidth: true
                                text: I18n.tr("已内置默认源：FOSS Torrents、Academic Torrents", "Built-in defaults: FOSS Torrents, Academic Torrents")
                                color: AwaTheme.muted
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            TextField { id: rssTitleInput; Layout.fillWidth: true; placeholderText: I18n.tr("订阅名称", "Feed name"); color: AwaTheme.ink; placeholderTextColor: AwaTheme.muted; background: FieldBackground {} }
                            TextField { id: rssUrlInput; Layout.fillWidth: true; placeholderText: "https://example.com/feed.xml"; color: AwaTheme.ink; placeholderTextColor: AwaTheme.muted; background: FieldBackground {} }
                            RowLayout {
                                Layout.fillWidth: true
                                AcidButton {
                                    text: I18n.tr("添加订阅", "Add Feed")
                                    tone: "primary"
                                    onClicked: {
                                        rssService.addSubscription(rssTitleInput.text, rssUrlInput.text)
                                        rssTitleInput.clear()
                                        rssUrlInput.clear()
                                    }
                                }
                                AcidButton { text: I18n.tr("刷新全部", "Refresh All"); onClicked: rssService.refreshAll() }
                                Switch {
                                    text: I18n.tr("自动匹配", "Auto Match")
                                    checked: rssService.autoDownloadEnabled()
                                    onToggled: rssService.setAutoDownloadEnabled(checked)
                                }
                            }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 136
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 10
                            SectionTitle { text: I18n.tr("RSS 状态", "RSS Status") }
                            Text { Layout.fillWidth: true; text: I18n.tr("订阅数量：", "Subscriptions: ") + rssService.subscriptionCount; color: AwaTheme.inkSoft; font.pixelSize: 13 }
                            Text { Layout.fillWidth: true; text: rssService.lastStatus.length > 0 ? I18n.dynamic(rssService.lastStatus) : I18n.tr("尚未刷新", "Not refreshed yet"); color: AwaTheme.muted; font.pixelSize: 13; elide: Text.ElideRight }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                visible: currentPage === 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: AwaTheme.page

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 268

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            columns: 2
                            rowSpacing: 13
                            columnSpacing: 18

                            LabelText { text: I18n.tr("监听状态", "Listening") }
                            Text { text: apiServer.listening ? I18n.tr("运行中", "Running") : I18n.tr("已停止", "Stopped"); color: apiServer.listening ? AwaTheme.success : AwaTheme.danger; font.pixelSize: 13; font.weight: Font.DemiBold }
                            LabelText { text: I18n.tr("HTTP", "HTTP") }
                            Text { Layout.fillWidth: true; text: "http://127.0.0.1:" + apiServer.port + "/api/v1/downloads"; color: AwaTheme.ink; font.pixelSize: 13; elide: Text.ElideRight }
                            LabelText { text: I18n.tr("WebSocket", "WebSocket") }
                            Text { Layout.fillWidth: true; text: "ws://127.0.0.1:" + (apiServer.port + 1) + "/api/v1/events"; color: AwaTheme.ink; font.pixelSize: 13; elide: Text.ElideRight }
                            LabelText { text: I18n.tr("Token", "Token") }
                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 76
                                readOnly: true
                                wrapMode: TextArea.WrapAnywhere
                                text: apiServer.token
                                color: AwaTheme.ink
                                selectedTextColor: "white"
                                selectionColor: AwaTheme.primary
                                background: FieldBackground {}
                            }
                            Item {}
                            RowLayout {
                                AcidButton { text: I18n.tr("启动", "Start"); tone: "primary"; enabled: !apiServer.listening; onClicked: apiServer.start(settingsService.apiPort()) }
                                AcidButton { text: I18n.tr("停止", "Stop"); tone: "danger"; enabled: apiServer.listening; onClicked: apiServer.stop() }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: I18n.tr("请求需携带 Authorization: Bearer <Token>。API 默认只监听 127.0.0.1。", "Requests must include Authorization: Bearer <Token>. The API listens on 127.0.0.1 by default.")
                        color: AwaTheme.muted
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                visible: currentPage === 3
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: AwaTheme.page

                ScrollView {
                    id: settingsScrollView
                    anchors.fill: parent
                    clip: true

                    ColumnLayout {
                        width: Math.max(parent.width - 48, 620)
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 16

                        Item { Layout.preferredHeight: 8 }

                        Panel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: appSettingsContent.implicitHeight + 40

                            ColumnLayout {
                                id: appSettingsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14
                                SectionTitle { text: I18n.tr("应用与提醒", "App and Alerts") }
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 12
                                    columnSpacing: 18
                                    LabelText { text: I18n.tr("界面语言", "Language") }
                                    ComboBox {
                                        id: languageBox
                                        Layout.fillWidth: true
                                        textRole: "text"
                                        valueRole: "value"
                                        model: [
                                            { "text": I18n.tr("简体中文", "Simplified Chinese"), "value": "zh_CN" },
                                            { "text": I18n.tr("英语", "English"), "value": "en_US" },
                                            { "text": I18n.tr("日语", "Japanese"), "value": "ja_JP" }
                                        ]
                                        currentIndex: {
                                            for (let i = 0; i < model.length; ++i) {
                                                if (model[i].value === selectedLanguage) {
                                                    return i
                                                }
                                            }
                                            return 0
                                        }
                                        onActivated: function(index) {
                                            const nextLanguage = currentValue
                                            if (selectedLanguage !== nextLanguage) {
                                                selectedLanguage = nextLanguage
                                                I18n.language = nextLanguage
                                                settingsService.setLanguage(nextLanguage)
                                                showToast(I18n.tr("语言已切换", "Language changed"))
                                            }
                                        }
                                    }
                                    LabelText { text: I18n.tr("完成声音提示", "Completion Sound") }
                                    Switch {
                                        Layout.fillWidth: true
                                        checked: completionSoundEnabled
                                        text: checked ? I18n.tr("下载完成时播放提示音", "Play sound when downloads finish") : I18n.tr("下载完成时保持安静", "Stay quiet when downloads finish")
                                        onToggled: {
                                            completionSoundEnabled = checked
                                            settingsService.setDownloadCompletionSoundEnabled(checked)
                                        }
                                    }
                                }
                            }
                        }

                        Panel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: downloadSettingsContent.implicitHeight + 40

                            ColumnLayout {
                                id: downloadSettingsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14
                                SectionTitle { text: I18n.tr("下载设置", "Download Settings") }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: settingsPathInput
                                        Layout.fillWidth: true
                                        text: downloadManager.defaultSavePath
                                        placeholderText: I18n.tr("默认保存目录", "Default save folder")
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    AcidButton { text: I18n.tr("选择", "Browse"); onClicked: folderDialog.open() }
                                    AcidButton {
                                        text: I18n.tr("保存", "Save")
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.defaultSavePath = settingsPathInput.text
                                            settingsService.setDownloadDirectory(settingsPathInput.text)
                                            showToast(I18n.tr("设置已保存", "Settings saved"))
                                        }
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: dlLimitInput
                                        Layout.fillWidth: true
                                        text: downloadManager.downloadLimitKiB.toString()
                                        placeholderText: I18n.tr("下载限速 KiB/s，0 表示不限", "Download limit KiB/s, 0 for unlimited")
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    TextField {
                                        id: ulLimitInput
                                        Layout.fillWidth: true
                                        text: downloadManager.uploadLimitKiB.toString()
                                        placeholderText: I18n.tr("上传限速 KiB/s，0 表示不限", "Upload limit KiB/s, 0 for unlimited")
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    AcidButton {
                                        text: I18n.tr("应用限速", "Apply Limits")
                                        tone: "primary"
                                        onClicked: {
                                            const dlLimit = positiveInt(dlLimitInput.text, 0)
                                            const ulLimit = positiveInt(ulLimitInput.text, 0)
                                            downloadManager.setSpeedLimits(dlLimit, ulLimit)
                                            settingsService.setDownloadLimitKiB(dlLimit)
                                            settingsService.setUploadLimitKiB(ulLimit)
                                            showToast(I18n.tr("限速已应用", "Speed limits applied"))
                                        }
                                    }
                                }
                            }
                        }

                        Panel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: strategySettingsContent.implicitHeight + 40

                            ColumnLayout {
                                id: strategySettingsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14
                                SectionTitle { text: I18n.tr("块选择与上传博弈", "Piece Selection and Choking") }
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 10
                                    columnSpacing: 14
                                    LabelText { text: I18n.tr("块选择策略", "Piece Strategy") }
                                    Text { Layout.fillWidth: true; text: I18n.tr("稀有块优先", "Rarest First"); color: AwaTheme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                                    LabelText { text: I18n.tr("上传博弈策略", "Choking Strategy") }
                                    ComboBox {
                                        id: chokingAlgorithmBox
                                        Layout.fillWidth: true
                                        model: [I18n.tr("互惠固定槽位", "Fixed Slots"), I18n.tr("互惠速率自适应", "Rate Based")]
                                        currentIndex: downloadManager.chokingAlgorithm
                                    }
                                    LabelText { text: I18n.tr("做种时策略", "Seeding Strategy") }
                                    ComboBox {
                                        id: seedChokingAlgorithmBox
                                        Layout.fillWidth: true
                                        model: [I18n.tr("轮询", "Round Robin"), I18n.tr("最快上传优先", "Fastest Upload"), I18n.tr("反吸血博弈", "Anti-leech")]
                                        currentIndex: downloadManager.seedChokingAlgorithm
                                    }
                                    LabelText { text: I18n.tr("完成后做种", "Seed After Complete") }
                                    Switch {
                                        id: seedOnCompletionSwitch
                                        Layout.fillWidth: true
                                        checked: downloadManager.seedOnCompletionEnabled
                                        text: checked ? I18n.tr("继续做种", "Keep Seeding") : I18n.tr("完成即停止", "Stop When Done")
                                    }
                                    LabelText { text: I18n.tr("并行下载任务", "Active Downloads") }
                                    SpinBox { id: activeDownloadsSpin; Layout.fillWidth: true; from: 1; to: 200; value: downloadManager.maxActiveDownloads }
                                    LabelText { text: I18n.tr("动态并行块", "Dynamic Blocks") }
                                    Switch {
                                        id: dynamicBlockSwitch
                                        Layout.fillWidth: true
                                        checked: downloadManager.dynamicBlockTuningEnabled
                                        text: checked ? I18n.tr("自动调整", "Auto Tune") : I18n.tr("固定队列", "Fixed Queue")
                                    }
                                    LabelText { text: I18n.tr("上传槽位", "Upload Slots") }
                                    SpinBox { id: uploadSlotsSpin; Layout.fillWidth: true; from: 1; to: 200; value: downloadManager.uploadSlots }
                                    LabelText { text: I18n.tr("乐观解阻塞槽位", "Optimistic Slots") }
                                    SpinBox { id: optimisticSlotsSpin; Layout.fillWidth: true; from: 0; to: 10; value: downloadManager.optimisticSlots }
                                    Item {}
                                    AcidButton {
                                        text: I18n.tr("应用策略", "Apply Strategy")
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.setChokingStrategy(chokingAlgorithmBox.currentIndex, seedChokingAlgorithmBox.currentIndex, uploadSlotsSpin.value, optimisticSlotsSpin.value)
                                            downloadManager.setMaxActiveDownloads(activeDownloadsSpin.value)
                                            downloadManager.setDynamicBlockTuningEnabled(dynamicBlockSwitch.checked)
                                            downloadManager.setSeedOnCompletionEnabled(seedOnCompletionSwitch.checked)
                                            settingsService.setChokingAlgorithm(chokingAlgorithmBox.currentIndex)
                                            settingsService.setSeedChokingAlgorithm(seedChokingAlgorithmBox.currentIndex)
                                            settingsService.setMaxActiveDownloads(activeDownloadsSpin.value)
                                            settingsService.setDynamicBlockTuningEnabled(dynamicBlockSwitch.checked)
                                            settingsService.setSeedOnCompletionEnabled(seedOnCompletionSwitch.checked)
                                            settingsService.setUploadSlots(uploadSlotsSpin.value)
                                            settingsService.setOptimisticSlots(optimisticSlotsSpin.value)
                                            showToast(I18n.tr("稀有块优先与上传博弈策略已应用", "Piece selection and choking strategy applied"))
                                        }
                                    }
                                }
                            }
                        }

                        Panel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: trackerSettingsContent.implicitHeight + 40

                            ColumnLayout {
                                id: trackerSettingsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14
                                SectionTitle { text: I18n.tr("Trackers", "Trackers") }
                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 188
                                    clip: true
                                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                                    background: FieldBackground {}

                                    TextArea {
                                        id: trackerUrlsInput
                                        width: parent.availableWidth
                                        text: downloadManager.trackerUrlsText
                                        placeholderText: "udp://tracker.example.org:6969/announce"
                                        wrapMode: TextArea.WrapAnywhere
                                        selectByMouse: true
                                        activeFocusOnPress: true
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        selectedTextColor: "white"
                                        selectionColor: AwaTheme.primary
                                        background: null
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        Layout.fillWidth: true
                                        text: I18n.tr("每行一个 Tracker。所有 Tracker 会并行 announce。", "One tracker per line. All trackers are announced in parallel.")
                                        color: AwaTheme.muted
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }
                                    AcidButton {
                                        text: I18n.tr("默认", "Defaults")
                                        onClicked: trackerUrlsInput.text = settingsService.defaultTrackerUrlsText()
                                    }
                                    AcidButton {
                                        text: I18n.tr("应用", "Apply")
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.trackerUrlsText = trackerUrlsInput.text
                                            settingsService.setTrackerUrlsText(downloadManager.trackerUrlsText)
                                            trackerUrlsInput.text = downloadManager.trackerUrlsText
                                            showToast(I18n.tr("Trackers 已保存", "Trackers saved"))
                                        }
                                    }
                                }
                            }
                        }

                        Panel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: serviceSettingsContent.implicitHeight + 40

                            ColumnLayout {
                                id: serviceSettingsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14
                                SectionTitle { text: I18n.tr("后台服务", "Background Services") }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: apiPortInput
                                        Layout.fillWidth: true
                                        text: settingsService.apiPort().toString()
                                        placeholderText: I18n.tr("API 端口", "API port")
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    Switch {
                                        id: apiAutoStartSwitch
                                        text: I18n.tr("启动本地 API", "Start Local API")
                                        checked: settingsService.startApiAutomatically()
                                    }
                                    AcidButton {
                                        text: I18n.tr("保存 API", "Save API")
                                        tone: "primary"
                                        onClicked: {
                                            settingsService.setApiPort(positiveInt(apiPortInput.text, 18777))
                                            settingsService.setStartApiAutomatically(apiAutoStartSwitch.checked)
                                            showToast(I18n.tr("API 设置已保存，重启后生效", "API settings saved; restart to apply"))
                                        }
                                    }
                                }
                                Switch {
                                    text: I18n.tr("启动 RSS 自动匹配", "Start RSS Auto Match")
                                    checked: settingsService.startRssAutomatically()
                                    onToggled: {
                                        settingsService.setStartRssAutomatically(checked)
                                        rssService.setAutoDownloadEnabled(checked)
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                        Item { Layout.preferredHeight: 8 }
                    }
                }
            }
        }
    }
}
