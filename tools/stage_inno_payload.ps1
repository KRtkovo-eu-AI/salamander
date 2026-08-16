[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryRoot,

    [Parameter(Mandatory = $true)]
    [string]$DestinationRoot
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$destinationRoot = [System.IO.Path]::GetFullPath($DestinationRoot)
$innoScript = Join-Path $repositoryRoot 'doc\runbook-setup\inno_setup_salamander_x64.iss'

if (-not (Test-Path -LiteralPath $innoScript -PathType Leaf)) {
    throw "Inno Setup script was not found: $innoScript"
}

function Resolve-PayloadSource {
    param([string]$PayloadPath)

    $sourceRelativePath = switch -Regex ($PayloadPath) {
        '^toolbars\\' { Join-Path 'src\res' $PayloadPath; break }
        '^doc\\license\.txt$' { 'LICENSE'; break }
        '^doc\\license_gpl\.txt$' { Join-Path 'doc\license' 'license_gpl.txt'; break }
        default { $PayloadPath }
    }

    Join-Path $repositoryRoot $sourceRelativePath
}

function Copy-IfDifferent {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    $destinationDirectory = Split-Path -Parent $DestinationPath
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null

    $needsCopy = -not (Test-Path -LiteralPath $DestinationPath -PathType Leaf)
    if (-not $needsCopy) {
        $sourceHash = (Get-FileHash -LiteralPath $SourcePath -Algorithm SHA256).Hash
        $destinationHash = (Get-FileHash -LiteralPath $DestinationPath -Algorithm SHA256).Hash
        $needsCopy = $sourceHash -ne $destinationHash
    }

    if ($needsCopy) {
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
        return $true
    }

    return $false
}

$payloadPaths = Get-Content -LiteralPath $innoScript | ForEach-Object {
    if ($_ -match '^\s*Source:\s+"\{#PayloadDir\}\\((?:toolbars|doc|convert)\\[^";]+)"') {
        $Matches[1]
    }
} | Sort-Object -Unique

if ($payloadPaths.Count -eq 0) {
    throw "No toolbars, doc, or convert payload files were found in $innoScript"
}

$copiedCount = 0
foreach ($payloadPath in $payloadPaths) {
    $sourcePath = Resolve-PayloadSource $payloadPath
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Payload source for '$payloadPath' was not found: $sourcePath"
    }

    $destinationPath = Join-Path $destinationRoot $payloadPath
    if (Copy-IfDifferent -SourcePath $sourcePath -DestinationPath $destinationPath) {
        $copiedCount++
    }
}

Write-Host "Staged $($payloadPaths.Count) Inno payload files from toolbars, doc, and convert ($copiedCount updated)."
