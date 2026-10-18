// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/desktop_entry.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace sysograph {
namespace {

std::string trim(const std::string& s) {
  size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos) return {};
  size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

std::string unquote(const std::string& s) {
  std::string t = trim(s);
  if (t.size() >= 2 && ((t.front() == '"' && t.back() == '"') ||
                        (t.front() == '\'' && t.back() == '\''))) {
    return t.substr(1, t.size() - 2);
  }
  return t;
}

}  // namespace

DesktopEntry DesktopEntry::parse(const std::string& content) {
  DesktopEntry entry;
  std::string currentGroup;
  std::istringstream iss(content);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const std::string t = trim(line);
    if (t.empty() || t[0] == '#') continue;
    if (t.front() == '[' && t.back() == ']') {
      currentGroup = t.substr(1, t.size() - 2);
      for (auto& g : entry.groups_) {
        if (g.first == currentGroup) goto group_exists;
      }
      entry.groups_.emplace_back(currentGroup,
                                 std::vector<std::pair<std::string, std::string>>{});
    group_exists:
      continue;
    }
    const size_t eq = t.find('=');
    if (eq == std::string::npos || currentGroup.empty()) continue;
    const std::string key = trim(t.substr(0, eq));
    const std::string value = unquote(t.substr(eq + 1));
    for (auto& g : entry.groups_) {
      if (g.first != currentGroup) continue;
      for (auto& kv : g.second) {
        if (kv.first == key) {
          kv.second = value;
          goto stored;
        }
      }
      g.second.emplace_back(key, value);
    stored:
      break;
    }
  }
  return entry;
}

std::optional<std::string> DesktopEntry::value(
    const std::string& key, const std::string& group) const {
  for (const auto& g : groups_) {
    if (g.first != group) continue;
    for (const auto& kv : g.second) {
      if (kv.first == key) return kv.second;
    }
  }
  return std::nullopt;
}

std::string DesktopEntry::serializedWithOverride(const std::string& key,
                                                 const std::string& value) const {
  std::ostringstream out;
  bool desktopGroupDone = false;
  bool overrideWritten = false;

  std::vector<std::pair<std::string, std::string>> currentGroup;
  std::string currentGroupName;
  auto flush = [&](bool isLast) {
    if (currentGroupName.empty()) return;
    out << "[" << currentGroupName << "]\n";
    for (const auto& kv : currentGroup) {
      if (currentGroupName == "Desktop Entry" && kv.first == key) {
        out << key << "=" << value << "\n";
        overrideWritten = true;
      } else {
        out << kv.first << "=" << kv.second << "\n";
      }
    }
    if (currentGroupName == "Desktop Entry" && !overrideWritten) {
      out << key << "=" << value << "\n";
      overrideWritten = true;
      desktopGroupDone = true;
    }
    if (!isLast) out << "\n";
    currentGroup.clear();
    currentGroupName.clear();
  };

  for (const auto& g : groups_) {
    if (g.first == "Desktop Entry") desktopGroupDone = true;
    if (g.first != currentGroupName) {
      flush(false);
      currentGroupName = g.first;
    }
    for (const auto& kv : g.second) currentGroup.push_back(kv);
  }
  flush(true);

  if (!desktopGroupDone) {
    out << "\n[Desktop Entry]\n" << key << "=" << value << "\n";
  }
  return out.str();
}

}  // namespace sysograph
