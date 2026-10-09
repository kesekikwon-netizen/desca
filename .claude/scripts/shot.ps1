# Purpose: scene-named screenshot of SectionViewer for the ui-visual-check skill
#          (used by commands.screenshot in .claude/cpp-project.json).
# Usage:   powershell -NoProfile -ExecutionPolicy Bypass -File .claude/scripts/shot.ps1 -Scene work|draw|start -Out <png> [-Size 1920x1040]
# Scenes:  work  = work screen with the standard synthetic section line   (--shot)
#          draw  = while drawing a section line, context bar visible      (--ctx-shot)
#          start = start screen without a file                            (--start-shot)
# Settings are isolated per run (--settings in a fresh folder) so the user's real settings stay untouched.
# Exit codes: 0 ok, 2 app not built, 3 capture failed (see the app's exit code in the message).
param(
    [Parameter(Mandatory = $true)][ValidateSet('work', 'draw', 'start')][string]$Scene,
    [Parameter(Mandatory = $true)][string]$Out,
    [string]$Size = '1920x1040',
    [string]$Exe = 'C:\dev\kerf-v4c\app\SectionViewer.exe',
    [string]$Model = 'C:\dev\kerf-synth\Synthetic.3mx',
    [string[]]$Line = @('200002', '450006', '200014', '450006')
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Exe)) { Write-Error "SectionViewer is not built: $Exe"; exit 2 }
if ($Scene -ne 'start' -and -not (Test-Path $Model)) { Write-Error "Synthetic model missing: $Model (make it with asec-make-synthetic.exe C:\dev\kerf-synth)"; exit 2 }

$Out = [IO.Path]::GetFullPath($Out)
$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir | Out-Null }

# ASCII-only settings folder: the app is tested with C:\dev\tmp; fall back to the user temp folder.
$base = if (Test-Path 'C:\dev\tmp') { 'C:\dev\tmp' } else { [IO.Path]::GetTempPath() }
$settings = Join-Path $base ('kerf-shot-' + [IO.Path]::GetRandomFileName().Replace('.', ''))

$common = @('--size', $Size, '--settings', $settings, '--quit')
switch ($Scene) {
    'start' { $argv = @('--start-shot', $Out) + $common }
    'work'  { $argv = @($Model, '--line') + $Line + @('--shot', $Out) + $common }
    'draw'  { $argv = @($Model, '--line') + $Line + @('--ctx-shot', $Out) + $common }
}
# PowerShell 5.1 does not quote array elements that contain spaces: quote them here.
$argLine = ($argv | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } }) -join ' '

$p = Start-Process -FilePath $Exe -ArgumentList $argLine -Wait -PassThru
$code = $p.ExitCode
Remove-Item $settings -Recurse -Force -ErrorAction SilentlyContinue

if ($code -ne 0 -or -not (Test-Path $Out)) { Write-Error "shot failed: scene=$Scene exit=$code out=$Out"; exit 3 }
Write-Output "shot ok: scene=$Scene size=$Size -> $Out"
exit 0
