pragma Singleton

import QtQuick

QtObject {
    id: root

    property string language: "zh_CN"
    readonly property bool english: language === "en_US"
    readonly property bool japanese: language === "ja_JP"
    readonly property var jaText: ({
        "API 端口": "API ポート",
        "API 设置已保存，重启后生效": "API 設定を保存しました。再起動後に反映されます",
        "BT下载器": "BT ダウンローダー",
        "DHT": "DHT",
        "English": "English",
        "HTTP": "HTTP",
        "Japanese": "日本語",
        "Peers / Seeds": "ピア / シード",
        "RSS 状态": "RSS 状態",
        "RSS 自动下载默认关闭": "RSS 自動ダウンロードは既定でオフです",
        "RSS 订阅": "RSS フィード",
        "简体中文": "簡体字中国語",
        "Simplified Chinese": "簡体字中国語",
        "Token": "トークン",
        "Tracker": "トラッカー",
        "Trackers": "トラッカー",
        "Trackers 已保存": "トラッカーを保存しました",
        "Upload": "アップロード",
        "Upload Slots": "アップロードスロット",
        "Upload limit KiB/s, 0 for unlimited": "アップロード速度制限 KiB/s、0 で無制限",
        "WebSocket": "WebSocket",
        "上传": "アップロード",
        "上传博弈策略": "チョーキング戦略",
        "上传槽位": "アップロードスロット",
        "上传限速 KiB/s，0 表示不限": "アップロード速度制限 KiB/s、0 で無制限",
        "下载": "ダウンロード",
        "下载中": "ダウンロード中",
        "下载任务": "ダウンロード",
        "下载设置": "ダウンロード設定",
        "下载限速 KiB/s，0 表示不限": "ダウンロード速度制限 KiB/s、0 で無制限",
        "乐观解阻塞槽位": "楽観的アンチチョーク枠",
        "互惠固定槽位": "固定スロット",
        "互惠速率自适应": "速度ベース",
        "任务详情": "タスク詳細",
        "保存": "保存先",
        "保存 API": "API を保存",
        "保存路径": "保存先パス",
        "做种中": "シード中",
        "做种时策略": "シード戦略",
        "停止": "停止",
        "关闭": "閉じる",
        "关闭详情": "詳細を閉じる",
        "分块": "ピース",
        "分块跟踪": "ピースマップ",
        "刷新全部": "すべて更新",
        "反吸血博弈": "アンチリーチ",
        "取消": "キャンセル",
        "启动": "開始",
        "启动 RSS 自动匹配": "RSS 自動マッチを開始",
        "启动本地 API": "ローカル API を起動",
        "启用 RSS 自动匹配": "RSS 自動マッチを有効化",
        "块选择与上传博弈": "ピース選択とチョーキング",
        "块选择策略": "ピース戦略",
        "大小": "サイズ",
        "完成即停止": "完了後に停止",
        "完成后做种": "完了後にシード",
        "完成后的任务会出现在这里": "完了したタスクはここに表示されます",
        "尚未刷新": "まだ更新されていません",
        "已停止": "停止中",
        "已内置默认源：FOSS Torrents、Academic Torrents": "組み込み既定フィード: FOSS Torrents、Academic Torrents",
        "已完成": "完了",
        "应用": "適用",
        "应用策略": "戦略を適用",
        "应用限速": "速度制限を適用",
        "打开文件夹": "フォルダーを開く",
        "拖入 .torrent 文件，或添加磁力链接": ".torrent ファイルをドラッグするか、マグネットリンクを追加してください",
        "排队中": "待機中",
        "暂停": "一時停止",
        "暂停下载": "一時停止中",
        "暂停做种": "シード一時停止",
        "暂无下载中的任务": "進行中のタスクはありません",
        "暂无已完成的任务": "完了したタスクはありません",
        "最快上传优先": "最速アップロード優先",
        "未命名任务": "名称未設定のタスク",
        "未完成": "未完了",
        "未知": "不明",
        "本地 API": "ローカル API",
        "日语": "日本語",
        "每行一个 Tracker。所有 Tracker 会并行 announce。": "トラッカーは 1 行に 1 つです。すべてのトラッカーに並列で announce します。",
        "添加 RSS 源": "RSS フィードを追加",
        "添加任务": "タスクを追加",
        "添加磁力": "マグネットを追加",
        "添加磁力链接": "マグネットリンクを追加",
        "添加订阅": "フィードを追加",
        "状态": "状態",
        "界面语言": "表示言語",
        "监听状态": "待機状態",
        "移除任务": "タスクを削除",
        "稀有块优先": "レアストファースト",
        "等待分块数据": "ピースデータを待機中",
        "粘贴 magnet URI，并确认保存目录": "magnet URI を貼り付けて保存先を確認してください",
        "继续": "再開",
        "继续做种": "シードを継続",
        "自动匹配": "自動マッチ",
        "获取元数据": "メタデータ取得中",
        "订阅名称": "フィード名",
        "订阅数量：": "購読数: ",
        "设置": "設定",
        "设置已保存": "設定を保存しました",
        "详情": "詳細",
        "语言已切换": "言語を切り替えました",
        "请求需携带 Authorization: Bearer <Token>。API 默认只监听 127.0.0.1。": "リクエストには Authorization: Bearer <Token> が必要です。API は既定で 127.0.0.1 のみを監視します。",
        "轮询": "ラウンドロビン",
        "运行中": "実行中",
        "英语": "英語",
        "进展": "進捗",
        "远程 API": "リモート API",
        "连接": "接続",
        "选择": "参照",
        "选择种子": "Torrent を開く",
        "选择种子文件": "Torrent ファイルを選択",
        "选择默认保存目录": "既定の保存先を選択",
        "错误": "エラー",
        "预计 ": "残り ",
        "默认": "既定値",
        "默认保存目录": "既定の保存先",
        "下载内核未就绪": "ダウンロードエンジンの準備ができていません",
        "保存目录不可用": "保存フォルダーを利用できません",
        "保存目录不存在": "保存フォルダーが存在しません",
        "无法打开保存目录": "保存フォルダーを開けません",
        "限速已应用": "速度制限を適用しました",
        "稀有块优先与上传博弈策略已应用": "ピース選択とチョーキング戦略を適用しました",
        "任务完成后将继续做种": "タスク完了後もシードを継続します",
        "任务完成后将停止做种": "タスク完了後にシードを停止します",
        "正在连接 DHT/Tracker 获取元数据": "DHT/トラッカーに接続してメタデータを取得中",
        "正在恢复连接...": "接続を復旧中...",
        "任务来源不可用，无法恢复": "タスクのソースが利用できないため復元できません",
        "继续做种中": "シード中",
        "下载完成": "ダウンロード完了",
        "已恢复任务": "タスクを復元しました",
        "Tracker 暂无响应，继续通过 DHT 查找 peer": "トラッカーからまだ応答がありません。DHT でピア探索を継続します",
        "暂无 peer，正在重新向 Tracker/DHT 查找连接": "ピアがまだ見つかりません。Tracker/DHT で再探索中です",
        "正在重新发现 peer": "ピアを再探索中",
        "下载优先，等待下载任务完成后继续做种": "ダウンロードを優先します。完了後にシードを継続します",
        "DHT 查询中": "DHT 問い合わせ中",
        "DHT 等待下一次查询": "DHT 次回問い合わせ待ち",
        "正在等待 Tracker 响应": "トラッカーの応答を待機中",
        "正在通过 Tracker/DHT 查找 peer": "Tracker/DHT でピアを探索中",
        "等待可用 peer": "利用可能なピアを待機中",
        "采用稀有块优先策略获取元数据": "レアストファーストでメタデータを取得中",
        "网络已断开，等待恢复后重连": "ネットワークが切断されました。復旧後に再接続します",
        "网络变化，正在重新连接": "ネットワークが変化したため再接続中です",
        "网络环境变化，已重启下载连接": "ネットワーク環境の変化によりダウンロード接続を再起動しました",
        "RSS 地址无效": "RSS URL が無効です",
        "暂无 RSS 订阅": "RSS 購読はありません",
        "RSS 自动匹配已启用": "RSS 自動マッチを有効にしました",
        "RSS 自动匹配已关闭": "RSS 自動マッチを無効にしました",
        "已加载默认 RSS 源": "既定の RSS フィードを読み込みました",
        "写入 RSS 种子缓存失败": "RSS Torrent キャッシュの書き込みに失敗しました"
    })
    readonly property var dynamicText: ({
        "排队中": "Queued",
        "获取元数据": "Fetching metadata",
        "下载中": "Downloading",
        "暂停下载": "Paused",
        "做种中": "Seeding",
        "已完成": "Finished",
        "错误": "Error",
        "暂停做种": "Paused seeding",
        "未知": "Unknown",
        "下载内核未就绪": "Download engine is not ready",
        "保存目录不可用": "Save folder is unavailable",
        "保存目录不存在": "Save folder does not exist",
        "无法打开保存目录": "Could not open save folder",
        "限速已应用": "Speed limits applied",
        "稀有块优先与上传博弈策略已应用": "Piece selection and choking strategy applied",
        "任务完成后将继续做种": "Tasks will keep seeding after completion",
        "任务完成后将停止做种": "Tasks will stop seeding after completion",
        "正在连接 DHT/Tracker 获取元数据": "Connecting to DHT/Tracker for metadata",
        "正在恢复连接...": "Restoring connection...",
        "任务来源不可用，无法恢复": "Task source is unavailable; cannot restore",
        "继续做种中": "Seeding",
        "下载完成": "Download complete",
        "已恢复任务": "Task restored",
        "Tracker 暂无响应，继续通过 DHT 查找 peer": "No tracker response yet; continuing peer discovery through DHT",
        "暂无 peer，正在重新向 Tracker/DHT 查找连接": "No peers yet; searching Tracker/DHT again",
        "正在重新发现 peer": "Rediscovering peers",
        "下载优先，等待下载任务完成后继续做种": "Download has priority; seeding will continue after downloads finish",
        "DHT 查询中": "Querying DHT",
        "DHT 等待下一次查询": "DHT waiting for next query",
        "正在等待 Tracker 响应": "Waiting for tracker response",
        "正在通过 Tracker/DHT 查找 peer": "Searching for peers through Tracker/DHT",
        "等待可用 peer": "Waiting for available peers",
        "采用稀有块优先策略获取元数据": "Fetching metadata with rarest-first piece selection",
        "网络已断开，等待恢复后重连": "Network is offline; waiting to reconnect",
        "网络变化，正在重新连接": "Network changed; reconnecting",
        "网络环境变化，已重启下载连接": "Network changed; download connections restarted",
        "RSS 地址无效": "RSS URL is invalid",
        "暂无 RSS 订阅": "No RSS subscriptions",
        "RSS 自动匹配已启用": "RSS auto match enabled",
        "RSS 自动匹配已关闭": "RSS auto match disabled",
        "已加载默认 RSS 源": "Default RSS feeds loaded",
        "写入 RSS 种子缓存失败": "Failed to write RSS torrent cache"
    })

    function tr(zh, en) {
        if (english) {
            return en
        }
        if (japanese) {
            return jaText[zh] || en
        }
        return zh
    }

    function dynamic(text) {
        if (!text || text.length === 0) {
            return text
        }

        if (english && dynamicText[text]) {
            return dynamicText[text]
        }
        if (japanese && jaText[text]) {
            return jaText[text]
        }
        if (!english && !japanese) {
            return text
        }

        const prefixed = [
            ["无法读取种子：", english ? "Unable to read torrent: " : "Torrent を読み取れません: "],
            ["添加种子失败：", english ? "Failed to add torrent: " : "Torrent の追加に失敗しました: "],
            ["磁力链接无效：", english ? "Invalid magnet link: " : "無効なマグネットリンクです: "],
            ["添加磁力失败：", english ? "Failed to add magnet: " : "マグネットの追加に失敗しました: "],
            ["添加失败：", english ? "Add failed: " : "追加に失敗しました: "],
            ["恢复任务失败：", english ? "Failed to restore task: " : "タスクの復元に失敗しました: "],
            ["Tracker 警告：", english ? "Tracker warning: " : "トラッカー警告: "],
            ["订阅已存在：", english ? "Subscription already exists: " : "購読は既に存在します: "],
            ["已添加订阅：", english ? "Subscription added: " : "購読を追加しました: "],
            ["下载 RSS 种子失败：", english ? "Failed to download RSS torrent: " : "RSS Torrent のダウンロードに失敗しました: "]
        ]
        for (let i = 0; i < prefixed.length; ++i) {
            if (text.startsWith(prefixed[i][0])) {
                return prefixed[i][1] + text.slice(prefixed[i][0].length)
            }
        }

        let match = text.match(/^Tracker 已响应，发现 (\d+) 个 peer$/)
        if (match) {
            return english
                ? "Tracker responded, found " + match[1] + " peers"
                : "トラッカーが応答しました。 " + match[1] + " 個のピアが見つかりました"
        }
        match = text.match(/^正在重新获取元数据，第 (\d+) 次重试$/)
        if (match) {
            return english
                ? "Refetching metadata, retry " + match[1]
                : "メタデータを再取得中です。再試行 " + match[1] + " 回目"
        }
        match = text.match(/^已连接 (\d+) 个 peer$/)
        if (match) {
            return english
                ? "Connected to " + match[1] + " peers"
                : match[1] + " 個のピアに接続中"
        }
        match = text.match(/^发现 (\d+) 个 peer，正在建立连接$/)
        if (match) {
            return english
                ? "Found " + match[1] + " peers; connecting"
                : match[1] + " 個のピアが見つかりました。接続中です"
        }
        match = text.match(/^获取元数据：(.+)，Peers (\d+)，连接 (\d+)，Tracker (\d+)\/(\d+)$/)
        if (match) {
            return english
                ? "Fetching metadata: " + dynamic(match[1]) + ", Peers " + match[2] + ", Connections " + match[3] + ", Trackers " + match[4] + "/" + match[5]
                : "メタデータ取得中: " + dynamic(match[1]) + "、ピア " + match[2] + "、接続 " + match[3] + "、トラッカー " + match[4] + "/" + match[5]
        }
        match = text.match(/^下载中：Peers (\d+)，Seeds (\d+)，连接 (\d+)，Tracker (\d+)\/(\d+)$/)
        if (match) {
            return english
                ? "Downloading: Peers " + match[1] + ", Seeds " + match[2] + ", Connections " + match[3] + ", Trackers " + match[4] + "/" + match[5]
                : "ダウンロード中: ピア " + match[1] + "、シード " + match[2] + "、接続 " + match[3] + "、トラッカー " + match[4] + "/" + match[5]
        }
        match = text.match(/^刷新完成，匹配 (\d+) 个条目$/)
        if (match) {
            return english
                ? "Refresh complete; matched " + match[1] + " items"
                : "更新完了。 " + match[1] + " 件一致しました"
        }

        return text
    }
}
