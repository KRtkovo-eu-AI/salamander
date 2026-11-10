# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Add an explicitly gated FlaUI test runner. | Keep native UI tests bound to a caller-selected executable and disposable Windows profile. |  | GPT-6 (Codex) |

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath,

    [Parameter(Mandatory = $true)]
    [switch]$ConfirmDisposableProfile,

    [string]$Filter = 'TestCategory=UI'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $ConfirmDisposableProfile) {
    throw 'Run only in a disposable Windows user profile or VM, then pass -ConfirmDisposableProfile.'
}

$resolvedExecutable = (Resolve-Path -LiteralPath $ExecutablePath -ErrorAction Stop).Path
$testProject = Join-Path $PSScriptRoot 'ui-automation\Samandarin.UiTests.csproj'
if (-not (Test-Path -LiteralPath $testProject -PathType Leaf)) {
    throw "The UI automation project is missing: $testProject"
}
if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) {
    throw 'dotnet was not found on PATH.'
}
$runningInstances = @(Get-Process -Name salamand -ErrorAction SilentlyContinue)
if ($runningInstances.Count -gt 0) {
    throw 'Close all Salamander instances before running the UI tests; the application uses a single-instance launch path.'
}

$previousExe = $env:SAMANDARIN_UI_EXE
$previousConfirmation = $env:SAMANDARIN_UI_DISPOSABLE_PROFILE
$testExitCode = 1
try {
    $env:SAMANDARIN_UI_EXE = $resolvedExecutable
    $env:SAMANDARIN_UI_DISPOSABLE_PROFILE = 'YES'
    $arguments = @('test', $testProject, '--filter', $Filter, '--logger', 'console;verbosity=normal')
    & dotnet @arguments
    $testExitCode = $LASTEXITCODE
}
finally {
    if ($null -eq $previousExe) {
        Remove-Item Env:SAMANDARIN_UI_EXE -ErrorAction SilentlyContinue
    }
    else {
        $env:SAMANDARIN_UI_EXE = $previousExe
    }
    if ($null -eq $previousConfirmation) {
        Remove-Item Env:SAMANDARIN_UI_DISPOSABLE_PROFILE -ErrorAction SilentlyContinue
    }
    else {
        $env:SAMANDARIN_UI_DISPOSABLE_PROFILE = $previousConfirmation
    }
}

exit $testExitCode
