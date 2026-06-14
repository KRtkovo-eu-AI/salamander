<#
.SYNOPSIS
    Simple entry point for translating one Samandarin module.

.EXAMPLE
    pwsh -File .\tools\localization\localize.ps1 start czech salamand -BuildRoot .\build\out\salamand\Release_x64
    pwsh -File .\tools\localization\localize.ps1 finish czech salamand
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $true)]
    [ValidateSet("check", "start", "open", "finish", "build")]
    [string]$Action,

    [Parameter(Position = 1)]
    [string]$Language,

    [Parameter(Position = 2)]
    [string]$Module = "salamand",

    [string]$BuildRoot = ".\build\out\salamand\Release_x64"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$workspace = Join-Path $repoRoot "out\localization"
$project = Join-Path $workspace "projects\$Language\$Module\$Module.atp"
$translator = Join-Path $workspace "runtime\utils\translator.exe"

function Require-Language
{
    if ([string]::IsNullOrWhiteSpace($Language))
    {
        throw "Zadejte jazyk. Například: localize.ps1 start czech salamand"
    }
}

function Require-Project
{
    Require-Language
    if (-not (Test-Path -LiteralPath $project))
    {
        throw "Projekt neexistuje. Nejdříve spusťte: localize.ps1 start $Language $Module -BuildRoot <cesta-k-buildu>"
    }
}

function Invoke-TranslatorQuiet
{
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [Parameter(Mandatory = $true)]
        [string]$FailureMessage
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $translator
    $startInfo.WorkingDirectory = Split-Path $translator -Parent
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments)
    {
        [void]$startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $process.WaitForExit()

    $diagnosticLog = Join-Path $workspace "localize.log"
    @(
        "Timestamp: $([DateTime]::Now.ToString('s'))"
        "Command: $translator $($Arguments -join ' ')"
        "Exit code: $($process.ExitCode)"
        ""
    ) | Add-Content -LiteralPath $diagnosticLog -Encoding UTF8

    # This repository's Translator uses exit code 1 for a successful quiet
    # import/export. Unlike Sally's newer Translator, it does not create
    # <module>.quiet.log files.
    if ($process.ExitCode -ne 1)
    {
        throw "$FailureMessage Diagnostika: $diagnosticLog"
    }
}

switch ($Action)
{
    "check"
    {
        & python (Join-Path $PSScriptRoot "audit_translation_coverage.py")
        if (-not $?)
        {
            throw "Kontrola pokrytí překladů selhala."
        }
    }

    "start"
    {
        Require-Language
        & (Join-Path $PSScriptRoot "prepare_translation_workspace.ps1") `
            -BuildRoot $BuildRoot `
            -OutputDir $workspace `
            -Languages $Language `
            -Modules $Module `
            -Force

        if (-not $?)
        {
            throw "Příprava překladového workspace selhala."
        }

        $legacyArchive = Join-Path $repoRoot "translations\$Language\$Module.slt"
        $hadLegacyArchive = Test-Path -LiteralPath $legacyArchive
        if ($hadLegacyArchive)
        {
            # Translator's SLT importer requires exactly the same resource
            # structure as the current SLG. Export the current skeleton and
            # rebase the old archive before importing it.
            $skeletonDir = Join-Path $workspace "current-skeleton"
            $rebasedDir = Join-Path $workspace "translations\$Language"
            New-Item -ItemType Directory -Path $skeletonDir, $rebasedDir -Force | Out-Null

            Invoke-TranslatorQuiet `
                -Arguments @("-quiet-export-slt", $skeletonDir, $project) `
                -FailureMessage "Nepodařilo se exportovat aktuální resource kostru."

            & (Join-Path $PSScriptRoot "rebase_text_archive.ps1") `
                -CurrentArchive (Join-Path $skeletonDir "$Module.slt") `
                -LegacyArchive $legacyArchive `
                -OutputArchive (Join-Path $rebasedDir "$Module.slt")
            if (-not $?)
            {
                throw "Převod starého překladu na aktuální resource kostru selhal."
            }

            Invoke-TranslatorQuiet `
                -Arguments @("-quiet-import-slt", $rebasedDir, $project) `
                -FailureMessage "Nepodařilo se importovat rebased překlad."
        }

        Write-Host ""
        Write-Host "Translator se otevře s projektem $Language/$Module."
        Write-Host "Projekt je založený na aktuální english.slg z buildu."
        if ($hadLegacyArchive)
        {
            Write-Host "Starý překlad byl automaticky převeden na aktuální resource kostru."
        }
        else
        {
            Write-Host "Pro modul zatím neexistuje překlad; všechny texty začínají anglicky."
        }
        Write-Host "Nové resource texty v Translatoru uvidíte jako nepřeložené položky."
        Write-Host "V Translatoru přeložte nepřeložené texty, opravte případné kontroly a projekt uložte (Ctrl+S)."
        Write-Host "Po zavření Translatoru spusťte:"
        Write-Host "  pwsh -File .\tools\localization\localize.ps1 finish $Language $Module"
        Start-Process -FilePath $translator -ArgumentList @($project)
    }

    "open"
    {
        Require-Project
        Start-Process -FilePath $translator -ArgumentList @($project)
    }

    "finish"
    {
        Require-Project
        $destination = Join-Path $repoRoot "translations\$Language"
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        Invoke-TranslatorQuiet `
            -Arguments @("-quiet-export-slt", $destination, $project) `
            -FailureMessage "Export selhal."

        Write-Host ""
        Write-Host "Hotovo: translations\$Language\$Module.slt"
        Write-Host "Zkontrolujte změny pomocí: git diff -- translations\$Language\$Module.slt"
    }

    "build"
    {
        & (Join-Path $PSScriptRoot "build_language_packs.ps1") -BuildRoot $BuildRoot
        if (-not $?)
        {
            throw "Sestavení jazykových balíčků selhalo."
        }
    }
}
