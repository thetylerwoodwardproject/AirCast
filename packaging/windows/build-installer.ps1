# Builds AirCast's Windows installer from an already-built Release VST3.
# Parses the version from CMakeLists.txt (single source of truth - never
# hand-edit AirCast.iss's version per release) and invokes the Inno Setup
# compiler (ISCC.exe) with it.
#
# Requires: Inno Setup 6 installed (https://jrsoftware.org/isinfo.php).
# Requires: a Release build already done - see INSTALL.md. This script does
# NOT build the plugin itself.
#
# NOT verified by execution - authored to spec against Inno Setup's
# documented CLI/preprocessor behavior. No Windows machine was available to
# actually run this. Review carefully before relying on it for a real release.

param(
    [string]$BuildDir = "..\..\build\AirCast_artefacts\Release",
    [string]$IsccPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$cmakeListsPath = Join-Path $ScriptDir "..\..\CMakeLists.txt"
if (-not (Test-Path $cmakeListsPath)) {
    throw "CMakeLists.txt not found at $cmakeListsPath"
}

$content = Get-Content -Raw $cmakeListsPath
$match = [regex]::Match($content, 'project\(AirCast VERSION ([0-9]+\.[0-9]+\.[0-9]+)')
if (-not $match.Success) {
    throw "Could not parse a version out of project(AirCast VERSION x.y.z) in CMakeLists.txt"
}
$version = $match.Groups[1].Value
Write-Host "Packaging AirCast $version..."

$vst3Path = Join-Path $BuildDir "VST3\AirCast.vst3"
if (-not (Test-Path $vst3Path)) {
    throw "Couldn't find a built VST3 at $vst3Path. Build a Release configuration first (see INSTALL.md), or pass -BuildDir."
}

if (-not (Test-Path $IsccPath)) {
    throw "ISCC.exe not found at $IsccPath. Install Inno Setup 6 (https://jrsoftware.org/isinfo.php), or pass -IsccPath."
}

New-Item -ItemType Directory -Force -Path (Join-Path $ScriptDir "dist") | Out-Null

& $IsccPath "/DMyAppVersion=$version" "/DSourceArtefacts=$BuildDir" "AirCast.iss"

if ($LASTEXITCODE -ne 0) {
    throw "ISCC.exe exited with code $LASTEXITCODE"
}

Write-Host ""
Write-Host "Built: dist\AirCast-$version-Windows-Setup.exe"
Write-Host ""
Write-Host "NOTE: this installer is not signed (no Authenticode certificate configured)."
Write-Host "End users will see a SmartScreen warning and need to click 'More info' -> 'Run anyway'."
