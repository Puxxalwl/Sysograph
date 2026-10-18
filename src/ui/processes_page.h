// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The "Processes" tab: sortable, searchable table with a details panel
// and process termination actions.
#pragma once

#include <gtk/gtk.h>

#include <functional>
#include <string>

#include "core/types.h"

namespace sysograph {
namespace ui {

class ProcessesPage {
 public:
  // started_formatter turns a ProcessInfo into a human "started" string
  // (owned by MainWindow, which knows the sampler and uptime).
  ProcessesPage(GtkWidget* parentWindow);
  ~ProcessesPage();

  GtkWidget* widget();
  void update(const SampledMetrics& metrics);

  void setStartedFormatter(
      std::function<std::string(const ProcessInfo&)> fn) {
    started_formatter_ = std::move(fn);
  }

 private:
  void buildModel();
  void buildToolbar();
  void buildTree();
  void buildDetails();
  void setRow(GtkListStore* store, GtkTreeIter* iter,
              const ProcessInfo& p);
  void refreshSelection();
  int selectedPid();
  void updateDetails(int pid);
  void requestKill(int pid, bool force);
  void showResult(const char* message);

  static gboolean visibleFuncStatic(GtkTreeModel* model, GtkTreeIter* iter,
                                    gpointer user_data);
  static void cellCpuStatic(GtkTreeViewColumn* col, GtkCellRenderer* cell,
                            GtkTreeModel* model, GtkTreeIter* iter,
                            gpointer user_data);
  static void cellMemStatic(GtkTreeViewColumn* col, GtkCellRenderer* cell,
                            GtkTreeModel* model, GtkTreeIter* iter,
                            gpointer user_data);
  static void selectionChangedStatic(GtkTreeSelection* sel,
                                     gpointer user_data);
  static void searchChangedStatic(GtkSearchEntry* entry, gpointer user_data);
  static void endTaskStatic(GtkButton* button, gpointer user_data);
  static void forceKillStatic(GtkButton* button, gpointer user_data);
  static void chooseKillResponseStatic(GObject* source, GAsyncResult* res,
                                       gpointer user_data);

  GtkWidget* parent_window_ = nullptr;
  GtkWidget* root_ = nullptr;
  GtkSearchEntry* search_ = nullptr;
  GtkListStore* store_ = nullptr;
  GtkTreeModelFilter* filter_ = nullptr;
  GtkTreeModelSort* sort_ = nullptr;
  GtkTreeView* view_ = nullptr;
  GtkWidget* end_button_ = nullptr;
  GtkWidget* kill_button_ = nullptr;

  // Details labels.
  GtkLabel* d_name_ = nullptr;
  GtkLabel* d_pid_ = nullptr;
  GtkLabel* d_state_ = nullptr;
  GtkLabel* d_user_ = nullptr;
  GtkLabel* d_cpu_ = nullptr;
  GtkLabel* d_mem_ = nullptr;
  GtkLabel* d_virt_ = nullptr;
  GtkLabel* d_shared_ = nullptr;
  GtkLabel* d_threads_ = nullptr;
  GtkLabel* d_nice_ = nullptr;
  GtkLabel* d_cpu_time_ = nullptr;
  GtkLabel* d_started_ = nullptr;
  GtkLabel* d_exe_ = nullptr;
  GtkLabel* d_cwd_ = nullptr;
  GtkLabel* d_fds_ = nullptr;
  GtkLabel* d_cmd_ = nullptr;

  std::string search_lower_;
  SampledMetrics metrics_;
  std::function<std::string(const ProcessInfo&)> started_formatter_;
};

}  // namespace ui
}  // namespace sysograph
