# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Add an interactive one-archive AI localization runner. | Process state=0 archives one language/module at a time and refresh the report after every run. | | openai/gpt-5.6-luna-pro |
<#
.SYNOPSIS
    Interactively translate one language/module with state=0 entries at a time.

.DESCRIPTION
    Reads the JSON report produced by report_untranslated_entries.py, prompts
    before each localization job, runs localize_all_openai.ps1 for exactly one
    language and module, and refreshes the report after the job finishes.

    Press Enter to run the displayed job, S to skip it for this session, or Q
    to quit. A job remains in the report when it still has state=0 entries, so
    failed or partial API runs can be retried explicitly.

.PARAMETER RepoRoot
    Path to the repository root. Auto-detected if omitted.

.PARAMETER BuildRoot
    Populated Salamander build used by localize_all_openai.ps1.

.PARAMETER PythonExe
    Python executable used to run the JSON report.

.PARAMETER Provider
    AI provider passed to localize_all_openai.ps1.

.EXAMPLE
    pwsh -File .\tools\localization\run_untranslated_localization.ps1 `
        -BuildRoot .\build\salamander\Release_x64
#>
[CmdletBinding()]
param(
    [string]$RepoRoot,

    [string]$BuildRoot = ".\build\out\salamand\Release_x64",

    [string]$PythonExe = "python",

    [ValidateSet("openrouter", "cursor", "openai")]
    [string]$Provider = "openrouter"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot)
{
    $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
}
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

$reportScript = Join-Path $RepoRoot "tools\localization\report_untranslated_entries.py"
$localizationScript = Join-Path $RepoRoot "tools\localization\localize_all_openai.ps1"
if (-not (Test-Path -LiteralPath $reportScript))
{
    throw "Translation report script not found: $reportScript"
}
if (-not (Test-Path -LiteralPath $localizationScript))
{
    throw "Localization script not found: $localizationScript"
}

if ([IO.Path]::IsPathRooted($BuildRoot))
{
    $resolvedBuildRoot = $BuildRoot
}
else
{
    $resolvedBuildRoot = Join-Path $RepoRoot $BuildRoot
}

function Get-UntranslatedReport
{
    $jsonLines = @(& $PythonExe $reportScript --repo-root $RepoRoot --json 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        $diagnostic = ($jsonLines | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
        throw "Translation report failed with exit code $LASTEXITCODE.`n$diagnostic"
    }

    $json = ($jsonLines | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
    try
    {
        return $json | ConvertFrom-Json
    }
    catch
    {
        throw "Translation report did not return valid JSON: $($_.Exception.Message)"
    }
}

function Get-ArchiveKey
{
    param(
        [Parameter(Mandatory = $true)]
        [object]$Archive
    )

    return ("{0}/{1}" -f $Archive.language.ToLowerInvariant(), $Archive.file.ToLowerInvariant())
}

function Find-Archive
{
    param(
        [Parameter(Mandatory = $true)]
        [object]$Report,

        [Parameter(Mandatory = $true)]
        [string]$Key
    )

    foreach ($archive in @($Report.archives))
    {
        if ((Get-ArchiveKey $archive) -eq $Key)
        {
            return $archive
        }
    }

    return $null
}

$skipped = @{}
$completedJobs = 0
$report = Get-UntranslatedReport

while ($true)
{
    $available = @(
        @($report.archives) |
            Where-Object { -not $skipped.ContainsKey((Get-ArchiveKey $_)) } |
            Sort-Object language, file
    )

    if ($available.Count -eq 0)
    {
        if (@($report.archives).Count -eq 0)
        {
            Write-Host "No state=0 translation records remain."
        }
        else
        {
            Write-Host "All currently reported archives were skipped for this session."
        }
        break
    }

    $archive = $available[0]
    $key = Get-ArchiveKey $archive
    Write-Host ""
    Write-Host ("Next: {0}/{1} has {2} state=0 record(s) of {3}." -f `
        $archive.language, $archive.file, $archive.untranslated, $archive.total_records) -ForegroundColor Cyan
    $answer = (Read-Host "Press Enter to translate, S to skip, or Q to quit").Trim().ToLowerInvariant()

    if ($answer -eq "q")
    {
        Write-Host "Stopped by user."
        break
    }
    if ($answer -eq "s")
    {
        $skipped[$key] = $true
        continue
    }
    if ($answer -ne "")
    {
        Write-Host "Unknown response; use Enter, S, or Q."
        continue
    }

    $module = [IO.Path]::GetFileNameWithoutExtension($archive.file)
    Write-Host "Starting localization for $($archive.language)/$module ..."
    & pwsh -NoProfile -File $localizationScript `
        -BuildRoot $resolvedBuildRoot `
        -Languages $archive.language `
        -Modules $module `
        -Provider $Provider `
        -ReuseWorkspace
    $localizationExitCode = $LASTEXITCODE

    $report = Get-UntranslatedReport
    $remaining = Find-Archive -Report $report -Key $key
    if ($null -eq $remaining)
    {
        Write-Host "$($archive.language)/$module is complete and no longer has state=0 records." -ForegroundColor Green
        $completedJobs++
    }
    elseif ($remaining.untranslated -lt $archive.untranslated)
    {
        Write-Host ("$($archive.language)/$module reduced state=0 records from {0} to {1}." -f `
            $archive.untranslated, $remaining.untranslated) -ForegroundColor Yellow
        $completedJobs++
    }
    else
    {
        Write-Warning "$($archive.language)/$module still has $($remaining.untranslated) state=0 record(s); it will remain available for retry."
    }

    if ($localizationExitCode -ne 0)
    {
        Write-Warning "Localization command exited with code $localizationExitCode."
    }
}

Write-Host "Completed or partially completed jobs in this session: $completedJobs"
