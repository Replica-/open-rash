# Build the SDL3-backed ddraw.dll (src\platform\ddraw\rrddraw.c) -> analysis\build\sdl\ddraw.dll (+ SDL3.dll).
. "$PSScriptRoot\common.ps1"
$sdl = Get-RrSdl; $out = "$Build\sdl"; $src = "$Src\platform\ddraw"
Enter-RrVcVars
New-Item -ItemType Directory -Force $out | Out-Null; Push-Location $out
cl /nologo /O2 /W3 /LD "/I$sdl\include" "$src\rrddraw.c" "$src\wicjpeg.c" /Fe:ddraw.dll /link "/DEF:$src\rrddraw.def" "/LIBPATH:$sdl\lib\x86" SDL3.lib user32.lib gdi32.lib winmm.lib ole32.lib windowscodecs.lib uuid.lib mfplat.lib mfreadwrite.lib mfuuid.lib 2>&1 | Where-Object { $_ -match "error|fatal|LNK" }
Copy-Item "$sdl\lib\x86\SDL3.dll" $out -Force; Pop-Location; Get-Item "$out\ddraw.dll" | Select-Object Name,Length
