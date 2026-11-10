# Run the LATEST build (analysis\build\OpenRash.exe + build\sdl\ddraw.dll, SDL3.dll, dsound.dll) as a normal window with the real data folder as working directory (data\ holds everything: the game files, VIDEO_UP / VIDEO, AUDIO\Music songs and the mod\widescreen art mod). Not for tests (they use rrtest.ps1).
# -Renderer gpu|soft = force the renderer for this run (RR_RENDERER; default: Video Options); -World3D on|off = force the true-3D world of GPU races (RR_WORLD3D; default: Video Options)
# -Cop "<tick>,<dist>" = police test (RR_COP); -Cash <n> = a fresh Big Game starts with $n (RR_CASH); -RacePct <1..99> = races end after that % of their length (RR_RACE_PCT);
# -Track <1..5> = straight into a race on that track, no menus (RR_AUTORACE; 1 City 2 Sierra 3 Peninsula 4 Napa 5 Pacific Highway); -SkipTo <segment> = fast-forward that
# race to that road segment (RR_SKIP_TO; the "SEG" of the position overlay), then normal speed; -Bike <0..14> = the player's bike in that race (RR_BIKE:
# 0..4 Rat Banzai / Corsair / Kamikaze / Perro / Killer, 5..9 Sport Diablo / Kamikaze / Stiletto / DMG / Perro, 10..14 Super Banzai / Diablo / Stiletto / Corsair / Kamikaze);
# -Fork A|B = the player takes the left (A) / right (B) carriageway at every split road (RR_FORK);
# -NoFork = forks off: every split road is just its left road, every rider takes it (RR_NOFORK, = Video Options > Forks = Off).
# -Demo [-Speed <percent>] = demo: every track at level 5 in turn, the bike riding by itself; Up / Down = game speed 0 (paused) .. x8 (default x3; RR_DEMO_ALL);
#   Q = quit, terminating every other running copy of the game too (OpenRash.exe / the tests' RoadRash.exe).
# -ShowPos = the segment tracker on for this run (RR_SHOW_POS; else Video Options > Segment tracker, default off).
# -Build = build first (build_quick.ps1).  The game gets a clean environment: no RR_* switch of an earlier launch (the PowerShell extension reuses one session) or of a test.
param([switch]$Wait,[string]$Cop="",[int]$Cash=0,[int]$RacePct=0,[int]$Track=0,[int]$SkipTo=0,[int]$Bike=-1,[ValidateSet("","A","B")][string]$Fork="",[switch]$NoFork,[switch]$Build,[ValidateSet("","gpu","soft")][string]$Renderer="",[ValidateSet("","on","off")][string]$World3D="",[switch]$Demo,[int]$Speed=300,[switch]$ShowPos)
$root=(Resolve-Path "$PSScriptRoot\..").Path; $a="$root\analysis"; $play="$a\build\play"; $data="$root\data"
if($Build){ & "$PSScriptRoot\build_quick.ps1"; if(-not $?){ "build failed - not starting"; exit 1 } }
if(-not (Test-Path "$a\build\OpenRash.exe")){ "no build: analysis\build\OpenRash.exe is missing (run the Build entry)"; exit 1 }
New-Item -ItemType Directory -Force $play | Out-Null
Get-Process OpenRash -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$play*" } | Stop-Process -Force
Start-Sleep -Milliseconds 300
Copy-Item "$a\build\OpenRash.exe" "$play\OpenRash.exe" -Force
if(Test-Path "$a\build\OpenRash.map"){ Copy-Item "$a\build\OpenRash.map" "$play\OpenRash.map" -Force }
foreach($f in "ddraw.dll","SDL3.dll","dsound.dll"){ if(Test-Path "$a\build\sdl\$f"){ Copy-Item "$a\build\sdl\$f" $play -Force } else { "missing $a\build\sdl\$f (run the Build entry)" } }
$psi = New-Object Diagnostics.ProcessStartInfo "$play\OpenRash.exe"
$psi.WorkingDirectory = $data; $psi.UseShellExecute = $false
$e = $psi.EnvironmentVariables
foreach($k in @($e.Keys)){ if($k -like "RR_*"){ $e.Remove($k) } }
$e["RR_SHOT_DIR"] = "$root\screenshots"   # P = screenshot, saved here and opened in the image viewer (rrddraw.c ShotKey)
if($ShowPos){ $e["RR_SHOW_POS"] = "1" }   # course position top left in races, whatever Video Options > Segment tracker says (rrddraw.c ShowRoadPos)
if($Cop){ $e["RR_COP"] = $Cop }
if($Cash -gt 0){ $e["RR_CASH"] = "$Cash" }
if($RacePct -gt 0){ $e["RR_RACE_PCT"] = "$RacePct" }
if($Track -ge 1 -and $Track -le 5){ $e["RR_AUTORACE"] = "$Track" }
if($SkipTo -gt 0){ $e["RR_SKIP_TO"] = "$SkipTo" }
if($Bike -ge 0 -and $Bike -le 14){ $e["RR_BIKE"] = "$Bike" }
if($NoFork){ $e["RR_NOFORK"] = "1" }   # forks off: every split is its left road only (world3d.inc, race/physics.c)
if($Fork){ $e["RR_FORK"] = $Fork }   # the player takes carriageway A (left) / B (right) at every split (race/physics.c)
if($Renderer){ $e["RR_RENDERER"] = $Renderer }
if($World3D){ $e["RR_WORLD3D"] = $(if($World3D -eq "on"){ "1" } else { "0" }) }
if($Demo){   # -Demo [-Speed <percent>]: every track at level 5 in turn, round and round, the player's bike riding by itself (always wins, no police); Up / Down = game speed
    $e["RR_DEMO_ALL"] = "1"; $e["RR_SPEED"] = "$Speed"; $e["RR_LEVEL"] = "5"; $e["RR_FORCE_WIN"] = "1"; $e["RR_NOCOPS"] = "1"
    if(-not $e.ContainsKey("RR_AUTORACE")){ $e["RR_AUTORACE"] = "1" } }
$p = [Diagnostics.Process]::Start($psi)
"started $play\OpenRash.exe (pid $($p.Id))" + $(if($Renderer){ ", renderer $Renderer" } else { "" }) + $(if($World3D){ ", 3D world $World3D" } else { "" })
if($Wait){ $p.WaitForExit() }
