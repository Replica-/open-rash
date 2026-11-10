# Incremental build: recompiles only the src\ C files newer than their object in analysis\build\ng (or the files named with -Files), then link (with the resources).
# Same compiler flags as build_src.ps1 (RR_NOGAME build). A changed header is NOT tracked: name the files that include it (-Files a.c,b.c) or run build.ps1.
# usage: build_inc.ps1 [-Files x.c,y.c] [-Shim]   (-Shim also rebuilds ddraw.dll)
param([string[]]$Files = @(), [switch]$Shim)
. "$PSScriptRoot\common.ps1"
$out = "$Build\ng"
Get-Process OpenRash -ErrorAction SilentlyContinue | Stop-Process -Force
Enter-RrVcVars
New-Item -ItemType Directory -Force $out | Out-Null
$all = Get-ChildItem $Src -Recurse -Filter *.c | Where-Object { $_.FullName -notlike "*\src\platform\*" }
$n = 0
foreach ($f in $all) {
    $obj = "$out\$($f.BaseName).obj"
    $want = ($Files | Where-Object { $_ -ieq $f.Name -or $_ -ieq $f.BaseName }).Count -gt 0
    if (-not $want -and (Test-Path $obj) -and (Get-Item $obj).LastWriteTime -ge $f.LastWriteTime) { continue }
    & cl.exe /nologo /c /O2 /GS- /Gy /W3 /Zl /DRR_NOGAME /Fo"$obj" $f.FullName | Where-Object { $_ -match 'error|warning C4[0-9]{3}' }
    $n++
}
"compiled $n file(s)"
& "$PSScriptRoot\link.ps1"
if ($Shim) { & "$PSScriptRoot\build_ddraw.ps1" }
if (Test-Path "$Build\OpenRash.exe") { "built: " + (Get-Item "$Build\OpenRash.exe").Length + " bytes" }
