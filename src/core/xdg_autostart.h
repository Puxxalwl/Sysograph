// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Scans XDG autostart locations and manages user-level enable/disable
// overrides, exactly the way described by the public XDG Autostart
// Specification (user file with the same basename overrides the system
// entry; Hidden=true disables it).
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sysograph {

struct AutostartEntry {
  std::string id;         // basename of the .desktop file
  std::string name;       // Name= (fallback: id)
  std::string comment;    // Comment= (optional)
  std::string exec;       // Exec=
  bool enabled = true;    // effective state after overrides
  std::string origin;     // "user", "system" or "system+override"
  std::filesystem::path user_path;    // effective user override path
  std::filesystem::path system_path;  // empty for user-only entries
};

// Scans userDir (e.g. ~/.config/autostart) and systemDirs (e.g.
// /etc/xdg/autostart, /usr/share/autostart-style paths supplied by the
// caller). Unreadable files are skipped; entries without Name or Exec
// are still listed with fallbacks.
std::vector<AutostartEntry> scanAutostart(
    const std::filesystem::path& userDir,
    const std::vector<std::filesystem::path>& systemDirs);

// Enables/disables an entry by writing a user-level override (Hidden
// key). If the entry is system-level, the .desktop file is copied into
// the user directory first, then the Hidden key is set. Removing the
// override entirely when enabling a copied entry would restore the
// system default; instead we keep an explicit Hidden=false so the user
// choice survives system updates of the original file.
bool setAutostartEnabled(const AutostartEntry& entry, bool enabled,
                         std::string* error);

}  // namespace sysograph
