# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-25 | Stage runtime files without a stale installer inventory. | Host builds skip plugin inventory and refuse a leftover manifest. | https://github.com/KRtkovo-eu-AI/salamander/pull/843 | Grok 4.7 |
# | 2026-11-20 | Separate runtime staging from installer inventory generation. | Allow the host to build independently of plugin outputs. |  | gpt-6.2-sol |
# | 2026-11-16 | Support explicit runtime-only staging. | Keep host-only builds independent of unfinished plugin binaries. | | GPT-6 |
# | 2026-11-14 | Keep runtime payload staging independent of installer catalog generation. | A catalog refresh is an installer-only step and must not fail the C++ post-build copy. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-14 | Refresh the Inno web-package include from the stable catalogs during staging. | Keep both installer variants synchronized with release URLs, hashes, and versions. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-14 | Emit ownership inventory for installer-managed plugin files. | Upgrades need safe, component-scoped stale-file cleanup. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-14 | Stage extension trees declared by the Inno payload. | Release x64 builds must contain JSON Viewer and other installer extensions. | | openrouter/openai/gpt-6-luna-pro |

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryRoot,

    [Parameter(Mandatory = $true)]
    [string]$DestinationRoot,

    [switch]$RuntimeOnly
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$destinationRoot = [System.IO.Path]::GetFullPath($DestinationRoot)
$innoScript = Join-Path $repositoryRoot 'doc\runbook-setup\inno_setup_salamander_x64.iss'

if (-not (Test-Path -LiteralPath $innoScript -PathType Leaf)) {
    throw "Inno Setup script was not found: $innoScript"
}

function Resolve-PayloadSource {
    param([string]$PayloadPath)

    $sourceRelativePath = switch -Regex ($PayloadPath) {
        '^toolbars\\' { Join-Path 'src\res' $PayloadPath; break }
        '^doc\\license\.txt$' { 'LICENSE'; break }
        '^doc\\license_gpl\.txt$' { Join-Path 'doc\license' 'license_gpl.txt'; break }
        default { $PayloadPath }
    }

    Join-Path $repositoryRoot $sourceRelativePath
}

function Get-FileSha256 {
    param([string]$Path)
    if (Get-Command -Name Get-FileHash -ErrorAction SilentlyContinue) {
        return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    }
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $bytes = $hasher.ComputeHash($stream)
        return ([System.BitConverter]::ToString($bytes) -replace '-', '')
    }
    finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}

function Copy-IfDifferent {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    $destinationDirectory = Split-Path -Parent $DestinationPath
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null

    $needsCopy = -not (Test-Path -LiteralPath $DestinationPath -PathType Leaf)
    if (-not $needsCopy) {
        $sourceHash = Get-FileSha256 -Path $SourcePath
        $destinationHash = Get-FileSha256 -Path $DestinationPath
        $needsCopy = $sourceHash -ne $destinationHash
    }

    if ($needsCopy) {
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
        return $true
    }

    return $false
}

$innoLines = Get-Content -LiteralPath $innoScript
$payloadPaths = $innoLines | ForEach-Object {
    if ($_ -match '^\s*Source:\s+"\{#PayloadDir\}\\((?:toolbars|doc|convert)\\[^";]+)"') {
        $Matches[1]
    }
} | Sort-Object -Unique

$extensionRoots = $innoLines | ForEach-Object {
    if ($_ -match '^\s*Source:\s+"\{#PayloadDir\}\\extensions\\([^";]+)"') {
        $sourcePattern = $Matches[1].Replace('/', '\')
        if ($sourcePattern -match '^(?<root>[^*?]+)\\\*$') {
            $Matches['root'].TrimEnd('\')
        }
    }
} | Sort-Object -Unique

if ($payloadPaths.Count -eq 0 -and $extensionRoots.Count -eq 0) {
    throw "No supported Inno payload sources were found in $innoScript"
}

$copiedCount = 0
foreach ($payloadPath in $payloadPaths) {
    $sourcePath = Resolve-PayloadSource $payloadPath
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Payload source for '$payloadPath' was not found: $sourcePath"
    }

    $destinationPath = Join-Path $destinationRoot $payloadPath
    if (Copy-IfDifferent -SourcePath $sourcePath -DestinationPath $destinationPath) {
        $copiedCount++
    }
}

$extensionFileCount = 0
foreach ($extensionRoot in $extensionRoots) {
    $normalizedRoot = $extensionRoot.Replace('/', '\')
    if ($normalizedRoot.StartsWith('\') -or $normalizedRoot.Contains(':') -or
        @($normalizedRoot.Split('\') | Where-Object { $_ -eq '' -or $_ -eq '.' -or $_ -eq '..' }).Count -gt 0) {
        throw "Unsafe extension payload path in $innoScript`: $extensionRoot"
    }

    $sourceRoot = Join-Path (Join-Path $repositoryRoot 'src\extensions') $normalizedRoot
    if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
        throw "Extension payload source '$normalizedRoot' was not found: $sourceRoot"
    }
    $sourceRootFull = [System.IO.Path]::GetFullPath($sourceRoot).TrimEnd('\') + '\'
    $files = @(Get-ChildItem -LiteralPath $sourceRootFull -Recurse -File -Force |
        Where-Object { ($_.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0 })
    if (-not @($files | Where-Object { $_.Name -eq 'extension.json' }).Count) {
        throw "Extension payload '$normalizedRoot' does not contain an extension.json manifest."
    }
    foreach ($file in $files) {
        $relativePath = $file.FullName.Substring($sourceRootFull.Length)
        $destinationPath = Join-Path (Join-Path (Join-Path $destinationRoot 'extensions') $normalizedRoot) $relativePath
        if (Copy-IfDifferent -SourcePath $file.FullName -DestinationPath $destinationPath) {
            $copiedCount++
        }
        $extensionFileCount++
    }
}

if ($RuntimeOnly) {
    # A pre-existing manifest could be mistaken for an inventory of this runtime-only build.
    $managedManifestPath = Join-Path $destinationRoot 'managed-plugin-files.txt'
    if (Test-Path -LiteralPath $managedManifestPath) {
        throw "Runtime-only staging cannot use a directory containing an existing managed-plugin manifest: $managedManifestPath"
    }
    Write-Host "Staged $($payloadPaths.Count) Inno payload files and $extensionFileCount files from $($extensionRoots.Count) extension trees ($copiedCount updated); installer manifest was not generated."
    return
}

$managedFileEntries = [System.Collections.Generic.List[string]]::new()
foreach ($line in $innoLines) {
    if ($line -notmatch '^\s*Source:\s+"\{#PayloadDir\}\\([^";]+)"\s*;\s*DestDir:\s+"\{app\}([^";]*)"') {
        continue
    }
    $sourcePattern = $Matches[1].Replace('/', '\')
    $destinationPrefix = $Matches[2].Replace('/', '\').TrimStart('\').TrimEnd('\')
    if ($line -notmatch "Check:\s*IsPluginSelected\('(?<plugin>[A-Za-z0-9_-]+)'\)") {
        continue
    }
    $pluginId = $Matches['plugin'].ToLowerInvariant()

    $wildcardIndex = $sourcePattern.IndexOfAny([char[]]@('*', '?'))
    if ($wildcardIndex -ge 0) {
        $separatorIndex = $sourcePattern.LastIndexOf('\', $wildcardIndex)
        if ($separatorIndex -lt 0) {
            throw "Unsupported managed plugin source pattern in $innoScript`: $sourcePattern"
        }
        $sourceRootRelative = $sourcePattern.Substring(0, $separatorIndex)
        $filePattern = $sourcePattern.Substring($separatorIndex + 1)
        $sourceRoot = Join-Path $destinationRoot $sourceRootRelative
        if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
            throw "Managed plugin source directory was not found: $sourceRoot"
        }
        if ($line -match 'Flags:\s*[^;]*\brecursesubdirs\b') {
            $sourceFiles = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File -Filter $filePattern -Force |
                Where-Object { ($_.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0 })
        }
        else {
            $sourceFiles = @(Get-ChildItem -LiteralPath $sourceRoot -File -Filter $filePattern -Force |
                Where-Object { ($_.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0 })
        }

        $sourceRootFull = [System.IO.Path]::GetFullPath($sourceRoot).TrimEnd('\') + '\'
        foreach ($sourceFile in $sourceFiles) {
            $relativePath = $sourceFile.FullName.Substring($sourceRootFull.Length)
            $destinationPath = if ($destinationPrefix -eq '') { $relativePath } else { Join-Path $destinationPrefix $relativePath }
            $managedFileEntries.Add("$pluginId|$($destinationPath.Replace('/', '\'))")
        }
    }
    else {
        $sourceFile = Join-Path $destinationRoot $sourcePattern
        if (-not (Test-Path -LiteralPath $sourceFile -PathType Leaf)) {
            throw "Managed plugin source file was not found: $sourceFile"
        }
        $destinationPath = if ($destinationPrefix -eq '') { Split-Path -Leaf $sourcePattern } else { Join-Path $destinationPrefix (Split-Path -Leaf $sourcePattern) }
        $managedFileEntries.Add("$pluginId|$($destinationPath.Replace('/', '\'))")
    }
}

$managedManifestPath = Join-Path $destinationRoot 'managed-plugin-files.txt'
$managedManifest = @($managedFileEntries | Sort-Object -Unique)
[System.IO.File]::WriteAllLines($managedManifestPath, $managedManifest, [System.Text.UTF8Encoding]::new($false))

Write-Host "Recorded $($managedManifest.Count) component-owned installer files in $managedManifestPath."
