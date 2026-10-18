// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/xdg_autostart.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>

#include "core/desktop_entry.h"

namespace sysograph {
namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool isHidden(const DesktopEntry& entry) {
  auto hidden = entry.value("Hidden");
  if (!hidden) return false;
  std::string v = *hidden;
  std::transform(v.begin(), v.end(), v.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return v == "true" || v == "1" || v == "yes";
}

AutostartEntry makeEntry(const std::filesystem::path& path,
                         const std::filesystem::path& userDir,
                         const std::filesystem::path& systemPath) {
  AutostartEntry e;
  e.id = path.filename().string();
  const DesktopEntry entry = DesktopEntry::parse(readFile(path));
  e.name = entry.value("Name").value_or(e.id);
  e.comment = entry.value("Comment").value_or("");
  e.exec = entry.value("Exec").value_or("");
  e.enabled = !isHidden(entry);
  e.user_path = userDir / path.filename();
  e.system_path = systemPath;
  e.origin = systemPath.empty() ? "user" : "system+override";
  return e;
}

}  // namespace

std::vector<AutostartEntry> scanAutostart(
    const std::filesystem::path& userDir,
    const std::vector<std::filesystem::path>& systemDirs) {
  std::vector<AutostartEntry> out;
  std::set<std::string> seen;

  // User entries take precedence over same-id system entries.
  std::error_code ec;
  if (std::filesystem::exists(userDir, ec)) {
    for (const auto& f : std::filesystem::directory_iterator(userDir, ec)) {
      if (!f.is_regular_file()) continue;
      if (f.path().extension() != ".desktop") continue;
      const std::string id = f.path().filename().string();
      if (!seen.insert(id).second) continue;
      out.push_back(makeEntry(f.path(), userDir, {}));
    }
  }

  for (const auto& dir : systemDirs) {
    if (!std::filesystem::exists(dir, ec)) continue;
    for (const auto& f : std::filesystem::directory_iterator(dir, ec)) {
      if (!f.is_regular_file()) continue;
      if (f.path().extension() != ".desktop") continue;
      const std::string id = f.path().filename().string();
      if (seen.count(id)) continue;  // user override wins
      seen.insert(id);
      out.push_back(makeEntry(f.path(), userDir, f.path()));
    }
  }

  std::sort(out.begin(), out.end(),
            [](const AutostartEntry& a, const AutostartEntry& b) {
              return a.name < b.name;
            });
  return out;
}

bool setAutostartEnabled(const AutostartEntry& entry, bool enabled,
                         std::string* error) {
  const std::filesystem::path source =
      std::filesystem::exists(entry.user_path) ? entry.user_path
                                               : entry.system_path;
  if (source.empty()) {
    if (error) *error = "no source .desktop file";
    return false;
  }

  std::string content = readFile(source);
  if (content.empty()) {
    if (error) *error = "cannot read " + source.string();
    return false;
  }

  const DesktopEntry parsed = DesktopEntry::parse(content);
  const std::string updated = parsed.serializedWithOverride(
      "Hidden", enabled ? "false" : "true");

  std::error_code ec;
  std::filesystem::create_directories(entry.user_path.parent_path(), ec);
  std::ofstream out(entry.user_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot write " + entry.user_path.string();
    return false;
  }
  out << updated;
  if (!out) {
    if (error) *error = "write failed for " + entry.user_path.string();
    return false;
  }
  return true;
}

}  // namespace sysograph
