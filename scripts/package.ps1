# LUMI//DANCER - portable package builder.
# Collects the Release VST3, standalone, docs, presets and licence files
# into dist/LUMI-DANCER-v<version>-win64.zip. Returns nonzero on failure.
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$version = (Select-String -Path "CMakeLists.txt" -Pattern 'VERSION (\d+\.\d+\.\d+)' | Select-Object -First 1).Matches[0].Groups[1].Value
if (-not $version) { Write-Error "Could not read project version from CMakeLists.txt"; exit 1 }

$artefacts  = Join-Path $repoRoot "build\windows\LumiDancer_artefacts\Release"
$vst3       = Join-Path $artefacts "VST3\LUMI DANCER.vst3"
$standalone = Join-Path $artefacts "Standalone\LUMI DANCER.exe"

foreach ($required in @($vst3, $standalone)) {
    if (-not (Test-Path $required)) {
        Write-Error "Missing build artefact: $required  (run scripts/build-windows.ps1 first)"
        exit 1
    }
}

$stage = Join-Path $repoRoot "dist\stage"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null

Copy-Item -Recurse $vst3 (Join-Path $stage "LUMI DANCER.vst3")
New-Item -ItemType Directory -Force -Path (Join-Path $stage "Standalone") | Out-Null
Copy-Item $standalone (Join-Path $stage "Standalone\LUMI DANCER.exe")

foreach ($doc in @("README.md", "LICENSE", "CHANGELOG.md", "THIRD_PARTY_NOTICES.md")) {
    Copy-Item (Join-Path $repoRoot $doc) $stage
}
New-Item -ItemType Directory -Force -Path (Join-Path $stage "docs") | Out-Null
Copy-Item (Join-Path $repoRoot "docs\*.md") (Join-Path $stage "docs")

$zip = Join-Path $repoRoot "dist\LUMI-DANCER-v$version-win64.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip

Remove-Item -Recurse -Force $stage

Write-Host "`n== Package ==" -ForegroundColor Green
Write-Host "  $zip"
Get-Item $zip | ForEach-Object { Write-Host ("  {0:N1} MB" -f ($_.Length / 1MB)) }
exit 0
