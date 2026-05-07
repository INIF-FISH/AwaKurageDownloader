#pragma once

#include "awa/core/downloaditem.h"
#include "awa/core/downloadlistmodel.h"

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantMap>

namespace awa::core {

class TorrentBackend : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~TorrentBackend() = default;

    virtual void addTorrentFile(const QString& path, const DownloadOptions& options) = 0;
    virtual void addMagnet(const QString& uri, const DownloadOptions& options) = 0;
    virtual void pause(const QString& id) = 0;
    virtual void resume(const QString& id) = 0;
    virtual void remove(const QString& id, bool removeFiles) = 0;
    virtual void setFilePriorities(const QString& id, const QVector<FilePriority>& priorities) = 0;
    virtual void setSpeedLimits(int downloadKiB, int uploadKiB) = 0;
    virtual void setChokingStrategy(int chokingAlgorithm, int seedChokingAlgorithm, int uploadSlots, int optimisticSlots) = 0;
    virtual void setMaxActiveDownloads(int count) { (void)count; }
    virtual void setDynamicBlockTuningEnabled(bool enabled) { (void)enabled; }
    virtual void setSeedOnCompletionEnabled(bool enabled) { (void)enabled; }
    virtual void setTrackers(const QStringList& trackers) {}
    virtual void loadPersistedTasks() {}

signals:
    void itemUpdated(const awa::core::DownloadItem& item);
    void itemRemoved(const QString& id);
    void errorRaised(const QString& message);
};

class DownloadManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(awa::core::DownloadListModel* downloads READ downloads CONSTANT)
    Q_PROPERTY(QString defaultSavePath READ defaultSavePath WRITE setDefaultSavePath NOTIFY defaultSavePathChanged)
    Q_PROPERTY(int downloadLimitKiB READ downloadLimitKiB NOTIFY speedLimitsChanged)
    Q_PROPERTY(int uploadLimitKiB READ uploadLimitKiB NOTIFY speedLimitsChanged)
    Q_PROPERTY(int chokingAlgorithm READ chokingAlgorithm NOTIFY chokingStrategyChanged)
    Q_PROPERTY(int seedChokingAlgorithm READ seedChokingAlgorithm NOTIFY chokingStrategyChanged)
    Q_PROPERTY(int uploadSlots READ uploadSlots NOTIFY chokingStrategyChanged)
    Q_PROPERTY(int optimisticSlots READ optimisticSlots NOTIFY chokingStrategyChanged)
    Q_PROPERTY(int maxActiveDownloads READ maxActiveDownloads NOTIFY downloadConcurrencyChanged)
    Q_PROPERTY(bool dynamicBlockTuningEnabled READ dynamicBlockTuningEnabled NOTIFY blockStrategyChanged)
    Q_PROPERTY(bool seedOnCompletionEnabled READ seedOnCompletionEnabled NOTIFY seedingBehaviorChanged)
    Q_PROPERTY(QString trackerUrlsText READ trackerUrlsText WRITE setTrackerUrlsText NOTIFY trackersChanged)

public:
    explicit DownloadManager(QObject* parent = nullptr);

    DownloadListModel* downloads();
    QString defaultSavePath() const;
    void setDefaultSavePath(const QString& path);
    int downloadLimitKiB() const;
    int uploadLimitKiB() const;
    int chokingAlgorithm() const;
    int seedChokingAlgorithm() const;
    int uploadSlots() const;
    int optimisticSlots() const;
    int maxActiveDownloads() const;
    bool dynamicBlockTuningEnabled() const;
    bool seedOnCompletionEnabled() const;
    QString trackerUrlsText() const;
    void setTrackerUrlsText(const QString& text);
    void setBackend(TorrentBackend* backend);

    Q_INVOKABLE void addTorrentFile(const QString& path, const QVariantMap& options = {});
    Q_INVOKABLE void addMagnet(const QString& uri, const QVariantMap& options = {});
    Q_INVOKABLE void pause(const QString& id);
    Q_INVOKABLE void resume(const QString& id);
    Q_INVOKABLE void remove(const QString& id, bool removeFiles);
    Q_INVOKABLE void openSavePath(const QString& id);
    Q_INVOKABLE void setSpeedLimits(int downloadKiB, int uploadKiB);
    Q_INVOKABLE void setChokingStrategy(int chokingAlgorithm, int seedChokingAlgorithm, int uploadSlots, int optimisticSlots);
    Q_INVOKABLE void setMaxActiveDownloads(int count);
    Q_INVOKABLE void setDynamicBlockTuningEnabled(bool enabled);
    Q_INVOKABLE void setSeedOnCompletionEnabled(bool enabled);

signals:
    void defaultSavePathChanged();
    void speedLimitsChanged();
    void chokingStrategyChanged();
    void downloadConcurrencyChanged();
    void blockStrategyChanged();
    void seedingBehaviorChanged();
    void trackersChanged();
    void toastRequested(const QString& message);

private:
    DownloadOptions parseOptions(const QVariantMap& options) const;
    QStringList parseTrackerUrls(const QString& text) const;

    DownloadListModel m_downloads;
    QPointer<TorrentBackend> m_backend;
    QString m_defaultSavePath;
    int m_downloadLimitKiB = 0;
    int m_uploadLimitKiB = 0;
    int m_chokingAlgorithm = 0;
    int m_seedChokingAlgorithm = 2;
    int m_uploadSlots = 8;
    int m_optimisticSlots = 1;
    int m_maxActiveDownloads = 20;
    bool m_dynamicBlockTuningEnabled = true;
    bool m_seedOnCompletionEnabled = true;
    QString m_trackerUrlsText;
};

} // namespace awa::core
