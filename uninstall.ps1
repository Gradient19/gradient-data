$ErrorActionPreference = 'Stop'
$dest = Join-Path $env:LOCALAPPDATA 'Programs\GradientData\0.3.0-rc.2'
$receiptPath = Join-Path $dest 'INSTALL_RECEIPT'
$manifestPath = Join-Path $dest 'SHA256SUMS'
if (-not (Test-Path -LiteralPath $dest -PathType Container) -or (Get-Item -LiteralPath $dest).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Matching installation not found' }
$receipt = [IO.File]::ReadAllLines($receiptPath)
$manifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($receipt.Count -ne 2 -or $receipt[0] -cne 'Gradient Data 0.3.0-rc.2' -or $receipt[1] -cne $manifestHash) { throw 'Installation receipt mismatch' }
$rows = @{}
foreach ($line in [IO.File]::ReadAllLines($manifestPath)) {
    if ($line -cnotmatch '^([0-9a-f]{64})  ([A-Za-z0-9_./-]+)$') { throw 'Invalid installed manifest' }
    $name = $Matches[2]
    if ($rows.ContainsKey($name) -or $name.StartsWith('/') -or $name.Split('/') -contains '..' -or $name.Split('/') -contains '.' -or $name.EndsWith('/') -or $name.Contains('//')) { throw 'Invalid installed manifest path' }
    $rows[$name] = $Matches[1]
}
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
$pending.Push(@{ Path = $dest; Prefix = '' })
while ($pending.Count -gt 0) {
    $directory = $pending.Pop()
    foreach ($item in Get-ChildItem -LiteralPath $directory.Path -Force) {
        $relative = if ($directory.Prefix) { $directory.Prefix + '/' + $item.Name } else { $item.Name }
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Installation contains a link: $relative" }
        if ($item -is [IO.DirectoryInfo]) {
            $actualDirs[$relative] = $true
            $pending.Push(@{ Path = $item.FullName; Prefix = $relative })
        }
        elseif ($item -is [IO.FileInfo]) { $actualFiles[$relative] = $true }
        else { throw "Installation contains a special entry: $relative" }
    }
}
if ($actualDirs.Count -ne $expectedDirs.Count -or $actualFiles.Count -ne $rows.Count + 2) { throw 'Installation contains unexpected files or directories' }
foreach ($name in $actualDirs.Keys) { if (-not $expectedDirs.ContainsKey($name)) { throw "Unexpected installed directory: $name" } }
foreach ($name in $actualFiles.Keys) { if ($name -ne 'SHA256SUMS' -and $name -ne 'INSTALL_RECEIPT' -and -not $rows.ContainsKey($name)) { throw "Unexpected installed file: $name" } }
foreach ($name in $rows.Keys) {
    if ((Get-FileHash -LiteralPath (Join-Path $dest $name) -Algorithm SHA256).Hash.ToLowerInvariant() -cne $rows[$name]) { throw "Installed file modified: $name" }
}
foreach ($name in $rows.Keys) { [IO.File]::Delete((Join-Path $dest $name)) }
[IO.File]::Delete($receiptPath)
[IO.File]::Delete($manifestPath)
foreach ($name in ($expectedDirs.Keys | Sort-Object { $_.Split('/').Count } -Descending)) { [IO.Directory]::Delete((Join-Path $dest $name), $false) }
[IO.Directory]::Delete($dest, $false)
Write-Output "Removed $dest"
