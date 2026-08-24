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
Override the plugin description used in the release body. When omitted, the script
uses a built-in description table generated from source plugin metadata.

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
Open the generated URL in the default browser. The script opens the URL by default;
this switch is kept for explicit/compatible usage.

.PARAMETER NoOpen
Only print/copy the generated URL and do not open it in the default browser.

.PARAMETER CopyToClipboard
Copy the generated URL to the clipboard.

.EXAMPLE
.\tools\new_plugin_release_url.ps1 -ArchivePath .\plugin-packages-x64\plugin_5.0_ftp_5.01_x64.7z

.EXAMPLE
.\tools\new_plugin_release_url.ps1 .\plugin-packages-x64\plugin_5.0_ftp_5.01_x64.7z -Open -CopyToClipboard
#>
[CmdletBinding(DefaultParameterSetName = 'WithTag')]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$ArchivePath,

    [Parameter(Position = 1)]
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

    [switch]$NoOpen,

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
                $crc = ([uint32]3988292384 -bxor ($crc -shr 1))
            }
            else {
                $crc = $crc -shr 1
            }
        }
        $table[$i] = $crc
    }

    [uint32]$result = [uint32]::MaxValue
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

    return ('{0:X8}' -f ($result -bxor [uint32]::MaxValue))
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


function Get-KnownPluginDescriptions {
    return @{
        '7zip' = '7-Zip plugin for Open Salamander'
        'automation' = 'Automation plugin for Open Salamander'
        'checksum' = 'Checksum plugin for Open Salamander'
        'checkver' = 'Check Version plugin for Open Salamander'
        'csdemo' = 'Managed demo plugin for Open Salamander'
        'dbviewer' = 'Database Viewer for Open Salamander'
        'demomenu' = 'Sample plugin for Open Salamander'
        'demoplug' = 'Sample plugin for Open Salamander'
        'demoview' = 'Sample plugin for Open Salamander'
        'diskmap' = 'DiskMap Plugin for Open Salamander'
        'filecomp' = 'File Comparator for Open Salamander'
        'folders' = 'Folders plugin for Open Salamander'
        'ftp' = 'FTP Client for Open Salamander'
        'hypervm' = 'Show local Hyper-V virtual machines in the panel'
        'ieviewer' = 'Internet Explorer Viewer for Open Salamander'
        'jsonviewer' = 'JSON Viewer .NET plugin for Open Salamander'
        'mmviewer' = 'Multimedia Viewer for Open Salamander'
        'nethood' = 'Network plugin for Open Salamander'
        'pak' = 'Quake PAK archiver for Open Salamander'
        'peviewer' = 'Portable Executable Viewer for Open Salamander'
        'pictview' = 'Picture Viewer for Open Salamander'
        'portables' = 'Windows Portable Devices for Open Salamander'
        'regedt' = 'Registry Editor for Open Salamander'
        'renamer' = 'Renamer plugin for Open Salamander'
        'salamatrix' = 'Automation Framework provider for Salamatrix UI, Commands, FileOperations and Automation services.'
        'samandarin' = 'Samandarin update notification plugin for Open Salamander'
        'serviceexplorer' = 'Browse and configure services.'
        'splitcbn' = 'Split & Combine plugin for Open Salamander'
        'tar' = 'TAR plugin for Open Salamander'
        'textviewer' = 'Prism Text Viewer plugin for Open Salamander'
        'unarj' = 'UnARJ plugin for Open Salamander'
        'uncab' = 'UnCAB plugin for Open Salamander'
        'unchm' = 'UnCHM plugin for Open Salamander'
        'undelete' = 'Undelete plugin for Open Salamander'
        'unfat' = 'UnFAT plugin for Open Salamander'
        'uniso' = 'UnISO plugin for Open Salamander'
        'unlha' = 'UnLHA plugin for Open Salamander'
        'unmime' = 'UnMIME plugin for Open Salamander'
        'unole' = 'UnOLE2 plugin for Open Salamander'
        'unrar' = 'UnRAR plugin for Open Salamander'
        'webview2renderviewer' = 'WebView2 Render Viewer .NET plugin for Open Salamander'
        'wmobile' = 'Windows Mobile plugin for Open Salamander'
        'zip' = 'ZIP plugin for Open Salamander'
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
if (-not $PluginDescription) {
    $knownDescriptions = Get-KnownPluginDescriptions
    if ($knownDescriptions.ContainsKey($PluginName)) {
        $PluginDescription = $knownDescriptions[$PluginName]
    }
    else {
        throw "No built-in description is available for plugin '$PluginName'. Pass -PluginDescription to provide one."
    }
}

$title = 'plugin {0} {1} (x64) for Open Salamander 5.0' -f $PluginName, $PluginVersion
$body = @"
## $PluginName $PluginVersion (x64)
**$PluginDescription**

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
if ($Open -or -not $NoOpen) {
    try {
        Start-Process $url
    }
    catch {
        Write-Warning "Could not open release URL automatically: $_"
    }
}
