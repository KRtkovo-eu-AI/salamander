[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [Parameter(Position = 1)]
    [ValidateSet('Win32', 'x86', 'x64')]
    [string]$Platform = 'x64',

    [string]$BuildDir
)

$ErrorActionPreference = 'Stop'

function Resolve-BuildPlatform
{
    param(
        [string]$Name
    )

    switch ($Name.ToLowerInvariant())
    {
        'win32' { return 'Win32' }
        'x86' { return 'Win32' }
        'x64' { return 'x64' }
        default { throw "Unsupported platform: $Name" }
    }
}

function Normalize-BuildDir
{
    param(
        [string]$PathValue
    )

    $fullPath = [System.IO.Path]::GetFullPath($PathValue)
    if (!$fullPath.EndsWith('\') -and !$fullPath.EndsWith('/'))
    {
        $fullPath += '\'
    }
    return $fullPath
}

$repoRoot = $PSScriptRoot
$vcxprojDir = Join-Path $repoRoot 'src\vcxproj'
$buildCmd = Join-Path $vcxprojDir 'build.cmd'

if (!(Test-Path -LiteralPath $buildCmd))
{
    throw "build.cmd not found at $buildCmd"
}

$resolvedPlatform = Resolve-BuildPlatform -Name $Platform

if ([string]::IsNullOrWhiteSpace($BuildDir))
{
    $BuildDir = Join-Path $repoRoot '.localbuild'
}

$env:OPENSAL_BUILD_DIR = Normalize-BuildDir -PathValue $BuildDir

Write-Host "Building $Configuration/$resolvedPlatform"
Write-Host "OPENSAL_BUILD_DIR=$env:OPENSAL_BUILD_DIR"

Push-Location $vcxprojDir
try
{
    & cmd.exe /c "build.cmd $Configuration $resolvedPlatform"
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}
finally
{
    Pop-Location
}
