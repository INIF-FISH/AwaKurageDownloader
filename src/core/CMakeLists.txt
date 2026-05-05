qt_add_library(awa_core STATIC
    include/awa/core/downloaditem.h
    include/awa/core/downloadlistmodel.h
    include/awa/core/downloadmanager.h
    include/awa/core/settingsservice.h
    downloadlistmodel.cpp
    downloadmanager.cpp
    settingsservice.cpp
)

target_include_directories(awa_core PUBLIC include)
target_link_libraries(awa_core
    PUBLIC Qt6::Core Qt6::Gui Qt6::Sql
    PRIVATE spdlog::spdlog
)
