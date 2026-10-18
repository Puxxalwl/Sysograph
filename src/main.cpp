// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Application entry point.
#include <gtk/gtk.h>

#include <cstdio>

#include "core/i18n.h"
#include "ui/main_window.h"

namespace {

void onActivate(GtkApplication* app, gpointer user_data) {
  (void)user_data;
  auto* window = new sysograph::ui::MainWindow(app);
  gtk_window_present(GTK_WINDOW(window->widget()));
  // The window owns itself via the application; release our C++ wrapper
  // once the widget is destroyed.
  g_signal_connect_swapped(window->widget(), "destroy",
                           G_CALLBACK(+[](gpointer data) {
                             delete static_cast<sysograph::ui::MainWindow*>(
                                 data);
                           }),
                           window);
}

}  // namespace

int main(int argc, char* argv[]) {
  sysograph::i18n::init();

  GtkApplication* app = gtk_application_new(
      "org.sysograph.Sysograph", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(&onActivate), nullptr);

  const int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
