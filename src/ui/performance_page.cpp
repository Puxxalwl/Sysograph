// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/performance_page.h"

#include <glib.h>

#include <sys/statvfs.h>

#include <cmath>
#include <cstdio>
#include <string>

#include "core/formatter.h"
#include "core/i18n.h"

namespace sysograph {
namespace ui {

namespace {

GtkWidget* newCard(const char* title, GtkWidget* child) {
  GtkWidget* frame = gtk_frame_new(nullptr);
  gtk_widget_add_css_class(frame, "card");
  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 10);
  gtk_widget_set_margin_end(box, 10);
  gtk_widget_set_margin_top(box, 10);
  gtk_widget_set_margin_bottom(box, 10);
  gtk_frame_set_child(GTK_FRAME(frame), box);

  GtkLabel* label = GTK_LABEL(gtk_label_new(nullptr));
  std::string markup = std::string("<b>") + title + "</b>";
  gtk_label_set_markup(label, markup.c_str());
  gtk_label_set_xalign(label, 0.0f);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(label));
  gtk_box_append(GTK_BOX(box), child);
  return frame;
}

GtkLabel* bigValueLabel() {
  GtkLabel* label = GTK_LABEL(gtk_label_new("-"));
  gtk_label_set_xalign(label, 0.0f);
  gtk_widget_add_css_class(GTK_WIDGET(label), "big-value");
  return label;
}

GtkLabel* infoLabel() {
  GtkLabel* label = GTK_LABEL(gtk_label_new("-"));
  gtk_label_set_xalign(label, 0.0f);
  gtk_widget_add_css_class(GTK_WIDGET(label), "dim-label");
  gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
  gtk_label_set_wrap(label, TRUE);
  return label;
}

std::string rootFilesystemSummary() {
  struct statvfs vfs;
  if (statvfs("/", &vfs) != 0) return std::string();
  if (vfs.f_blocks == 0) return std::string();
  const uint64_t total =
      static_cast<uint64_t>(vfs.f_blocks) * static_cast<uint64_t>(vfs.f_frsize);
  const uint64_t free_bytes =
      static_cast<uint64_t>(vfs.f_bfree) * static_cast<uint64_t>(vfs.f_frsize);
  const uint64_t used = total > free_bytes ? total - free_bytes : 0;
  const double pct = total > 0 ? 100.0 * static_cast<double>(used) /
                                     static_cast<double>(total)
                               : 0.0;
  char buf[128];
  g_snprintf(buf, sizeof(buf), " / : %s / %s (%.1f %%)",
             formatBytes(used).c_str(), formatBytes(total).c_str(), pct);
  return std::string(buf);
}

}  // namespace

PerformancePage::PerformancePage() { build(); }PerformancePage::~PerformancePage() {
  delete cpu_graph_;
  delete mem_graph_;
  delete disk_graph_;
  delete net_graph_;
}

GtkWidget* PerformancePage::widget() { return root_; }

void PerformancePage::build() {
  root_ = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(root_), 12);
  gtk_grid_set_row_spacing(GTK_GRID(root_), 12);
  gtk_widget_set_margin_start(root_, 8);
  gtk_widget_set_margin_end(root_, 8);
  gtk_widget_set_margin_top(root_, 8);
  gtk_widget_set_margin_bottom(root_, 8);
  gtk_widget_set_hexpand(root_, TRUE);
  gtk_widget_set_vexpand(root_, TRUE);

  // --- CPU card ---
  cpu_graph_ = new LineGraph(true);
  {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    cpu_value_ = bigValueLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(cpu_value_));
    gtk_box_append(GTK_BOX(box), cpu_graph_->widget());
    cpu_info_ = infoLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(cpu_info_));
    cores_grid_ = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(cores_grid_, 12);
    gtk_grid_set_row_spacing(cores_grid_, 4);
    gtk_widget_set_hexpand(GTK_WIDGET(cores_grid_), TRUE);
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(cores_grid_));
    gtk_grid_attach(GTK_GRID(root_), newCard(_("CPU"), box), 0, 0, 1, 1);
  }

  // --- Memory card ---
  mem_graph_ = new LineGraph(false);
  mem_graph_->setSecondaryEnabled(true);
  {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    mem_value_ = bigValueLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(mem_value_));
    gtk_box_append(GTK_BOX(box), mem_graph_->widget());
    mem_info_ = infoLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(mem_info_));
    gtk_grid_attach(GTK_GRID(root_), newCard(_("Memory"), box), 1, 0, 1, 1);
  }

  // --- Disk card ---
  disk_graph_ = new LineGraph(false);
  disk_graph_->setSecondaryEnabled(true);
  {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    disk_value_ = bigValueLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(disk_value_));
    gtk_box_append(GTK_BOX(box), disk_graph_->widget());
    disk_info_ = infoLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(disk_info_));
    gtk_grid_attach(GTK_GRID(root_), newCard(_("Disk"), box), 0, 1, 1, 1);
  }

  // --- Network card ---
  net_graph_ = new LineGraph(false);
  net_graph_->setSecondaryEnabled(true);
  {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    net_value_ = bigValueLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(net_value_));
    gtk_box_append(GTK_BOX(box), net_graph_->widget());
    net_info_ = infoLabel();
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(net_info_));
    gtk_grid_attach(GTK_GRID(root_), newCard(_("Network"), box), 1, 1, 1, 1);
  }
}

void PerformancePage::ensureCoreBars(unsigned count) {
  if (count == core_bars_count_) return;
  // Detach the old cells (the grid owns them).
  for (GtkWidget* cell : core_cells_) {
    gtk_grid_remove(cores_grid_, cell);
  }
  core_cells_.clear();
  core_bars_.clear();
  core_bars_count_ = count;
  const unsigned columns = 4;
  for (unsigned i = 0; i < count; ++i) {
    GtkWidget* cell = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    char label_buf[16];
    g_snprintf(label_buf, sizeof(label_buf), "%u", i);
    GtkLabel* label = GTK_LABEL(gtk_label_new(label_buf));
    gtk_widget_add_css_class(GTK_WIDGET(label), "dim-label");
    gtk_box_append(GTK_BOX(cell), GTK_WIDGET(label));

    GtkWidget* bar = gtk_level_bar_new();
    gtk_level_bar_set_min_value(GTK_LEVEL_BAR(bar), 0.0);
    gtk_level_bar_set_max_value(GTK_LEVEL_BAR(bar), 1.0);
    gtk_widget_set_hexpand(bar, TRUE);
    gtk_widget_set_valign(bar, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(cell), bar);

    gtk_grid_attach(cores_grid_, cell, static_cast<int>(i % columns),
                    static_cast<int>(i / columns), 1, 1);
    core_cells_.push_back(cell);
    core_bars_.push_back(GTK_LEVEL_BAR(bar));
  }
}

void PerformancePage::update(const SampledMetrics& m) {
  // --- CPU ---
  cpu_graph_->pushPrimary(m.cpu_total_percent);
  gtk_label_set_text(cpu_value_, formatPercent(m.cpu_total_percent).c_str());
  char info[256];
  g_snprintf(info, sizeof(info), "%s: %zu   %s: %u   %s: %u   %s: %s",
             _("Processes"), m.processes.size(), _("Threads"),
             m.threads_total, _("Running"), m.procs_running, _("Uptime"),
             formatUptime(m.uptime_seconds).c_str());
  gtk_label_set_text(cpu_info_, info);

  if (m.cpu_per_core_percent.size() != core_bars_.size()) {
    ensureCoreBars(static_cast<unsigned>(m.cpu_per_core_percent.size()));
  }
  for (size_t i = 0; i < m.cpu_per_core_percent.size() &&
                    i < core_bars_.size();
       ++i) {
    gtk_level_bar_set_value(core_bars_[i],
                            m.cpu_per_core_percent[i] / 100.0);
  }

  // --- Memory ---
  mem_graph_->pushPrimary(static_cast<double>(m.memory.used()));
  mem_graph_->pushSecondary(static_cast<double>(m.memory.swap_used()));
  gtk_label_set_text(
      mem_value_,
      (formatBytes(m.memory.used()) + " / " + formatBytes(m.memory.total))
          .c_str());
  g_snprintf(info, sizeof(info),
             "%s: %s   %s: %s   %s: %s   %s: %s / %s",
             _("Available"), formatBytes(m.memory.available).c_str(),
             _("Buffers"), formatBytes(m.memory.buffers).c_str(),
             _("Cached"), formatBytes(m.memory.cached).c_str(), _("Swap"),
             formatBytes(m.memory.swap_used()).c_str(),
             formatBytes(m.memory.swap_total).c_str());
  gtk_label_set_text(mem_info_, info);

  // --- Disk ---
  disk_graph_->pushPrimary(m.disk_read_bps);
  disk_graph_->pushSecondary(m.disk_write_bps);
  gtk_label_set_text(
      disk_value_,
      (std::string(_("Read")) + ": " + formatBytesPerSecond(m.disk_read_bps) +
       "   " + _("Write") + ": " + formatBytesPerSecond(m.disk_write_bps))
          .c_str());
  {
    const std::string fs = rootFilesystemSummary();
    g_snprintf(info, sizeof(info), "%s: %.0f + %.0f ops/s%s",
               _("Operations"), m.disk_read_ops, m.disk_write_ops,
               fs.empty() ? "" : fs.c_str());
    gtk_label_set_text(disk_info_, info);
  }

  // --- Network ---
  net_graph_->pushPrimary(m.net_rx_bps);
  net_graph_->pushSecondary(m.net_tx_bps);
  gtk_label_set_text(
      net_value_,
      (std::string(_("Receiving")) + ": " +
       formatBytesPerSecond(m.net_rx_bps) + "   " + _("Sending") + ": " +
       formatBytesPerSecond(m.net_tx_bps))
          .c_str());
  g_snprintf(info, sizeof(info), "%s: %s   %s: %s",
             _("Total received"), formatBytes(m.net.rx_bytes).c_str(),
             _("Total sent"), formatBytes(m.net.tx_bytes).c_str());
  gtk_label_set_text(net_info_, info);
}

}  // namespace ui
}  // namespace sysograph
