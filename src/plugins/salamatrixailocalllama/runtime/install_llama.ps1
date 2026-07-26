# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Destination
)

$ErrorActionPreference = 'Stop'

$llamaUrl = 'https://github.com/ggml-org/llama.cpp/releases/download/b10107/llama-b10107-bin-win-cpu-x64.zip'
$llamaSha256 = '52133A0A5A8F6035B1BDD2F89C3425EA8B742413D9BDB9A2DEE30E3A1681B18'
$modelUrl = 'https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF/resolve/main/qwen2.5-coder-0.5b-instruct-q4_k_m.gguf?download=true'
$modelSha256 = '1D9614638D18024D0FBB36575A15F1302A3ADF044DF10345688EC4F6E1C4FF32'
$modelLicenseUrl = 'https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF/resolve/main/LICENSE'

$runtime = Join-Path $Destination 'runtime'
$temp = Join-Path ([System.IO.Path]::GetTempPath()) ('salamatrix-llama-' + [guid]::NewGuid().ToString('N'))
$archive = Join-Path $temp 'llama.zip'
$model = Join-Path $temp 'salamatrix.gguf'
$extract = Join-Path $temp 'extract'

try {
    New-Item -ItemType Directory -Force -Path $temp, $extract, $runtime | Out-Null
    Write-Host 'Downloading llama.cpp Windows x64 CPU package...'
    Invoke-WebRequest -UseBasicParsing -Uri $llamaUrl -OutFile $archive
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash -ne $llamaSha256) {
        throw 'llama.cpp archive SHA-256 verification failed.'
    }

    Expand-Archive -LiteralPath $archive -DestinationPath $extract -Force
    $llamaExe = Get-ChildItem -LiteralPath $extract -Filter 'llama-cli.exe' -File -Recurse | Select-Object -First 1
    if ($null -eq $llamaExe) { throw 'The downloaded llama.cpp archive has no llama-cli.exe.' }
    Get-ChildItem -LiteralPath $llamaExe.DirectoryName -File | Copy-Item -Destination $runtime -Force

    Write-Host 'Downloading Qwen2.5-Coder GGUF model...'
    Invoke-WebRequest -UseBasicParsing -Uri $modelUrl -OutFile $model
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $model).Hash -ne $modelSha256) {
        throw 'Qwen model SHA-256 verification failed.'
    }
    Move-Item -LiteralPath $model -Destination (Join-Path $runtime 'salamatrix.gguf') -Force
    Invoke-WebRequest -UseBasicParsing -Uri $modelLicenseUrl -OutFile (Join-Path $runtime 'Qwen2.5-Coder.LICENSE.txt')
    Get-ChildItem -LiteralPath $llamaExe.DirectoryName -File -Filter '*LICENSE*' |
        Copy-Item -Destination $runtime -Force
    Write-Host 'Salamatrix Local LLaMA installation completed.'
}
finally {
    if (Test-Path -LiteralPath $temp) {
        Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
    }
}
