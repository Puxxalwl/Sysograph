// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Aggregates per-process resource usage by user account.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/types.h"

namespace sysograph {

struct UserAggregate {
  uint32_t uid = 0;
  std::string user;
  unsigned process_count = 0;
  double cpu_percent = 0.0;    // sum over that user's processes
  uint64_t memory_bytes = 0;   // sum of RSS
  unsigned threads = 0;
};

// Aggregates a snapshot by uid; sorted by total cpu percent descending.
std::vector<UserAggregate> aggregateByUser(
    const std::vector<ProcessInfo>& processes);

}  // namespace sysograph
