$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$manifest = Join-Path $root 'SHA256SUMS'
$rows = @{}
foreach ($line in [IO.File]::ReadAllLines($manifest)) {
    if ($line -cnotmatch '^([0-9a-f]{64})  ([A-Za-z0-9_./-]+)$') { throw 'Invalid SHA256SUMS' }
    $name = $Matches[2]
    if ($rows.ContainsKey($name) -or $name.StartsWith('/') -or $name.Split('/') -contains '..' -or $name.Split('/') -contains '.' -or $name.EndsWith('/') -or $name.Contains('//')) { throw 'Invalid manifest path' }
    $rows[$name] = $Matches[1]
}
$required = @(
    'windows-x86_64/uacd.exe', 'windows-x86_64/uacd-gui.exe',
    'windows-x86_64/gradient_data_c.dll', 'windows-x86_64/gradient_data_c.dll.lib',
    'windows-x86_64/gradient_data_c.lib',
    'include/gradient_data.h', 'include/gradient_data.hpp',
    'include/uacd_archive.h', 'include/uacd_archive.hpp',
    'cmake/GradientDataConfig.cmake',
    'examples/read_member.c', 'examples/read_member.cpp', 'examples/runtime_asset_reader.c',
    'GUI.md', 'Start_Gradient_Data.cmd'
)
foreach ($name in $required) { if (-not $rows.ContainsKey($name)) { throw "Missing: $name" } }
$expectedDirs = @{}
foreach ($name in $rows.Keys) {
    $parts = $name.Split('/')
    for ($i = 1; $i -lt $parts.Length; $i++) { $expectedDirs[($parts[0..($i - 1)] -join '/')] = $true }
}
$actualFiles = @{}
$actualDirs = @{}
# Build relative names from directory entries, never from string lengths of
# absolute paths. Windows may expand a short (8.3) ancestor in FullName.
$pending = New-Object System.Collections.Stack
$pending.Push(@{ Path = $root; Prefix = '' })
while ($pending.Count -gt 0) {
    $directory = $pending.Pop()
    foreach ($item in Get-ChildItem -LiteralPath $directory.Path -Force) {
        $relative = if ($directory.Prefix) { $directory.Prefix + '/' + $item.Name } else { $item.Name }
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Bundle contains a link: $relative" }
        if ($item -is [IO.DirectoryInfo]) {
            $actualDirs[$relative] = $true
            $pending.Push(@{ Path = $item.FullName; Prefix = $relative })
        }
        elseif ($item -is [IO.FileInfo]) { $actualFiles[$relative] = $true }
        else { throw "Bundle contains a special entry: $relative" }
    }
}
if ($actualDirs.Count -ne $expectedDirs.Count -or $actualFiles.Count -ne $rows.Count + 1) { throw 'Bundle file/directory roster mismatch' }
foreach ($name in $actualDirs.Keys) { if (-not $expectedDirs.ContainsKey($name)) { throw "Unexpected directory: $name" } }
foreach ($name in $actualFiles.Keys) { if ($name -ne 'SHA256SUMS' -and -not $rows.ContainsKey($name)) { throw "Unexpected file: $name" } }
foreach ($name in $rows.Keys) {
    $file = Join-Path $root $name
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() -cne $rows[$name]) { throw "Checksum mismatch: $name" }
}
$parent = Join-Path $env:LOCALAPPDATA 'Programs\GradientData'
$dest = Join-Path $parent '0.3.0-rc.3'
if (Test-Path -LiteralPath $dest) { throw "Installation already exists: $dest" }
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$stage = Join-Path $parent ('.gradient-data-install-' + [Guid]::NewGuid().ToString('N'))
# MoveFileExW with flags 0 refuses an existing destination; Move-Item could merge.
if (-not ('GradientDataCustomerMoveV1' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class GradientDataCustomerMoveV1 {
    [DllImport("kernel32.dll", EntryPoint="MoveFileExW", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern bool MoveFileEx(string source, string destination, uint flags);
}
'@
}
$ownedStage = $false
try {
    New-Item -ItemType Directory -Path $stage | Out-Null
    $ownedStage = $true
    foreach ($item in Get-ChildItem -LiteralPath $root -Force) { Copy-Item -LiteralPath $item.FullName -Destination $stage -Recurse -Force }
    $manifestHash = (Get-FileHash -LiteralPath (Join-Path $stage 'SHA256SUMS') -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText((Join-Path $stage 'INSTALL_RECEIPT'), "Gradient Data 0.3.0-rc.3`n$manifestHash`n", [Text.Encoding]::ASCII)
    foreach ($name in $rows.Keys) {
        if ((Get-FileHash -LiteralPath (Join-Path $stage $name) -Algorithm SHA256).Hash.ToLowerInvariant() -cne $rows[$name]) { throw "Staged file mismatch: $name" }
    }
    $stageEntries = @(Get-ChildItem -LiteralPath $stage -Recurse -Force)
    foreach ($item in $stageEntries) { if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Staged bundle contains a link' } }
    $stageFiles = @($stageEntries | Where-Object { $_ -is [IO.FileInfo] })
    $stageDirs = @($stageEntries | Where-Object { $_ -is [IO.DirectoryInfo] })
    if ($stageFiles.Count -ne $rows.Count + 2 -or $stageDirs.Count -ne $expectedDirs.Count) { throw 'Staged roster mismatch' }
    if (-not [GradientDataCustomerMoveV1]::MoveFileEx($stage, $dest, [uint32]0)) {
        $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Exclusive installation publish failed (Win32 $code); destination may already exist"
    }
    $ownedStage = $false
} finally {
    if ($ownedStage -and (Test-Path -LiteralPath $stage -PathType Container)) {
        # Delete only this stage's listed files; never recurse into an unexpected file.
        foreach ($name in $rows.Keys) {
            $path = Join-Path $stage $name
            if ([IO.File]::Exists($path)) { [IO.File]::Delete($path) }
        }
        foreach ($name in @('SHA256SUMS','INSTALL_RECEIPT')) {
            $path = Join-Path $stage $name
            if ([IO.File]::Exists($path)) { [IO.File]::Delete($path) }
        }
        foreach ($name in ($expectedDirs.Keys | Sort-Object { $_.Split('/').Count } -Descending)) {
            $path = Join-Path $stage $name
            if ([IO.Directory]::Exists($path)) { try { [IO.Directory]::Delete($path, $false) } catch {} }
        }
        try { [IO.Directory]::Delete($stage, $false) } catch {}
    }
}
Write-Output "Installed at $dest"
