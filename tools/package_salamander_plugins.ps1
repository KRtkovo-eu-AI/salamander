<#
.SYNOPSIS
Packages each unpacked Open Salamander plugin directory into a separate 7z archive.

.DESCRIPTION
The script expects a path to an unpacked Open Salamander x64 directory that contains
an immediate plugins subdirectory. Every immediate child directory under plugins is
packed as a whole directory, so the archive contains e.g. automation\automation.spl
instead of only automation.spl at the archive root.

Archive names use the plugin directory name, the version read from the first plugin
binary (*.spl preferred, then *.dll/*.exe), and the x64 suffix:
  <plugin>_<version>_x64.7z

.PARAMETER SalamanderPath
Path to an unpacked Open Salamander directory containing the plugins directory.

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
    $normalized = $normalized -replace '\s+', ''
    $normalized = $normalized -replace '[^0-9A-Za-z._-]', '_'
    $normalized = $normalized.Trim('._-')

    if (-not $normalized) {
        throw 'Version is empty after normalization.'
    }

    return $normalized
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
    $version = if ($info.ProductVersion) { $info.ProductVersion } else { $info.FileVersion }
    if (-not $version) {
        throw "Cannot read version metadata from '$($binary.FullName)'."
    }

    return Convert-ToArchiveSafeVersion -Version $version
}

$salamanderRoot = Resolve-ExistingDirectory -Path $SalamanderPath -Description 'Salamander path'
$pluginsRoot = Join-Path $salamanderRoot 'plugins'
$pluginsRoot = Resolve-ExistingDirectory -Path $pluginsRoot -Description 'Plugins path'
$sevenZip = Resolve-SevenZip -RequestedPath $SevenZipPath
$outputRoot = New-Item -ItemType Directory -Path $OutputPath -Force

$pluginDirectories = Get-ChildItem -LiteralPath $pluginsRoot -Directory | Sort-Object Name
if ($pluginDirectories.Count -eq 0) {
    throw "No plugin directories found in '$pluginsRoot'."
}

foreach ($pluginDirectory in $pluginDirectories) {
    $version = Get-PluginVersion -PluginDirectory $pluginDirectory
    $archiveName = '{0}_{1}_x64.7z' -f $pluginDirectory.Name, $version
    $archivePath = Join-Path $outputRoot.FullName $archiveName

    if ((Test-Path -LiteralPath $archivePath) -and -not $Force) {
        throw "Archive already exists: $archivePath. Use -Force to overwrite it."
    }

    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }

    Write-Host "Packing $($pluginDirectory.Name) -> $archiveName"
    Push-Location -LiteralPath $pluginsRoot
    try {
        & $sevenZip a -t7z -mx=9 $archivePath $pluginDirectory.Name | Write-Host
        if ($LASTEXITCODE -ne 0) {
            throw "7-Zip failed for plugin '$($pluginDirectory.Name)' with exit code $LASTEXITCODE."
        }
    }
    finally {
        Pop-Location
    }
}

Write-Host "Created $($pluginDirectories.Count) archive(s) in $($outputRoot.FullName)."
