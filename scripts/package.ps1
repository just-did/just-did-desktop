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

$cmake = "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
$windeployqt = "D:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe"
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
