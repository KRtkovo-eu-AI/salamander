<#
.SYNOPSIS
    Pack non-English .slg artifacts into one official langpack.dll.

.DESCRIPTION
    Takes a populated build tree (after build_language_packs.ps1) and creates
    lang\langpack.dll containing every non-English .slg as RT_RCDATA named
    LP_<LANGUAGE>_<MODULE>. The seed PE is copied from lang\english.slg so the
    pack matches the release architecture and VERSIONINFO.

    Optionally removes non-English .slg files from the runtime tree so the
    ship payload only needs host binaries + langpack.dll (+ legacy .slg for
    third-party plug-ins if present).

.PARAMETER BuildRoot
    Path to the populated build output (e.g. build/out/salamand/Release_x64).

.PARAMETER KeepPackedSlg
    Keep non-English .slg files after packing (default: remove them).

.PARAMETER RemoveEnglishSlg
    Also remove english.slg intermediates from the runtime tree.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildRoot,

    [switch]$KeepPackedSlg,

    [switch]$RemoveEnglishSlg
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$buildRootFull = if (Test-Path -LiteralPath $BuildRoot) {
    (Resolve-Path -LiteralPath $BuildRoot).Path
} else {
    throw "BuildRoot not found: $BuildRoot"
}

$seedSlg = Join-Path $buildRootFull "lang\english.slg"
if (-not (Test-Path -LiteralPath $seedSlg)) {
    throw "Seed english.slg not found at: $seedSlg"
}

$langDir = Join-Path $buildRootFull "lang"
$outPack = Join-Path $langDir "langpack.dll"
Copy-Item -LiteralPath $seedSlg -Destination $outPack -Force

Add-Type -Namespace SalLangPack -Name Native -MemberDefinition @"
using System;
using System.Runtime.InteropServices;
public static class Native {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern IntPtr BeginUpdateResource(string pFileName, bool bDeleteExistingResources);
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern bool UpdateResource(IntPtr hUpdate, IntPtr lpType, string lpName, ushort wLanguage, byte[] lpData, uint cbData);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool EndUpdateResource(IntPtr hUpdate, bool fDiscard);
  public static readonly IntPtr RT_RCDATA = (IntPtr)10;
}
"@

function Get-LangPackResourceName([string]$LanguageBase, [string]$ModuleKey) {
    $lang = ($LanguageBase -replace '\.slg$', '').ToUpperInvariant()
    $mod = $ModuleKey.ToUpperInvariant()
    return "LP_${lang}_${mod}"
}

$entries = @()

# Main application languages
Get-ChildItem -LiteralPath $langDir -Filter "*.slg" -File | ForEach-Object {
    $base = [IO.Path]::GetFileNameWithoutExtension($_.Name)
    if ($base -ieq "english") { return }
    $entries += [pscustomobject]@{
        Module = "salamand"
        Language = $base.ToLowerInvariant()
        Path = $_.FullName
    }
}

# Plugin languages
$pluginsRoot = Join-Path $buildRootFull "plugins"
if (Test-Path -LiteralPath $pluginsRoot) {
    Get-ChildItem -LiteralPath $pluginsRoot -Directory | ForEach-Object {
        $pluginLang = Join-Path $_.FullName "lang"
        if (-not (Test-Path -LiteralPath $pluginLang)) { return }
        $module = $_.Name.ToLowerInvariant()
        Get-ChildItem -LiteralPath $pluginLang -Filter "*.slg" -File | ForEach-Object {
            $base = [IO.Path]::GetFileNameWithoutExtension($_.Name)
            if ($base -ieq "english") { return }
            $entries += [pscustomobject]@{
                Module = $module
                Language = $base.ToLowerInvariant()
                Path = $_.FullName
            }
        }
    }
}

if ($entries.Count -eq 0) {
    Write-Warning "No non-English .slg files found under $buildRootFull; langpack.dll will only contain the English seed PE."
}

$hUpdate = [SalLangPack.Native]::BeginUpdateResource($outPack, $false)
if ($hUpdate -eq [IntPtr]::Zero) {
    throw "BeginUpdateResource failed for $outPack (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
}

try {
    foreach ($entry in $entries) {
        $resName = Get-LangPackResourceName -LanguageBase $entry.Language -ModuleKey $entry.Module
        $bytes = [IO.File]::ReadAllBytes($entry.Path)
        $ok = [SalLangPack.Native]::UpdateResource(
            $hUpdate,
            [SalLangPack.Native]::RT_RCDATA,
            $resName,
            [uint16]0,
            $bytes,
            [uint32]$bytes.Length)
        if (-not $ok) {
            throw "UpdateResource failed for $resName from $($entry.Path) (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
        }
        Write-Host "Packed $($entry.Module)/$($entry.Language) ($($bytes.Length) bytes) as $resName"
    }
}
catch {
    [void][SalLangPack.Native]::EndUpdateResource($hUpdate, $true)
    throw
}

if (-not [SalLangPack.Native]::EndUpdateResource($hUpdate, $false)) {
    throw "EndUpdateResource failed for $outPack (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
}

Write-Host "Wrote official language pack: $outPack ($($entries.Count) language modules)"

if (-not $KeepPackedSlg) {
    foreach ($entry in $entries) {
        Remove-Item -LiteralPath $entry.Path -Force -ErrorAction SilentlyContinue
    }
    Write-Host "Removed $($entries.Count) packed non-English .slg file(s) from the runtime tree."
}

if ($RemoveEnglishSlg) {
    Get-ChildItem -LiteralPath $langDir -Filter "english.slg" -File -ErrorAction SilentlyContinue |
        Remove-Item -Force
    if (Test-Path -LiteralPath $pluginsRoot) {
        Get-ChildItem -LiteralPath $pluginsRoot -Recurse -Filter "english.slg" -File -ErrorAction SilentlyContinue |
            Remove-Item -Force
    }
    Write-Host "Removed english.slg intermediates from the runtime tree."
}
