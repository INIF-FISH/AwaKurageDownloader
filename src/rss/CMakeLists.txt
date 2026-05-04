qt_add_library(awa_rss STATIC
    include/awa/rss/rssservice.h
    rssservice.cpp
)

target_include_directories(awa_rss PUBLIC include)
target_link_libraries(awa_rss
    PUBLIC awa_core Qt6::Core Qt6::Network
)
