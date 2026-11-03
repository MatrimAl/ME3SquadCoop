# Compile owner-authored classes into a fresh private LE3 package. Never install.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$GamePath,
    [Parameter(Mandatory)][string]$LegendaryExplorerPath,
    [Parameter(Mandatory)][string[]]$Sources,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z][A-Za-z0-9_]*$')][string]$PackageName,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if($PSVersionTable.PSVersion.Major -lt 7){throw 'Use PowerShell7 for the supplied Legendary Explorer runtime.'}
$scriptRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$scriptPrivate=[IO.Path]::GetFullPath((Join-Path $scriptRepo 'local'))+[IO.Path]::DirectorySeparatorChar
if(![IO.Path]::IsPathFullyQualified($OutputDirectory)){throw 'Use an absolute output directory.'}
$scriptOutput=[IO.Path]::GetFullPath($OutputDirectory)
if(!$scriptOutput.StartsWith($scriptPrivate,[StringComparison]::OrdinalIgnoreCase)){throw 'Output must be a new directory below this repository local/.'}
if(Test-Path -LiteralPath $scriptOutput){throw 'Output directory already exists; choose a fresh build directory.'}
# A lexical local/ prefix must not follow a junction into an installed game.
$scriptAncestor=[IO.Path]::GetDirectoryName($scriptOutput)
while($scriptAncestor){
    if(Test-Path -LiteralPath $scriptAncestor){
        $scriptAttributes=(Get-Item -LiteralPath $scriptAncestor -Force).Attributes
        if($scriptAttributes -band [IO.FileAttributes]::ReparsePoint){throw 'Output ancestors must not be junctions or symbolic links.'}
    }
    $scriptParent=[IO.Path]::GetDirectoryName($scriptAncestor)
    if($scriptParent -eq $scriptAncestor){break}
    $scriptAncestor=$scriptParent
}
$scriptGame=(Resolve-Path -LiteralPath $GamePath).Path
$scriptTool=(Resolve-Path -LiteralPath $LegendaryExplorerPath).Path
$scriptAssembly=Join-Path $scriptTool 'LegendaryExplorerCore.dll'
$scriptOodle=Join-Path $scriptGame 'Binaries\Win64\oo2core_8_win64.dll'
$scriptInputs=@($scriptAssembly,$scriptOodle)
foreach($scriptFile in @('Core.pcc','Engine.pcc','SFXGame.pcc')){
    $scriptInputs+=Join-Path $scriptGame ('BioGame\CookedPCConsole\'+$scriptFile)
}
$scriptSourcePaths=@($Sources|ForEach-Object {(Resolve-Path -LiteralPath $_).Path})
if(!$scriptSourcePaths.Count){throw 'At least one authored .uc source is required.'}
if(@($scriptSourcePaths|Sort-Object -Unique).Count -ne $scriptSourcePaths.Count){throw 'Duplicate source paths.'}
foreach($scriptSource in $scriptSourcePaths){
    if([IO.Path]::GetExtension($scriptSource) -ne '.uc'){throw 'Sources must be authored .uc files.'}
}
$scriptInputs+=$scriptSourcePaths
$scriptHashes=@{}
foreach($scriptInput in $scriptInputs){$scriptHashes[$scriptInput]=(Get-FileHash -LiteralPath $scriptInput -Algorithm SHA256).Hash}

# Resolve bundled dependencies without changing PATH, registering an app or
# downloading anything. Tool/game locations are caller-selected and retained
# only in the private receipt.
if(!('ME3ScriptAssemblyResolver' -as [type])){
    Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Reflection;
public static class ME3ScriptAssemblyResolver {
    public static void Install(string folder) {
        AppDomain.CurrentDomain.AssemblyResolve += (sender,args) => {
            string path=Path.Combine(folder,new AssemblyName(args.Name).Name+".dll");
            return File.Exists(path)?Assembly.LoadFrom(path):null;
        };
    }
}
'@
}
[ME3ScriptAssemblyResolver]::Install($scriptTool)
$null=[Reflection.Assembly]::LoadFrom($scriptAssembly)
[LegendaryExplorerCore.LegendaryExplorerCoreLib]::InitLib([Threading.Tasks.TaskScheduler]::Default,$null,$null,[LegendaryExplorerCore.Packages.MEGame[]]@([LegendaryExplorerCore.Packages.MEGame]::LE3),$false)
[LegendaryExplorerCore.Compression.OodleHelper]::LoadOodleDll($scriptOodle)
$null=New-Item -ItemType Directory -Path $scriptOutput
$scriptPackagePath=Join-Path $scriptOutput ($PackageName+'.pcc')
$scriptReceiptPath=Join-Path $scriptOutput 'compile-result.json'
$scriptPackage=$null
$scriptLibrary=$null
$scriptReceipt=[ordered]@{Status='preparing';GameExecuted=$false;TestsRun=$false;Installed=$false;Package=$scriptPackagePath;Sources=$scriptSourcePaths;InputsBefore=$scriptHashes;Classes=@();Error=$null}
try {
    $scriptPackage=[LegendaryExplorerCore.Packages.MEPackageHandler]::CreateAndOpenPackage($scriptPackagePath,[LegendaryExplorerCore.Packages.MEGame]::LE3,$false)
    $scriptOptions=[LegendaryExplorerCore.UnrealScript.UnrealScriptOptionsPackage]::new()
    $scriptOptions.GamePathOverride=$scriptGame
    $scriptLibrary=[LegendaryExplorerCore.UnrealScript.FileLib]::new($scriptPackage,$false)
    if(!$scriptLibrary.Initialize($scriptOptions,$false)){throw 'Script dependency initialization failed.'}
    $scriptReceipt.Status='compiling'
    foreach($scriptSource in $scriptSourcePaths){
        $scriptText=[IO.File]::ReadAllText($scriptSource)
        $scriptClassName=[IO.Path]::GetFileNameWithoutExtension($scriptSource)
        $scriptResult=[LegendaryExplorerCore.UnrealScript.UnrealScriptCompiler]::CompileClass($scriptPackage,$scriptText,$scriptLibrary,$scriptOptions,$null,$null,$null,$scriptClassName)
        $scriptLog=$scriptResult.Item2
        if($null -eq $scriptLog){throw 'Compiler returned no diagnostics.'}
        $scriptReceipt.Classes+=@{Source=$scriptSource;HasErrors=$scriptLog.HasErrors;Errors=@($scriptLog.AllErrors|ForEach-Object {$_.ToString()});Warnings=@($scriptLog.AllWarnings|ForEach-Object {$_.ToString()})}
        if($scriptLog.HasErrors -or $null -eq $scriptResult.Item1){throw ('Class compilation failed: '+[IO.Path]::GetFileName($scriptSource))}
    }
    foreach($scriptInput in $scriptInputs){
        if((Get-FileHash -LiteralPath $scriptInput -Algorithm SHA256).Hash -ne $scriptHashes[$scriptInput]){
            throw 'An input changed before save; refusing package output.'
        }
    }
    # No save occurs for a partially compiled source set. The only save target
    # is the fresh package in local/, never a basegame or installed DLC path.
    [LegendaryExplorerCore.Packages.PackageSaver]::Save($scriptPackage,$scriptPackagePath,$false,$false,$false,$null)
    if(!(Test-Path -LiteralPath $scriptPackagePath)){throw 'Compiler save produced no package.'}
    $scriptReceipt.PackageSHA256=(Get-FileHash -LiteralPath $scriptPackagePath -Algorithm SHA256).Hash
    $scriptReceipt.Status='compiled-not-installed'
} catch {
    $scriptReceipt.Status='failed'
    $scriptReceipt.Error=$_.Exception.Message
    throw
} finally {
    if($scriptLibrary -is [IDisposable]){$scriptLibrary.Dispose()}
    if($null -ne $scriptPackage){$scriptPackage.Dispose()}
    $scriptAfter=@{}
    foreach($scriptInput in $scriptInputs){$scriptAfter[$scriptInput]=(Get-FileHash -LiteralPath $scriptInput -Algorithm SHA256).Hash}
    $scriptReceipt.InputsAfter=$scriptAfter
    $scriptChanged=@($scriptInputs|Where-Object {$scriptHashes[$_] -ne $scriptAfter[$_]})
    $scriptReceipt.InputsUnchanged=$scriptChanged.Count -eq 0
    if($scriptChanged.Count){$scriptReceipt.Status='input-integrity-failure';$scriptReceipt.Error='An input digest changed during compilation.'}
    $scriptReceipt|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $scriptReceiptPath -Encoding utf8
    if($scriptChanged.Count){throw 'Input digest changed; inspect private receipt.'}
}
Write-Output ('Compiled private package: '+$scriptPackagePath)
Write-Output 'Engine acceptance, loading, replication and gameplay remain unverified.'
