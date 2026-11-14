# Full rebuild: the SDL3 platform DLLs the exe loads (rrddraw.dll, rrdsound.dll - first: the game links against their import libraries), then the game (build.ps1: C -> link -> resources).
# Output: analysis\build\OpenRash.exe + analysis\build\sdl\rrddraw.dll, rrdsound.dll, SDL3.dll.   usage: build_quick.ps1 [-NoShim]
param([switch]$NoShim,[switch]$Regen)   # -Regen: kept for old command lines, does nothing
. "$PSScriptRoot\common.ps1"
if(-not $NoShim){ & "$PSScriptRoot\build_ddraw.ps1"; & "$PSScriptRoot\build_dsound.ps1" }
& "$PSScriptRoot\build.ps1"
"built: " + (Get-Item "$Build\OpenRash.exe").Length + " bytes, " + (Get-Item "$Build\OpenRash.exe").LastWriteTime
