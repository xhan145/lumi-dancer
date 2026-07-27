# LUMI//DANCER - test runner.
# Builds test targets where required, runs the JUCE-free core suite and the
# JUCE-linked suite, prints a summary, returns nonzero on any failure.
[CmdletBinding()]
param(
    [switch]$CoreOnly
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

Write-Host "== LUMI//DANCER tests ==" -ForegroundColor Cyan

# ------------------------------------------------------- core (JUCE-free) suite
Write-Host "`n-- Core suite (JUCE-free)" -ForegroundColor Cyan
cmake --preset core
if ($LASTEXITCODE -ne 0) { Write-Error "Core configure failed."; exit 1 }
cmake --build --preset core
if ($LASTEXITCODE -ne 0) { Write-Error "Core build failed."; exit 1 }
ctest --preset core
$coreResult = $LASTEXITCODE

$juceResult = 0
if (-not $CoreOnly) {
    Write-Host "`n-- JUCE-linked suite" -ForegroundColor Cyan
    cmake --preset windows
    if ($LASTEXITCODE -ne 0) { Write-Error "Windows configure failed."; exit 1 }
    cmake --build --preset windows-release --target LumiTests lumi_core_tests
    if ($LASTEXITCODE -ne 0) { Write-Error "Test build failed."; exit 1 }
    ctest --preset windows-tests
    $juceResult = $LASTEXITCODE
}

# --------------------------------------------------------------------- summary
Write-Host "`n== Test summary ==" -ForegroundColor Green
Write-Host ("  Core suite: " + ($(if ($coreResult -eq 0) { "PASS" } else { "FAIL" })))
if (-not $CoreOnly) {
    Write-Host ("  JUCE suite: " + ($(if ($juceResult -eq 0) { "PASS" } else { "FAIL" })))
}

if ($coreResult -ne 0 -or $juceResult -ne 0) { exit 1 }
exit 0
