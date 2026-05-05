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
    color: "#f4f8f8"

    property string selectedDownloadId: ""
    property var selectedDownload: ({})
    property string toastText: ""
    property int currentPage: 0
    property int downloadsRevision: 0
    readonly property var pageTitles: ["下载任务", "RSS 订阅", "远程 API", "设置"]
    readonly property var pageSubtitles: [
        "种子、磁力、RSS 自动规则与远程控制",
        "管理订阅源，并按规则自动添加磁力任务",
        "本机 HTTP/WebSocket 控制接口",
        "下载目录、限速和应用偏好"
    ]

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

    Connections {
        target: downloadManager
        function onToastRequested(message) {
            toastText = message
            toastPopup.open()
        }
    }

    Connections {
        target: downloadManager.downloads
        function onRowsInserted(parent, first, last) {
            downloadsRevision += 1
            if (selectedDownloadId.length === 0 && first >= 0) {
                selectDownload(downloadManager.downloads.get(first).downloadId)
            } else {
                refreshSelectedDownload()
            }
        }
        function onRowsRemoved(parent, first, last) {
            downloadsRevision += 1
            if (selectedDownloadId.length === 0) {
                return
            }

            const row = downloadManager.downloads.indexOfId(selectedDownloadId)
            if (row >= 0) {
                refreshSelectedDownload()
            } else if (downloadManager.downloads.count() > 0) {
                selectDownload(downloadManager.downloads.get(0).downloadId)
            } else {
                selectedDownloadId = ""
                selectedDownload = ({})
            }
        }
        function onDataChanged(topLeft, bottomRight, roles) {
            downloadsRevision += 1
            refreshSelectedDownload()
        }
        function onModelReset() {
            downloadsRevision += 1
            refreshSelectedDownload()
        }
    }

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
        title: "添加磁力链接"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 560
        anchors.centerIn: parent
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            TextArea {
                id: magnetInput
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                placeholderText: "magnet:?xt=urn:btih:..."
                wrapMode: TextArea.WrapAnywhere
            }
            TextField {
                id: savePathInput
                Layout.fillWidth: true
                text: downloadManager.defaultSavePath
                placeholderText: "保存路径"
            }
        }
        onAccepted: {
            downloadManager.addMagnet(magnetInput.text, {"savePath": savePathInput.text})
            magnetInput.clear()
        }
    }

    Popup {
        id: toastPopup
        x: Math.round((window.width - width) / 2)
        y: 24
        width: Math.min(520, window.width - 48)
        height: 46
        modal: false
        closePolicy: Popup.NoAutoClose
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140 } }
        background: Rectangle {
            radius: 8
            color: "#0f172a"
        }
        contentItem: Text {
            text: toastText
            color: "white"
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
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
            Layout.preferredWidth: 246
            Layout.fillHeight: true
            color: "#fbfffd"
            border.color: "#dbe7ec"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 22
                anchors.bottomMargin: 18
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Item {
                        Layout.preferredWidth: 46
                        Layout.preferredHeight: 46
                        Image {
                            anchors.centerIn: parent
                            width: 44
                            height: 44
                            source: appLogoSource
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }
                    ColumnLayout {
                        spacing: 1
                        Text { text: "AwaKurage"; font.pixelSize: 17; font.weight: Font.DemiBold; color: "#0f172a" }
                        Text { text: "BT 下载器"; font.pixelSize: 12; color: "#6b7f99" }
                    }
                }

                SidebarButton { Layout.fillWidth: true; text: "下载任务"; iconText: "↓"; checked: currentPage === 0; onClicked: currentPage = 0 }
                SidebarButton { Layout.fillWidth: true; text: "RSS 订阅"; iconText: "≋"; checked: currentPage === 1; onClicked: currentPage = 1 }
                SidebarButton { Layout.fillWidth: true; text: "远程 API"; iconText: "{}"; checked: currentPage === 2; onClicked: currentPage = 2 }
                SidebarButton { Layout.fillWidth: true; text: "设置"; iconText: "⚙"; checked: currentPage === 3; onClicked: currentPage = 3 }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    height: 86
                    radius: 10
                    clip: true
                    color: "#f4f8f8"
                    border.color: "#dbe7ec"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6
                        Text { text: "本地 API"; color: "#0f172a"; font.pixelSize: 13; font.weight: Font.DemiBold }
                        Text { width: parent.width; text: "127.0.0.1:" + apiServer.port; color: "#5d718a"; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { width: parent.width; text: "WebSocket: " + (apiServer.port + 1); color: "#9aaabd"; font.pixelSize: 11; elide: Text.ElideRight }
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
                Layout.preferredHeight: 76
                color: "#fbfffd"
                border.color: "#dbe7ec"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: pageTitles[currentPage]; color: "#07111f"; font.pixelSize: 24; font.weight: Font.DemiBold }
                        Text { text: pageSubtitles[currentPage]; color: "#6b7f99"; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                    }

                    AcidButton {
                        visible: currentPage === 0
                        text: "添加磁力"
                        tone: "primary"
                        onClicked: magnetDialog.open()
                    }
                    AcidButton {
                        visible: currentPage === 0
                        text: "选择种子"
                        onClicked: torrentDialog.open()
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
                    color: "#f4f8f8"

                    Image {
                        id: posterBackground
                        source: appPosterSource
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: -42
                        anchors.bottomMargin: -34
                        width: Math.min(parent.width * 0.62, parent.height * 0.82)
                        height: width
                        fillMode: Image.PreserveAspectFit
                        opacity: listView.count === 0 ? 0.88 : 0.24
                        smooth: true
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "#f4f8f8"
                        opacity: listView.count === 0 ? 0.18 : 0.62
                    }

                    ListView {
                        id: listView
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10
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
                            rowIndex: index
                            selected: selectedDownloadId === itemData.downloadId
                            onOpenDetails: function(item) { selectDownload(item.downloadId, item) }
                            onPauseTask: function(id) { downloadManager.pause(id) }
                            onResumeTask: function(id) { downloadManager.resume(id) }
                            onRemoveTask: function(id) { downloadManager.remove(id, false) }
                        }

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: Math.max(34, Math.round(parent.width * 0.14))
                            width: Math.min(430, parent.width - 68)
                            height: 210
                            radius: 14
                            visible: listView.count === 0
                            color: "#fbfffd"
                            border.color: "#d9e8ec"
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 14
                                Text { text: "暂无下载任务"; color: "#07111f"; font.pixelSize: 22; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "拖入 .torrent 文件，或添加磁力链接"; color: "#6b7f99"; font.pixelSize: 14; Layout.alignment: Qt.AlignHCenter }
                                AcidButton { text: "添加磁力"; tone: "primary"; Layout.alignment: Qt.AlignHCenter; onClicked: magnetDialog.open() }
                            }
                        }
                    }
                }

                Rectangle {
                    id: detailPane
                    SplitView.preferredWidth: 360
                    SplitView.minimumWidth: 300
                    color: "#fbfffd"
                    border.color: "#dbe7ec"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 16
                        Text {
                            Layout.fillWidth: true
                            Layout.maximumWidth: parent.width
                            text: selectedDownload.name || "任务详情"
                            color: "#0f172a"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 1
                            value: selectedDownload.progress || 0
                        }
                        GridLayout {
                            columns: 2
                            Layout.fillWidth: true
                            rowSpacing: 10
                            columnSpacing: 16
                            clip: true
                            Text { text: "状态"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: selectedDownload.stateText || "-"; color: "#0f172a"; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 230 }
                            Text { text: "进展"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: selectedDownload.statusText || "-"; color: "#0f172a"; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 230 }
                            Text { text: "保存"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: selectedDownload.savePath || "-"; color: "#0f172a"; font.pixelSize: 12; elide: Text.ElideMiddle; Layout.fillWidth: true; Layout.maximumWidth: 230 }
                            Text { text: "大小"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: formatBytes(selectedDownload.downloadedBytes || 0) + " / " + formatBytes(selectedDownload.totalBytes || 0); color: "#0f172a"; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 230 }
                            Text { text: "分块"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: (selectedDownload.completedPieces || 0) + " / " + (selectedDownload.pieceCount || 0) + " pieces"; color: "#0f172a"; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 230 }
                            Text { text: "下载"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: Math.round((selectedDownload.downloadRate || 0) / 1024) + " KiB/s"; color: "#0f172a"; font.pixelSize: 12 }
                            Text { text: "上传"; color: "#64748b"; font.pixelSize: 12 }
                            Text { text: Math.round((selectedDownload.uploadRate || 0) / 1024) + " KiB/s"; color: "#0f172a"; font.pixelSize: 12 }
                        }

                        Item {
                            id: detailPieceBar
                            Layout.fillWidth: true
                            Layout.preferredHeight: 8
                            visible: (selectedDownload.pieceMap || "").length > 0
                            clip: true
                            readonly property int sampleCount: Math.min((selectedDownload.pieceMap || "").length, 96)
                            Row {
                                anchors.fill: parent
                                spacing: 2
                                Repeater {
                                    model: detailPieceBar.sampleCount
                                    Rectangle {
                                        width: detailPieceBar.sampleCount > 0
                                            ? Math.max(1, (detailPieceBar.width - ((detailPieceBar.sampleCount - 1) * 2)) / detailPieceBar.sampleCount)
                                            : 0
                                        height: detailPieceBar.height
                                        radius: 2
                                        color: selectedDownload.pieceMap.charAt(index) === "1" ? "#0f766e" : "#e2e8f0"
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            AcidButton {
                                text: "暂停"
                                Layout.fillWidth: true
                                enabled: selectedDownloadId.length > 0 && selectedDownload.stateText !== "已暂停"
                                onClicked: downloadManager.pause(selectedDownloadId)
                            }
                            AcidButton {
                                text: "继续"
                                tone: "primary"
                                Layout.fillWidth: true
                                enabled: selectedDownloadId.length > 0 && selectedDownload.stateText === "已暂停"
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

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#e2e8f0"
                        }

                        Text { Layout.fillWidth: true; text: "RSS 自动下载默认关闭"; color: "#64748b"; font.pixelSize: 12; elide: Text.ElideRight }
                        Switch {
                            Layout.fillWidth: true
                            text: "启用 RSS 自动匹配"
                            checked: rssService.autoDownloadEnabled()
                            onToggled: rssService.setAutoDownloadEnabled(checked)
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Rectangle {
                visible: currentPage === 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#f8fafc"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: rssAddContent.implicitHeight + 36
                        Layout.minimumHeight: 260
                        radius: 8
                        color: "#ffffff"
                        border.color: "#e2e8f0"

                        ColumnLayout {
                            id: rssAddContent
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 12
                            Text { text: "添加 RSS 源"; color: "#0f172a"; font.pixelSize: 17; font.weight: Font.DemiBold }
                            Text {
                                Layout.fillWidth: true
                                text: "已内置默认源：FOSS Torrents、Academic Torrents"
                                color: "#64748b"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            TextField { id: rssTitleInput; Layout.fillWidth: true; placeholderText: "订阅名称" }
                            TextField { id: rssUrlInput; Layout.fillWidth: true; placeholderText: "https://example.com/feed.xml" }
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

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 8
                        color: "#ffffff"
                        border.color: "#e2e8f0"
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 10
                            Text { text: "RSS 状态"; color: "#0f172a"; font.pixelSize: 17; font.weight: Font.DemiBold }
                            Text { Layout.fillWidth: true; text: "订阅数量：" + rssService.subscriptionCount; color: "#334155"; font.pixelSize: 13 }
                            Text { Layout.fillWidth: true; text: rssService.lastStatus.length > 0 ? rssService.lastStatus : "尚未刷新"; color: "#64748b"; font.pixelSize: 13; elide: Text.ElideRight }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                visible: currentPage === 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#f8fafc"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 260
                        radius: 8
                        color: "#ffffff"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            columns: 2
                            rowSpacing: 12
                            columnSpacing: 18

                            Text { text: "监听状态"; color: "#64748b"; font.pixelSize: 13 }
                            Text { text: apiServer.listening ? "运行中" : "已停止"; color: apiServer.listening ? "#0f766e" : "#b91c1c"; font.pixelSize: 13; font.weight: Font.DemiBold }
                            Text { text: "HTTP"; color: "#64748b"; font.pixelSize: 13 }
                            Text { Layout.fillWidth: true; text: "http://127.0.0.1:" + apiServer.port + "/api/v1/downloads"; color: "#0f172a"; font.pixelSize: 13; elide: Text.ElideRight }
                            Text { text: "WebSocket"; color: "#64748b"; font.pixelSize: 13 }
                            Text { Layout.fillWidth: true; text: "ws://127.0.0.1:" + (apiServer.port + 1) + "/api/v1/events"; color: "#0f172a"; font.pixelSize: 13; elide: Text.ElideRight }
                            Text { text: "Token"; color: "#64748b"; font.pixelSize: 13 }
                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                readOnly: true
                                wrapMode: TextArea.WrapAnywhere
                                text: apiServer.token
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
                        color: "#64748b"
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
                color: "#f8fafc"

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    ColumnLayout {
                        width: Math.max(parent.width - 48, 620)
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 16

                        Item { Layout.preferredHeight: 8 }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: settingsContent.implicitHeight + 36
                            radius: 8
                            color: "#ffffff"
                            border.color: "#e2e8f0"

                            ColumnLayout {
                                id: settingsContent
                                anchors.fill: parent
                                anchors.margins: 18
                                spacing: 14
                                Text { text: "下载设置"; color: "#0f172a"; font.pixelSize: 17; font.weight: Font.DemiBold }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: settingsPathInput
                                        Layout.fillWidth: true
                                        text: downloadManager.defaultSavePath
                                        placeholderText: "默认保存目录"
                                    }
                                    AcidButton { text: "选择"; onClicked: folderDialog.open() }
                                    AcidButton {
                                        text: "保存"
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.defaultSavePath = settingsPathInput.text
                                            settingsService.setDownloadDirectory(settingsPathInput.text)
                                            toastText = "设置已保存"
                                            toastPopup.open()
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
                                    }
                                    TextField {
                                        id: ulLimitInput
                                        Layout.fillWidth: true
                                        text: downloadManager.uploadLimitKiB.toString()
                                        placeholderText: "上传限速 KiB/s，0 表示不限"
                                        inputMethodHints: Qt.ImhDigitsOnly
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
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: "#e2e8f0"
                                }
                                Text { text: "块选择与上传博弈"; color: "#0f172a"; font.pixelSize: 15; font.weight: Font.DemiBold }
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 10
                                    columnSpacing: 14
                                    Text { text: "块选择策略"; color: "#64748b"; font.pixelSize: 13 }
                                    Text {
                                        Layout.fillWidth: true
                                        text: "稀有块优先"
                                        color: "#0f172a"
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                    Text { text: "上传博弈策略"; color: "#64748b"; font.pixelSize: 13 }
                                    ComboBox {
                                        id: chokingAlgorithmBox
                                        Layout.fillWidth: true
                                        model: ["互惠固定槽位", "互惠速率自适应"]
                                        currentIndex: downloadManager.chokingAlgorithm
                                    }
                                    Text { text: "做种时策略"; color: "#64748b"; font.pixelSize: 13 }
                                    ComboBox {
                                        id: seedChokingAlgorithmBox
                                        Layout.fillWidth: true
                                        model: ["轮询", "最快上传优先", "反吸血博弈"]
                                        currentIndex: downloadManager.seedChokingAlgorithm
                                    }
                                    Text { text: "上传槽位"; color: "#64748b"; font.pixelSize: 13 }
                                    SpinBox {
                                        id: uploadSlotsSpin
                                        Layout.fillWidth: true
                                        from: 1
                                        to: 200
                                        value: downloadManager.uploadSlots
                                    }
                                    Text { text: "乐观解阻塞槽位"; color: "#64748b"; font.pixelSize: 13 }
                                    SpinBox {
                                        id: optimisticSlotsSpin
                                        Layout.fillWidth: true
                                        from: 0
                                        to: 10
                                        value: downloadManager.optimisticSlots
                                    }
                                    Item {}
                                    AcidButton {
                                        text: "应用策略"
                                        tone: "primary"
                                        onClicked: {
                                            downloadManager.setChokingStrategy(chokingAlgorithmBox.currentIndex, seedChokingAlgorithmBox.currentIndex, uploadSlotsSpin.value, optimisticSlotsSpin.value)
                                            settingsService.setChokingAlgorithm(chokingAlgorithmBox.currentIndex)
                                            settingsService.setSeedChokingAlgorithm(seedChokingAlgorithmBox.currentIndex)
                                            settingsService.setUploadSlots(uploadSlotsSpin.value)
                                            settingsService.setOptimisticSlots(optimisticSlotsSpin.value)
                                        }
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: "#e2e8f0"
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField {
                                        id: apiPortInput
                                        Layout.fillWidth: true
                                        text: settingsService.apiPort().toString()
                                        placeholderText: "API 端口"
                                        inputMethodHints: Qt.ImhDigitsOnly
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
                                            toastText = "API 设置已保存，重启后生效"
                                            toastPopup.open()
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
