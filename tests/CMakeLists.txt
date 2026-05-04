add_executable(awa_tests
    core_tests.cpp
    torrent_tests.cpp
)

target_link_libraries(awa_tests
    PRIVATE awa_core awa_torrent Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(awa_tests)
