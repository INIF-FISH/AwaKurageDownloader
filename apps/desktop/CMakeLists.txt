set(AWA_DESKTOP_SOURCES
    main.cpp
    ../../resources/awa_resources.qrc
)

if(WIN32)
    list(APPEND AWA_DESKTOP_SOURCES windows/app.rc)
endif()

qt_add_executable(AwaKurageDownloader
    ${AWA_DESKTOP_SOURCES}
)

set_source_files_properties(qml/AwaTheme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(qml/I18n.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(../../resources/world_map_points.json
    PROPERTIES QT_RESOURCE_ALIAS maps/world_points.json
)

set_source_files_properties(../../resources/world_map_dots.png
    PROPERTIES QT_RESOURCE_ALIAS maps/world_dots.png
)

qt_add_qml_module(AwaKurageDownloader
    URI AwaKurageDownloader
    VERSION 1.0
    QML_FILES
        qml/AwaTheme.qml
        qml/I18n.qml
        qml/Main.qml
        qml/SharedGlobe.qml
        qml/DownloadRow.qml
        qml/SidebarButton.qml
        qml/AcidButton.qml
        qml/AcidToolButton.qml
    RESOURCES
        ../../resources/world_map_points.json
        ../../resources/world_map_dots.png
)

target_link_libraries(AwaKurageDownloader
    PRIVATE
        awa_core
        awa_torrent
        awa_rss
        awa_api
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Widgets
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Quick3D
)

set_target_properties(AwaKurageDownloader PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS AwaKurageDownloader
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/APP.png"
    DESTINATION bin/resources
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/Poster.png"
    DESTINATION bin/resources
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/QPoster.png"
    DESTINATION bin/resources
)

add_custom_command(TARGET AwaKurageDownloader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AwaKurageDownloader>/resources/maps"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/world_map_points.json"
        "$<TARGET_FILE_DIR:AwaKurageDownloader>/resources/maps/world_points.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/world_map_dots.png"
        "$<TARGET_FILE_DIR:AwaKurageDownloader>/resources/maps/world_dots.png"
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/world_map_points.json"
    DESTINATION bin/resources/maps
    RENAME world_points.json
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/world_map_dots.png"
    DESTINATION bin/resources/maps
    RENAME world_dots.png
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/geoip.dat")
    add_custom_command(TARGET AwaKurageDownloader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AwaKurageDownloader>/resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/geoip.dat"
            "$<TARGET_FILE_DIR:AwaKurageDownloader>/resources/geoip.dat"
    )

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/geoip.dat"
        DESTINATION bin/resources
    )
endif()

install(FILES $<TARGET_RUNTIME_DLLS:AwaKurageDownloader>
    DESTINATION bin
)

if(WIN32 AND DEFINED VCPKG_TARGET_TRIPLET)
    set(AWA_VCPKG_RUNTIME_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
    if(EXISTS "${AWA_VCPKG_RUNTIME_DIR}")
        install(DIRECTORY "${AWA_VCPKG_RUNTIME_DIR}/"
            DESTINATION bin
            FILES_MATCHING PATTERN "*.dll"
        )
    endif()
endif()

qt_generate_deploy_qml_app_script(
    TARGET AwaKurageDownloader
    OUTPUT_SCRIPT AWA_DESKTOP_DEPLOY_SCRIPT
)
install(SCRIPT ${AWA_DESKTOP_DEPLOY_SCRIPT})
