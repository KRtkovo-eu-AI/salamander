<#
.SYNOPSIS
Interactively builds selected Open Salamander dependencies and deploys them
into the matching Salamander payload layout.

.DESCRIPTION
Without -Dependency or -All, the script displays a menu before doing any work.
For automation, pass one or more dependency IDs with -Dependency, or use -All.

The default payload directory is:

  %OPENSAL_BUILD_DIR%\salamander\<Configuration>_<platform>

Use -PayloadDir to deploy into a different already-built or staging payload.
The script never starts Salamander and never deletes files from the payload.

.EXAMPLE
.\tools\vcpkg\build-and-deploy-dependencies.ps1

.EXAMPLE
.\tools\vcpkg\build-and-deploy-dependencies.ps1 -Dependency unrar,lua

.EXAMPLE
.\tools\vcpkg\build-and-deploy-dependencies.ps1 -All -Triplet x64-windows

.EXAMPLE
.\tools\vcpkg\build-and-deploy-dependencies.ps1 -Dependency lua -SkipInstall
#>
[CmdletBinding()]
param(
    [ValidateSet('unrar', 'ftp-openssl', 'lua', 'sftp', 'dbghelp')]
    [string[]]$Dependency = @(),

    [switch]$All,

    [ValidateSet('x86-windows', 'x64-windows', 'arm64-windows')]
    [string]$Triplet = 'x64-windows',

    [string]$Configuration = 'Release',
    [string]$OpenSalBuildDir = $env:OPENSAL_BUILD_DIR,
    [string]$PayloadDir,
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$PrebuiltDllsDir,
    [switch]$NoBootstrap,
    [switch]$SkipInstall
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$platformByTriplet = @{
    'x86-windows' = 'x86'
    'x64-windows' = 'x64'
    'arm64-windows' = 'arm64'
}
$shortPlatform = $platformByTriplet[$Triplet]
$knownDependencies = @('unrar', 'ftp-openssl', 'lua', 'sftp', 'dbghelp')
$displayNames = @{
    'unrar' = 'UnRAR plug-in (unrar.dll)'
    'ftp-openssl' = 'FTP legacy OpenSSL (libeay32.dll, ssleay32.dll)'
    'lua' = 'Salamatrix Lua Runtime (lua.exe, lua.dll, MIT notice)'
    'sftp' = 'SFTP plug-in (libssh2, OpenSSL 3, zlib runtime)'
    'dbghelp' = 'Windows Debug Help (dbghelp.dll; staging only, not vcpkg)'
}

function Resolve-FullPath {
    param([Parameter(Mandatory = $true)][string]$PathValue)
    return [IO.Path]::GetFullPath($PathValue)
}

function Select-Dependencies {
    Write-Host ''
    Write-Host 'Select dependencies to build and deploy:'
    for ($index = 0; $index -lt $knownDependencies.Count; $index++) {
        $id = $knownDependencies[$index]
        Write-Host ("  [{0}] {1}" -f ($index + 1), $displayNames[$id])
    }
    Write-Host '  [A] All'
    Write-Host '  [Q] Quit'
    Write-Host ''

    while ($true) {
        $answer = (Read-Host 'Selection (comma-separated, for example 1,3,4)').Trim()
        if ($answer -match '^(?i:q|quit)$') {
            return @()
        }
        if ($answer -match '^(?i:a|all)$') {
            return $knownDependencies
        }

        $selected = [Collections.Generic.List[string]]::new()
        $valid = $true
        foreach ($token in @($answer -split '[,; ]+' | Where-Object { $_ })) {
            $number = 0
            if (![int]::TryParse($token, [ref]$number) -or
                $number -lt 1 -or $number -gt $knownDependencies.Count) {
                $valid = $false
                break
            }
            $id = $knownDependencies[$number - 1]
            if (!$selected.Contains($id)) {
                $selected.Add($id)
            }
        }
        if ($valid -and $selected.Count -gt 0) {
            return $selected.ToArray()
        }
        Write-Warning 'Invalid selection. Enter one or more numbers, A, or Q.'
    }
}

function Copy-DependencyFile {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$RelativeDestination
    )

    if (!(Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "Dependency output does not exist: $Source"
    }
    $destination = Join-Path $PayloadDir $RelativeDestination
    $destinationDirectory = Split-Path -Parent $destination
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $destination -Force
    Write-Host "  $RelativeDestination"
    $script:deployedFiles.Add($destination)
}

function Resolve-SingleFile {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Filter,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $matches = @(Get-ChildItem -LiteralPath $Directory -File -Filter $Filter -ErrorAction SilentlyContinue)
    if ($matches.Count -ne 1) {
        throw "Expected one $Description in '$Directory' matching '$Filter', found $($matches.Count)."
    }
    return $matches[0].FullName
}

if ($All -and $Dependency.Count -gt 0) {
    throw 'Use either -All or -Dependency, not both.'
}
if ($All) {
    $Dependency = $knownDependencies
}
elseif ($Dependency.Count -eq 0) {
    $Dependency = @(Select-Dependencies)
    if ($Dependency.Count -eq 0) {
        Write-Host 'Nothing selected.'
        return
    }
}
$Dependency = @($knownDependencies | Where-Object { $_ -in $Dependency })

if ([string]::IsNullOrWhiteSpace($PayloadDir)) {
    if ([string]::IsNullOrWhiteSpace($OpenSalBuildDir)) {
        throw 'Set OPENSAL_BUILD_DIR, pass -OpenSalBuildDir, or pass -PayloadDir.'
    }
    $PayloadDir = Join-Path $OpenSalBuildDir ("salamander\{0}_{1}" -f $Configuration, $shortPlatform)
}
$PayloadDir = Resolve-FullPath $PayloadDir

Write-Host ''
Write-Host "Triplet: $Triplet"
Write-Host "Payload: $PayloadDir"
Write-Host 'Selected:'
foreach ($id in $Dependency) {
    Write-Host "  - $($displayNames[$id])"
}
Write-Host ''

$mainFeatures = [Collections.Generic.List[string]]::new()
if ('unrar' -in $Dependency) { $mainFeatures.Add('unrar-runtime') }
if ('ftp-openssl' -in $Dependency) { $mainFeatures.Add('ftp-openssl') }
if ('lua' -in $Dependency) { $mainFeatures.Add('lua-runtime') }
$buildSftp = 'sftp' -in $Dependency

# Manifest mode reconciles an install root with the selected feature set. Keep
# previously installed groups enabled so a partial run never removes artifacts
# needed by another provider's subsequent build.
$existingMainInstall = Join-Path $repoRoot "build\vcpkg_installed_third_party\$Triplet"
if ($mainFeatures.Count -gt 0) {
    if ((Test-Path -LiteralPath (Join-Path $existingMainInstall 'bin\unrar.dll')) -and
        !$mainFeatures.Contains('unrar-runtime')) {
        $mainFeatures.Add('unrar-runtime')
    }
    if ((Test-Path -LiteralPath (Join-Path $existingMainInstall 'bin\libeay32.dll')) -and
        (Test-Path -LiteralPath (Join-Path $existingMainInstall 'bin\ssleay32.dll')) -and
        !$mainFeatures.Contains('ftp-openssl')) {
        $mainFeatures.Add('ftp-openssl')
    }
    if ((Test-Path -LiteralPath (Join-Path $existingMainInstall 'tools\lua\lua.exe')) -and
        (Test-Path -LiteralPath (Join-Path $existingMainInstall 'tools\lua\lua.dll')) -and
        !$mainFeatures.Contains('lua-runtime')) {
        $mainFeatures.Add('lua-runtime')
    }
}

if ($mainFeatures.Count -gt 0 -or $buildSftp) {
    $buildScript = Join-Path $PSScriptRoot 'build-third-party-libs.ps1'
    $buildArguments = @{
        Triplet = $Triplet
        SkipLegacyCopy = $true
    }
    if (![string]::IsNullOrWhiteSpace($VcpkgRoot)) {
        $buildArguments.VcpkgRoot = $VcpkgRoot
    }
    if ($NoBootstrap) { $buildArguments.NoBootstrap = $true }
    if ($SkipInstall) { $buildArguments.SkipInstall = $true }
    if ($mainFeatures.Count -gt 0) {
        $buildArguments.ManifestFeature = $mainFeatures.ToArray()
        $buildArguments.NoDefaultFeatures = $true
    }
    else {
        $buildArguments.OnlySftpPlugin = $true
    }
    if ($buildSftp) { $buildArguments.SftpPlugin = $true }

    & $buildScript @buildArguments
    if (!$?) {
        throw 'Dependency build failed.'
    }
}

$mainInstall = Join-Path $repoRoot "build\vcpkg_installed_third_party\$Triplet"
$sftpInstall = Join-Path $repoRoot "build\vcpkg_installed_sftp\$Triplet"
$script:deployedFiles = [Collections.Generic.List[string]]::new()

Write-Host ''
Write-Host 'Deploying:'
if ('unrar' -in $Dependency) {
    Copy-DependencyFile `
        -Source (Join-Path $mainInstall 'bin\unrar.dll') `
        -RelativeDestination 'plugins\unrar\unrar.dll'
}
if ('ftp-openssl' -in $Dependency) {
    Copy-DependencyFile `
        -Source (Join-Path $mainInstall 'bin\libeay32.dll') `
        -RelativeDestination 'utils\libeay32.dll'
    Copy-DependencyFile `
        -Source (Join-Path $mainInstall 'bin\ssleay32.dll') `
        -RelativeDestination 'utils\ssleay32.dll'
}
if ('lua' -in $Dependency) {
    Copy-DependencyFile `
        -Source (Join-Path $mainInstall 'tools\lua\lua.exe') `
        -RelativeDestination 'plugins\extension-runtimes\luaruntime\runtime\lua.exe'
    Copy-DependencyFile `
        -Source (Join-Path $mainInstall 'tools\lua\lua.dll') `
        -RelativeDestination 'plugins\extension-runtimes\luaruntime\runtime\lua.dll'
    Copy-DependencyFile `
        -Source (Join-Path $mainInstall 'share\lua\copyright') `
        -RelativeDestination 'plugins\extension-runtimes\luaruntime\runtime\LICENSE-LUA.txt'
}
if ($buildSftp) {
    $sftpBin = Join-Path $sftpInstall 'bin'
    $sftpCrypto = Resolve-SingleFile $sftpBin 'libcrypto-3*.dll' 'OpenSSL runtime DLL'
    Copy-DependencyFile `
        -Source (Join-Path $sftpBin 'libssh2.dll') `
        -RelativeDestination 'plugins\sftp\libssh2.dll'
    Copy-DependencyFile `
        -Source $sftpCrypto `
        -RelativeDestination ('plugins\sftp\' + (Split-Path -Leaf $sftpCrypto))
    Copy-DependencyFile `
        -Source (Join-Path $sftpBin 'z.dll') `
        -RelativeDestination 'plugins\sftp\z.dll'
}
if ('dbghelp' -in $Dependency) {
    $dbghelpCandidates = [Collections.Generic.List[string]]::new()
    if (![string]::IsNullOrWhiteSpace($PrebuiltDllsDir)) {
        $dbghelpCandidates.Add((Join-Path $PrebuiltDllsDir 'dbghelp.dll'))
    }
    if ($Triplet -eq 'x86-windows') {
        $dbghelpCandidates.Add((Join-Path $env:SystemRoot 'SysWOW64\dbghelp.dll'))
    }
    elseif ($Triplet -eq 'x64-windows') {
        $dbghelpCandidates.Add((Join-Path $repoRoot 'build\libs\dbghelp.dll'))
        $dbghelpCandidates.Add((Join-Path $env:SystemRoot 'System32\dbghelp.dll'))
    }
    elseif ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') {
        $dbghelpCandidates.Add((Join-Path $env:SystemRoot 'System32\dbghelp.dll'))
    }
    $dbghelpSource = $dbghelpCandidates | Where-Object {
        Test-Path -LiteralPath $_ -PathType Leaf
    } | Select-Object -First 1
    if (!$dbghelpSource) {
        throw 'No architecture-compatible dbghelp.dll found. Pass -PrebuiltDllsDir.'
    }
    Copy-DependencyFile `
        -Source $dbghelpSource `
        -RelativeDestination 'utils\dbghelp.dll'
}

Write-Host ''
Write-Host "Done. Deployed $($deployedFiles.Count) file(s) into $PayloadDir"
