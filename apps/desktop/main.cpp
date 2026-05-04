#include "awa/api/localapiserver.h"
#include "awa/core/downloaditem.h"
#include "awa/core/downloadmanager.h"
#include "awa/core/settingsservice.h"
#include "awa/rss/rssservice.h"
#include "awa/torrent/torrentservice.h"

#include <QGuiApplication>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("AwaKurageDownloader"));
    QGuiApplication::setOrganizationName(QStringLiteral("AwaKurage"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    const QString packagedLogoPath = QGuiApplication::applicationDirPath() + QStringLiteral("/resources/LOGO.png");
    const QString logoUrl = QFileInfo::exists(packagedLogoPath)
        ? QUrl::fromLocalFile(packagedLogoPath).toString()
        : QStringLiteral("qrc:/images/logo.png");
    QGuiApplication::setWindowIcon(QIcon(QFileInfo::exists(packagedLogoPath)
        ? packagedLogoPath
        : QStringLiteral(":/images/logo.png")));
    QQuickStyle::setStyle(QStringLiteral("Material"));

    qRegisterMetaType<awa::core::DownloadItem>("awa::core::DownloadItem");

    awa::core::SettingsService settings;
    awa::core::DownloadManager manager;
    manager.setDefaultSavePath(settings.downloadDirectory());

    awa::torrent::TorrentService torrentService;
    manager.setBackend(&torrentService);

    awa::rss::RssService rssService(&manager);
    rssService.setAutoDownloadEnabled(settings.startRssAutomatically());
    awa::api::LocalApiServer apiServer(&manager);
    apiServer.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("downloadManager"), &manager);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsService"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("rssService"), &rssService);
    engine.rootContext()->setContextProperty(QStringLiteral("apiServer"), &apiServer);
    engine.rootContext()->setContextProperty(QStringLiteral("appLogoSource"), logoUrl);
    engine.loadFromModule("AwaKurageDownloader", "Main");

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
