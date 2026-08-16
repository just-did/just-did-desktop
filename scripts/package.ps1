# One-click packaging: build -> windeployqt -> staging -> dist/JustDid_v<Version>_portable.zip
# Usage: powershell -ExecutionPolicy Bypass -File scripts/package.ps1 [-Version 1.0.0]
# NOTE: keep this file ASCII-only. Windows PowerShell 5.1 reads BOM-less files as ANSI;
#       non-ASCII comments can swallow newlines and silently skip code blocks.
param([string]$Version = "1.0.0")

$ErrorActionPreference = "Stop"

$repo = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repo "build"
$releaseDir = Join-Path $buildDir "src\Release"
$exe = Join-Path $releaseDir "JustDid.exe"
$distDir = Join-Path $repo "dist"
$staging = Join-Path $distDir "JustDid_v$Version"

# --- Tool discovery ---------------------------------------------------------
# Priority: env var override (JUSTDID_CMAKE / JUSTDID_WINDEPLOYQT) > auto probe

function Find-CMakeExecutable {
    $candidates = @()
    if ($env:JUSTDID_CMAKE) { $candidates += $env:JUSTDID_CMAKE }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -find "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
        foreach ($f in $found) { if (Test-Path $f) { $candidates += $f } }
    }

    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { $candidates += $cmd.Source }

    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    throw "cmake not found. Set env var JUSTDID_CMAKE, install the VS2022 CMake component, or add cmake to PATH."
}

function Find-WindeployqtExecutable {
    $candidates = @()
    if ($env:JUSTDID_WINDEPLOYQT) { $candidates += $env:JUSTDID_WINDEPLOYQT }

    # Qt-installed windeployqt first: PATH may expose a mismatched Qt
    # (e.g. Anaconda's), which cannot deploy this project's msvc2022_64 build.
    foreach ($root in @("C:/Qt", "D:/Qt", "C:/Qt6", "D:/Qt6")) {
        if (-not (Test-Path $root)) { continue }
        $kits = Get-ChildItem $root -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match "^6\.\d+(\.\d+)*$" } |
            Sort-Object { [version] $_.Name } -Descending
        foreach ($kit in $kits) {
            $exe = Join-Path $kit.FullName "msvc2022_64/bin/windeployqt.exe"
            if (Test-Path $exe) { $candidates += $exe }
        }
    }

    $cmd = Get-Command windeployqt -ErrorAction SilentlyContinue
    if ($cmd) { $candidates += $cmd.Source }

    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    throw "windeployqt not found. Set env var JUSTDID_WINDEPLOYQT or install a Qt 6 msvc2022_64 kit under C:/Qt or D:/Qt."
}

$cmake = Find-CMakeExecutable
$windeployqt = Find-WindeployqtExecutable
$qmldir = Join-Path $repo "src\ui\qml"

if (-not (Test-Path $exe)) {
    throw "JustDid.exe not found ($exe), run CMake configure first"
}

Write-Host "== [1/4] Build =="
& $cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed (close the app if the exe is locked)" }

Write-Host "== [2/4] windeployqt =="
& $windeployqt --release --no-translations --qmldir $qmldir $exe
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

Write-Host "== [3/4] Stage: $staging =="
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging | Out-Null
Copy-Item (Join-Path $releaseDir "*") -Destination $staging -Recurse

# Exclude developer runtime data (privacy data and local config) and debug noise
$junk = @("config.yml", "just_do.db", "qt_debug.log", "qr_debug.png", "data", "logs", "qmltooling")
foreach ($name in $junk) {
    $path = Join-Path $staging $name
    if (Test-Path $path) { Remove-Item $path -Recurse -Force }
}

Write-Host "== [4/4] Compress =="
$zip = Join-Path $distDir "JustDid_v${Version}_portable.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $staging -DestinationPath $zip

Write-Host "Done: $zip"
