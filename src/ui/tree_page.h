// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The "Process tree" tab: hierarchical view with expandable nodes.
// Expansion state survives periodic rebuilds.
#pragma once

#include <gtk/gtk.h>

#include <set>
#include <string>

#include "core/types.h"

namespace sysograph {
namespace ui {

class TreePage {
 public:
  TreePage();
  ~TreePage();

  GtkWidget* widget();
  void update(const SampledMetrics& metrics);

 private:
  void build();

  static void rowExpandedStatic(GtkTreeView* view, GtkTreeIter* iter,
                                GtkTreePath* path, gpointer user_data);
  static void rowCollapsedStatic(GtkTreeView* view, GtkTreeIter* iter,
                                 GtkTreePath* path, gpointer user_data);

  GtkWidget* root_ = nullptr;
  GtkTreeStore* store_ = nullptr;
  GtkTreeView* view_ = nullptr;
  std::set<std::string> expanded_paths_;
};

}  // namespace ui
}  // namespace sysograph
