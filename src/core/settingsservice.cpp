#include "awa/core/settingsservice.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace awa::core {
namespace {
QString defaultTrackers()
{
    return QStringList {
        QStringLiteral("udp://tracker.opentrackr.org:1337/announce"),
        QStringLiteral("udp://tracker.torrent.eu.org:451/announce"),
        QStringLiteral("udp://tracker.openbittorrent.com:6969/announce"),
        QStringLiteral("udp://open.stealth.si:80/announce"),
        QStringLiteral("udp://tracker.moeking.me:6969/announce"),
        QStringLiteral("udp://tracker.dler.org:6969/announce"),
        QStringLiteral("udp://explodie.org:6969/announce"),
        QStringLiteral("udp://tracker.bittor.pw:1337/announce"),
        QStringLiteral("udp://tracker.opentrackr.org:6969/announce"),
        QStringLiteral("udp://open.demonii.com:1337/announce"),
        QStringLiteral("http://tracker.files.fm:6969/announce"),
        QStringLiteral("http://tracker.openbittorrent.com:80/announce"),
        QStringLiteral("https://tracker.nitrix.me:443/announce"),
        QStringLiteral("http://tracker.torrent.eu.org:451/announce")
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

} // namespace awa::core
