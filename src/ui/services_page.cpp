// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/services_page.h"

#include <gio/gio.h>
#include <glib.h>

#include "core/i18n.h"

namespace sysograph {
namespace ui {

namespace {

enum {
  SVC_COL_UNIT,
  SVC_COL_LOAD,
  SVC_COL_ACTIVE,
  SVC_COL_SUB,
  SVC_COL_DESCRIPTION,
  SVC_N_COLUMNS
};

}  // namespace

ServicesPage::ServicesPage(GtkWidget* parentWindow)
    : parent_window_(parentWindow) {
  build();
}

ServicesPage::~ServicesPage() = default;

GtkWidget* ServicesPage::widget() { return root_; }

void ServicesPage::build() {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(root_, 8);
  gtk_widget_set_margin_end(root_, 8);
  gtk_widget_set_margin_top(root_, 8);
  gtk_widget_set_margin_bottom(root_, 8);

  GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(root_), bar);

  GtkWidget* refresh_button = gtk_button_new_with_label(_("Refresh"));
  g_signal_connect(refresh_button, "clicked", G_CALLBACK(&refreshStatic),
                   this);
  gtk_box_append(GTK_BOX(bar), refresh_button);

  status_label_ = gtk_label_new(nullptr);
  gtk_label_set_xalign(GTK_LABEL(status_label_), 0.0f);
  gtk_widget_add_css_class(status_label_, "dim-label");
  gtk_widget_set_hexpand(status_label_, TRUE);
  gtk_box_append(GTK_BOX(bar), status_label_);

  store_ = gtk_list_store_new(SVC_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  GtkWidget* scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(root_), scrolled);

  GtkTreeView* view = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_model(view, GTK_TREE_MODEL(store_));

  auto append_text = [&](const char* title, int col) {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        title, renderer, "text", col, nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, col);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view, column);
  };

  append_text(_("Unit"), SVC_COL_UNIT);
  append_text(_("Load"), SVC_COL_LOAD);
  append_text(_("Active"), SVC_COL_ACTIVE);
  append_text(_("Sub"), SVC_COL_SUB);
  append_text(_("Description"), SVC_COL_DESCRIPTION);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(view));

  gtk_label_set_text(GTK_LABEL(status_label_), _("Loading services…"));
  refresh();
}

void ServicesPage::refreshStatic(GtkButton*, gpointer user_data) {
  static_cast<ServicesPage*>(user_data)->refresh();
}

void ServicesPage::refresh() {
  gtk_label_set_text(GTK_LABEL(status_label_), _("Loading services…"));

  GError* error = nullptr;
  // Arguments are passed directly; no shell is involved at any point.
  GSubprocess* sub = g_subprocess_new(
      G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error, "systemctl", "list-units",
      "--type=service", "--all", "--plain", "--no-pager", "--no-legend",
      static_cast<const char*>(nullptr));
  if (!sub) {
    g_clear_error(&error);
    gtk_label_set_text(GTK_LABEL(status_label_),
                       _("Failed to run systemctl."));
    return;
  }
  g_subprocess_communicate_utf8_async(sub, nullptr, nullptr, &listUnitsDone,
                                      this);
}

void ServicesPage::listUnitsDone(GObject* source, GAsyncResult* result,
                                 gpointer user_data) {
  ServicesPage* self = static_cast<ServicesPage*>(user_data);
  GSubprocess* sub = G_SUBPROCESS(source);

  char* out = nullptr;
  GError* error = nullptr;
  const gboolean ok = g_subprocess_communicate_utf8_finish(sub, result, &out,
                                                           nullptr, &error);
  if (!ok || !g_subprocess_get_successful(sub)) {
    g_clear_error(&error);
    g_free(out);
    g_object_unref(sub);
    gtk_label_set_text(GTK_LABEL(self->status_label_),
                       _("systemd is not available on this system."));
    return;
  }
  const std::vector<ServiceRow> rows = parse_list_units(out ? out : "");
  g_free(out);
  g_object_unref(sub);
  self->setRows(rows);
}

void ServicesPage::setRows(const std::vector<ServiceRow>& rows) {
  gtk_list_store_clear(store_);
  for (const auto& r : rows) {
    GtkTreeIter iter;
    gtk_list_store_append(store_, &iter);
    gtk_list_store_set(store_, &iter, SVC_COL_UNIT, r.unit.c_str(),
                       SVC_COL_LOAD, r.load.c_str(), SVC_COL_ACTIVE,
                       r.active.c_str(), SVC_COL_SUB, r.sub.c_str(),
                       SVC_COL_DESCRIPTION, r.description.c_str(), -1);
  }
  gtk_label_set_text(GTK_LABEL(status_label_),
                     _("The services list is provided by systemd. Changes to services are not offered here; use your distribution's service manager."));
}

}  // namespace ui
}  // namespace sysograph
