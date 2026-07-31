<#
.SYNOPSIS
Packages each unpacked Open Salamander plugin and extension directory into a separate 7z archive.

.DESCRIPTION
The script expects a path to an unpacked Open Salamander x64 directory that contains
a plugins and/or extensions subdirectory. Ordinary plugin directories are discovered
directly under plugins. Runtime plugins are discovered one level deeper under the
plugins\extension-runtimes container. Extensions are discovered recursively by their
extension.json manifests, except extensions\demos, whose scripts form the single
salamatrixdemos package. Each package is packed as a whole directory, so an archive
contains e.g. automation\automation.spl, pythonruntime\pythonruntime.spl,
git-worktree-navigator\extension.json, or the complete demos directory instead of
only the directory contents at the archive root.

Archive names use the package directory name and version. Plugin versions are read
from the first plugin binary (*.spl preferred, then *.dll/*.exe); extension versions
are read from extension.json. All archive names use the existing plugin package format:
  <plugin>_<version>_x64.7z

.PARAMETER SalamanderPath
Path to an unpacked Open Salamander directory containing a plugins directory,
an extensions directory, or both.

.PARAMETER OutputPath
Directory where the resulting archives are written. Defaults to a
plugin-packages-x64 directory under the current working directory.

.PARAMETER SevenZipPath
Path to 7z.exe/7zz/7za. When omitted, the script searches PATH for 7z, 7zz, and 7za.

.PARAMETER Force
Overwrite existing archives.

.EXAMPLE
.\tools\package_salamander_plugins.ps1 -SalamanderPath C:\temp\salamander\Release_x64

.EXAMPLE
.\tools\package_salamander_plugins.ps1 -SalamanderPath C:\temp\salamander\Release_x64 -OutputPath C:\temp\plugin-packages -Force
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$SalamanderPath,

    [Parameter(Position = 1)]
    [ValidateNotNullOrEmpty()]
    [string]$OutputPath = (Join-Path (Get-Location) 'plugin-packages-x64'),

    [ValidateNotNullOrEmpty()]
    [string]$SevenZipPath,

    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExistingDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Description
    )

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    $item = Get-Item -LiteralPath $resolved.Path
    if (-not $item.PSIsContainer) {
        throw "$Description is not a directory: $Path"
    }

    return $item.FullName
}

function Resolve-SevenZip {
    param([string]$RequestedPath)

    if ($RequestedPath) {
        $command = Get-Command -LiteralPath $RequestedPath -ErrorAction Stop
        return $command.Source
    }

    foreach ($name in @('7z', '7zz', '7za')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($command) {
            return $command.Source
        }
    }

    throw '7-Zip command was not found. Install 7-Zip or pass -SevenZipPath.'
}

function Convert-ToArchiveSafeVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Version
    )

    $normalized = $Version.Trim()
    $normalized = $normalized -replace ',', '.'
    $normalized = $normalized -replace '\s+', '_'
    $normalized = $normalized -replace '[^0-9A-Za-z._-]', '_'
    $normalized = $normalized.Trim('._-')

    if (-not $normalized) {
        throw 'Version is empty after normalization.'
    }

    return $normalized
}

function Get-VersionStringFromFileVersionInfo {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.FileVersionInfo]$VersionInfo
    )

    if (-not [string]::IsNullOrWhiteSpace($VersionInfo.FileVersion)) {
        return $VersionInfo.FileVersion -replace '\s*\((?:x86|x64)\)\s*$', ''
    }

    if ($VersionInfo.FileMajorPart -ne 0 -or $VersionInfo.FileMinorPart -ne 0) {
        $version = '{0}.{1}' -f $VersionInfo.FileMajorPart, $VersionInfo.FileMinorPart
        if ($VersionInfo.FileBuildPart -ne 0) {
            $version = '{0}{1}' -f $version, $VersionInfo.FileBuildPart
        }

        return $version
    }

    throw 'File version metadata is empty.'
}

function Get-PluginVersion {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.DirectoryInfo]$PluginDirectory
    )

    $binary = Get-ChildItem -LiteralPath $PluginDirectory.FullName -File |
        Sort-Object @{ Expression = { if ($_.Extension -ieq '.spl') { 0 } elseif ($_.Extension -iin @('.dll', '.exe')) { 1 } else { 2 } } }, Name |
        Where-Object { $_.Extension -iin @('.spl', '.dll', '.exe') } |
        Select-Object -First 1

    if (-not $binary) {
        throw "Plugin '$($PluginDirectory.Name)' does not contain a .spl, .dll, or .exe file with version metadata."
    }

    $info = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($binary.FullName)
    try {
        $version = Get-VersionStringFromFileVersionInfo -VersionInfo $info
    }
    catch {
        throw "Cannot read plugin file version from '$($binary.FullName)': $_"
    }

    return Convert-ToArchiveSafeVersion -Version $version
}

function Get-ExtensionVersion {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.DirectoryInfo]$ExtensionDirectory
    )

    $manifestPath = Join-Path $ExtensionDirectory.FullName 'extension.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Extension '$($ExtensionDirectory.Name)' does not contain extension.json."
    }

    try {
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    }
    catch {
        throw "Cannot read extension manifest '$manifestPath': $_"
    }

    $versionProperty = $manifest.PSObject.Properties['version']
    if (-not $versionProperty -or
        $versionProperty.Value -isnot [string] -or
        [string]::IsNullOrWhiteSpace($versionProperty.Value)) {
        throw "Extension manifest '$manifestPath' does not contain a string version."
    }

    try {
        return Convert-ToArchiveSafeVersion -Version $versionProperty.Value
    }
    catch {
        throw "Cannot use extension version from '$manifestPath': $_"
    }
}

function Get-ExtensionBundleVersion {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.DirectoryInfo]$ExtensionBundleDirectory
    )

    $manifests = @(
        Get-ChildItem -LiteralPath $ExtensionBundleDirectory.FullName `
            -File -Filter 'extension.json' -Recurse |
            Sort-Object FullName
    )
    if ($manifests.Count -eq 0) {
        throw "Extension bundle '$($ExtensionBundleDirectory.Name)' does not contain any extension.json manifests."
    }

    $versions = @(
        $manifests |
            ForEach-Object { Get-ExtensionVersion -ExtensionDirectory $_.Directory } |
            Sort-Object -Unique
    )
    if ($versions.Count -ne 1) {
        throw "Extension bundle '$($ExtensionBundleDirectory.Name)' contains inconsistent versions: $($versions -join ', ')."
    }

    return $versions[0]
}

function New-PackageDefinition {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [ValidateSet('plugin', 'extension')]
        [string]$PackageType,

        [Parameter(Mandatory = $true)]
        [System.IO.DirectoryInfo]$Directory,

        [string]$PackageId,

        [switch]$ExtensionBundle
    )

    if ([string]::IsNullOrWhiteSpace($PackageId)) {
        $PackageId = $Directory.Name
    }

    return [PSCustomObject]@{
        Root = $Root
        PackageType = $PackageType
        Directory = $Directory
        PackageId = $PackageId
        ExtensionBundle = [bool]$ExtensionBundle
    }
}

function Get-PackageDefinitions {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SalamanderRoot
    )

    $packages = @()
    $pluginsRoot = Join-Path $SalamanderRoot 'plugins'
    if (Test-Path -LiteralPath $pluginsRoot -PathType Container) {
        $pluginsRoot = (Get-Item -LiteralPath $pluginsRoot).FullName
        foreach ($pluginDirectory in @(Get-ChildItem -LiteralPath $pluginsRoot -Directory | Sort-Object Name)) {
            if ($pluginDirectory.Name -ieq 'extension-runtimes') {
                $runtimeDirectories = @(
                    Get-ChildItem -LiteralPath $pluginDirectory.FullName -Directory |
                        Sort-Object Name
                )
                if ($runtimeDirectories.Count -eq 0) {
                    throw "Plugin container '$($pluginDirectory.FullName)' does not contain any runtime plugin directories."
                }

                foreach ($runtimeDirectory in $runtimeDirectories) {
                    $packages += New-PackageDefinition `
                        -Root $pluginDirectory.FullName `
                        -PackageType 'plugin' `
                        -Directory $runtimeDirectory
                }
            }
            else {
                $packages += New-PackageDefinition `
                    -Root $pluginsRoot `
                    -PackageType 'plugin' `
                    -Directory $pluginDirectory
            }
        }
    }

    $extensionsRoot = Join-Path $SalamanderRoot 'extensions'
    if (Test-Path -LiteralPath $extensionsRoot -PathType Container) {
        $extensionsRoot = (Get-Item -LiteralPath $extensionsRoot).FullName
        $demosDirectory = Get-Item -LiteralPath (Join-Path $extensionsRoot 'demos') `
            -ErrorAction SilentlyContinue
        $demosPrefix = $null
        if ($demosDirectory -and $demosDirectory.PSIsContainer) {
            $demosPrefix = $demosDirectory.FullName.TrimEnd('\') + '\'
            $packages += New-PackageDefinition `
                -Root $extensionsRoot `
                -PackageType 'extension' `
                -Directory $demosDirectory `
                -PackageId 'salamatrixdemos' `
                -ExtensionBundle
        }

        $extensionManifests = @(
            Get-ChildItem -LiteralPath $extensionsRoot -File -Filter 'extension.json' -Recurse |
                Sort-Object FullName
        )
        foreach ($manifest in $extensionManifests) {
            if ($demosPrefix -and
                $manifest.FullName.StartsWith(
                    $demosPrefix,
                    [System.StringComparison]::OrdinalIgnoreCase)) {
                continue
            }

            $extensionDirectory = $manifest.Directory
            $packages += New-PackageDefinition `
                -Root $extensionDirectory.Parent.FullName `
                -PackageType 'extension' `
                -Directory $extensionDirectory
        }
    }

    return $packages
}

$salamanderRoot = Resolve-ExistingDirectory -Path $SalamanderPath -Description 'Salamander path'
$sevenZip = Resolve-SevenZip -RequestedPath $SevenZipPath
$outputRoot = New-Item -ItemType Directory -Path $OutputPath -Force

$packages = @(Get-PackageDefinitions -SalamanderRoot $salamanderRoot)
if ($packages.Count -eq 0) {
    throw "No plugin or extension directories found under '$salamanderRoot'."
}

$duplicatePackageNames = @(
    $packages |
        Group-Object PackageId |
        Where-Object Count -gt 1
)
if ($duplicatePackageNames.Count -gt 0) {
    $names = ($duplicatePackageNames.Name | Sort-Object) -join ', '
    throw "Package directory names must be unique because they form archive names. Duplicates: $names"
}

$archiveCount = 0
foreach ($package in $packages) {
    $packageDirectory = $package.Directory
    if ($package.ExtensionBundle) {
        $version = Get-ExtensionBundleVersion -ExtensionBundleDirectory $packageDirectory
    }
    elseif ($package.PackageType -eq 'extension') {
        $version = Get-ExtensionVersion -ExtensionDirectory $packageDirectory
    }
    else {
        $version = Get-PluginVersion -PluginDirectory $packageDirectory
    }

    $archiveName = 'plugin_5.0_{0}_{1}_x64.7z' -f $package.PackageId, $version
    $archivePath = Join-Path $outputRoot.FullName $archiveName

    if ((Test-Path -LiteralPath $archivePath) -and -not $Force) {
        throw "Archive already exists: $archivePath. Use -Force to overwrite it."
    }

    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }

    Write-Host "Packing $($package.PackageType) $($package.PackageId) -> $archiveName"
    Push-Location -LiteralPath $package.Root
    try {
        & $sevenZip a -t7z -mx=9 $archivePath $packageDirectory.Name | Write-Host
        if ($LASTEXITCODE -ne 0) {
            throw "7-Zip failed for $($package.PackageType) '$($package.PackageId)' with exit code $LASTEXITCODE."
        }
    }
    finally {
        Pop-Location
    }

    $archiveCount++
}

Write-Host "Created $archiveCount archive(s) in $($outputRoot.FullName)."
