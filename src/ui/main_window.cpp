// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main_window.h"

#include <glib.h>

#include <ctime>
#include <fstream>
#include <memory>
#include <string>

#include "core/i18n.h"
#include "ui/performance_page.h"
#include "ui/processes_page.h"
#include "ui/services_page.h"
#include "ui/startup_page.h"
#include "ui/tree_page.h"
#include "ui/users_page.h"

#ifndef SYSSOGRAPH_VERSION
#define SYSSOGRAPH_VERSION "0.1.0"
#endif

#ifndef SYSSOGRAPH_DATA_DIR
#define SYSSOGRAPH_DATA_DIR "/usr/local/share/sysograph"
#endif

#ifndef SYSSOGRAPH_SOURCE_DIR
#define SYSSOGRAPH_SOURCE_DIR "."
#endif

namespace sysograph {
namespace ui {

namespace {

void loadStyleSheet() {
  // Prefer the installed data dir; fall back to the source tree so that
  // running straight from the build directory works too.
  std::string path = std::string(SYSSOGRAPH_DATA_DIR) + "/style.css";
  if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) {
    path = std::string(SYSSOGRAPH_SOURCE_DIR) + "/data/style.css";
  }
  if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) return;

  GtkCssProvider* provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(provider, path.c_str());
  GdkDisplay* display = gdk_display_get_default();
  if (display) {
    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  g_object_unref(provider);
}

}  // namespace

MainWindow::MainWindow(GtkApplication* app) {
  window_ = GTK_APPLICATION_WINDOW(gtk_application_window_new(app));
  gtk_window_set_title(GTK_WINDOW(window_), "Sysograph");
  gtk_window_set_default_size(GTK_WINDOW(window_), 980, 680);
  gtk_window_set_icon_name(GTK_WINDOW(window_), "org.sysograph.Sysograph");

  loadStyleSheet();

  buildHeader();
  build();

  sampler_ = std::make_unique<Sampler>(std::make_unique<Procfs>());
  processes_->setStartedFormatter([this](const ProcessInfo& p) {
    return startedTextFor(p);
  });

  onTick();  // first sample immediately
  startTimer();
}

MainWindow::~MainWindow() {
  if (timeout_id_ != 0) {
    g_source_remove(timeout_id_);
    timeout_id_ = 0;
  }
}

GtkWidget* MainWindow::widget() { return GTK_WIDGET(window_); }

void MainWindow::buildHeader() {
  GtkHeaderBar* header = GTK_HEADER_BAR(gtk_header_bar_new());
  gtk_window_set_titlebar(GTK_WINDOW(window_), GTK_WIDGET(header));

  switcher_ = GTK_STACK_SWITCHER(gtk_stack_switcher_new());

  // Update-interval menu.
  GSimpleAction* interval_action = g_simple_action_new_stateful(
      "interval", G_VARIANT_TYPE_STRING, g_variant_new_string("1"));
  g_signal_connect(interval_action, "change-state",
                   G_CALLBACK(&intervalChangedStatic), this);
  g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(interval_action));
  g_object_unref(interval_action);

  GSimpleAction* about_action = g_simple_action_new("about", nullptr);
  g_signal_connect(about_action, "activate", G_CALLBACK(&aboutStatic), this);
  g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(about_action));
  g_object_unref(about_action);

  GMenu* menu = g_menu_new();
  GMenu* interval_section = g_menu_new();
  struct {
    const char* label;
    const char* target;
  } intervals[] = {
      {_("1 second"), "1"},
      {_("2 seconds"), "2"},
      {_("5 seconds"), "5"},
  };
  for (const auto& iv : intervals) {
    GMenuItem* item = g_menu_item_new(iv.label, nullptr);
    g_menu_item_set_detailed_action(
        item, ("win.interval('" + std::string(iv.target) + "')").c_str());
    g_menu_append_item(interval_section, item);
    g_object_unref(item);
  }
  g_menu_append_section(menu, _("Update interval"),
                        G_MENU_MODEL(interval_section));
  g_object_unref(interval_section);
  g_menu_append(menu, _("About Sysograph"), "win.about");

  GtkWidget* menu_button = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button),
                                "preferences-system-symbolic");
  gtk_menu_button_set_popover(
      GTK_MENU_BUTTON(menu_button),
      gtk_popover_menu_new_from_model(G_MENU_MODEL(menu)));
  g_object_unref(menu);

  gtk_header_bar_pack_start(header, GTK_WIDGET(menu_button));
  gtk_header_bar_set_title_widget(header, GTK_WIDGET(switcher_));
}

void MainWindow::build() {
  stack_ = GTK_STACK(gtk_stack_new());
  gtk_stack_set_transition_type(stack_, GTK_STACK_TRANSITION_TYPE_NONE);
  gtk_window_set_child(GTK_WINDOW(window_), GTK_WIDGET(stack_));
  gtk_stack_switcher_set_stack(switcher_, stack_);

  processes_ = std::make_unique<ProcessesPage>(GTK_WIDGET(window_));
  addPage(processes_->widget(), "processes", _("Processes"),
          "utilities-system-monitor-symbolic");

  tree_ = std::make_unique<TreePage>();
  addPage(tree_->widget(), "tree", _("Process tree"),
          "view-list-symbolic");

  performance_ = std::make_unique<PerformancePage>();
  addPage(performance_->widget(), "performance", _("Performance"),
          "audio-volume-high-symbolic");

  users_ = std::make_unique<UsersPage>();
  addPage(users_->widget(), "users", _("Users"), "system-users-symbolic");

  startup_ = std::make_unique<StartupPage>(GTK_WIDGET(window_));
  addPage(startup_->widget(), "startup", _("Startup"),
          "preferences-system-symbolic");

  services_ = std::make_unique<ServicesPage>(GTK_WIDGET(window_));
  addPage(services_->widget(), "services", _("Services"),
          "system-run-symbolic");
}

void MainWindow::addPage(GtkWidget* page, const char* id, const char* title,
                         const char* icon_name) {
  gtk_stack_add_titled(stack_, page, id, title);
  (void)icon_name;
}

void MainWindow::startTimer() {
  if (timeout_id_ != 0) g_source_remove(timeout_id_);
  timeout_id_ = g_timeout_add(static_cast<guint>(interval_ms_),
                              &tickStatic, this);
}

gboolean MainWindow::tickStatic(gpointer user_data) {
  static_cast<MainWindow*>(user_data)->onTick();
  return G_SOURCE_CONTINUE;
}

void MainWindow::onTick() {
  const SampledMetrics m = sampler_->sample();
  processes_->update(m);
  tree_->update(m);
  performance_->update(m);
  users_->update(m);
}

void MainWindow::setIntervalMs(int ms) {
  if (ms == interval_ms_) return;
  interval_ms_ = ms;
  startTimer();
}

void MainWindow::intervalChangedStatic(GSimpleAction* action,
                                       GVariant* value, gpointer user_data) {
  MainWindow* self = static_cast<MainWindow*>(user_data);
  const char* seconds = g_variant_get_string(value, nullptr);
  int ms = 1000;
  if (seconds) {
    if (g_strcmp0(seconds, "2") == 0) ms = 2000;
    else if (g_strcmp0(seconds, "5") == 0) ms = 5000;
    else ms = 1000;
  }
  self->setIntervalMs(ms);
  g_simple_action_set_state(action, value);
}

void MainWindow::aboutStatic(GSimpleAction*, GVariant*, gpointer user_data) {
  static_cast<MainWindow*>(user_data)->showAbout();
}

void MainWindow::showAbout() {
  GtkAboutDialog* dialog = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
  gtk_about_dialog_set_program_name(dialog, "Sysograph");
  gtk_about_dialog_set_version(dialog, SYSSOGRAPH_VERSION);
  gtk_about_dialog_set_comments(
      dialog,
      _("A free system task manager for Linux"));
  gtk_about_dialog_set_copyright(
      dialog, "Copyright (C) 2025 The Sysograph Project Contributors");
  gtk_about_dialog_set_license_type(dialog, GTK_LICENSE_GPL_3_0);
  gtk_about_dialog_set_logo_icon_name(dialog, "org.sysograph.Sysograph");
  gtk_about_dialog_set_translator_credits(dialog,
                                          "ru — The Sysograph Project");
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window_));
  g_signal_connect(dialog, "response", G_CALLBACK(&aboutResponseStatic),
                   nullptr);
  gtk_window_present(GTK_WINDOW(dialog));
}

void MainWindow::aboutResponseStatic(GtkDialog* dialog, int, gpointer) {
  gtk_window_destroy(GTK_WINDOW(dialog));
}

std::string MainWindow::startedTextFor(const ProcessInfo& p) {
  const long hz = ::sysconf(_SC_CLK_TCK);
  if (hz <= 0 || p.starttime_ticks == 0) return std::string("-");

  double uptime = 0.0;
  {
    std::ifstream up("/proc/uptime");
    up >> uptime;
  }
  if (uptime <= 0.0) return std::string("-");

  const double since_boot = static_cast<double>(p.starttime_ticks) / hz;
  if (since_boot > uptime) return std::string("-");

  // Seconds since boot -> wall clock epoch.
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  const double now_epoch = static_cast<double>(ts.tv_sec);
  const double start_epoch = now_epoch - (uptime - since_boot);

  char buf[64];
  time_t t = static_cast<time_t>(start_epoch);
  struct tm tm_buf;
  if (localtime_r(&t, &tm_buf) == nullptr) return std::string("-");
  if (now_epoch - start_epoch > 24.0 * 3600.0) {
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
  } else {
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm_buf);
  }
  return std::string(buf);
}

}  // namespace ui
}  // namespace sysograph
