import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window
    width: 1180
    height: 760
    minimumWidth: 920
    minimumHeight: 620
    visible: true
    title: "AwaKurageDownloader"
    color: AwaTheme.page

    property string selectedDownloadId: ""
    property var selectedDownload: ({})
    property string toastText: ""
    property int currentPage: 0
    property int downloadSectionTab: 0
    property int downloadsRevision: 0
    property int pieceMapRevision: 0
    property int downloadingCount: 0
    property int completedCount: 0
    property int downloadingUnseenCount: 0
    property int completedUnseenCount: 0
    property var downloadTabSnapshot: ({})
    readonly property bool hasSelectedDownload: selectedDownloadId.length > 0
    readonly property int selectedState: selectedDownload.state === undefined ? -1 : selectedDownload.state
    readonly property string selectedStateText: selectedDownload.stateText || ""
    readonly property bool selectedIsPaused: selectedState === 3 || selectedState === 7
    readonly property bool selectedIsTerminal: selectedState === 5 || selectedState === 6
    readonly property bool selectedCanPause: hasSelectedDownload && !selectedIsPaused && !selectedIsTerminal
    readonly property bool selectedCanResume: hasSelectedDownload && selectedIsPaused
    readonly property var pageTitles: ["下载任务", "RSS 订阅", "远程 API", "设置"]

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
        toastText = message
        toastPopup.open()
    }

    function isCompletedState(state) {
        return state === 4 || state === 5 || state === 7
    }

    function matchesDownloadSection(item) {
        if (!item || item.state === undefined) {
            return false
        }
        return downloadSectionTab === 0 ? !isCompletedState(item.state) : isCompletedState(item.state)
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
        return isCompletedState(item.state) ? 1 : 0
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

            const previousSection = downloadTabSnapshot[item.downloadId]
            if (!initialLoad && section >= 0 && previousSection !== section) {
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

    onCurrentPageChanged: markCurrentDownloadTabSeen()
    onDownloadSectionTabChanged: markCurrentDownloadTabSeen()

    Component.onCompleted: refreshDownloadTabBadges(true)

    FileDialog {
        id: torrentDialog
        title: "选择种子文件"
        nameFilters: ["Torrent files (*.torrent)"]
        onAccepted: downloadManager.addTorrentFile(selectedFile.toString().replace("file:///", ""))
    }

    FolderDialog {
        id: folderDialog
        title: "选择默认保存目录"
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
                        Text { text: "添加磁力链接"; color: AwaTheme.ink; font.pixelSize: 20; font.weight: Font.DemiBold }
                        Text { text: "粘贴 magnet URI，并确认保存目录"; color: AwaTheme.muted; font.pixelSize: 12 }
                    }
                    AcidToolButton {
                        text: "x"
                        ToolTip.visible: hovered
                        ToolTip.text: "关闭"
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
                    placeholderText: "保存路径"
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
                    AcidButton { text: "取消"; onClicked: magnetDialog.close() }
                    AcidButton { text: "添加任务"; tone: "primary"; onClicked: addMagnetFromDialog() }
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
            Layout.preferredWidth: 286
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
                anchors.leftMargin: 22
                anchors.rightMargin: 22
                anchors.topMargin: 24
                anchors.bottomMargin: 20
                spacing: 18

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 108
                        Layout.preferredHeight: 108
                        source: appLogoSource
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "AwaKurage"
                        color: AwaTheme.ink
                        font.pixelSize: 23
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "BT下载器"
                        color: AwaTheme.muted
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    SidebarButton { Layout.fillWidth: true; text: "下载任务"; iconText: "↓"; active: currentPage === 0; onClicked: currentPage = 0 }
                    SidebarButton { Layout.fillWidth: true; text: "RSS 订阅"; iconText: "≋"; active: currentPage === 1; onClicked: currentPage = 1 }
                    SidebarButton { Layout.fillWidth: true; text: "远程 API"; iconText: "{}"; active: currentPage === 2; onClicked: currentPage = 2 }
                    SidebarButton { Layout.fillWidth: true; text: "设置"; iconText: "⚙"; active: currentPage === 3; onClicked: currentPage = 3 }
                }

                Item { Layout.fillHeight: true }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 158
                    clip: true
                    Image {
                        source: appQPosterSource
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: -34
                        anchors.bottomMargin: -42
                        width: 150
                        height: 150
                        opacity: 0.42
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 7
                        Text { text: "本地 API"; color: AwaTheme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                        Text { width: parent.width; text: "127.0.0.1:" + apiServer.port; color: AwaTheme.inkSoft; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { width: parent.width; text: "WebSocket: " + (apiServer.port + 1); color: AwaTheme.muted; font.pixelSize: 11; elide: Text.ElideRight }
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
                Layout.preferredHeight: currentPage === 0 ? 122 : 86
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
                    anchors.leftMargin: 28
                    anchors.rightMargin: 28
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: currentPage === 0 ? 8 : 3
                        Text { text: pageTitles[currentPage]; color: AwaTheme.ink; font.pixelSize: 25; font.weight: Font.DemiBold }
                        Rectangle {
                            visible: currentPage === 0
                            Layout.topMargin: 0
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 360
                            radius: AwaTheme.radiusMd
                            color: "#edf4fb"
                            border.color: "#d7e3ef"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 0

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    radius: AwaTheme.radiusSm
                                    color: downloadSectionTab === 0 ? "#ffffff" : "transparent"
                                    border.width: downloadSectionTab === 0 ? 1 : 0
                                    border.color: "#c8d8e8"

                                    Text {
                                        anchors.centerIn: parent
                                        text: downloadTabLabel("下载中", downloadingCount, downloadingUnseenCount)
                                        color: downloadSectionTab === 0 ? AwaTheme.ink : AwaTheme.inkSoft
                                        font.pixelSize: 13
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
                                    Layout.preferredHeight: 32
                                    radius: AwaTheme.radiusSm
                                    color: downloadSectionTab === 1 ? "#ffffff" : "transparent"
                                    border.width: downloadSectionTab === 1 ? 1 : 0
                                    border.color: "#c8d8e8"

                                    Text {
                                        anchors.centerIn: parent
                                        text: downloadTabLabel("已完成", completedCount, completedUnseenCount)
                                        color: downloadSectionTab === 1 ? AwaTheme.ink : AwaTheme.inkSoft
                                        font.pixelSize: 13
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
                        spacing: 12

                        AcidButton {
                            text: "添加磁力"
                            tone: "primary"
                            onClicked: magnetDialog.open()
                        }
                        AcidButton {
                            text: "选择种子"
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
                        anchors.margins: 22
                        spacing: 12
                        clip: true
                        model: downloadManager.downloads
                        displaced: Transition { NumberAnimation { properties: "x,y"; duration: 180; easing.type: Easing.OutCubic } }
                        add: Transition {
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 }
                            NumberAnimation { property: "y"; from: 18; duration: 180; easing.type: Easing.OutCubic }
                        }
                        delegate: DownloadRow {
                            width: ListView.view.width
                            itemData: {
                                downloadsRevision
                                return downloadManager.downloads.get(index)
                            }
                            visible: matchesDownloadSection(itemData)
                            height: visible ? 118 : 0
                            rowIndex: index
                            selected: selectedDownloadId === itemData.downloadId
                            onOpenDetails: function(item) { selectDownload(item.downloadId, item) }
                            onPauseTask: function(id) { downloadManager.pause(id) }
                            onResumeTask: function(id) { downloadManager.resume(id) }
                            onOpenFolder: function(id) { downloadManager.openSavePath(id) }
                            onRemoveTask: function(id) { downloadManager.remove(id, false) }
                        }

                        Panel {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: Math.max(28, Math.round(parent.width * 0.1))
                            width: Math.min(460, parent.width - 56)
                            height: 236
                            visible: filteredDownloadCount() === 0
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 24
                                spacing: 14
                                Text { text: downloadSectionTab === 0 ? "暂无下载中的任务" : "暂无已完成的任务"; color: AwaTheme.ink; font.pixelSize: 24; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                Text { Layout.fillWidth: true; text: downloadSectionTab === 0 ? "拖入 .torrent 文件，或添加磁力链接" : "完成后的任务会出现在这里"; color: AwaTheme.muted; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap }
                                RowLayout {
                                    visible: downloadSectionTab === 0
                                    Layout.alignment: Qt.AlignHCenter
                                    AcidButton { text: "添加磁力"; tone: "primary"; onClicked: magnetDialog.open() }
                                    AcidButton { text: "选择种子"; onClicked: torrentDialog.open() }
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
                        anchors.topMargin: 64
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ColumnLayout {
                            width: detailScrollView.availableWidth
                            spacing: 16
                            Text {
                                Layout.fillWidth: true
                                Layout.maximumWidth: parent.width
                                text: selectedDownload.name || "任务详情"
                                color: AwaTheme.ink
                                font.pixelSize: 19
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
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
                            LabelText { text: "状态" }
                            Text { text: selectedDownload.stateText || "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: "进展" }
                            Text { text: selectedDownload.statusText || "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: "保存" }
                            Text { text: selectedDownload.savePath || "-"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideMiddle; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: "大小" }
                            Text { text: formatBytes(selectedDownload.downloadedBytes || 0) + " / " + formatBytes(selectedDownload.totalBytes || 0); color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: "分块" }
                            Text { text: (selectedDownload.completedPieces || 0) + " / " + (selectedDownload.pieceCount || 0) + " pieces"; color: AwaTheme.ink; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 238 }
                            LabelText { text: "下载" }
                            Text { text: Math.round((selectedDownload.downloadRate || 0) / 1024) + " KiB/s"; color: AwaTheme.primary; font.pixelSize: 12; font.weight: Font.DemiBold }
                            LabelText { text: "上传" }
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
                                        text: "分块跟踪"
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
                                        color: AwaTheme.primaryPale
                                        border.color: AwaTheme.border
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
                                                        ctx.fillStyle = state === "1" ? "#a9eec1" : state === "2" ? "#fff2a8" : "#dceeff"
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
                                        text: "等待分块数据"
                                        color: AwaTheme.muted
                                        font.pixelSize: 11
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#a9eec1"; border.color: "#6fcf8d" }
                                    Text { text: "已完成"; color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#fff2a8"; border.color: "#e5c84c" }
                                    Text { text: "下载中"; color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: "#dceeff"; border.color: AwaTheme.border }
                                    Text { text: "未完成"; color: AwaTheme.inkSoft; font.pixelSize: 11 }
                                    Item { Layout.fillWidth: true }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            AcidButton {
                                text: "暂停"
                                Layout.fillWidth: true
                                enabled: selectedCanPause
                                onClicked: downloadManager.pause(selectedDownloadId)
                            }
                            AcidButton {
                                text: "继续"
                                tone: "primary"
                                Layout.fillWidth: true
                                enabled: selectedCanResume
                                onClicked: downloadManager.resume(selectedDownloadId)
                            }
                        }
                        AcidButton {
                            Layout.fillWidth: true
                            text: "移除任务"
                            tone: "danger"
                            enabled: selectedDownloadId.length > 0
                            onClicked: downloadManager.remove(selectedDownloadId, false)
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AwaTheme.border }

                        Text { Layout.fillWidth: true; text: "RSS 自动下载默认关闭"; color: AwaTheme.muted; font.pixelSize: 12; elide: Text.ElideRight }
                        Switch {
                            Layout.fillWidth: true
                            text: "启用 RSS 自动匹配"
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
                        text: "X"
                        ToolTip.visible: hovered
                        ToolTip.text: "关闭详情"
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
                            SectionTitle { text: "添加 RSS 源" }
                            Text {
                                Layout.fillWidth: true
                                text: "已内置默认源：FOSS Torrents、Academic Torrents"
                                color: AwaTheme.muted
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            TextField { id: rssTitleInput; Layout.fillWidth: true; placeholderText: "订阅名称"; color: AwaTheme.ink; placeholderTextColor: AwaTheme.muted; background: FieldBackground {} }
                            TextField { id: rssUrlInput; Layout.fillWidth: true; placeholderText: "https://example.com/feed.xml"; color: AwaTheme.ink; placeholderTextColor: AwaTheme.muted; background: FieldBackground {} }
                            RowLayout {
                                Layout.fillWidth: true
                                AcidButton {
                                    text: "添加订阅"
                                    tone: "primary"
                                    onClicked: {
                                        rssService.addSubscription(rssTitleInput.text, rssUrlInput.text)
                                        rssTitleInput.clear()
                                        rssUrlInput.clear()
                                    }
                                }
                                AcidButton { text: "刷新全部"; onClicked: rssService.refreshAll() }
                                Switch {
                                    text: "自动匹配"
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
                            SectionTitle { text: "RSS 状态" }
                            Text { Layout.fillWidth: true; text: "订阅数量：" + rssService.subscriptionCount; color: AwaTheme.inkSoft; font.pixelSize: 13 }
                            Text { Layout.fillWidth: true; text: rssService.lastStatus.length > 0 ? rssService.lastStatus : "尚未刷新"; color: AwaTheme.muted; font.pixelSize: 13; elide: Text.ElideRight }
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

                            LabelText { text: "监听状态" }
                            Text { text: apiServer.listening ? "运行中" : "已停止"; color: apiServer.listening ? AwaTheme.success : AwaTheme.danger; font.pixelSize: 13; font.weight: Font.DemiBold }
                            LabelText { text: "HTTP" }
                            Text { Layout.fillWidth: true; text: "http://127.0.0.1:" + apiServer.port + "/api/v1/downloads"; color: AwaTheme.ink; font.pixelSize: 13; elide: Text.ElideRight }
                            LabelText { text: "WebSocket" }
                            Text { Layout.fillWidth: true; text: "ws://127.0.0.1:" + (apiServer.port + 1) + "/api/v1/events"; color: AwaTheme.ink; font.pixelSize: 13; elide: Text.ElideRight }
                            LabelText { text: "Token" }
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
                                AcidButton { text: "启动"; tone: "primary"; enabled: !apiServer.listening; onClicked: apiServer.start(settingsService.apiPort()) }
                                AcidButton { text: "停止"; tone: "danger"; enabled: apiServer.listening; onClicked: apiServer.stop() }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "请求需携带 Authorization: Bearer <Token>。API 默认只监听 127.0.0.1。"
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
                            Layout.preferredHeight: settingsContent.implicitHeight + 40

                            ColumnLayout {
                                id: settingsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14
                                SectionTitle { text: "下载设置" }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: settingsPathInput
                                        Layout.fillWidth: true
                                        text: downloadManager.defaultSavePath
                                        placeholderText: "默认保存目录"
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    AcidButton { text: "选择"; onClicked: folderDialog.open() }
                                    AcidButton {
                                        text: "保存"
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.defaultSavePath = settingsPathInput.text
                                            settingsService.setDownloadDirectory(settingsPathInput.text)
                                            showToast("设置已保存")
                                        }
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: dlLimitInput
                                        Layout.fillWidth: true
                                        text: downloadManager.downloadLimitKiB.toString()
                                        placeholderText: "下载限速 KiB/s，0 表示不限"
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    TextField {
                                        id: ulLimitInput
                                        Layout.fillWidth: true
                                        text: downloadManager.uploadLimitKiB.toString()
                                        placeholderText: "上传限速 KiB/s，0 表示不限"
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    AcidButton {
                                        text: "应用限速"
                                        tone: "primary"
                                        onClicked: {
                                            const dlLimit = positiveInt(dlLimitInput.text, 0)
                                            const ulLimit = positiveInt(ulLimitInput.text, 0)
                                            downloadManager.setSpeedLimits(dlLimit, ulLimit)
                                            settingsService.setDownloadLimitKiB(dlLimit)
                                            settingsService.setUploadLimitKiB(ulLimit)
                                        }
                                    }
                                }
                                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AwaTheme.border }
                                Text { text: "块选择与上传博弈"; color: AwaTheme.ink; font.pixelSize: 15; font.weight: Font.DemiBold }
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 10
                                    columnSpacing: 14
                                    LabelText { text: "块选择策略" }
                                    Text { Layout.fillWidth: true; text: "稀有块优先"; color: AwaTheme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                                    LabelText { text: "上传博弈策略" }
                                    ComboBox {
                                        id: chokingAlgorithmBox
                                        Layout.fillWidth: true
                                        model: ["互惠固定槽位", "互惠速率自适应"]
                                        currentIndex: downloadManager.chokingAlgorithm
                                    }
                                    LabelText { text: "做种时策略" }
                                    ComboBox {
                                        id: seedChokingAlgorithmBox
                                        Layout.fillWidth: true
                                        model: ["轮询", "最快上传优先", "反吸血博弈"]
                                        currentIndex: downloadManager.seedChokingAlgorithm
                                    }
                                    LabelText { text: "完成后做种" }
                                    Switch {
                                        id: seedOnCompletionSwitch
                                        Layout.fillWidth: true
                                        checked: downloadManager.seedOnCompletionEnabled
                                        text: checked ? "继续做种" : "完成即停止"
                                    }
                                    LabelText { text: "上传槽位" }
                                    SpinBox { id: uploadSlotsSpin; Layout.fillWidth: true; from: 1; to: 200; value: downloadManager.uploadSlots }
                                    LabelText { text: "乐观解阻塞槽位" }
                                    SpinBox { id: optimisticSlotsSpin; Layout.fillWidth: true; from: 0; to: 10; value: downloadManager.optimisticSlots }
                                    Item {}
                                    AcidButton {
                                        text: "应用策略"
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.setChokingStrategy(chokingAlgorithmBox.currentIndex, seedChokingAlgorithmBox.currentIndex, uploadSlotsSpin.value, optimisticSlotsSpin.value)
                                            downloadManager.setSeedOnCompletionEnabled(seedOnCompletionSwitch.checked)
                                            settingsService.setChokingAlgorithm(chokingAlgorithmBox.currentIndex)
                                            settingsService.setSeedChokingAlgorithm(seedChokingAlgorithmBox.currentIndex)
                                            settingsService.setSeedOnCompletionEnabled(seedOnCompletionSwitch.checked)
                                            settingsService.setUploadSlots(uploadSlotsSpin.value)
                                            settingsService.setOptimisticSlots(optimisticSlotsSpin.value)
                                        }
                                    }
                                }
                                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AwaTheme.border }
                                Text { text: "Trackers"; color: AwaTheme.ink; font.pixelSize: 15; font.weight: Font.DemiBold }
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
                                        text: "One tracker per line. All trackers are announced in parallel."
                                        color: AwaTheme.muted
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }
                                    AcidButton {
                                        text: "Defaults"
                                        onClicked: trackerUrlsInput.text = settingsService.defaultTrackerUrlsText()
                                    }
                                    AcidButton {
                                        text: "Apply"
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.trackerUrlsText = trackerUrlsInput.text
                                            settingsService.setTrackerUrlsText(downloadManager.trackerUrlsText)
                                            trackerUrlsInput.text = downloadManager.trackerUrlsText
                                            showToast("Trackers saved")
                                        }
                                    }
                                }
                                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AwaTheme.border }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: apiPortInput
                                        Layout.fillWidth: true
                                        text: settingsService.apiPort().toString()
                                        placeholderText: "API 端口"
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        color: AwaTheme.ink
                                        placeholderTextColor: AwaTheme.muted
                                        background: FieldBackground {}
                                    }
                                    Switch {
                                        id: apiAutoStartSwitch
                                        text: "启动本地 API"
                                        checked: settingsService.startApiAutomatically()
                                    }
                                    AcidButton {
                                        text: "保存 API"
                                        tone: "primary"
                                        onClicked: {
                                            settingsService.setApiPort(positiveInt(apiPortInput.text, 18777))
                                            settingsService.setStartApiAutomatically(apiAutoStartSwitch.checked)
                                            showToast("API 设置已保存，重启后生效")
                                        }
                                    }
                                }
                                Switch {
                                    text: "启动 RSS 自动匹配"
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
