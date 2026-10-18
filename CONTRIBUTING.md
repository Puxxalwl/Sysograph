# Contributing to Sysograph

Thank you for your interest! Contributions are welcome under the terms
below.

## License

By contributing you agree that your contributions will be licensed under
GPL-3.0-or-later, like the rest of the project. Do not submit code you
do not have the right to license that way.

## Originality rules (important)

To keep the project legally clean, every contribution must follow these
rules:

1. **Write your own code.** Do not copy code, comments, algorithms
   verbatim, artwork, icons, strings, layouts or any other material from
   proprietary products (including any operating system vendor's task
   manager) or from projects with incompatible licenses.
2. **Do not decompile or disassemble** any proprietary software as part
   of working on this project.
3. Use only public documentation (man pages, kernel documentation, XDG
   and freedesktop.org specifications, GTK/GLib documentation).
4. New runtime dependencies must be free software (GPL/LGPL/MIT/BSD-like
   compatible) and must be recorded in THIRD_PARTY_LICENSES.md.
5. Run `scripts/check_licenses.sh` before submitting; CI enforces it.

## Development workflow

* C++20; the core (`src/core`, `src/platform`) stays GUI-free and
  unit-tested — put new parsing/calculation logic there, not in widgets.
* Keep `-Wall -Wextra -Wpedantic` clean; UI sources may suppress
  deprecation warnings for the transitional widget set.
* Add or extend tests under `tests/` for every core change.
* Verify the full build and test suite:

```sh
cmake --build build -j$(nproc) && (cd build && ctest --output-on-failure)
```

## Commit style

Short, imperative subject line; body explains *why* when non-obvious.
Reference issues where applicable.

## Reporting bugs

Include: distribution, GTK version, how to reproduce, and — for crashes —
a backtrace (`coredumpctl gdb` / `gdb ./build/sysograph`). Never include
sensitive data (command lines may contain secrets).
