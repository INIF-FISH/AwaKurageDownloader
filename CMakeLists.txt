cmake_minimum_required(VERSION 3.25)

project(AwaKurageDownloader
    VERSION 0.1.2
    DESCRIPTION "A modern cross-platform BitTorrent downloader"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AWA_ENABLE_PACKAGING "Enable packaging targets" ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Quick3D Network Sql WebSockets)
find_package(spdlog CONFIG REQUIRED)
find_package(LibtorrentRasterbar CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src/core)
add_subdirectory(src/torrent)
add_subdirectory(src/rss)
add_subdirectory(src/api)
add_subdirectory(apps/desktop)

if(AWA_ENABLE_PACKAGING)
    include(AwaPackaging)
endif()
