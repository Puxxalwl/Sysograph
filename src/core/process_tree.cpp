// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/process_tree.h"

#include <algorithm>
#include <set>

namespace sysograph {

void buildProcessTree(const std::vector<ProcessInfo>& processes,
                      std::map<int, ProcessTreeNode>* nodes,
                      std::vector<int>* root_pids) {
  nodes->clear();
  root_pids->clear();

  std::map<int, const ProcessInfo*> byPid;
  for (const auto& p : processes) byPid[p.pid] = &p;

  for (const auto& [pid, info] : byPid) {
    ProcessTreeNode node;
    node.info = info;
    (*nodes)[pid] = node;
  }
  for (const auto& [pid, info] : byPid) {
    const int ppid = info->ppid;
    // A process is a root when its parent is absent from the snapshot
    // (already exited, hidden by permissions, or the kernel's pid 0).
    if (ppid <= 0 || !byPid.count(ppid)) {
      root_pids->push_back(pid);
      continue;
    }
    (*nodes)[ppid].child_pids.push_back(pid);
  }

  // Guard against ppid cycles: walk up from every node with a visited set;
  // if the ancestry loops, treat the node as a root.
  for (auto& [pid, node] : *nodes) {
    std::set<int> seen;
    int cur = pid;
    bool cyclic = false;
    while (cur > 0 && byPid.count(cur)) {
      if (!seen.insert(cur).second) {
        cyclic = true;
        break;
      }
      cur = byPid.at(cur)->ppid;
    }
    if (cyclic) {
      // Break the cycle: this node becomes a root and is detached from
      // its (looping) parent.
      const int ppid = byPid.at(pid)->ppid;
      if (ppid > 0 && nodes->count(ppid)) {
        auto& siblings = (*nodes)[ppid].child_pids;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), pid),
                       siblings.end());
      }
      root_pids->push_back(pid);
    }
  }
  for (auto& [pid, node] : *nodes) {
    std::sort(node.child_pids.begin(), node.child_pids.end());
  }
  std::sort(root_pids->begin(), root_pids->end());
}

}  // namespace sysograph
