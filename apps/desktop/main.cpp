#include "awa/api/localapiserver.h"
#include "awa/core/downloaditem.h"
#include "awa/core/downloadmanager.h"
#include "awa/core/settingsservice.h"
#include "awa/rss/rssservice.h"
#include "awa/torrent/torrentservice.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QAction>
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QMenu>
#include <QPointer>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QWindow>

#include <memory>

namespace {
class TrayController : public QObject
{
    Q_OBJECT

public:
    struct TrayText
    {
        QString showMainWindow;
        QString quit;
        QString closeTitle;
        QString closeMessage;
        QString downloadCompleteTitle;
        QString downloadCompleteMessage;
    };

    explicit TrayController(const QIcon& icon, QObject* parent = nullptr)
        : QObject(parent)
        , m_trayIcon(new QSystemTrayIcon(icon, this))
        , m_menu(new QMenu())
    {
        m_menu->setAttribute(Qt::WA_DeleteOnClose, false);

        m_showAction = m_menu->addAction(QString());
        m_menu->addSeparator();
        m_quitAction = m_menu->addAction(QString());
        setLanguage(QStringLiteral("zh_CN"));

        connect(m_showAction, &QAction::triggered, this, &TrayController::showMainWindow);
        connect(m_quitAction, &QAction::triggered, this, &TrayController::quitFromTray);
        connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                showMainWindow();
            }
        });

        m_trayIcon->setToolTip(QStringLiteral("AwaKurageDownloader"));
        m_trayIcon->setContextMenu(m_menu);
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            m_trayIcon->show();
        }
    }

    void setLanguage(const QString& language)
    {
        m_text = trayTextForLanguage(language);
        if (m_showAction) {
            m_showAction->setText(m_text.showMainWindow);
        }
        if (m_quitAction) {
            m_quitAction->setText(m_text.quit);
        }
    }

    ~TrayController() override
    {
        m_trayIcon->hide();
        delete m_menu;
    }

    void setMainWindow(QWindow* window)
    {
        m_mainWindow = window;
        if (m_showRequested) {
            m_showRequested = false;
            showMainWindow();
        }
    }

    void showMainWindow()
    {
        if (!m_mainWindow) {
            m_showRequested = true;
            return;
        }
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->requestActivate();
    }

    Q_INVOKABLE bool closeToTray(QWindow* window)
    {
        if (!QSystemTrayIcon::isSystemTrayAvailable()) {
            return false;
        }

        if (window) {
            m_mainWindow = window;
        }
        if (m_mainWindow) {
            m_mainWindow->hide();
        }

        if (!m_closeMessageShown && m_trayIcon->supportsMessages()) {
            m_trayIcon->showMessage(
                m_text.closeTitle,
                m_text.closeMessage,
                QSystemTrayIcon::Information,
                2500);
            m_closeMessageShown = true;
        }
        return true;
    }

    void showDownloadCompleted(const awa::core::DownloadItem& item)
    {
        if (!m_trayIcon->isVisible() || !m_trayIcon->supportsMessages()) {
            return;
        }

        QString name = item.name.trimmed();
        if (name.isEmpty()) {
            name = item.source.trimmed();
        }
        if (name.isEmpty()) {
            name = item.id;
        }

        m_trayIcon->showMessage(
            m_text.downloadCompleteTitle,
            m_text.downloadCompleteMessage.arg(name),
            QSystemTrayIcon::Information,
            5000);
    }

private slots:
    void quitFromTray()
    {
        m_trayIcon->hide();
        QCoreApplication::quit();
    }

private:
    static TrayText trayTextForLanguage(const QString& language)
    {
        if (language == QStringLiteral("en_US")) {
            return {
                QStringLiteral("Show main window"),
                QStringLiteral("Exit"),
                QStringLiteral("AwaKurageDownloader"),
                QStringLiteral("The main window was minimized to the system tray. To fully exit, choose Exit from the tray menu."),
                QStringLiteral("Download complete"),
                QStringLiteral("\"%1\" has finished downloading.")
            };
        }
        if (language == QStringLiteral("ja_JP")) {
            return {
                QStringLiteral("\u30e1\u30a4\u30f3\u753b\u9762\u3092\u8868\u793a"),
                QStringLiteral("\u7d42\u4e86"),
                QStringLiteral("AwaKurageDownloader"),
                QStringLiteral("\u30e1\u30a4\u30f3\u753b\u9762\u3092\u30b7\u30b9\u30c6\u30e0\u30c8\u30ec\u30a4\u306b\u6700\u5c0f\u5316\u3057\u307e\u3057\u305f\u3002\u5b8c\u5168\u306b\u7d42\u4e86\u3059\u308b\u306b\u306f\u3001\u30c8\u30ec\u30a4\u30e1\u30cb\u30e5\u30fc\u304b\u3089\u300c\u7d42\u4e86\u300d\u3092\u9078\u629e\u3057\u3066\u304f\u3060\u3055\u3044\u3002"),
                QStringLiteral("\u30c0\u30a6\u30f3\u30ed\u30fc\u30c9\u5b8c\u4e86"),
                QStringLiteral("\u300c%1\u300d\u306e\u30c0\u30a6\u30f3\u30ed\u30fc\u30c9\u304c\u5b8c\u4e86\u3057\u307e\u3057\u305f\u3002")
            };
        }
        return {
            QStringLiteral("\u663e\u793a\u4e3b\u754c\u9762"),
            QStringLiteral("\u9000\u51fa"),
            QStringLiteral("AwaKurageDownloader"),
            QStringLiteral("\u4e3b\u754c\u9762\u5df2\u6700\u5c0f\u5316\u5230\u6258\u76d8\u3002\u9700\u8981\u5b8c\u5168\u9000\u51fa\u65f6\uff0c\u8bf7\u5728\u6258\u76d8\u83dc\u5355\u4e2d\u9009\u62e9\u201c\u9000\u51fa\u201d\u3002"),
            QStringLiteral("\u4e0b\u8f7d\u5b8c\u6210"),
            QStringLiteral("\u201c%1\u201d \u5df2\u4e0b\u8f7d\u5b8c\u6210\u3002")
        };
    }

    QPointer<QWindow> m_mainWindow;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_showAction = nullptr;
    QAction* m_quitAction = nullptr;
    TrayText m_text;
    bool m_closeMessageShown = false;
    bool m_showRequested = false;
};

constexpr auto singleInstanceServerName = "AwaKurageDownloader.SingleInstance";

QString singleInstanceLockPath()
{
    QString lockDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (lockDir.isEmpty()) {
        lockDir = QDir::tempPath();
    }
    QDir().mkpath(lockDir);
    return QDir(lockDir).filePath(QStringLiteral("AwaKurageDownloader.lock"));
}

std::unique_ptr<QLockFile> createSingleInstanceLock()
{
    auto lock = std::make_unique<QLockFile>(singleInstanceLockPath());
    if (!lock->tryLock(100)) {
        return {};
    }
    return lock;
}

bool notifyRunningInstance()
{
    QLocalSocket socket;
    socket.connectToServer(QString::fromLatin1(singleInstanceServerName), QIODevice::WriteOnly);
    if (!socket.waitForConnected(300)) {
        return false;
    }

    socket.write("show\n");
    socket.flush();
    socket.waitForBytesWritten(300);
    return true;
}

std::unique_ptr<QLocalServer> createSingleInstanceServer()
{
    auto server = std::make_unique<QLocalServer>();
    server->setSocketOptions(QLocalServer::UserAccessOption);

    if (server->listen(QString::fromLatin1(singleInstanceServerName))) {
        return server;
    }

    QLocalServer::removeServer(QString::fromLatin1(singleInstanceServerName));
    if (server->listen(QString::fromLatin1(singleInstanceServerName))) {
        return server;
    }

    return {};
}

void installStartupLogger()
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (logDir.isEmpty()) {
        logDir = QCoreApplication::applicationDirPath();
    }
    QDir().mkpath(logDir);
    static QFile logFile(QDir(logDir).filePath(QStringLiteral("startup.log")));
    (void)logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
        if (!logFile.isOpen()) {
            return;
        }
        QTextStream stream(&logFile);
        const char* level = "info";
        switch (type) {
        case QtDebugMsg: level = "debug"; break;
        case QtInfoMsg: level = "info"; break;
        case QtWarningMsg: level = "warning"; break;
        case QtCriticalMsg: level = "critical"; break;
        case QtFatalMsg: level = "fatal"; break;
        }
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << level << "] " << message;
        if (context.file) {
            stream << " (" << context.file << ':' << context.line << ')';
        }
        stream << '\n';
        stream.flush();
        if (type == QtFatalMsg) {
            abort();
        }
    });
}
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    installStartupLogger();
    QApplication::setApplicationName(QStringLiteral("AwaKurageDownloader"));
    QApplication::setOrganizationName(QStringLiteral("AwaKurage"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.2"));

    auto singleInstanceLock = createSingleInstanceLock();
    if (!singleInstanceLock) {
        notifyRunningInstance();
        return 0;
    }
    auto singleInstanceServer = createSingleInstanceServer();

    const QString packagedLogoPngPath = QApplication::applicationDirPath() + QStringLiteral("/resources/APP.png");
    const QString logoPath = QFileInfo::exists(packagedLogoPngPath)
        ? packagedLogoPngPath
        : QStringLiteral(":/images/logo.png");
    const QString logoUrl = logoPath.startsWith(QLatin1Char(':'))
        ? QStringLiteral("qrc:/images/logo.png")
        : QUrl::fromLocalFile(logoPath).toString();
    const QString packagedPosterPath = QApplication::applicationDirPath() + QStringLiteral("/resources/Poster.png");
    const QString posterUrl = QFileInfo::exists(packagedPosterPath)
        ? QUrl::fromLocalFile(packagedPosterPath).toString()
        : QStringLiteral("qrc:/images/poster.png");
    const QString packagedQPosterPath = QApplication::applicationDirPath() + QStringLiteral("/resources/QPoster.png");
    const QString qPosterUrl = QFileInfo::exists(packagedQPosterPath)
        ? QUrl::fromLocalFile(packagedQPosterPath).toString()
        : QStringLiteral("qrc:/images/qposter.png");
    const QIcon appIcon(logoPath);
    QApplication::setWindowIcon(appIcon);
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    qRegisterMetaType<awa::core::DownloadItem>("awa::core::DownloadItem");

    awa::core::SettingsService settings;
    awa::core::DownloadManager manager;
    manager.setDefaultSavePath(settings.downloadDirectory());
    manager.setSpeedLimits(settings.downloadLimitKiB(), settings.uploadLimitKiB());
    manager.setChokingStrategy(
        settings.chokingAlgorithm(),
        settings.seedChokingAlgorithm(),
        settings.uploadSlots(),
        settings.optimisticSlots());
    manager.setMaxActiveDownloads(settings.maxActiveDownloads());
    manager.setDynamicBlockTuningEnabled(settings.dynamicBlockTuningEnabled());
    manager.setSeedOnCompletionEnabled(settings.seedOnCompletionEnabled());
    manager.setTrackerUrlsText(settings.trackerUrlsText());

    awa::torrent::TorrentService torrentService;
    manager.setBackend(&torrentService);

    awa::rss::RssService rssService(&manager);
    rssService.setAutoDownloadEnabled(settings.startRssAutomatically());
    awa::api::LocalApiServer apiServer(&manager);
    if (settings.startApiAutomatically()) {
        apiServer.start(static_cast<quint16>(settings.apiPort()));
    }

    TrayController trayController(appIcon);
    trayController.setLanguage(settings.language());
    QObject::connect(&settings, &awa::core::SettingsService::languageChanged,
        &trayController, &TrayController::setLanguage);
    QObject::connect(&manager, &awa::core::DownloadManager::downloadCompleted,
        &trayController, &TrayController::showDownloadCompleted);
    if (singleInstanceServer) {
        QObject::connect(singleInstanceServer.get(), &QLocalServer::newConnection, &trayController, [&trayController, server = singleInstanceServer.get()] {
            while (auto* socket = server->nextPendingConnection()) {
                QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
                socket->readAll();
                socket->disconnectFromServer();
            }
            trayController.showMainWindow();
        });
    }
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("downloadManager"), &manager);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsService"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("rssService"), &rssService);
    engine.rootContext()->setContextProperty(QStringLiteral("apiServer"), &apiServer);
    engine.rootContext()->setContextProperty(QStringLiteral("trayController"), &trayController);
    engine.rootContext()->setContextProperty(QStringLiteral("appLogoSource"), logoUrl);
    engine.rootContext()->setContextProperty(QStringLiteral("appPosterSource"), posterUrl);
    engine.rootContext()->setContextProperty(QStringLiteral("appQPosterSource"), qPosterUrl);
    engine.loadFromModule("AwaKurageDownloader", "Main");

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    trayController.setMainWindow(qobject_cast<QWindow*>(engine.rootObjects().constFirst()));
    return app.exec();
}

#include "main.moc"
