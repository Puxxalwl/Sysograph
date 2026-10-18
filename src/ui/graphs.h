// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Self-contained Cairo line/area graphs used by the Performance page.
// The widgets keep a fixed-size history ring buffer (never grow), draw
// grid lines plus one filled and one optional secondary series, and use
// the project's own colour palette (see data/style.css for the rest).
#pragma once

#include <gtk/gtk.h>

#include <cairo.h>

#include <cmath>
#include <limits>

#include "core/sampler.h"

namespace sysograph {
namespace ui {

class LineGraph {
 public:
  // fixed_scale: y axis is pinned to 0..100 (percent graphs); otherwise
  // the axis auto-scales to the series maximum with headroom.
  explicit LineGraph(bool fixed_scale);
  ~LineGraph();

  GtkWidget* widget();

  void pushPrimary(double v);
  void pushSecondary(double v);
  void setSecondaryEnabled(bool enabled) { secondary_enabled_ = enabled; }

 private:
  static void drawStatic(GtkDrawingArea* area, cairo_t* cr, int width,
                         int height, gpointer user_data);
  void draw(cairo_t* cr, int width, int height);
  void drawSeries(cairo_t* cr, int width, int height, bool primary);

  GtkDrawingArea* area_ = nullptr;
  HistoryBuffer<double, 90> primary_;
  HistoryBuffer<double, 90> secondary_;
  bool fixed_scale_;
  bool secondary_enabled_ = false;

  // Project palette (original artwork choice).
  static constexpr double kPrimary[4] = {0.21, 0.52, 0.89, 1.0};
  static constexpr double kPrimaryFill[4] = {0.21, 0.52, 0.89, 0.28};
  static constexpr double kSecondary[4] = {0.90, 0.38, 0.00, 1.0};
  static constexpr double kGrid[4] = {0.5, 0.5, 0.5, 0.35};
  static constexpr double kFrame[4] = {0.3, 0.3, 0.3, 0.8};
};

}  // namespace ui
}  // namespace sysograph
