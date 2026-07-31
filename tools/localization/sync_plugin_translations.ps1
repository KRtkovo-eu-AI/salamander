<#
.SYNOPSIS
    Sync .slt translation archives from plugin submodules into the repo translations/ directory.

.DESCRIPTION
    Each plugin that maintains its own .slt files under src/plugins/<name>/translations/{lang}/
    gets synced into the top-level translations/{lang}/ directory. This ensures the build
    pipeline (build_language_packs.ps1) picks up the latest translations without requiring
    translators to manually copy files.

    Plugin discovery is automatic: any directory under src/plugins/*/translations/ that
    contains .slt files is treated as a plugin source.

.PARAMETER RepoRoot
    Path to the repository root. Auto-detected if omitted.

.PARAMETER Modules
    Comma-separated list of plugin modules to synchronize. If omitted, all
    plugin translation archives are synchronized.

.EXAMPLE
    pwsh -File tools\localization\sync_plugin_translations.ps1
    pwsh -File tools\localization\sync_plugin_translations.ps1 -RepoRoot H:\_projects\salamander
    pwsh -File tools\localization\sync_plugin_translations.ps1 -Modules sftp
#>
[CmdletBinding()]
param(
    [string]$RepoRoot,

    [string[]]$Modules
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot)
{
    $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
}

$translationsDir = Join-Path $RepoRoot "translations"
$pluginsDir = Join-Path $RepoRoot "src\plugins"

if (-not (Test-Path -LiteralPath $pluginsDir))
{
    throw "Plugins directory not found: $pluginsDir"
}

$synced = 0
$requestedModules = @()
if ($Modules -and $Modules.Count -gt 0)
{
    foreach ($module in $Modules)
    {
        foreach ($piece in ($module -split "[,;]"))
        {
            $trimmed = $piece.Trim().ToLowerInvariant()
            if ($trimmed -ne "")
            {
                $requestedModules += $trimmed
            }
        }
    }
    $requestedModules = @($requestedModules | Sort-Object -Unique)
}

# Scan each plugin for a translations/ subdirectory
foreach ($pluginDir in (Get-ChildItem -Path $pluginsDir -Directory))
{
    if ($requestedModules.Count -gt 0 -and
        $requestedModules -notcontains $pluginDir.Name.ToLowerInvariant())
    {
        continue
    }

    $pluginTranslationsDir = Join-Path $pluginDir.FullName "translations"
    if (-not (Test-Path -LiteralPath $pluginTranslationsDir))
    {
        continue
    }

    # Scan language subdirectories within the plugin
    foreach ($langDir in (Get-ChildItem -Path $pluginTranslationsDir -Directory))
    {
        $sltFiles = Get-ChildItem -LiteralPath $langDir.FullName -Filter "*.slt" -File
        if (-not $sltFiles)
        {
            continue
        }

        # Ensure target language directory exists
        $targetLangDir = Join-Path $translationsDir $langDir.Name
        if (-not (Test-Path -LiteralPath $targetLangDir))
        {
            New-Item -ItemType Directory -Path $targetLangDir -Force | Out-Null
            Write-Host "  Created: translations\$($langDir.Name)"
        }

        foreach ($sltFile in $sltFiles)
        {
            $targetPath = Join-Path $targetLangDir $sltFile.Name
            Copy-Item -LiteralPath $sltFile.FullName -Destination $targetPath -Force
            Write-Host "  Synced:  $($pluginDir.Name)\translations\$($langDir.Name)\$($sltFile.Name) -> translations\$($langDir.Name)\$($sltFile.Name)"
            $synced++
        }
    }
}

if ($synced -eq 0)
{
    Write-Host "No plugin .slt files found to sync."
}
else
{
    Write-Host "Synced $synced .slt file(s) from plugin translations."
}
