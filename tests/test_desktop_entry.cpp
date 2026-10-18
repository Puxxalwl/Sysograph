// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_util.h"

#include <filesystem>
#include <fstream>
#include <unistd.h>

#include "core/desktop_entry.h"
#include "core/xdg_autostart.h"

void run_desktop_entry_tests() {
  // --- Parsing -------------------------------------------------------------
  {
    const std::string content =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Example App\n"
        "Name[ru]=Пример\n"
        "Exec=/usr/bin/example --flag\n"
        "Comment=\"Quoted comment\"\n"
        "Hidden=false\n"
        "\n"
        "[Other Group]\n"
        "Key=value\n"
        "NotAnAssignment\n";
    const sysograph::DesktopEntry entry =
        sysograph::DesktopEntry::parse(content);
    CHECK(entry.value("Type").value_or("") == "Application");
    CHECK(entry.value("Name").value_or("") == "Example App");
    CHECK(entry.value("Name[ru]").value_or("") == "Пример");
    CHECK(entry.value("Exec").value_or("") == "/usr/bin/example --flag");
    CHECK(entry.value("Comment").value_or("") == "Quoted comment");
    CHECK(entry.value("Key", "Other Group").value_or("") == "value");
    CHECK(!entry.value("Missing").has_value());
  }
  {
    // Malformed content must parse into an empty-ish entry without throwing.
    const sysograph::DesktopEntry entry =
        sysograph::DesktopEntry::parse("=novalue\nno-equals\n[]\n");
    CHECK(!entry.value("no-equals").has_value());
  }

  // --- Override serialization ----------------------------------------------
  {
    const std::string content =
        "[Desktop Entry]\nType=Application\nName=App\nExec=/bin/app\n";
    const sysograph::DesktopEntry entry =
        sysograph::DesktopEntry::parse(content);
    const std::string withHidden =
        entry.serializedWithOverride("Hidden", "true");
    const sysograph::DesktopEntry reparsed =
        sysograph::DesktopEntry::parse(withHidden);
    CHECK(reparsed.value("Hidden").value_or("") == "true");
    CHECK(reparsed.value("Name").value_or("") == "App");
    CHECK(reparsed.value("Exec").value_or("") == "/bin/app");

    const std::string withEnabled =
        reparsed.serializedWithOverride("Hidden", "false");
    const sysograph::DesktopEntry again =
        sysograph::DesktopEntry::parse(withEnabled);
    CHECK(again.value("Hidden").value_or("") == "false");
  }

  // --- XDG autostart scanning and overrides --------------------------------
  {
    std::error_code ec;
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() /
        ("sysograph_autostart_" + std::to_string(::getpid()));
    std::filesystem::remove_all(base, ec);
    const std::filesystem::path userDir = base / "user" / "autostart";
    const std::filesystem::path systemDir = base / "system" / "autostart";
    std::filesystem::create_directories(userDir);
    std::filesystem::create_directories(systemDir);

    {
      std::ofstream out(systemDir / "system.desktop");
      out << "[Desktop Entry]\nType=Application\nName=System Tool\n"
             "Exec=/usr/bin/systemtool\nComment=Runs at login\n";
    }
    {
      std::ofstream out(userDir / "personal.desktop");
      out << "[Desktop Entry]\nType=Application\nName=Personal\n"
             "Exec=/usr/bin/personal\n";
    }

    auto entries = sysograph::scanAutostart(userDir, {systemDir});
    CHECK(entries.size() == 2);
    if (entries.size() == 2) {
      // Sorted by name: "Personal" < "System Tool".
      CHECK_EQ_STR(entries[0].name, "Personal");
      CHECK_EQ_STR(entries[0].origin, "user");
      CHECK(entries[0].enabled);
      CHECK_EQ_STR(entries[1].name, "System Tool");
      CHECK_EQ_STR(entries[1].origin, "system+override");
      CHECK(entries[1].enabled);
      CHECK_EQ_STR(entries[1].exec, "/usr/bin/systemtool");

      // Disable the system entry: an override file must appear in userDir,
      // so from now on the entry is read from the user directory.
      std::string error;
      CHECK(sysograph::setAutostartEnabled(entries[1], false, &error));
      entries = sysograph::scanAutostart(userDir, {systemDir});
      CHECK(entries.size() == 2);
      if (entries.size() == 2) {
        CHECK(!entries[1].enabled);  // "System Tool" now disabled
        CHECK_EQ_STR(entries[1].origin, "user");
        CHECK_EQ_STR(entries[1].exec, "/usr/bin/systemtool");
        CHECK(std::filesystem::exists(userDir / "system.desktop"));
      }
      // Re-enable.
      CHECK(sysograph::setAutostartEnabled(entries[1], true, &error));
      entries = sysograph::scanAutostart(userDir, {systemDir});
      CHECK(entries.size() == 2);
      if (entries.size() == 2) CHECK(entries[1].enabled);
    }

    std::filesystem::remove_all(base, ec);
  }
}
