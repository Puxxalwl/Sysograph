// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/sampler.h"

#include <algorithm>
#include <cmath>
#include <ctime>

namespace sysograph {
namespace {

// Simple monotonic clock source, so the core stays GLib-free and testable.
double monotonic_time_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<double>(ts.tv_sec) * 1000.0 +
         static_cast<double>(ts.tv_nsec) / 1e6;
}

// Unsigned subtraction saturating at zero (counters can reset).
CpuTimes subtract(const CpuTimes& a, const CpuTimes& b) {
  CpuTimes r;
  auto sat = [](uint64_t x, uint64_t y) { return x > y ? x - y : 0; };
  r.user = sat(a.user, b.user);
  r.nice = sat(a.nice, b.nice);
  r.system = sat(a.system, b.system);
  r.idle = sat(a.idle, b.idle);
  r.iowait = sat(a.iowait, b.iowait);
  r.irq = sat(a.irq, b.irq);
  r.softirq = sat(a.softirq, b.softirq);
  r.steal = sat(a.steal, b.steal);
  return r;
}

}  // namespace

double process_cpu_percent(uint64_t proc_delta, uint64_t total_delta) {
  if (total_delta == 0) return 0.0;
  double pct = 100.0 * static_cast<double>(proc_delta) /
               static_cast<double>(total_delta);
  return std::clamp(pct, 0.0, 100.0);
}

double core_busy_percent(const CpuTimes& delta) {
  const uint64_t total = delta.total();
  if (total == 0) return 0.0;
  return std::clamp(100.0 * static_cast<double>(delta.busy()) /
                        static_cast<double>(total),
                    0.0, 100.0);
}

double per_second_rate(uint64_t counter_delta, double elapsed_ms) {
  if (elapsed_ms <= 0.0) return 0.0;
  return static_cast<double>(counter_delta) * 1000.0 / elapsed_ms;
}

Sampler::Sampler(std::unique_ptr<Procfs> procfs)
    : procfs_(std::move(procfs)) {}

SampledMetrics Sampler::sample() {
  SampledMetrics m;

  const uint64_t now_ms = static_cast<uint64_t>(
      std::max(0.0, floor(0.5 + monotonic_time_ms())));
  const double elapsed_ms =
      first_sample_ ? 0.0 : static_cast<double>(now_ms - prev_ms_);

  CpuTimes total;
  std::vector<CpuTimes> cores;
  const bool have_cpu = procfs_->readCpuTimes(&total, &cores);

  if (have_cpu && !first_sample_) {
    const uint64_t total_delta =
        total.total() > prev_total_.total()
            ? total.total() - prev_total_.total()
            : 0;
    m.cpu_total_percent = core_busy_percent(subtract(total, prev_total_));
    m.cpu_per_core_percent.reserve(cores.size());
    for (size_t i = 0; i < cores.size() && i < prev_cores_.size(); ++i) {
      m.cpu_per_core_percent.push_back(
          core_busy_percent(subtract(cores[i], prev_cores_[i])));
    }
    for (size_t i = prev_cores_.size(); i < cores.size(); ++i) {
      m.cpu_per_core_percent.push_back(0.0);
    }
  } else if (have_cpu) {
    m.cpu_per_core_percent.assign(cores.size(), 0.0);
  }
  m.cpu_cores = static_cast<unsigned>(cores.size());

  m.processes = procfs_->listProcesses();
  m.threads_total = 0;
  if (!first_sample_ && have_cpu) {
    const uint64_t total_delta =
        total.total() > prev_total_.total()
            ? total.total() - prev_total_.total()
            : 0;
    for (auto& p : m.processes) {
      m.threads_total += static_cast<unsigned>(p.threads);
      const uint64_t ticks = p.utime + p.stime;
      auto it = prev_proc_ticks_.find(p.pid);
      if (it != prev_proc_ticks_.end() && ticks >= it->second) {
        p.cpu_percent =
            process_cpu_percent(ticks - it->second, total_delta);
      }
    }
  } else {
    for (const auto& p : m.processes) {
      m.threads_total += static_cast<unsigned>(p.threads);
    }
  }
  prev_proc_ticks_.clear();
  for (const auto& p : m.processes) {
    prev_proc_ticks_[p.pid] = p.utime + p.stime;
  }

  procfs_->readMemory(&m.memory);

  NetTotals net;
  if (procfs_->readNetwork(&net) && !first_sample_) {
    m.net_rx_bps = per_second_rate(net.rx_bytes - prev_net_.rx_bytes,
                                   elapsed_ms);
    m.net_tx_bps = per_second_rate(net.tx_bytes - prev_net_.tx_bytes,
                                   elapsed_ms);
  }
  m.net = net;

  DiskTotals disk;
  if (procfs_->readDisks(&disk) && !first_sample_) {
    m.disk_read_bps =
        per_second_rate((disk.sectors_read - prev_disk_.sectors_read) * 512,
                        elapsed_ms);
    m.disk_write_bps = per_second_rate(
        (disk.sectors_written - prev_disk_.sectors_written) * 512,
        elapsed_ms);
    m.disk_read_ops =
        per_second_rate(disk.reads - prev_disk_.reads, elapsed_ms);
    m.disk_write_ops =
        per_second_rate(disk.writes - prev_disk_.writes, elapsed_ms);
  }
  m.disk = disk;

  unsigned running = 0, blocked = 0;
  procfs_->readProcsCounts(&running, &blocked);
  m.procs_running = running;
  m.procs_blocked = blocked;

  double uptime = 0.0;
  if (procfs_->readUptime(&uptime)) m.uptime_seconds = uptime;

  m.timestamp_ms = now_ms;

  prev_total_ = total;
  prev_cores_ = cores;
  prev_net_ = net;
  prev_disk_ = disk;
  prev_ms_ = static_cast<double>(now_ms);
  first_sample_ = false;
  return m;
}

std::optional<double> Sampler::processStartTime(double uptime_seconds,
                                                const ProcessInfo& p) const {
  const long hz = procfs_->clockTicksPerSecond();
  if (hz <= 0) return std::nullopt;
  const double since_boot = static_cast<double>(p.starttime_ticks) / hz;
  if (since_boot > uptime_seconds) return std::nullopt;
  return uptime_seconds - since_boot;
}

}  // namespace sysograph
