// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Computes rates and percentages from consecutive raw procfs reads.
// All math is pure and unit-testable.
#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "core/procfs.h"
#include "core/types.h"

namespace sysograph {

// A point for the UI graphs: a value and a flag whether it is valid
// (e.g. the first tick has no cpu percentage yet).
struct HistoryPoint {
  std::optional<double> value;
};

// Fixed-capacity ring buffer used by the performance graphs. Never grows,
// so long-running sessions do not leak memory.
template <typename T, size_t N>
class HistoryBuffer {
 public:
  void push(const T& v) {
    data_[head_] = v;
    head_ = (head_ + 1) % N;
    if (size_ < N) ++size_;
  }
  size_t size() const { return size_; }
  constexpr size_t capacity() const { return N; }

  // Element i counts back from the newest (0 = newest).
  const T& newest(size_t i) const {
    size_t idx = (head_ + N - 1 - i) % N;
    return data_[idx];
  }

  void clear() {
    head_ = 0;
    size_ = 0;
  }

 private:
  T data_[N]{};
  size_t head_ = 0;
  size_t size_ = 0;
};

// CPU percentage of a process given counter deltas, expressed in
// percent of total machine capacity (all cores).
double process_cpu_percent(uint64_t proc_delta, uint64_t total_delta);

// CPU busy percentage of one core given its CpuTimes delta.
double core_busy_percent(const CpuTimes& delta);

// Rate (per second) given counter deltas and elapsed monotonic ms.
double per_second_rate(uint64_t counter_delta, double elapsed_ms);

// Combines raw procfs reads into SampledMetrics with computed rates.
class Sampler {
 public:
  explicit Sampler(std::unique_ptr<Procfs> procfs);

  // Takes one sample. Never throws; missing procfs entries simply yield
  // zero rates for that tick.
  SampledMetrics sample();

  // Wall-clock start time of a process, seconds since epoch; nullopt if
  // unknown. Exposed for tests.
  std::optional<double> processStartTime(double uptime_seconds,
                                         const ProcessInfo& p) const;

 private:
  std::unique_ptr<Procfs> procfs_;

  bool first_sample_ = true;
  CpuTimes prev_total_;
  std::vector<CpuTimes> prev_cores_;
  std::unordered_map<int, uint64_t> prev_proc_ticks_;
  NetTotals prev_net_;
  DiskTotals prev_disk_;
  double prev_ms_ = 0.0;
};

}  // namespace sysograph
