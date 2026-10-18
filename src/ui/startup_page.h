// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The "Startup" tab: XDG autostart entries with enable/disable via
// user-level .desktop overrides (no root required).
#pragma once

#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "core/types.h"
#include "core/xdg_autostart.h"

namespace sysograph {
namespace ui {

class StartupPage {
 public:
  StartupPage(GtkWidget* parentWindow);
  ~StartupPage();

  GtkWidget* widget();
  void refresh();

 private:
  void build();
  void applySelectionAction(bool enable);
  AutostartEntry* selectedEntry();

  static void enableStatic(GtkButton*, gpointer);
  static void disableStatic(GtkButton*, gpointer);

  GtkWidget* parent_window_ = nullptr;
  GtkWidget* root_ = nullptr;
  GtkListStore* store_ = nullptr;
  GtkTreeView* view_ = nullptr;
  GtkWidget* status_label_ = nullptr;
  std::vector<AutostartEntry> entries_;
};

}  // namespace ui
}  // namespace sysograph
