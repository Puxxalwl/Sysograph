# Sysograph — a free, original Linux system task manager.
# Copyright (C) 2025 The Sysograph Project Contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(sysograph
  VERSION 0.1.0
  DESCRIPTION "A free system task manager for Linux"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

# ---------------------------------------------------------------------------
# Core (platform + calculations): deliberately free of GUI dependencies so
# it stays unit-testable everywhere.
# ---------------------------------------------------------------------------
add_library(sysograph_core STATIC
  src/core/types.cpp
  src/core/procfs.cpp
  src/core/sampler.cpp
  src/core/process_tree.cpp
  src/core/users.cpp
  src/core/desktop_entry.cpp
  src/core/xdg_autostart.cpp
  src/core/formatter.cpp
  src/core/i18n.cpp
)
target_include_directories(sysograph_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(sysograph_core PRIVATE -Wall -Wextra -Wpedantic)

# ---------------------------------------------------------------------------
# GUI executable
# ---------------------------------------------------------------------------
add_executable(sysograph
  src/main.cpp
  src/ui/graphs.cpp
  src/ui/processes_page.cpp
  src/ui/tree_page.cpp
  src/ui/performance_page.cpp
  src/ui/users_page.cpp
  src/ui/startup_page.cpp
  src/ui/services_page.cpp
  src/ui/main_window.cpp
)
target_link_libraries(sysograph PRIVATE sysograph_core PkgConfig::GTK4)
target_compile_definitions(sysograph PRIVATE
  SYSSOGRAPH_VERSION="${PROJECT_VERSION}"
  SYSSOGRAPH_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/sysograph"
  SYSSOGRAPH_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
# GtkTreeView & friends are deprecated in newer GTK 4 releases but remain
# fully functional; suppress the noise while migration is planned.
target_compile_options(sysograph PRIVATE
  -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)

# ---------------------------------------------------------------------------
# Unit tests (core only — no display required)
# ---------------------------------------------------------------------------
enable_testing()
add_executable(sysograph_tests
  tests/test_main.cpp
  tests/test_procfs.cpp
  tests/test_sampler.cpp
  tests/test_tree_users.cpp
  tests/test_desktop_entry.cpp
  tests/test_service_list.cpp
  tests/test_formatter.cpp
)
target_link_libraries(sysograph_tests PRIVATE sysograph_core)
target_compile_options(sysograph_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME core COMMAND sysograph_tests)

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS sysograph RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/style.css DESTINATION ${CMAKE_INSTALL_DATADIR}/sysograph)
install(FILES data/icons/org.sysograph.Sysograph.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES packaging/org.sysograph.Sysograph.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/org.sysograph.Sysograph.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES LICENSE NOTICE THIRD_PARTY_LICENSES.md
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/sysograph)
install(DIRECTORY THIRD_PARTY/LICENSES
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/sysograph/THIRD_PARTY)

# ---------------------------------------------------------------------------
# Packaging (DEB / RPM); see packaging/cpack-config.cmake for per-format
# dependency handling.
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "sysograph")
set(CPACK_PACKAGE_VENDOR "The Sysograph Project")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A free system task manager for Linux (processes, performance, users, services)")
set(CPACK_PACKAGE_DESCRIPTION
"Sysograph is an original, free (GPL-3.0+) system task manager for Linux
built with C++ and GTK 4. It shows processes with a details pane and a
process tree, performance graphs for CPU, memory, disk and network,
per-user resource aggregation, XDG autostart entries and systemd
services. It runs without root privileges and collects no data.")
set(CPACK_PACKAGE_HOMEPAGE_URL "")
set(CPACK_PACKAGE_CONTACT "The Sysograph Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES ON)
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PROJECT_CONFIG_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/cpack-config.cmake")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "The Sysograph Project")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Base")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")

include(CPack)
