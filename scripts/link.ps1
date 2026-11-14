# Link the C objects in analysis\build\ng + the resources (src\resources, via make_res.ps1) + the Windows SDK import libs.
# Output: analysis\build\OpenRash.exe (+ .map, .pdb).
. "$PSScriptRoot\common.ps1"
Enter-RrVcVars
$vc = (Resolve-Path "$VsBuildTools\VC\Tools\MSVC\*\bin\Hostx64\x86").Path
$libs = "kernel32","user32","gdi32","advapi32","comctl32","winmm","wsock32","msacm32","tapi32" | ForEach-Object { "$WinSdkLib\$_.lib" }
# DirectDrawCreate / DirectSoundCreate from OUR shims, rrddraw.dll / rrdsound.dll: names with no built-in copy in Windows or Wine / Proton, so the game
# always loads them - no DLL override (the Steam Deck had silently used Wine's own ddraw.dll).  Their import libraries are the ones the shim builds write
# (build\sdl\rrddraw.lib / rrdsound.lib: they map the game's _DirectDrawCreate@12 to the exported name); missing -> the shims are built first
foreach ($d in "ddraw", "dsound") {
    if (-not (Test-Path "$Build\sdl\rr$d.lib")) { & "$PSScriptRoot\build_$d.ps1" | Out-Null }
    $libs += "$Build\sdl\rr$d.lib" }
$res = "$Build\resources.res"
& "$PSScriptRoot\make_res.ps1" -Out $res | Out-Null
$objs = Get-ChildItem "$Build\ng" -Filter *.obj -ErrorAction SilentlyContinue | ForEach-Object FullName
Remove-Item "$Build\OpenRash.pdb" -ErrorAction SilentlyContinue
$args2 = @("/nologo","/MACHINE:X86","/SUBSYSTEM:WINDOWS","/ENTRY:rr_entry","/BASE:0x400000","/FIXED","/NODEFAULTLIB","/MAP:$Build\OpenRash.map","/DEBUG","/PDB:$Build\OpenRash.pdb","/PDBALTPATH:OpenRash.pdb","/OPT:REF","/OPT:ICF","/OUT:$Build\OpenRash.exe") + $objs + $libs + @($res)
($args2 | ForEach-Object { '"' + $_ + '"' }) | Set-Content "$Build\link.rsp" -Encoding ascii
& "$vc\link.exe" "@$Build\link.rsp" 2>&1 | Where-Object { $_ -match 'error|LNK[12]' }
