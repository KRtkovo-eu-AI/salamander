<#
.SYNOPSIS
    Build translated .slg language packs from committed .slt archives.

.DESCRIPTION
    Post-build step that produces non-English .slg files for all available
    translations. Generates a translator workspace, imports .slt archives
    via translator.exe quiet mode, and copies the resulting .slg files into
    the populated runtime tree.

    This script is meant to run after a normal Samandarin build + populate.
    It requires translator.exe in the build output.

.PARAMETER BuildRoot
    Path to the populated build output (e.g., build/out/salamand/Release_x64).

.PARAMETER Languages
    Comma-separated list of languages to build. If omitted, builds all
    languages that have translation archives in the repo.

.PARAMETER Modules
    Comma-separated list of modules to build. If omitted, builds all modules
    available in the populated runtime tree.

.PARAMETER WorkspaceDir
    Path for the temporary translator workspace. Defaults to a temp directory.

.PARAMETER SkipValidation
    Skip quiet-validate-all after import (faster, less safe).

.PARAMETER AllowSeedRejections
    Treat seed rejections (untranslated modules) as warnings instead of errors.
    Useful for local dev builds where incomplete localization is acceptable.

.PARAMETER TranslatorExe
    Path to translator.exe to use instead of the one in BuildRoot.
    Enables cross-architecture builds: an x64 translator.exe can produce SLGs
    for any architecture since BeginUpdateResource is arch-agnostic.

.EXAMPLE
    pwsh -File tools\localization\build_language_packs.ps1 `
        -BuildRoot build\out\salamand\Release_x64

.EXAMPLE
    pwsh -File tools\localization\build_language_packs.ps1 `
        -BuildRoot build\out\salamand\Release_x64 `
        -Modules sftp
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildRoot,

    [string[]]$Languages,

    [string[]]$Modules,

    [string]$WorkspaceDir,

    [switch]$SkipValidation,

    [switch]$AllowSeedRejections,

    [string]$TranslatorExe
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
$repoRoot = Split-Path (Split-Path $scriptDir -Parent) -Parent
$prepareScript = Join-Path $scriptDir "prepare_translation_workspace.ps1"
Import-Module (Join-Path $scriptDir "Localization.Common.psm1") -Force

if (-not (Test-Path -LiteralPath $prepareScript))
{
    throw "Cannot find prepare_translation_workspace.ps1 at: $prepareScript"
}

$buildRootFull = if (Test-Path -LiteralPath $BuildRoot) {
    (Resolve-Path -LiteralPath $BuildRoot).Path
} else {
    throw "BuildRoot not found: $BuildRoot"
}

if (-not $TranslatorExe)
{
    $TranslatorExe = Join-Path $buildRootFull "utils\translator.exe"
}
if (-not (Test-Path -LiteralPath $TranslatorExe))
{
    throw "translator.exe not found at: $TranslatorExe"
}

# Normalize the optional module filter before synchronizing plugin archives.
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

# Sync .slt files from plugin submodules into translations/
$syncScript = Join-Path $scriptDir "sync_plugin_translations.ps1"
if (Test-Path -LiteralPath $syncScript)
{
    Write-Host "Syncing plugin translations..."
    $syncArgs = @{ RepoRoot = $repoRoot }
    if ($requestedModules.Count -gt 0)
    {
        $syncArgs['Modules'] = $requestedModules
    }
    & $syncScript @syncArgs
    Write-Host ""
}

# Determine languages
$translationsDir = Join-Path $repoRoot "translations"
$allLanguages = Get-ChildItem -Path $translationsDir -Directory |
    Where-Object { (Get-ChildItem -LiteralPath $_.FullName -Filter "*.slt" -File).Count -gt 0 } |
    ForEach-Object { $_.Name } |
    Sort-Object

if ($Languages -and $Languages.Count -gt 0)
{
    $requestedLanguages = @()
    foreach ($lang in $Languages)
    {
        foreach ($piece in ($lang -split "[,;]"))
        {
            $trimmed = $piece.Trim().ToLowerInvariant()
            if ($trimmed -ne "")
            {
                $requestedLanguages += $trimmed
            }
        }
    }
}
else
{
    $requestedLanguages = $allLanguages
}

Write-Host "Building language packs for: $($requestedLanguages -join ', ')"
if ($requestedModules.Count -gt 0)
{
    Write-Host "Building modules: $($requestedModules -join ', ')"
}
else
{
    Write-Host "Building modules: all"
}
Write-Host "Build root: $buildRootFull"

# Create workspace directory
if (-not $WorkspaceDir)
{
    $WorkspaceDir = Join-Path ([System.IO.Path]::GetTempPath()) "salamand-langpack-workspace"
}
else
{
    # Translator stores the export path from the project file as-is.  A
    # relative workspace therefore becomes relative to each project directory
    # (for example, projects\czech\7zip\.\out\...), which makes quiet
    # round-trip export fail with "path not found".  Normalize it once before
    # generating any project files.
    if (-not [System.IO.Path]::IsPathRooted($WorkspaceDir))
    {
        $WorkspaceDir = Join-Path (Get-Location).Path $WorkspaceDir
    }
    $WorkspaceDir = [System.IO.Path]::GetFullPath($WorkspaceDir)
}

if (Test-Path -LiteralPath $WorkspaceDir)
{
    Remove-Item -Recurse -Force -LiteralPath $WorkspaceDir
}

# Generate workspace with archive import
Write-Host ""
Write-Host "Generating translator workspace..."
$prepareArgs = @{
    BuildRoot = $buildRootFull
    OutputDir = $WorkspaceDir
    Languages = ($requestedLanguages -join ",")
    ImportArchives = $true
    Force = $true
}
if ($requestedModules.Count -gt 0)
{
    $prepareArgs['Modules'] = $requestedModules
}
if ($TranslatorExe)
{
    $prepareArgs['TranslatorExe'] = (Resolve-Path -LiteralPath $TranslatorExe).Path
}
& $prepareScript @prepareArgs

if ((Test-Path variable:LASTEXITCODE) -and $LASTEXITCODE -ne 0)
{
    throw "Workspace generation failed with exit code $LASTEXITCODE"
}

# Copy translated .slg files into the runtime tree
$projectsRoot = Join-Path $WorkspaceDir "projects"
$workspaceRuntimeRoot = Join-Path $WorkspaceDir "runtime"
$diagnosticLog = Join-Path $WorkspaceDir "localize.log"
$copied = 0
$copyFailures = New-Object System.Collections.Generic.List[string]
$seedRejections = New-Object System.Collections.Generic.List[string]
$coreSeedRejections = New-Object System.Collections.Generic.List[string]
$validated = 0
$validationWarnings = New-Object System.Collections.Generic.List[string]
$roundtripFailures = New-Object System.Collections.Generic.List[string]
$pendingCopies = New-Object System.Collections.Generic.List[object]
$languageLangIds = @{
    chinesesimplified = 2052
    czech = 1029
    dutch = 1043
    french = 1036
    german = 1031
    hungarian = 1038
    italian = 1040
    romanian = 1048
    russian = 1049
    slovak = 1051
    spanish = 3082
}

foreach ($language in $requestedLanguages)
{
    $langProjectDir = Join-Path $projectsRoot $language
    if (-not (Test-Path -LiteralPath $langProjectDir))
    {
        Write-Warning "No projects found for language: $language"
        continue
    }

    foreach ($moduleDir in (Get-ChildItem -LiteralPath $langProjectDir -Directory))
    {
        $moduleName = $moduleDir.Name
        $slgFile = Join-Path $moduleDir.FullName "$language.slg"
        $atpFile = Join-Path $moduleDir.FullName "$moduleName.atp"

        if (-not (Test-Path -LiteralPath $slgFile))
        {
            $copyFailures.Add("${language}/${moduleName}: .slg not found")
            continue
        }

        # Reject untouched English seeds — if import failed, the .slg is
        # still an identical copy of english.slg and must not be shipped
        # under a translated filename.
        $englishSlgPath = if ($moduleName -eq "salamand") {
            Join-Path $workspaceRuntimeRoot "lang\english.slg"
        } else {
            Join-Path $workspaceRuntimeRoot "plugins\$moduleName\lang\english.slg"
        }
        if ((Test-Path -LiteralPath $englishSlgPath) -and
            (Get-FileHash -LiteralPath $slgFile).Hash -eq (Get-FileHash -LiteralPath $englishSlgPath).Hash)
        {
            $rejection = "${language}/${moduleName}: rejected (identical to English seed)"
            $seedRejections.Add($rejection)
            if ($moduleName -eq "salamand")
            {
                $coreSeedRejections.Add($rejection)
            }
            continue
        }

        # Optional validation — uses Invoke-SalamanderTranslatorQuiet for proper window detection
        if (-not $SkipValidation -and (Test-Path -LiteralPath $atpFile))
        {
            try
            {
                Invoke-SalamanderTranslatorQuiet `
                    -TranslatorExe $TranslatorExe `
                    -Arguments @("-quiet-validate-layout", $atpFile) `
                    -FailureMessage "$language/${moduleName}: layout validation failed." `
                    -DiagnosticLog $diagnosticLog `
                    -ExpectedExitCodes @(1) `
                    -TimeoutSeconds 60
                $validated++
            }
            catch
            {
                # Validation failure is non-fatal — record and continue
                $validationWarnings.Add("${language}/${moduleName}: $($_.Exception.Message)")
            }
        }

        # Verify that Translator preserved the source SLT text after the
        # import/export round-trip.  This catches code-page or encoding
        # regressions before a corrupted .slg language pack is shipped.
        $sourceArchivePath = Join-Path $WorkspaceDir "translations\$language\$moduleName.slt"
        if (Test-Path -LiteralPath $sourceArchivePath)
        {
            $roundtripDir = Join-Path $WorkspaceDir "roundtrip\$language\$moduleName"
            New-Item -ItemType Directory -Path $roundtripDir -Force | Out-Null
            try
            {
                Invoke-SalamanderTranslatorQuiet `
                    -TranslatorExe $TranslatorExe `
                    -Arguments @("-quiet-export-slt-for-diff", $roundtripDir, $atpFile) `
                    -FailureMessage "$language/${moduleName}: SLT round-trip export failed." `
                    -DiagnosticLog $diagnosticLog `
                    -TimeoutSeconds 180 `
                    -WindowGraceSeconds 0

                $roundtripArchivePath = Join-Path $roundtripDir "$moduleName.slt"
                $expectedLangId = $languageLangIds[$language]
                $verifyArgs = @($sourceArchivePath, $roundtripArchivePath)
                if ($null -ne $expectedLangId)
                {
                    $verifyArgs += @("--expected-langid", $expectedLangId)
                }
                $verifyOutput = & python (Join-Path $scriptDir "verify_slt_roundtrip.py") @verifyArgs 2>&1
                if (-not $?)
                {
                    $details = ($verifyOutput | Select-Object -First 12) -join "`n    "
                    $roundtripFailures.Add("${language}/${moduleName}: SLT text changed after Translator import/export`n    $details")
                    continue
                }
            }
            catch
            {
                $roundtripFailures.Add("${language}/${moduleName}: $($_.Exception.Message)")
                continue
            }
        }

        # Determine destination in runtime tree
        if ($moduleName -eq "salamand")
        {
            $destDir = Join-Path $buildRootFull "lang"
        }
        else
        {
            $destDir = Join-Path $buildRootFull "plugins\$moduleName\lang"
        }

        if (-not (Test-Path -LiteralPath $destDir))
        {
            # Plugin directory doesn't exist in runtime — skip silently
            # (e.g., ftp, pictview archives exist but plugin doesn't build)
            continue
        }

        $destFile = Join-Path $destDir "$language.slg"
        $pendingCopies.Add([pscustomobject]@{ Source = $slgFile; Destination = $destFile })
    }
}

# Do not touch the runtime tree if any fatal validation failed.  Otherwise a
# failed build can still leave corrupted or partially rebuilt .slg files in
# BuildRoot, which makes the next manual Salamander smoke test misleading.
$fatalFailures = $copyFailures.Count + $roundtripFailures.Count + $coreSeedRejections.Count + $(if ($AllowSeedRejections) { 0 } else { $seedRejections.Count })
if ($fatalFailures -eq 0)
{
    foreach ($copy in $pendingCopies)
    {
        Copy-Item -LiteralPath $copy.Source -Destination $copy.Destination -Force
        $copied++
    }
}

# Summary
Write-Host ""
Write-Host "========================================"
Write-Host "  Language Pack Build Summary"
Write-Host "========================================"
Write-Host "  Languages:           $($requestedLanguages.Count)"
Write-Host "  .slg files copied:   $copied"
Write-Host "  Seed rejections:     $($seedRejections.Count)"
Write-Host "  Core rejections:     $($coreSeedRejections.Count)"
Write-Host "  Round-trip failures: $($roundtripFailures.Count)"

if (-not $SkipValidation)
{
    Write-Host "  Projects validated:  $validated"
    Write-Host "  Validation warnings: $($validationWarnings.Count)"
}

if ($seedRejections.Count -gt 0)
{
    Write-Host ""
    Write-Warning "Seed rejections (import failed, English seed not shipped):"
    foreach ($rejection in $seedRejections)
    {
        Write-Warning "  $rejection"
    }
}

if ($copyFailures.Count -gt 0)
{
    Write-Host ""
    Write-Warning "Copy failures:"
    foreach ($failure in $copyFailures)
    {
        Write-Warning "  $failure"
    }
}

if ($roundtripFailures.Count -gt 0)
{
    Write-Host ""
    Write-Warning "SLT round-trip failures (generated .slg does not preserve source translations):"
    foreach ($failure in $roundtripFailures)
    {
        Write-Warning "  $failure"
    }
}

if ($validationWarnings.Count -gt 0)
{
    Write-Host ""
    Write-Host "  Validation warnings (non-blocking):"
    foreach ($warning in $validationWarnings)
    {
        Write-Host "    $warning"
    }
}

# Verify
if ($fatalFailures -eq 0)
{
    $langDir = Join-Path $buildRootFull "lang"
    $coreLangs = Get-ChildItem -LiteralPath $langDir -Filter "*.slg" | ForEach-Object { $_.Name }
    Write-Host ""
    Write-Host "  Language packs now in runtime lang/: $($coreLangs -join ', ')"
}
else
{
    Write-Host ""
    Write-Warning "Language packs were not copied into the runtime tree because the build has fatal validation failures."
}

# Cleanup workspace
if (-not $env:SAMANDARIN_KEEP_LANGPACK_WORKSPACE)
{
    Remove-Item -Recurse -Force -LiteralPath $WorkspaceDir -ErrorAction SilentlyContinue
    Write-Host "  Workspace cleaned up."
}
else
{
    Write-Host "  Workspace preserved at: $WorkspaceDir"
}

# Fail the build if core Samandarin imports produced untranslated seeds. Plugin seed
# rejections may be tolerated for local builds and optional/incomplete plugins,
# but dropping the main application language packs makes the release incomplete.
if ($fatalFailures -gt 0)
{
    throw "Language pack build completed with $fatalFailures failure(s): $($seedRejections.Count) seed rejection(s), $($coreSeedRejections.Count) core seed rejection(s), $($roundtripFailures.Count) round-trip failure(s), $($copyFailures.Count) copy failure(s). The release zip has incomplete localization."
}
