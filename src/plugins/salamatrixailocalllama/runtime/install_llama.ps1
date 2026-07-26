# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Destination
)

$ErrorActionPreference = 'Stop'

$llamaUrl = 'https://github.com/ggml-org/llama.cpp/releases/download/b10107/llama-b10107-bin-win-cpu-x64.zip'
$llamaSha256 = '52133A0A5A8F6035B1BDD2F89C3425EA8B742413D9BDB9A2DEE30E3A1681B18C'
$modelUrl = 'https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF/resolve/main/qwen2.5-coder-0.5b-instruct-q4_k_m.gguf?download=true'
$modelSha256 = '1D9614638D18024D0FBB36575A15F1302A3ADF044DF10345688EC4F6E1C4FF32'
$modelLicenseUrl = 'https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF/resolve/main/LICENSE'

$runtime = Join-Path $Destination 'runtime'
$temp = Join-Path ([System.IO.Path]::GetTempPath()) ('salamatrix-llama-' + [guid]::NewGuid().ToString('N'))
$archive = Join-Path $temp 'llama.zip'
$model = Join-Path $temp 'salamatrix.gguf'
$extract = Join-Path $temp 'extract'

function Download-VerifiedFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Uri,
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$ExpectedHash,
        [Parameter(Mandatory = $true)]
        [string]$Description
    )

    $normalizedExpected = $ExpectedHash.Trim().ToUpperInvariant()
    $partPath = "$Path.part"
    $lastActualHash = '<download failed>'
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        Remove-Item -LiteralPath $Path, $partPath -Force -ErrorAction SilentlyContinue

        try {
            $ProgressPreference = 'SilentlyContinue'
            $head = Invoke-WebRequest -UseBasicParsing -Method Head -Uri $Uri
            $contentLength = [int64]$head.Headers['Content-Length']
            if ($contentLength -le 0) {
                throw "The server did not provide a valid Content-Length for $Description."
            }

            $chunkSize = 8MB
            $offset = [int64]0
            while ($offset -lt $contentLength) {
                $end = [Math]::Min($offset + $chunkSize - 1, $contentLength - 1)
                Remove-Item -LiteralPath $partPath -Force -ErrorAction SilentlyContinue
                Invoke-WebRequest -UseBasicParsing -Uri $Uri `
                    -Headers @{ Range = "bytes=$offset-$end" } -OutFile $partPath

                $chunkLength = (Get-Item -LiteralPath $partPath).Length
                $expectedChunkLength = $end - $offset + 1
                if ($chunkLength -ne $expectedChunkLength) {
                    throw ("The server returned {0} bytes for range {1}-{2}; expected {3}." -f
                        $chunkLength, $offset, $end, $expectedChunkLength)
                }

                $sourceStream = [System.IO.File]::OpenRead($partPath)
                try {
                    $output = [System.IO.File]::Open($Path,
                        [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write,
                        [System.IO.FileShare]::None)
                    try { $sourceStream.CopyTo($output) }
                    finally { $output.Dispose() }
                }
                finally { $sourceStream.Dispose() }

                $offset = $end + 1
            }

            $lastActualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.Trim().ToUpperInvariant()
            if ($lastActualHash -ceq $normalizedExpected) {
                Remove-Item -LiteralPath $partPath -Force -ErrorAction SilentlyContinue
                return
            }

            Write-Warning ("{0} SHA-256 mismatch on attempt {1}/3. Expected {2}, got {3}." -f
                $Description, $attempt, $normalizedExpected, $lastActualHash)
        }
        catch {
            Write-Warning ("{0} download attempt {1}/3 failed: {2}" -f
                $Description, $attempt, $_.Exception.Message)
        }
        finally {
            Remove-Item -LiteralPath $partPath -Force -ErrorAction SilentlyContinue
        }
    }

    throw ("{0} SHA-256 verification failed after 3 attempts. Expected {1}, got {2}." -f
        $Description, $normalizedExpected, $lastActualHash)
}

try {
    New-Item -ItemType Directory -Force -Path $temp, $extract, $runtime | Out-Null
    Write-Host 'Downloading llama.cpp Windows x64 CPU package...'
    Download-VerifiedFile -Uri $llamaUrl -Path $archive -ExpectedHash $llamaSha256 `
        -Description 'llama.cpp archive'

    Expand-Archive -LiteralPath $archive -DestinationPath $extract -Force
    $llamaExe = Get-ChildItem -LiteralPath $extract -Filter 'llama-cli.exe' -File -Recurse | Select-Object -First 1
    if ($null -eq $llamaExe) { throw 'The downloaded llama.cpp archive has no llama-cli.exe.' }
    Get-ChildItem -LiteralPath $llamaExe.DirectoryName -File | Copy-Item -Destination $runtime -Force

    Write-Host 'Downloading Qwen2.5-Coder GGUF model...'
    Download-VerifiedFile -Uri $modelUrl -Path $model -ExpectedHash $modelSha256 `
        -Description 'Qwen model'
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
