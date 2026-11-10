# Compile everything under src\ (except src\platform: the shims) into analysis\build\src\*.obj (MSVC x86). No CRT: sources may only use Win32 API.
# RR_NOGAME=1: objects for the game build (analysis\build\ng, what build.ps1 uses); RR_GS=1: stack cookies (src\runtime\gs_cookie.c)
. "$PSScriptRoot\common.ps1"
$out = if ($env:RR_NOGAME) { "$Build\ng" } else { "$Build\src" }; $defs = if ($env:RR_NOGAME) { "/DRR_NOGAME" } else { "" }; $gs = if ($env:RR_GS) { "/GS" } else { "/GS-" }
Enter-RrVcVars
New-Item -ItemType Directory -Force $out | Out-Null; Remove-Item "$out\*.obj" -ErrorAction SilentlyContinue
$n=0; foreach($f in Get-ChildItem $Src -Recurse -Filter *.c | Where-Object { $_.FullName -notlike "*\src\platform\*" }){
  & cl.exe /nologo /c /O2 $gs /Gy /W3 /Zl $defs /Fo"$out\$($f.BaseName).obj" $f.FullName | ? { $_ -match 'error|warning C4[0-9]{3}' }; $n++ }
foreach($f in (Get-ChildItem $Src -Recurse -Filter *.asm | Where-Object { $env:RR_NOGAME -or $_.Name -ne "orig_data.asm" })){ & ml.exe /nologo /c /coff /Cp /Fo"$out\$($f.BaseName).obj" $f.FullName | ? { $_ -match "error" }; $n++ }   # old generated MASM modules (git-ignored), if any are left
"compiled $n source file(s)"
