[CmdletBinding()]
param(
    [string]$OutputDir,
    [switch]$Force
)

<#
.SYNOPSIS
Downloads and stages the pinned CPU llama.cpp runtime and the bundled GGUF model.

.DESCRIPTION
The SalamatrixAI plug-in deliberately launches llama.cpp as an isolated process.
This script creates the runtime asset directory consumed by the plug-in build:

  build\libs\salamatrixai\llama-cli.exe
  build\libs\salamatrixai\*.dll
  build\libs\salamatrixai\salamatrix.gguf

The executable archive and model are pinned by URL and SHA-256. The model is
renamed to the stable SalamatrixAI name expected by the provider. No Salamander
process is started or loaded by this script.
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $repoRoot 'build\libs\salamatrixai'
}
$OutputDir = [IO.Path]::GetFullPath($OutputDir)

$llamaVersion = 'b10107'
$llamaArchiveName = "llama-$llamaVersion-bin-win-cpu-x64.zip"
$llamaArchiveUrl = "https://github.com/ggml-org/llama.cpp/releases/download/$llamaVersion/$llamaArchiveName"
$llamaArchiveSha256 = '52133A0A5A8F6035B1BDD2F89C3425EA8B742413D9BDB9A2DEE30E3A1681B18C'
$llamaLicenseUrl = "https://raw.githubusercontent.com/ggml-org/llama.cpp/$llamaVersion/LICENSE"
$llamaLicenseSha256 = '94F29BBED6A22C35B992C5C6EBF0E7C92F13B836B90F36F461C9CF2F0F1D010D'

$modelRepo = 'Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF'
$modelFile = 'qwen2.5-coder-0.5b-instruct-q4_k_m.gguf'
$modelUrl = "https://huggingface.co/$modelRepo/resolve/main/${modelFile}?download=true"
$modelSha256 = '1D9614638D18024D0FBB36575A15F1302A3ADF044DF10345688EC4F6E1C4FF32'
$modelLicenseUrl = "https://huggingface.co/$modelRepo/resolve/main/LICENSE?download=true"
$modelLicenseSha256 = '832DD9E00A68DD83B3C3FB9F5588DAD7DCF337A0DB50F7D9483F310CD292E92E'

function Get-VerifiedFile {
    param(
        [Parameter(Mandatory = $true)] [string]$Uri,
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Sha256
    )

    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Host "Downloading $Uri"
        Invoke-WebRequest -Uri $Uri -OutFile $Path -UseBasicParsing
    }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($actual -ne $Sha256.ToUpperInvariant()) {
        throw "SHA-256 mismatch for '$Path': expected $Sha256, got $actual."
    }
}

$existing = @(Get-ChildItem -LiteralPath $OutputDir -File -ErrorAction SilentlyContinue)
if ($existing.Count -gt 0 -and !$Force) {
    throw "Output directory is not empty: $OutputDir. Use -Force to replace its asset files."
}

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ('salamatrixai-assets-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null
try {
    $archivePath = Join-Path $tempRoot $llamaArchiveName
    $modelPath = Join-Path $tempRoot $modelFile
    $llamaLicensePath = Join-Path $tempRoot 'llama.cpp.LICENSE.txt'
    $modelLicensePath = Join-Path $tempRoot 'Qwen2.5-Coder.LICENSE.txt'

    Get-VerifiedFile -Uri $llamaArchiveUrl -Path $archivePath -Sha256 $llamaArchiveSha256
    Get-VerifiedFile -Uri $modelUrl -Path $modelPath -Sha256 $modelSha256
    Get-VerifiedFile -Uri $llamaLicenseUrl -Path $llamaLicensePath -Sha256 $llamaLicenseSha256
    Get-VerifiedFile -Uri $modelLicenseUrl -Path $modelLicensePath -Sha256 $modelLicenseSha256

    $unpackRoot = Join-Path $tempRoot 'llama'
    Expand-Archive -LiteralPath $archivePath -DestinationPath $unpackRoot -Force
    $cli = Get-ChildItem -LiteralPath $unpackRoot -Recurse -File -Filter 'llama-cli.exe' | Select-Object -First 1
    if (!$cli) { throw "The pinned llama.cpp archive does not contain llama-cli.exe." }
    $runtimeSource = $cli.Directory.FullName
    $runtimeFiles = @(Get-ChildItem -LiteralPath $runtimeSource -File | Where-Object {
        $_.Name -eq 'llama-cli.exe' -or $_.Extension -ieq '.dll'
    })
    if (!$runtimeFiles) { throw "The pinned llama.cpp archive contains no runtime files." }

    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    foreach ($file in $runtimeFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $OutputDir $file.Name) -Force
    }
    Copy-Item -LiteralPath $modelPath -Destination (Join-Path $OutputDir 'salamatrix.gguf') -Force
    Copy-Item -LiteralPath $llamaLicensePath -Destination (Join-Path $OutputDir (Split-Path $llamaLicensePath -Leaf)) -Force
    Copy-Item -LiteralPath $modelLicensePath -Destination (Join-Path $OutputDir (Split-Path $modelLicensePath -Leaf)) -Force

    $manifest = [ordered]@{
        llamaCppVersion = $llamaVersion
        llamaCppArchive = $llamaArchiveName
        llamaCppArchiveSha256 = $llamaArchiveSha256
        model = $modelFile
        modelSha256 = $modelSha256
        modelRepository = $modelRepo
        target = 'Windows x64 CPU'
    }
    $manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDir 'asset-manifest.json') -Encoding UTF8
    Write-Host "Staged SalamatrixAI assets in $OutputDir"
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
