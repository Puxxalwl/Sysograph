// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Application window: tab stack, header bar, update timer and About dialog.
#pragma once

#include <gtk/gtk.h>

#include <memory>
#include <string>

#include "core/sampler.h"

namespace sysograph {
namespace ui {

class ProcessesPage;
class TreePage;
class PerformancePage;
class UsersPage;
class StartupPage;
class ServicesPage;

class MainWindow {
 public:
  explicit MainWindow(GtkApplication* app);
  ~MainWindow();

  GtkWidget* widget();

  void onTick();

 private:
  void build();
  void buildHeader();
  void addPage(GtkWidget* page, const char* id, const char* title,
               const char* icon_name);
  void setIntervalMs(int ms);
  void startTimer();
  void showAbout();
  std::string startedTextFor(const ProcessInfo& p);

  static gboolean tickStatic(gpointer user_data);
  static void intervalChangedStatic(GSimpleAction* action,
                                    GVariant* value, gpointer user_data);
  static void aboutStatic(GSimpleAction* action, GVariant* parameter,
                          gpointer user_data);
  static void aboutResponseStatic(GtkDialog* dialog, int response,
                                  gpointer user_data);

  GtkApplicationWindow* window_ = nullptr;
  GtkStack* stack_ = nullptr;
  GtkStackSwitcher* switcher_ = nullptr;

  std::unique_ptr<Sampler> sampler_;
  int interval_ms_ = 1000;
  guint timeout_id_ = 0;

  std::unique_ptr<ProcessesPage> processes_;
  std::unique_ptr<TreePage> tree_;
  std::unique_ptr<PerformancePage> performance_;
  std::unique_ptr<UsersPage> users_;
  std::unique_ptr<StartupPage> startup_;
  std::unique_ptr<ServicesPage> services_;
};

}  // namespace ui
}  // namespace sysograph
