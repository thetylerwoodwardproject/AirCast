# AirCast — Install Guide (Windows / macOS / Linux)

AirCast doesn't currently ship pre-built installers — there's no CI or packaging step in this
repo yet, only a CMake project. Building from source is the way to get it running on any
platform. **macOS is the only platform this project has actually been built and tested on so
far** (see `build/` and `build-release/` in the repo). The Windows and Linux steps below are
the standard JUCE + CMake procedure and should work as-is, but treat them as unverified for
this specific project until someone's actually run them here — if something doesn't match,
that's more likely a Windows/Linux quirk nobody's hit yet than a wrong instruction.

If you already have a built `AirCast.vst3` / `AirCast.component` / standalone app (someone
sent you one, or you built it earlier), skip straight to §5 for where to put it.

## 1. What you're building

`CMakeLists.txt` produces up to three plugin formats plus a diagnostic console app:

- **VST3** — all three platforms
- **AU** (Audio Unit) — **macOS only**, automatically skipped on Windows/Linux
- **Standalone** — a plain app, no DAW required, all three platforms
- `DspProbe` — a console-only diagnostic tool, not a plugin, not required for normal use

JUCE is *not* vendored in the repo — `CMakeLists.txt` pulls JUCE 8.0.6 automatically via
`FetchContent` the first time you configure the project, so you don't need to install JUCE
separately.

## 2. Prerequisites (all platforms)

- **CMake 3.22 or newer**
- **Git** (CMake's `FetchContent` uses it to clone JUCE)
- **A C++17 compiler** — see per-platform notes below
- Internet access for the first configure step (downloads JUCE, ~1-2 GB with its examples/modules)

## 3. Build steps

### macOS

1. Install Xcode or at least the Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. From the `AirCast/` directory:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --target AirCast_VST3 -j 8
   ```
   For a release build, use a separate build directory (matches the `build-release/` folder
   already in this repo):
   ```bash
   cmake -B build-release -DCMAKE_BUILD_TYPE=Release
   cmake --build build-release --target AirCast_VST3 -j 8
   ```
3. To also get the AU and Standalone formats, build those targets too (or just build the
   default `all` target, which builds everything):
   ```bash
   cmake --build build --target AirCast_AU -j 8
   cmake --build build --target AirCast_Standalone -j 8
   ```
4. `COPY_PLUGIN_AFTER_BUILD TRUE` in `CMakeLists.txt` means the build **automatically copies**
   the built plugin(s) into your user plugin folders — no manual copy step needed on macOS.
   You should see `-- Installing: ~/Library/Audio/Plug-Ins/VST3/AirCast.vst3` etc. in the
   build output. Minimum deployment target is macOS 11.0 (set in `CMakeLists.txt`).

macOS will ad-hoc-sign the plugin at build time (you'll see "replacing invalid signature
with ad-hoc signature" in the build log — that's expected and harmless for local use). If a
DAW refuses to load it as being from an "unidentified developer," see §6.

### Windows

1. Install **Visual Studio 2022** (Community edition is fine) with the "Desktop development
   with C++" workload — this gives you MSVC, which JUCE on Windows expects.
2. Install [CMake](https://cmake.org/download/) and [Git for Windows](https://git-scm.com/download/win) if you don't have them.
3. From a Developer Command Prompt (or PowerShell with Visual Studio's environment loaded),
   in the `AirCast/` directory:
   ```bat
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release --target AirCast_VST3
   ```
4. Like macOS, `COPY_PLUGIN_AFTER_BUILD TRUE` should auto-install the VST3 to the standard
   system location:
   ```
   C:\Program Files\Common Files\VST3\AirCast.vst3
   ```
   This is a system-wide (all-users) folder, so the build may need to run from an elevated
   (Administrator) prompt to write there — if the copy step fails silently, that's the first
   thing to check. If you'd rather not run the build elevated, build normally and manually
   copy the `.vst3` folder from `build\AirCast_artefacts\Release\VST3\AirCast.vst3` to the
   path above yourself.
5. There is no AU target on Windows — `AirCast_AU` won't exist as a build target; only
   `AirCast_VST3` and `AirCast_Standalone`.

### Linux

1. Install a C++17 compiler and the standard JUCE build dependencies. On Debian/Ubuntu:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake git pkg-config \
     libasound2-dev libjack-jackd2-dev \
     libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
     libxinerama-dev libxrandr-dev libxrender-dev \
     libfreetype6-dev libglu1-mesa-dev mesa-common-dev
   ```
   This project sets `JUCE_WEB_BROWSER=0` and `JUCE_USE_CURL=0` in `CMakeLists.txt`, so you
   do **not** need `libwebkit2gtk-dev` or `libcurl4-*-dev` — those are common JUCE Linux
   dependencies this project has deliberately opted out of.
2. From the `AirCast/` directory:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --target AirCast_VST3 -j $(nproc)
   ```
3. `COPY_PLUGIN_AFTER_BUILD TRUE` should install to the standard per-user VST3 location:
   ```
   ~/.vst3/AirCast.vst3
   ```
   For a system-wide install instead, copy the built bundle from
   `build/AirCast_artefacts/Release/VST3/AirCast.vst3` to `/usr/lib/vst3/` (or
   `/usr/local/lib/vst3/`) yourself — JUCE's auto-copy targets the per-user path.
4. There is no AU target on Linux, same as Windows.

## 4. Verifying the install

After building, rescan plugins in your DAW (most have a "rescan"/"reset & rescan" option in
plugin manager preferences) and look for **AirCast** under Fully Modulated / the VST3 or AU
category, depending on your DAW's browser. If it doesn't show up, check the DAW's plugin scan
log for an error — it usually points straight at a missing dependency or a failed load.

You can also just run the **Standalone** build directly without a DAW at all, which is the
fastest way to confirm the build itself works before troubleshooting a specific DAW's plugin
scan.

## 5. Manually installing a plugin someone gave you

If you have a pre-built `.vst3` (a folder, not a single file) or `.component` from someone
else rather than building it yourself, drop it into the matching folder for your OS and
rescan in your DAW:

| Platform | Format | Location |
|---|---|---|
| macOS | VST3 | `~/Library/Audio/Plug-Ins/VST3/` (per-user) or `/Library/Audio/Plug-Ins/VST3/` (all users) |
| macOS | AU | `~/Library/Audio/Plug-Ins/Components/` (per-user) or `/Library/Audio/Plug-Ins/Components/` (all users) |
| Windows | VST3 | `C:\Program Files\Common Files\VST3\` |
| Linux | VST3 | `~/.vst3/` (per-user) or `/usr/lib/vst3/` (all users) |

## 6. Troubleshooting

**macOS: DAW refuses to load it / "AirCast can't be opened because it is from an
unidentified developer."** The build produces an ad-hoc-signed, unnotarized plugin — fine
for local development but Gatekeeper can still be picky depending on your Mac's security
settings. Clear the quarantine flag manually:
```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/AirCast.vst3
```

**Windows: the copy-after-build step fails or the plugin doesn't appear in
`C:\Program Files\Common Files\VST3\`.** That folder needs admin rights to write to. Either
build from an elevated prompt, or copy the built `.vst3` folder there manually.

**Linux: DAW doesn't see it in `~/.vst3/`.** Confirm your specific DAW actually scans the
per-user path (some default to system-wide `/usr/lib/vst3/` only) — check your DAW's plugin
path settings and add `~/.vst3` if it's missing, or copy the bundle to the system path
instead.

**Build fails immediately on `FetchContent_MakeAvailable(JUCE)`.** That step needs network
access to clone JUCE from GitHub the first time — check your connection/proxy, and that `git`
is on your `PATH`.

**Rebuilding after a source change.** You don't need to reconfigure — just re-run the build
command (`cmake --build build --target AirCast_VST3 ...`) and the auto-install/copy step
runs again automatically.
