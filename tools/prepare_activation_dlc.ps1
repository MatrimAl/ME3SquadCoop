# Stage an asset-free activation DLC with a generated root TOC. No install or game launch.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$LegendaryExplorerPath,
 [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'PowerShell7 required'}
$activationRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$activationPrivate=[IO.Path]::GetFullPath((Join-Path $activationRepo 'local'))+[IO.Path]::DirectorySeparatorChar
$activationOutput=[IO.Path]::GetFullPath($OutputDirectory)
if(!$activationOutput.StartsWith($activationPrivate,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $activationOutput)){throw 'Fresh output below repository local/ required'}
$activationAncestor=[IO.Path]::GetDirectoryName($activationOutput)
while($activationAncestor){
 if((Test-Path -LiteralPath $activationAncestor) -and ((Get-Item -LiteralPath $activationAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Output reparse ancestry forbidden'}
 $activationAncestor=[IO.Path]::GetDirectoryName($activationAncestor)
}
$activationTool=(Resolve-Path -LiteralPath $LegendaryExplorerPath).Path
$activationAssembly=Join-Path $activationTool 'LegendaryExplorerCore.dll'
$activationToolHash=(Get-FileHash -LiteralPath $activationAssembly -Algorithm SHA256).Hash
Add-Type -TypeDefinition @"
using System;using System.IO;using System.Reflection;
public static class ME3ActivationResolver {
 public static void Install(string folder) {
  AppDomain.CurrentDomain.AssemblyResolve += (sender,args) => {
   string path=Path.Combine(folder,new AssemblyName(args.Name).Name+".dll");
   return File.Exists(path)?Assembly.LoadFrom(path):null;
  };
 }
}
"@
[ME3ActivationResolver]::Install($activationTool)
$null=[Reflection.Assembly]::LoadFrom($activationAssembly)
$activationRoot=Join-Path $activationOutput 'DLC_MOD_ME3SquadCoop'
$activationCooked=Join-Path $activationRoot 'CookedPCConsole'
$null=New-Item -ItemType Directory -Path $activationCooked
$activationStatus=[ordered]@{Status='preparing';Installed=$false;EngineAccepted=$false;GameExecuted=$false;
 ContainsOwnerGameAssets=$false;ToolSHA256=$activationToolHash;Files=@()}
try {
 $activationStream=[IO.MemoryStream]::new()
 $activationWriter=[IO.BinaryWriter]::new($activationStream)
 try {
  foreach($activationWord in @([uint32]1,[uint32]0x2ad,[uint32]0xcd,[uint32]0x3006b,[uint32]5100,[uint32]0,[uint32]0x0c,[uint32]0,[uint32]0)){$activationWriter.Write([uint32]$activationWord)}
  $activationWriter.Write([byte[]]::new(72))
  $activationBytes=$activationStream.ToArray()
 } finally {$activationWriter.Dispose();$activationStream.Dispose()}
 if($activationBytes.Length -ne 108){throw 'Unexpected Mount length'}
 [IO.File]::WriteAllBytes((Join-Path $activationCooked 'Mount.dlc'),$activationBytes)
 $activationTOC=[LegendaryExplorerCore.Unreal.TOCCreator]::CreateDLCTOCForDirectory($activationRoot,[LegendaryExplorerCore.Packages.MEGame]::LE3)
 try {[IO.File]::WriteAllBytes((Join-Path $activationRoot 'PCConsoleTOC.bin'),$activationTOC.ToArray())} finally {$activationTOC.Dispose()}
 $activationParsed=[LegendaryExplorerCore.Unreal.TOCBinFile]::new((Join-Path $activationRoot 'PCConsoleTOC.bin'))
 $activationEntries=@($activationParsed.GetAllEntries()|ForEach-Object {@{Name=$_.name;Size=$_.size}})
 $activationMountEntries=@($activationEntries|Where-Object {$_.Name.Replace('/','\') -eq 'CookedPCConsole\Mount.dlc'})
 if($activationMountEntries.Count -ne 1 -or $activationMountEntries[0].Size -ne 108){throw 'TOC Mount entry mismatch'}
 if((Get-FileHash -LiteralPath $activationAssembly -Algorithm SHA256).Hash -ne $activationToolHash){throw 'Tool assembly changed'}
 $activationStatus.TOCEntries=$activationEntries
 $activationStatus.Files=@(Get-ChildItem -LiteralPath $activationRoot -File -Recurse|ForEach-Object {@{RelativePath=[IO.Path]::GetRelativePath($activationRoot,$_.FullName);Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
 if($activationStatus.Files.Count -ne 2){throw 'Unexpected activation payload'}
 $activationStatus.Status='prepared-not-installed'
} catch {$activationStatus.Status='failed';$activationStatus.Error=$_.Exception.Message;throw}
finally {$activationStatus|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $activationOutput 'preparation.json') -Encoding utf8}
$activationStatus|ConvertTo-Json -Depth 6
