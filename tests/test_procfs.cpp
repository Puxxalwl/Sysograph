// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

#include "core/procfs.h"

namespace {

std::filesystem::path makeTempProc() {
  static int counter = 0;
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() /
      ("sysograph_test_" + std::to_string(::getpid()) + "_" +
       std::to_string(++counter));
  std::filesystem::create_directories(dir);
  return dir;
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << content;
}

}  // namespace

void run_procfs_tests() {
  // --- /proc/<pid>/stat parsing -----------------------------------------
  {
    const std::string line =
        "42 (my proc) R 40 42 42 0 -1 4194560 123 0 0 0 5 3 0 0 20 0 2 0 "
        "999999 123456789 512 0 0 0 0 0 0 0 0 0 0 0 0 0 0 3";
    sysograph::ProcessInfo p;
    CHECK(sysograph::parse_process_stat(line, &p));
    CHECK(p.pid == 42);
    CHECK_EQ_STR(p.name, "my proc");
    CHECK(p.state == 'R');
    CHECK(p.ppid == 40);
    CHECK(p.utime == 5);
    CHECK(p.stime == 3);
    CHECK(p.priority == 20);
    CHECK(p.nice == 0);
    CHECK(p.threads == 2);
    CHECK(p.starttime_ticks == 999999);
    CHECK(p.vsize_bytes == 123456789);
    CHECK(p.rss_bytes == 512);
    CHECK(p.last_cpu == 3);
  }
  {
    // Degenerate inputs must be rejected, not crash.
    sysograph::ProcessInfo p;
    CHECK(!sysograph::parse_process_stat("", &p));
    CHECK(!sysograph::parse_process_stat("garbage without parens", &p));
    CHECK(!sysograph::parse_process_stat("12 (only-name", &p));
  }

  // --- /proc/<pid>/status parsing ----------------------------------------
  {
    sysograph::ProcessInfo p;
    sysograph::parse_process_status(
        "Name:\tmy\nUid:\t1000\t1000\t1000\t1000\nThreads:\t3\n"
        "RssFile:\t128 kB\nRssShmem:\t64 kB\n",
        &p);
    CHECK(p.uid == 1000);
    CHECK(p.threads == 3);
    CHECK(p.shared_bytes == 192u * 1024u);
  }
  {
    // Empty status (permission denied on some systems) leaves defaults.
    sysograph::ProcessInfo p;
    sysograph::parse_process_status("", &p);
    CHECK(p.uid == 0);
    CHECK(p.threads == 1);
  }

  // --- /proc/stat cpu lines ----------------------------------------------
  {
    bool is_total = false;
    sysograph::CpuTimes t;
    CHECK(sysograph::parse_stat_cpu_line("cpu  100 5 50 200 10 5 2 1", &is_total, &t));
    CHECK(is_total);
    CHECK(t.user == 100);
    CHECK(t.nice == 5);
    CHECK(t.system == 50);
    CHECK(t.idle == 200);
    CHECK(t.total() == 373);
    CHECK(t.busy() == 163);

    CHECK(sysograph::parse_stat_cpu_line("cpu0 1 0 0 4 0 0 0 0", &is_total, &t));
    CHECK(!is_total);
    CHECK(t.total() == 5);

    CHECK(!sysograph::parse_stat_cpu_line("procs_running 4", &is_total, &t));
    CHECK(!sysograph::parse_stat_cpu_line("cpu 1 2 3", &is_total, &t));  // <4 fields
  }

  // --- /proc/meminfo ------------------------------------------------------
  {
    sysograph::MemoryInfo mem;
    CHECK(sysograph::parse_meminfo(
        "MemTotal:       1000 kB\nMemFree:         200 kB\n"
        "MemAvailable:    500 kB\nBuffers:          10 kB\n"
        "Cached:           20 kB\nSwapTotal:       100 kB\n"
        "SwapFree:         25 kB\n",
        &mem));
    CHECK(mem.total == 1000ull * 1024);
    CHECK(mem.available == 500ull * 1024);
    CHECK(mem.used() == 500ull * 1024);
    CHECK(mem.swap_total == 100ull * 1024);
    CHECK(mem.swap_used() == 75ull * 1024);
  }

  // --- /proc/net/dev ------------------------------------------------------
  {
    sysograph::NetTotals net;
    CHECK(sysograph::parse_net_dev(
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "  lo:       100 1 0 0 0 0 0 0        100 1 0 0 0 0 0 0\n"
        "  eth0:     1000 2 0 0 0 0 0 0        500 1 0 0 0 0 0 0\n"
        "  wlan0:     300 4 0 0 0 0 0 0         42 4 0 0 0 0 0 0\n",
        &net));
    CHECK(net.rx_bytes == 1300);
    CHECK(net.tx_bytes == 542);
  }

  // --- /proc/diskstats ----------------------------------------------------
  {
    sysograph::DiskTotals disk;
    CHECK(sysograph::parse_diskstats(
        " 8 0 sda 100 5 2000 10 50 5 1000 5 0 0 0\n"
        " 7 0 loop0 1 1 1 1 1 1 1 1 0 0 0\n"
        " 259 0 nvme0n1 10 2 200 3 20 4 400 4 0 0 0\n",
        &disk));
    CHECK(disk.reads == 110);
    CHECK(disk.sectors_read == 2200);
    CHECK(disk.writes == 70);
    CHECK(disk.sectors_written == 1400);
  }

  // --- Procfs class against a fake tree -----------------------------------
  {
    const std::filesystem::path root = makeTempProc();
    writeFile(root / "stat",
              "cpu  100 5 50 200 10 5 2 1\ncpu0 50 2 25 100 5 2 1 0\n"
              "cpu1 50 3 25 100 5 3 1 1\nprocs_running 2\nprocs_blocked 1\n");
    writeFile(root / "meminfo",
              "MemTotal: 1000 kB\nMemAvailable: 500 kB\n");
    writeFile(root / "net" / "dev", "  eth0: 1000 2 0 0 0 0 0 0 500 1 0 0 0 0 0 0\n");
    writeFile(root / "diskstats", " 8 0 sda 100 5 2000 10 50 5 1000 5 0 0 0\n");
    writeFile(root / "uptime", "12345.67 23456.78\n");

    writeFile(root / "10" / "stat",
              "10 (kworker) S 2 0 0 0 -1 0 0 0 0 0 0 0 0 20 0 1 0 100 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
    writeFile(root / "10" / "status", "Name:\tkworker\nUid:\t0 0 0 0\nThreads:\t1\n");
    writeFile(root / "10" / "cmdline", "");

    writeFile(root / "20" / "stat",
              "20 (my app) R 10 20 20 0 -1 4194560 9 0 0 0 7 4 0 0 20 0 3 0 5 123 512 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1");
    writeFile(root / "20" / "status",
              "Name:\tmy app\nUid:\t1000 1000 1000 1000\nThreads:\t3\n");
    writeFile(root / "20" / "cmdline", std::string("/usr/bin/my\0app\0arg1", 20));

    // A directory that is not a pid must be ignored.
    std::filesystem::create_directories(root / "irq");

    sysograph::Procfs procfs(root);
    sysograph::CpuTimes total;
    std::vector<sysograph::CpuTimes> cores;
    CHECK(procfs.readCpuTimes(&total, &cores));
    CHECK(total.total() == 373);
    CHECK(cores.size() == 2);

    sysograph::MemoryInfo mem;
    CHECK(procfs.readMemory(&mem));
    CHECK(mem.total == 1000ull * 1024);

    sysograph::NetTotals net;
    CHECK(procfs.readNetwork(&net));
    CHECK(net.rx_bytes == 1000);

    sysograph::DiskTotals disk;
    CHECK(procfs.readDisks(&disk));
    CHECK(disk.reads == 100);

    double uptime = 0.0;
    CHECK(procfs.readUptime(&uptime));
    CHECK_NEAR(uptime, 12345.67, 0.01);

    unsigned running = 0, blocked = 0;
    CHECK(procfs.readProcsCounts(&running, &blocked));
    CHECK(running == 2);
    CHECK(blocked == 1);

    auto processes = procfs.listProcesses();
    CHECK(processes.size() == 2);
    if (processes.size() == 2) {
      CHECK(processes[0].pid == 10);
      CHECK(processes[1].pid == 20);
      CHECK_EQ_STR(processes[1].name, "my app");
      CHECK_EQ_STR(processes[1].cmdline, "/usr/bin/my app arg1");
      CHECK_EQ_STR(processes[0].cmdline, "");  // kernel thread
      CHECK(processes[1].rss_bytes == 512u * 4096u);
      CHECK(processes[1].threads == 3);
      CHECK(processes[1].ppid == 10);
    }

    std::filesystem::remove_all(root);
  }
}
