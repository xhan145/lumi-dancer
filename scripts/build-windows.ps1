# LUMI//DANCER - Windows build script.
# Validates prerequisites, configures CMake, builds Debug + Release,
# reports artefact paths. Returns nonzero on failure.
[CmdletBinding()]
param(
    [switch]$SkipDebug,
    [switch]$SkipRelease
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

Write-Host "== LUMI//DANCER Windows build ==" -ForegroundColor Cyan

# ---------------------------------------------------------------- prerequisites
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Error "cmake not found on PATH. Install CMake >= 3.25."
    exit 1
}
$cmakeVersion = (cmake --version | Select-Object -First 1)
Write-Host "Using $cmakeVersion"

# ------------------------------------------------------------------- configure
Write-Host "`n-- Configure (preset: windows)" -ForegroundColor Cyan
cmake --preset windows
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed."; exit 1 }

# ----------------------------------------------------------------------- build
if (-not $SkipDebug) {
    Write-Host "`n-- Build Debug" -ForegroundColor Cyan
    cmake --build --preset windows-debug
    if ($LASTEXITCODE -ne 0) { Write-Error "Debug build failed."; exit 1 }
}

if (-not $SkipRelease) {
    Write-Host "`n-- Build Release" -ForegroundColor Cyan
    cmake --build --preset windows-release
    if ($LASTEXITCODE -ne 0) { Write-Error "Release build failed."; exit 1 }
}

# --------------------------------------------------------------------- summary
Write-Host "`n== Build outputs ==" -ForegroundColor Green
$artefacts = Join-Path $repoRoot "build\windows\LumiDancer_artefacts"
foreach ($config in @('Debug', 'Release')) {
    $vst3 = Join-Path $artefacts "$config\VST3\LUMI DANCER.vst3"
    $standalone = Join-Path $artefacts "$config\Standalone\LUMI DANCER.exe"
    if (Test-Path $vst3)       { Write-Host "  [$config] VST3:       $vst3" }
    if (Test-Path $standalone) { Write-Host "  [$config] Standalone: $standalone" }
}
Write-Host "`nBuild OK."
exit 0
