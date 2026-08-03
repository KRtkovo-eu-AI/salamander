param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $Arguments
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent (Split-Path -Parent $scriptRoot)
$defaultInnoScript = Join-Path $repoRoot 'doc\runbook-setup\inno_setup_salamander_x64.iss'

function Write-Usage {
    Write-Host 'Usage:'
    Write-Host '  codesign_certum.cmd --file <path-to-exe-dll-or-spl>'
    Write-Host '  codesign_certum.cmd --inno-x64 --payload-dir <Release_x64 payload directory>'
    Write-Host '  codesign_certum.cmd --slg-by-lang --payload-dir <Release_x64 payload directory>'
    Write-Host ''
    Write-Host 'Required environment:'
    Write-Host '  CODESIGN_ENABLED=1'
    Write-Host '  CODESIGN_CERT_SHA1=<Certum certificate thumbprint without spaces>'
}

function Get-RequiredValue([string] $name) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "Environment variable $name is required."
    }
    return $value
}

function Get-OptionalValue([string] $name, [string] $defaultValue) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if ([string]::IsNullOrWhiteSpace($value)) {
        return $defaultValue
    }
    return $value
}

function Get-SignToolPath {
    $configured = [Environment]::GetEnvironmentVariable('CODESIGN_SIGNTOOL')
    if (-not [string]::IsNullOrWhiteSpace($configured)) {
        return $configured
    }
    return 'signtool.exe'
}

function Test-SignableExtension([string] $path) {
    $extension = [IO.Path]::GetExtension($path).ToLowerInvariant()
    return $extension -eq '.exe' -or $extension -eq '.dll' -or $extension -eq '.spl' -or $extension -eq '.slg'
}

function Test-ExcludedExternalBinary([string] $path) {
    $name = [IO.Path]::GetFileName($path).ToLowerInvariant()
    $exactExclusions = @(
        '7za.dll',
        '7zwrapper.dll',
        'chmlib.dll',
        'newtonsoft.json.dll',
        'markdig.dll',
        'prismsharp.dll',
        'ucrtbase.dll',
        'vcruntime140.dll',
        'vcruntime140_1.dll',
        'msvcp140.dll',
        'concrt140.dll'
    )
    if ($exactExclusions -contains $name) {
        return $true
    }

    $wildcardExclusions = @(
        'api-ms-win-*.dll',
        'webview2*.dll',
        'system.*.dll',
        'microsoft.web.webview2.*.dll'
    )
    foreach ($pattern in $wildcardExclusions) {
        if ($name -like $pattern) {
            return $true
        }
    }
    return $false
}

function Get-InnoSourcePaths([string] $innoScript, [string] $payloadDir) {
    if (-not (Test-Path -LiteralPath $innoScript)) {
        throw "Inno Setup script not found: $innoScript"
    }
    if (-not (Test-Path -LiteralPath $payloadDir)) {
        throw "Payload directory not found: $payloadDir"
    }

    $payloadRoot = (Resolve-Path -LiteralPath $payloadDir).Path.TrimEnd('\')
    $paths = New-Object System.Collections.Generic.List[string]

    foreach ($line in Get-Content -LiteralPath $innoScript) {
        if ($line -match '^Source:\s*"([^"]+)"') {
            $source = $Matches[1]
            if ($source.StartsWith('{#PayloadDir}\', [StringComparison]::OrdinalIgnoreCase)) {
                $relative = $source.Substring('{#PayloadDir}\'.Length)
                $paths.Add((Join-Path $payloadRoot $relative))
            }
        }
    }

    return $paths
}

function Get-InnoSignTargets([string] $innoScript, [string] $payloadDir) {
    $sourcePaths = Get-InnoSourcePaths -innoScript $innoScript -payloadDir $payloadDir
    $targets = New-Object System.Collections.Generic.List[string]
    $missing = New-Object System.Collections.Generic.List[string]

    foreach ($path in $sourcePaths) {
        if (-not (Test-SignableExtension $path)) {
            continue
        }
        if (Test-ExcludedExternalBinary $path) {
            continue
        }
        if (Test-Path -LiteralPath $path) {
            $targets.Add((Resolve-Path -LiteralPath $path).Path)
        }
        else {
            $missing.Add($path)
        }
    }

    if ($missing.Count -gt 0) {
        Write-Warning "Skipped $($missing.Count) signable file(s) listed by Inno Setup because they do not exist in the payload directory."
        foreach ($path in $missing) {
            Write-Warning "  missing: $path"
        }
    }

    return $targets | Sort-Object -Unique
}

function Invoke-SignTool([string[]] $arguments) {
    $signTool = Get-SignToolPath
    Write-Host "> $signTool $($arguments -join ' ')"

    # Native command output is part of a PowerShell function's success stream.
    # Capture and re-emit it so callers assigning this function's result receive
    # only the numeric process exit code, not SignTool's verbose text plus the code.
    $output = & $signTool @arguments 2>&1
    $exitCode = $LASTEXITCODE
    foreach ($line in $output) {
        Write-Host $line
    }
    return [int] $exitCode
}

function New-SignArguments([string[]] $paths) {
    if ([Environment]::GetEnvironmentVariable('CODESIGN_ENABLED') -ne '1') {
        throw 'Code signing is disabled. Set CODESIGN_ENABLED=1 before running this manual signing script.'
    }

    $thumbprint = (Get-RequiredValue 'CODESIGN_CERT_SHA1') -replace '\s', ''
    $timestampUrl = Get-OptionalValue 'CODESIGN_TIMESTAMP_URL' 'http://time.certum.pl'
    $digestAlgorithm = Get-OptionalValue 'CODESIGN_DIGEST_ALGORITHM' 'sha256'
    $timestampDigestAlgorithm = Get-OptionalValue 'CODESIGN_TIMESTAMP_DIGEST_ALGORITHM' 'sha256'
    $description = [Environment]::GetEnvironmentVariable('CODESIGN_DESCRIPTION')
    $descriptionUrl = [Environment]::GetEnvironmentVariable('CODESIGN_DESCRIPTION_URL')

    $signArguments = @(
        'sign',
        '/sha1', $thumbprint,
        '/tr', $timestampUrl,
        '/td', $timestampDigestAlgorithm,
        '/fd', $digestAlgorithm,
        '/v'
    )
    if (-not [string]::IsNullOrWhiteSpace($description)) {
        $signArguments += @('/d', $description)
    }
    if (-not [string]::IsNullOrWhiteSpace($descriptionUrl)) {
        $signArguments += @('/du', $descriptionUrl)
    }
    $signArguments += $paths
    return $signArguments
}

function Get-RetryCount {
    return [int] (Get-OptionalValue 'CODESIGN_RETRIES' '3')
}

function Get-RetryDelaySeconds {
    return [int] (Get-OptionalValue 'CODESIGN_RETRY_DELAY_SECONDS' '10')
}

function Get-SignBatchSize {
    $batchSize = [int] (Get-OptionalValue 'CODESIGN_BATCH_SIZE' '25')
    if ($batchSize -lt 1) {
        throw 'CODESIGN_BATCH_SIZE must be greater than zero.'
    }
    return $batchSize
}

function Test-SigningTarget([string] $path) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Signing target does not exist: $path"
    }
    if (-not (Test-SignableExtension $path)) {
        throw "Signing target must be .exe, .dll, .spl, or .slg: $path"
    }
}

function Test-FileSigned([string] $path) {
    Write-Host "Checking existing signature for $path ..."
    $verifyExitCode = Invoke-SignTool @('verify', '/pa', '/all', '/q', $path)
    return $verifyExitCode -eq 0
}

function Verify-OneFile([string] $path) {
    Write-Host "Verifying $path ..."
    $verifyExitCode = Invoke-SignTool @('verify', '/pa', '/all', '/v', $path)
    if ($verifyExitCode -ne 0) {
        throw "Signature verification failed for $path with exit code $verifyExitCode."
    }
}

function Get-UnsignedFiles([string[]] $paths) {
    $unsignedPaths = New-Object System.Collections.Generic.List[string]

    foreach ($path in $paths) {
        if (Test-FileSigned $path) {
            Write-Host "Skipping already signed file: $path"
            continue
        }
        $unsignedPaths.Add($path)
    }

    return $unsignedPaths.ToArray()
}

function Invoke-SignWithRetry([string[]] $paths, [string] $label) {
    $retries = Get-RetryCount
    $retryDelaySeconds = Get-RetryDelaySeconds
    $signArguments = New-SignArguments $paths

    for ($attempt = 1; $attempt -le $retries; $attempt++) {
        Write-Host "Signing $label (attempt $attempt of $retries) ..."
        $exitCode = Invoke-SignTool $signArguments
        if ($exitCode -eq 0) {
            foreach ($path in $paths) {
                Verify-OneFile $path
            }
            return
        }

        if ($attempt -lt $retries) {
            Write-Warning "Signing failed with exit code $exitCode. Retrying in $retryDelaySeconds second(s)."
            Start-Sleep -Seconds $retryDelaySeconds
        }
        else {
            throw "Signing failed for $label after $retries attempt(s). Last exit code: $exitCode."
        }
    }
}

function Invoke-SignBatches([string[]] $paths, [string] $label) {
    $batchSize = Get-SignBatchSize
    $total = $paths.Count
    $batchCount = [int] [Math]::Ceiling($total / $batchSize)

    for ($start = 0; $start -lt $total; $start += $batchSize) {
        $end = [Math]::Min($start + $batchSize - 1, $total - 1)
        $batchPaths = @($paths[$start..$end])
        $batchNumber = [int] [Math]::Floor($start / $batchSize) + 1
        $batchLabel = "$label batch $batchNumber of $batchCount ($($batchPaths.Count) file(s))"
        Invoke-SignWithRetry -paths $batchPaths -label $batchLabel
    }
}

function Sign-OneFile([string] $path) {
    Test-SigningTarget $path
    $resolvedPath = (Resolve-Path -LiteralPath $path).Path
    $unsignedPaths = @(Get-UnsignedFiles @($resolvedPath))
    if ($unsignedPaths.Count -eq 0) {
        Write-Host "No unsigned files remain for signing."
        return
    }
    Invoke-SignWithRetry -paths @($unsignedPaths) -label $resolvedPath
}

function Sign-ManyFiles([string[]] $paths) {
    if ($paths.Count -eq 0) {
        throw 'No files were provided for signing.'
    }

    $resolvedPaths = foreach ($path in $paths) {
        Test-SigningTarget $path
        (Resolve-Path -LiteralPath $path).Path
    }

    $unsignedPaths = @(Get-UnsignedFiles @($resolvedPaths))
    if ($unsignedPaths.Count -eq 0) {
        Write-Host "No unsigned files remain for signing."
        return
    }

    Invoke-SignBatches -paths @($unsignedPaths) -label "$($unsignedPaths.Count) unsigned Inno payload file(s)"
}

function Sign-InnoPayload([string] $payloadDir, [string] $innoScript) {
    $targets = @(Get-InnoSignTargets -innoScript $innoScript -payloadDir $payloadDir)
    if ($targets.Count -eq 0) {
        throw "No signable non-external .exe/.dll/.spl files were found in $payloadDir from $innoScript."
    }

    Write-Host "Found $($targets.Count) file(s) to sign from Inno Setup payload."
    foreach ($target in $targets) {
        Write-Host "  $target"
    }

    Sign-ManyFiles @($targets)
}

function Get-SlgFilesByLanguage([string] $payloadDir, [string] $innoScript) {
    $sourcePaths = Get-InnoSourcePaths -innoScript $innoScript -payloadDir $payloadDir
    $slgByLang = @{}

    foreach ($path in $sourcePaths) {
        $extension = [IO.Path]::GetExtension($path).ToLowerInvariant()
        if ($extension -ne '.slg') {
            continue
        }
        if (-not (Test-Path -LiteralPath $path)) {
            continue
        }

        $langName = [IO.Path]::GetFileNameWithoutExtension($path).ToLowerInvariant()
        $resolvedPath = (Resolve-Path -LiteralPath $path).Path

        if (-not $slgByLang.ContainsKey($langName)) {
            $slgByLang[$langName] = New-Object System.Collections.Generic.List[string]
        }
        $slgByLang[$langName].Add($resolvedPath)
    }

    return $slgByLang
}

function Sign-SlgByLanguage([string] $payloadDir, [string] $innoScript) {
    $slgByLang = Get-SlgFilesByLanguage -payloadDir $payloadDir -innoScript $innoScript

    if ($slgByLang.Count -eq 0) {
        throw "No .slg files were found in $payloadDir from $innoScript."
    }

    Write-Host "Found $($slgByLang.Count) language(s) with .slg files to sign."
    $totalFiles = 0
    foreach ($lang in ($slgByLang.Keys | Sort-Object)) {
        $count = $slgByLang[$lang].Count
        $totalFiles += $count
        Write-Host "  ${lang}: $count file(s)"
    }
    Write-Host "Total: $totalFiles file(s)"
    Write-Host ''

    foreach ($lang in ($slgByLang.Keys | Sort-Object)) {
        $files = $slgByLang[$lang]
        Write-Host "Signing language: $lang ($($files.Count) file(s)) ..."
        foreach ($file in $files) {
            Write-Host "  $file"
        }
        Sign-ManyFiles @($files)
        Write-Host ''
    }
}

if ($Arguments.Count -eq 0) {
    Write-Usage
    exit 2
}

$mode = $Arguments[0].ToLowerInvariant()
switch ($mode) {
    '--file' {
        if ($Arguments.Count -ne 2) {
            Write-Usage
            exit 2
        }
        Sign-OneFile $Arguments[1]
    }
    '--inno-x64' {
        $payloadDir = $null
        $innoScript = $defaultInnoScript
        for ($i = 1; $i -lt $Arguments.Count; $i++) {
            switch ($Arguments[$i].ToLowerInvariant()) {
                '--payload-dir' {
                    $i++
                    if ($i -ge $Arguments.Count) { throw '--payload-dir requires a value.' }
                    $payloadDir = $Arguments[$i]
                }
                '--inno-script' {
                    $i++
                    if ($i -ge $Arguments.Count) { throw '--inno-script requires a value.' }
                    $innoScript = $Arguments[$i]
                }
                default {
                    throw "Unknown argument for --inno-x64: $($Arguments[$i])"
                }
            }
        }
        if ([string]::IsNullOrWhiteSpace($payloadDir)) {
            $buildRoot = [Environment]::GetEnvironmentVariable('OPENSAL_BUILD_DIR')
            if (-not [string]::IsNullOrWhiteSpace($buildRoot)) {
                $payloadDir = Join-Path $buildRoot 'salamander\Release_x64'
            }
        }
        if ([string]::IsNullOrWhiteSpace($payloadDir)) {
            throw 'Use --payload-dir or set OPENSAL_BUILD_DIR.'
        }
        Sign-InnoPayload -payloadDir $payloadDir -innoScript $innoScript
    }
    '--slg-by-lang' {
        $payloadDir = $null
        $innoScript = $defaultInnoScript
        for ($i = 1; $i -lt $Arguments.Count; $i++) {
            switch ($Arguments[$i].ToLowerInvariant()) {
                '--payload-dir' {
                    $i++
                    if ($i -ge $Arguments.Count) { throw '--payload-dir requires a value.' }
                    $payloadDir = $Arguments[$i]
                }
                '--inno-script' {
                    $i++
                    if ($i -ge $Arguments.Count) { throw '--inno-script requires a value.' }
                    $innoScript = $Arguments[$i]
                }
                default {
                    throw "Unknown argument for --slg-by-lang: $($Arguments[$i])"
                }
            }
        }
        if ([string]::IsNullOrWhiteSpace($payloadDir)) {
            $buildRoot = [Environment]::GetEnvironmentVariable('OPENSAL_BUILD_DIR')
            if (-not [string]::IsNullOrWhiteSpace($buildRoot)) {
                $payloadDir = Join-Path $buildRoot 'salamander\Release_x64'
            }
        }
        if ([string]::IsNullOrWhiteSpace($payloadDir)) {
            throw 'Use --payload-dir or set OPENSAL_BUILD_DIR.'
        }
        Sign-SlgByLanguage -payloadDir $payloadDir -innoScript $innoScript
    }
    default {
        Write-Usage
        exit 2
    }
}
