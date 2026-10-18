// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal test harness: no external framework dependency, keeping the
// dependency surface of the project (and thus license auditing) simple.
#pragma once

#include <cmath>
#include <cstdio>
#include <string>

extern int g_checks;
extern int g_failures;

#define CHECK(cond)                                                    \
  do {                                                                 \
    ++g_checks;                                                        \
    if (!(cond)) {                                                     \
      ++g_failures;                                                    \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,     \
                   #cond);                                             \
    }                                                                  \
  } while (0)

#define CHECK_EQ_STR(a, b)                                             \
  do {                                                                 \
    ++g_checks;                                                        \
    if (std::string(a) != std::string(b)) {                            \
      ++g_failures;                                                    \
      std::fprintf(stderr, "FAIL %s:%d: \"%s\" != \"%s\"\n",           \
                   __FILE__, __LINE__, std::string(a).c_str(),         \
                   std::string(b).c_str());                            \
    }                                                                  \
  } while (0)

#define CHECK_NEAR(a, b, tol)                                          \
  do {                                                                 \
    ++g_checks;                                                        \
    if (std::fabs((a) - (b)) > (tol)) {                                \
      ++g_failures;                                                    \
      std::fprintf(stderr, "FAIL %s:%d: %f != %f (±%f)\n",             \
                   __FILE__, __LINE__, static_cast<double>(a),         \
                   static_cast<double>(b), static_cast<double>(tol));  \
    }                                                                  \
  } while (0)

// Declared suites (one translation unit each).
void run_procfs_tests();
void run_sampler_tests();
void run_tree_users_tests();
void run_desktop_entry_tests();
void run_service_list_tests();
void run_formatter_tests();
