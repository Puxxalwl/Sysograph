// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The "Services" tab: read-only listing of systemd service units obtained
// by invoking `systemctl list-units` as a separate process (no shell, no
// parsing of shell strings — arguments are passed directly).
#pragma once

#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "core/procfs.h"
#include "core/types.h"

namespace sysograph {
namespace ui {

class ServicesPage {
 public:
  ServicesPage(GtkWidget* parentWindow);
  ~ServicesPage();

  GtkWidget* widget();
  void refresh();

 private:
  void build();
  void setRows(const std::vector<ServiceRow>& rows);

  static void refreshStatic(GtkButton*, gpointer);
  static void listUnitsDone(GObject* source, GAsyncResult* result,
                            gpointer user_data);

  GtkWidget* parent_window_ = nullptr;
  GtkWidget* root_ = nullptr;
  GtkListStore* store_ = nullptr;
  GtkWidget* status_label_ = nullptr;
};

}  // namespace ui
}  // namespace sysograph
