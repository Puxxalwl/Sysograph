// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The "Performance" tab: CPU / Memory / Disk / Network cards with graphs,
// per-core level bars and a filesystem capacity readout.
#pragma once

#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "core/types.h"
#include "ui/graphs.h"

namespace sysograph {
namespace ui {

class PerformancePage {
 public:
  PerformancePage();
  ~PerformancePage();

  GtkWidget* widget();
  void update(const SampledMetrics& metrics);

 private:
  void build();
  void ensureCoreBars(unsigned count);

  GtkWidget* root_ = nullptr;

  // CPU card.
  LineGraph* cpu_graph_ = nullptr;
  GtkLabel* cpu_value_ = nullptr;
  GtkLabel* cpu_info_ = nullptr;
  GtkGrid* cores_grid_ = nullptr;
  std::vector<GtkWidget*> core_cells_;
  std::vector<GtkLevelBar*> core_bars_;
  unsigned core_bars_count_ = 0;

  // Memory card.
  LineGraph* mem_graph_ = nullptr;
  GtkLabel* mem_value_ = nullptr;
  GtkLabel* mem_info_ = nullptr;

  // Disk card.
  LineGraph* disk_graph_ = nullptr;
  GtkLabel* disk_value_ = nullptr;
  GtkLabel* disk_info_ = nullptr;

  // Network card.
  LineGraph* net_graph_ = nullptr;
  GtkLabel* net_value_ = nullptr;
  GtkLabel* net_info_ = nullptr;
};

}  // namespace ui
}  // namespace sysograph
