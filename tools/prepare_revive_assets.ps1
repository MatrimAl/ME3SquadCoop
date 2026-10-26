# Derive a PRIVATE owner-asset package. Never install or redistribute it.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$CompileReceipt,
 [Parameter(Mandatory)][string]$SourcePackage,
 [Parameter(Mandatory)][string]$GamePath,
 [Parameter(Mandatory)][string]$LegendaryExplorerPath,
 [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'PowerShell7 required'}
$assetRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$assetPrivate=[IO.Path]::GetFullPath((Join-Path $assetRepo 'local'))+[IO.Path]::DirectorySeparatorChar
$assetOutput=[IO.Path]::GetFullPath($OutputDirectory)
if(![IO.Path]::IsPathFullyQualified($OutputDirectory) -or !$assetOutput.StartsWith($assetPrivate,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $assetOutput)){throw 'Fresh absolute output below repository local/ required'}
$assetAncestor=[IO.Path]::GetDirectoryName($assetOutput)
while($assetAncestor){
 if((Test-Path -LiteralPath $assetAncestor) -and ((Get-Item -LiteralPath $assetAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Output reparse ancestors forbidden'}
 $assetAncestor=[IO.Path]::GetDirectoryName($assetAncestor)
}
$assetReceiptPath=(Resolve-Path -LiteralPath $CompileReceipt).Path
if(!$assetReceiptPath.StartsWith($assetPrivate,[StringComparison]::OrdinalIgnoreCase)){throw 'Private compile receipt required'}
$assetReceipt=Get-Content -LiteralPath $assetReceiptPath -Raw|ConvertFrom-Json
if($assetReceipt.Status -ne 'compiled-not-installed' -or $assetReceipt.InputsUnchanged -ne $true -or !$assetReceipt.Classes.Count){throw 'Successful unmodified compile receipt required'}
foreach($assetClass in $assetReceipt.Classes){if($assetClass.HasErrors -or $assetClass.Errors.Count -or $assetClass.Warnings.Count){throw 'Unreviewed compiler diagnostics'}}
$assetOriginal=(Resolve-Path -LiteralPath $assetReceipt.Package).Path
if(!$assetOriginal.StartsWith($assetPrivate,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($assetOriginal) -ne 'ME3SquadCoopActions.pcc' -or (Get-FileHash -LiteralPath $assetOriginal).Hash -ne $assetReceipt.PackageSHA256){throw 'Unexpected authored script package'}
$assetSource=(Resolve-Path -LiteralPath $SourcePackage).Path
$assetGame=(Resolve-Path -LiteralPath $GamePath).Path
$assetTool=(Resolve-Path -LiteralPath $LegendaryExplorerPath).Path
$assetInputs=@{}
foreach($assetInput in $assetReceipt.InputsAfter.PSObject.Properties){
 if((Get-FileHash -LiteralPath $assetInput.Name).Hash -ne $assetInput.Value){throw 'Compilation input changed'}
 $assetInputs[$assetInput.Name]=$assetInput.Value
}
foreach($assetFile in @($assetOriginal,$assetReceiptPath,$assetSource,$PSCommandPath,(Join-Path $assetTool 'LegendaryExplorerCore.dll'))){$assetInputs[$assetFile]=(Get-FileHash -LiteralPath $assetFile).Hash}
if(!('ME3ReviveAssetResolver' -as [type])){
 Add-Type -TypeDefinition @'
using System;using System.IO;using System.Reflection;
public static class ME3ReviveAssetResolver {
 public static void Install(string folder) {
  AppDomain.CurrentDomain.AssemblyResolve += (sender,args) => {
   string path=Path.Combine(folder,new AssemblyName(args.Name).Name+".dll");
   return File.Exists(path)?Assembly.LoadFrom(path):null;
  };
 }
}
'@
}
[ME3ReviveAssetResolver]::Install($assetTool)
$null=[Reflection.Assembly]::LoadFrom((Join-Path $assetTool 'LegendaryExplorerCore.dll'))
[LegendaryExplorerCore.LegendaryExplorerCoreLib]::InitLib([Threading.Tasks.TaskScheduler]::Default,$null,$null,[LegendaryExplorerCore.Packages.MEGame[]]@([LegendaryExplorerCore.Packages.MEGame]::LE3),$false)
[LegendaryExplorerCore.Compression.OodleHelper]::LoadOodleDll((Join-Path $assetGame 'Binaries\Win64\oo2core_8_win64.dll'))
function Get-AssetBytesHash([byte[]]$Bytes){[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes))}
function Get-AssetProperty($Entry,[string]$Name){$Entry.GetProperties()|Where-Object {$_.Name.Name -eq $Name}}
$null=New-Item -ItemType Directory -Path $assetOutput
$assetPackagePath=Join-Path $assetOutput 'ME3SquadCoopActions.pcc'
Copy-Item -LiteralPath $assetOriginal -Destination $assetPackagePath
$assetStatus=[ordered]@{Status='preparing';GameExecuted=$false;TestsRun=$false;Installed=$false;
 ContainsOwnerGameAssets=$true;DistributionAllowed=$false;Package=$assetPackagePath;PackageSHA256=$null;
 Sources=$assetReceipt.Sources;Classes=$assetReceipt.Classes;ScriptCompileReceipt=$assetReceiptPath;
 ScriptPackageSHA256=$assetReceipt.PackageSHA256;InputsBefore=$assetInputs;InputsAfter=@{};InputsUnchanged=$false;
 ScriptExecutableExportsPreserved=$false;FactoryAnchorInjected=$false;CompressedSequencePreserved=$false;
 RelinkErrors=@();AssetContract=$null;Error=$null}
$assetSrc=$null;$assetDst=$null;$assetReadback=$null
try {
 $assetSrc=[LegendaryExplorerCore.Packages.MEPackageHandler]::OpenMEPackage($assetSource)
 $assetDst=[LegendaryExplorerCore.Packages.MEPackageHandler]::OpenMEPackage($assetPackagePath)
 if($assetSrc.Game -ne [LegendaryExplorerCore.Packages.MEGame]::LE3){throw 'LE3 source required'}
 $assetSets=@($assetSrc.Exports|Where-Object InstancedFullPath -eq 'BioChar_MPPlayers.Archetypes.Soldier.HumanFemale_Soldier.MY_DYN_HMM_DG_Deaths')
 $assetSequences=@($assetSrc.Exports|Where-Object InstancedFullPath -eq 'BIOG_HMM_DG_A.HMM_DG_Deaths_CB_MP_Revive')
 if($assetSets.Count -ne 1 -or $assetSequences.Count -ne 1 -or $assetSets[0].ClassName -ne 'BioDynamicAnimSet' -or $assetSequences[0].ClassName -ne 'AnimSequence'){throw 'Reviewed source asset identities not found'}
 $assetSet=$assetSets[0];$assetSequence=$assetSequences[0]
 $assetSequenceName=Get-AssetProperty $assetSequence 'SequenceName'
 $assetLength=Get-AssetProperty $assetSequence 'SequenceLength'
 $assetFrames=Get-AssetProperty $assetSequence 'NumFrames'
 if($assetSequenceName.Value.Name -ne 'CB_MP_Revive' -or ![float]::IsFinite($assetLength.Value) -or $assetLength.Value -le 0 -or $assetFrames.Value -le 0){throw 'Invalid source sequence metadata'}
 $assetSourceBinary=[LegendaryExplorerCore.Unreal.BinaryConverters.ObjectBinary]::From($assetSequence,$null,$null)
 if(!$assetSourceBinary.CompressedAnimationData.Length){throw 'No embedded compressed animation'}
 $assetCompressedHash=Get-AssetBytesHash $assetSourceBinary.CompressedAnimationData
 $assetSourceBoneData=$assetSrc.GetEntry((Get-AssetProperty $assetSequence 'm_pBioAnimSetData').Value)
 $assetBoneNames=@(Get-AssetProperty $assetSourceBoneData 'TrackBoneNames'|ForEach-Object {$_.Value.ToString()})
 $assetTranslationNames=@(Get-AssetProperty $assetSourceBoneData 'UseTranslationBoneNames'|ForEach-Object {$_.Value.ToString()})
 if(!$assetBoneNames.Count -or $assetSourceBoneData.ClassName -ne 'BioAnimSetData'){throw 'Missing bone-track metadata'}
 $assetProps=$assetSet.GetProperties()
 $assetArray=$assetProps|Where-Object {$_.Name.Name -eq 'Sequences'}
 $assetSelected=@($assetArray|Where-Object {$_.Value -eq $assetSequence.UIndex})
 $assetBinary=[LegendaryExplorerCore.Unreal.BinaryConverters.ObjectBinary]::From($assetSet,$null,$null)
 $assetMapping=@($assetBinary.SequenceNamesToUnkMap|Where-Object {$_.Key.Name -eq 'CB_MP_Revive'})
 if($assetSelected.Count -ne 1 -or $assetMapping.Count -ne 1){throw 'Ambiguous source sequence references'}
 # In-memory source edits only. SourcePackage is never a save destination.
 $assetArray.Clear();$assetArray.Add($assetSelected[0])
 $assetBinary.SequenceNamesToUnkMap.Clear();$assetBinary.SequenceNamesToUnkMap.Add($assetMapping[0].Key,$assetMapping[0].Value)
 $assetSet.WritePropertiesAndBinary($assetProps,$assetBinary)
 $assetOriginalExportCount=$assetDst.Exports.Count
 $assetOriginalExportHashes=@($assetDst.Exports|ForEach-Object {Get-AssetBytesHash $_.Data})
 $assetOriginalNames=@($assetDst.Names)
 $assetOriginalImports=@($assetDst.Imports|ForEach-Object {$_.ClassName+'|'+$_.InstancedFullPath})
 $assetFactoryDefaults=@($assetDst.Exports|Where-Object {$_.ObjectName.Name -eq 'Default__SFXSquadCoop_ReviveFactory'})
 if($assetFactoryDefaults.Count -ne 1 -or $assetFactoryDefaults[0].ClassName -ne 'SFXSquadCoop_ReviveFactory' -or $assetFactoryDefaults[0].GetProperties().Count){throw 'Expected sparse factory default before anchor injection'}
 $assetFactoryDefaultIndex=$assetFactoryDefaults[0].UIndex
 $assetOptions=[LegendaryExplorerCore.Packages.CloningImportingAndRelinking.RelinkerOptionsPackage]::new()
 $assetOptions.GamePathOverride=$assetGame;$assetOptions.ImportExportDependencies=$true
 $assetOptions.GenerateImportsForGlobalFiles=$true;$assetOptions.PortExportsAsImportsWhenPossible=$false
 $assetPorted=$null
 $assetReport=[LegendaryExplorerCore.Packages.CloningImportingAndRelinking.EntryImporter]::ImportAndRelinkEntries([LegendaryExplorerCore.Packages.CloningImportingAndRelinking.EntryImporter+PortingOption]::CloneAllDependencies,$assetSet,$assetDst,$null,$true,$assetOptions,[ref]$assetPorted)
 $assetStatus.RelinkErrors=@($assetReport|ForEach-Object {$_.ToString()})+@($assetOptions.RelinkReport|ForEach-Object {$_.ToString()})
 if($assetStatus.RelinkErrors.Count){throw 'Relinker reported unresolved work'}
 $assetAdded=@($assetDst.Exports|Where-Object {$_.UIndex -gt $assetOriginalExportCount})
 $assetAllowed=@('BioDynamicAnimSet','AnimSequence','BioAnimSetData','Package','SFXAnimNotify_RaceWwiseEvent','AnimNotify_WwiseEvent','WwiseEvent')
 if(!$assetPorted -or $assetAdded.Count -gt 32 -or @($assetAdded|Where-Object {$_.ClassName -notin $assetAllowed}).Count -or @($assetAdded|Where-Object ClassName -eq 'AnimSequence').Count -ne 1 -or @($assetAdded|Where-Object ClassName -eq 'BioDynamicAnimSet').Count -ne 1 -or @($assetAdded|Where-Object ClassName -eq 'BioAnimSetData').Count -ne 1){throw 'Unexpected dependency graph; no pawn/class/world port is allowed'}
 $assetPorted.idxLink=0;$assetPorted.ObjectName=[LegendaryExplorerCore.Unreal.NameReference]::new('ReviveAnimSet',0)
 $assetContainer=@($assetAdded|Where-Object {$_.ClassName -eq 'Package' -and $_.ObjectName.Name -eq 'BIOG_HMM_DG_A'})
 if($assetContainer.Count -ne 1){throw 'Expected isolated animation container'}
 $assetContainer[0].ObjectName=[LegendaryExplorerCore.Unreal.NameReference]::new('ReviveAnimationData',0)
 # Preserve the global Foley import paths; copied event remains a direct
 # reference, while its bank/streams are external owner-install dependencies.
 for($assetIndex=0;$assetIndex -lt $assetOriginalExportCount;$assetIndex++){
  if((Get-AssetBytesHash $assetDst.Exports[$assetIndex].Data) -ne $assetOriginalExportHashes[$assetIndex]){throw 'Existing script export changed'}
 }
 for($assetIndex=0;$assetIndex -lt $assetOriginalNames.Count;$assetIndex++){
  if($assetDst.Names[$assetIndex] -cne $assetOriginalNames[$assetIndex]){throw 'Original name indices changed'}
 }
 $assetAnchor=[LegendaryExplorerCore.Unreal.ObjectProperty]::new($assetPorted.UIndex,[LegendaryExplorerCore.Unreal.NameReference]::new('ReviveAnimation',0))
 $assetAnchorProps=$assetFactoryDefaults[0].GetProperties()
 $assetAnchorProps.AddOrReplaceProp($assetAnchor)
 $assetFactoryDefaults[0].WriteProperties($assetAnchorProps)
 [LegendaryExplorerCore.Packages.PackageSaver]::Save($assetDst,$assetPackagePath,$false,$false,$false,$null)
 $assetDst.Dispose();$assetDst=$null
 $assetReadback=[LegendaryExplorerCore.Packages.MEPackageHandler]::OpenMEPackage($assetPackagePath)
 for($assetIndex=0;$assetIndex -lt $assetOriginalExportCount;$assetIndex++){
  if($assetIndex+1 -ne $assetFactoryDefaultIndex -and (Get-AssetBytesHash $assetReadback.Exports[$assetIndex].Data) -ne $assetOriginalExportHashes[$assetIndex]){throw 'Saved script export changed outside explicit factory anchor'}
 }
 for($assetIndex=0;$assetIndex -lt $assetOriginalNames.Count;$assetIndex++){
  if($assetReadback.Names[$assetIndex] -cne $assetOriginalNames[$assetIndex]){throw 'Saved original name indices changed'}
 }
 for($assetIndex=0;$assetIndex -lt $assetOriginalImports.Count;$assetIndex++){
  if(($assetReadback.Imports[$assetIndex].ClassName+'|'+$assetReadback.Imports[$assetIndex].InstancedFullPath) -cne $assetOriginalImports[$assetIndex]){throw 'Saved original imports changed'}
 }
 $assetSavedSet=@($assetReadback.Exports|Where-Object InstancedFullPath -eq 'ReviveAnimSet')
 if($assetSavedSet.Count -ne 1 -or $assetSavedSet[0].ClassName -ne 'BioDynamicAnimSet'){throw 'Saved set identity mismatch'}
 $assetSavedAnchorProps=$assetReadback.GetEntry($assetFactoryDefaultIndex).GetProperties()
 if($assetSavedAnchorProps.Count -ne 1 -or $assetSavedAnchorProps[0].Name.Name -ne 'ReviveAnimation' -or $assetSavedAnchorProps[0] -isnot [LegendaryExplorerCore.Unreal.ObjectProperty] -or $assetSavedAnchorProps[0].Value -ne $assetSavedSet[0].UIndex){throw 'Saved factory anchor mismatch'}
 $assetSavedRefs=@(Get-AssetProperty $assetSavedSet[0] 'Sequences')
 if($assetSavedRefs.Count -ne 1){throw 'Saved set must contain one sequence'}
 $assetSavedSequence=$assetReadback.GetEntry($assetSavedRefs[0].Value)
 $assetSavedBinary=[LegendaryExplorerCore.Unreal.BinaryConverters.ObjectBinary]::From($assetSavedSequence,$null,$null)
 $assetSavedMap=[LegendaryExplorerCore.Unreal.BinaryConverters.ObjectBinary]::From($assetSavedSet[0],$null,$null)
 $assetSavedMappings=@($assetSavedMap.SequenceNamesToUnkMap)
 if((Get-AssetProperty $assetSavedSequence 'SequenceName').Value.Name -ne 'CB_MP_Revive' -or $assetSavedMappings.Count -ne 1 -or $assetSavedMappings[0].Key.Name -ne 'CB_MP_Revive' -or $assetSavedMappings[0].Value -ne $assetMapping[0].Value -or (Get-AssetBytesHash $assetSavedBinary.CompressedAnimationData) -ne $assetCompressedHash){throw 'Saved sequence/mapping changed'}
 $assetSavedBoneRef=(Get-AssetProperty $assetSavedSequence 'm_pBioAnimSetData').Value
 $assetSavedBoneData=$assetReadback.GetEntry($assetSavedBoneRef)
 $assetSavedBoneNames=@(Get-AssetProperty $assetSavedBoneData 'TrackBoneNames'|ForEach-Object {$_.Value.ToString()})
 $assetSavedTranslationNames=@(Get-AssetProperty $assetSavedBoneData 'UseTranslationBoneNames'|ForEach-Object {$_.Value.ToString()})
 if((Get-AssetProperty $assetSavedSet[0] 'm_pBioAnimSetData').Value -ne $assetSavedBoneRef -or $assetSavedBoneData.ClassName -ne 'BioAnimSetData' -or ($assetSavedBoneNames -join '|') -cne ($assetBoneNames -join '|') -or ($assetSavedTranslationNames -join '|') -cne ($assetTranslationNames -join '|') -or (Get-AssetProperty $assetSavedSequence 'SequenceLength').Value -ne $assetLength.Value -or (Get-AssetProperty $assetSavedSequence 'NumFrames').Value -ne $assetFrames.Value){throw 'Saved bone/sequence metadata changed'}
 $assetStatus.ScriptExecutableExportsPreserved=$true;$assetStatus.FactoryAnchorInjected=$true;$assetStatus.CompressedSequencePreserved=$true
 $assetStatus.AssetContract=@{AnimSet='ME3SquadCoopActions.ReviveAnimSet';SequenceName='CB_MP_Revive';SequenceCount=1;
  SequenceLength=$assetLength.Value;NumFrames=$assetFrames.Value;CompressedSHA256=$assetCompressedHash;
  AddedExports=@($assetAdded|ForEach-Object {@{Class=$_.ClassName;Path=$_.InstancedFullPath}});
  ExternalAudioDependencies=@($assetReadback.Imports|Where-Object {$_.InstancedFullPath -like 'Wwise_Generic_Foley.*'}|ForEach-Object {$_.InstancedFullPath});
  PlaybackVerified=$false;SkeletonCompatibilityVerified=$false;RuntimeLoaded=$false}
 $assetStatus.PackageSHA256=(Get-FileHash -LiteralPath $assetPackagePath).Hash
 $assetStatus.Status='compiled-with-owner-assets-not-installed'
} catch {$assetStatus.Status='failed';$assetStatus.Error=$_.Exception.Message;throw}
finally {
 foreach($assetOpen in @($assetSrc,$assetDst,$assetReadback)){if($null -ne $assetOpen){$assetOpen.Dispose()}}
 foreach($assetInputPath in $assetInputs.Keys){$assetStatus.InputsAfter[$assetInputPath]=(Get-FileHash -LiteralPath $assetInputPath).Hash}
 $assetStatus.InputsUnchanged=@($assetInputs.Keys|Where-Object {$assetInputs[$_] -ne $assetStatus.InputsAfter[$_]}).Count -eq 0
 if(!$assetStatus.InputsUnchanged){$assetStatus.Status='input-integrity-failure';$assetStatus.Error='Input changed during private derivation'}
 $assetStatus|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $assetOutput 'asset-result.json') -Encoding utf8
 if(!$assetStatus.InputsUnchanged){throw 'Input digest changed; derived output unusable'}
}
[pscustomobject]$assetStatus|Select-Object Status,InputsUnchanged,ScriptExecutableExportsPreserved,FactoryAnchorInjected,CompressedSequencePreserved,DistributionAllowed,PackageSHA256|ConvertTo-Json
