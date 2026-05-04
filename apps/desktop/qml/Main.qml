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
    color: "#f8fafc"

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
            Layout.preferredWidth: 224
            Layout.fillHeight: true
            color: "#ffffff"
            border.color: "#e2e8f0"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: 8
                        color: "#ffffff"
                        border.color: "#e2e8f0"
                        clip: true
                        Image {
                            anchors.centerIn: parent
                            width: 32
                            height: 32
                            source: "qrc:/images/logo.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }
                    ColumnLayout {
                        spacing: 1
                        Text { text: "AwaKurage"; font.pixelSize: 16; font.weight: Font.DemiBold; color: "#0f172a" }
                        Text { text: "BT 下载器"; font.pixelSize: 12; color: "#64748b" }
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
                    radius: 8
                    clip: true
                    color: "#f8fafc"
                    border.color: "#e2e8f0"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6
                        Text { text: "本地 API"; color: "#334155"; font.pixelSize: 13; font.weight: Font.DemiBold }
                        Text { width: parent.width; text: "127.0.0.1:" + apiServer.port; color: "#64748b"; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { width: parent.width; text: "WebSocket: " + (apiServer.port + 1); color: "#94a3b8"; font.pixelSize: 11; elide: Text.ElideRight }
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
                color: "#ffffff"
                border.color: "#e2e8f0"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: pageTitles[currentPage]; color: "#0f172a"; font.pixelSize: 22; font.weight: Font.DemiBold }
                        Text { text: pageSubtitles[currentPage]; color: "#64748b"; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                    }

                    Button {
                        visible: currentPage === 0
                        text: "添加磁力"
                        onClicked: magnetDialog.open()
                    }
                    Button {
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
                    color: "#f8fafc"

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
                            anchors.centerIn: parent
                            width: 360
                            height: 180
                            radius: 8
                            visible: listView.count === 0
                            color: "#ffffff"
                            border.color: "#e2e8f0"
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 12
                                Text { text: "暂无下载任务"; color: "#0f172a"; font.pixelSize: 18; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "拖入 .torrent 文件，或添加磁力链接"; color: "#64748b"; font.pixelSize: 13; Layout.alignment: Qt.AlignHCenter }
                                Button { text: "添加磁力"; Layout.alignment: Qt.AlignHCenter; onClicked: magnetDialog.open() }
                            }
                        }
                    }
                }

                Rectangle {
                    id: detailPane
                    SplitView.preferredWidth: 360
                    SplitView.minimumWidth: 300
                    color: "#ffffff"
                    border.color: "#e2e8f0"

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
                            Button {
                                text: "暂停"
                                Layout.fillWidth: true
                                enabled: selectedDownloadId.length > 0 && selectedDownload.stateText !== "已暂停"
                                onClicked: downloadManager.pause(selectedDownloadId)
                            }
                            Button {
                                text: "继续"
                                Layout.fillWidth: true
                                enabled: selectedDownloadId.length > 0 && selectedDownload.stateText === "已暂停"
                                onClicked: downloadManager.resume(selectedDownloadId)
                            }
                        }
                        Button {
                            Layout.fillWidth: true
                            text: "移除任务"
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
                                Button {
                                    text: "添加订阅"
                                    onClicked: {
                                        rssService.addSubscription(rssTitleInput.text, rssUrlInput.text)
                                        rssTitleInput.clear()
                                        rssUrlInput.clear()
                                    }
                                }
                                Button { text: "刷新全部"; onClicked: rssService.refreshAll() }
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
                                Button { text: "启动"; enabled: !apiServer.listening; onClicked: apiServer.start(18777) }
                                Button { text: "停止"; enabled: apiServer.listening; onClicked: apiServer.stop() }
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

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 250
                        radius: 8
                        color: "#ffffff"
                        border.color: "#e2e8f0"

                        ColumnLayout {
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
                                Button { text: "选择"; onClicked: folderDialog.open() }
                                Button {
                                    text: "保存"
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
                                TextField { id: dlLimitInput; Layout.fillWidth: true; placeholderText: "下载限速 KiB/s，0 表示不限" }
                                TextField { id: ulLimitInput; Layout.fillWidth: true; placeholderText: "上传限速 KiB/s，0 表示不限" }
                                Button {
                                    text: "应用限速"
                                    onClicked: downloadManager.setSpeedLimits(parseInt(dlLimitInput.text || "0"), parseInt(ulLimitInput.text || "0"))
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
                }
            }
        }
    }
}
