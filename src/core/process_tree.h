// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Builds a parent/child hierarchy out of a flat process list. Cycles are
// broken deterministically so that corrupted ppids cannot hang the UI.
#pragma once

#include <map>
#include <vector>

#include "core/types.h"

namespace sysograph {

struct ProcessTreeNode {
  const ProcessInfo* info = nullptr;
  std::vector<int> child_pids;
};

// Returns a pid -> node map. Roots (processes whose parent is not in the
// snapshot, including kernel thread 0 and pid 2 with a missing parent)
// are listed in *root_pids, ordered by pid. Children are ordered by pid.
void buildProcessTree(const std::vector<ProcessInfo>& processes,
                      std::map<int, ProcessTreeNode>* nodes,
                      std::vector<int>* root_pids);

}  // namespace sysograph
