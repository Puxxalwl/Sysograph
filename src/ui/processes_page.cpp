// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/processes_page.h"

#include <glib.h>

#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "core/formatter.h"
#include "core/i18n.h"
#include "core/procfs.h"
#include "core/sampler.h"

namespace sysograph {
namespace ui {

namespace {

enum {
  PROC_COL_PID,
  PROC_COL_NAME,
  PROC_COL_USER,
  PROC_COL_CPU,
  PROC_COL_MEM,
  PROC_COL_STATE,
  PROC_COL_PPID,
  PROC_COL_CMD,
  PROC_COL_HAYSTACK,
  PROC_COL_THREADS,
  PROC_COL_NICE,
  PROC_COL_SHARED,
  PROC_N_COLUMNS
};

std::string toLower(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

struct KillRequest {
  ProcessesPage* page = nullptr;
  int pid = 0;
  bool force = false;
  std::string name;
};

}  // namespace

ProcessesPage::ProcessesPage(GtkWidget* parentWindow)
    : parent_window_(parentWindow) {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(root_, 8);
  gtk_widget_set_margin_end(root_, 8);
  gtk_widget_set_margin_top(root_, 8);
  gtk_widget_set_margin_bottom(root_, 8);

  buildModel();
  buildToolbar();
  buildTree();
  buildDetails();
}

ProcessesPage::~ProcessesPage() = default;

GtkWidget* ProcessesPage::widget() { return root_; }

void ProcessesPage::buildModel() {
  store_ = gtk_list_store_new(PROC_N_COLUMNS, G_TYPE_INT, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_UINT64,
                              G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                              G_TYPE_UINT64);
  filter_ = GTK_TREE_MODEL_FILTER(
      gtk_tree_model_filter_new(GTK_TREE_MODEL(store_), nullptr));
  gtk_tree_model_filter_set_visible_func(filter_, &visibleFuncStatic, this,
                                         nullptr);
  sort_ = GTK_TREE_MODEL_SORT(
      gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(filter_)));
}

void ProcessesPage::buildToolbar() {
  GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(root_), bar);

  search_ = GTK_SEARCH_ENTRY(gtk_search_entry_new());
  gtk_search_entry_set_placeholder_text(search_, _("Search"));
  gtk_widget_set_tooltip_text(GTK_WIDGET(search_),
                              _("Filter processes by name, user or command"));
  gtk_widget_set_hexpand(GTK_WIDGET(search_), TRUE);
  g_signal_connect(search_, "search-changed",
                   G_CALLBACK(&searchChangedStatic), this);
  gtk_box_append(GTK_BOX(bar), GTK_WIDGET(search_));

  end_button_ = gtk_button_new_with_label(_("End process"));
  g_signal_connect(end_button_, "clicked", G_CALLBACK(&endTaskStatic), this);
  gtk_box_append(GTK_BOX(bar), end_button_);

  kill_button_ = gtk_button_new_with_label(_("Force kill"));
  g_signal_connect(kill_button_, "clicked", G_CALLBACK(&forceKillStatic),
                   this);
  gtk_box_append(GTK_BOX(bar), kill_button_);
}

namespace {

GtkTreeViewColumn* makeTextColumn(const char* title, int col,
                                  bool sortable, int xalign = 0.0) {
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  g_object_set(renderer, "xalign", static_cast<double>(xalign), nullptr);
  GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
      title, renderer, "text", col, nullptr);
  gtk_tree_view_column_set_resizable(column, TRUE);
  if (sortable) {
    gtk_tree_view_column_set_sort_column_id(column, col);
    gtk_tree_view_column_set_clickable(column, TRUE);
  }
  return column;
}

}  // namespace

void ProcessesPage::buildTree() {
  GtkWidget* scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled),
                                             220);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(root_), scrolled);

  view_ = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_model(view_, GTK_TREE_MODEL(sort_));

  gtk_tree_view_append_column(
      view_, makeTextColumn(_("Name"), PROC_COL_NAME, true));
  gtk_tree_view_append_column(
      view_, makeTextColumn(_("PID"), PROC_COL_PID, true, 1.0));
  gtk_tree_view_append_column(
      view_, makeTextColumn(_("User"), PROC_COL_USER, true));
  {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, nullptr);
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        _("CPU"), renderer, nullptr);
    gtk_tree_view_column_set_cell_data_func(
        column, renderer, &cellCpuStatic, this, nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, PROC_COL_CPU);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view_, column);
  }
  {
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, nullptr);
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        _("Memory"), renderer, nullptr);
    gtk_tree_view_column_set_cell_data_func(
        column, renderer, &cellMemStatic, this, nullptr);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, PROC_COL_MEM);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_append_column(view_, column);
  }
  gtk_tree_view_append_column(
      view_, makeTextColumn(_("State"), PROC_COL_STATE, false));
  gtk_tree_view_append_column(
      view_, makeTextColumn(_("Threads"), PROC_COL_THREADS, true, 1.0));
  gtk_tree_view_append_column(
      view_, makeTextColumn(_("Priority"), PROC_COL_NICE, true, 1.0));
  gtk_tree_view_append_column(
      view_, makeTextColumn(_("Command line"), PROC_COL_CMD, true));

  gtk_tree_view_set_headers_clickable(view_, TRUE);

  GtkTreeSelection* selection = gtk_tree_view_get_selection(view_);
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  g_signal_connect(selection, "changed",
                   G_CALLBACK(&selectionChangedStatic), this);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(view_));
}

void ProcessesPage::buildDetails() {
  GtkWidget* frame = gtk_frame_new(nullptr);
  gtk_widget_set_margin_top(frame, 4);
  gtk_box_append(GTK_BOX(root_), frame);

  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);
  gtk_frame_set_child(GTK_FRAME(frame), box);

  d_name_ = GTK_LABEL(gtk_label_new(_("No process selected")));
  gtk_widget_set_halign(GTK_WIDGET(d_name_), GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(d_name_));

  GtkWidget* grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 18);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
  gtk_box_append(GTK_BOX(box), grid);

  auto add_row = [&](int row, const char* title, GtkLabel** value_slot) {
    GtkLabel* title_label =
        GTK_LABEL(gtk_label_new(nullptr));
    gtk_label_set_markup(title_label,
                         (std::string("<span alpha=\"55%\">") + title +
                          "</span>")
                             .c_str());
    gtk_widget_set_halign(GTK_WIDGET(title_label), GTK_ALIGN_START);
    gtk_label_set_xalign(title_label, 0.0f);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(title_label), 0, row, 1, 1);
    GtkLabel* value = GTK_LABEL(gtk_label_new("-"));
    gtk_label_set_xalign(value, 0.0f);
    gtk_label_set_selectable(value, TRUE);
    gtk_label_set_ellipsize(value, PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(GTK_WIDGET(value), TRUE);
    gtk_widget_set_halign(GTK_WIDGET(value), GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(value), 1, row, 1, 1);
    *value_slot = value;
  };

  add_row(0, _("PID"), &d_pid_);
  add_row(1, _("State"), &d_state_);
  add_row(2, _("User"), &d_user_);
  add_row(3, _("CPU"), &d_cpu_);
  add_row(4, _("Memory"), &d_mem_);
  add_row(5, _("Virtual memory"), &d_virt_);
  add_row(6, _("Threads"), &d_threads_);
  add_row(7, _("Priority"), &d_nice_);
  add_row(8, _("CPU time"), &d_cpu_time_);
  add_row(9, _("Started"), &d_started_);
  add_row(10, _("Executable"), &d_exe_);
  add_row(11, _("Working directory"), &d_cwd_);
  add_row(12, _("Open files"), &d_fds_);

  d_cmd_ = GTK_LABEL(gtk_label_new(nullptr));
  gtk_label_set_xalign(d_cmd_, 0.0f);
  gtk_label_set_selectable(d_cmd_, TRUE);
  gtk_label_set_ellipsize(d_cmd_, PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign(GTK_WIDGET(d_cmd_), GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(d_cmd_));
}

void ProcessesPage::update(const SampledMetrics& metrics) {
  metrics_ = metrics;

  std::unordered_map<int, const ProcessInfo*> infos;
  for (const auto& p : metrics_.processes) infos[p.pid] = &p;

  // Update existing rows in place, remove vanished processes.
  GtkTreeIter iter;
  gboolean valid =
      gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_), &iter);
  std::unordered_map<int, bool> seen;
  while (valid) {
    int pid = 0;
    gtk_tree_model_get(GTK_TREE_MODEL(store_), &iter, PROC_COL_PID, &pid,
                       -1);
    auto it = infos.find(pid);
    if (it == infos.end()) {
      valid = gtk_list_store_remove(store_, &iter);
      continue;
    }
    if (seen.insert({pid, true}).second) {
      setRow(store_, &iter, *it->second);
    } else {
      valid = gtk_list_store_remove(store_, &iter);
      continue;
    }
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_), &iter);
  }

  // Append new processes.
  for (const auto& [pid, info] : infos) {
    if (seen.count(pid)) continue;
    GtkTreeIter new_iter;
    gtk_list_store_append(store_, &new_iter);
    setRow(store_, &new_iter, *info);
  }

  refreshSelection();
}

void ProcessesPage::setRow(GtkListStore* store, GtkTreeIter* iter,
                           const ProcessInfo& p) {
  std::string haystack = toLower(p.name + " " + p.user + " " + p.cmdline);
  std::string state_text = _(process_state_key(p.state));
  gtk_list_store_set(store, iter, PROC_COL_PID, p.pid, PROC_COL_NAME,
                     p.name.c_str(), PROC_COL_USER, p.user.c_str(),
                     PROC_COL_CPU, p.cpu_percent, PROC_COL_MEM,
                     static_cast<guint64>(p.rss_bytes), PROC_COL_STATE,
                     state_text.c_str(), PROC_COL_PPID, p.ppid, PROC_COL_CMD,
                     p.cmdline.c_str(), PROC_COL_HAYSTACK,
                     haystack.c_str(), PROC_COL_THREADS, p.threads,
                     PROC_COL_NICE, p.nice, PROC_COL_SHARED,
                     static_cast<guint64>(p.shared_bytes), -1);
}

gboolean ProcessesPage::visibleFuncStatic(GtkTreeModel* model,
                                          GtkTreeIter* iter,
                                          gpointer user_data) {
  ProcessesPage* self = static_cast<ProcessesPage*>(user_data);
  if (self->search_lower_.empty()) return TRUE;
  gchar* hay = nullptr;
  gtk_tree_model_get(model, iter, PROC_COL_HAYSTACK, &hay, -1);
  gboolean visible = FALSE;
  if (hay) {
    std::string hay_lower = toLower(hay);
    visible = hay_lower.find(self->search_lower_) != std::string::npos
                  ? TRUE
                  : FALSE;
    g_free(hay);
  }
  return visible;
}

void ProcessesPage::cellCpuStatic(GtkTreeViewColumn*, GtkCellRenderer* cell,
                                  GtkTreeModel* model, GtkTreeIter* iter,
                                  gpointer) {
  gdouble cpu = 0.0;
  gtk_tree_model_get(model, iter, PROC_COL_CPU, &cpu, -1);
  char buf[32];
  g_snprintf(buf, sizeof(buf), "%.1f %%", static_cast<double>(cpu));
  g_object_set(cell, "text", buf, nullptr);
}

void ProcessesPage::cellMemStatic(GtkTreeViewColumn*, GtkCellRenderer* cell,
                                  GtkTreeModel* model, GtkTreeIter* iter,
                                  gpointer) {
  guint64 mem = 0;
  gtk_tree_model_get(model, iter, PROC_COL_MEM, &mem, -1);
  g_object_set(cell, "text", formatBytes(mem).c_str(), nullptr);
}

void ProcessesPage::selectionChangedStatic(GtkTreeSelection* sel,
                                            gpointer user_data) {
  ProcessesPage* self = static_cast<ProcessesPage*>(user_data);
  (void)sel;
  self->refreshSelection();
}

void ProcessesPage::refreshSelection() {
  GtkTreeSelection* selection = gtk_tree_view_get_selection(view_);
  GtkTreeModel* model = nullptr;
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    int pid = 0;
    gtk_tree_model_get(model, &iter, PROC_COL_PID, &pid, -1);
    updateDetails(pid);
    gtk_widget_set_sensitive(end_button_, TRUE);
    gtk_widget_set_sensitive(kill_button_, TRUE);
  } else {
    gtk_label_set_text(d_name_, _("No process selected"));
    gtk_widget_set_sensitive(end_button_, FALSE);
    gtk_widget_set_sensitive(kill_button_, FALSE);
  }
}

void ProcessesPage::updateDetails(int pid) {
  const ProcessInfo* p = nullptr;
  for (const auto& proc : metrics_.processes) {
    if (proc.pid == pid) {
      p = &proc;
      break;
    }
  }
  if (!p) {
    gtk_label_set_text(d_name_, _("No process selected"));
    return;
  }

  gtk_label_set_text(d_name_, p->name.c_str());
  gtk_label_set_text(d_pid_, std::to_string(p->pid).c_str());
  gtk_label_set_text(d_state_, _(process_state_key(p->state)));
  gtk_label_set_text(d_user_, p->user.c_str());
  gtk_label_set_text(d_cpu_, formatPercent(p->cpu_percent).c_str());
  gtk_label_set_text(d_mem_, formatBytes(p->rss_bytes).c_str());
  gtk_label_set_text(d_virt_, formatBytes(p->vsize_bytes).c_str());
  gtk_label_set_text(d_threads_, std::to_string(p->threads).c_str());
  gtk_label_set_text(d_nice_, std::to_string(p->nice).c_str());

  const long hz = ::sysconf(_SC_CLK_TCK);
  const double user_s = hz > 0 ? static_cast<double>(p->utime) / hz : 0.0;
  const double sys_s = hz > 0 ? static_cast<double>(p->stime) / hz : 0.0;
  gtk_label_set_text(
      d_cpu_time_,
      (formatDurationHMS(user_s) + " / " + formatDurationHMS(sys_s)).c_str());
  gtk_label_set_text(
      d_started_,
      started_formatter_ ? started_formatter_(*p).c_str() : "-");

  const std::filesystem::path procDir =
      std::filesystem::path("/proc") / std::to_string(pid);
  std::string exe = Procfs::readLinkField(procDir, "exe");
  gtk_label_set_text(d_exe_, exe.empty() ? _("Not available")
                                         : exe.c_str());
  std::string cwd = Procfs::readLinkField(procDir, "cwd");
  gtk_label_set_text(d_cwd_, cwd.empty() ? _("Not available")
                                         : cwd.c_str());
  const int fds = Procfs::countFileDescriptors(procDir);
  gtk_label_set_text(d_fds_,
                     fds < 0 ? _("Not available")
                             : std::to_string(fds).c_str());
  gtk_label_set_text(
      d_cmd_, p->cmdline.empty() ? p->name.c_str() : p->cmdline.c_str());
}

void ProcessesPage::searchChangedStatic(GtkSearchEntry* entry,
                                         gpointer user_data) {
  ProcessesPage* self = static_cast<ProcessesPage*>(user_data);
  const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));
  self->search_lower_ = toLower(text ? text : "");
  gtk_tree_model_filter_refilter(self->filter_);
}

void ProcessesPage::endTaskStatic(GtkButton*, gpointer user_data) {
  static_cast<ProcessesPage*>(user_data)->requestKill(
      static_cast<ProcessesPage*>(user_data)->selectedPid(), false);
}

void ProcessesPage::forceKillStatic(GtkButton*, gpointer user_data) {
  static_cast<ProcessesPage*>(user_data)->requestKill(
      static_cast<ProcessesPage*>(user_data)->selectedPid(), true);
}

int ProcessesPage::selectedPid() {
  GtkTreeSelection* selection = gtk_tree_view_get_selection(view_);
  GtkTreeModel* model = nullptr;
  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return -1;
  int pid = -1;
  gtk_tree_model_get(model, &iter, PROC_COL_PID, &pid, -1);
  return pid;
}

void ProcessesPage::requestKill(int pid, bool force) {
  if (pid <= 0) return;
  const ProcessInfo* p = nullptr;
  for (const auto& proc : metrics_.processes) {
    if (proc.pid == pid) {
      p = &proc;
      break;
    }
  }
  auto* req = new KillRequest{this, pid, force,
                              p ? p->name : std::to_string(pid)};

  const char* message =
      force
          ? _("Force kill the selected process? It will be terminated immediately (SIGKILL) and unsaved work will be lost.")
          : _("End the selected process? It will be asked to terminate (SIGTERM) and unsaved work may be lost.");

  std::string title = (force ? _("Force kill") : _("End process"));
  title += ": ";
  title += req->name;
  title += " (";
  title += std::to_string(pid);
  title += ")";

  const char* buttons[] = {title.c_str(), _("Cancel"), nullptr};
  GtkAlertDialog* dialog = gtk_alert_dialog_new("%s", message);
  gtk_alert_dialog_set_detail(dialog, message);
  gtk_alert_dialog_set_buttons(dialog, buttons);
  gtk_alert_dialog_set_default_button(dialog, 1);
  gtk_alert_dialog_set_cancel_button(dialog, 1);
  gtk_alert_dialog_choose(dialog, GTK_WINDOW(parent_window_), nullptr,
                          &chooseKillResponseStatic, req);
  g_object_unref(dialog);
}

void ProcessesPage::chooseKillResponseStatic(GObject* source,
                                             GAsyncResult* res,
                                             gpointer user_data) {
  std::unique_ptr<KillRequest> req(
      static_cast<KillRequest*>(user_data));
  GtkAlertDialog* dialog = GTK_ALERT_DIALOG(source);
  GError* error = nullptr;
  const int response = gtk_alert_dialog_choose_finish(dialog, res, &error);
  g_clear_error(&error);
  if (response != 0) return;

  const int signal_number = req->force ? SIGKILL : SIGTERM;
  if (::kill(req->pid, signal_number) == 0) {
    return;  // success; the next refresh will drop the row
  }
  const std::string message = std::string(req->name) + " (" +
                              std::to_string(req->pid) + "): " +
                              std::strerror(errno);
  req->page->showResult(message.c_str());
}

void ProcessesPage::showResult(const char* message) {
  GtkAlertDialog* dialog = gtk_alert_dialog_new("%s", message);
  const char* buttons[] = {_("OK"), nullptr};
  gtk_alert_dialog_set_buttons(dialog, buttons);
  gtk_alert_dialog_show(dialog, GTK_WINDOW(parent_window_));
  g_object_unref(dialog);
}

}  // namespace ui
}  // namespace sysograph
