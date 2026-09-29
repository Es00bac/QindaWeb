# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)
project(QindaWeb VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)
include(CTest)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml Quick Network WebSockets WaylandCompositor)
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
    src/gecko_surface_host.cpp src/gecko_surface_host.h
    src/profile.cpp src/profile.h src/placement_client.cpp src/placement_client.h
    src/navigation.cpp src/navigation.h)
qt_add_library(qindaweb_core STATIC ${browser_sources})
qt_add_resources(qindaweb_core "browser" PREFIX "/" FILES
    qml/BrowserWindow.qml qml/PlacementChooser.qml data/userChrome.css)
target_link_libraries(qindaweb_core PUBLIC QindaTK::qindatk Qt6::Quick
    Qt6::WebSockets Qt6::WaylandCompositor)
target_include_directories(qindaweb_core PUBLIC src)
target_compile_options(qindaweb_core PRIVATE -Wall -Wextra -Wpedantic)
qt_add_executable(qindaweb src/main.cpp)
target_link_libraries(qindaweb PRIVATE qindaweb_core)
target_compile_definitions(qindaweb PRIVATE QINDAWEB_VERSION="${PROJECT_VERSION}")
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
    qt_add_executable(tst_browser tests/tst_browser.cpp)
    target_link_libraries(tst_browser PRIVATE qindaweb_core Qt6::Test)
    target_compile_definitions(tst_browser PRIVATE QINDAWEB_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixture.html")
    # Explicit native acceptance executable: never silently run headful Firefox
    # in routine CTest or against the user's ordinary browser profile.
endif()
