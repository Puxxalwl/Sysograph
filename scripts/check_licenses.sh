#!/usr/bin/env bash
# Sysograph — a free, original Linux system task manager.
# Copyright (C) 2025 The Sysograph Project Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# License-compliance gate for CI and local use. Exits non-zero when:
#   * required license documents are missing or empty;
#   * the vendored LGPL text is gone;
#   * third-party product/company marks appear in code, data or packaging.
#
# Notes on the word lists: the checked words are trademarks of their
# respective owners; mentioning them in marketing or in this very script
# would be fine nominatively, but shipping them inside the product's own
# code, artwork or packaging invites confusion, so we forbid them there.

set -u

fail() {
  echo "LICENSE-CHECK FAILED: $*" >&2
  exit 1
}

[ -s LICENSE ]                 || fail "LICENSE missing or empty"
[ -s NOTICE ]                 || fail "NOTICE missing or empty"
[ -s THIRD_PARTY_LICENSES.md ] || fail "THIRD_PARTY_LICENSES.md missing or empty"
[ -s THIRD_PARTY/LICENSES/LGPL-2.1.txt ] || fail "vendored LGPL-2.1 text missing"
[ -s data/style.css ]          || fail "stylesheet missing"
[ -s data/icons/org.sysograph.Sysograph.svg ] || fail "application icon missing"

# The icon must carry our own copyright/SPDX header.
grep -q "SPDX-License-Identifier" data/icons/org.sysograph.Sysograph.svg \
  || fail "icon misses SPDX header"

# Forbidden marks in product code, artwork and packaging.
# Deliberately excluded: docs (this script and legal disclaimers live there).
if grep -rniE 'microsoft|windows task manager|taskmgr' src data packaging >/dev/null 2>&1; then
  grep -rniE 'microsoft|windows task manager|taskmgr' src data packaging >&2 || true
  fail "third-party product marks found in src/, data/ or packaging/"
fi

# Every source file must carry the license header.
missing_header=$(grep -rL 'SPDX-License-Identifier: GPL-3.0-or-later' \
  src --include='*.cpp' --include='*.h' 2>/dev/null | head -5 || true)
if [ -n "$missing_header" ]; then
  echo "Files without SPDX header:" >&2
  echo "$missing_header" >&2
  fail "source files missing SPDX license headers"
fi

echo "LICENSE-CHECK OK"
exit 0
