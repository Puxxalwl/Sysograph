// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal, dependency-free reader for XDG .desktop entries (the subset
// needed for autostart listings). Implemented from the public XDG
// Desktop Entry Specification; no third-party library is used, which
// keeps the licensing surface of this project trivial.
#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sysograph {

class DesktopEntry {
 public:
  // Parses INI-style content. Malformed lines are ignored (never throw).
  static DesktopEntry parse(const std::string& content);

  // Returns a value from [Desktop Entry] (or any given group). Optional
  // surrounding quotes are stripped. Localised keys (Name[ru]) are not
  // resolved here; callers may look them up directly via value().
  std::optional<std::string> value(const std::string& key,
                                   const std::string& group = "Desktop Entry") const;

  // Re-serialises the entry, setting (or replacing) a key in the
  // [Desktop Entry] group. Used to write user-level autostart overrides.
  std::string serializedWithOverride(const std::string& key,
                                     const std::string& value) const;

 private:
  // group -> (key -> value), preserving insertion order via vector.
  std::vector<std::pair<std::string,
                        std::vector<std::pair<std::string, std::string>>>>
      groups_;
};

}  // namespace sysograph
