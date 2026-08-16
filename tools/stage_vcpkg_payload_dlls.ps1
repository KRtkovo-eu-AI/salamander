[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryRoot,

    [Parameter(Mandatory = $true)]
    [string]$DestinationRoot
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$destinationRoot = [IO.Path]::GetFullPath($DestinationRoot)
$innoScript = Join-Path $repositoryRoot 'doc\runbook-setup\inno_setup_salamander_x64.iss'

function Get-UnsignedPeBytes {
    param([string]$Path)

    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40) { return $null }

    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($peOffset -lt 0 -or $peOffset + 0x18 -ge $bytes.Length -or
        $bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
        $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
        return $null
    }

    $optionalHeaderOffset = $peOffset + 0x18
    $optionalHeaderMagic = [BitConverter]::ToUInt16($bytes, $optionalHeaderOffset)
    if ($optionalHeaderMagic -eq 0x10b) {
        $dataDirectoryOffset = $optionalHeaderOffset + 0x60
    } elseif ($optionalHeaderMagic -eq 0x20b) {
        $dataDirectoryOffset = $optionalHeaderOffset + 0x70
    } else {
        return $null
    }

    $certificateDirectoryOffset = $dataDirectoryOffset + (8 * 4)
    if ($certificateDirectoryOffset + 8 -gt $bytes.Length) { return $null }

    $certificateOffset = [BitConverter]::ToInt32($bytes, $certificateDirectoryOffset)
    $certificateSize = [BitConverter]::ToInt32($bytes, $certificateDirectoryOffset + 4)
    $unsignedLength = $bytes.Length
    if ($certificateOffset -gt 0 -and $certificateSize -gt 0 -and
        $certificateOffset -lt $bytes.Length -and
        $certificateOffset + $certificateSize -le $bytes.Length) {
        $unsignedLength = $certificateOffset
    }

    $normalized = New-Object byte[] $unsignedLength
    [Array]::Copy($bytes, $normalized, $unsignedLength)

    # Authenticode changes the PE checksum and security-directory entry when signing.
    $checksumOffset = $optionalHeaderOffset + 0x40
    if ($checksumOffset + 4 -le $normalized.Length) {
        [Array]::Clear($normalized, $checksumOffset, 4)
    }
    if ($certificateDirectoryOffset + 8 -le $normalized.Length) {
        [Array]::Clear($normalized, $certificateDirectoryOffset, 8)
    }
    return $normalized
}

function Test-OnlyAuthenticodeDifference {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    $sourceBytes = Get-UnsignedPeBytes $SourcePath
    $destinationBytes = Get-UnsignedPeBytes $DestinationPath
    if ($null -eq $sourceBytes -or $null -eq $destinationBytes -or
        $sourceBytes.Length -ne $destinationBytes.Length) {
        return $false
    }

    for ($i = 0; $i -lt $sourceBytes.Length; $i++) {
        if ($sourceBytes[$i] -ne $destinationBytes[$i]) { return $false }
    }
    return $true
}

function Test-FreshCertumSignature {
    param([string]$Path)

    $signature = Get-AuthenticodeSignature -LiteralPath $Path
    if ($signature.Status -ne 'Valid' -or $null -eq $signature.SignerCertificate) {
        return $false
    }

    $expectedThumbprint = [Environment]::GetEnvironmentVariable('CODESIGN_CERT_SHA1')
    $expectedThumbprint = if ($expectedThumbprint) { $expectedThumbprint -replace '\s', '' } else { '' }
    $actualThumbprint = ($signature.SignerCertificate.Thumbprint -replace '\s', '')
    $isCertum = if ($expectedThumbprint) {
        $actualThumbprint -ieq $expectedThumbprint
    } else {
        ($signature.SignerCertificate.Issuer -match '(?i)Certum') -or
            ($signature.SignerCertificate.Subject -match '(?i)Certum')
    }
    if (-not $isCertum) { return $false }

    # Authenticode exposes the timestamp certificate, but not the countersignature
    # time. The staged file is written by the signing step, so its write time is
    # the conservative age guard for preserving a signed destination.
    $fileAgeLimit = (Get-Date).AddMonths(-1)
    return (Get-Item -LiteralPath $Path).LastWriteTime -ge $fileAgeLimit
}

function Copy-VcpkgDllIfNeeded {
    param(
        [string]$SourcePath,
        [string]$DestinationPath,
        [string]$PayloadPath
    )

    $destinationDirectory = Split-Path -Parent $DestinationPath
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null

    if (-not (Test-Path -LiteralPath $DestinationPath -PathType Leaf)) {
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
        return 'copied'
    }

    $sourceHash = (Get-FileHash -LiteralPath $SourcePath -Algorithm SHA256).Hash
    $destinationHash = (Get-FileHash -LiteralPath $DestinationPath -Algorithm SHA256).Hash
    if ($sourceHash -eq $destinationHash) { return 'unchanged' }

    if ((Test-FreshCertumSignature $DestinationPath) -and
        (Test-OnlyAuthenticodeDifference -SourcePath $SourcePath -DestinationPath $DestinationPath)) {
        return 'preserved-signed'
    }

    Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
    return 'updated'
}

if (-not (Test-Path -LiteralPath $innoScript -PathType Leaf)) {
    throw "Inno Setup script was not found: $innoScript"
}

$payloadDlls = Get-Content -LiteralPath $innoScript | ForEach-Object {
    if ($_ -match '^\s*Source:\s+"\{#PayloadDir\}\\([^";]+\.dll)"') {
        $Matches[1]
    }
} | Sort-Object -Unique

$candidateRoots = @(
    (Join-Path $repositoryRoot 'build\libs'),
    (Join-Path $repositoryRoot 'build\vcpkg_installed_third_party\x64-windows\bin'),
    (Join-Path $repositoryRoot 'build\vcpkg_installed_sftp\x64-windows\bin')
)
$nonVcpkgDlls = @('dbghelp.dll')
$candidatesByName = @{}
foreach ($candidateRoot in $candidateRoots) {
    Get-ChildItem -LiteralPath $candidateRoot -Filter '*.dll' -File -ErrorAction SilentlyContinue | ForEach-Object {
        $key = $_.Name.ToLowerInvariant()
        if ($key -in $nonVcpkgDlls) { return }
        if (-not $candidatesByName.ContainsKey($key)) {
            $candidatesByName[$key] = $_.FullName
        }
    }
}

$copied = 0
$updated = 0
$preserved = 0
$matchedPayloads = 0
foreach ($payloadDll in $payloadDlls) {
    $name = (Split-Path -Leaf $payloadDll).ToLowerInvariant()
    if (-not $candidatesByName.ContainsKey($name)) { continue }

    $matchedPayloads++
    $sourcePath = $candidatesByName[$name]
    $destinationPath = Join-Path $destinationRoot $payloadDll
    $result = Copy-VcpkgDllIfNeeded -SourcePath $sourcePath -DestinationPath $destinationPath -PayloadPath $payloadDll
    switch ($result) {
        'copied' { $copied++ }
        'updated' { $updated++ }
        'preserved-signed' { $preserved++ }
    }
}

Write-Host "Staged $matchedPayloads vcpkg DLL payloads ($copied copied, $updated updated, $preserved fresh Certum-signed destinations preserved)."
