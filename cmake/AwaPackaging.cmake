set(CPACK_PACKAGE_NAME "AwaKurageDownloader")
set(CPACK_PACKAGE_VENDOR "AwaKurage")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AwaKurageDownloader")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

set(AWA_WINDOWS_PACKAGE_FORMAT "AUTO" CACHE STRING "Windows package format: AUTO, MSI, or ZIP")
set_property(CACHE AWA_WINDOWS_PACKAGE_FORMAT PROPERTY STRINGS AUTO MSI ZIP)
set(AWA_WIX_CULTURE "en-us" CACHE STRING "WiX installer culture: en-us, zh-cn, or ja-jp")
set_property(CACHE AWA_WIX_CULTURE PROPERTY STRINGS zh-cn en-us ja-jp)

if(NOT AWA_WIX_CULTURE STREQUAL "zh-cn"
   AND NOT AWA_WIX_CULTURE STREQUAL "en-us"
   AND NOT AWA_WIX_CULTURE STREQUAL "ja-jp")
    message(FATAL_ERROR "Unsupported AWA_WIX_CULTURE='${AWA_WIX_CULTURE}'. Expected one of: zh-cn, en-us, ja-jp.")
endif()

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
        if(AWA_WIX_CULTURE STREQUAL "ja-jp")
            set(AWA_WIX_PRODUCT_LANGUAGE 1041)
            set(AWA_WIX_PRODUCT_CODEPAGE 932)
            set(AWA_WIX_DOWNGRADE_MESSAGE "[ProductName] &#x306E;&#x65B0;&#x3057;&#x3044;&#x30D0;&#x30FC;&#x30B8;&#x30E7;&#x30F3;&#x304C;&#x65E2;&#x306B;&#x30A4;&#x30F3;&#x30B9;&#x30C8;&#x30FC;&#x30EB;&#x3055;&#x308C;&#x3066;&#x3044;&#x307E;&#x3059;&#x3002;&#x30BB;&#x30C3;&#x30C8;&#x30A2;&#x30C3;&#x30D7;&#x3092;&#x7D42;&#x4E86;&#x3057;&#x307E;&#x3059;&#x3002;")
            set(AWA_WIX_ROOT_FEATURE_TITLE "AwaKurageDownloader")
            set(AWA_WIX_ROOT_FEATURE_DESCRIPTION "&#x30B3;&#x30A2;&#x30A2;&#x30D7;&#x30EA;&#x30B1;&#x30FC;&#x30B7;&#x30E7;&#x30F3;&#x30D5;&#x30A1;&#x30A4;&#x30EB;&#x3068;&#x30B9;&#x30BF;&#x30FC;&#x30C8;&#x30E1;&#x30CB;&#x30E5;&#x30FC;&#x306E;&#x30E9;&#x30F3;&#x30C1;&#x30E3;&#x30FC;&#x3002;")
            set(AWA_WIX_START_MENU_SHORTCUT_NAME "AwaKurageDownloader (&#x30E1;&#x30A4;&#x30F3;&#x30A2;&#x30D7;&#x30EA;)")
            set(AWA_WIX_START_MENU_SHORTCUT_DESCRIPTION "AwaKurageDownloader &#x30E1;&#x30A4;&#x30F3;&#x30A2;&#x30D7;&#x30EA;&#x3092;&#x8D77;&#x52D5;")
            set(AWA_WIX_SHORTCUT_DESCRIPTION "AwaKurageDownloader &#x3092;&#x8D77;&#x52D5;")
            set(AWA_WIX_SHORTCUT_FEATURE_TITLE "&#x30C7;&#x30B9;&#x30AF;&#x30C8;&#x30C3;&#x30D7;&#x30B7;&#x30E7;&#x30FC;&#x30C8;&#x30AB;&#x30C3;&#x30C8;&#x3092;&#x4F5C;&#x6210;")
            set(AWA_WIX_SHORTCUT_FEATURE_DESCRIPTION "AwaKurageDownloader &#x306E;&#x30C7;&#x30B9;&#x30AF;&#x30C8;&#x30C3;&#x30D7;&#x30B7;&#x30E7;&#x30FC;&#x30C8;&#x30AB;&#x30C3;&#x30C8;&#x3092;&#x4F5C;&#x6210;&#x3057;&#x307E;&#x3059;&#x3002;")
        elseif(AWA_WIX_CULTURE STREQUAL "en-us")
            set(AWA_WIX_PRODUCT_LANGUAGE 1033)
            set(AWA_WIX_PRODUCT_CODEPAGE 1252)
            set(AWA_WIX_DOWNGRADE_MESSAGE "A newer version of [ProductName] is already installed. Setup will now exit.")
            set(AWA_WIX_ROOT_FEATURE_TITLE "AwaKurageDownloader")
            set(AWA_WIX_ROOT_FEATURE_DESCRIPTION "Core application files and Start Menu launcher.")
            set(AWA_WIX_START_MENU_SHORTCUT_NAME "AwaKurageDownloader (Main App)")
            set(AWA_WIX_START_MENU_SHORTCUT_DESCRIPTION "Launch the main AwaKurageDownloader app")
            set(AWA_WIX_SHORTCUT_DESCRIPTION "Launch AwaKurageDownloader")
            set(AWA_WIX_SHORTCUT_FEATURE_TITLE "Create desktop shortcut")
            set(AWA_WIX_SHORTCUT_FEATURE_DESCRIPTION "Create a desktop shortcut for AwaKurageDownloader.")
        else()
            set(AWA_WIX_PRODUCT_LANGUAGE 2052)
            set(AWA_WIX_PRODUCT_CODEPAGE 936)
            set(AWA_WIX_DOWNGRADE_MESSAGE "&#x5DF2;&#x5B89;&#x88C5;&#x8F83;&#x65B0;&#x7248;&#x672C;&#x7684; [ProductName]&#x3002;&#x5B89;&#x88C5;&#x7A0B;&#x5E8F;&#x5C06;&#x7ACB;&#x5373;&#x9000;&#x51FA;&#x3002;")
            set(AWA_WIX_ROOT_FEATURE_TITLE "AwaKurageDownloader")
            set(AWA_WIX_ROOT_FEATURE_DESCRIPTION "&#x6838;&#x5FC3;&#x5E94;&#x7528;&#x6587;&#x4EF6;&#x548C;&#x5F00;&#x59CB;&#x83DC;&#x5355;&#x542F;&#x52A8;&#x9879;&#x3002;")
            set(AWA_WIX_START_MENU_SHORTCUT_NAME "AwaKurageDownloader (&#x4E3B;&#x5E94;&#x7528;)")
            set(AWA_WIX_START_MENU_SHORTCUT_DESCRIPTION "&#x542F;&#x52A8; AwaKurageDownloader &#x4E3B;&#x5E94;&#x7528;")
            set(AWA_WIX_SHORTCUT_DESCRIPTION "&#x542F;&#x52A8; AwaKurageDownloader")
            set(AWA_WIX_SHORTCUT_FEATURE_TITLE "&#x521B;&#x5EFA;&#x684C;&#x9762;&#x56FE;&#x6807;")
            set(AWA_WIX_SHORTCUT_FEATURE_DESCRIPTION "&#x4E3A; AwaKurageDownloader &#x521B;&#x5EFA;&#x684C;&#x9762;&#x5FEB;&#x6377;&#x65B9;&#x5F0F;&#x3002;")
        endif()

        set(AWA_WIX_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpack-wix")
        file(MAKE_DIRECTORY "${AWA_WIX_GENERATED_DIR}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-main-template.wxs.in"
            "${AWA_WIX_GENERATED_DIR}/wix-main-template.wxs"
            @ONLY
        )
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-shortcuts.wxs.in"
            "${AWA_WIX_GENERATED_DIR}/wix-shortcuts.wxs"
            @ONLY
        )
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-shortcuts-patch.xml.in"
            "${AWA_WIX_GENERATED_DIR}/wix-shortcuts-patch.xml"
            @ONLY
        )

        set(CPACK_GENERATOR "WIX")
        set(CPACK_WIX_UPGRADE_GUID "7F8DB8F5-2C9F-4F01-9C6D-3BBF7A7B8B74")
        set(CPACK_WIX_UI_REF "WixUI_FeatureTree")
        set(CPACK_WIX_CULTURES "${AWA_WIX_CULTURE}")
        set(CPACK_WIX_TEMPLATE "${AWA_WIX_GENERATED_DIR}/wix-main-template.wxs")
        set(CPACK_WIX_PROGRAM_MENU_FOLDER "AwaKurageDownloader")
        set(CPACK_WIX_PATCH_FILE "${AWA_WIX_GENERATED_DIR}/wix-shortcuts-patch.xml")
        set(CPACK_WIX_EXTRA_SOURCES "${AWA_WIX_GENERATED_DIR}/wix-shortcuts.wxs")
        set(CPACK_WIX_ROOT_FEATURE_TITLE "${AWA_WIX_ROOT_FEATURE_TITLE}")
        set(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "${AWA_WIX_ROOT_FEATURE_DESCRIPTION}")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
            set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
        endif()
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://example.invalid/awakurage")
        set(CPACK_PACKAGE_FILE_NAME "AwaKurageDownloader-${PROJECT_VERSION}-win64-${AWA_WIX_CULTURE}")
    elseif(AWA_WINDOWS_PACKAGE_FORMAT STREQUAL "MSI")
        message(FATAL_ERROR "WiX Toolset v3 candle.exe/light.exe were not found. Install WiX v3 or add it to PATH, then re-run CMake configure.")
    else()
        message(WARNING "WiX candle/light were not found. CPack will generate a portable ZIP instead of an MSI.")
        set(CPACK_GENERATOR "ZIP")
        set(CPACK_PACKAGE_FILE_NAME "AwaKurageDownloader-${PROJECT_VERSION}-win64-portable")
    endif()
endif()

include(CPack)
