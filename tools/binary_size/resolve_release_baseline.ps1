# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryPath,

    [string]$TagPrefix = '5.0-samandarin-'
)

$ErrorActionPreference = 'Stop'

function Compare-VersionParts {
    param([int[]]$Left, [int[]]$Right)

    $count = [Math]::Max($Left.Count, $Right.Count)
    for ($index = 0; $index -lt $count; $index++) {
        $leftPart = if ($index -lt $Left.Count) { $Left[$index] } else { 0 }
        $rightPart = if ($index -lt $Right.Count) { $Right[$index] } else { 0 }
        if ($leftPart -lt $rightPart) { return -1 }
        if ($leftPart -gt $rightPart) { return 1 }
    }
    return 0
}

$repository = [IO.Path]::GetFullPath($RepositoryPath)
$insideWorkTree = (& git -C $repository rev-parse --is-inside-work-tree 2>$null).Trim()
if ($LASTEXITCODE -ne 0 -or $insideWorkTree -ne 'true') {
    throw "Not a Git checkout: $repository"
}

$escapedPrefix = [Regex]::Escape($TagPrefix)
$best = $null
$tags = @(& git -C $repository tag --list "$TagPrefix*")
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to enumerate release tags.'
}

foreach ($tag in $tags) {
    if ($tag -notmatch "^$escapedPrefix(?<version>[0-9]+(?:\.[0-9]+)*)$") {
        continue
    }

    $parts = @($Matches.version.Split('.') | ForEach-Object { [int]$_ })
    if ($null -eq $best -or
        (Compare-VersionParts -Left $parts -Right $best.Parts) -gt 0 -or
        ((Compare-VersionParts -Left $parts -Right $best.Parts) -eq 0 -and
         [string]::CompareOrdinal($tag, $best.Tag) -gt 0)) {
        $best = [pscustomobject]@{ Tag = $tag; Parts = $parts }
    }
}

if ($null -eq $best) {
    throw "No numeric release tag matching '$TagPrefix*' was found."
}

$commit = (& git -C $repository rev-list -n 1 $best.Tag).Trim()
if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-fA-F]{40}$') {
    throw "Unable to resolve tag '$($best.Tag)' to a commit."
}

[pscustomobject]@{
    tag = $best.Tag
    version = ($best.Parts -join '.')
    commit = $commit.ToLowerInvariant()
} | ConvertTo-Json -Compress
