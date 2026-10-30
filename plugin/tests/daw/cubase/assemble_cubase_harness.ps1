# assemble_cubase_harness.ps1: turn the parts from make_cubase_harness.py into "<name> harness.cpr" in Cubase (Claude,
# 2026-10-29). The steps are CUBASE-PLAYBOOK.md's "Tertiary harness project, built from files" recipe, which built
# Tertiary's harness on 2026-10-29 one step at a time.
#
# STATUS: NOT YET RUN as one script. The pixel positions are the mini PC's (1920x1080, Cubase 12 Pro, its window
# layout). Run it with someone watching the first time, and fix the positions here if they moved.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File plugin\tests\daw\cubase\assemble_cubase_harness.ps1 [-Tools <folder>]
#
# Before: the plugin's VST3 is installed where Cubase scans it (run_cubase_harness.ps1 -NoInstall does not install;
# run it once without -NoInstall, or copy the bundle to %LOCALAPPDATA%\Programs\Common\VST3), and Cubase is closed.
# Every step checks the front window; anything unknown stops with a screenshot. Never answers save or overwrite
# prompts; never kills Cubase.
param([string]$Tools = '')
$ErrorActionPreference = 'Continue'
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$cfg = Get-Content (Join-Path $Repo 'config.cmake') -Raw
if ($cfg -notmatch 'set\(\s*wPLUGIN_NAME\s+"([^"]+)"') { 'config.cmake has no wPLUGIN_NAME'; exit 3 }
$Name = $Matches[1]; $Proj = "$Name harness"
if (-not $Tools) { $Tools = if ($env:WLA_CUBASE_TOOLS) { $env:WLA_CUBASE_TOOLS } else { Join-Path $env:USERPROFILE "Dropbox\Joe's Music\_Production\_PIPELINE\tools" } }
$T = $Tools
$H = Join-Path $PSScriptRoot $Proj
$Cpr = Join-Path $H "$Proj.cpr"
$Cubase = 'C:\Program Files\Steinberg\Cubase 12\Cubase12.exe'
function PS1($script) { & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\$script" @args }
function Screen { & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" @args }
function Front { (PS1 'cubase_windows.ps1') | Where-Object { $_ -match '^\*' } | Select-Object -First 1 }
function Stop-Here($m) { "STOP: $m"; Screen shot "assemble-stop"; 'Nothing clicked after this; Cubase left as it is.'; exit 2 }
function Expect($rx, $what) { Start-Sleep 1; $f = Front; if ($f -notmatch $rx) { Stop-Here "expected $what, front is: $f" } }

foreach ($p in 'Audio\harness-input.wav', 'Misc\harness-tempo.smt', 'Misc\harness-track.xml') {
  if (-not (Test-Path -LiteralPath (Join-Path $H $p))) { "missing ${p}: run  python make_cubase_harness.py parts  first"; exit 3 }
}
if (Test-Path -LiteralPath $Cpr) { "$Cpr exists already; move it away first (this script never overwrites)"; exit 3 }
if (Get-Process Cubase12 -ErrorAction SilentlyContinue) { 'Cubase is running; close it with Ctrl+Q first'; exit 3 }

# 1. Cubase with the Hub; File > New Project; Create Empty; Set Project Folder = the harness folder
Start-Process -FilePath $Cubase
$w = PS1 'cubase_wait.ps1' -Want '^(Steinberg Hub|Cubase Pro)$' -Timeout 180; $w
PS1 'cubase_menu.ps1' run 'File>New Project'
Screen wait 2 click 1321 821 wait 2                        # Hub: Create Empty
Expect '"Set Project Folder"' 'Set Project Folder'
Screen click 556 472 type $H key '{ENTER}' key '^a' key '{DEL}' button 'Select Folder' wait 3
Expect '"Cubase Pro Project - ' 'the new empty project'

# 2. Tempo track, then the track archive with its automation
PS1 'cubase_import.ps1' tempo (Join-Path $H 'Misc\harness-tempo.smt')
Expect '"Cubase Pro Project - ' 'the project after the tempo import'
PS1 'cubase_import.ps1' archive (Join-Path $H 'Misc\harness-track.xml')
Expect '"Import Options"' 'Import Options'
Screen click 503 172 click 971 237 wait 1                  # Select All; Automation (off by default; must be on)
Screen shot "assemble-import-options"
Screen button 'OK' wait 3
Expect '"Cubase Pro Project - ' 'the project after the track import'

# 3. Read automation on; locators bar 1 to 13; cycle on
Screen click 404 228                                       # the track's R button (check the screenshot: green)
PS1 'cubase_menu.ps1' run 'Transport>Locators>Enter Right Locator Position'
Screen type '13.1.1.0' key '{ENTER}'
PS1 'cubase_menu.ps1' run 'Transport>Transport Commands>Cycle'
Screen shot "assemble-before-save"

# 4. Save As the harness project
PS1 'cubase_import.ps1' saveas $Cpr
Start-Sleep 3
if (-not (Test-Path -LiteralPath $Cpr)) { Stop-Here 'the .cpr was not written' }
"saved $Cpr. Check the screenshots in %TEMP%\cubase-screens (assemble-*), then quit Cubase with Ctrl+Q."
