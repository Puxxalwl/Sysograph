// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Reader for the Linux procfs pseudo-filesystem. All functions accept a
// root directory (defaulting to "/proc") so that unit tests can feed a
// fake tree; every parser is therefore fully deterministic and testable.
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/types.h"

namespace sysograph {

// --- Free parsing functions (unit-testable on strings) -----------------

// Parses one "cpu ..." line from /proc/stat. Returns false if malformed.
bool parse_stat_cpu_line(const std::string& line, bool* is_total,
                         CpuTimes* out);

// Parses a /proc/<pid>/stat line into a ProcessInfo (pid/ppid/name/state/
// times/threads/priority/nice/vsize/rss/last_cpu). Returns false if the
// line is too short to be a valid stat line.
bool parse_process_stat(const std::string& line, ProcessInfo* out);

// Parses the whole /proc/<pid>/status file body, filling uid, threads and
// shared memory (RssFile + RssShmem). Unknown lines are ignored.
void parse_process_status(const std::string& content, ProcessInfo* out);

// Parses /proc/meminfo content. Values are converted from kB to bytes.
bool parse_meminfo(const std::string& content, MemoryInfo* out);

// Parses /proc/net/dev content, accumulating all interfaces except "lo".
bool parse_net_dev(const std::string& content, NetTotals* out);

// Parses /proc/diskstats content, accumulating physical devices
// (virtual/loop devices such as loop*, ram*, zram*, dm- and sr* are
// skipped, mirroring what a user would consider "real" disks).
bool parse_diskstats(const std::string& content, DiskTotals* out);

// Parses "systemctl list-units --plain --no-legend" output lines into
// rows. Header/legend/empty lines are skipped.
struct ServiceRow {
  std::string unit;
  std::string load;
  std::string active;
  std::string sub;
  std::string description;
};
std::vector<ServiceRow> parse_list_units(const std::string& output);

// --- Procfs reader class ------------------------------------------------

class Procfs {
 public:
  explicit Procfs(std::filesystem::path root = "/proc");

  bool readCpuTimes(CpuTimes* total, std::vector<CpuTimes>* per_core);
  bool readMemory(MemoryInfo* out);
  bool readNetwork(NetTotals* out);
  bool readDisks(DiskTotals* out);
  bool readUptime(double* seconds);
  bool readProcsCounts(unsigned* running, unsigned* blocked);

  // Lists all processes without cpu_percent (deltas are computed by
  // the Sampler). Processes whose stat vanished mid-read are skipped.
  std::vector<ProcessInfo> listProcesses();

  // Resolves a uid to a user name using getpwuid_r with a cache.
  // Falls back to the decimal uid on failure.
  std::string userName(uint32_t uid);

  // Extra details for the details pane (empty / false on failure).
  static std::string readLinkField(const std::filesystem::path& proc,
                                   const char* field);
  static int countFileDescriptors(const std::filesystem::path& proc);

  long clockTicksPerSecond() const;
  long pageSize() const;

 private:
  std::filesystem::path root_;
  std::unordered_map<uint32_t, std::string> user_cache_;
};

}  // namespace sysograph
