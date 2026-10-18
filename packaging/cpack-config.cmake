# Sysograph — a free, original Linux system task manager.
# Copyright (C) 2025 The Sysograph Project Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Per-generator dependency overrides, referenced from the root CMakeLists
# via CPACK_PROJECT_CONFIG_FILE. CPack evaluates this file once per
# generator, so runtime dependencies can differ between DEB and RPM.

if(CPACK_GENERATOR STREQUAL "DEB")
  # libgtk-4-1 on Debian/12+, Ubuntu 22.04+; resolved additionally by
  # SHLIBDEPS in the root file, this line is the safety net.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS
      "libgtk-4-1 (>= 4.6)")
elseif(CPACK_GENERATOR STREQUAL "RPM")
  set(CPACK_RPM_PACKAGE_REQUIRES "gtk4 >= 4.6")
endif()
