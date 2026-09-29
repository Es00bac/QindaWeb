# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)
project(QindaWeb VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)
include(CTest)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml Quick Network WebSockets WaylandCompositor DBus)
find_package(KWayland REQUIRED CONFIG)
find_library(QINDAWEB_MENU_EXPORTER NAMES qindaqt_global_menu_exporter REQUIRED)
find_library(QINDAWEB_MENU_DBUSMENU NAMES qindaqt_global_menu_dbusmenu REQUIRED)
find_library(QINDAWEB_MENU_PROTOCOL NAMES qindaqt_global_menu_protocol REQUIRED)
find_library(QINDAWEB_SETTINGS_CLIENT NAMES qindaqt_settings_client REQUIRED)
find_library(QINDAWEB_SETTINGS_PROTOCOL NAMES qindaqt_settings_protocol REQUIRED)
find_library(QINDAWEB_THEMES NAMES qindaqt_themes REQUIRED)
find_library(QINDAWEB_DESIGN_TOKENS NAMES qindaqt_design_tokens REQUIRED)
find_library(QINDAWEB_TOKENS_QML NAMES qindaqt_tokens_qml REQUIRED
    PATHS /usr/lib64/qt6/qml/QindaQt/Tokens /usr/lib/qt6/qml/QindaQt/Tokens)
find_package(QindaTK 0.1 REQUIRED CONFIG)
option(QINDAWEB_REQUIRE_DESKTOP_SDK "Require the public QindaQt placement SDK" OFF)
if(QINDAWEB_REQUIRE_DESKTOP_SDK)
    find_package(QindaQtApplicationWindowManagement 1 REQUIRED CONFIG)
else()
    find_package(QindaQtApplicationWindowManagement 1 QUIET CONFIG)
endif()
set(browser_sources
    src/browser_controller.cpp src/browser_controller.h
    src/browser_commands.cpp src/browser_view.cpp src/browser_view.h
    src/bidi_client.cpp src/bidi_client.h
    src/firefox_engine.cpp src/firefox_engine.h
    src/firefox_windows.cpp
    src/gecko_surface_host.cpp src/gecko_surface_host.h
    src/profile.cpp src/profile.h src/placement_client.cpp src/placement_client.h
    src/global_menu_source.cpp src/global_menu_source.h src/global_menu_export.cpp src/global_menu_export.h
    src/desktop_appearance.cpp src/desktop_appearance.h
    src/application_instance.cpp src/application_instance.h
    src/navigation.cpp src/navigation.h)
qt_add_library(qindaweb_core STATIC ${browser_sources})
qt_add_resources(qindaweb_core "browser" PREFIX "/" FILES
    qml/BrowserWindow.qml qml/PlacementChooser.qml data/userChrome.css)
target_link_libraries(qindaweb_core PUBLIC QindaTK::qindatk Qt6::Quick
    Qt6::WebSockets Qt6::WaylandCompositor Qt6::DBus Plasma::KWaylandClient
    ${QINDAWEB_MENU_DBUSMENU} ${QINDAWEB_MENU_EXPORTER} ${QINDAWEB_MENU_PROTOCOL}
    ${QINDAWEB_SETTINGS_CLIENT} ${QINDAWEB_SETTINGS_PROTOCOL}
    ${QINDAWEB_TOKENS_QML} ${QINDAWEB_DESIGN_TOKENS} ${QINDAWEB_THEMES})
target_include_directories(qindaweb_core PUBLIC src)
target_compile_options(qindaweb_core PRIVATE -Wall -Wextra -Wpedantic)
qt_add_executable(qindaweb src/main.cpp)
target_link_libraries(qindaweb PRIVATE qindaweb_core)
target_compile_definitions(qindaweb PRIVATE QINDAWEB_VERSION="${PROJECT_VERSION}")
get_filename_component(qindaweb_tokens_dir "${QINDAWEB_TOKENS_QML}" DIRECTORY)
file(RELATIVE_PATH qindaweb_tokens_relative "${CMAKE_INSTALL_FULL_BINDIR}" "${qindaweb_tokens_dir}")
set_target_properties(qindaweb PROPERTIES INSTALL_RPATH "$ORIGIN/${qindaweb_tokens_relative};${qindaweb_tokens_dir}")
if(QindaQtApplicationWindowManagement_FOUND)
    target_link_libraries(qindaweb_core PUBLIC QindaQt::ApplicationWindowClient)
    target_compile_definitions(qindaweb_core PRIVATE QINDAWEB_HAVE_PLACEMENT=1)
else()
    message(STATUS "QindaQt placement SDK unavailable: ordinary windows remain usable")
endif()
install(TARGETS qindaweb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/org.qindaqt.QindaWeb.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/qindaweb.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_navigation tests/tst_navigation.cpp src/navigation.cpp)
    target_include_directories(tst_navigation PRIVATE src)
    target_link_libraries(tst_navigation PRIVATE Qt6::Test)
    add_test(NAME navigation COMMAND tst_navigation)
    qt_add_executable(tst_instance tests/tst_instance.cpp src/application_instance.cpp src/application_instance.h)
    target_include_directories(tst_instance PRIVATE src)
    target_link_libraries(tst_instance PRIVATE Qt6::Test Qt6::DBus)
    find_program(QINDAWEB_DBUS_RUN_SESSION dbus-run-session)
    if(QINDAWEB_DBUS_RUN_SESSION)
        add_test(NAME launch-ipc COMMAND ${QINDAWEB_DBUS_RUN_SESSION} -- $<TARGET_FILE:tst_instance>)
    endif()
    qt_add_executable(tst_browser tests/tst_browser.cpp)
    target_link_libraries(tst_browser PRIVATE qindaweb_core Qt6::Test)
    target_compile_definitions(tst_browser PRIVATE QINDAWEB_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixture.html")
    # Explicit native acceptance executable: never silently run headful Firefox
    # in routine CTest or against the user's ordinary browser profile.
endif()
