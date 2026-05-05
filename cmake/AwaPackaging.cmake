set(CPACK_PACKAGE_NAME "AwaKurageDownloader")
set(CPACK_PACKAGE_VENDOR "AwaKurage")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AwaKurageDownloader")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_EXECUTABLES "AwaKurageDownloader" "AwaKurageDownloader")

set(AWA_WINDOWS_PACKAGE_FORMAT "AUTO" CACHE STRING "Windows package format: AUTO, MSI, or ZIP")
set_property(CACHE AWA_WINDOWS_PACKAGE_FORMAT PROPERTY STRINGS AUTO MSI ZIP)

if(WIN32)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
    include(InstallRequiredSystemLibraries)
endif()

if(WIN32)
    find_program(AWA_WIX_CANDLE candle.exe
        PATHS
            "$ENV{ProgramFiles\(x86\)}/WiX Toolset v3.14/bin"
            "$ENV{ProgramFiles\(x86\)}/WiX Toolset v3.11/bin"
            "$ENV{ProgramFiles}/WiX Toolset v3.14/bin"
            "$ENV{ProgramFiles}/WiX Toolset v3.11/bin"
    )
    find_program(AWA_WIX_LIGHT light.exe
        PATHS
            "$ENV{ProgramFiles\(x86\)}/WiX Toolset v3.14/bin"
            "$ENV{ProgramFiles\(x86\)}/WiX Toolset v3.11/bin"
            "$ENV{ProgramFiles}/WiX Toolset v3.14/bin"
            "$ENV{ProgramFiles}/WiX Toolset v3.11/bin"
    )

    if(AWA_WINDOWS_PACKAGE_FORMAT STREQUAL "ZIP")
        set(CPACK_GENERATOR "ZIP")
        set(CPACK_PACKAGE_FILE_NAME "AwaKurageDownloader-${PROJECT_VERSION}-win64-portable")
    elseif(AWA_WIX_CANDLE AND AWA_WIX_LIGHT)
        set(CPACK_GENERATOR "WIX")
        set(CPACK_WIX_UPGRADE_GUID "7F8DB8F5-2C9F-4F01-9C6D-3BBF7A7B8B74")
        set(CPACK_WIX_UI_REF "WixUI_FeatureTree")
        set(CPACK_WIX_CULTURES "zh-cn")
        set(CPACK_WIX_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-main-template.wxs")
        set(CPACK_WIX_PROGRAM_MENU_FOLDER "AwaKurageDownloader")
        set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-shortcuts-patch.xml")
        set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-shortcuts.wxs")
        set(CPACK_WIX_ROOT_FEATURE_TITLE "AwaKurageDownloader")
        set(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "Core application files and Start Menu launcher.")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
            set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
        endif()
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://example.invalid/awakurage")
        set(CPACK_PACKAGE_FILE_NAME "AwaKurageDownloader-${PROJECT_VERSION}-win64")
    elseif(AWA_WINDOWS_PACKAGE_FORMAT STREQUAL "MSI")
        message(FATAL_ERROR "WiX Toolset v3 candle.exe/light.exe were not found. Install WiX v3 or add it to PATH, then re-run CMake configure.")
    else()
        message(WARNING "WiX candle/light were not found. CPack will generate a portable ZIP instead of an MSI.")
        set(CPACK_GENERATOR "ZIP")
        set(CPACK_PACKAGE_FILE_NAME "AwaKurageDownloader-${PROJECT_VERSION}-win64-portable")
    endif()
endif()

include(CPack)
