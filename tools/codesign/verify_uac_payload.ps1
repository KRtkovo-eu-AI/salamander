# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-08 | Verify the x64 UAC pilot payload. | Check signatures and the broker loading policy before packaging. | | GPT-6.1 Sol (Codex) |

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$PayloadDir,

    [string]$Mt = 'mt.exe',
    [string]$Dumpbin = 'dumpbin.exe',

    [switch]$UnsignedLab
)

$ErrorActionPreference = 'Stop'

function Invoke-InspectionTool([string]$Tool, [string[]]$Arguments) {
    $output = & $Tool @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Tool exited with code $LASTEXITCODE."
    }
    $output
}

function Test-Image([string]$Path, [string]$ExpectedLevel, [string]$ManifestPath) {
    $headers = Invoke-InspectionTool $Dumpbin @('/nologo', '/headers', $Path)
    if (!($headers -match '^\s*8664 machine \(x64\)\s*$')) {
        throw "Expected an x64 image in $Path."
    }

    Invoke-InspectionTool $Mt @('-nologo', "-inputresource:$Path;#1", "-out:$ManifestPath") | Out-Null
    [xml]$manifest = Get-Content -LiteralPath $ManifestPath -Raw
    $levels = $manifest.SelectNodes("//*[local-name()='requestedExecutionLevel']")
    if ($levels.Count -ne 1 -or $levels[0].level -cne $ExpectedLevel -or
        $levels[0].uiAccess -cne 'false') {
        throw "Expected one requestedExecutionLevel of $ExpectedLevel with uiAccess=false in $Path."
    }
}

function Get-VerifiedSignature([string]$Path) {
    $signature = Get-AuthenticodeSignature -LiteralPath $Path
    if ($signature.Status -ne 'Valid' -or !$signature.SignerCertificate -or
        !$signature.TimeStamperCertificate) {
        throw "Expected a valid timestamped Authenticode signature in $Path. Status: $($signature.Status)."
    }
    $signature
}

$payload = (Resolve-Path -LiteralPath $PayloadDir).ProviderPath
$hostPath = Join-Path $payload 'salamand.exe'
$brokerPath = Join-Path $payload 'uacbroker.exe'
foreach ($path in $hostPath, $brokerPath) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing payload file $path."
    }
}

$signer = $null
if (!$UnsignedLab) {
    $hostSignature = Get-VerifiedSignature $hostPath
    $brokerSignature = Get-VerifiedSignature $brokerPath
    $signer = $hostSignature.SignerCertificate.Thumbprint
    if ($signer -ne $brokerSignature.SignerCertificate.Thumbprint) {
        throw 'The application and broker must be signed by the same certificate.'
    }
}

$Mt = (Get-Command -Name $Mt -CommandType Application).Source
$Dumpbin = (Get-Command -Name $Dumpbin -CommandType Application).Source
$temp = New-Item -ItemType Directory -Path (Join-Path ([IO.Path]::GetTempPath()) "samandarin-uac-$([guid]::NewGuid())")
$hostManifest = Join-Path $temp.FullName 'host.manifest'
$brokerManifest = Join-Path $temp.FullName 'broker.manifest'
try {
    Test-Image $hostPath 'asInvoker' $hostManifest
    Test-Image $brokerPath 'requireAdministrator' $brokerManifest

    $loadConfig = Invoke-InspectionTool $Dumpbin @('/nologo', '/loadconfig', $brokerPath)
    if (!($loadConfig -match '^\s*0800 Dependent Load Flag\s*$')) {
        throw 'Expected LOAD_LIBRARY_SEARCH_SYSTEM32 (0x0800) in the broker load configuration.'
    }

    $allowedImports = 'ADVAPI32.dll', 'ole32.dll', 'SHELL32.dll', 'ntdll.dll', 'KERNEL32.dll'
    $dependencies = Invoke-InspectionTool $Dumpbin @('/nologo', '/dependents', $brokerPath)
    $imports = @($dependencies | ForEach-Object {
        if ($_ -match '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') { $Matches[1] }
    })
    $unexpectedImports = @($imports | Where-Object { $_ -notin $allowedImports })
    if ($imports.Count -eq 0 -or $unexpectedImports.Count -gt 0) {
        throw "Unexpected broker import list: $($imports -join ', ')."
    }
}
finally {
    foreach ($manifest in $hostManifest, $brokerManifest) {
        if (Test-Path -LiteralPath $manifest -PathType Leaf) {
            Remove-Item -LiteralPath $manifest
        }
    }
    Remove-Item -LiteralPath $temp.FullName
}

[pscustomobject]@{
    PayloadDir = $payload
    HostSha256 = (Get-FileHash -LiteralPath $hostPath -Algorithm SHA256).Hash
    BrokerSha256 = (Get-FileHash -LiteralPath $brokerPath -Algorithm SHA256).Hash
    BrokerImports = $imports
    SignaturesVerified = !$UnsignedLab
    SignerThumbprint = $signer
}
