# LUMI//DANCER - elevated system VST3 install (rename-swap safe against a
# host holding the old DLL loaded). Invoked via UAC by install helpers.
$ErrorActionPreference = 'Stop'
$v3   = 'C:\Program Files\Common Files\VST3'
$name = 'LUMI DANCER.vst3'
$src  = 'C:\Users\xhan1\lumi-dancer\build\windows\LumiDancer_artefacts\Release\VST3\LUMI DANCER.vst3'
$log  = 'C:\Users\xhan1\lumi-dancer\swap-result.txt'

try {
    # Sweep any unlockable leftovers from previous swaps.
    Get-ChildItem $v3 -Filter ($name + '.old*') -ErrorAction SilentlyContinue |
        ForEach-Object { try { Remove-Item $_.FullName -Recurse -Force -ErrorAction Stop } catch {} }

    $dst = Join-Path $v3 $name
    if (Test-Path $dst) {
        # Unique suffix: a still-locked previous .old must not block the swap.
        Rename-Item $dst ($name + '.old' + [System.IO.Path]::GetRandomFileName().Substring(0,4))
    }
    Copy-Item -Recurse -Force $src $dst
    'SWAP-OK' | Out-File $log
} catch {
    $_.Exception.Message | Out-File $log
    exit 1
}
