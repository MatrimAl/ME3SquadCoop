# Offline authored-script DLC candidate. Never install, launch or activate.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$CompileReceipt,
 [Parameter(Mandatory)][string]$LegendaryExplorerPath,
 [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'PowerShell7 required'}
$dlcRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$dlcPrivate=[IO.Path]::GetFullPath((Join-Path $dlcRepo 'local'))+[IO.Path]::DirectorySeparatorChar
$dlcOutput=[IO.Path]::GetFullPath($OutputDirectory)
if(!$dlcOutput.StartsWith($dlcPrivate,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $dlcOutput)){throw 'Fresh output below repository local/ required'}
$dlcAncestor=[IO.Path]::GetDirectoryName($dlcOutput)
while($dlcAncestor){
 if((Test-Path -LiteralPath $dlcAncestor) -and ((Get-Item -LiteralPath $dlcAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Output reparse ancestry forbidden'}
 $dlcAncestor=[IO.Path]::GetDirectoryName($dlcAncestor)
}
$dlcReceiptPath=(Resolve-Path -LiteralPath $CompileReceipt).Path
if(!$dlcReceiptPath.StartsWith($dlcPrivate,[StringComparison]::OrdinalIgnoreCase)){throw 'Private repository receipt required'}
$dlcReceipt=Get-Content -LiteralPath $dlcReceiptPath -Raw|ConvertFrom-Json
$dlcOwnerAssets=$dlcReceipt.Status -eq 'compiled-with-owner-assets-not-installed'
if($dlcReceipt.Status -notin @('compiled-not-installed','compiled-with-owner-assets-not-installed') -or !$dlcReceipt.InputsUnchanged -or !$dlcReceipt.Classes.Count){throw 'Unusable package receipt'}
if($dlcOwnerAssets -and ($dlcReceipt.ContainsOwnerGameAssets -ne $true -or $dlcReceipt.DistributionAllowed -ne $false -or !$dlcReceipt.ScriptExecutableExportsPreserved -or !$dlcReceipt.FactoryAnchorInjected -or !$dlcReceipt.CompressedSequencePreserved -or $dlcReceipt.RelinkErrors.Count -or $dlcReceipt.AssetContract.AnimSet -ne 'ME3SquadCoopActions.ReviveAnimSet')){throw 'Unusable private owner-asset derivation'}
foreach($dlcClass in $dlcReceipt.Classes){if($dlcClass.HasErrors -or $dlcClass.Errors.Count -or $dlcClass.Warnings.Count){throw 'Unreviewed compiler diagnostics'}}
foreach($dlcInput in $dlcReceipt.InputsAfter.PSObject.Properties){if((Get-FileHash -LiteralPath $dlcInput.Name -Algorithm SHA256).Hash -ne $dlcInput.Value){throw 'Compile input changed'}}
$dlcPackage=(Resolve-Path -LiteralPath $dlcReceipt.Package).Path
if(!$dlcPackage.StartsWith($dlcPrivate,[StringComparison]::OrdinalIgnoreCase) -or (Get-FileHash -LiteralPath $dlcPackage -Algorithm SHA256).Hash -ne $dlcReceipt.PackageSHA256){throw 'Compiled package identity mismatch'}
$dlcName=[IO.Path]::GetFileNameWithoutExtension($dlcPackage)
if($dlcName -notmatch '^ME3SquadCoop[A-Za-z0-9_]*$'){throw 'Unexpected authored package name'}
$dlcTool=(Resolve-Path -LiteralPath $LegendaryExplorerPath).Path
$dlcAssembly=Join-Path $dlcTool 'LegendaryExplorerCore.dll'
$dlcAssemblyHash=(Get-FileHash -LiteralPath $dlcAssembly -Algorithm SHA256).Hash
Add-Type -TypeDefinition @'
using System;using System.IO;using System.Reflection;
public static class ME3DLCAssemblyResolver {
 public static void Install(string folder) {
  AppDomain.CurrentDomain.AssemblyResolve += (sender,args) => {
   string path=Path.Combine(folder,new AssemblyName(args.Name).Name+".dll");
   return File.Exists(path)?Assembly.LoadFrom(path):null;
  };
 }
}
'@
[ME3DLCAssemblyResolver]::Install($dlcTool)
$null=[Reflection.Assembly]::LoadFrom($dlcAssembly)
$dlcRoot=Join-Path $dlcOutput 'DLC_MOD_ME3SquadCoop'
$dlcCooked=Join-Path $dlcRoot 'CookedPCConsole'
$null=New-Item -ItemType Directory -Path $dlcCooked
$dlcStatus=[ordered]@{Status='preparing';GameExecuted=$false;TestsRun=$false;Installed=$false;EngineAccepted=$false;RuntimeInvoked=$false;
 CompileReceipt=$dlcReceiptPath;PackageSHA256=$dlcReceipt.PackageSHA256;ToolSHA256=$dlcAssemblyHash;InputsUnchanged=$false;
 ContainsOwnerGameAssets=$dlcOwnerAssets;DistributionAllowed=$false}
try {
 $dlcCopy=Join-Path $dlcCooked ($dlcName+'.pcc')
 Copy-Item -LiteralPath $dlcPackage -Destination $dlcCopy
 if((Get-FileHash -LiteralPath $dlcCopy -Algorithm SHA256).Hash -ne $dlcReceipt.PackageSHA256){throw 'Package copy changed'}
 $dlcXml=@"
<?xml version="1.0" encoding="utf-8"?>
<CoalesceAsset id="BioEngine" name="bioengine.ini" source="..\..\biogame\config\bioengine.ini">
 <Sections>
  <Section name="core.system"><Property name="seekfreepcpaths" type="2">..\..\BIOGame\DLC\DLC_MOD_ME3SquadCoop\CookedPCConsole</Property></Section>
  <Section name="engine.startuppackages">
   <Property name="dlcstartuppackage" type="3">$dlcName</Property>
   <Property name="dlcstartuppackagename" type="0">$dlcName</Property>
  </Section>
 </Sections>
</CoalesceAsset>
"@
 $dlcAssets=[Collections.Generic.Dictionary[string,string]]::new()
 $dlcAssets.Add('BioEngine.xml',$dlcXml)
 $dlcConfig=Join-Path $dlcCooked 'Default_DLC_MOD_ME3SquadCoop.bin'
 $dlcMemory=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::CompileFromMemory($dlcAssets)
 try {[IO.File]::WriteAllBytes($dlcConfig,$dlcMemory.ToArray())} finally {$dlcMemory.Dispose()}
 $dlcStream=[IO.File]::OpenRead($dlcConfig)
 try {$dlcReadback=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::DecompileGame3ToMemory($dlcStream)} finally {$dlcStream.Dispose()}
 [xml]$dlcReadXml=$dlcReadback['BioEngine.xml']
 foreach($dlcPropName in @('dlcstartuppackage','dlcstartuppackagename')){
  $dlcProp=$dlcReadXml.SelectSingleNode("//Section[@name='engine.startuppackages']/Property[@name='$dlcPropName']")
  if(!$dlcProp -or $dlcProp.InnerText -ne $dlcName){throw 'Startup config roundtrip mismatch'}
  $dlcExpectedType=if($dlcPropName -eq 'dlcstartuppackage'){'3'}else{'0'}
  if($dlcProp.GetAttribute('type') -ne $dlcExpectedType){throw 'Startup config operation type mismatch'}
 }
 $dlcMountStream=[IO.MemoryStream]::new()
 $dlcMountWriter=[IO.BinaryWriter]::new($dlcMountStream)
 try {
  foreach($dlcWord in @([uint32]1,[uint32]0x2ad,[uint32]0xcd,[uint32]0x3006b,[uint32]5100,[uint32]0,[uint32]0x0c,[uint32]0,[uint32]0)){$dlcMountWriter.Write([uint32]$dlcWord)}
  $dlcMountWriter.Write([byte[]]::new(72))
  $dlcMountBytes=$dlcMountStream.ToArray()
 } finally {$dlcMountWriter.Dispose();$dlcMountStream.Dispose()}
 if($dlcMountBytes.Length -ne 108){throw 'Unexpected Mount length'}
 [IO.File]::WriteAllBytes((Join-Path $dlcCooked 'Mount.dlc'),$dlcMountBytes)
 $dlcTOC=[LegendaryExplorerCore.Unreal.TOCCreator]::CreateDLCTOCForDirectory($dlcRoot,[LegendaryExplorerCore.Packages.MEGame]::LE3)
 try {[IO.File]::WriteAllBytes((Join-Path $dlcRoot 'PCConsoleTOC.bin'),$dlcTOC.ToArray())} finally {$dlcTOC.Dispose()}
 $dlcParsedTOC=[LegendaryExplorerCore.Unreal.TOCBinFile]::new((Join-Path $dlcRoot 'PCConsoleTOC.bin'))
 $dlcEntries=@($dlcParsedTOC.GetAllEntries()|ForEach-Object {@{Name=$_.name;Size=$_.size}})
 foreach($dlcExpected in @('Mount.dlc',($dlcName+'.pcc'),'Default_DLC_MOD_ME3SquadCoop.bin')){
  $dlcEntry=@($dlcEntries|Where-Object {$_.Name.Replace('/','\') -eq ('CookedPCConsole\'+$dlcExpected)})
  if($dlcEntry.Count -ne 1 -or $dlcEntry[0].Size -ne (Get-Item -LiteralPath (Join-Path $dlcCooked $dlcExpected)).Length){throw 'TOC missing or wrong-size payload entry'}
 }
 foreach($dlcInput in $dlcReceipt.InputsAfter.PSObject.Properties){if((Get-FileHash -LiteralPath $dlcInput.Name -Algorithm SHA256).Hash -ne $dlcInput.Value){throw 'Compile input changed during preparation'}}
 if((Get-FileHash -LiteralPath $dlcAssembly -Algorithm SHA256).Hash -ne $dlcAssemblyHash){throw 'Tool assembly changed'}
 $dlcStatus.InputsUnchanged=$true
 $dlcStatus.Status='prepared-not-installed'
 $dlcStatus.TOCEntries=$dlcEntries
 $dlcStatus.Files=@(Get-ChildItem -LiteralPath $dlcRoot -File -Recurse|ForEach-Object {@{RelativePath=[IO.Path]::GetRelativePath($dlcRoot,$_.FullName);Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
} catch {$dlcStatus.Status='failed';$dlcStatus.Error=$_.Exception.Message;throw}
finally {$dlcStatus|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $dlcOutput 'preparation.json') -Encoding utf8}
[pscustomobject]$dlcStatus|Select-Object Status,InputsUnchanged,Installed,EngineAccepted,RuntimeInvoked,TOCEntries|ConvertTo-Json -Depth 4
