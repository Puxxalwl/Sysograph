# Building a Sysograph AppImage

The repository does not bundle any third-party binaries, and the AppImage
is produced locally with free tools only. Because GTK 4 is dynamically
linked in normal packages, the AppImage bundles GTK 4 and its free
dependencies (all LGPL-2.1+/MIT) inside the image — keep that in mind for
the compliance notes below.

## One-time setup

Install the standard AppImage tooling (all free software):

```sh
# download linuxdeploy and its GTK plugin (check their releases for URLs)
wget https://github.com/linuxdeploy/linuxdeploy/releases/latest/download/linuxdeploy-x86_64.Appimage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-gtk/raw/master/linuxdeploy-plugin-gtk.sh
chmod +x linuxdeploy-x86_64.Appimage linuxdeploy-plugin-gtk.sh
```

## Build & package

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
DESTDIR=AppDir cmake --install build

OUTPUT=Sysograph-0.1.0-x86_64.Appimage \
  ./linuxdeploy-x86_64.Appimage \
  --appdir AppDir \
  --desktop-file packaging/org.sysograph.Sysograph.desktop \
  --icon-file data/icons/org.sysograph.Sysograph.svg \
  --plugin gtk \
  --output appimage
```

## Compliance checklist for published AppImages

1. **Ship the sources.** GPL-3.0 section 6 requires offering the
   corresponding source; attach the release tarball or a link that stays
   valid for at least three years.
2. **Ship the licenses.** The AppDir must contain `LICENSE`,
   `NOTICE` and `THIRD_PARTY_LICENSES.md` (the install step already puts
   them into `/usr/share/doc/sysograph`). Verify:
   `find AppDir -name 'LICENSE*' -o -name 'NOTICE'`.
3. **Audit bundled libraries.** Every `.so` inside `AppDir/usr/lib`
   must be free software; list them and confirm their licenses:
   `find AppDir/usr/lib -name '*.so*'`.
4. **No proprietary components** may end up in the image (e.g. firmware
   blobs, codec packs, vendor telemetry).
5. Re-run `scripts/check_licenses.sh` before publishing.
