#include "awa/core/downloadlistmodel.h"
#include "awa/core/downloadmanager.h"

#include <catch2/catch_test_macros.hpp>

using namespace awa::core;

TEST_CASE("DownloadListModel inserts updates and removes items")
{
    DownloadListModel model;

    DownloadItem item;
    item.id = "abc";
    item.name = "sample";
    item.state = DownloadState::Downloading;
    item.progress = 0.25;

    model.upsert(item);
    REQUIRE(model.rowCount() == 1);
    REQUIRE(model.indexOfId("abc") == 0);
    REQUIRE(model.get(0).value("name").toString() == "sample");

    item.progress = 0.75;
    model.upsert(item);
    REQUIRE(model.rowCount() == 1);
    REQUIRE(model.get(0).value("progress").toDouble() == 0.75);

    model.removeById("abc");
    REQUIRE(model.rowCount() == 0);
}

TEST_CASE("DownloadState exposes waiting text")
{
    REQUIRE(stateToString(DownloadState::Waiting) == QStringLiteral("等待下载"));
}

TEST_CASE("DownloadManager uses default save path when options omit one")
{
    class FakeBackend final : public TorrentBackend {
    public:
        DownloadOptions lastOptions;
        void addTorrentFile(const QString&, const DownloadOptions& options) override { lastOptions = options; }
        void addMagnet(const QString&, const DownloadOptions& options) override { lastOptions = options; }
        void pause(const QString&) override {}
        void resume(const QString&) override {}
        void remove(const QString&, bool) override {}
        void setFilePriorities(const QString&, const QVector<FilePriority>&) override {}
        void setSpeedLimits(int, int) override {}
        void setChokingStrategy(int, int, int, int) override {}
    };

    DownloadManager manager;
    FakeBackend backend;
    manager.setBackend(&backend);
    manager.setDefaultSavePath("E:/Downloads");

    manager.addMagnet("magnet:?xt=urn:btih:example");
    REQUIRE(backend.lastOptions.savePath == "E:/Downloads");
}
