#pragma once

#include <QObject>
#include <QString>

namespace awa::core {

class SettingsService final : public QObject {
    Q_OBJECT

public:
    explicit SettingsService(QObject* parent = nullptr);

    Q_INVOKABLE QString downloadDirectory() const;
    Q_INVOKABLE void setDownloadDirectory(const QString& path);
    Q_INVOKABLE bool startRssAutomatically() const;
    Q_INVOKABLE void setStartRssAutomatically(bool enabled);

private:
    QString m_settingsPath;
};

} // namespace awa::core
