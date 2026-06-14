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

switch ($Action)
{
    "check"
    {
        & python (Join-Path $PSScriptRoot "audit_translation_coverage.py")
        exit $LASTEXITCODE
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

        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

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

            & $translator -quiet-export-slt-for-diff $skeletonDir $project
            if ($LASTEXITCODE -ne 0)
            {
                throw "Nepodařilo se exportovat aktuální resource kostru. Podrobnosti jsou v projects\$Language\$Module\$Module.quiet.log."
            }

            & (Join-Path $PSScriptRoot "rebase_text_archive.ps1") `
                -CurrentArchive (Join-Path $skeletonDir "$Module.slt") `
                -LegacyArchive $legacyArchive `
                -OutputArchive (Join-Path $rebasedDir "$Module.slt")

            & $translator -quiet-import-slt $rebasedDir $project
            if ($LASTEXITCODE -ne 0)
            {
                throw "Nepodařilo se importovat rebased překlad. Podrobnosti jsou v projects\$Language\$Module\$Module.quiet.log."
            }
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
        & $translator -quiet-export-slt $destination $project
        if ($LASTEXITCODE -ne 0)
        {
            throw "Export selhal. Podrobnosti jsou v projects\$Language\$Module\$Module.quiet.log."
        }

        Write-Host ""
        Write-Host "Hotovo: translations\$Language\$Module.slt"
        Write-Host "Zkontrolujte změny pomocí: git diff -- translations\$Language\$Module.slt"
    }

    "build"
    {
        & (Join-Path $PSScriptRoot "build_language_packs.ps1") -BuildRoot $BuildRoot
        exit $LASTEXITCODE
    }
}
