// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

#include "core/formatter.h"

void run_formatter_tests() {
  CHECK_EQ_STR(sysograph::formatBytes(0), "0 B");
  CHECK_EQ_STR(sysograph::formatBytes(1536), "1.5 KiB");
  CHECK_EQ_STR(sysograph::formatBytes(2ull * 1024 * 1024), "2.0 MiB");
  CHECK_EQ_STR(sysograph::formatBytes(1536ull * 1024 * 1024 * 1024), "1.5 TiB");
  CHECK_EQ_STR(sysograph::formatBytes(100 * 1024), "100 KiB");

  CHECK_EQ_STR(sysograph::formatBytesPerSecond(1536), "1.5 KiB/s");
  CHECK_EQ_STR(sysograph::formatBytesPerSecond(0), "0 B/s");

  CHECK_EQ_STR(sysograph::formatPercent(12.34), "12.3 %");
  CHECK_EQ_STR(sysograph::formatPercent(-5), "0.0 %");
  CHECK_EQ_STR(sysograph::formatPercent(500), "100.0 %");

  CHECK_EQ_STR(sysograph::formatDurationHMS(0), "0:00:00");
  CHECK_EQ_STR(sysograph::formatDurationHMS(65), "0:01:05");
  CHECK_EQ_STR(sysograph::formatDurationHMS(3725), "1:02:05");
  CHECK_EQ_STR(sysograph::formatDurationHMS(90061), "1d 1:01:01");

  CHECK_EQ_STR(sysograph::formatUptime(30), "0m");
  CHECK_EQ_STR(sysograph::formatUptime(300), "5m");
  CHECK_EQ_STR(sysograph::formatUptime(3900), "1h 5m");
  CHECK_EQ_STR(sysograph::formatUptime(90000), "1d 1h");
}
