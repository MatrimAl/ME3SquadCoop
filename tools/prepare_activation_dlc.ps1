# Stage an asset-free activation DLC with a generated root TOC. No install or game launch.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$LegendaryExplorerPath,
 [Parameter(Mandatory)][string]$OutputDirectory,
 [switch]$IncludeMinimalConfig,
 [switch]$IncludeEnglishTlk
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'PowerShell7 required'}
if($IncludeEnglishTlk -and !$IncludeMinimalConfig){throw 'English metadata variant requires the minimal named config'}
$activationStringID=if($IncludeEnglishTlk){1915100}else{0}
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
 ContainsOwnerGameAssets=$false;ToolSHA256=$activationToolHash;Files=@();
 Variant=$(if($IncludeEnglishTlk){'mount-toc-config-english-tlk'}elseif($IncludeMinimalConfig){'mount-toc-minimal-config'}else{'mount-toc'});
 ResearchStringID=$activationStringID;StringIDAllocationVerified=$false}
try {
 $activationStream=[IO.MemoryStream]::new()
 $activationWriter=[IO.BinaryWriter]::new($activationStream)
 try {
  foreach($activationWord in @([uint32]1,[uint32]0x2ad,[uint32]0xcd,[uint32]0x3006b,[uint32]5100,[uint32]0,[uint32]0x0c,[uint32]$activationStringID,[uint32]$activationStringID)){$activationWriter.Write([uint32]$activationWord)}
  $activationWriter.Write([byte[]]::new(72))
  $activationBytes=$activationStream.ToArray()
 } finally {$activationWriter.Dispose();$activationStream.Dispose()}
 if($activationBytes.Length -ne 108){throw 'Unexpected Mount length'}
 [IO.File]::WriteAllBytes((Join-Path $activationCooked 'Mount.dlc'),$activationBytes)
 if($IncludeMinimalConfig){
  $activationXml=@"
<?xml version="1.0" encoding="utf-8"?>
<CoalesceAsset id="BioEngine" name="bioengine.ini" source="..\..\biogame\config\bioengine.ini">
 <Sections><Section name="core.system"><Property name="seekfreepcpaths" type="2">..\..\BIOGame\DLC\DLC_MOD_ME3SquadCoop\CookedPCConsole</Property></Section></Sections>
</CoalesceAsset>
"@
  $activationAssets=[Collections.Generic.Dictionary[string,string]]::new()
  $activationAssets.Add('BioEngine.xml',$activationXml)
  $activationConfig=Join-Path $activationCooked 'Default_DLC_MOD_ME3SquadCoop.bin'
  $activationMemory=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::CompileFromMemory($activationAssets)
  try {[IO.File]::WriteAllBytes($activationConfig,$activationMemory.ToArray())} finally {$activationMemory.Dispose()}
  $activationConfigStream=[IO.File]::OpenRead($activationConfig)
  try {$activationConfigReadback=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::DecompileGame3ToMemory($activationConfigStream)} finally {$activationConfigStream.Dispose()}
  [xml]$activationReadXml=$activationConfigReadback['BioEngine.xml']
  $activationProperties=@($activationReadXml.SelectNodes('//Property'))
  if($activationProperties.Count -ne 1 -or $activationProperties[0].GetAttribute('name') -ne 'seekfreepcpaths' -or
     $activationProperties[0].GetAttribute('type') -ne '2' -or $activationProperties[0].InnerText -ne '..\..\BIOGame\DLC\DLC_MOD_ME3SquadCoop\CookedPCConsole'){throw 'Minimal config roundtrip mismatch'}
 }
 if($IncludeEnglishTlk){
  # Project-authored strings only; no copied game/default TLK content.
  $activationStrings=[Collections.Generic.List[LegendaryExplorerCore.TLK.TLKStringRef]]::new()
  $activationTexts=@('ME3 Squad Co-op','DLC_MOD_ME3SquadCoop','en-us','Male','Female')
  $activationIDs=@($activationStringID,($activationStringID+1),($activationStringID+2),($activationStringID+3),($activationStringID+3))
  for($activationIndex=0;$activationIndex -lt $activationTexts.Count;$activationIndex++){
   $activationStrings.Add([LegendaryExplorerCore.TLK.TLKStringRef]::new($activationIDs[$activationIndex],($activationTexts[$activationIndex]+[char]0),1,-1))
  }
  $activationTlk=Join-Path $activationCooked 'DLC_MOD_ME3SquadCoop_INT.tlk'
  [LegendaryExplorerCore.TLK.ME2ME3.HuffmanCompression]::SaveToTlkFile($activationTlk,$activationStrings)
  $activationTlkReader=[LegendaryExplorerCore.TLK.ME2ME3.ME2ME3TalkFile]::new($activationTlk)
  for($activationIndex=0;$activationIndex -lt 4;$activationIndex++){
   if($activationTlkReader.FindDataById(($activationStringID+$activationIndex),$false,$true,$true,$true) -ne $activationTexts[$activationIndex]){throw 'Authored TLK male readback mismatch'}
  }
  if($activationTlkReader.FindDataById(($activationStringID+3),$false,$true,$true,$false) -ne 'Female'){throw 'Authored TLK female readback mismatch'}
 }
 $activationSavedCulture=[Threading.Thread]::CurrentThread.CurrentCulture
 try {
  # Retained LEC hashes uppercased filenames using the current culture.
  # A Turkish .bin must hash like native ASCII .BIN, not dotted-capital-I.
  [Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
  $activationTOC=[LegendaryExplorerCore.Unreal.TOCCreator]::CreateDLCTOCForDirectory($activationRoot,[LegendaryExplorerCore.Packages.MEGame]::LE3)
 } finally {[Threading.Thread]::CurrentThread.CurrentCulture=$activationSavedCulture}
 try {[IO.File]::WriteAllBytes((Join-Path $activationRoot 'PCConsoleTOC.bin'),$activationTOC.ToArray())} finally {$activationTOC.Dispose()}
 $activationParsed=[LegendaryExplorerCore.Unreal.TOCBinFile]::new((Join-Path $activationRoot 'PCConsoleTOC.bin'))
 $activationEntries=@($activationParsed.GetAllEntries()|ForEach-Object {@{Name=$_.name;Size=$_.size}})
 $activationMountEntries=@($activationEntries|Where-Object {$_.Name.Replace('/','\') -eq 'CookedPCConsole\Mount.dlc'})
 if($activationMountEntries.Count -ne 1 -or $activationMountEntries[0].Size -ne 108){throw 'TOC Mount entry mismatch'}
 if($IncludeMinimalConfig){
  $activationConfigEntries=@($activationEntries|Where-Object {$_.Name.Replace('/','\') -eq 'CookedPCConsole\Default_DLC_MOD_ME3SquadCoop.bin'})
  if($activationConfigEntries.Count -ne 1 -or $activationConfigEntries[0].Size -ne (Get-Item -LiteralPath $activationConfig).Length){throw 'TOC config entry mismatch'}
 }
 if($IncludeEnglishTlk){
  $activationTlkEntries=@($activationEntries|Where-Object {$_.Name.Replace('/','\') -eq 'CookedPCConsole\DLC_MOD_ME3SquadCoop_INT.tlk'})
  if($activationTlkEntries.Count -ne 1 -or $activationTlkEntries[0].Size -ne (Get-Item -LiteralPath $activationTlk).Length){throw 'TOC TLK entry mismatch'}
 }
 if((Get-FileHash -LiteralPath $activationAssembly -Algorithm SHA256).Hash -ne $activationToolHash){throw 'Tool assembly changed'}
 $activationStatus.TOCEntries=$activationEntries
 $activationStatus.Files=@(Get-ChildItem -LiteralPath $activationRoot -File -Recurse|ForEach-Object {@{RelativePath=[IO.Path]::GetRelativePath($activationRoot,$_.FullName);Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
 $activationExpectedCount=if($IncludeEnglishTlk){4}elseif($IncludeMinimalConfig){3}else{2}
 if($activationStatus.Files.Count -ne $activationExpectedCount){throw 'Unexpected activation payload'}
 $activationStatus.Status='prepared-not-installed'
} catch {$activationStatus.Status='failed';$activationStatus.Error=$_.Exception.Message;throw}
finally {$activationStatus|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $activationOutput 'preparation.json') -Encoding utf8}
$activationStatus|ConvertTo-Json -Depth 6
