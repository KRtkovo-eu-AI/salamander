[CmdletBinding()]
param(
    [string]$Triplet = 'x64-windows',
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$OutputDir,
    [switch]$NoBootstrap,
    [switch]$SkipInstall
)

$ErrorActionPreference = 'Stop'

$VcpkgRepository = 'https://github.com/microsoft/vcpkg.git'
$VcpkgBaseline = 'a0b1c8d3a477c1cb4813d8e127a56961707ca42b'
$RequiredDlls = @('unrar.dll', 'libeay32.dll', 'ssleay32.dll')

function Resolve-FullPath
{
    param([string]$PathValue)

    return [System.IO.Path]::GetFullPath($PathValue)
}

function Invoke-LoggedCommand
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [string[]]$Arguments = @(),

        [string]$WorkingDirectory
    )

    $previousLocation = $null
    if (![string]::IsNullOrWhiteSpace($WorkingDirectory))
    {
        $previousLocation = Get-Location
        Set-Location -LiteralPath $WorkingDirectory
    }

    try
    {
        Write-Host "> $FilePath $($Arguments -join ' ')"
        & $FilePath @Arguments
        if ($LASTEXITCODE -ne 0)
        {
            throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
        }
    }
    finally
    {
        if ($null -ne $previousLocation)
        {
            Set-Location -LiteralPath $previousLocation
        }
    }
}

$repoRoot = Resolve-FullPath (Join-Path $PSScriptRoot '..\..')
$manifestRoot = Join-Path $PSScriptRoot 'third-party-libs'

if ([string]::IsNullOrWhiteSpace($VcpkgRoot))
{
    $VcpkgRoot = Join-Path $repoRoot 'build\vcpkg'
}
$VcpkgRoot = Resolve-FullPath $VcpkgRoot

if ([string]::IsNullOrWhiteSpace($OutputDir))
{
    $OutputDir = Join-Path $repoRoot 'build\libs'
}
$OutputDir = Resolve-FullPath $OutputDir

$installRoot = Join-Path $repoRoot 'build\vcpkg_installed_third_party'
$tripletBinDir = Join-Path (Join-Path $installRoot $Triplet) 'bin'
$vcpkgExe = Join-Path $VcpkgRoot 'vcpkg.exe'

Write-Host "Repository root: $repoRoot"
Write-Host "vcpkg root:     $VcpkgRoot"
Write-Host "Manifest root:  $manifestRoot"
Write-Host "Triplet:        $Triplet"
Write-Host "Output dir:     $OutputDir"

if (!(Test-Path -LiteralPath $VcpkgRoot))
{
    Write-Host "Cloning vcpkg into $VcpkgRoot"
    Invoke-LoggedCommand -FilePath 'git' -Arguments @('clone', $VcpkgRepository, $VcpkgRoot)
}

if (!(Test-Path -LiteralPath (Join-Path $VcpkgRoot '.git')))
{
    throw "VcpkgRoot does not look like a git checkout: $VcpkgRoot"
}

Invoke-LoggedCommand -FilePath 'git' -Arguments @('fetch', '--tags', '--prune', 'origin') -WorkingDirectory $VcpkgRoot
Invoke-LoggedCommand -FilePath 'git' -Arguments @('checkout', $VcpkgBaseline) -WorkingDirectory $VcpkgRoot

if (!$NoBootstrap -or !(Test-Path -LiteralPath $vcpkgExe))
{
    $bootstrap = Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat'
    if (!(Test-Path -LiteralPath $bootstrap))
    {
        throw "Unable to find vcpkg bootstrap script: $bootstrap"
    }

    Invoke-LoggedCommand -FilePath $bootstrap -Arguments @('-disableMetrics') -WorkingDirectory $VcpkgRoot
}

if (!(Test-Path -LiteralPath $vcpkgExe))
{
    throw "Unable to find vcpkg executable: $vcpkgExe"
}

if (!$SkipInstall)
{
    Invoke-LoggedCommand -FilePath $vcpkgExe -Arguments @(
        'install',
        '--triplet', $Triplet,
        '--x-install-root', $installRoot
    ) -WorkingDirectory $manifestRoot
}

if (!(Test-Path -LiteralPath $tripletBinDir))
{
    throw "vcpkg bin directory does not exist: $tripletBinDir"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

foreach ($dllName in $RequiredDlls)
{
    $source = Join-Path $tripletBinDir $dllName
    if (!(Test-Path -LiteralPath $source))
    {
        $matches = @(Get-ChildItem -LiteralPath (Join-Path $installRoot $Triplet) -Recurse -File -Filter $dllName -ErrorAction SilentlyContinue)
        if ($matches.Count -eq 1)
        {
            $source = $matches[0].FullName
        }
        elseif ($matches.Count -gt 1)
        {
            throw "Found multiple candidates for ${dllName}: $($matches.FullName -join ', ')"
        }
        else
        {
            throw "Required DLL was not produced by vcpkg: $dllName"
        }
    }

    $destination = Join-Path $OutputDir $dllName
    Copy-Item -LiteralPath $source -Destination $destination -Force
    Write-Host "Copied $source -> $destination"
}

Write-Host "Done. Third-party DLLs are available in $OutputDir"
