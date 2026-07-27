# LUMI//DANCER - elevated system VST3 install (rename-swap safe against a
# host holding the old DLL loaded). Invoked via UAC by install helpers.
$ErrorActionPreference = 'Stop'
$v3   = 'C:\Program Files\Common Files\VST3'
$name = 'LUMI DANCER.vst3'
$src  = 'C:\Users\xhan1\lumi-dancer\build\windows\LumiDancer_artefacts\Release\VST3\LUMI DANCER.vst3'
$log  = 'C:\Users\xhan1\lumi-dancer\swap-result.txt'

try {
    # Quarantine lives OUTSIDE the VST3 tree: hosts scan the VST3 directory
    # recursively, so a renamed-in-place bundle would still get found and
    # loaded. Same volume as VST3, so moving a locked (memory-mapped)
    # bundle still succeeds as a rename.
    $trash = 'C:\ProgramData\lumi-dancer-old-vst3'
    New-Item -ItemType Directory -Force -Path $trash | Out-Null
    Get-ChildItem $trash -ErrorAction SilentlyContinue |
        ForEach-Object { try { Remove-Item $_.FullName -Recurse -Force -ErrorAction Stop } catch {} }
    # Also sweep any legacy in-tree .old leftovers from earlier installs.
    Get-ChildItem $v3 -Filter ($name + '.old*') -ErrorAction SilentlyContinue |
        ForEach-Object {
            try { Move-Item $_.FullName (Join-Path $trash ([System.IO.Path]::GetRandomFileName())) -ErrorAction Stop } catch {}
        }

    $dst = Join-Path $v3 $name
    if (Test-Path $dst) {
        Move-Item $dst (Join-Path $trash ([System.IO.Path]::GetRandomFileName()))
    }
    Copy-Item -Recurse -Force $src $dst
    'SWAP-OK' | Out-File $log
} catch {
    $_.Exception.Message | Out-File $log
    exit 1
}
