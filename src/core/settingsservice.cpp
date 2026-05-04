#include "awa/core/settingsservice.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

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

} // namespace awa::core
