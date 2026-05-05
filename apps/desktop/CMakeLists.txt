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

qt_add_qml_module(AwaKurageDownloader
    URI AwaKurageDownloader
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/DownloadRow.qml
        qml/SidebarButton.qml
        qml/AcidButton.qml
        qml/AcidToolButton.qml
)

target_link_libraries(AwaKurageDownloader
    PRIVATE
        awa_core
        awa_torrent
        awa_rss
        awa_api
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)

set_target_properties(AwaKurageDownloader PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS AwaKurageDownloader
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

if(WIN32)
    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/../../packaging/windows/AwaKurageDownloader.cmd"
        DESTINATION .
    )
endif()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/LOGO.png"
    DESTINATION bin/resources
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/Poster.png"
    DESTINATION bin/resources
)

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
