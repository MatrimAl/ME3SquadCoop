# Record one explicit existing game window. Never launch a game or send input.
[CmdletBinding()]
param(
 [Parameter(Mandatory)][int]$GameProcessId,
 [Parameter(Mandatory)][uint64]$WindowHandle,
 [Parameter(Mandatory)][string]$SandboxRoot,
 [Parameter(Mandatory)][string]$FFmpegPath,
 [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$recordRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$recordPrivate=[IO.Path]::GetFullPath((Join-Path $recordRepo 'local'))+[IO.Path]::DirectorySeparatorChar
$recordOutput=[IO.Path]::GetFullPath($OutputDirectory)
if(!$recordOutput.StartsWith($recordPrivate,[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $recordOutput)){throw 'Use a fresh private output directory below repository local/'}
$recordAncestor=[IO.Path]::GetDirectoryName($recordOutput)
while($recordAncestor){
 if((Test-Path -LiteralPath $recordAncestor) -and ((Get-Item -LiteralPath $recordAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Output reparse ancestors forbidden'}
 $recordAncestor=[IO.Path]::GetDirectoryName($recordAncestor)
}
$recordSandbox=(Resolve-Path -LiteralPath $SandboxRoot).Path
if(!(Test-Path -LiteralPath (Join-Path $recordSandbox '.me3-coop-sandbox.json'))){throw 'Protected sandbox marker required; launch guards must be established separately'}
$recordExpected=[IO.Path]::GetFullPath((Join-Path $recordSandbox 'Binaries\Win64\MassEffect3.exe'))
$recordGame=Get-Process -Id $GameProcessId -ErrorAction Stop
if($recordGame.Path -ne $recordExpected){throw 'PID is not the explicitly selected sandbox executable'}
if((Get-FileHash -LiteralPath $recordExpected -Algorithm SHA256).Hash -ne 'B453871EB6EACFF0C646490FFBC588F743399A829B1F9E31CD7775DC95DA8001'){throw 'Unsupported game identity'}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ME3FeatureWindow {
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr handle, out uint processId);
 [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr handle);
 [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr handle, out Rect rect);
}
'@
$recordOwner=[uint32]0
$recordHwnd=[IntPtr]::new([long]$WindowHandle)
$null=[ME3FeatureWindow]::GetWindowThreadProcessId($recordHwnd,[ref]$recordOwner)
if(![ME3FeatureWindow]::IsWindow($recordHwnd) -or $recordOwner -ne $GameProcessId){throw 'Window does not belong to selected PID'}
$recordRect=[ME3FeatureWindow+Rect]::new()
if(![ME3FeatureWindow]::GetClientRect($recordHwnd,[ref]$recordRect) -or
   ($recordRect.Right-$recordRect.Left) -lt 640 -or ($recordRect.Bottom-$recordRect.Top) -lt 360){
 throw 'Game client area is not ready for recording; wait for the full game window'
}
$recordEncoder=(Resolve-Path -LiteralPath $FFmpegPath).Path
$null=New-Item -ItemType Directory -Path $recordOutput
$recordStart=[Diagnostics.ProcessStartInfo]::new()
$recordStart.FileName=$recordEncoder
$recordStart.UseShellExecute=$false
$recordStart.CreateNoWindow=$true
$recordStart.RedirectStandardInput=$true
$recordStart.WorkingDirectory=$recordOutput
$recordStart.Environment['FFREPORT']='file=encoder.log:level=32'
$recordArgs=@('-hide_banner','-report','-loglevel','error','-n','-f','lavfi','-i',
 "gfxcapture=hwnd=$WindowHandle`:capture_cursor=1:max_framerate=30:width=-2:height=-2,hwdownload,format=bgra",
 '-an','-c:v','libx264','-preset','veryfast','-crf','23','-pix_fmt','yuv420p',
 '-force_key_frames','expr:gte(t,n_forced*30)','-f','segment','-segment_time','30','-reset_timestamps','1','part-%05d.mkv')
foreach($recordArg in $recordArgs){$recordStart.ArgumentList.Add($recordArg)}
$recordInfo=[ordered]@{Status='starting';GamePID=$GameProcessId;WindowHandle=$WindowHandle;GamePath=$recordExpected;
 FFmpegPath=$recordEncoder;FFmpegSHA256=(Get-FileHash -LiteralPath $recordEncoder -Algorithm SHA256).Hash;
 StartUTC=[DateTime]::UtcNow.ToString('o');StartTimestamp=[Diagnostics.Stopwatch]::GetTimestamp();
 StopwatchFrequency=[Diagnostics.Stopwatch]::Frequency;Arguments=$recordArgs;Audio='none';
 RecordingVerified=$false;GameLaunched=$false;InputsSent=$false;StopFile='STOP';SegmentSeconds=30;
 ClientWidth=($recordRect.Right-$recordRect.Left);ClientHeight=($recordRect.Bottom-$recordRect.Top)}
$recordMeta=Join-Path $recordOutput 'recording.json'
$recordInfo|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $recordMeta -Encoding utf8
$recordProcess=[Diagnostics.Process]::Start($recordStart)
$recordInfo.EncoderPID=$recordProcess.Id
$recordInfo.Status='capture-requested'
$recordInfo|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $recordMeta -Encoding utf8
try {
 while(!$recordProcess.WaitForExit(250)){
  $recordOwner=[uint32]0
  $null=[ME3FeatureWindow]::GetWindowThreadProcessId($recordHwnd,[ref]$recordOwner)
  $recordGame.Refresh()
  if((Test-Path -LiteralPath (Join-Path $recordOutput 'STOP')) -or $recordGame.HasExited -or $recordOwner -ne $GameProcessId){
   try {$recordProcess.StandardInput.WriteLine('q')} catch {
    # The encoder can exit between WaitForExit and this write when its game
    # window disappears. Preserve other write failures while accepting that
    # observed terminal state and collecting its actual exit code below.
    if(!$recordProcess.HasExited){throw}
   }
   if(!$recordProcess.WaitForExit(20000)){$recordProcess.Kill();$recordProcess.WaitForExit()}
   break
  }
 }
 $recordInfo.Status='encoder-exited'
 $recordInfo.ExitCode=$recordProcess.ExitCode
} finally {
 if(!$recordProcess.HasExited){$recordProcess.Kill();$recordProcess.WaitForExit()}
 $recordInfo.FinishUTC=[DateTime]::UtcNow.ToString('o')
 $recordInfo.Segments=@(Get-ChildItem -LiteralPath $recordOutput -Filter 'part-*.mkv'|ForEach-Object {@{Name=$_.Name;Bytes=$_.Length}})
 $recordInfo|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $recordMeta -Encoding utf8
 $recordProcess.Dispose()
}
# Encoder exit, file presence and nonzero length are not decoded-frame proof.
$recordInfo|ConvertTo-Json -Depth 4
