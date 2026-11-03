# Combine a verified authored activation with a PRIVATE owner-derived package.
# Fresh repository local/ output only. Never install, launch or modify inputs.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$ActivationReceipt,
 [Parameter(Mandatory)][string]$CompileReceipt,
 [Parameter(Mandatory)][string]$StarterConfigTemplate,
 [Parameter(Mandatory)][string]$LegendaryExplorerPath,
 [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'PowerShell7 required'}
$addonRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$addonPrivate=[IO.Path]::GetFullPath((Join-Path $addonRepo 'local'))+[IO.Path]::DirectorySeparatorChar
function AssertAddonPlainPath([string]$Path){
 $addonWalk=[IO.Path]::GetFullPath($Path)
 while($addonWalk){
  if((Test-Path -LiteralPath $addonWalk) -and ((Get-Item -LiteralPath $addonWalk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Reparse target or ancestry forbidden'}
  $addonWalk=[IO.Path]::GetDirectoryName($addonWalk)
 }
}
function ResolveAddonPrivateFile([string]$Path){
 $addonResolved=(Resolve-Path -LiteralPath $Path).Path
 if(!$addonResolved.StartsWith($addonPrivate,[StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath $addonResolved -PathType Leaf)){throw 'Private repository file required'}
 AssertAddonPlainPath $addonResolved
 return $addonResolved
}
$addonOutput=[IO.Path]::GetFullPath($OutputDirectory)
$addonOutputRoot=[IO.Path]::GetFullPath((Join-Path $addonRepo 'local/revive-addon'))+[IO.Path]::DirectorySeparatorChar
if(![IO.Path]::IsPathFullyQualified($OutputDirectory) -or !$addonOutput.StartsWith($addonOutputRoot,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $addonOutput)){throw 'Fresh absolute output below repository local/revive-addon/ required'}
AssertAddonPlainPath $addonOutput
$addonActivation=ResolveAddonPrivateFile $ActivationReceipt
$addonReceiptPath=ResolveAddonPrivateFile $CompileReceipt
$addonReceipt=Get-Content -LiteralPath $addonReceiptPath -Raw|ConvertFrom-Json
if($addonReceipt.Status -ne 'compiled-with-owner-assets-not-installed' -or $addonReceipt.Installed -ne $false -or
   $addonReceipt.GameExecuted -ne $false -or $addonReceipt.ContainsOwnerGameAssets -ne $true -or $addonReceipt.DistributionAllowed -ne $false -or
   $addonReceipt.InputsUnchanged -ne $true -or !$addonReceipt.ScriptExecutableExportsPreserved -or !$addonReceipt.FactoryAnchorInjected -or
   !$addonReceipt.CompressedSequencePreserved -or @($addonReceipt.RelinkErrors).Count -or @($addonReceipt.Classes).Count -ne 7){throw 'Verified seven-class private owner-asset receipt required'}
$addonClassNames=@('Context','Action','Binding','Animation','Setup','Service','Factory')|ForEach-Object {'SFXSquadCoop_Revive'+$_+'.uc'}
$addonSources=@($addonReceipt.Sources|ForEach-Object {[IO.Path]::GetFullPath($_)})
$addonExpectedSources=@($addonClassNames|ForEach-Object {[IO.Path]::GetFullPath((Join-Path $addonRepo ('scripts/'+$_)))})
if($addonSources.Count -ne 7 -or @(Compare-Object $addonSources $addonExpectedSources).Count -or @($addonSources|Sort-Object -Unique).Count -ne 7){throw 'Unexpected authored source set'}
foreach($addonClass in $addonReceipt.Classes){
 if($addonClass.HasErrors -or @($addonClass.Errors).Count -or @($addonClass.Warnings).Count){throw 'Unreviewed compiler diagnostics'}
}
if(@(Compare-Object @($addonReceipt.Classes|ForEach-Object {$_.Source}) $addonExpectedSources).Count){throw 'Compiler class/source mismatch'}
$addonPackage=ResolveAddonPrivateFile $addonReceipt.Package
if([IO.Path]::GetFileName($addonPackage) -cne 'ME3SquadCoopActions.pcc' -or
   (Get-FileHash -LiteralPath $addonPackage -Algorithm SHA256).Hash -ne $addonReceipt.PackageSHA256){throw 'Private package identity mismatch'}
$addonScriptReceipt=ResolveAddonPrivateFile $addonReceipt.ScriptCompileReceipt
$addonScript=Get-Content -LiteralPath $addonScriptReceipt -Raw|ConvertFrom-Json
if($addonScript.Status -ne 'compiled-not-installed' -or !$addonScript.InputsUnchanged -or
   $addonScript.PackageSHA256 -ne $addonReceipt.ScriptPackageSHA256 -or
   @(Compare-Object @($addonScript.Sources) $addonExpectedSources).Count -or @($addonScript.Classes).Count -ne 7){throw 'Original script compilation provenance mismatch'}
$addonClosure=@('ReviveAnimSet|BioDynamicAnimSet','ReviveAnimationData|Package',
 'ReviveAnimationData.HMM_DG_Deaths_CB_MP_Revive|AnimSequence','ReviveAnimationData.HMM_DG_Deaths_BioAnimSetData|BioAnimSetData',
 'ReviveAnimationData.HMM_DG_Deaths_CB_MP_Revive.SFXAnimNotify_RaceWwiseEvent_8|SFXAnimNotify_RaceWwiseEvent',
 'ReviveAnimationData.HMM_DG_Deaths_CB_MP_Revive.SFXAnimNotify_RaceWwiseEvent_9|SFXAnimNotify_RaceWwiseEvent',
 'ReviveAnimationData.HMM_DG_Deaths_CB_MP_Revive.AnimNotify_WwiseEvent_8|AnimNotify_WwiseEvent',
 'ReviveAnimationData.HMM_DG_Deaths_CB_MP_Revive.SFXAnimNotify_RaceWwiseEvent_11|SFXAnimNotify_RaceWwiseEvent',
 'Wwise_Generic_Foley|Package','Wwise_Generic_Foley._Play_foot_scuffs_short|WwiseEvent')
$addonActualClosure=@($addonReceipt.AssetContract.AddedExports|ForEach-Object {$_.Path+'|'+$_.Class})
if($addonReceipt.AssetContract.AnimSet -cne 'ME3SquadCoopActions.ReviveAnimSet' -or $addonReceipt.AssetContract.SequenceCount -ne 1 -or
   $addonActualClosure.Count -ne 10 -or @(Compare-Object $addonActualClosure $addonClosure -CaseSensitive).Count){throw 'Unexpected owner-asset closure'}
$addonTool=(Resolve-Path -LiteralPath $LegendaryExplorerPath).Path
$addonAssembly=Join-Path $addonTool 'LegendaryExplorerCore.dll'
$addonTemplate=(Resolve-Path -LiteralPath $StarterConfigTemplate).Path
$addonVerifier=Join-Path $PSScriptRoot 'verify_activation_dlc.ps1'
$addonInputs=@{}
foreach($addonInput in @($addonReceipt.InputsAfter.PSObject.Properties)){
 AssertAddonPlainPath $addonInput.Name
 if((Get-FileHash -LiteralPath $addonInput.Name -Algorithm SHA256).Hash -ne $addonInput.Value){throw 'Owner-asset derivation input changed'}
 $addonInputs[$addonInput.Name]=$addonInput.Value
}
foreach($addonSource in $addonExpectedSources){
 if(!$addonInputs.ContainsKey($addonSource)){throw 'Source missing from package provenance'}
}
foreach($addonInput in @($addonAssembly,$addonTemplate,$addonVerifier,$addonActivation,$addonReceiptPath,$addonScriptReceipt,$addonPackage,$PSCommandPath)){
 AssertAddonPlainPath $addonInput
 $addonInputs[$addonInput]=(Get-FileHash -LiteralPath $addonInput -Algorithm SHA256).Hash
}
$addonBaseRoot=Join-Path ([IO.Path]::GetDirectoryName($addonActivation)) 'DLC_MOD_ME3SquadCoop'
foreach($addonInputDirectory in @([IO.Path]::GetDirectoryName($addonActivation),[IO.Path]::GetDirectoryName($addonReceiptPath),[IO.Path]::GetDirectoryName($addonPackage))){
 if($addonOutput.StartsWith(($addonInputDirectory+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)){throw 'Output must not be inside an input bundle'}
}
# The existing verifier is read-only and checks the exact four-file set,
# Mount/TLK identities, template semantics, all hashes and invariant TOC buckets.
$addonVerification=(& $addonVerifier -Receipt $addonActivation -LegendaryExplorerPath $addonTool -StarterConfigTemplate $addonTemplate|Out-String)|ConvertFrom-Json
if($addonVerification.Status -ne 'offline-verified-not-installed' -or !$addonVerification.StarterConfigVerified -or
   !$addonVerification.TOCHashBucketsVerified -or $addonVerification.Files -ne 4){throw 'Current authored starter activation required'}
$addonBase=Get-Content -LiteralPath $addonActivation -Raw|ConvertFrom-Json
if($addonBase.Variant -cne 'mount-toc-starter-config-english-tlk' -or $addonBase.ToolSHA256 -ne $addonInputs[$addonAssembly]){throw 'Activation/tool identity mismatch'}
foreach($addonFile in $addonBase.Files){$addonInput=Join-Path $addonBaseRoot $addonFile.RelativePath;$addonInputs[$addonInput]=$addonFile.SHA256}
$addonRoot=Join-Path $addonOutput 'DLC_MOD_ME3SquadCoop'
$addonCooked=Join-Path $addonRoot 'CookedPCConsole'
$addonStatus=[ordered]@{Status='preparing';Installed=$false;GameExecuted=$false;TestsRun=$false;EngineAccepted=$false;RuntimeInvoked=$false;
 ContainsOwnerGameAssets=$true;DistributionAllowed=$false;Variant='private-owner-revive-with-authored-starter';
 ActivationReceipt=$addonActivation;ActivationReceiptSHA256=$addonInputs[$addonActivation];CompileReceipt=$addonReceiptPath;
 CompileReceiptSHA256=$addonInputs[$addonReceiptPath];ScriptCompileReceipt=$addonScriptReceipt;ScriptCompileReceiptSHA256=$addonInputs[$addonScriptReceipt];
 PackageSHA256=$addonReceipt.PackageSHA256;Sources=$addonExpectedSources;InputsBefore=$addonInputs;InputsAfter=@{};InputsUnchanged=$false;
 ConfigBasePropertiesPreserved=$false;MountAndTlkPreserved=$false;TOCHashBucketsVerified=$false;Error=$null}
$null=New-Item -ItemType Directory -Path $addonCooked
try {
 foreach($addonFile in $addonBase.Files){Copy-Item -LiteralPath (Join-Path $addonBaseRoot $addonFile.RelativePath) -Destination (Join-Path $addonRoot $addonFile.RelativePath)}
 Copy-Item -LiteralPath $addonPackage -Destination (Join-Path $addonCooked 'ME3SquadCoopActions.pcc')
 $addonConfig=Join-Path $addonCooked 'Default_DLC_MOD_ME3SquadCoop.bin'
 $addonStream=[IO.File]::OpenRead($addonConfig)
 try {$addonAssets=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::DecompileGame3ToMemory($addonStream)} finally {$addonStream.Dispose()}
 $addonOriginal=[Collections.Generic.Dictionary[string,string]]::new()
 foreach($addonKey in $addonAssets.Keys){$addonOriginal.Add($addonKey,$addonAssets[$addonKey])}
 [xml]$addonEngine=$addonAssets['BioEngine.xml']
 $addonSections=$addonEngine.SelectSingleNode('/CoalesceAsset/Sections')
 $addonSeek=@($addonEngine.SelectNodes("//Section[@name='core.system']/Property[@name='seekfreepcpaths']/Value[@type='2']")|Where-Object {$_.InnerText -ceq '..\..\BIOGame\DLC\DLC_MOD_ME3SquadCoop\CookedPCConsole'})
 if(!$addonSections -or $addonSeek.Count -ne 1 -or $addonEngine.SelectNodes("//Section[@name='engine.startuppackages']").Count){throw 'Unexpected starter startup/seekfree configuration'}
 $addonStartup=$addonEngine.CreateElement('Section');$addonStartup.SetAttribute('name','engine.startuppackages')
 foreach($addonName in @('dlcstartuppackage','dlcstartuppackagename')){
  $addonProperty=$addonEngine.CreateElement('Property');$addonProperty.SetAttribute('name',$addonName)
  $addonProperty.SetAttribute('type',$(if($addonName -eq 'dlcstartuppackage'){'3'}else{'0'}))
  $addonProperty.InnerText='ME3SquadCoopActions';$null=$addonStartup.AppendChild($addonProperty)
 }
 $null=$addonSections.AppendChild($addonStartup);$addonAssets['BioEngine.xml']=$addonEngine.OuterXml
 $addonMemory=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::CompileFromMemory($addonAssets)
 try {[IO.File]::WriteAllBytes($addonConfig,$addonMemory.ToArray())} finally {$addonMemory.Dispose()}
 $addonStream=[IO.File]::OpenRead($addonConfig)
 try {$addonReadback=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::DecompileGame3ToMemory($addonStream)} finally {$addonStream.Dispose()}
 if($addonReadback.Count -ne 8 -or @(Compare-Object @($addonReadback.Keys) @($addonOriginal.Keys) -CaseSensitive).Count){throw 'Starter config asset set changed'}
 foreach($addonKey in $addonOriginal.Keys){
  [xml]$addonBeforeXml=$addonOriginal[$addonKey];[xml]$addonAfterXml=$addonReadback[$addonKey]
  if($addonKey -ceq 'BioEngine.xml'){
   $addonAdded=@($addonAfterXml.SelectNodes("//Section[@name='engine.startuppackages']"))
   if($addonAdded.Count -ne 1 -or @($addonAdded[0].SelectNodes('Property')).Count -ne 2){throw 'Startup config roundtrip mismatch'}
   foreach($addonName in @('dlcstartuppackage','dlcstartuppackagename')){
    $addonProperty=$addonAdded[0].SelectSingleNode("Property[@name='$addonName']")
    $addonType=if($addonName -eq 'dlcstartuppackage'){'3'}else{'0'}
    if(!$addonProperty -or $addonProperty.InnerText -cne 'ME3SquadCoopActions' -or $addonProperty.GetAttribute('type') -cne $addonType){throw 'Startup property mismatch'}
   }
   $null=$addonAdded[0].ParentNode.RemoveChild($addonAdded[0])
  }
  if($addonBeforeXml.OuterXml -cne $addonAfterXml.OuterXml){throw 'Authored base configuration changed'}
 }
 $addonStatus.ConfigBasePropertiesPreserved=$true
 foreach($addonName in @('Mount.dlc','DLC_MOD_ME3SquadCoop_INT.tlk')){
  $addonRelative='CookedPCConsole/'+$addonName
  $addonExpected=@($addonBase.Files|Where-Object {$_.RelativePath.Replace('\','/') -ceq $addonRelative})[0].SHA256
  if((Get-FileHash -LiteralPath (Join-Path $addonCooked $addonName) -Algorithm SHA256).Hash -ne $addonExpected){throw 'Working Mount/TLK changed'}
 }
 $addonStatus.MountAndTlkPreserved=$true
 $addonCulture=[Threading.Thread]::CurrentThread.CurrentCulture
 try {
  [Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
  $addonTocMemory=[LegendaryExplorerCore.Unreal.TOCCreator]::CreateDLCTOCForDirectory($addonRoot,[LegendaryExplorerCore.Packages.MEGame]::LE3)
 } finally {[Threading.Thread]::CurrentThread.CurrentCulture=$addonCulture}
 try {[IO.File]::WriteAllBytes((Join-Path $addonRoot 'PCConsoleTOC.bin'),$addonTocMemory.ToArray())} finally {$addonTocMemory.Dispose()}
 $addonToc=[LegendaryExplorerCore.Unreal.TOCBinFile]::new((Join-Path $addonRoot 'PCConsoleTOC.bin'))
 $addonExpected=@('CookedPCConsole/Mount.dlc','CookedPCConsole/DLC_MOD_ME3SquadCoop_INT.tlk','CookedPCConsole/Default_DLC_MOD_ME3SquadCoop.bin','CookedPCConsole/ME3SquadCoopActions.pcc')
 $addonEntries=@($addonToc.GetAllEntries())
 if($addonEntries.Count -ne 4){throw 'Unexpected TOC entry count'}
 foreach($addonRelative in $addonExpected){
  $addonMatches=@($addonEntries|Where-Object {$_.name.Replace('\','/') -ceq $addonRelative})
  if($addonMatches.Count -ne 1 -or $addonMatches[0].size -ne (Get-Item -LiteralPath (Join-Path $addonRoot $addonRelative)).Length){throw 'TOC entry size/path mismatch'}
 }
 for($addonBucket=0;$addonBucket -lt $addonToc.HashBuckets.Count;$addonBucket++){
  foreach($addonEntry in $addonToc.HashBuckets[$addonBucket].TOCEntries){
   $addonLeaf=[IO.Path]::GetFileName($addonEntry.name.Replace('/','\'))
   if([ME3ActivationTocHash]::Bucket($addonLeaf,$addonToc.HashBuckets.Count) -ne $addonBucket){throw 'TOC invariant hash bucket mismatch'}
  }
 }
 $addonStatus.TOCHashBucketsVerified=$true
 $addonFiles=@(Get-ChildItem -LiteralPath $addonRoot -Recurse -Force -File)
 $addonActual=@($addonFiles|ForEach-Object {[IO.Path]::GetRelativePath($addonRoot,$_.FullName).Replace('\','/')})
 if($addonFiles.Count -ne 5 -or @(Compare-Object $addonActual @($addonExpected+'PCConsoleTOC.bin') -CaseSensitive).Count){throw 'Unexpected output payload'}
 if((Get-FileHash -LiteralPath (Join-Path $addonCooked 'ME3SquadCoopActions.pcc') -Algorithm SHA256).Hash -ne $addonReceipt.PackageSHA256){throw 'PCC copy changed'}
 foreach($addonInput in $addonInputs.GetEnumerator()){
  $addonHash=(Get-FileHash -LiteralPath $addonInput.Key -Algorithm SHA256).Hash
  if($addonHash -ne $addonInput.Value){throw 'Input changed during preparation'}
  $addonStatus.InputsAfter[$addonInput.Key]=$addonHash
 }
 $addonStatus.InputsUnchanged=$true;$addonStatus.Status='prepared-private-owner-addon-not-installed'
 $addonStatus.TOCEntries=@($addonEntries|ForEach-Object {@{Name=$_.name;Size=$_.size}})
 $addonStatus.Files=@($addonFiles|ForEach-Object {@{RelativePath=[IO.Path]::GetRelativePath($addonRoot,$_.FullName);Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
} catch {$addonStatus.Status='failed';$addonStatus.Error=$_.Exception.Message;throw}
finally {$addonStatus|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $addonOutput 'preparation.json') -Encoding utf8}
[pscustomobject]$addonStatus|Select-Object Status,ContainsOwnerGameAssets,DistributionAllowed,InputsUnchanged,MountAndTlkPreserved,ConfigBasePropertiesPreserved,TOCHashBucketsVerified,Installed,EngineAccepted|ConvertTo-Json
