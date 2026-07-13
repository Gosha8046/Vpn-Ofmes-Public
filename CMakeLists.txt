cmake_minimum_required(VERSION 3.21)

project(VpnOfmes
    VERSION 1.0.0
    DESCRIPTION "Vpn-Ofmes - a WireGuard/OpenVPN client for Windows"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Language / toolchain settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt's meta-object compiler, resource compiler and UI compiler need to run
# on our sources automatically.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/Logger.cpp
    src/core/ServerProfile.cpp
    src/core/ConfigManager.cpp
    src/core/CredentialStore.cpp
    src/core/VpnEngine.cpp
    src/core/WireGuardEngine.cpp
    src/core/OpenVpnEngine.cpp
    src/core/ConnectionManager.cpp
    src/core/NetworkMonitor.cpp
    src/core/IpAddressService.cpp
    src/core/UpdateChecker.cpp
)

set(UI_SOURCES
    src/ui/StatusIndicator.cpp
    src/ui/SpeedWidget.cpp
    src/ui/TrayManager.cpp
    src/ui/HomePage.cpp
    src/ui/SettingsPage.cpp
    src/ui/LogsPage.cpp
    src/ui/MainWindow.cpp
)

# Headers are listed explicitly (rather than relying on the .cpp/.h being
# next to each other) because they live under include/ while their .cpp
# counterparts live under src/. Listing them as target sources is what lets
# CMAKE_AUTOMOC find the Q_OBJECT classes declared in them.
set(CORE_HEADERS
    include/vpnofmes/Version.h
    include/vpnofmes/core/Logger.h
    include/vpnofmes/core/ServerProfile.h
    include/vpnofmes/core/ConfigManager.h
    include/vpnofmes/core/CredentialStore.h
    include/vpnofmes/core/VpnEngine.h
    include/vpnofmes/core/WireGuardEngine.h
    include/vpnofmes/core/OpenVpnEngine.h
    include/vpnofmes/core/ConnectionManager.h
    include/vpnofmes/core/NetworkMonitor.h
    include/vpnofmes/core/IpAddressService.h
    include/vpnofmes/core/UpdateChecker.h
)

set(UI_HEADERS
    include/vpnofmes/ui/StatusIndicator.h
    include/vpnofmes/ui/SpeedWidget.h
    include/vpnofmes/ui/TrayManager.h
    include/vpnofmes/ui/HomePage.h
    include/vpnofmes/ui/SettingsPage.h
    include/vpnofmes/ui/LogsPage.h
    include/vpnofmes/ui/MainWindow.h
)

set(APP_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${CORE_HEADERS}
    ${UI_HEADERS}
    resources/resources.qrc
)

# A real Windows GUI application (no console window). On other platforms
# (used only for local compile-checks of the portable parts of the code)
# this simply falls back to a normal executable.
if(WIN32)
    add_executable(VpnOfmes WIN32 ${APP_SOURCES})
else()
    add_executable(VpnOfmes ${APP_SOURCES})
endif()

target_include_directories(VpnOfmes PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(VpnOfmes PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

target_link_libraries(VpnOfmes PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
)

if(WIN32)
    # Windows SDK headers gate the Vista-era IP Helper API (GetIfTable2,
    # MIB_IF_TABLE2, used by NetworkMonitor) behind _WIN32_WINNT/
    # NTDDI_VERSION. The MSVC project CMake generates doesn't define
    # these high enough by default, which fails with "undeclared
    # identifier" errors; pin them to Windows 10 explicitly.
    target_compile_definitions(VpnOfmes PRIVATE
        _WIN32_WINNT=0x0A00
        NTDDI_VERSION=0x0A000000
    )

    # IP Helper API (NetworkMonitor's interface byte counters) and
    # Crypt32 (CredentialStore's DPAPI-based secret storage).
    target_link_libraries(VpnOfmes PRIVATE iphlpapi crypt32 ws2_32)
endif()

# ---------------------------------------------------------------------------
# Ship the runtime assets (stylesheet, icons, default config, sample VPN
# configs) next to the built executable so applicationDirPath()-relative
# lookups in ConfigManager/MainWindow work straight out of the build tree,
# not just from an installed package.
# ---------------------------------------------------------------------------
add_custom_command(TARGET VpnOfmes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:VpnOfmes>/styles"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/styles/dark_purple.qss"
            "$<TARGET_FILE_DIR:VpnOfmes>/styles/dark_purple.qss"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/config"
            "$<TARGET_FILE_DIR:VpnOfmes>/config"
    COMMENT "Copying Vpn-Ofmes runtime assets next to the executable"
)

# ---------------------------------------------------------------------------
# Installation / packaging (produces the release artifact)
# ---------------------------------------------------------------------------
install(TARGETS VpnOfmes RUNTIME DESTINATION .)
install(DIRECTORY styles/ DESTINATION styles)
install(DIRECTORY config/ DESTINATION config)
install(FILES README.md DESTINATION .)

# Bundle the Qt runtime DLLs next to the executable on Windows using
# Qt's own deployment tool, if it can be found alongside qmake/Qt6::Widgets.
if(WIN32)
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
    if(_qt6_qmake_location)
        get_filename_component(_qt_bin_dir "${_qt6_qmake_location}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
        if(WINDEPLOYQT_EXECUTABLE)
            install(CODE "
                execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --release --no-translations
                                        \"\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:VpnOfmes>\")
            ")
        endif()
    endif()
endif()

set(CPACK_PACKAGE_NAME "Vpn-Ofmes")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Vpn-Ofmes")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vpn-Ofmes")
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
else()
    set(CPACK_GENERATOR "ZIP")
endif()
include(CPack)
