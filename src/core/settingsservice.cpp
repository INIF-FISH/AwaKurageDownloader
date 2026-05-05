#include "awa/core/settingsservice.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace awa::core {

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

} // namespace awa::core
