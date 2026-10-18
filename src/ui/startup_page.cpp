// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/startup_page.h"

#include <glib.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "core/i18n.h"

namespace sysograph {
namespace ui {

namespace {

enum {
  START_COL_NAME,
  START_COL_EXEC,
  START_COL_ORIGIN,
  START_COL_STATUS,
  START_COL_COMMENT,
  START_COL_INDEX,  // hidden: position in entries_
  START_N_COLUMNS
};

std::vector<std::filesystem::path> systemAutostartDirs() {
  std::vector<std::filesystem::path> dirs;
  const char* config_dirs = std::getenv("XDG_CONFIG_DIRS");
  if (!config_dirs || !*config_dirs) config_dirs = "/etc/xdg";
  std::string list(config_dirs);
  size_t start = 0;
  while (start <= list.size()) {
    size_t colon = list.find(':', start);
    const std::string dir =
        list.substr(start, colon == std::string::npos ? std::string::npos
                                                      : colon - start);
    if (!dir.empty()) dirs.emplace_back(dir + "/autostart");
    if (colon == std::string::npos) break;
    start = colon + 1;
  }
  return dirs;
}

std::filesystem::path userAutostartDir() {
  const char* config_home = std::getenv("XDG_CONFIG_HOME");
  std::string base;
  if (config_home && *config_home) {
    base = config_home;
  } else {
    const char* home = std::getenv("HOME");
    base = home ? std::string(home) + "/.config" : std::string();
  }
  return std::filesystem::path(base) / "autostart";
}

}  // namespace

StartupPage::StartupPage(GtkWidget* parentWindow)
    : parent_window_(parentWindow) {
  build();
  refresh();
}

StartupPage::~StartupPage() = default;

GtkWidget* StartupPage::widget() { return root_; }

void StartupPage::build() {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(root_, 8);
  gtk_widget_set_margin_end(root_, 8);
  gtk_widget_set_margin_top(root_, 8);
  gtk_widget_set_margin_bottom(root_, 8);

  GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(root_), bar);

  GtkWidget* enable = gtk_button_new_with_label(_("Enable"));
  g_signal_connect(enable, "clicked", G_CALLBACK(&enableStatic), this);
  gtk_box_append(GTK_BOX(bar), enable);

  GtkWidget* disable = gtk_button_new_with_label(_("Disable"));
  g_signal_connect(disable, "clicked", G_CALLBACK(&disableStatic), this);
  gtk_box_append(GTK_BOX(bar), disable);

  status_label_ = gtk_label_new(nullptr);
  gtk_label_set_xalign(GTK_LABEL(status_label_), 0.0f);
  gtk_widget_add_css_class(status_label_, "dim-label");
  gtk_widget_set_hexpand(status_label_, TRUE);
  gtk_box_append(GTK_BOX(bar), status_label_);

  store_ = gtk_list_store_new(START_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_INT);

  GtkWidget* scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(root_), scrolled);

  view_ = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_model(view_, GTK_TREE_MODEL(store_));

  auto append_text = [&](const char* title, int col) {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        title, renderer, "text", col, nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, col);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view_, column);
  };

  append_text(_("Entry"), START_COL_NAME);
  append_text(_("Command"), START_COL_EXEC);
  append_text(_("Origin"), START_COL_ORIGIN);
  append_text(_("Status"), START_COL_STATUS);
  append_text(_("Description"), START_COL_COMMENT);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(view_));
}

void StartupPage::refresh() {
  const std::filesystem::path user_dir = userAutostartDir();
  entries_ = scanAutostart(user_dir, systemAutostartDirs());

  gtk_list_store_clear(store_);
  for (size_t i = 0; i < entries_.size(); ++i) {
    const auto& e = entries_[i];
    GtkTreeIter iter;
    gtk_list_store_append(store_, &iter);
    gtk_list_store_set(store_, &iter, START_COL_NAME, e.name.c_str(),
                       START_COL_EXEC, e.exec.c_str(),
                       START_COL_ORIGIN, _(e.origin.c_str()),
                       START_COL_STATUS,
                       e.enabled ? _("enabled") : _("disabled"),
                       START_COL_COMMENT, e.comment.c_str(), START_COL_INDEX,
                       static_cast<gint>(i), -1);
  }

  gtk_label_set_text(
      GTK_LABEL(status_label_),
      user_dir.empty()
          ? _("The list is read-only because no writable autostart directory exists.")
          : "");
}

void StartupPage::enableStatic(GtkButton*, gpointer user_data) {
  static_cast<StartupPage*>(user_data)->applySelectionAction(true);
}

void StartupPage::disableStatic(GtkButton*, gpointer user_data) {
  static_cast<StartupPage*>(user_data)->applySelectionAction(false);
}

AutostartEntry* StartupPage::selectedEntry() {
  GtkTreeSelection* selection = gtk_tree_view_get_selection(view_);
  GtkTreeModel* model = nullptr;
  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
    return nullptr;
  }
  gint index = -1;
  gtk_tree_model_get(model, &iter, START_COL_INDEX, &index, -1);
  if (index < 0 || index >= static_cast<int>(entries_.size())) return nullptr;
  return &entries_[static_cast<size_t>(index)];
}

void StartupPage::applySelectionAction(bool enable) {
  AutostartEntry* entry = selectedEntry();
  if (!entry) return;

  std::string error;
  if (setAutostartEnabled(*entry, enable, &error)) {
    refresh();
  } else {
    GtkAlertDialog* dialog = gtk_alert_dialog_new("%s", error.c_str());
    const char* buttons[] = {_("OK"), nullptr};
    gtk_alert_dialog_set_buttons(dialog, buttons);
    gtk_alert_dialog_show(dialog, GTK_WINDOW(parent_window_));
    g_object_unref(dialog);
  }
}

}  // namespace ui
}  // namespace sysograph
