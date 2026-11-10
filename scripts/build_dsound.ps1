# Build the SDL3-backed dsound.dll (src\platform\dsound\rrdsound.c) -> analysis\build\sdl\dsound.dll
. "$PSScriptRoot\common.ps1"
$sdl = Get-RrSdl; $out = "$Build\sdl"; $src = "$Src\platform\dsound"
Enter-RrVcVars
New-Item -ItemType Directory -Force $out | Out-Null; Push-Location $out
cl /nologo /Zi /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /LD "/I$sdl\include" "$src\rrdsound.c" /Fe:dsound.dll /link /DEBUG /OPT:REF /OPT:ICF "/DEF:$src\rrdsound.def" "/LIBPATH:$sdl\lib\x86" SDL3.lib winmm.lib user32.lib ole32.lib mfplat.lib mfreadwrite.lib mfuuid.lib delayimp.lib "/DELAYLOAD:mfplat.dll" "/DELAYLOAD:mfreadwrite.dll" 2>&1 | Select-String -Pattern "error|warning C4(013|047|133)" | Select-Object -First 12
Pop-Location; Get-Item "$out\dsound.dll" -ea 0 | Select-Object Name,Length
