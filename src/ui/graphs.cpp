// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/graphs.h"

#include <algorithm>

namespace sysograph {
namespace ui {

constexpr double LineGraph::kPrimary[4];
constexpr double LineGraph::kPrimaryFill[4];
constexpr double LineGraph::kSecondary[4];
constexpr double LineGraph::kGrid[4];
constexpr double LineGraph::kFrame[4];

LineGraph::LineGraph(bool fixed_scale) : fixed_scale_(fixed_scale) {
  area_ = GTK_DRAWING_AREA(gtk_drawing_area_new());
  gtk_drawing_area_set_content_height(area_, 84);
  gtk_widget_set_hexpand(GTK_WIDGET(area_), TRUE);
  gtk_widget_set_size_request(GTK_WIDGET(area_), 180, 84);
  gtk_drawing_area_set_draw_func(area_, &LineGraph::drawStatic, this, nullptr);
}

LineGraph::~LineGraph() {
  // The widget itself is owned by its container; nothing to free here.
}

GtkWidget* LineGraph::widget() { return GTK_WIDGET(area_); }

void LineGraph::pushPrimary(double v) {
  if (std::isnan(v)) v = 0.0;
  primary_.push(v);
}

void LineGraph::pushSecondary(double v) {
  if (std::isnan(v)) v = 0.0;
  secondary_.push(v);
}

void LineGraph::drawStatic(GtkDrawingArea* area, cairo_t* cr, int width,
                           int height, gpointer user_data) {
  (void)area;
  static_cast<LineGraph*>(user_data)->draw(cr, width, height);
}

void LineGraph::draw(cairo_t* cr, int width, int height) {
  // Background.
  cairo_set_source_rgba(cr, 0.06, 0.08, 0.11, 1.0);
  cairo_paint(cr);

  // Horizontal grid at 25/50/75%.
  cairo_set_source_rgba(cr, kGrid[0], kGrid[1], kGrid[2], kGrid[3]);
  cairo_set_line_width(cr, 1.0);
  for (int i = 1; i <= 3; ++i) {
    const double y = height * i / 4.0;
    cairo_move_to(cr, 0, y);
    cairo_line_to(cr, width, y);
    cairo_stroke(cr);
  }

  drawSeries(cr, width, height, true);
  if (secondary_enabled_) drawSeries(cr, width, height, false);

  // Frame.
  cairo_set_source_rgba(cr, kFrame[0], kFrame[1], kFrame[2], kFrame[3]);
  cairo_set_line_width(cr, 1.0);
  cairo_rectangle(cr, 0.5, 0.5, width - 1.0, height - 1.0);
  cairo_stroke(cr);
}

void LineGraph::drawSeries(cairo_t* cr, int width, int height,
                           bool primary_series) {
  const HistoryBuffer<double, 90>& buf =
      primary_series ? primary_ : secondary_;
  if (buf.size() < 2) return;

  double max_value = 1.0;
  if (!fixed_scale_) {
    for (size_t i = 0; i < buf.size(); ++i) {
      const double v = buf.newest(i);
      if (v > max_value) max_value = v;
    }
    max_value *= 1.15;
  }

  const double* color =
      primary_series ? kPrimary : kSecondary;
  const double* fill = kPrimaryFill;

  const size_t n = buf.size();
  const double step_x =
      static_cast<double>(width) /
      static_cast<double>(std::max<size_t>(1, buf.capacity() - 1));
  // n values share the full width, oldest on the left.
  const double x0 = static_cast<double>(width) - static_cast<double>(n) * step_x;

  auto value_to_y = [&](double v) {
    double frac = v / (fixed_scale_ ? 100.0 : max_value);
    frac = std::clamp(frac, 0.0, 1.0);
    return (1.0 - frac) * (height - 2.0) + 1.0;
  };

  // Path.
  cairo_new_path(cr);
  for (size_t i = 0; i < n; ++i) {
    // i = 0 is the newest; draw left-to-right (oldest first).
    const double v = buf.newest(n - 1 - i);
    const double x = x0 + static_cast<double>(i) * step_x;
    const double y = value_to_y(v);
    if (i == 0) cairo_move_to(cr, x, y);
    else cairo_line_to(cr, x, y);
  }

  if (primary_series) {
    // Filled area down to the bottom.
    cairo_save(cr);
    cairo_line_to(cr, static_cast<double>(width), height);
    cairo_line_to(cr, x0, height);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, fill[0], fill[1], fill[2], fill[3]);
    cairo_fill(cr);
    cairo_restore(cr);
    // Rebuild the line path (fill consumed it).
    cairo_new_path(cr);
    for (size_t i = 0; i < n; ++i) {
      const double v = buf.newest(n - 1 - i);
      const double x = x0 + static_cast<double>(i) * step_x;
      const double y = value_to_y(v);
      if (i == 0) cairo_move_to(cr, x, y);
      else cairo_line_to(cr, x, y);
    }
  }

  cairo_set_source_rgba(cr, color[0], color[1], color[2], color[3]);
  cairo_set_line_width(cr, 2.0);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  cairo_stroke(cr);
}

}  // namespace ui
}  // namespace sysograph
