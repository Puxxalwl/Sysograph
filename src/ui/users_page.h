// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The "Users" tab: per-account aggregation of processes, CPU and memory.
#pragma once

#include <gtk/gtk.h>

#include "core/types.h"

namespace sysograph {
namespace ui {

class UsersPage {
 public:
  UsersPage();
  ~UsersPage();

  GtkWidget* widget();
  void update(const SampledMetrics& metrics);

 private:
  void build();

  GtkWidget* root_ = nullptr;
  GtkListStore* store_ = nullptr;
  GtkTreeModelSort* sort_ = nullptr;
};

}  // namespace ui
}  // namespace sysograph
