# run_cubase_harness.ps1: one Cubase check of this repo's plugin, start to finish, no hands (Claude, 2026-10-29).
# Generic form of Tertiary's plugin/tests/daw/cubase/tertiary_harness_run.ps1; the plugin name comes from config.cmake.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File plugin\tests\daw\cubase\run_cubase_harness.ps1
#        [-Vst3 <bundle>] [-MakeGolden] [-Tolerance 1e-4] [-NoInstall] [-Tools <folder>] [-Golden <folder>] [-WarnAfter <s>]
#
# Needs: "<name> harness\<name> harness.cpr" next to this script (make_cubase_harness.py parts, then
# assemble_cubase_harness.ps1), and the generic Cubase screen helpers cubase_windows.ps1, cubase_wait.ps1,
# cubase_menu.ps1 and screen.ps1 (Joe's pipeline tools; -Tools, else $env:WLA_CUBASE_TOOLS, else
# Dropbox\Joe's Music\_Production\_PIPELINE\tools).
#
# 1. Install: copies the built <name>.vst3 (or -Vst3) to the per-user VST3 folder (%LOCALAPPDATA%\Programs\Common\VST3,
#    which Cubase 12 scans; no admin). Only with Cubase closed: if Cubase is running the script stops and says so.
#    It never closes or kills Cubase itself.
# 2. Opens the harness by launching Cubase with its path.
# 3. File > Export > Audio Mixdown with the project's saved settings, Export Audio.
#    NOT YET WORKING (2026-10-29): the dialog opens, but the synthetic click on Export Audio does nothing (no progress
#    window, no file), in any project on the mini PC. See CUBASE-PLAYBOOK.md, "Tertiary harness run, and where it
#    stopped". Until that is solved every run ends STOPPED at this step.
# 4. Quits Cubase with Ctrl+Q.
# 5. Moves the export into Runs\<date-time>\harness.wav and compares it with the golden using
#    plugin\tests\regression\compare_renders.py. -MakeGolden stores this run's render as the golden instead (only after
#    listening to it). Goldens are per machine, so they live outside the repo:
#    %USERPROFILE%\.wla-test\goldens\<name>\cubase\harness.wav (-Golden to change).
#
# Screen-control rules (HQ agent/README.md): every step that can raise a window is followed by a check of the front
# window; an unknown window means a screenshot, nothing clicked, and exit 2. Save, don't-save, overwrite and install
# prompts are never answered. Exit 0 = pass, 1 = render differs, 2 = stopped on a window, 3 = could not start.
param([string]$Vst3 = '', [switch]$MakeGolden, [double]$Tolerance = 1e-4, [switch]$NoInstall, [string]$Compare = '',
      [string]$Tools = '', [string]$Golden = '', [int]$WarnAfter = 0)
$ErrorActionPreference = 'Continue'
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$cfg = Get-Content (Join-Path $Repo 'config.cmake') -Raw
if ($cfg -notmatch 'set\(\s*wPLUGIN_NAME\s+"([^"]+)"') { 'config.cmake has no wPLUGIN_NAME'; exit 3 }
$Name = $Matches[1]
$Proj = "$Name harness"
$TitleRx = '^Cubase Pro Project - ' + [regex]::Escape($Proj)
$WinRx = '"Cubase Pro Project - ' + [regex]::Escape($Proj) + '"'
if (-not $Tools) { $Tools = if ($env:WLA_CUBASE_TOOLS) { $env:WLA_CUBASE_TOOLS } else { Join-Path $env:USERPROFILE "Dropbox\Joe's Music\_Production\_PIPELINE\tools" } }
$T = $Tools
$H = Join-Path $PSScriptRoot $Proj
$Cpr = Join-Path $H "$Proj.cpr"
# Cubase 12 exports to the project's Mixdown folder; older settings used the project folder itself
$Exports = @((Join-Path $H "Mixdown\$Proj.wav"), (Join-Path $H "$Proj.wav"))
$Export = $Exports[0]
if (-not $Golden) { $Golden = Join-Path $env:USERPROFILE (".wla-test\goldens\{0}\cubase" -f $Name.ToLower()) }
$Cubase = 'C:\Program Files\Steinberg\Cubase 12\Cubase12.exe'
$UserVst3 = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
$stamp = Get-Date -Format 'yyyy-MM-dd-HHmmss'
if (-not (Test-Path -LiteralPath $Cpr)) { "no harness project at $Cpr (make_cubase_harness.py parts, then assemble_cubase_harness.ps1)"; exit 3 }
$Run = Join-Path $H "Runs\$stamp"
New-Item -ItemType Directory -Force $Run | Out-Null
$Log = Join-Path $Run 'run.log'
function Append($path, $text) { for ($k = 0; $k -lt 20; $k++) { try { [IO.File]::AppendAllText($path, $text + "`r`n"); return } catch { Start-Sleep -Milliseconds 250 } } }
function Say($m) { $line = "{0}  {1}" -f (Get-Date -Format 'HH:mm:ss'), $m; $line; Append $Log $line }
function Windows { & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\cubase_windows.ps1" }
function Front { (Windows) | Where-Object { $_ -match '^\*' } | Select-Object -First 1 }
function Shot($n) { $s = & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" shot "harness-$stamp-$n"; $p = ($s -replace '^shot ', ''); if (Test-Path $p) { Copy-Item $p (Join-Path $Run "$n.png") }; $p }
function Result($verdict, $detail) { Append (Join-Path $H 'Runs\results.log') ("{0}  {1,-7} {2}" -f $stamp, $verdict, $detail) }
function Stop-Run($m, $code = 2) { Say "STOP: $m"; Say ("screenshot: " + (Shot 'stop')); Say 'Nothing was clicked after this. Cubase is left as it is (never killed).'; Result 'STOPPED' $m; exit $code }

Say "$Proj run $stamp"
$need = @('cubase_windows.ps1', 'cubase_wait.ps1', 'cubase_menu.ps1', 'screen.ps1' | Where-Object { -not (Test-Path (Join-Path $T $_)) })
if ($need) { Say "Cubase helpers missing from ${T}: $($need -join ', ') (pass -Tools <folder>)"; Result 'STOPPED' 'helpers missing'; exit 3 }
if (-not (Test-Path -LiteralPath (Join-Path $H 'Audio\harness-input.wav'))) { Say "no Audio\harness-input.wav (not in git): run  python `"$PSScriptRoot\make_cubase_harness.py`" parts  first"; Result 'STOPPED' 'no input audio'; exit 3 }
$sess = (query session 2>$null) -join "`n"
if ($sess -notmatch '>?console\s+\S+\s+\d+\s+Active') { Say "this session is not on the console (Remote Desktop?): screen work would fail. $sess"; Result 'STOPPED' 'session not on console'; exit 3 }

# 1. Install (Cubase must be closed)
if (Get-Process Cubase12 -ErrorAction SilentlyContinue) { Say 'Cubase is running. Close it with Ctrl+Q first: the plug-in can only be replaced with Cubase closed.'; Result 'STOPPED' 'Cubase was open'; exit 3 }
$inner = "Contents\x86_64-win\$Name.vst3"
if (-not $NoInstall) {
  if (-not $Vst3) {
    $cands = @('build', 'build-tests' | ForEach-Object { Join-Path $Repo "$_\plugin\${Name}_artefacts\Release\VST3\$Name.vst3" } |
      Where-Object { Test-Path (Join-Path $_ $inner) } | Sort-Object { (Get-Item (Join-Path $_ $inner)).LastWriteTime } -Descending)
    if (-not $cands) { Say "no built $Name.vst3 in build\ or build-tests\ (or pass -Vst3)"; Result 'STOPPED' 'no build'; exit 3 }
    $Vst3 = $cands[0]
  }
  $bin = Join-Path $Vst3 $inner
  $hash = (Get-FileHash $bin -Algorithm SHA256).Hash.Substring(0, 16)
  $dest = Join-Path $UserVst3 "$Name.vst3"
  $old = if (Test-Path (Join-Path $dest $inner)) { (Get-FileHash (Join-Path $dest $inner) -Algorithm SHA256).Hash.Substring(0, 16) } else { 'none' }
  Say ("build: $Vst3 (built {0}, sha256 {1})" -f (Get-Item $bin).LastWriteTime.ToString('yyyy-MM-dd HH:mm'), $hash)
  if ($old -eq $hash) { Say 'per-user VST3 already this build' }
  else {
    New-Item -ItemType Directory -Force $UserVst3 | Out-Null
    & robocopy $Vst3 $dest /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { Say "robocopy failed ($LASTEXITCODE)"; Result 'STOPPED' 'install failed'; exit 3 }
    Say "installed to $dest (was $old)"
  }
} else {
  $bin = Join-Path $UserVst3 "$Name.vst3\$inner"
  $hash = (Get-FileHash $bin -Algorithm SHA256).Hash.Substring(0, 16)
  Say "no install; per-user VST3 sha256 $hash"
}
foreach ($e in $Exports) { if (Test-Path -LiteralPath $e) { Move-Item -LiteralPath $e (Join-Path $Run ('stale-' + (Split-Path $e -Leaf))); Say "moved a leftover export out of the way (no overwrite prompt): $e" } }

# 2. Open the harness
$t0 = Get-Date
Start-Process -FilePath $Cubase -ArgumentList ('"' + $Cpr + '"')
Say 'Cubase started with the harness'
$w = & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\cubase_wait.ps1" -Want $TitleRx -Timeout 180
$code = $LASTEXITCODE
$w | ForEach-Object { Say "  $_" }
if ($code -ne 0) { Stop-Run "the harness did not open cleanly (cubase_wait exit $code)" }
Start-Sleep -Seconds 2
$f = Front
if ($f -notmatch $WinRx) { Stop-Run "unexpected window in front after opening: $f" }
Say ("project open after {0:N0} s" -f ((Get-Date) - $t0).TotalSeconds)

# 3. Export Audio Mixdown with the saved settings (NOT YET WORKING: see the header)
& powershell -NoProfile -ExecutionPolicy Bypass -File "$T\cubase_menu.ps1" run 'File>Export>Audio Mixdown...' | ForEach-Object { Say "  $_" }
$dlg = $null
$okTitles = '^(Cubase Pro Project - ' + [regex]::Escape($Proj) + '|Cubase Pro|Uninitialized 12)$'
for ($i = 0; $i -lt 30 -and -not $dlg; $i++) {
  Start-Sleep -Milliseconds 500
  $all = Windows
  $hit = $all | Where-Object { $_ -match '"Export Audio Mixdown"\s+\S+\s+(-?\d+),(-?\d+)\s+(\d+)x(\d+)' } | Select-Object -First 1
  if ($hit) { [void]($hit -match '"Export Audio Mixdown"\s+\S+\s+(-?\d+),(-?\d+)\s+(\d+)x(\d+)'); $dlg = $Matches; break }
  $odd = $all | Where-Object { $_ -match '^\*?\s*"([^"]+)"' -and $Matches[1] -notmatch $okTitles } | Select-Object -First 1
  if ($odd) { Stop-Run "unexpected window instead of Export Audio Mixdown: $odd" }
}
if (-not $dlg) { Stop-Run 'Export Audio Mixdown did not open' }
$x = [int]$dlg[1] + 819; $y = [int]$dlg[2] + 852          # "Export Audio" button, bottom right of the 935x905 dialog
& powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" focus '^Export Audio Mixdown$' | Out-Null
$f = Front
if ($f -notmatch '"Export Audio Mixdown"') { Stop-Run "could not bring Export Audio Mixdown to the front (front: $f); not clicking" }
Shot 'export-dialog' | Out-Null
& powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" click $x $y | Out-Null
$tExport = ((Get-Date) - $t0).TotalSeconds
Say ("Export Audio pressed {0:N0} s after launch" -f $tExport)
if ($WarnAfter -gt 0 -and $tExport -gt $WarnAfter) { Say "WARNING: over $WarnAfter s from launch (an unlicensed plugin's demo cycle may fall inside the render)" }
$done = $false; $lastSize = -1
for ($i = 0; $i -lt 240 -and -not $done; $i++) {
  Start-Sleep -Milliseconds 500
  $f = Front
  if ($f -match $WinRx -or -not $f -or $f -match '^\*\s+"Cubase Pro"\s+SmtgMain') {
    $hitE = @($Exports | Where-Object { Test-Path -LiteralPath $_ })[0]; if ($hitE) { $Export = $hitE }
    if (Test-Path -LiteralPath $Export) { $s = (Get-Item -LiteralPath $Export).Length; if ($s -gt 0 -and $s -eq $lastSize) { $done = $true }; $lastSize = $s }
  } elseif ($f -match '"(Export Audio Mixdown|Perform Audio Export|Audio Export|Progress)"' -or $f -match '^\*\s+""\s') { }
  else { Stop-Run "unexpected window during the export: $f" }
}
if (-not $done) { Stop-Run 'the export did not finish within 2 minutes (known: the automated Export Audio click does nothing yet)' }
Say ("exported {0:N1} MB" -f ((Get-Item -LiteralPath $Export).Length / 1MB))

# 4. Quit with Ctrl+Q (never kill)
& powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" focus $TitleRx | Out-Null
if ((Front) -notmatch $WinRx) { Start-Sleep 1; & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" focus $TitleRx | Out-Null }
if ((Front) -match $WinRx) { & powershell -NoProfile -ExecutionPolicy Bypass -File "$T\screen.ps1" key '^q' | Out-Null }
else { Say "WARNING: could not bring the project to the front for Ctrl+Q (front: $(Front))" }
$quit = $false
for ($i = 0; $i -lt 60; $i++) { Start-Sleep -Seconds 1; if (-not (Get-Process Cubase12 -ErrorAction SilentlyContinue)) { $quit = $true; break }; $f = Front; if ($f -and $f -notmatch $WinRx -and $f -notmatch 'SmtgMain') { break } }
$quitNote = ''
if ($quit) { Say 'Cubase closed' } else { $quitNote = "Cubase did not close; front window: $(Front)"; Say "WARNING: $quitNote (left open, nothing clicked)"; Say ("screenshot: " + (Shot 'quit')) }

# 5. Compare with the golden
Move-Item -LiteralPath $Export (Join-Path $Run 'harness.wav')
if (-not $Compare) { $Compare = Join-Path $Repo 'plugin\tests\regression\compare_renders.py' }
$gold = Join-Path $Golden 'harness.wav'
if ($MakeGolden) {
  New-Item -ItemType Directory -Force $Golden | Out-Null
  # an old golden goes into replaced\: compare_renders.py checks every WAV in the golden folder itself
  if (Test-Path -LiteralPath $gold) { New-Item -ItemType Directory -Force (Join-Path $Golden 'replaced') | Out-Null; Move-Item -LiteralPath $gold (Join-Path $Golden "replaced\harness.replaced-$stamp.wav") }
  Copy-Item (Join-Path $Run 'harness.wav') $gold
  Set-Content -LiteralPath (Join-Path $Golden 'golden-info.txt') -Value "Golden made $stamp from run $Run`r`nbuild sha256 $hash`r`nCubase Pro 12, $env:COMPUTERNAME, Export Audio Mixdown"
  Say "stored as the golden: $gold"
  Result 'GOLDEN' "build $hash; export at $([int]$tExport) s $quitNote"
  exit 0
}
if (-not (Test-Path -LiteralPath $gold)) { Say 'no golden yet: listen to this run, then make it the golden with -MakeGolden'; Result 'NOGOLD' "build $hash"; exit 1 }
if (-not (Test-Path -LiteralPath $Compare)) { Say "no compare_renders.py at $Compare"; Result 'STOPPED' 'no compare script'; exit 3 }
$out = & python $Compare $Golden $Run --tolerance $Tolerance --report (Join-Path $Run 'compare.txt') 2>&1
$cmp = $LASTEXITCODE
$out | ForEach-Object { Say "  $_" }
if ($cmp -eq 0) { Say 'PASS'; Result 'PASS' "build $hash; $(($out | Select-Object -Last 1)) $quitNote"; if ($quitNote) { exit 2 }; exit 0 }
Say 'FAIL: the render differs from the golden'; Result 'FAIL' "build $hash; $(($out | Select-Object -Last 1)) $quitNote"; exit 1
