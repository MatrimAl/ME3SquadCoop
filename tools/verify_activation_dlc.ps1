# Read-only semantic verification of private activation candidates. No install/launch.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$Receipt,
 [Parameter(Mandatory)][string]$LegendaryExplorerPath,
 [string]$StarterConfigTemplate
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'PowerShell7 required'}
$verifyRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$verifyPrivate=[IO.Path]::GetFullPath((Join-Path $verifyRepo 'local'))+[IO.Path]::DirectorySeparatorChar
$verifyReceipt=[IO.Path]::GetFullPath($Receipt)
if(!$verifyReceipt.StartsWith($verifyPrivate,[StringComparison]::OrdinalIgnoreCase)){throw 'Private repository receipt required'}
function AssertActivationPlainPath([string]$Path){
 $verifyWalk=[IO.Path]::GetFullPath($Path)
 while($verifyWalk){
  if((Get-Item -LiteralPath $verifyWalk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse target or ancestry forbidden'}
  $verifyWalk=[IO.Path]::GetDirectoryName($verifyWalk)
 }
}
AssertActivationPlainPath $verifyReceipt
$verifyBefore=(Get-FileHash -LiteralPath $verifyReceipt -Algorithm SHA256).Hash
$verifyData=Get-Content -LiteralPath $verifyReceipt -Raw|ConvertFrom-Json
if($verifyData.Status -ne 'prepared-not-installed' -or $verifyData.Installed -ne $false -or
   $verifyData.EngineAccepted -ne $false -or $verifyData.GameExecuted -ne $false -or
   $verifyData.ContainsOwnerGameAssets -ne $false){throw 'Unusable preparation receipt'}
$verifyStarter=$verifyData.Variant -eq 'mount-toc-starter-config-english-tlk'
$verifyConfig=$verifyData.Variant -in @('mount-toc-minimal-config','mount-toc-config-english-tlk','mount-toc-starter-config-english-tlk')
$verifyEnglish=$verifyData.Variant -in @('mount-toc-config-english-tlk','mount-toc-starter-config-english-tlk')
if($verifyStarter -and !$StarterConfigTemplate){throw 'Explicit pinned starter template required'}
if(!$verifyStarter -and $StarterConfigTemplate){throw 'Starter template applies only to its declared research variant'}
if(!$verifyConfig -and $verifyData.Variant -ne 'mount-toc'){throw 'Unsupported activation variant'}
$verifyID=if($verifyEnglish){1915100}else{0}
if($verifyData.ResearchStringID -ne $verifyID -or $verifyData.StringIDAllocationVerified -ne $false){throw 'Unexpected research string identity'}
$verifyExpected=@('PCConsoleTOC.bin','CookedPCConsole/Mount.dlc')
if($verifyConfig){$verifyExpected+='CookedPCConsole/Default_DLC_MOD_ME3SquadCoop.bin'}
if($verifyEnglish){$verifyExpected+='CookedPCConsole/DLC_MOD_ME3SquadCoop_INT.tlk'}
$verifyRoot=Join-Path ([IO.Path]::GetDirectoryName($verifyReceipt)) 'DLC_MOD_ME3SquadCoop'
AssertActivationPlainPath $verifyRoot
$verifyFiles=@(Get-ChildItem -LiteralPath $verifyRoot -Recurse -Force -File)
foreach($verifyFile in (Get-ChildItem -LiteralPath $verifyRoot -Recurse -Force)){AssertActivationPlainPath $verifyFile.FullName}
$verifyActual=@($verifyFiles|ForEach-Object {[IO.Path]::GetRelativePath($verifyRoot,$_.FullName).Replace('\','/')})
if($verifyFiles.Count -ne $verifyExpected.Count -or @(Compare-Object $verifyActual $verifyExpected -CaseSensitive).Count){throw 'Unexpected activation file set'}
if(@($verifyData.Files).Count -ne $verifyExpected.Count){throw 'Receipt file count mismatch'}
$verifyHashes=@{}
foreach($verifyRelative in $verifyExpected){
 $verifyMatches=@($verifyData.Files|Where-Object {$_.RelativePath.Replace('\','/') -ceq $verifyRelative})
 if($verifyMatches.Count -ne 1){throw 'Missing or duplicate receipt entry'}
 $verifyFile=Join-Path $verifyRoot $verifyRelative
 $verifyDigest=(Get-FileHash -LiteralPath $verifyFile -Algorithm SHA256).Hash
 if($verifyMatches[0].Bytes -ne (Get-Item -LiteralPath $verifyFile).Length -or $verifyDigest -ne $verifyMatches[0].SHA256){throw 'Payload hash or size mismatch'}
 $verifyHashes[$verifyRelative]=$verifyDigest
}
$verifyMount=[IO.File]::ReadAllBytes((Join-Path $verifyRoot 'CookedPCConsole/Mount.dlc'))
if($verifyMount.Length -ne 108){throw 'Mount length mismatch'}
$verifyWords=@(1,0x2ad,0xcd,0x3006b,5100,0,0x0c,$verifyID,$verifyID)
for($verifyIndex=0;$verifyIndex -lt 27;$verifyIndex++){
 $verifyWant=if($verifyIndex -lt 9){$verifyWords[$verifyIndex]}else{0}
 if([BitConverter]::ToUInt32($verifyMount,$verifyIndex*4) -ne $verifyWant){throw 'Mount contract mismatch'}
}
$verifyTool=(Resolve-Path -LiteralPath $LegendaryExplorerPath).Path
$verifyAssembly=Join-Path $verifyTool 'LegendaryExplorerCore.dll'
$verifyToolHash=(Get-FileHash -LiteralPath $verifyAssembly -Algorithm SHA256).Hash
if($verifyToolHash -ne $verifyData.ToolSHA256){throw 'Preparation tool identity mismatch'}
Add-Type -TypeDefinition @"
using System;using System.IO;using System.Reflection;
public static class ME3ActivationVerificationResolver {
 public static void Install(string folder) {
  AppDomain.CurrentDomain.AssemblyResolve += (sender,args) => {
   string path=Path.Combine(folder,new AssemblyName(args.Name).Name+".dll");
   return File.Exists(path)?Assembly.LoadFrom(path):null;
  };
 }
}
public static class ME3ActivationTocHash {
 public static int Bucket(string name,int count) {
  uint[] table=new uint[256];
  unchecked {
   for(uint i=0;i<256;i++) {
    uint crc=i<<24;
    for(int bit=0;bit<8;bit++)crc=(crc<<1)^((crc&0x80000000)!=0?0x04c11db7u:0u);
    table[i]=crc;
   }
   uint hash=0;
   foreach(char c in name.ToUpperInvariant()) {
    if(c>127)throw new InvalidDataException("ASCII activation filename required");
    hash=(hash>>8)^table[(hash^(byte)c)&255];
    hash=(hash>>8)^table[hash&255];
   }
   return (int)(hash%(uint)count);
  }
 }
}
"@
[ME3ActivationVerificationResolver]::Install($verifyTool)
$null=[Reflection.Assembly]::LoadFrom($verifyAssembly)
$verifyToc=[LegendaryExplorerCore.Unreal.TOCBinFile]::new((Join-Path $verifyRoot 'PCConsoleTOC.bin'))
$verifyEntries=@($verifyToc.GetAllEntries())
if($verifyEntries.Count -ne $verifyExpected.Count-1){throw 'TOC unexpected entry count'}
for($verifyBucketIndex=0;$verifyBucketIndex -lt $verifyToc.HashBuckets.Count;$verifyBucketIndex++){
 foreach($verifyBucketEntry in $verifyToc.HashBuckets[$verifyBucketIndex].TOCEntries){
  $verifyLeaf=[IO.Path]::GetFileName($verifyBucketEntry.name.Replace('/','\'))
  if([ME3ActivationTocHash]::Bucket($verifyLeaf,$verifyToc.HashBuckets.Count) -ne $verifyBucketIndex){throw 'TOC invariant filename hash bucket mismatch'}
 }
}
foreach($verifyRelative in ($verifyExpected|Where-Object {$_ -ne 'PCConsoleTOC.bin'})){
 $verifyMatches=@($verifyEntries|Where-Object {$_.name.Replace('\','/') -ceq $verifyRelative})
 if($verifyMatches.Count -ne 1 -or $verifyMatches[0].size -ne (Get-Item -LiteralPath (Join-Path $verifyRoot $verifyRelative)).Length){throw 'TOC content entry mismatch'}
}
if($verifyConfig){
 $verifyStream=[IO.File]::OpenRead((Join-Path $verifyRoot 'CookedPCConsole/Default_DLC_MOD_ME3SquadCoop.bin'))
 try {$verifyAssets=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::DecompileGame3ToMemory($verifyStream)} finally {$verifyStream.Dispose()}
 if($verifyStarter){
  AssertActivationPlainPath $StarterConfigTemplate
  $verifyTemplateHash=(Get-FileHash -LiteralPath $StarterConfigTemplate -Algorithm SHA256).Hash
  if($verifyTemplateHash -ne 'BF830CA939A25A4C2E256B86C6201431420EC7695FA4B0C160F011939202B879' -or
     $verifyData.TemplateSHA256 -ne $verifyTemplateHash){throw 'Unexpected starter template identity'}
  $verifyStream=[IO.File]::OpenRead($StarterConfigTemplate)
  try {$verifyTemplateAssets=[LegendaryExplorerCore.Coalesced.CoalescedConverter]::DecompileGame3ToMemory($verifyStream)} finally {$verifyStream.Dispose()}
  $verifyKeys=@('BioAI.xml','BioCredits.xml','BioDifficulty.xml','BioEngine.xml','BioGame.xml','BioInput.xml','BioUI.xml','BioWeapon.xml')
  if($verifyAssets.Count -ne 8 -or $verifyTemplateAssets.Count -ne 8 -or
     @(Compare-Object @($verifyAssets.Keys) $verifyKeys -CaseSensitive).Count){throw 'Unexpected starter config assets'}
  foreach($verifyKey in $verifyKeys){
   $verifyExpectedText=$verifyTemplateAssets[$verifyKey]
   if($verifyKey -ceq 'BioEngine.xml'){$verifyExpectedText=$verifyExpectedText.Replace('StarterKit','ME3SquadCoop')}
   [xml]$verifyExpectedXml=$verifyExpectedText;[xml]$verifyActualXml=$verifyAssets[$verifyKey]
   if($verifyExpectedXml.OuterXml -cne $verifyActualXml.OuterXml){throw 'Starter config semantic mismatch'}
  }
  if((Get-FileHash -LiteralPath $StarterConfigTemplate -Algorithm SHA256).Hash -ne $verifyTemplateHash){throw 'Starter template changed during verification'}
 }else{
 if($verifyAssets.Count -ne 1 -or !$verifyAssets.ContainsKey('BioEngine.xml')){throw 'Unexpected config assets'}
 [xml]$verifyXml=$verifyAssets['BioEngine.xml']
 $verifyProps=@($verifyXml.SelectNodes('//Property'))
 if($verifyProps.Count -ne 1 -or $verifyProps[0].GetAttribute('name') -cne 'seekfreepcpaths' -or
    $verifyProps[0].GetAttribute('type') -ne '2' -or $verifyProps[0].ParentNode.GetAttribute('name') -cne 'core.system' -or
    $verifyProps[0].InnerText -cne '..\..\BIOGame\DLC\DLC_MOD_ME3SquadCoop\CookedPCConsole'){throw 'Config semantic mismatch'}
 }
}
if($verifyEnglish){
 $verifyTlk=[LegendaryExplorerCore.TLK.ME2ME3.ME2ME3TalkFile]::new((Join-Path $verifyRoot 'CookedPCConsole/DLC_MOD_ME3SquadCoop_INT.tlk'))
 $verifyTexts=@('ME3 Squad Co-op','DLC_MOD_ME3SquadCoop','en-us','Male')
 for($verifyIndex=0;$verifyIndex -lt 4;$verifyIndex++){
  if($verifyTlk.FindDataById(($verifyID+$verifyIndex),$false,$true,$true,$true) -cne $verifyTexts[$verifyIndex]){throw 'TLK metadata mismatch'}
 }
 if($verifyTlk.FindDataById(($verifyID+3),$false,$true,$true,$false) -cne 'Female'){throw 'TLK female metadata mismatch'}
}
foreach($verifyRelative in $verifyExpected){
 if((Get-FileHash -LiteralPath (Join-Path $verifyRoot $verifyRelative) -Algorithm SHA256).Hash -ne $verifyHashes[$verifyRelative]){throw 'Payload changed during verification'}
}
if((Get-FileHash -LiteralPath $verifyReceipt -Algorithm SHA256).Hash -ne $verifyBefore -or
   (Get-FileHash -LiteralPath $verifyAssembly -Algorithm SHA256).Hash -ne $verifyToolHash){throw 'Receipt or tool changed during verification'}
@{Status='offline-verified-not-installed';Variant=$verifyData.Variant;Files=$verifyExpected.Count;
 ReceiptSHA256=$verifyBefore;MountContractVerified=$true;TOCVerified=$true;TOCHashBucketsVerified=$true;ConfigVerified=$verifyConfig;
 EnglishMetadataVerified=$verifyEnglish;StarterConfigVerified=$verifyStarter;Installed=$false;EngineAccepted=$false;GameExecuted=$false}|ConvertTo-Json
