# Build the SDL3-backed rrddraw.dll (src\platform\ddraw\rrddraw.c) -> analysis\build\sdl\rrddraw.dll (+ SDL3.dll); the game imports DirectDrawCreate from it (scripts\link.ps1).
. "$PSScriptRoot\common.ps1"
$sdl = Get-RrSdl; $out = "$Build\sdl"; $src = "$Src\platform\ddraw"
Enter-RrVcVars
New-Item -ItemType Directory -Force $out | Out-Null; Push-Location $out
cl /nologo /O2 /W3 /LD "/I$sdl\include" "$src\rrddraw.c" "$src\jpeg.c" /Fe:rrddraw.dll /link "/DEF:$src\rrddraw.def" "/LIBPATH:$sdl\lib\x86" SDL3.lib user32.lib gdi32.lib winmm.lib ole32.lib uuid.lib mfplat.lib mfreadwrite.lib mfuuid.lib 2>&1 | Where-Object { $_ -match "error|fatal|LNK" }
Copy-Item "$sdl\lib\x86\SDL3.dll" $out -Force; Pop-Location; Get-Item "$out\rrddraw.dll" | Select-Object Name,Length
