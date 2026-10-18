// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/formatter.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace sysograph {
namespace {

void splitBytes(double bytes, double* value, const char** unit) {
  static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
  size_t u = 0;
  while (bytes >= 1024.0 && u + 1 < sizeof(units) / sizeof(units[0])) {
    bytes /= 1024.0;
    ++u;
  }
  *value = bytes;
  *unit = units[u];
}

}  // namespace

std::string formatBytes(uint64_t bytes) {
  double value = 0.0;
  const char* unit = "B";
  splitBytes(static_cast<double>(bytes), &value, &unit);
  char buf[48];
  if (value >= 100.0 || bytes < 1024) {
    std::snprintf(buf, sizeof(buf), "%.0f %s", value, unit);
  } else {
    std::snprintf(buf, sizeof(buf), "%.1f %s", value, unit);
  }
  return buf;
}

std::string formatBytesPerSecond(double bytes_per_second) {
  if (bytes_per_second < 0.0) bytes_per_second = 0.0;
  double value = 0.0;
  const char* unit = "B";
  splitBytes(bytes_per_second, &value, &unit);
  char buf[48];
  if (value >= 100.0 || bytes_per_second < 1024.0) {
    std::snprintf(buf, sizeof(buf), "%.0f %s/s", value, unit);
  } else {
    std::snprintf(buf, sizeof(buf), "%.1f %s/s", value, unit);
  }
  return buf;
}

std::string formatPercent(double percent) {
  if (percent < 0.0) percent = 0.0;
  if (percent > 100.0) percent = 100.0;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.1f %%", percent);
  return buf;
}

std::string formatDurationHMS(double seconds) {
  if (seconds < 0.0 || std::isnan(seconds)) seconds = 0.0;
  const uint64_t total = static_cast<uint64_t>(seconds + 0.5);
  const uint64_t h = total / 3600;
  const uint64_t m = (total % 3600) / 60;
  const uint64_t s = total % 60;
  char buf[48];
  if (h >= 24) {
    std::snprintf(buf, sizeof(buf), "%lud %lu:%02lu:%02lu",
                  static_cast<unsigned long>(h / 24),
                  static_cast<unsigned long>(h % 24),
                  static_cast<unsigned long>(m),
                  static_cast<unsigned long>(s));
  } else {
    std::snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu",
                  static_cast<unsigned long>(h),
                  static_cast<unsigned long>(m),
                  static_cast<unsigned long>(s));
  }
  return buf;
}

std::string formatUptime(double seconds) {
  if (seconds < 0.0 || std::isnan(seconds)) seconds = 0.0;
  const uint64_t total = static_cast<uint64_t>(seconds + 0.5);
  const uint64_t d = total / 86400;
  const uint64_t h = (total % 86400) / 3600;
  const uint64_t m = (total % 3600) / 60;
  char buf[48];
  if (d > 0) {
    std::snprintf(buf, sizeof(buf), "%lud %luh",
                  static_cast<unsigned long>(d),
                  static_cast<unsigned long>(h));
  } else if (h > 0) {
    std::snprintf(buf, sizeof(buf), "%luh %lum",
                  static_cast<unsigned long>(h),
                  static_cast<unsigned long>(m));
  } else {
    std::snprintf(buf, sizeof(buf), "%lum",
                  static_cast<unsigned long>(m));
  }
  return buf;
}

std::string formatCount(double value) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.0f", value);
  return buf;
}

}  // namespace sysograph
