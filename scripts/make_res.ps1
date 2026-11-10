# Build a .res file (the linker's resource input) from src\resources\resources.txt + the .bin files it lists, byte for byte (no rc.exe needed).
# .res format: an empty 32-byte entry, then per resource: DataSize, HeaderSize (32), type (0xFFFF + id), name (0xFFFF + id), DataVersion, MemoryFlags, LanguageId,
# Version, Characteristics, the data, padding to 4 bytes.   usage: make_res.ps1 -Out <file.res>
param([Parameter(Mandatory)][string]$Out)
. "$PSScriptRoot\common.ps1"
$dir = "$Src\resources"
$ms = New-Object IO.MemoryStream; $w = New-Object IO.BinaryWriter $ms
function Entry([int]$type, [int]$id, [int]$lang, [byte[]]$data) {
    $w.Write([uint32]$data.Length); $w.Write([uint32]32)
    $w.Write([uint16]0xFFFF); $w.Write([uint16]$type); $w.Write([uint16]0xFFFF); $w.Write([uint16]$id)
    $w.Write([uint32]0); $w.Write([uint16]$(if ($type -eq 3) { 0x1010 } else { 0x1030 })); $w.Write([uint16]$lang); $w.Write([uint32]0); $w.Write([uint32]0)
    $w.Write($data); while ($ms.Length % 4) { $w.Write([byte]0) }
}
$w.Write([uint32]0); $w.Write([uint32]32); $w.Write([uint16]0xFFFF); $w.Write([uint16]0); $w.Write([uint16]0xFFFF); $w.Write([uint16]0)
$w.Write([uint32]0); $w.Write([uint16]0); $w.Write([uint16]0); $w.Write([uint32]0); $w.Write([uint32]0)
$n = 0
foreach ($l in Get-Content "$dir\resources.txt") {
    if ($l -match '^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\S+)\s*$') { Entry ([int]$Matches[1]) ([int]$Matches[2]) ([int]$Matches[3]) ([IO.File]::ReadAllBytes("$dir\$($Matches[4])")); $n++ }
}
$w.Flush(); [IO.File]::WriteAllBytes($Out, $ms.ToArray())
"resources: $n"
