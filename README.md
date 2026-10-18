# Sysograph

**A free, original system task manager for Linux.**

[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)
[Русская версия документа](README.ru.md)

Sysograph is a native Linux application — written from scratch in modern
C++ with GTK 4 — that gives you the full picture of what your machine is
doing: processes, performance, users, services and autostart entries.

* **100 % free software.** The whole project is licensed under
  GPL-3.0-or-later; it links only to other free libraries (GTK 4 / GLib,
  LGPL-2.1+). See [NOTICE](NOTICE) and
  [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
* **Original work.** No code, artwork, icons, text or other material from
  any proprietary vendor (including Microsoft) was copied, extracted or
  imitated. The name, logo and interface are the project's own design.
  Sysograph is not affiliated with any company or product.
* **No root, no network, no data collection.** It reads the standard,
  publicly documented `/proc` and `/sys` interfaces and the XDG autostart
  conventions, and that's it.

## Features

| Tab | What you get |
|-----|--------------|
| **Processes** | Live table of every process: name, PID, user, CPU %, memory, state, threads, priority, command line. Search-as-you-type, click-to-sort columns, an details pane (executable path, working directory, open file count, start time, CPU time) and safe process termination (SIGTERM) with a force-kill (SIGKILL) option. |
| **Process tree** | The same data as a hierarchy: who spawned whom, with expandable nodes whose state survives refreshes. |
| **Performance** | Real-time graphs for total and per-core CPU, memory and swap, disk throughput and network throughput, plus uptime, running/blocked process counts and root filesystem usage. |
| **Users** | Resource usage aggregated per user account: process count, CPU %, memory, threads. |
| **Startup** | XDG autostart entries with their origin (user / system) and status, and one-click enable/disable implemented via standard user-level `.desktop` overrides — no root needed. |
| **Services** | Read-only listing of systemd service units (state, sub-state, description), refreshed through `systemctl` invoked directly, never through a shell. |

Quality details that matter:

* Every procfs parser tolerates malformed, truncated or vanished entries —
  a dying process mid-read can never crash the app.
* Process CPU % is correctly normalised against total machine capacity
  (all cores), matching the semantics people expect.
* History for graphs is kept in fixed-size ring buffers: memory usage does
  not grow over days of running.
* PID-reuse races are mitigated: the kill dialog always shows the process
  name captured at request time.
* The refresh interval (1/2/5 s) is selectable from the header menu.
* Interface is available in English and Russian; the language follows your
  locale (`SYSSOGRAPH_LANG=en|ru` to override).

## Building from source

Requirements: a C++20 compiler, CMake ≥ 3.16, pkg-config and GTK 4
development packages.

```sh
# Debian / Ubuntu
sudo apt install build-essential cmake pkg-config libgtk-4-dev

# Fedora / RHEL
sudo dnf install gcc-c++ cmake pkgconf-pkg-config gtk4-devel

git clone <your-repo-url> sysograph && cd sysograph
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/sysograph
```

More details (including debug builds and running tests) are in
[BUILDING.md](BUILDING.md).

## Installation

```sh
cmake --install build            # classic install
```

or install the distribution packages produced by CPack:

```sh
cd build && cpack
# -> sysograph-0.1.0-Linux.deb   (Debian, Ubuntu and derivatives)
# -> sysograph-0.1.0-Linux.rpm   (Fedora, RHEL and derivatives)
```

For AppImage instructions see [packaging/appimage/README.md](packaging/appimage/README.md).

## Legal notes

* **License:** GPL-3.0-or-later ([LICENSE](LICENSE)).
* **Trademarks / non-affiliation:** Sysograph is an independent project.
  It is not a copy of any other task manager; comparable functionality is
  achieved through an original implementation of publicly documented
  interfaces. No proprietary material is included, linked or bundled.
* **Dependencies:** GTK 4 and GLib (LGPL-2.1-or-later, dynamically linked),
  which is fully compatible with the GPL-3.0+ licensing of this project.
  See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for the complete
  compliance overview and verification commands.

This repository ships a compliance check
([scripts/check_licenses.sh](scripts/check_licenses.sh)) that fails the
build if the license files are missing or if forbidden third-party marks
creep into code, data or packaging.

Nothing here is legal advice; for distribution decisions beyond the
permissive defaults of these licenses, consult a lawyer.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). All contributions are accepted
under GPL-3.0-or-later.
