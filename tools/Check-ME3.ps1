<# Read-only compatibility summary for the owner and a friend. No installation or game launch. #>
param(
    [Parameter(Mandatory=$true)][string]$GameDirectory,
    [string]$OutputFile = (Join-Path $PSScriptRoot 'ME3-Compatibility.json')
)
$ErrorActionPreference = 'Stop'
$gameRoot = (Resolve-Path -LiteralPath $GameDirectory).Path
$exe = Join-Path $gameRoot 'Binaries\Win64\MassEffect3.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw 'ME3 root folder required: it must contain Binaries\Win64\MassEffect3.exe'
}
$names = @('Binaries\Win64\MassEffect3.exe','BioGame\CookedPCConsole\SFXGame.pcc','BioGame\CookedPCConsole\Engine.pcc')
$files = @()
foreach ($name in $names) {
    $path = Join-Path $gameRoot $name
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $file = Get-Item -LiteralPath $path
        $files += [ordered]@{ file=$name; size=$file.Length; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
}
$dlcRoot = Join-Path $gameRoot 'BioGame\DLC'
$dlcs = @()
if (Test-Path -LiteralPath $dlcRoot) {
    $dlcs = @(Get-ChildItem -LiteralPath $dlcRoot -Directory | Where-Object { $_.Name.StartsWith('DLC_MOD_', [StringComparison]::OrdinalIgnoreCase) } | Select-Object -ExpandProperty Name)
}
$asiRoot = Join-Path $gameRoot 'Binaries\Win64\ASI'
$asis = @()
if (Test-Path -LiteralPath $asiRoot) {
    $asis = @(Get-ChildItem -LiteralPath $asiRoot -File -Filter '*.asi' | ForEach-Object {
        [ordered]@{ name=$_.Name; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    })
}
$versionInfo = (Get-Item -LiteralPath $exe).VersionInfo
# Whether the native pins describe THIS executable, decided offline. Reported as its own field rather than
# as a pass/fail, because "different build" and "the mod is wrong" have different owners.
$supportedSha = 'b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001'
$exeSha = ($files | Where-Object { $_.file -eq 'Binaries\Win64\MassEffect3.exe' }).sha256
$pinCheck = python -I (Join-Path (Split-Path -Parent $PSScriptRoot) 'tools\check_pins.py') --exe $exe 2>&1
# The checker's outcome is the RESULT/UNREADABLE line, not the last line of a multi-line message.
$pinSummary = ($pinCheck | Where-Object { $_ -match '^(RESULT|UNREADABLE|SUPPORT)\b' } | Select-Object -First 1)
if (-not $pinSummary) { $pinSummary = ($pinCheck | Select-Object -Last 1) }
$pinReport = [ordered]@{
    supportedExecutableSha256 = $supportedSha
    executableMatchesSupported = ($exeSha -eq $supportedSha)
    exitCode = $LASTEXITCODE
    # Exit 2 means the code sections are not plaintext, so no pin comparison is possible - it is a skip,
    # not a pass and not a mod defect. Spelled out so a 2 is never read as a failure of the mod.
    exitCodeMeaning = switch ($LASTEXITCODE) {
        0 { "all recognized pins match" }
        1 { "a pin does not match the supported build - the mod is wrong for its target" }
        2 { "the executable's code sections are not plaintext (or it is missing) - no pin comparison possible" }
        3 { "pins do not match, but this is not the supported build - about the build, not the mod" }
        default { "see tools/check_pins.py" }
    }
    summary = ($pinSummary -replace '^\s+', '').Trim()
}
$report = [ordered]@{
    schema=1
    game='ME3 Legendary Edition'
    fileVersion=$versionInfo.FileVersion
    fixedFileVersion=('{0}.{1}.{2}.{3}' -f $versionInfo.FileMajorPart,$versionInfo.FileMinorPart,$versionInfo.FileBuildPart,$versionInfo.FilePrivatePart)
    files=$files
    nativePins=$pinReport
    activeCustomDLC=$dlcs
    activeASI=$asis
    campaignCoopAvailable=$false
    note='Compatibility data only. This script does not install a co-op mod or launch the game.'
}
$outputPath = [IO.Path]::GetFullPath($OutputFile)
if (Test-Path -LiteralPath $outputPath) { throw 'Output file already exists; choose a new file name.' }
for ($ancestor = [IO.DirectoryInfo]::new([IO.Path]::GetDirectoryName($outputPath)); $null -ne $ancestor; $ancestor = $ancestor.Parent) {
    if ($ancestor.Exists -and ($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Output through a junction or symbolic link is not supported.'
    }
}
if ($outputPath.StartsWith($gameRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Save the compatibility report outside the game installation.'
}
$json = ($report | ConvertTo-Json -Depth 6) + [Environment]::NewLine
$bytes = [Text.UTF8Encoding]::new($false).GetBytes($json)
$stream = [IO.File]::Open($outputPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
try { $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }
Write-Output 'Compatibility report created. No game file or save was modified.'
Write-Output $outputPath
