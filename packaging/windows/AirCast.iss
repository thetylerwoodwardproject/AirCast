; AirCast Windows installer (Inno Setup 6). Packages an already-built Release
; VST3 into the standard system VST3 folder. Does NOT build the plugin - build
; a Release configuration first (see INSTALL.md), then compile this with ISCC,
; normally via build-installer.ps1 so the version gets threaded in from
; CMakeLists.txt automatically.
;
; NOT signed - no Authenticode certificate is configured. Windows SmartScreen
; will show "Windows protected your PC" for end users; they need to click
; "More info" -> "Run anyway". See PACKAGING.md.

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0-dev"
#endif
#ifndef SourceArtefacts
  #define SourceArtefacts "..\..\build\AirCast_artefacts\Release"
#endif

#define MyAppName "AirCast"
#define MyAppPublisher "Fully Modulated"
; Static GUID for this installer's AppId - generate ONCE, keep forever. Inno
; Setup uses this (not the app name) to detect upgrades vs. fresh installs.
; Placeholder below - replace before any real release and never change again.
#define MyAppId "{{B37C7B7B-4F0C-4B0B-9B0F-000000000000}}"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commoncf}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=no
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=dist
OutputBaseFilename=AirCast-{#MyAppVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={commoncf}\VST3\AirCast.vst3
#ifexist "..\..\LICENSE"
LicenseFile=..\..\LICENSE
#endif

[Files]
Source: "{#SourceArtefacts}\VST3\AirCast.vst3\*"; DestDir: "{commoncf}\VST3\AirCast.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\AirCast.vst3"
