// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/users.h"

#include <algorithm>

namespace sysograph {

std::vector<UserAggregate> aggregateByUser(
    const std::vector<ProcessInfo>& processes) {
  std::map<uint32_t, UserAggregate> byUid;
  for (const auto& p : processes) {
    UserAggregate& agg = byUid[p.uid];
    agg.uid = p.uid;
    if (agg.user.empty()) agg.user = p.user;
    agg.process_count += 1;
    agg.cpu_percent += p.cpu_percent;
    agg.memory_bytes += p.rss_bytes;
    agg.threads += static_cast<unsigned>(p.threads);
  }
  std::vector<UserAggregate> out;
  out.reserve(byUid.size());
  for (auto& [uid, agg] : byUid) out.push_back(std::move(agg));
  std::sort(out.begin(), out.end(), [](const UserAggregate& a,
                                       const UserAggregate& b) {
    if (a.cpu_percent != b.cpu_percent) return a.cpu_percent > b.cpu_percent;
    return a.uid < b.uid;
  });
  return out;
}

}  // namespace sysograph
