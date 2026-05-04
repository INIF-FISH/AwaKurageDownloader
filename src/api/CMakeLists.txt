qt_add_library(awa_api STATIC
    include/awa/api/localapiserver.h
    localapiserver.cpp
)

target_include_directories(awa_api PUBLIC include)
target_link_libraries(awa_api
    PUBLIC awa_core Qt6::Core Qt6::Network Qt6::WebSockets
)
