# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Embed plug-in translations in their .spl files. | Keep independently shipped plug-ins self-contained while retaining the host language pack. | | openrouter/openai/gpt-6-luna-pro |
<#
.SYNOPSIS
    Pack main-application translations into langpack.dll and plug-in translations into their .spl files.

.DESCRIPTION
    Takes a populated build tree (after build_language_packs.ps1) and creates
    lang\langpack.dll containing the main application's non-English .slg files
    as RT_RCDATA named LP_<LANGUAGE>_SALAMAND. The seed PE is copied from
    lang\english.slg so the pack matches the release architecture and VERSIONINFO.
    Each plug-in's non-English resources are embedded in plugins\<module>\<module>.spl
    with the same LP_<LANGUAGE>_<MODULE> names.

    Optionally removes non-English .slg files from the runtime tree so the
    ship payload contains host binaries, langpack.dll and self-contained plug-in
    binaries (+ legacy .slg for third-party plug-ins if present).

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

if ($null -eq ("SalLangPack.Native" -as [type])) {
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
namespace SalLangPack {
public static class Native {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern IntPtr BeginUpdateResource(string pFileName, bool bDeleteExistingResources);
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern bool UpdateResource(IntPtr hUpdate, IntPtr lpType, string lpName, ushort wLanguage, byte[] lpData, uint cbData);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool EndUpdateResource(IntPtr hUpdate, bool fDiscard);
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern IntPtr LoadLibraryEx(string fileName, IntPtr hFile, uint flags);
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool FreeLibrary(IntPtr hModule);
  [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet=CharSet.Unicode)]
  public delegate bool EnumResNameProc(IntPtr hModule, IntPtr type, IntPtr name, IntPtr lParam);
  [DllImport("kernel32.dll", EntryPoint="EnumResourceNamesW", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern bool EnumResourceNames(IntPtr hModule, IntPtr type, EnumResNameProc callback, IntPtr lParam);
  public static readonly IntPtr RT_RCDATA = (IntPtr)10;

  public static string[] GetRcDataNames(string fileName, string prefix) {
    IntPtr hModule = LoadLibraryEx(fileName, IntPtr.Zero, 0x22); // DATAFILE | IMAGE_RESOURCE
    if (hModule == IntPtr.Zero)
      throw new InvalidOperationException("LoadLibraryEx failed (Win32=" + Marshal.GetLastWin32Error() + ")");
    try {
      var names = new System.Collections.Generic.List<string>();
      EnumResNameProc callback = delegate(IntPtr module, IntPtr type, IntPtr name, IntPtr lParam) {
        if (name != IntPtr.Zero && name.ToInt64() > 0xFFFF) {
          string resourceName = Marshal.PtrToStringUni(name);
          if (resourceName != null && resourceName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            names.Add(resourceName);
        }
        return true;
      };
      if (!EnumResourceNames(hModule, RT_RCDATA, callback, IntPtr.Zero)) {
        int error = Marshal.GetLastWin32Error();
        if (error != 1813) // ERROR_RESOURCE_TYPE_NOT_FOUND
          throw new InvalidOperationException("EnumResourceNames failed (Win32=" + error + ")");
      }
      GC.KeepAlive(callback);
      return names.ToArray();
    }
    finally {
      FreeLibrary(hModule);
    }
  }
}
}
"@
}

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
        Target = $outPack
    }
}

# Plugin languages
$pluginsRoot = Join-Path $buildRootFull "plugins"
$pluginTargets = @{}
if (Test-Path -LiteralPath $pluginsRoot) {
    $pluginLangDirs = Get-ChildItem -LiteralPath $pluginsRoot -Directory -Recurse |
        Where-Object { $_.Name -ieq "lang" }
    foreach ($pluginLang in $pluginLangDirs) {
        $pluginDir = $pluginLang.Parent
        $pluginSlgs = @(Get-ChildItem -LiteralPath $pluginLang.FullName -Filter "*.slg" -File)
        $pluginEntries = @()
        $module = $pluginDir.Name.ToLowerInvariant()
        foreach ($slg in $pluginSlgs) {
            $base = [IO.Path]::GetFileNameWithoutExtension($slg.Name)
            if ($base -ieq "english") { continue }
            $pluginEntries += [pscustomobject]@{
                Module = $module
                Language = $base.ToLowerInvariant()
                Path = $slg.FullName
            }
        }

        $pluginBinary = Join-Path $pluginDir.FullName "$module.spl"
        if ($pluginEntries.Count -gt 0 -and -not (Test-Path -LiteralPath $pluginBinary -PathType Leaf)) {
            throw "Plug-in translations exist under $pluginLang, but the matching .spl was not found: $pluginBinary"
        }
        if (-not (Test-Path -LiteralPath $pluginBinary -PathType Leaf)) { continue }

        # Retain targets even when no translated .slg remains, so stale language
        # resources from an earlier packaging run can be removed.
        $pluginTargets[$pluginBinary] = $module
        foreach ($entry in $pluginEntries) {
            $entry | Add-Member -NotePropertyName Target -NotePropertyValue $pluginBinary
            $entries += $entry
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
    foreach ($entry in ($entries | Where-Object { $_.Target -eq $outPack })) {
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
        Write-Host "Packed $($entry.Module)/$($entry.Language) ($($bytes.Length) bytes) into langpack.dll as $resName"
    }
}
catch {
    [void][SalLangPack.Native]::EndUpdateResource($hUpdate, $true)
    throw
}

if (-not [SalLangPack.Native]::EndUpdateResource($hUpdate, $false)) {
    throw "EndUpdateResource failed for $outPack (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
}

# Update each plug-in independently. Remove stale resources from prior runs so
# the .spl exactly reflects this build's translated language set.
$pluginFilesUpdated = 0
foreach ($target in $pluginTargets.Keys) {
    $module = [string]$pluginTargets[$target]
    $pluginEntries = @($entries | Where-Object { $_.Target -eq $target })
    $resourcePattern = '^LP_[A-Z0-9]+_' + [regex]::Escape($module.ToUpperInvariant()) + '$'
    $existingResources = @([SalLangPack.Native]::GetRcDataNames($target, "LP_") |
        Where-Object { $_ -match $resourcePattern }
    )
    if ($existingResources.Count -eq 0 -and $pluginEntries.Count -eq 0) { continue }

    $hUpdate = [SalLangPack.Native]::BeginUpdateResource($target, $false)
    if ($hUpdate -eq [IntPtr]::Zero) {
        throw "BeginUpdateResource failed for $target (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
    }
    try {
        foreach ($resName in $existingResources) {
            if (-not [SalLangPack.Native]::UpdateResource($hUpdate, [SalLangPack.Native]::RT_RCDATA,
                    $resName, [uint16]0, $null, [uint32]0)) {
                throw "Could not remove stale language resource $resName from $target (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
            }
        }
        foreach ($entry in $pluginEntries) {
            $resName = Get-LangPackResourceName -LanguageBase $entry.Language -ModuleKey $entry.Module
            $bytes = [IO.File]::ReadAllBytes($entry.Path)
            if (-not [SalLangPack.Native]::UpdateResource($hUpdate, [SalLangPack.Native]::RT_RCDATA,
                    $resName, [uint16]0, $bytes, [uint32]$bytes.Length)) {
                throw "UpdateResource failed for $resName from $($entry.Path) (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
            }
            Write-Host "Packed $($entry.Module)/$($entry.Language) ($($bytes.Length) bytes) into $target as $resName"
        }
    }
    catch {
        [void][SalLangPack.Native]::EndUpdateResource($hUpdate, $true)
        throw
    }

    if (-not [SalLangPack.Native]::EndUpdateResource($hUpdate, $false)) {
        throw "EndUpdateResource failed for $target (Win32=$( [Runtime.InteropServices.Marshal]::GetLastWin32Error() ))"
    }
    $pluginFilesUpdated++
}

$mainLanguageCount = @($entries | Where-Object { $_.Target -eq $outPack }).Count
Write-Host "Wrote main-application language pack: $outPack ($mainLanguageCount language(s))"
Write-Host "Updated plug-in translations in $pluginFilesUpdated .spl file(s)."

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
