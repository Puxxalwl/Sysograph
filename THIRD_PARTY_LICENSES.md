Third-Party Licenses
====================

Sysograph is an independent, original work licensed under
GPL-3.0-or-later (see LICENSE).  It deliberately keeps the runtime
dependency footprint minimal so that license compliance stays
simple and verifiable.

Runtime / build dependencies
----------------------------

| Component              | Used for                              | License                  | Notes |
|------------------------|---------------------------------------|--------------------------|-------|
| GTK 4                  | User interface toolkit                | LGPL-2.1-or-later        | Dynamic linking only (see below) |
| GLib / GIO / GObject   | Core runtime (bundled with GTK)       | LGPL-2.1-or-later        | Dynamic linking only |
| Cairo / Pango / GDK-Pixbuf | Text, vector drawing (pulled in by GTK) | LGPL-2.1-or-later / MIT | Indirect dependencies of GTK |
| C++ standard library (libstdc++) | Language runtime          | GPL-3.0-or-later with GCC Runtime Library Exception | |

Optional, at runtime only (never linked, never bundled):

| Component              | Used for                              | License                  |
|------------------------|---------------------------------------|--------------------------|
| systemd (systemctl)    | Listing services on the "Services" tab | LGPL-2.1-or-later (systemd) — invoked as a separate process |
| XDG autostart .desktop files | "Startup" tab reads standard XDG files | Per-entry; treated as data, not code |

Full license texts
------------------
* GNU GPL 3.0 (this project):              [LICENSE](LICENSE)
* GNU LGPL 2.1 (GTK/GLib and friends):      [THIRD_PARTY/LICENSES/LGPL-2.1.txt](THIRD_PARTY/LICENSES/LGPL-2.1.txt)
* GPL-3.0 text (for comparison / GCC runtime): https://www.gnu.org/licenses/gpl-3.0.txt
* GCC Runtime Library Exception:            https://www.gnu.org/licenses/gcc-exception-3.1.html

Why this is license-safe
------------------------
* Sysograph is *dynamically linked* against GTK/GLib.  The LGPL-2.1
  explicitly permits this for GPL-3.0+ programs; the combination may
  be distributed under GPL-3.0+.
* No proprietary or non-free component is linked, bundled or invoked.
* No telemetry, analytics, or network service is used; the app is
  fully offline.
* The license compliance check script (scripts/check_licenses.sh)
  fails the build if this file, LICENSE, or NOTICE are missing or if
  protected third-party marks appear in code, data or packaging.

Verification
------------
On Debian/Ubuntu you can confirm the licenses of every linked
library of the installed binary with:

    dpkg-query -S $(ldd /usr/bin/sysograph | awk '/=> \//{print $3}') | cut -d: -f1 | sort -u
    apt-cache show <package> | grep -E '^(Package|License)'

This file is informational; the authoritative texts are the license
documents referenced above.
