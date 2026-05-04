#pragma once

#include "awa/core/downloaditem.h"
#include "awa/core/downloadlistmodel.h"

#include <QObject>
#include <QPointer>
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

signals:
    void itemUpdated(const awa::core::DownloadItem& item);
    void itemRemoved(const QString& id);
    void errorRaised(const QString& message);
};

class DownloadManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(awa::core::DownloadListModel* downloads READ downloads CONSTANT)
    Q_PROPERTY(QString defaultSavePath READ defaultSavePath WRITE setDefaultSavePath NOTIFY defaultSavePathChanged)

public:
    explicit DownloadManager(QObject* parent = nullptr);

    DownloadListModel* downloads();
    QString defaultSavePath() const;
    void setDefaultSavePath(const QString& path);
    void setBackend(TorrentBackend* backend);

    Q_INVOKABLE void addTorrentFile(const QString& path, const QVariantMap& options = {});
    Q_INVOKABLE void addMagnet(const QString& uri, const QVariantMap& options = {});
    Q_INVOKABLE void pause(const QString& id);
    Q_INVOKABLE void resume(const QString& id);
    Q_INVOKABLE void remove(const QString& id, bool removeFiles);
    Q_INVOKABLE void setSpeedLimits(int downloadKiB, int uploadKiB);

signals:
    void defaultSavePathChanged();
    void toastRequested(const QString& message);

private:
    DownloadOptions parseOptions(const QVariantMap& options) const;

    DownloadListModel m_downloads;
    QPointer<TorrentBackend> m_backend;
    QString m_defaultSavePath;
};

} // namespace awa::core
