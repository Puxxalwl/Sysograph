// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/users_page.h"

#include <glib.h>

#include <string>

#include "core/formatter.h"
#include "core/i18n.h"
#include "core/users.h"

namespace sysograph {
namespace ui {

namespace {

enum {
  USER_COL_USER,
  USER_COL_UID,
  USER_COL_PROCESSES,
  USER_COL_CPU,
  USER_COL_MEM,
  USER_N_COLUMNS
};

void cellUserCpu(GtkTreeViewColumn*, GtkCellRenderer* cell, GtkTreeModel* model,
                 GtkTreeIter* iter, gpointer) {
  gdouble cpu = 0.0;
  gtk_tree_model_get(model, iter, USER_COL_CPU, &cpu, -1);
  char buf[32];
  g_snprintf(buf, sizeof(buf), "%.1f %%", static_cast<double>(cpu));
  g_object_set(cell, "text", buf, nullptr);
}

void cellUserMem(GtkTreeViewColumn*, GtkCellRenderer* cell, GtkTreeModel* model,
                 GtkTreeIter* iter, gpointer) {
  guint64 mem = 0;
  gtk_tree_model_get(model, iter, USER_COL_MEM, &mem, -1);
  g_object_set(cell, "text", formatBytes(mem).c_str(), nullptr);
}

}  // namespace

UsersPage::UsersPage() { build(); }

UsersPage::~UsersPage() = default;

GtkWidget* UsersPage::widget() { return root_; }

void UsersPage::build() {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(root_, 8);
  gtk_widget_set_margin_end(root_, 8);
  gtk_widget_set_margin_top(root_, 8);
  gtk_widget_set_margin_bottom(root_, 8);

  store_ = gtk_list_store_new(USER_N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT,
                              G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_UINT64);
  sort_ = GTK_TREE_MODEL_SORT(
      gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store_)));

  GtkWidget* scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(root_), scrolled);

  GtkTreeView* view = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_model(view, GTK_TREE_MODEL(sort_));

  auto append_text = [&](const char* title, int col, bool right) {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    if (right) g_object_set(renderer, "xalign", 1.0, nullptr);
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        title, renderer, "text", col, nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, col);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view, column);
  };

  append_text(_("User"), USER_COL_USER, false);
  append_text("UID", USER_COL_UID, true);
  append_text(_("Processes"), USER_COL_PROCESSES, true);
  {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, nullptr);
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        _("CPU"), renderer, nullptr);
    gtk_tree_view_column_set_cell_data_func(column, renderer, &cellUserCpu,
                                            nullptr, nullptr);
    gtk_tree_view_column_set_sort_column_id(column, USER_COL_CPU);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view, column);
  }
  {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, nullptr);
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        _("Memory"), renderer, nullptr);
    gtk_tree_view_column_set_cell_data_func(column, renderer, &cellUserMem,
                                            nullptr, nullptr);
    gtk_tree_view_column_set_sort_column_id(column, USER_COL_MEM);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view, column);
  }

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(view));
}

void UsersPage::update(const SampledMetrics& metrics) {
  const std::vector<UserAggregate> rows = aggregateByUser(metrics.processes);
  gtk_list_store_clear(store_);
  for (const auto& r : rows) {
    GtkTreeIter iter;
    gtk_list_store_append(store_, &iter);
    gtk_list_store_set(store_, &iter, USER_COL_USER, r.user.c_str(),
                       USER_COL_UID, static_cast<guint>(r.uid),
                       USER_COL_PROCESSES, static_cast<guint>(r.process_count),
                       USER_COL_CPU, r.cpu_percent,
                       USER_COL_MEM, static_cast<guint64>(r.memory_bytes),
                       -1);
  }
}

}  // namespace ui
}  // namespace sysograph
