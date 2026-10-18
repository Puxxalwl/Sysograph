// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

#include "core/procfs.h"

void run_service_list_tests() {
  {
    const std::string output =
        "UNIT                         LOAD   ACTIVE SUB     DESCRIPTION\n"
        "accounts-daemon.service      loaded active running Accounts service\n"
        "cups.service                 loaded inactive dead    CUPS Scheduler\n"
        "sysstat-collect.service      loaded active exited  System activity\n"
        "\n"
        "LOAD   = Reflects whether the unit file was properly loaded.\n"
        "ACTIVE = The high-level unit activation state, i.e. generalization of SUB.\n"
        "SUB    = The low-level unit activation state, values depend on unit type.\n"
        "\n"
        "1 loaded units listed. Pass --all to see loaded but inactive units.\n"
        "To show all installed unit files use 'systemctl list-unit-files'.\n";
    const std::vector<sysograph::ServiceRow> rows =
        sysograph::parse_list_units(output);
    CHECK(rows.size() == 3);
    if (rows.size() == 3) {
      CHECK_EQ_STR(rows[0].unit, "accounts-daemon.service");
      CHECK_EQ_STR(rows[0].load, "loaded");
      CHECK_EQ_STR(rows[0].active, "active");
      CHECK_EQ_STR(rows[0].sub, "running");
      CHECK_EQ_STR(rows[0].description, "Accounts service");

      CHECK_EQ_STR(rows[1].unit, "cups.service");
      CHECK_EQ_STR(rows[1].active, "inactive");
      CHECK_EQ_STR(rows[1].description, "CUPS Scheduler");

      CHECK_EQ_STR(rows[2].sub, "exited");
    }
  }
  {
    // Empty / garbage output must produce an empty list.
    CHECK(sysograph::parse_list_units("").empty());
    CHECK(sysograph::parse_list_units("total garbage\n1 2\n").empty());
  }
}
