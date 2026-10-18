// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>

#include "core/sampler.h"

namespace {

std::filesystem::path makeTempProc() {
  static int counter = 0;
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() /
      ("sysograph_sampler_" + std::to_string(::getpid()) + "_" +
       std::to_string(++counter));
  std::filesystem::create_directories(dir / "20");
  return dir;
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << content;
}

// cpu: user nice system idle iowait irq softirq steal
std::string cpuLine(unsigned user, unsigned nice, unsigned system,
                    unsigned idle) {
  return "cpu  " + std::to_string(user) + " " + std::to_string(nice) + " " +
         std::to_string(system) + " " + std::to_string(idle) + " 0 0 0 0\n";
}

std::string procStat(unsigned utime, unsigned stime) {
  // pid (comm) state ppid pgrp session tty tpgid flags minflt cminflt
  // majflt cmajflt utime stime cutime cstime priority nice threads itreal
  // starttime vsize rss ... last_cpu
  return "20 (my app) R 10 20 20 0 -1 4194560 9 0 0 0 " +
         std::to_string(utime) + " " + std::to_string(stime) +
         " 0 0 20 0 3 0 5 123 512 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1";
}

}  // namespace

void run_sampler_tests() {
  // --- Pure rate math ------------------------------------------------------
  CHECK_NEAR(sysograph::process_cpu_percent(25, 100), 25.0, 1e-9);
  CHECK_NEAR(sysograph::process_cpu_percent(200, 100), 100.0, 1e-9);  // clamped
  CHECK_NEAR(sysograph::process_cpu_percent(0, 0), 0.0, 1e-9);
  CHECK_NEAR(sysograph::per_second_rate(100, 1000.0), 100.0, 1e-9);
  CHECK_NEAR(sysograph::per_second_rate(100, 0.0), 0.0, 1e-9);
  {
    sysograph::CpuTimes delta;
    delta.user = 10;
    delta.system = 10;
    delta.idle = 80;
    CHECK_NEAR(sysograph::core_busy_percent(delta), 20.0, 1e-9);
  }

  // --- Sampler over a mutating fake tree -----------------------------------
  {
    const std::filesystem::path root = makeTempProc();
    writeFile(root / "stat", cpuLine(100, 0, 50, 200));
    writeFile(root / "20" / "stat", procStat(10, 5));
    writeFile(root / "20" / "status",
              "Name:\tmy app\nUid:\t1000 1000 1000 1000\nThreads:\t3\n");
    writeFile(root / "meminfo", "MemTotal: 2000 kB\nMemAvailable: 1000 kB\n");
    writeFile(root / "uptime", "100.0 100.0\n");

    auto sampler = std::make_unique<sysograph::Sampler>(
        std::make_unique<sysograph::Procfs>(root));

    sysograph::SampledMetrics first = sampler->sample();
    CHECK(first.processes.size() == 1);
    CHECK_NEAR(first.cpu_total_percent, 0.0, 1e-9);  // first sample: no delta
    CHECK_NEAR(first.processes[0].cpu_percent, 0.0, 1e-9);
    CHECK(first.memory.total == 2000ull * 1024);
    CHECK(first.memory.used() == 1000ull * 1024);

    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    // cpu busy total grows by 50 jiffies; total grows by 200
    // (user 100->150, idle 200->350).
    writeFile(root / "stat", cpuLine(150, 0, 50, 350));
    // process burns 25 more jiffies (10+5 -> 30+10).
    writeFile(root / "20" / "stat", procStat(30, 10));

    sysograph::SampledMetrics second = sampler->sample();
    CHECK(second.processes.size() == 1);
    // busy delta 50 / total delta 200 => 25%; process delta 25/200 = 12.5%.
    CHECK_NEAR(second.cpu_total_percent, 25.0, 5.0);
    CHECK_NEAR(second.processes[0].cpu_percent, 12.5, 5.0);

    // Process vanished -> next sample drops it without crashing.
    std::filesystem::remove_all(root / "20");
    sysograph::SampledMetrics third = sampler->sample();
    CHECK(third.processes.empty());

    std::filesystem::remove_all(root);
  }
}
