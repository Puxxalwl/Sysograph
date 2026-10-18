// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

#include <map>
#include <vector>

#include "core/process_tree.h"
#include "core/users.h"

namespace {

sysograph::ProcessInfo makeProc(int pid, int ppid, const char* name,
                                const char* user, uint32_t uid,
                                double cpu = 0.0,
                                uint64_t rss = 0, int threads = 1) {
  sysograph::ProcessInfo p;
  p.pid = pid;
  p.ppid = ppid;
  p.name = name;
  p.state = 'S';
  p.uid = uid;
  p.user = user;
  p.cpu_percent = cpu;
  p.rss_bytes = rss;
  p.threads = threads;
  return p;
}

}  // namespace

void run_tree_users_tests() {
  // --- Process tree --------------------------------------------------------
  {
    std::vector<sysograph::ProcessInfo> procs = {
        makeProc(1, 0, "systemd", "root", 0),
        makeProc(2, 1, "kthreadd", "root", 0),
        makeProc(10, 1, "sshd", "root", 0),
        makeProc(11, 10, "bash", "alice", 1000),
        makeProc(12, 10, "sftp", "bob", 1000),
        makeProc(99, 7, "orphan", "root", 0),  // parent 7 missing -> root
    };

    std::map<int, sysograph::ProcessTreeNode> nodes;
    std::vector<int> roots;
    sysograph::buildProcessTree(procs, &nodes, &roots);

    CHECK(nodes.size() == 6);
    CHECK(roots.size() == 2);
    if (roots.size() == 2) {
      CHECK(roots[0] == 1);
      CHECK(roots[1] == 99);
    }
    CHECK(nodes.at(1).child_pids.size() == 2);   // 2 and 10
    CHECK(nodes.at(10).child_pids.size() == 2);  // 11 and 12
    CHECK(nodes.at(99).child_pids.empty());
  }
  {
    // A ppid cycle must not hang and must produce roots.
    std::vector<sysograph::ProcessInfo> procs = {
        makeProc(10, 11, "a", "root", 0),
        makeProc(11, 10, "b", "root", 0),
    };

    std::map<int, sysograph::ProcessTreeNode> nodes;
    std::vector<int> roots;
    sysograph::buildProcessTree(procs, &nodes, &roots);
    CHECK(nodes.size() == 2);
    CHECK(!roots.empty());
    bool reachable = false;
    for (int root : roots) {
      if (root == 10 || root == 11) reachable = true;
    }
    CHECK(reachable);
  }

  // --- User aggregation ----------------------------------------------------
  {
    std::vector<sysograph::ProcessInfo> procs = {
        makeProc(1, 0, "a", "alice", 1000, 10.0, 1ull << 20, 2),
        makeProc(2, 0, "b", "alice", 1000, 20.0, 2ull << 20, 3),
        makeProc(3, 0, "c", "root", 0, 5.0, 3ull << 20, 1),
    };
    const std::vector<sysograph::UserAggregate> agg =
        sysograph::aggregateByUser(procs);
    CHECK(agg.size() == 2);
    if (agg.size() == 2) {
      // alice first: 30% cpu total.
      CHECK_EQ_STR(agg[0].user, "alice");
      CHECK(agg[0].process_count == 2);
      CHECK_NEAR(agg[0].cpu_percent, 30.0, 1e-9);
      CHECK(agg[0].memory_bytes == 3ull << 20);
      CHECK(agg[0].threads == 5);
      CHECK_EQ_STR(agg[1].user, "root");
    }
  }
}
