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
    Q_INVOKABLE int downloadLimitKiB() const;
    Q_INVOKABLE void setDownloadLimitKiB(int limit);
    Q_INVOKABLE int uploadLimitKiB() const;
    Q_INVOKABLE void setUploadLimitKiB(int limit);
    Q_INVOKABLE int apiPort() const;
    Q_INVOKABLE void setApiPort(int port);
    Q_INVOKABLE bool startApiAutomatically() const;
    Q_INVOKABLE void setStartApiAutomatically(bool enabled);
    Q_INVOKABLE int chokingAlgorithm() const;
    Q_INVOKABLE void setChokingAlgorithm(int algorithm);
    Q_INVOKABLE int seedChokingAlgorithm() const;
    Q_INVOKABLE void setSeedChokingAlgorithm(int algorithm);
    Q_INVOKABLE bool seedOnCompletionEnabled() const;
    Q_INVOKABLE void setSeedOnCompletionEnabled(bool enabled);
    Q_INVOKABLE int uploadSlots() const;
    Q_INVOKABLE void setUploadSlots(int slotCount);
    Q_INVOKABLE int optimisticSlots() const;
    Q_INVOKABLE void setOptimisticSlots(int slotCount);
    Q_INVOKABLE int maxActiveDownloads() const;
    Q_INVOKABLE void setMaxActiveDownloads(int count);
    Q_INVOKABLE bool dynamicBlockTuningEnabled() const;
    Q_INVOKABLE void setDynamicBlockTuningEnabled(bool enabled);
    Q_INVOKABLE QString trackerUrlsText() const;
    Q_INVOKABLE void setTrackerUrlsText(const QString& text);
    Q_INVOKABLE QString defaultTrackerUrlsText() const;
    Q_INVOKABLE QString language() const;
    Q_INVOKABLE void setLanguage(const QString& language);
    Q_INVOKABLE bool downloadCompletionSoundEnabled() const;
    Q_INVOKABLE void setDownloadCompletionSoundEnabled(bool enabled);
    Q_INVOKABLE void playDownloadCompleteSound() const;

private:
    QString m_settingsPath;
};

} // namespace awa::core
