#include "awa/core/settingsservice.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>
#include <cstdio>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace awa::core {
namespace {
QString defaultTrackers()
{
    return QStringList {
        QStringLiteral("http://1337.abcvg.info:80/announce"),
        QStringLiteral("http://bt1.archive.org:6969/announce"),
        QStringLiteral("http://bt2.archive.org:6969/announce"),
        QStringLiteral("http://nyaa.tracker.wf:7777/announce"),
        QStringLiteral("http://torrentsmd.com:8080/announce"),
        QStringLiteral("http://tracker.bt4g.com:2095/announce"),
        QStringLiteral("http://tracker.dhitechnical.com:6969/announce"),
        QStringLiteral("http://tracker.mywaifu.best:6969/announce"),
        QStringLiteral("http://tracker.renfei.net:8080/announce"),
        QStringLiteral("http://tracker.skyts.net:6969/announce"),
        QStringLiteral("http://tracker.tritan.gg:8080/announce"),
        QStringLiteral("http://tracker.waaa.moe:6969/announce"),
        QStringLiteral("http://tracker.xn--djrq4gl4hvoi.top:80/announce"),
        QStringLiteral("http://tracker3.ctix.cn:8080/announce"),
        QStringLiteral("http://www.wareztorrent.com:80/announce"),
        QStringLiteral("https://1337.abcvg.info:443/announce"),
        QStringLiteral("https://http1.torrust-tracker-demo.com:443/announce"),
        QStringLiteral("https://t.213891.xyz:443/announce"),
        QStringLiteral("https://torrents.tmtime.dev:443/announce"),
        QStringLiteral("https://tr.abiir.top:443/announce"),
        QStringLiteral("https://tr.nyacat.pw:443/announce"),
        QStringLiteral("https://tracker.7471.top:443/announce"),
        QStringLiteral("https://tracker.kuroy.me:443/announce"),
        QStringLiteral("https://tracker.nekomi.cn:443/announce"),
        QStringLiteral("https://tracker.pmman.tech:443/announce"),
        QStringLiteral("https://tracker.qingwapt.org:443/announce"),
        QStringLiteral("https://tracker.zhuqiy.com:443/announce"),
        QStringLiteral("https://tracker1.520.jp:443/announce"),
        QStringLiteral("udp://archive.torrentonline.cc:42069/announce"),
        QStringLiteral("udp://bittorrent-tracker.e-n-c-r-y-p-t.net:1337/announce"),
        QStringLiteral("udp://evan.im:6969/announce"),
        QStringLiteral("udp://martin-gebhardt.eu:25/announce"),
        QStringLiteral("udp://ns575949.ip-51-222-82.net:6969/announce"),
        QStringLiteral("udp://open.demonii.com:1337/announce"),
        QStringLiteral("udp://open.stealth.si:80/announce"),
        QStringLiteral("udp://open.tracker.ink:6969/announce"),
        QStringLiteral("udp://opentor.org:2710/announce"),
        QStringLiteral("udp://p4p.arenabg.com:1337/announce"),
        QStringLiteral("udp://retracker.hotplug.ru:2710/announce"),
        QStringLiteral("udp://torrents.tmtime.dev:6969/announce"),
        QStringLiteral("udp://tr4ck3r.duckdns.org:6969/announce"),
        QStringLiteral("udp://tracker-udp.gbitt.info:80/announce"),
        QStringLiteral("udp://tracker.004430.xyz:1337/announce"),
        QStringLiteral("udp://tracker.1h.is:1337/announce"),
        QStringLiteral("udp://tracker.bittor.pw:1337/announce"),
        QStringLiteral("udp://tracker.bluefrog.pw:2710/announce"),
        QStringLiteral("udp://tracker.breizh.pm:6969/announce"),
        QStringLiteral("udp://tracker.corpscorp.online:80/announce"),
        QStringLiteral("udp://tracker.dler.com:6969/announce"),
        QStringLiteral("udp://tracker.flatuslifir.is:6969/announce"),
        QStringLiteral("udp://tracker.fnix.net:6969/announce"),
        QStringLiteral("udp://tracker.gmi.gd:6969/announce"),
        QStringLiteral("udp://tracker.hismz.cn:6969/announce"),
        QStringLiteral("udp://tracker.iperson.xyz:6969/announce"),
        QStringLiteral("udp://tracker.nyaa.vc:6969/announce"),
        QStringLiteral("udp://tracker.opentorrent.top:6969/announce"),
        QStringLiteral("udp://tracker.opentrackr.org:1337/announce"),
        QStringLiteral("udp://tracker.plx.im:6969/announce"),
        QStringLiteral("udp://tracker.qu.ax:6969/announce"),
        QStringLiteral("udp://tracker.skyts.net:6969/announce"),
        QStringLiteral("udp://tracker.srv00.com:6969/announce"),
        QStringLiteral("udp://tracker.t-1.org:6969/announce"),
        QStringLiteral("udp://tracker.torrent.eu.org:451/announce"),
        QStringLiteral("udp://tracker.torrust-demo.com:6969/announce"),
        QStringLiteral("udp://tracker.tryhackx.org:6969/announce"),
        QStringLiteral("udp://tracker.wildkat.net:6969/announce"),
        QStringLiteral("udp://uabits.today:6990/announce"),
        QStringLiteral("udp://udp.tracker.projectk.org:23333/announce"),
        QStringLiteral("udp://udp1.torrust-tracker-demo.com:6969/announce"),
        QStringLiteral("udp://vito-tracker.duckdns.org:6969/announce"),
        QStringLiteral("udp://vito-tracker.space:2095/announce"),
        QStringLiteral("udp://vito-tracker.space:6969/announce"),
        QStringLiteral("udp://www.nartlof.com:6969/announce"),
        QStringLiteral("udp://yuptracker-eu.gaijinent.com:27022/announce"),
        QStringLiteral("wss://tracker.openwebtorrent.com:443/announce")
    }.join(QLatin1Char('\n'));
}
} // namespace

SettingsService::SettingsService(QObject* parent)
    : QObject(parent)
{
    const auto configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_settingsPath = configDir + QStringLiteral("/settings.ini");
}

QString SettingsService::downloadDirectory() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("downloads/directory"),
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
}

void SettingsService::setDownloadDirectory(const QString& path)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("downloads/directory"), path);
}

bool SettingsService::startRssAutomatically() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("rss/startAutomatically"), false).toBool();
}

void SettingsService::setStartRssAutomatically(bool enabled)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("rss/startAutomatically"), enabled);
}

int SettingsService::downloadLimitKiB() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("downloads/downloadLimitKiB"), 0).toInt();
}

void SettingsService::setDownloadLimitKiB(int limit)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("downloads/downloadLimitKiB"), std::max(0, limit));
}

int SettingsService::uploadLimitKiB() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("downloads/uploadLimitKiB"), 0).toInt();
}

void SettingsService::setUploadLimitKiB(int limit)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("downloads/uploadLimitKiB"), std::max(0, limit));
}

int SettingsService::apiPort() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("api/port"), 18777).toInt();
}

void SettingsService::setApiPort(int port)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("api/port"), std::clamp(port, 1024, 65534));
}

bool SettingsService::startApiAutomatically() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("api/startAutomatically"), true).toBool();
}

void SettingsService::setStartApiAutomatically(bool enabled)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("api/startAutomatically"), enabled);
}

int SettingsService::chokingAlgorithm() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/chokingAlgorithm"), 0).toInt();
}

void SettingsService::setChokingAlgorithm(int algorithm)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/chokingAlgorithm"), std::clamp(algorithm, 0, 1));
}

int SettingsService::seedChokingAlgorithm() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/seedChokingAlgorithm"), 2).toInt();
}

void SettingsService::setSeedChokingAlgorithm(int algorithm)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/seedChokingAlgorithm"), std::clamp(algorithm, 0, 2));
}

bool SettingsService::seedOnCompletionEnabled() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/seedOnCompletionEnabled"), true).toBool();
}

void SettingsService::setSeedOnCompletionEnabled(bool enabled)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/seedOnCompletionEnabled"), enabled);
}

int SettingsService::uploadSlots() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/uploadSlots"), 8).toInt();
}

void SettingsService::setUploadSlots(int slotCount)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/uploadSlots"), std::clamp(slotCount, 1, 200));
}

int SettingsService::optimisticSlots() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/optimisticSlots"), 1).toInt();
}

void SettingsService::setOptimisticSlots(int slotCount)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/optimisticSlots"), std::clamp(slotCount, 0, 10));
}

int SettingsService::maxActiveDownloads() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/maxActiveDownloads"), 20).toInt();
}

void SettingsService::setMaxActiveDownloads(int count)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/maxActiveDownloads"), std::clamp(count, 1, 200));
}

bool SettingsService::dynamicBlockTuningEnabled() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/dynamicBlockTuningEnabled"), true).toBool();
}

void SettingsService::setDynamicBlockTuningEnabled(bool enabled)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/dynamicBlockTuningEnabled"), enabled);
}

QString SettingsService::trackerUrlsText() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("torrent/trackerUrls"), defaultTrackers()).toString();
}

void SettingsService::setTrackerUrlsText(const QString& text)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("torrent/trackerUrls"), text.trimmed());
}

QString SettingsService::defaultTrackerUrlsText() const
{
    return defaultTrackers();
}

QString SettingsService::language() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    const QString language = settings.value(QStringLiteral("ui/language"), QStringLiteral("zh_CN")).toString();
    return language == QStringLiteral("en_US") || language == QStringLiteral("ja_JP")
        ? language
        : QStringLiteral("zh_CN");
}

void SettingsService::setLanguage(const QString& language)
{
    const QString normalized = language == QStringLiteral("en_US") || language == QStringLiteral("ja_JP")
        ? language
        : QStringLiteral("zh_CN");
    if (normalized == this->language()) {
        return;
    }

    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("ui/language"), normalized);
    emit languageChanged(normalized);
}

bool SettingsService::downloadCompletionSoundEnabled() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("notifications/downloadCompletionSoundEnabled"), true).toBool();
}

void SettingsService::setDownloadCompletionSoundEnabled(bool enabled)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("notifications/downloadCompletionSoundEnabled"), enabled);
}

bool SettingsService::closeToTrayMessageShown() const
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("notifications/closeToTrayMessageShown"), false).toBool();
}

void SettingsService::setCloseToTrayMessageShown(bool shown)
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("notifications/closeToTrayMessageShown"), shown);
}

void SettingsService::playDownloadCompleteSound() const
{
#ifdef Q_OS_WIN
    MessageBeep(MB_OK);
#else
    std::fputc('\a', stderr);
    std::fflush(stderr);
#endif
}

} // namespace awa::core
