#include "awa/api/localapiserver.h"
#include "awa/core/downloaditem.h"
#include "awa/core/downloadmanager.h"
#include "awa/core/settingsservice.h"
#include "awa/rss/rssservice.h"
#include "awa/torrent/torrentservice.h"

#include <QGuiApplication>
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
#include <QQuickStyle>
#include <QUrl>

namespace {
void installStartupLogger()
{
    static QFile logFile(QCoreApplication::applicationDirPath() + QStringLiteral("/startup.log"));
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
    QGuiApplication app(argc, argv);
    installStartupLogger();
    QGuiApplication::setApplicationName(QStringLiteral("AwaKurageDownloader"));
    QGuiApplication::setOrganizationName(QStringLiteral("AwaKurage"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    const QString packagedLogoPngPath = QGuiApplication::applicationDirPath() + QStringLiteral("/resources/LOGO.png");
    const QString logoPath = QFileInfo::exists(packagedLogoPngPath)
        ? packagedLogoPngPath
        : QStringLiteral(":/images/logo.png");
    const QString logoUrl = logoPath.startsWith(QLatin1Char(':'))
        ? QStringLiteral("qrc:/images/logo.png")
        : QUrl::fromLocalFile(logoPath).toString();
    const QString packagedPosterPath = QGuiApplication::applicationDirPath() + QStringLiteral("/resources/Poster.png");
    const QString posterUrl = QFileInfo::exists(packagedPosterPath)
        ? QUrl::fromLocalFile(packagedPosterPath).toString()
        : QStringLiteral("qrc:/images/poster.png");
    QGuiApplication::setWindowIcon(QIcon(logoPath));
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

    awa::torrent::TorrentService torrentService;
    manager.setBackend(&torrentService);

    awa::rss::RssService rssService(&manager);
    rssService.setAutoDownloadEnabled(settings.startRssAutomatically());
    awa::api::LocalApiServer apiServer(&manager);
    if (settings.startApiAutomatically()) {
        apiServer.start(static_cast<quint16>(settings.apiPort()));
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("downloadManager"), &manager);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsService"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("rssService"), &rssService);
    engine.rootContext()->setContextProperty(QStringLiteral("apiServer"), &apiServer);
    engine.rootContext()->setContextProperty(QStringLiteral("appLogoSource"), logoUrl);
    engine.rootContext()->setContextProperty(QStringLiteral("appPosterSource"), posterUrl);
    engine.loadFromModule("AwaKurageDownloader", "Main");

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
