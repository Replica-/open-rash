# BUILD the game from src\ alone (C + src\resources); no game files are needed to build.
# Steps: build_src.ps1 (RR_NOGAME=1, -> analysis\build\ng) -> link.ps1. Output: analysis\build\OpenRash.exe
param([switch]$Regen)   # kept for old command lines; there is nothing to regenerate any more
. "$PSScriptRoot\common.ps1"
Get-Process OpenRash -ErrorAction SilentlyContinue | Stop-Process -Force
$env:RR_NOGAME="1"; & "$PSScriptRoot\build_src.ps1"
$env:RR_NOGAME=$null
& "$PSScriptRoot\link.ps1"
if(Test-Path "$Build\OpenRash.exe"){ "built: " + (Get-Item "$Build\OpenRash.exe").Length + " bytes" }
