qt_add_library(awa_torrent STATIC
    include/awa/torrent/magnetutils.h
    include/awa/torrent/torrentservice.h
    magnetutils.cpp
    torrentservice.cpp
)

target_include_directories(awa_torrent PUBLIC include)
target_link_libraries(awa_torrent
    PUBLIC awa_core Qt6::Core LibtorrentRasterbar::torrent-rasterbar
    PRIVATE spdlog::spdlog
)
