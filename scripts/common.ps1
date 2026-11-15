# Shared paths for the build scripts (dot-sourced: . "$PSScriptRoot\common.ps1").
# $Root = the repository; $Build = build output (git-ignored). Building needs no game files.
# SDL3 3.4.20 (VC development package): deps\SDL3-3.4.20, or %RR_SDL_DIR%; the older analysis\sdl_dl\x3 location still works.
$Root  = (Resolve-Path "$PSScriptRoot\..").Path
$Build = "$Root\analysis\build"
$Src   = "$Root\src"
$VsBuildTools = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
$WinSdkLib    = "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x86"
# Elsewhere (e.g. a GitHub runner with Visual Studio Enterprise): the newest Visual Studio with the C++ x86 tools, the newest Windows 10/11 SDK
if (-not (Test-Path "$VsBuildTools\VC\Auxiliary\Build\vcvarsall.bat")) {
    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) { $VsBuildTools = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath }
}
if (-not (Test-Path "$WinSdkLib\kernel32.lib")) {
    $WinSdkLib = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\Lib\*\um\x86\kernel32.lib" -ErrorAction SilentlyContinue |
        Sort-Object { [version]$_.Directory.Parent.Parent.Name } | Select-Object -Last 1 | ForEach-Object { $_.DirectoryName }
}

function Get-RrSdl {
    foreach ($d in $env:RR_SDL_DIR, "$Root\deps\SDL3-3.4.20", "$Root\analysis\sdl_dl\x3\SDL3-3.4.20") {
        if ($d -and (Test-Path "$d\include\SDL3")) { return (Resolve-Path $d).Path }
    }
    throw "SDL3 3.4.20 not found: unpack SDL3-devel-3.4.20-VC.zip to $Root\deps (or set RR_SDL_DIR)"
}

# MSVC x86 environment (cl / link / ml / lib on PATH) in this PowerShell session
function Enter-RrVcVars {
    if ($env:VSCMD_ARG_TGT_ARCH -eq "x86") { return }
    $vcvars = (Resolve-Path "$VsBuildTools\VC\Auxiliary\Build\vcvarsall.bat").Path
    $env:PATH += ";C:\Program Files (x86)\Microsoft Visual Studio\Installer"
    cmd /c "`"$vcvars`" x86 >nul && set" | ForEach-Object { if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2]) } }
}
