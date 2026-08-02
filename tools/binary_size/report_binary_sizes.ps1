# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CurrentRoot,

    [string]$BaselineRoot,
    [string]$BaselineTag = '',
    [string]$BaselineCommit = '',
    [string]$CurrentLabel = 'Current',
    [string]$OutputJson,
    [string]$OutputMarkdown
)

$ErrorActionPreference = 'Stop'

function Get-MachineName {
    param([int]$Machine)
    switch ($Machine) {
        0x014c { return 'x86' }
        0x8664 { return 'x64' }
        0xaa64 { return 'ARM64' }
        0x01c4 { return 'ARM' }
        default { return ('0x{0:X4}' -f $Machine) }
    }
}

function Read-PeArtifact {
    param([IO.FileInfo]$File, [string]$Root)

    $stream = $null
    $reader = $null
    try {
        $stream = [IO.File]::Open($File.FullName, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        if ($stream.Length -lt 64) { return $null }
        $reader = [IO.BinaryReader]::new($stream)
        if ($reader.ReadUInt16() -ne 0x5a4d) { return $null }
        $stream.Position = 0x3c
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or $peOffset + 24 -gt $stream.Length) { return $null }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { return $null }

        $machine = $reader.ReadUInt16()
        $sectionCount = $reader.ReadUInt16()
        $stream.Position += 12
        $optionalHeaderSize = $reader.ReadUInt16()
        $stream.Position += 2
        $optionalHeaderOffset = $stream.Position
        if ($optionalHeaderSize -lt 64 -or $optionalHeaderOffset + $optionalHeaderSize -gt $stream.Length) { return $null }
        $magic = $reader.ReadUInt16()
        if ($magic -ne 0x10b -and $magic -ne 0x20b) { return $null }
        $stream.Position = $optionalHeaderOffset + 56
        $sizeOfImage = [long]$reader.ReadUInt32()
        $sectionTableOffset = $optionalHeaderOffset + $optionalHeaderSize
        if ($sectionTableOffset + (40L * $sectionCount) -gt $stream.Length) { return $null }

        $sections = @()
        for ($index = 0; $index -lt $sectionCount; $index++) {
            $stream.Position = $sectionTableOffset + (40L * $index)
            $name = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(8)).Trim([char]0)
            $virtualSize = [long]$reader.ReadUInt32()
            [void]$reader.ReadUInt32()
            $rawSize = [long]$reader.ReadUInt32()
            $sections += [pscustomobject]@{
                name = $name
                rawSizeBytes = $rawSize
                virtualSizeBytes = $virtualSize
            }
        }

        $rootPrefix = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
        $relativePath = $File.FullName.Substring($rootPrefix.Length).Replace('\', '/')
        return [pscustomobject]@{
            path = $relativePath
            fileSizeBytes = [long]$File.Length
            sizeOfImageBytes = $sizeOfImage
            machine = Get-MachineName -Machine $machine
            sha256 = (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            sections = $sections
        }
    }
    catch {
        return $null
    }
    finally {
        if ($null -ne $reader) { $reader.Dispose() }
        elseif ($null -ne $stream) { $stream.Dispose() }
    }
}

function New-Snapshot {
    param([string]$Root, [string]$Label)

    $fullRoot = [IO.Path]::GetFullPath($Root)
    if (-not (Test-Path -LiteralPath $fullRoot -PathType Container)) {
        throw "Binary output directory does not exist: $fullRoot"
    }

    $artifacts = @(
        Get-ChildItem -LiteralPath $fullRoot -File -Recurse |
            ForEach-Object { Read-PeArtifact -File $_ -Root $fullRoot } |
            Where-Object { $null -ne $_ } |
            Sort-Object path
    )

    $sectionTotals = [ordered]@{}
    foreach ($artifact in $artifacts) {
        foreach ($section in $artifact.sections) {
            if (-not $sectionTotals.Contains($section.name)) { $sectionTotals[$section.name] = [long]0 }
            $sectionTotals[$section.name] += $section.rawSizeBytes
        }
    }

    return [pscustomobject]@{
        label = $Label
        artifactCount = $artifacts.Count
        fileSizeBytes = [long](($artifacts | Measure-Object fileSizeBytes -Sum).Sum)
        sizeOfImageBytes = [long](($artifacts | Measure-Object sizeOfImageBytes -Sum).Sum)
        sectionRawSizeBytes = [pscustomobject]$sectionTotals
        artifacts = $artifacts
    }
}

function Format-Bytes {
    param([long]$Value, [switch]$Signed)
    $sign = if ($Signed -and $Value -gt 0) { '+' } else { '' }
    $absolute = [Math]::Abs([double]$Value)
    if ($absolute -ge 1MB) { return ('{0}{1:F2} MiB' -f $sign, ($Value / 1MB)) }
    if ($absolute -ge 1KB) { return ('{0}{1:F2} KiB' -f $sign, ($Value / 1KB)) }
    return ('{0}{1} B' -f $sign, $Value)
}

function Get-SectionMap {
    param($Artifact)
    $map = @{}
    if ($null -ne $Artifact) {
        foreach ($section in $Artifact.sections) { $map[$section.name] = $section }
    }
    return $map
}

function New-Comparison {
    param($Baseline, $Current)

    $baselineMap = @{}
    foreach ($artifact in $Baseline.artifacts) { $baselineMap[$artifact.path] = $artifact }
    $currentMap = @{}
    foreach ($artifact in $Current.artifacts) { $currentMap[$artifact.path] = $artifact }
    $paths = @($baselineMap.Keys + $currentMap.Keys | Sort-Object -Unique)
    $rows = foreach ($path in $paths) {
        $before = $baselineMap[$path]
        $after = $currentMap[$path]
        $beforeSize = if ($null -ne $before) { [long]$before.fileSizeBytes } else { [long]0 }
        $afterSize = if ($null -ne $after) { [long]$after.fileSizeBytes } else { [long]0 }
        $status = if ($null -eq $before) { 'added' } elseif ($null -eq $after) { 'removed' } elseif ($before.sha256 -eq $after.sha256) { 'unchanged' } else { 'changed' }

        $beforeSections = Get-SectionMap $before
        $afterSections = Get-SectionMap $after
        $sectionNames = @($beforeSections.Keys + $afterSections.Keys | Sort-Object -Unique)
        $sectionDeltas = foreach ($name in $sectionNames) {
            $beforeRaw = if ($beforeSections.ContainsKey($name)) { [long]$beforeSections[$name].rawSizeBytes } else { [long]0 }
            $afterRaw = if ($afterSections.ContainsKey($name)) { [long]$afterSections[$name].rawSizeBytes } else { [long]0 }
            [pscustomobject]@{ name = $name; baselineRawSizeBytes = $beforeRaw; currentRawSizeBytes = $afterRaw; deltaRawSizeBytes = $afterRaw - $beforeRaw }
        }

        [pscustomobject]@{
            path = $path
            status = $status
            machine = if ($null -ne $after) { $after.machine } else { $before.machine }
            baselineSizeBytes = $beforeSize
            currentSizeBytes = $afterSize
            deltaBytes = $afterSize - $beforeSize
            sectionDeltas = @($sectionDeltas)
        }
    }

    return @($rows | Sort-Object @{ Expression = { [Math]::Abs($_.deltaBytes) }; Descending = $true }, path)
}

function New-Markdown {
    param($Report)

    $baselineLabel = if ($Report.baselineTag) { $Report.baselineTag } else { $Report.baseline.label }
    $delta = [long]$Report.summary.deltaBytes
    $lines = [Collections.Generic.List[string]]::new()
    $lines.Add('# Binary size report')
    $lines.Add('')
    $lines.Add("Baseline: **$baselineLabel** (``$($Report.baselineCommit)``)  ")
    $lines.Add("Current: **$($Report.current.label)**  ")
    $lines.Add("Total: **$(Format-Bytes $Report.baseline.fileSizeBytes)** -> **$(Format-Bytes $Report.current.fileSizeBytes)** (**$(Format-Bytes $delta -Signed)**)")
    $lines.Add('')
    $interesting = @($Report.comparison | Where-Object { $_.deltaBytes -ne 0 })
    $unchangedCount = @($Report.comparison | Where-Object { $_.status -eq 'unchanged' }).Count
    if ($interesting.Count -eq 0) {
        $lines.Add('')
        $lines.Add("No binary size differences were found. Unchanged artifacts: **$unchangedCount**.")
    }
    else {
        $lines.Add('')
        $lines.Add('| Artifact | Architecture | Status | Baseline | Current | Difference |')
        $lines.Add('| --- | --- | --- | ---: | ---: | ---: |')
        foreach ($row in $interesting) {
            $lines.Add("| ``$($row.path)`` | $($row.machine) | $($row.status) | $(Format-Bytes $row.baselineSizeBytes) | $(Format-Bytes $row.currentSizeBytes) | $(Format-Bytes $row.deltaBytes -Signed) |")
        }
        $lines.Add('')
        $lines.Add("Unchanged artifacts omitted from the table: **$unchangedCount**.")
        $lines.Add('')
        $lines.Add('## PE section changes')
        foreach ($row in $interesting) {
            $sectionChanges = @($row.sectionDeltas | Where-Object { $_.deltaRawSizeBytes -ne 0 })
            if ($sectionChanges.Count -eq 0) { continue }
            $details = $sectionChanges | ForEach-Object { "``$($_.name)`` $(Format-Bytes $_.deltaRawSizeBytes -Signed)" }
            $lines.Add("- ``$($row.path)``: $($details -join ', ')")
        }
    }
    return $lines -join [Environment]::NewLine
}

$current = New-Snapshot -Root $CurrentRoot -Label $CurrentLabel
if ([string]::IsNullOrWhiteSpace($BaselineRoot)) {
    $report = [pscustomobject]@{ schemaVersion = 1; current = $current }
    $markdown = "# Binary size report`n`n**$($current.artifactCount)** PE artifacts, **$(Format-Bytes $current.fileSizeBytes)** total."
}
else {
    $baseline = New-Snapshot -Root $BaselineRoot -Label $BaselineTag
    $comparison = New-Comparison -Baseline $baseline -Current $current
    $delta = [long]$current.fileSizeBytes - [long]$baseline.fileSizeBytes
    $changedCount = @($comparison | Where-Object { $_.status -ne 'unchanged' }).Count
    $report = [pscustomobject]@{
        schemaVersion = 1
        baselineTag = $BaselineTag
        baselineCommit = $BaselineCommit
        baseline = $baseline
        current = $current
        summary = [pscustomobject]@{
            deltaBytes = $delta
            changedArtifactCount = $changedCount
            title = "$(Format-Bytes $delta -Signed) across $changedCount changed artifacts"
        }
        comparison = $comparison
    }
    $markdown = New-Markdown -Report $report
}

$json = $report | ConvertTo-Json -Depth 12
if ($OutputJson) {
    $directory = Split-Path -Parent ([IO.Path]::GetFullPath($OutputJson))
    if ($directory) { [IO.Directory]::CreateDirectory($directory) | Out-Null }
    [IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputJson), $json, [Text.UTF8Encoding]::new($false))
}
if ($OutputMarkdown) {
    $directory = Split-Path -Parent ([IO.Path]::GetFullPath($OutputMarkdown))
    if ($directory) { [IO.Directory]::CreateDirectory($directory) | Out-Null }
    [IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputMarkdown), $markdown, [Text.UTF8Encoding]::new($false))
}

Write-Output $markdown
