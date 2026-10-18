# Building Sysograph

## Requirements

| Tool | Version |
|------|---------|
| C++ compiler (GCC or Clang) | C++20 (GCC ≥ 10, Clang ≥ 12) |
| CMake | ≥ 3.16 |
| pkg-config / pkgconf | any |
| GTK 4 development package | ≥ 4.6 (`libgtk-4-dev` on Debian/Ubuntu, `gtk4-devel` on Fedora) |

No other libraries are required: networking, telemetry and heavy
frameworks are deliberately absent.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Run from the build tree (CSS and icon fall back to the source tree):

```sh
./build/sysograph
```

## Tests

The core (procfs parsing, rate math, tree building, user aggregation,
desktop-entry handling, service-list parsing, formatting) is covered by
unit tests that run without a display — ideal for CI:

```sh
cd build && ctest --output-on-failure
```

## Debug builds and sanitizers

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j$(nproc)

# Optional: sanitizers via the environment
CXXFLAGS="-fsanitize=address,undefined -g" cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan -j$(nproc)
./build-asan/sysograph_tests
```

## Install

```sh
cmake --install build            # honors CMAKE_INSTALL_PREFIX
```

Default layout: `bin/sysograph`, `share/applications`,
`share/metainfo`, `share/icons/hicolor/scalable/apps`,
`share/sysograph/style.css`, `share/doc/sysograph` (LICENSE, NOTICE,
third-party texts).

## Packages

```sh
cd build
cpack -G DEB    # .deb for Debian/Ubuntu
cpack -G RPM    # .rpm for Fedora/RHEL
cpack           # both
```

Runtime dependencies are declared per format in
`packaging/cpack-config.cmake` (`libgtk-4-1` for DEB, `gtk4` for RPM).
The RPM generator requires `rpmbuild` to be installed (present on
Fedora/RHEL by default; on Arch install it with `sudo pacman -S rpm`).

Verify a package's contents before publishing:

```sh
dpkg-deb -c sysograph-*-Linux.deb
rpm -qlp sysograph-*-Linux.rpm
```

## AppImage

See [packaging/appimage/README.md](packaging/appimage/README.md).

## License compliance gate

Run the repository self-check (also part of CI):

```sh
scripts/check_licenses.sh
```

It verifies that LICENSE / NOTICE / THIRD_PARTY_LICENSES.md exist, that
the LGPL text is vendored, and that no third-party product names appear
in `src/`, `data/` or `packaging/`.
