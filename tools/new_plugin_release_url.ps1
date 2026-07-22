<#
.SYNOPSIS
Builds a prefilled GitHub release URL for a packaged Open Salamander plugin archive.

.DESCRIPTION
The script accepts one 7z archive created by tools/package_salamander_plugins.ps1,
parses the plugin name and version from its file name, computes CRC32/MD5/SHA1/SHA256
checksums, and prints a GitHub releases/new URL with prefilled title, body, and tag.

Archive names are expected to follow this shape:
  plugin_5.0_<plugin>_<version>_x64.7z

.PARAMETER ArchivePath
Path to the plugin 7z archive.

.PARAMETER PluginDescription
English plugin description used in the release body.

.PARAMETER Repository
GitHub repository in OWNER/REPOSITORY form. Defaults to KRtkovo-eu-AI/salamander-plugins.

.PARAMETER PluginName
Override the plugin name parsed from the archive file name.

.PARAMETER PluginVersion
Override the plugin version parsed from the archive file name.

.PARAMETER Tag
Override the Git tag prefilled in the release form. Defaults to the archive base name.

.PARAMETER NoTag
Do not add the tag query parameter.

.PARAMETER Prerelease
Prefill the release form as a prerelease.

.PARAMETER Open
Open the generated URL in the default browser.

.PARAMETER CopyToClipboard
Copy the generated URL to the clipboard.

.EXAMPLE
.\tools\new_plugin_release_url.ps1 -ArchivePath .\plugin-packages-x64\plugin_5.0_ftp_5.01_x64.7z -PluginDescription 'FTP client plugin for Open Salamander.'

.EXAMPLE
.\tools\new_plugin_release_url.ps1 .\plugin-packages-x64\plugin_5.0_ftp_5.01_x64.7z 'FTP client plugin for Open Salamander.' -Open -CopyToClipboard
#>
[CmdletBinding(DefaultParameterSetName = 'WithTag')]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$ArchivePath,

    [Parameter(Mandatory = $true, Position = 1)]
    [ValidateNotNullOrEmpty()]
    [string]$PluginDescription,

    [ValidatePattern('^[^/\s]+/[^/\s]+$')]
    [string]$Repository = 'KRtkovo-eu-AI/salamander-plugins',

    [ValidateNotNullOrEmpty()]
    [string]$PluginName,

    [ValidateNotNullOrEmpty()]
    [string]$PluginVersion,

    [Parameter(ParameterSetName = 'WithTag')]
    [ValidateNotNullOrEmpty()]
    [string]$Tag,

    [Parameter(ParameterSetName = 'WithoutTag')]
    [switch]$NoTag,

    [switch]$Prerelease,

    [switch]$Open,

    [switch]$CopyToClipboard
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExistingFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    $item = Get-Item -LiteralPath $resolved.Path
    if ($item.PSIsContainer) {
        throw "Archive path is a directory, expected a file: $Path"
    }

    return $item
}

function Get-Crc32 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $table = New-Object 'uint32[]' 256
    for ($i = 0; $i -lt 256; $i++) {
        [uint32]$crc = $i
        for ($j = 0; $j -lt 8; $j++) {
            if (($crc -band 1) -ne 0) {
                $crc = (0xEDB88320 -bxor ($crc -shr 1))
            }
            else {
                $crc = $crc -shr 1
            }
        }
        $table[$i] = $crc
    }

    [uint32]$result = 0xFFFFFFFF
    $buffer = New-Object byte[] 65536
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        while (($read = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
            for ($i = 0; $i -lt $read; $i++) {
                $index = ($result -bxor $buffer[$i]) -band 0xFF
                $result = $table[$index] -bxor ($result -shr 8)
            }
        }
    }
    finally {
        $stream.Dispose()
    }

    return ('{0:X8}' -f (-bnot $result -band 0xFFFFFFFF))
}

function Get-ReleaseArchiveMetadata {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.FileInfo]$Archive
    )

    $match = [regex]::Match($Archive.Name, '^plugin_5\.0_(?<name>.+)_(?<version>[^_]+)_x64\.7z$', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if (-not $match.Success) {
        throw "Archive name '$($Archive.Name)' does not match expected format 'plugin_5.0_<plugin>_<version>_x64.7z'. Pass -PluginName and -PluginVersion to override parsing."
    }

    return [pscustomobject]@{
        PluginName = $match.Groups['name'].Value
        PluginVersion = $match.Groups['version'].Value
    }
}

function ConvertTo-GitHubQueryValue {
    param([AllowEmptyString()][string]$Value)

    return [System.Uri]::EscapeDataString($Value).Replace('%20', '+')
}

$archive = Resolve-ExistingFile -Path $ArchivePath
$metadata = $null
if (-not $PluginName -or -not $PluginVersion) {
    $metadata = Get-ReleaseArchiveMetadata -Archive $archive
}

if (-not $PluginName) {
    $PluginName = $metadata.PluginName
}
if (-not $PluginVersion) {
    $PluginVersion = $metadata.PluginVersion
}
if (-not $Tag -and -not $NoTag) {
    $Tag = [System.IO.Path]::GetFileNameWithoutExtension($archive.Name)
}

$title = 'plugin {0} {1} (x64) for Open Salamander 5.0' -f $PluginName, $PluginVersion
$body = @"
# $PluginName $PluginVersion
$PluginDescription

CRC: $(Get-Crc32 -Path $archive.FullName)
MD5: $((Get-FileHash -LiteralPath $archive.FullName -Algorithm MD5).Hash)
SHA1: $((Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA1).Hash)
SHA256: $((Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA256).Hash)
"@

$query = [ordered]@{}
if (-not $NoTag) {
    $query['tag'] = $Tag
}
$query['title'] = $title
$query['body'] = $body.TrimEnd("`r", "`n")
if ($Prerelease) {
    $query['prerelease'] = 'true'
}

$queryString = ($query.GetEnumerator() | ForEach-Object { '{0}={1}' -f $_.Key, (ConvertTo-GitHubQueryValue -Value ([string]$_.Value)) }) -join '&'
$url = 'https://github.com/{0}/releases/new?{1}' -f $Repository, $queryString

Write-Output $url

if ($CopyToClipboard) {
    Set-Clipboard -Value $url
}
if ($Open) {
    Start-Process $url
}
