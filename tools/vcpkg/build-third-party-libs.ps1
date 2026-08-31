[CmdletBinding()]
param(
    [string]$Triplet = 'x64-windows',
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$OutputDir,
    [string]$PrebuiltDllsDir,
    [switch]$NoBootstrap,
    [switch]$SkipInstall,
    [switch]$SftpPlugin,
    [switch]$OnlySftpPlugin,
    [switch]$OnlyMmviewerTagLib,
    [string[]]$ManifestFeature = @(),
    [switch]$NoDefaultFeatures,
    [switch]$SkipLegacyCopy
)

$ErrorActionPreference = 'Stop'

$VcpkgRepository = 'https://github.com/microsoft/vcpkg.git'
$VcpkgBaseline = 'a0b1c8d3a477c1cb4813d8e127a56961707ca42b'
$RequiredDlls = @('unrar.dll', 'libeay32.dll', 'ssleay32.dll', 'dbghelp.dll')

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

if (![string]::IsNullOrWhiteSpace($PrebuiltDllsDir))
{
    $PrebuiltDllsDir = Resolve-FullPath $PrebuiltDllsDir
    Write-Host "Prebuilt DLLs: $PrebuiltDllsDir"
}

$installRoot = Join-Path $repoRoot ($OnlyMmviewerTagLib ? 'build\vcpkg_installed_mmviewer_taglib' : 'build\vcpkg_installed_third_party')
$tripletBinDir = Join-Path (Join-Path $installRoot $Triplet) 'bin'
$vcpkgExe = Join-Path $VcpkgRoot 'vcpkg.exe'

Write-Host "Repository root: $repoRoot"
Write-Host "vcpkg root:     $VcpkgRoot"
if (!$OnlySftpPlugin)
{
    Write-Host "Manifest root:  $manifestRoot"
    Write-Host "Output dir:     $OutputDir"
}
Write-Host "Triplet:        $Triplet"

if (!(Test-Path -LiteralPath $VcpkgRoot))
{
    Write-Host "Cloning vcpkg into $VcpkgRoot"
    Invoke-LoggedCommand -FilePath 'git' -Arguments @('clone', $VcpkgRepository, $VcpkgRoot)
}

$vcpkgIsGitCheckout = Test-Path -LiteralPath (Join-Path $VcpkgRoot '.git')

if ($vcpkgIsGitCheckout)
{
    Invoke-LoggedCommand -FilePath 'git' -Arguments @('fetch', '--tags', '--prune', 'origin') -WorkingDirectory $VcpkgRoot
    Invoke-LoggedCommand -FilePath 'git' -Arguments @('checkout', $VcpkgBaseline) -WorkingDirectory $VcpkgRoot
}
elseif (Test-Path -LiteralPath $vcpkgExe)
{
    Write-Warning "Using existing non-git vcpkg root: $VcpkgRoot"
}
else
{
    throw "VcpkgRoot is neither a git checkout nor an existing vcpkg installation: $VcpkgRoot"
}

if ((!$NoBootstrap -and $vcpkgIsGitCheckout) -or !(Test-Path -LiteralPath $vcpkgExe))
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

if (!$OnlySftpPlugin)
{
    if (!$SkipInstall)
    {
        $installArguments = @(
            'install',
            '--triplet', $Triplet,
            '--x-install-root', $installRoot
        )
        if ($OnlyMmviewerTagLib)
        {
            $installArguments += @('--overlay-triplets', (Join-Path $PSScriptRoot 'triplets'))
        }
        if ($NoDefaultFeatures)
        {
            $installArguments += '--x-no-default-features'
        }
        foreach ($feature in $ManifestFeature)
        {
            $installArguments += "--x-feature=$feature"
        }
        $legacyOpenSslRequested = !$OnlyMmviewerTagLib -and
            (($NoDefaultFeatures -and ('ftp-openssl' -in $ManifestFeature)) -or !$NoDefaultFeatures)
        try
        {
            Invoke-LoggedCommand -FilePath $vcpkgExe -Arguments $installArguments -WorkingDirectory $manifestRoot
        }
        catch
        {
            if (!$legacyOpenSslRequested)
            {
                throw
            }

            # The pinned OpenSSL 1.0.2 port applies patches to its extracted source.
            # A failed/interrupted patch can leave that source tree already modified.
            # Remove only that port's buildtree and retry; vcpkg still supplies the
            # pinned legacy OpenSSL package and downloaded archives are preserved.
            $legacyOpenSslBuildTree = Join-Path $VcpkgRoot 'buildtrees\openssl'
            if (Test-Path -LiteralPath $legacyOpenSslBuildTree)
            {
                Write-Warning "Legacy OpenSSL install failed; removing stale buildtree and retrying: $legacyOpenSslBuildTree"
                Remove-Item -LiteralPath $legacyOpenSslBuildTree -Recurse -Force
                Invoke-LoggedCommand -FilePath $vcpkgExe -Arguments $installArguments -WorkingDirectory $manifestRoot
            }
            else
            {
                throw
            }
        }
    }

    if (!$SkipLegacyCopy -and !(Test-Path -LiteralPath $tripletBinDir))
    {
        throw "vcpkg bin directory does not exist: $tripletBinDir"
    }

    if (!$SkipLegacyCopy)
    {
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
                    # Not produced by vcpkg - try prebuilt DLLs directory
                    if (![string]::IsNullOrWhiteSpace($PrebuiltDllsDir))
                    {
                        $prebuilt = Join-Path $PrebuiltDllsDir $dllName
                        if (Test-Path -LiteralPath $prebuilt)
                        {
                            $source = $prebuilt
                        }
                        elseif (Test-Path -LiteralPath (Join-Path $OutputDir $dllName))
                        {
                            Write-Warning "DLL '$dllName' not in vcpkg or prebuilt dir, keeping existing copy in output dir"
                            continue
                        }
                        else
                        {
                            throw "Required DLL was not produced by vcpkg and not found in prebuilt dir: $dllName"
                        }
                    }
                    else
                    {
                        # Fallback: some DLLs are standard Windows system DLLs
                        $systemDll = Join-Path "$env:SystemRoot\System32" $dllName
                        if (Test-Path -LiteralPath $systemDll)
                        {
                            Write-Host "Using system DLL: $systemDll"
                            $source = $systemDll
                        }
                        elseif (Test-Path -LiteralPath (Join-Path $OutputDir $dllName))
                        {
                            Write-Warning "DLL '$dllName' not produced by vcpkg, keeping existing copy in output dir"
                            continue
                        }
                        else
                        {
                            throw "Required DLL was not produced by vcpkg: $dllName"
                        }
                    }
                }
            }

            $destination = Join-Path $OutputDir $dllName
            Copy-Item -LiteralPath $source -Destination $destination -Force
            Write-Host "Copied $source -> $destination"
        }

        Write-Host "Done. Third-party DLLs are available in $OutputDir"
    }
}

# --- SFTP plugin dependencies (libssh2 + OpenSSL 3.x) ---

if ($SftpPlugin)
{
    $sftpManifestRoot = Join-Path $PSScriptRoot 'sftp-plugin'
    $sftpInstallRoot = Join-Path $repoRoot 'build\vcpkg_installed_sftp'

    Write-Host ""
    Write-Host "=== SFTP plugin dependencies ==="
    Write-Host "Manifest:  $sftpManifestRoot"
    Write-Host "Install:   $sftpInstallRoot"
    Write-Host "Triplet:   $Triplet"

    if (!$SkipInstall)
    {
        Invoke-LoggedCommand -FilePath $vcpkgExe -Arguments @(
            'install',
            '--triplet', $Triplet,
            '--x-install-root', $sftpInstallRoot
        ) -WorkingDirectory $sftpManifestRoot
    }

    $sftpTripletDir = Join-Path $sftpInstallRoot $Triplet
    $sftpBinDir = Join-Path $sftpTripletDir 'bin'
    $sftpLibDir = Join-Path $sftpTripletDir 'lib'
    $sftpIncludeDir = Join-Path $sftpTripletDir 'include'

    Write-Host ""
    Write-Host "SFTP plugin libraries:"
    if (Test-Path -LiteralPath $sftpBinDir)
    {
        Get-ChildItem -LiteralPath $sftpBinDir -Filter '*.dll' | ForEach-Object {
            Write-Host "  $($_.Name)"
        }
    }
    if (Test-Path -LiteralPath $sftpLibDir)
    {
        Get-ChildItem -LiteralPath $sftpLibDir -Filter '*.lib' | ForEach-Object {
            Write-Host "  $($_.Name)"
        }
    }

    Write-Host ""
    Write-Host "Done. SFTP plugin dependencies installed in $sftpInstallRoot"
}
