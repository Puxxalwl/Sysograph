// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/tree_page.h"

#include <glib.h>

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/formatter.h"
#include "core/i18n.h"
#include "core/process_tree.h"

namespace sysograph {
namespace ui {

namespace {

enum {
  TREE_COL_NAME,
  TREE_COL_PID,
  TREE_COL_USER,
  TREE_COL_CPU,
  TREE_COL_MEM,
  TREE_COL_STATE,
  TREE_N_COLUMNS
};

}  // namespace

TreePage::TreePage() { build(); }

TreePage::~TreePage() = default;

GtkWidget* TreePage::widget() { return root_; }

void TreePage::build() {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(root_, 8);
  gtk_widget_set_margin_end(root_, 8);
  gtk_widget_set_margin_top(root_, 8);
  gtk_widget_set_margin_bottom(root_, 8);

  store_ = gtk_tree_store_new(
      TREE_N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_DOUBLE,
      G_TYPE_UINT64, G_TYPE_STRING);

  GtkWidget* scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(root_), scrolled);

  view_ = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_model(view_, GTK_TREE_MODEL(store_));

  auto append_text = [&](const char* title, int col, bool right_align) {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    if (right_align) {
      g_object_set(renderer, "xalign", 1.0, nullptr);
    }
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        title, renderer, "text", col, nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(view_, column);
  };

  append_text(_("Name"), TREE_COL_NAME, false);
  append_text(_("PID"), TREE_COL_PID, true);
  append_text(_("User"), TREE_COL_USER, false);
  append_text(_("CPU"), TREE_COL_CPU, true);
  append_text(_("Memory"), TREE_COL_MEM, true);
  append_text(_("State"), TREE_COL_STATE, false);

  g_signal_connect(view_, "row-expanded", G_CALLBACK(&rowExpandedStatic),
                   this);
  g_signal_connect(view_, "row-collapsed", G_CALLBACK(&rowCollapsedStatic),
                   this);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(view_));
}

void TreePage::rowExpandedStatic(GtkTreeView*, GtkTreeIter*, GtkTreePath* path,
                                 gpointer user_data) {
  TreePage* self = static_cast<TreePage*>(user_data);
  char* path_str = gtk_tree_path_to_string(path);
  if (path_str) {
    self->expanded_paths_.insert(path_str);
    g_free(path_str);
  }
}

void TreePage::rowCollapsedStatic(GtkTreeView*, GtkTreeIter*,
                                  GtkTreePath* path, gpointer user_data) {
  TreePage* self = static_cast<TreePage*>(user_data);
  char* path_str = gtk_tree_path_to_string(path);
  if (path_str) {
    self->expanded_paths_.erase(path_str);
    g_free(path_str);
  }
}

void TreePage::update(const SampledMetrics& metrics) {
  std::map<int, ProcessTreeNode> nodes;
  std::vector<int> roots;
  buildProcessTree(metrics.processes, &nodes, &roots);

  gtk_tree_store_clear(store_);

  // Recursive insertion driven by the prebuilt tree map. Depth is guarded
  // so that even corrupted ppid chains cannot overflow the stack.
  std::function<void(int, GtkTreeIter*, int)> insertNode =
      [&](int pid, GtkTreeIter* parent, int depth) {
    if (depth > 512) return;
    auto node_it = nodes.find(pid);
    if (node_it == nodes.end()) return;
    const ProcessInfo* p = node_it->second.info;
    if (!p) return;

    GtkTreeIter iter;
    gtk_tree_store_append(store_, &iter, parent);
    const std::string display = p->name + " (" + std::to_string(pid) + ")";
    gtk_tree_store_set(store_, &iter, TREE_COL_NAME, display.c_str(),
                       TREE_COL_PID, pid, TREE_COL_USER, p->user.c_str(),
                       TREE_COL_CPU, p->cpu_percent,
                       TREE_COL_MEM, static_cast<guint64>(p->rss_bytes),
                       TREE_COL_STATE, _(process_state_key(p->state)), -1);
    for (int child : node_it->second.child_pids) {
      insertNode(child, &iter, depth + 1);
    }
  };

  for (int root : roots) {
    insertNode(root, nullptr, 0);
  }
  // Restore expansion state.
  for (const auto& path_str : expanded_paths_) {
    GtkTreePath* path = gtk_tree_path_new_from_string(path_str.c_str());
    if (path) {
      gtk_tree_view_expand_row(view_, path, FALSE);
      gtk_tree_path_free(path);
    }
  }
}

}  // namespace ui
}  // namespace sysograph
