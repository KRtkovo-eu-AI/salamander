# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64')]
    [string]$Platform = 'x64',

    [string]$ResultsPath = 'test-results/native.xml',

    [ValidateRange(1, 119)]
    [int]$CommandTimeoutSeconds = 110
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$resultsFullPath = [System.IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot $ResultsPath))
$resultsDirectory = Split-Path -Parent $resultsFullPath
$ciBuildRoot = Join-Path $repositoryRoot 'build\ci-tests'
$buildOutput = Join-Path $ciBuildRoot "tests\$($Configuration)_$Platform"
$workerRoot = Join-Path $repositoryRoot 'build\ci-worker-root'
$results = [System.Collections.Generic.List[object]]::new()

function Invoke-TestProcess {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)]
        [string]$WorkingDirectory
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        [void]$startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    [void]$process.Start()
    $standardOutput = $process.StandardOutput.ReadToEndAsync()
    $standardError = $process.StandardError.ReadToEndAsync()
    $timedOut = -not $process.WaitForExit($CommandTimeoutSeconds * 1000)
    if ($timedOut) {
        try {
            $process.Kill($true)
        }
        catch {
            $process.Kill()
        }
        [void]$process.WaitForExit(5000)
    }
    [System.Threading.Tasks.Task]::WaitAll(
        [System.Threading.Tasks.Task[]]@($standardOutput, $standardError))
    $stopwatch.Stop()

    [pscustomobject]@{
        ExitCode = if ($timedOut) { -1 } else { $process.ExitCode }
        TimedOut = $timedOut
        Duration = $stopwatch.Elapsed.TotalSeconds
        Output = (($standardOutput.Result, $standardError.Result) -join
            [Environment]::NewLine).Trim()
    }
}

function Add-TestResult {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ClassName,
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [object]$ProcessResult
    )

    $message = if ($ProcessResult.TimedOut) {
        "Command exceeded the $CommandTimeoutSeconds second timeout."
    }
    elseif ($ProcessResult.ExitCode -ne 0) {
        "Command exited with code $($ProcessResult.ExitCode)."
    }
    else {
        ''
    }
    $results.Add([pscustomobject]@{
        ClassName = $ClassName
        Name = $Name
        Passed = $ProcessResult.ExitCode -eq 0 -and -not $ProcessResult.TimedOut
        Message = $message
        Output = $ProcessResult.Output
        Duration = $ProcessResult.Duration
    })
}

function Add-CData {
    param(
        [Parameter(Mandatory = $true)]
        [System.Xml.XmlDocument]$Document,
        [Parameter(Mandatory = $true)]
        [System.Xml.XmlElement]$Element,
        [string]$Value
    )

    $safeValue = if ($null -eq $Value) { '' } else {
        $Value.Replace(']]>', ']] >')
    }
    [void]$Element.AppendChild($Document.CreateCDataSection($safeValue))
}

function Resolve-ApplicationPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $command = Get-Command $Name -CommandType Application `
        -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $command -or [string]::IsNullOrWhiteSpace($command.Source)) {
        throw "Required application was not found: $Name"
    }
    return [string]$command.Source
}

try {
    New-Item -ItemType Directory -Force -Path $resultsDirectory, $workerRoot |
        Out-Null
    $workerFiles = @{
        'salamatrix_worker.py' =
            'src\plugins\pythonruntime\runtime\salamatrix_worker.py'
        'salamatrix_worker.ps1' =
            'src\plugins\powershellruntime\runtime\salamatrix_worker.ps1'
        'salamatrix_worker.php' =
            'src\plugins\phpruntime\runtime\salamatrix_worker.php'
    }
    foreach ($workerName in $workerFiles.Keys) {
        Copy-Item -LiteralPath (
            Join-Path $repositoryRoot $workerFiles[$workerName]
        ) -Destination $workerRoot -Force
    }
    $env:SALAMATRIX_WORKER_ROOT = $workerRoot

    $msbuild = Resolve-ApplicationPath -Name 'msbuild.exe'
    $testProjects = @(
        Get-ChildItem -Path (Join-Path $repositoryRoot 'src\plugins') `
            -Filter '*_tests.vcxproj' -File -Recurse |
            Where-Object { $_.Directory.Name -eq 'tests' } |
            Sort-Object FullName
    )
    if ($testProjects.Count -eq 0) {
        throw 'No native test projects were found.'
    }

    foreach ($project in $testProjects) {
        $testName = [System.IO.Path]::GetFileNameWithoutExtension($project.Name)
        Write-Host "Building $testName..."
        $buildResult = Invoke-TestProcess -FilePath $msbuild -Arguments @(
            $project.FullName,
            '/m',
            '/t:Build',
            "/p:Configuration=$Configuration",
            "/p:Platform=$Platform",
            "/p:OPENSAL_BUILD_DIR=$ciBuildRoot\",
            '/p:PreferredToolArchitecture=x64',
            '/p:RestoreFallbackFolders=',
            '/nr:false',
            '/v:minimal'
        ) -WorkingDirectory $repositoryRoot
        if ($buildResult.ExitCode -ne 0 -or $buildResult.TimedOut) {
            Add-TestResult -ClassName 'native-build' -Name $testName `
                -ProcessResult $buildResult
            continue
        }

        $testExecutable = Join-Path $buildOutput "$testName.exe"
        if (-not (Test-Path -LiteralPath $testExecutable -PathType Leaf)) {
            $missingResult = [pscustomobject]@{
                ExitCode = 1
                TimedOut = $false
                Duration = $buildResult.Duration
                Output = "MSBuild succeeded but did not create $testExecutable."
            }
            Add-TestResult -ClassName 'native-test' -Name $testName `
                -ProcessResult $missingResult
            continue
        }

        Write-Host "Running $testName..."
        $testResult = Invoke-TestProcess -FilePath $testExecutable `
            -WorkingDirectory $repositoryRoot
        Add-TestResult -ClassName 'native-test' -Name $testName `
            -ProcessResult $testResult
    }

    $python = Resolve-ApplicationPath -Name 'python.exe'
    foreach ($contractTest in @(
        @{
            Name = 'salamatrix_regression_tests'
            Arguments = @(
                '-B',
                (Join-Path $repositoryRoot `
                    'src\plugins\salamatrix\tests\salamatrix_regression_tests.py')
            )
        },
        @{
            Name = 'generated_automation_reference'
            Arguments = @(
                '-B',
                (Join-Path $repositoryRoot `
                    'tools\generate_salamatrix_automation_reference.py'),
                '--check'
            )
        }
    )) {
        Write-Host "Running $($contractTest.Name)..."
        $contractResult = Invoke-TestProcess -FilePath $python `
            -Arguments $contractTest.Arguments -WorkingDirectory $repositoryRoot
        Add-TestResult -ClassName 'source-contract' -Name $contractTest.Name `
            -ProcessResult $contractResult
    }

    $windowsPowerShell = Resolve-ApplicationPath -Name 'powershell.exe'
    Write-Host 'Running extension_menu_builder_smoke_tests...'
    $menuBuilderResult = Invoke-TestProcess -FilePath $windowsPowerShell `
        -Arguments @(
            '-NoProfile',
            '-ExecutionPolicy',
            'Bypass',
            '-File',
            (Join-Path $repositoryRoot `
                'src\plugins\salamatrix\tests\menu_builder_smoke_tests.ps1')
        ) -WorkingDirectory $repositoryRoot
    Add-TestResult -ClassName 'source-contract' `
        -Name 'extension_menu_builder_smoke_tests' `
        -ProcessResult $menuBuilderResult
}
catch {
    $infrastructureFailure = [pscustomobject]@{
        ExitCode = 1
        TimedOut = $false
        Duration = 0.0
        Output = ($_ | Out-String).Trim()
    }
    Add-TestResult -ClassName 'test-infrastructure' `
        -Name 'native-test-runner' -ProcessResult $infrastructureFailure
}

$document = [System.Xml.XmlDocument]::new()
[void]$document.AppendChild(
    $document.CreateXmlDeclaration('1.0', 'utf-8', $null))
$suite = $document.CreateElement('testsuite')
$suite.SetAttribute('name', 'Open Salamander native and contract tests')
$suite.SetAttribute('tests', [string]$results.Count)
$suite.SetAttribute(
    'failures', [string]@($results | Where-Object { -not $_.Passed }).Count)
$suite.SetAttribute('errors', '0')
$suite.SetAttribute('skipped', '0')
$suite.SetAttribute(
    'time',
    [string]::Format(
        [Globalization.CultureInfo]::InvariantCulture,
        '{0:0.000}',
        ($results | Measure-Object -Property Duration -Sum).Sum))
[void]$document.AppendChild($suite)

foreach ($result in $results) {
    $testCase = $document.CreateElement('testcase')
    $testCase.SetAttribute('classname', $result.ClassName)
    $testCase.SetAttribute('name', $result.Name)
    $testCase.SetAttribute(
        'time',
        [string]::Format(
            [Globalization.CultureInfo]::InvariantCulture,
            '{0:0.000}',
            $result.Duration))
    if (-not $result.Passed) {
        $failure = $document.CreateElement('failure')
        $failure.SetAttribute('message', $result.Message)
        Add-CData -Document $document -Element $failure -Value $result.Output
        [void]$testCase.AppendChild($failure)
    }
    elseif (-not [string]::IsNullOrWhiteSpace($result.Output)) {
        $systemOutput = $document.CreateElement('system-out')
        Add-CData -Document $document -Element $systemOutput `
            -Value $result.Output
        [void]$testCase.AppendChild($systemOutput)
    }
    [void]$suite.AppendChild($testCase)
}

$settings = [System.Xml.XmlWriterSettings]::new()
$settings.Encoding = [System.Text.UTF8Encoding]::new($false)
$settings.Indent = $true
$writer = [System.Xml.XmlWriter]::Create($resultsFullPath, $settings)
try {
    $document.Save($writer)
}
finally {
    $writer.Dispose()
}

$failedCount = @($results | Where-Object { -not $_.Passed }).Count
Write-Host "Wrote $($results.Count) results to $resultsFullPath."
if ($failedCount -ne 0) {
    Write-Host "$failedCount native, contract, or infrastructure test groups failed."
    exit 1
}
