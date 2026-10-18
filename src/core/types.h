// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Core data model shared by platform readers, calculations and UI.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sysograph {

// Raw cumulative CPU jiffy counters, as exported by /proc/stat.
struct CpuTimes {
  uint64_t user = 0;
  uint64_t nice = 0;
  uint64_t system = 0;
  uint64_t idle = 0;
  uint64_t iowait = 0;
  uint64_t irq = 0;
  uint64_t softirq = 0;
  uint64_t steal = 0;

  uint64_t total() const {
    return user + nice + system + idle + iowait + irq + softirq + steal;
  }
  uint64_t busy() const {
    return user + nice + system + irq + softirq + steal;
  }
};

// One process row, as read from /proc/<pid>/...
struct ProcessInfo {
  int pid = 0;
  int ppid = 0;
  std::string name;        // comm
  char state = '?';        // R, S, D, Z, T, ...
  std::string state_text;  // human-readable state (filled by UI/i18n)
  uint32_t uid = 0;
  std::string user;        // resolved user name, or the numeric uid
  double cpu_percent = 0.0;  // 0..100 of total machine capacity
  uint64_t rss_bytes = 0;
  uint64_t vsize_bytes = 0;
  uint64_t shared_bytes = 0;
  int threads = 1;
  int priority = 0;
  int nice = 0;
  uint64_t utime = 0;   // cumulative ticks
  uint64_t stime = 0;   // cumulative ticks
  uint64_t starttime_ticks = 0;  // ticks since boot
  std::string cmdline;
  int last_cpu = -1;
};

struct MemoryInfo {
  uint64_t total = 0;
  uint64_t free = 0;
  uint64_t available = 0;
  uint64_t buffers = 0;
  uint64_t cached = 0;
  uint64_t swap_total = 0;
  uint64_t swap_free = 0;

  uint64_t used() const { return total > available ? total - available : 0; }
  uint64_t swap_used() const { return swap_total > swap_free ? swap_total - swap_free : 0; }
};

struct NetTotals {
  uint64_t rx_bytes = 0;
  uint64_t tx_bytes = 0;
};

struct DiskTotals {
  uint64_t reads = 0;           // completed read ops
  uint64_t writes = 0;          // completed write ops
  uint64_t sectors_read = 0;    // 512-byte sectors
  uint64_t sectors_written = 0;
};

// Everything the UI needs for one refresh tick.
struct SampledMetrics {
  std::vector<ProcessInfo> processes;

  double cpu_total_percent = 0.0;
  std::vector<double> cpu_per_core_percent;
  unsigned cpu_cores = 0;

  MemoryInfo memory;
  NetTotals net;
  DiskTotals disk;

  // Rates (per second) computed from the previous sample.
  double net_rx_bps = 0.0;  // bytes per second
  double net_tx_bps = 0.0;
  double disk_read_bps = 0.0;   // bytes per second
  double disk_write_bps = 0.0;
  double disk_read_ops = 0.0;
  double disk_write_ops = 0.0;

  unsigned procs_running = 0;
  unsigned procs_blocked = 0;
  unsigned threads_total = 0;
  double uptime_seconds = 0.0;

  uint64_t timestamp_ms = 0;  // monotonic milliseconds
};

// Convert a /proc stat state character to a stable identifier used by
// translations ("running", "sleeping", ...). Original mapping.
const char* process_state_key(char state);

}  // namespace sysograph
