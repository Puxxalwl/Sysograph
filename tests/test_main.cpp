// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

int g_checks = 0;
int g_failures = 0;

int main() {
  run_procfs_tests();
  run_sampler_tests();
  run_tree_users_tests();
  run_desktop_entry_tests();
  run_service_list_tests();
  run_formatter_tests();

  std::printf("sysograph core tests: %d checks, %d failures\n", g_checks,
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
