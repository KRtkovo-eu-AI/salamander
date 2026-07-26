<#
.SYNOPSIS
Validates the standalone Salamatrix runtime and helper plugin binaries without
starting Salamander.

.DESCRIPTION
This is a target-independent packaging gate. It checks that the expected .spl
files exist below an unpacked Salamander directory, are valid PE images for the
requested architecture, export the two mandatory Salamander entry points, and
contain the runtime bootstrap files shipped next to the plugins.

The script intentionally does not load a plugin, modify the registry, or start
Salamander. It is therefore safe to run against a build or staging directory on
a build machine; loading and GUI behavior still need validation on the target
machine where the user's Salamander installation and runtimes are present.

.EXAMPLE
.\tools\verify_runtime_packages.ps1 -SalamanderPath .\build\salamander\Release_x64

.EXAMPLE
.\tools\verify_runtime_packages.ps1 -SalamanderPath C:\staging\Release_x64 -Architecture arm64
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$SalamanderPath,

    [ValidateSet('x64', 'arm64')]
    [string]$Architecture = 'x64'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExistingDirectory {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Description
    )

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    $item = Get-Item -LiteralPath $resolved.Path
    if (-not $item.PSIsContainer) {
        throw "$Description is not a directory: $Path"
    }
    return $item.FullName
}

function Read-UInt16At {
    param([byte[]]$Bytes, [int]$Offset)
    if ($Offset -lt 0 -or $Offset + 2 -gt $Bytes.Length) {
        throw "PE field is outside the file at offset 0x$('{0:X}' -f $Offset)."
    }
    return [BitConverter]::ToUInt16($Bytes, $Offset)
}

function Read-UInt32At {
    param([byte[]]$Bytes, [int]$Offset)
    if ($Offset -lt 0 -or $Offset + 4 -gt $Bytes.Length) {
        throw "PE field is outside the file at offset 0x$('{0:X}' -f $Offset)."
    }
    return [BitConverter]::ToUInt32($Bytes, $Offset)
}

function Convert-RvaToFileOffset {
    param(
        [byte[]]$Bytes,
        [uint32]$Rva,
        [int]$PeOffset,
        [uint16]$SectionCount,
        [uint16]$OptionalHeaderSize
    )

    $sectionOffset = $PeOffset + 24 + $OptionalHeaderSize
    for ($index = 0; $index -lt $SectionCount; $index++) {
        $current = $sectionOffset + ($index * 40)
        $virtualSize = Read-UInt32At -Bytes $Bytes -Offset ($current + 8)
        $virtualAddress = Read-UInt32At -Bytes $Bytes -Offset ($current + 12)
        $rawSize = Read-UInt32At -Bytes $Bytes -Offset ($current + 16)
        $rawOffset = Read-UInt32At -Bytes $Bytes -Offset ($current + 20)
        $span = [Math]::Max($virtualSize, $rawSize)
        if ($Rva -ge $virtualAddress -and $Rva -lt ($virtualAddress + $span)) {
            $fileOffset = [uint64]$rawOffset + ($Rva - $virtualAddress)
            if ($fileOffset -gt [int]::MaxValue -or $fileOffset -ge $Bytes.Length) {
                throw "PE RVA 0x$('{0:X}' -f $Rva) resolves outside the file."
            }
            return [int]$fileOffset
        }
    }

    if ($Rva -lt ($PeOffset + 24 + $OptionalHeaderSize + ($SectionCount * 40)) -and $Rva -lt $Bytes.Length) {
        return [int]$Rva
    }
    throw "PE RVA 0x$('{0:X}' -f $Rva) does not map to a section."
}

function Get-PeExports {
    param([Parameter(Mandatory = $true)] [string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
        throw "'$Path' is not an MZ image."
    }

    $peOffset = [int](Read-UInt32At -Bytes $bytes -Offset 0x3C)
    if ($peOffset -lt 0 -or $peOffset + 24 -gt $bytes.Length -or
        $bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
        $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
        throw "'$Path' has no valid PE signature."
    }

    $machine = Read-UInt16At -Bytes $bytes -Offset ($peOffset + 4)
    $machineName = switch ($machine) {
        0x8664 { 'x64' }
        0xAA64 { 'arm64' }
        default { '0x{0:X4}' -f $machine }
    }
    $sectionCount = Read-UInt16At -Bytes $bytes -Offset ($peOffset + 6)
    $optionalHeaderSize = Read-UInt16At -Bytes $bytes -Offset ($peOffset + 20)
    $optionalOffset = $peOffset + 24
    $magic = Read-UInt16At -Bytes $bytes -Offset $optionalOffset
    $directoryOffset = switch ($magic) {
        0x20B { $optionalOffset + 112 }
        0x10B { $optionalOffset + 96 }
        default { throw "'$Path' has unsupported PE optional-header magic 0x$('{0:X4}' -f $magic)." }
    }

    $exportRva = Read-UInt32At -Bytes $bytes -Offset $directoryOffset
    if ($exportRva -eq 0) {
        throw "'$Path' has no export directory."
    }
    $exportOffset = Convert-RvaToFileOffset -Bytes $bytes -Rva $exportRva -PeOffset $peOffset -SectionCount $sectionCount -OptionalHeaderSize $optionalHeaderSize
    $nameCount = Read-UInt32At -Bytes $bytes -Offset ($exportOffset + 24)
    $namesRva = Read-UInt32At -Bytes $bytes -Offset ($exportOffset + 32)
    $namesOffset = Convert-RvaToFileOffset -Bytes $bytes -Rva $namesRva -PeOffset $peOffset -SectionCount $sectionCount -OptionalHeaderSize $optionalHeaderSize
    $exports = [System.Collections.Generic.List[string]]::new()
    for ($index = 0; $index -lt $nameCount; $index++) {
        $nameRva = Read-UInt32At -Bytes $bytes -Offset ($namesOffset + ($index * 4))
        $nameOffset = Convert-RvaToFileOffset -Bytes $bytes -Rva $nameRva -PeOffset $peOffset -SectionCount $sectionCount -OptionalHeaderSize $optionalHeaderSize
        $end = $nameOffset
        while ($end -lt $bytes.Length -and $bytes[$end] -ne 0) { $end++ }
        if ($end -ge $bytes.Length) { throw "Unterminated PE export name in '$Path'." }
        $exports.Add([Text.Encoding]::ASCII.GetString($bytes, $nameOffset, $end - $nameOffset))
    }

    return [pscustomobject]@{ Machine = $machineName; Exports = $exports }
}

$expected = @(
    [pscustomobject]@{ Name = 'javascriptruntime'; Bootstrap = 'runtime\salamatrix_worker.mjs' },
    [pscustomobject]@{ Name = 'pythonruntime'; Bootstrap = 'runtime\salamatrix_worker.py' },
    [pscustomobject]@{ Name = 'powershellruntime'; Bootstrap = 'runtime\salamatrix_worker.ps1' },
    [pscustomobject]@{ Name = 'phpruntime'; Bootstrap = 'runtime\salamatrix_worker.php' },
    [pscustomobject]@{ Name = 'salamatrixai'; Bootstrap = 'runtime\salamatrix_ai_local.py' },
    [pscustomobject]@{ Name = 'salamatrixailocalllama'; Bootstrap = 'runtime\llama-cli.exe' }
)

$root = Resolve-ExistingDirectory -Path $SalamanderPath -Description 'Salamander path'
$pluginsRoot = Resolve-ExistingDirectory -Path (Join-Path $root 'plugins') -Description 'Plugins path'
$failures = [System.Collections.Generic.List[string]]::new()

foreach ($item in $expected) {
    $pluginRoot = Join-Path $pluginsRoot $item.Name
    $splPath = Join-Path $pluginRoot ($item.Name + '.spl')
    if (-not (Test-Path -LiteralPath $splPath -PathType Leaf)) {
        $failures.Add("$($item.Name): missing $splPath")
        continue
    }

    try {
        $pe = Get-PeExports -Path $splPath
        if ($pe.Machine -ne $Architecture) {
            $failures.Add("$($item.Name): expected $Architecture PE, found $($pe.Machine)")
        }
        foreach ($export in @('SalamanderPluginEntry', 'SalamanderPluginGetReqVer')) {
            if (-not $pe.Exports.Contains($export)) {
                $failures.Add("$($item.Name): missing export $export")
            }
        }
    }
    catch {
        $failures.Add("$($item.Name): $($_.Exception.Message)")
    }

    $bootstrapPath = Join-Path $pluginRoot $item.Bootstrap
    if (-not (Test-Path -LiteralPath $bootstrapPath -PathType Leaf)) {
        $failures.Add("$($item.Name): missing bootstrap $bootstrapPath")
    }

    if ($item.Name -eq 'salamatrixailocalllama') {
        foreach ($asset in @('llama-cli.exe', 'salamatrix.gguf')) {
            $assetPath = Join-Path $pluginRoot ('runtime\' + $asset)
            if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
                $failures.Add("$($item.Name): missing bundled asset $assetPath")
            }
        }
        $dllCount = @(Get-ChildItem -LiteralPath (Join-Path $pluginRoot 'runtime') -File -Filter '*.dll' -ErrorAction SilentlyContinue).Count
        if ($dllCount -eq 0) {
            $failures.Add("$($item.Name): bundled llama.cpp DLLs are missing")
        }
    }

}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "Validated $($expected.Count) standalone runtime/helper package(s) for $Architecture."
