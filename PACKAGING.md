# AirCast — Packaging Guide (maintainer-facing)

This is different from [INSTALL.md](INSTALL.md): that document is for someone building AirCast
from source. This one is for the maintainer cutting a distributable installer from a **Release
build that already exists** — the thing you'd hand to someone with no compiler, no CMake, no
JUCE.

## 1. Prerequisites per OS

- **macOS**: nothing beyond Xcode Command Line Tools (`xcode-select --install`) — `pkgbuild`,
  `productbuild`, and `plutil` all ship with it.
- **Windows**: [Inno Setup 6](https://jrsoftware.org/isinfo.php) installed, so `ISCC.exe`
  exists (default path `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`).
- **Linux**: nothing beyond `tar` and standard coreutils.

## 2. Version source of truth

The version lives in exactly one place: `CMakeLists.txt`'s `project(AirCast VERSION x.y.z)`
line. Every packaging script derives the version from there
(`packaging/common/get-version.sh` on macOS/Linux, a regex in `build-installer.ps1` on
Windows). Never hand-edit a version number anywhere else — bump it once, in `CMakeLists.txt`,
and every packaging script picks it up automatically.

## 3. Directory layout

```
packaging/
├── common/
│   └── get-version.sh       # echoes the current version, e.g. "0.1.0"
├── macos/
│   ├── build-pkg.sh         # run this
│   ├── distribution.xml.in  # template consumed by build-pkg.sh - don't run directly
│   ├── Resources/           # created by build-pkg.sh if a LICENSE exists
│   ├── .build/               (scratch, recreated each run)
│   └── dist/                 (output: AirCast-<version>-macOS.pkg)
├── windows/
│   ├── AirCast.iss          # Inno Setup script - compiled by build-installer.ps1, not by hand
│   ├── build-installer.ps1  # run this
│   └── dist/                  (output: AirCast-<version>-Windows-Setup.exe)
└── linux/
    ├── install.sh           # end-user-facing - ships INSIDE the tarball, don't run this one directly as a maintainer
    ├── make-tarball.sh      # run this
    ├── .build/                (scratch, recreated each run)
    └── dist/                  (output: AirCast-<version>-Linux.tar.gz)
```

## 4. Commands, per OS

All three assume a completed **Release** build already exists (see `INSTALL.md` — the
packaging scripts do not build the plugin themselves, only package what's already built).

### macOS

```bash
cmake --build build-release --target AirCast_VST3 -j 8
cmake --build build-release --target AirCast_AU -j 8
./packaging/macos/build-pkg.sh
```
Produces `packaging/macos/dist/AirCast-<version>-macOS.pkg`. Override the artefact location
with `--build-dir <path>` if you're not using the default `build-release/` folder.

### Windows

```powershell
cmake --build build --config Release --target AirCast_VST3
cd packaging\windows
.\build-installer.ps1
```
Produces `packaging\windows\dist\AirCast-<version>-Windows-Setup.exe`. Override the artefact
location with `-BuildDir <path>` and the compiler path with `-IsccPath <path>` if needed.

### Linux

```bash
cmake --build build --target AirCast_VST3 -j $(nproc)
./packaging/linux/make-tarball.sh
```
Produces `packaging/linux/dist/AirCast-<version>-Linux.tar.gz`, containing `AirCast.vst3/` and
`install.sh`. The end user extracts it and runs `./install.sh` (see `INSTALL.md` for what that
does).

## 5. Output naming convention

`AirCast-<version>-<OS>.<ext>` — `AirCast-0.1.0-macOS.pkg`, `AirCast-0.1.0-Windows-Setup.exe`,
`AirCast-0.1.0-Linux.tar.gz`.

## 6. Signing / notarization status

**None of these are signed.** There's no Apple Developer ID Installer certificate and no
Windows Authenticode certificate configured anywhere in this project. That's a deliberate,
stated limitation, not an oversight to quietly work around:

- **macOS**: the `.pkg` is unsigned. `pkgutil --check-signature` on it reports "no signature."
  End users need to right-click → Open, or clear the quarantine flag
  (`xattr -dr com.apple.quarantine <path>`), or approve it in
  System Settings → Privacy & Security.
- **Windows**: the installer is unsigned. Windows SmartScreen will show "Windows protected
  your PC" — users click "More info" → "Run anyway."
- **Linux**: no gatekeeping mechanism applies to shell scripts, so nothing extra needed there.

## 7. Known limitations / explicitly out of scope

- **No `.deb`/`.rpm`.** A `.deb` only serves Debian/Ubuntu users; matching it with an `.rpm`
  for parity would double the packaging surface for a single-maintainer 0.1.0 project with no
  CI to catch drift between a `postinst` script and `install.sh`'s logic. The tarball +
  `install.sh` is the only Linux distribution mechanism for now. Revisit if actual Linux users
  ask for a distro package specifically.
- **No CI-driven builds.** All three packaging scripts are run by hand, on a machine that
  already has the relevant Release build. There's no automated pipeline producing these on
  tag/release yet.
- **No auto-update mechanism.** AirCast doesn't check for or install updates itself; a new
  version means re-downloading and re-running the installer.
- **No built-in uninstaller for the macOS `.pkg`.** Apple doesn't provide one for flat
  packages — this is expected macOS behavior, not something missing from this setup. To
  remove AirCast on macOS, manually delete `AirCast.vst3`/`AirCast.component` from the
  Plug-Ins folders listed in `INSTALL.md`.
- **Windows and Linux packaging scripts are unverified by execution.** They were written
  correctly to spec (Inno Setup's documented CLI/preprocessor behavior; POSIX shell), but this
  development machine is macOS-only — there's no Windows box to run `ISCC.exe` on and no Linux
  box to build a real Linux VST3 on. `install.sh`'s logic (argument parsing, root/non-root
  guards, missing-bundle detection) was syntax-checked and dry-run tested on macOS against a
  fake bundle, which validates the script's control flow but not real `/usr/lib/vst3` or true
  root behavior. Treat both as "review before a real release," same honesty bar as
  `INSTALL.md`'s existing Windows/Linux build sections.

## 8. Release checklist

1. Bump the version in `CMakeLists.txt`'s `project(AirCast VERSION x.y.z)` line.
2. Rebuild a Release configuration on each platform you have access to (see `INSTALL.md`).
3. Run that platform's packaging script (§4 above).
4. Smoke-test the installer: on macOS, at minimum run
   `installer -pkg <pkg> -target CurrentUserHomeDirectory` and confirm
   `~/Library/Audio/Plug-Ins/VST3/AirCast.vst3` and
   `~/Library/Audio/Plug-Ins/Components/AirCast.component` both land correctly; on
   Windows/Linux, actually run the installer/`install.sh` on a real machine of that OS before
   shipping — they have not been execution-tested in this repo yet.
5. Attach the resulting `dist/` artifacts to the release.
