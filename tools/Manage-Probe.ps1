<# Development test-copy toggle only. Not a co-op installer. #>
param(
    [Parameter(Mandatory=$true)][string]$TestCopy,
    [Parameter(Mandatory=$true)][ValidateSet('Enable','Disable')][string]$Action
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $TestCopy).Path
$markerPath = Join-Path $root '.me3-coop-sandbox.json'
if (-not (Test-Path -LiteralPath $markerPath -PathType Leaf)) { throw 'Verified test-copy marker is required.' }
$marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json
if ($marker.status -ne 'verified-copy' -or $marker.destination -ne $root -or $marker.source -eq $root) {
    throw 'This is not the recorded independent test copy.'
}
$gameExe = Join-Path $root 'Binaries\Win64\MassEffect3.exe'
$running = @(Get-Process MassEffect3 -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $gameExe })
if ($running.Count -gt 0) { throw 'Exit the test game first. Toggle takes effect on the next launch.' }
$active = Join-Path $root 'ME3SquadCoop.probe.enabled'
$inactive = Join-Path $root 'ME3SquadCoop.probe.disabled'
if ($Action -eq 'Disable') { $from = $active; $to = $inactive } else { $from = $inactive; $to = $active }
if (Test-Path -LiteralPath $to) {
    if (Test-Path -LiteralPath $from) { throw 'Both markers exist; inspect before making changes.' }
    Write-Output ('Already ' + $Action.ToLowerInvariant() + 'd.'); return
}
if (-not (Test-Path -LiteralPath $from -PathType Leaf)) { throw 'The expected development marker is missing.' }
foreach ($path in @($from,$to)) {
    $resolved = [IO.Path]::GetFullPath($path)
    if (-not $resolved.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Marker leaves test copy.' }
}
Move-Item -LiteralPath $from -Destination $to
Write-Output ('Research probe: ' + $Action + '. No original game or save was changed. Restart required.')
