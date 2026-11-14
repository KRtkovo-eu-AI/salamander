# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Sync top-level translation archives into plugin translation trees. | Plugin submodules can contain stale copies of the current repository translations. | | openai/gpt-5.6-luna-pro |
# | 2026-11-11 | Keep WhatIf side-effect free for new language directories. | Directory creation must happen only after ShouldProcess approves the copy. | | openai/gpt-5.6-luna-pro |
<#
.SYNOPSIS
    Sync .slt translation archives from the repository translations/ directory
    into plugin translation directories.

.DESCRIPTION
    Each plugin that maintains its own .slt files under
    src/plugins/<name>/translations/{lang}/ is treated as a destination. For
    every plugin-owned .slt file, the matching archive from
    translations/{lang}/ is copied into the plugin tree. A language directory
    is also created when translations/{lang}/{plugin}.slt exists, which lets
    newly added repository languages enter the plugin submodule. Missing source
    files are skipped and reported; plugin-only files are not deleted.

.PARAMETER RepoRoot
    Path to the repository root. Auto-detected if omitted.

.PARAMETER Modules
    Comma-separated list of plugin modules to synchronize. If omitted, all
    plugin translation trees are synchronized.

.PARAMETER Languages
    Comma-separated list of language directories to synchronize. If omitted,
    all language directories already present in each plugin, plus repository
    languages containing the selected plugin archive, are synchronized.

.EXAMPLE
    pwsh -File tools\localization\sync_translations_to_plugin.ps1
    pwsh -File tools\localization\sync_translations_to_plugin.ps1 -Modules sftp
    pwsh -File tools\localization\sync_translations_to_plugin.ps1 -Modules sftp -Languages cs,sk
    pwsh -File tools\localization\sync_translations_to_plugin.ps1 -Modules sftp -WhatIf
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = "Medium")]
param(
    [string]$RepoRoot,

    [string[]]$Modules,

    [string[]]$Languages
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot)
{
    $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
}

$translationsDir = Join-Path $RepoRoot "translations"
$pluginsDir = Join-Path $RepoRoot "src\plugins"

if (-not (Test-Path -LiteralPath $translationsDir))
{
    throw "Translations directory not found: $translationsDir"
}

if (-not (Test-Path -LiteralPath $pluginsDir))
{
    throw "Plugins directory not found: $pluginsDir"
}

function Expand-Names
{
    param(
        [string[]]$Values
    )

    $expanded = @()
    foreach ($value in $Values)
    {
        foreach ($piece in ($value -split "[,;]"))
        {
            $trimmed = $piece.Trim().ToLowerInvariant()
            if ($trimmed -ne "")
            {
                $expanded += $trimmed
            }
        }
    }

    return @($expanded | Sort-Object -Unique)
}

$requestedModules = @()
if ($Modules -and $Modules.Count -gt 0)
{
    $requestedModules = @(Expand-Names $Modules)
}

$requestedLanguages = @()
if ($Languages -and $Languages.Count -gt 0)
{
    $requestedLanguages = @(Expand-Names $Languages)
}

$pluginDirs = @(Get-ChildItem -Path $pluginsDir -Directory)
if ($requestedModules.Count -gt 0)
{
    $availableModules = @($pluginDirs | ForEach-Object { $_.Name.ToLowerInvariant() })
    $unknownModules = @($requestedModules | Where-Object { $availableModules -notcontains $_ })
    if ($unknownModules.Count -gt 0)
    {
        throw "Plugin module(s) not found: $($unknownModules -join ', ')"
    }
}

$synced = 0
$planned = 0
$missingSources = 0

foreach ($pluginDir in $pluginDirs)
{
    $moduleName = $pluginDir.Name.ToLowerInvariant()
    if ($requestedModules.Count -gt 0 -and $requestedModules -notcontains $moduleName)
    {
        continue
    }

    $pluginTranslationsDir = Join-Path $pluginDir.FullName "translations"
    if (-not (Test-Path -LiteralPath $pluginTranslationsDir))
    {
        continue
    }

    $languageNames = @{}
    foreach ($langDir in (Get-ChildItem -Path $pluginTranslationsDir -Directory))
    {
        $languageNames[$langDir.Name] = $true
    }

    # Include repository languages that have this plugin's archive but are not
    # present in the submodule yet.
    foreach ($repoLanguageDir in (Get-ChildItem -Path $translationsDir -Directory))
    {
        $repoModuleArchive = Join-Path $repoLanguageDir.FullName "$moduleName.slt"
        if (Test-Path -LiteralPath $repoModuleArchive)
        {
            $languageNames[$repoLanguageDir.Name] = $true
        }
    }

    foreach ($languageName in ($languageNames.Keys | Sort-Object))
    {
        if ($requestedLanguages.Count -gt 0 -and
            $requestedLanguages -notcontains $languageName.ToLowerInvariant())
        {
            continue
        }

        $langDir = Join-Path $pluginTranslationsDir $languageName
        $language = $languageName.ToLowerInvariant()
        if ($requestedLanguages.Count -gt 0 -and $requestedLanguages -notcontains $language)
        {
            continue
        }

        $sltFiles = @()
        if (Test-Path -LiteralPath $langDir)
        {
            $sltFiles = @(Get-ChildItem -LiteralPath $langDir -Filter "*.slt" -File)
        }
        if ($sltFiles.Count -eq 0)
        {
            $newArchive = Join-Path $translationsDir "$languageName\$moduleName.slt"
            if (Test-Path -LiteralPath $newArchive)
            {
                $sltFiles = @([pscustomobject]@{ Name = "$moduleName.slt"; FullName = (Join-Path $langDir "$moduleName.slt") })
            }
        }

        foreach ($sltFile in $sltFiles)
        {
            $sourcePath = Join-Path (Join-Path $translationsDir $languageName) $sltFile.Name
            if (-not (Test-Path -LiteralPath $sourcePath))
            {
                Write-Warning "Source translation not found, skipped: translations\$($langDir.Name)\$($sltFile.Name)"
                $missingSources++
                continue
            }

            $targetPath = $sltFile.FullName
            $description = "Copy translations\$languageName\$($sltFile.Name) to $moduleName\translations\$languageName\$($sltFile.Name)"
            if ($PSCmdlet.ShouldProcess($targetPath, $description))
            {
                if (-not (Test-Path -LiteralPath $langDir))
                {
                    New-Item -ItemType Directory -Path $langDir -Force | Out-Null
                    Write-Host "  Created: src\plugins\$moduleName\translations\$languageName"
                }
                Copy-Item -LiteralPath $sourcePath -Destination $targetPath -Force
                Write-Host "  Synced:  translations\$languageName\$($sltFile.Name) -> src\plugins\$moduleName\translations\$languageName\$($sltFile.Name)"
                $synced++
            }
            else
            {
                $planned++
            }
        }
    }
}

if ($WhatIfPreference)
{
    Write-Host "Would sync $planned .slt file(s) into plugin translations."
}
elseif ($synced -eq 0)
{
    Write-Host "No plugin .slt files found to sync."
}
else
{
    Write-Host "Synced $synced .slt file(s) into plugin translations."
}

if ($missingSources -gt 0)
{
    Write-Warning "$missingSources plugin .slt file(s) had no matching source under translations/."
}
