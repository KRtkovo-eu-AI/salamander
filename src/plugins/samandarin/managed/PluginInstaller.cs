// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Serialize package copies that share a version file. | Scheduling a second extension before restart could drop the other extensions.ver record. | | Grok 4.7 |
// | 2026-11-04 | Overwrite extension files and record them for discovery. | Install wrote a receipt without registering the package, and updates left the previous files in place. | | Grok 4.7 |

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using SharpCompress.Archives;

namespace OpenSalamander.Samandarin;

internal static class PluginDependencyResolver
{
    private const string UnsafeAssemblyName = "System.Runtime.CompilerServices.Unsafe";
    private static readonly object SyncRoot = new();
    private static bool _initialized;

    public static void Initialize()
    {
        lock (SyncRoot)
        {
            if (_initialized) return;
            AppDomain.CurrentDomain.AssemblyResolve += ResolveLocalDependency;
            _initialized = true;
        }
    }

    private static Assembly? ResolveLocalDependency(object? sender, ResolveEventArgs args)
    {
        var requestedName = new AssemblyName(args.Name).Name;
        if (!string.Equals(requestedName, UnsafeAssemblyName, StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        var assemblyDirectory = Path.GetDirectoryName(typeof(PluginDependencyResolver).Assembly.Location);
        if (string.IsNullOrWhiteSpace(assemblyDirectory))
        {
            return null;
        }

        var path = Path.Combine(assemblyDirectory, UnsafeAssemblyName + ".dll");
        return File.Exists(path) ? Assembly.LoadFrom(path) : null;
    }
}

internal enum OfficialPackageKind
{
    Plugin,
    Extension,
}

internal sealed class OfficialPackageDescriptor
{
    private const string OfficialHost = "github.com";
    private const string OfficialPathPrefix = "/KRtkovo-eu-AI/salamander-plugins/releases/download/";

    private OfficialPackageDescriptor(Uri uri, OfficialPackageKind kind)
    {
        Uri = uri;
        Kind = kind;
    }

    public Uri Uri { get; }
    public OfficialPackageKind Kind { get; }

    public static bool TryParse(string? value, out OfficialPackageDescriptor? package)
    {
        package = null;
        if (!System.Uri.TryCreate(value, UriKind.Absolute, out var uri) ||
            uri.Scheme != Uri.UriSchemeHttps ||
            !string.Equals(uri.Host, OfficialHost, StringComparison.OrdinalIgnoreCase) ||
            !uri.AbsolutePath.StartsWith(OfficialPathPrefix, StringComparison.Ordinal) ||
            !uri.AbsolutePath.EndsWith(".7z", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        var fileName = System.Uri.UnescapeDataString(Path.GetFileName(uri.AbsolutePath));
        OfficialPackageKind kind;
        if (fileName.StartsWith("plugin", StringComparison.OrdinalIgnoreCase))
        {
            kind = OfficialPackageKind.Plugin;
        }
        else if (fileName.StartsWith("extension", StringComparison.OrdinalIgnoreCase))
        {
            kind = OfficialPackageKind.Extension;
        }
        else
        {
            return false;
        }

        package = new OfficialPackageDescriptor(uri, kind);
        return true;
    }
}

internal static class PluginPackageInstaller
{
    private const long MaxDownloadBytes = 256L * 1024 * 1024;
    private const long MaxExpandedBytes = 1024L * 1024 * 1024;

    public static bool CanInstall(PluginUpdateRow? row)
    {
        return row is not null &&
               OfficialPackageDescriptor.TryParse(row.WebUrl, out var package) &&
               IsHexSha256(row.PackageSha256) &&
               IsCompatibleWithInstalledPackage(row, ResolvePackageKind(row, package!));
    }

    public static bool TryTakeLastError(out string error)
    {
        return PluginReceiptStore.TryTakeLastError(out error);
    }

    public static async Task<string> StageAsync(PluginUpdateRow row)
    {
        if (!OfficialPackageDescriptor.TryParse(row.WebUrl, out var package) ||
            !IsCompatibleWithInstalledPackage(row, ResolvePackageKind(row, package!)))
        {
            throw new InvalidOperationException(NativeStrings.Get(NativeStringId.PluginInstallUnavailable));
        }

        if (!IsHexSha256(row.PackageSha256))
        {
            throw new InvalidDataException(NativeStrings.Get(NativeStringId.PluginInstallMissingHash));
        }

        var executableDirectory = PluginMetadata.GetExecutableDirectory()
            ?? throw new InvalidOperationException("The Salamander installation directory could not be determined.");
        var packageKind = ResolvePackageKind(row, package!);
        var packageRoot = packageKind == OfficialPackageKind.Plugin
            ? Path.Combine(executableDirectory, "plugins")
            : Path.Combine(executableDirectory, "extensions");
        var updateRoot = GetUpdateRoot();
        Directory.CreateDirectory(updateRoot);
        var stagingDirectory = Path.Combine(updateRoot, Guid.NewGuid().ToString("N", CultureInfo.InvariantCulture));
        var payloadDirectory = Path.Combine(stagingDirectory, "payload");
        Directory.CreateDirectory(payloadDirectory);

        try
        {
            var archivePath = Path.Combine(stagingDirectory, "package.7z");
            await DownloadAsync(package!.Uri, archivePath).ConfigureAwait(true);
            var actualHash = ComputeSha256(archivePath);
            if (!string.Equals(actualHash, row.PackageSha256, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(NativeStrings.Get(NativeStringId.PluginInstallHashMismatch));
            }

            var extractedRoot = ExtractAndValidate(archivePath, payloadDirectory, packageKind);
            AuthenticodeVerifier.VerifyExtractedPackage(extractedRoot, packageKind);
            var signer = FindFirstPartyPublisher(extractedRoot);
            var contentRoot = packageKind == OfficialPackageKind.Extension
                ? ResolveExtensionContentRoot(extractedRoot)
                : extractedRoot;
            var installed = !string.IsNullOrWhiteSpace(row.InstallDirectory);
            var targetDirectory = installed
                ? Path.GetFullPath(row.InstallDirectory)
                : Path.Combine(packageRoot, Path.GetFileName(
                    contentRoot.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)));
            ValidateTargetDirectory(targetDirectory, executableDirectory, packageRoot);
            var pluginRelativePath = packageKind == OfficialPackageKind.Plugin
                ? FindPluginRelativePath(extractedRoot, row.Id)
                : null;
            var extensionManifests = packageKind == OfficialPackageKind.Extension
                ? BuildExtensionManifestRecords(contentRoot, packageRoot, targetDirectory)
                : string.Empty;
            if (packageKind == OfficialPackageKind.Extension &&
                string.IsNullOrEmpty(extensionManifests) &&
                IsInside(targetDirectory, packageRoot))
            {
                throw new InvalidDataException(
                    "The extension package does not contain a manifest that can be registered.");
            }
            PluginReceiptStore.Upsert(new PluginReceiptRecord
            {
                id = row.Id,
                packageType = packageKind == OfficialPackageKind.Extension ? "extension" : "plugin",
                sourceUrl = row.WebUrl,
                packageSha256 = actualHash,
                signer = signer,
                version = row.LatestVersion,
                verifiedAt = DateTimeOffset.UtcNow.ToString("o", CultureInfo.InvariantCulture),
            });
            PluginReceiptStore.ClearLastError();
            var helperPath = WriteInstallHelper(
                stagingDirectory,
                contentRoot,
                targetDirectory,
                packageRoot,
                pluginRelativePath,
                appendPluginRecord: packageKind == OfficialPackageKind.Plugin && !installed,
                extensionManifests,
                PluginReceiptStore.GetStorePath());
            StartInstallHelper(
                helperPath,
                RequiresElevation(targetDirectory) || RequiresElevation(packageRoot));

            return installed
                ? NativeStrings.Get(NativeStringId.PluginUpdateScheduled)
                : NativeStrings.Get(NativeStringId.PluginInstallScheduled);
        }
        catch
        {
            TryDeleteDirectory(stagingDirectory);
            throw;
        }
    }

    private static OfficialPackageKind ResolvePackageKind(
        PluginUpdateRow row, OfficialPackageDescriptor package)
    {
        return row.CatalogKind switch
        {
            InstalledPackageKind.Plugin => OfficialPackageKind.Plugin,
            InstalledPackageKind.Extension => OfficialPackageKind.Extension,
            _ => package.Kind,
        };
    }

    private static bool IsCompatibleWithInstalledPackage(PluginUpdateRow row, OfficialPackageKind packageKind)
    {
        return row.InstalledKind == InstalledPackageKind.Unknown ||
               (row.InstalledKind == InstalledPackageKind.Plugin && packageKind == OfficialPackageKind.Plugin) ||
               (row.InstalledKind == InstalledPackageKind.Extension && packageKind == OfficialPackageKind.Extension);
    }

    private static async Task DownloadAsync(Uri uri, string destination)
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, uri);
        using var response = await SharedHttpClient.Instance.SendAsync(
            request,
            HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(false);
        response.EnsureSuccessStatusCode();
        if (response.Content.Headers.ContentLength is long contentLength &&
            contentLength > MaxDownloadBytes)
        {
            throw new InvalidDataException("The package is larger than the allowed download limit.");
        }

        using var source = await response.Content.ReadAsStreamAsync().ConfigureAwait(false);
        using var destinationStream = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        var buffer = new byte[81920];
        long total = 0;
        for (;;)
        {
            var read = await source.ReadAsync(buffer, 0, buffer.Length).ConfigureAwait(false);
            if (read == 0) break;
            total += read;
            if (total > MaxDownloadBytes)
            {
                throw new InvalidDataException("The package is larger than the allowed download limit.");
            }
            await destinationStream.WriteAsync(buffer, 0, read).ConfigureAwait(false);
        }
    }

    private static string ExtractAndValidate(string archivePath, string destination, OfficialPackageKind kind)
    {
        using var archive = ArchiveFactory.OpenArchive(archivePath);
        string? rootName = null;
        long expandedBytes = 0;
        foreach (var entry in archive.Entries)
        {
            var key = (entry.Key ?? string.Empty).Replace('/', Path.DirectorySeparatorChar);
            if (string.IsNullOrWhiteSpace(key)) continue;
            ValidateArchivePath(key);
            var segments = key.Split(new[] { Path.DirectorySeparatorChar }, StringSplitOptions.RemoveEmptyEntries);
            if (segments.Length == 0 || (segments.Length == 1 && !entry.IsDirectory))
            {
                throw new InvalidDataException("The package must contain exactly one top-level directory.");
            }
            if (rootName is null) rootName = segments[0];
            if (!string.Equals(rootName, segments[0], StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException("The package contains more than one top-level directory.");
            }
            if (!string.IsNullOrEmpty(entry.LinkTarget))
            {
                throw new InvalidDataException("Symbolic links are not allowed in update packages.");
            }

            var outputPath = Path.GetFullPath(Path.Combine(destination, key));
            EnsurePathIsInside(outputPath, destination);
            if (entry.IsDirectory)
            {
                Directory.CreateDirectory(outputPath);
                continue;
            }

            if (entry.Size < 0 || entry.Size > MaxExpandedBytes - expandedBytes)
            {
                throw new InvalidDataException("The expanded package is larger than the allowed limit.");
            }
            Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
            using var input = entry.OpenEntryStream();
            using var output = new FileStream(outputPath, FileMode.CreateNew, FileAccess.Write, FileShare.None);
            var buffer = new byte[81920];
            for (;;)
            {
                var read = input.Read(buffer, 0, buffer.Length);
                if (read == 0) break;
                expandedBytes += read;
                if (expandedBytes > MaxExpandedBytes)
                {
                    throw new InvalidDataException("The expanded package is larger than the allowed limit.");
                }
                output.Write(buffer, 0, read);
            }
        }

        if (string.IsNullOrWhiteSpace(rootName))
        {
            throw new InvalidDataException("The update package is empty.");
        }
        var root = Path.Combine(destination, rootName!);
        if (kind == OfficialPackageKind.Plugin && !Directory.EnumerateFiles(root, "*.spl", SearchOption.AllDirectories).Any())
        {
            throw new InvalidDataException("The plugin package does not contain an .spl module.");
        }
        if (kind == OfficialPackageKind.Extension &&
            !Directory.EnumerateFiles(root, "extension.json", SearchOption.AllDirectories).Any())
        {
            throw new InvalidDataException("The extension package does not contain extension.json.");
        }
        return root;
    }

    private static void ValidateArchivePath(string path)
    {
        if (Path.IsPathRooted(path) ||
            path.IndexOf(':') >= 0 ||
            path.Split(Path.DirectorySeparatorChar).Any(part => part == ".." || part == "."))
        {
            throw new InvalidDataException("The package contains an unsafe path.");
        }
    }

    private static void EnsurePathIsInside(string path, string root)
    {
        var normalizedRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        if (!path.StartsWith(normalizedRoot, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException("The package contains a path outside its top-level directory.");
        }
    }

    private static void ValidateTargetDirectory(string target, string executableDirectory, string packageRoot)
    {
        var normalizedTarget = Path.GetFullPath(target).TrimEnd(Path.DirectorySeparatorChar);
        var forbidden = new[] { executableDirectory, packageRoot }
            .Select(path => Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar));
        if (forbidden.Any(path => string.Equals(path, normalizedTarget, StringComparison.OrdinalIgnoreCase)) ||
            string.IsNullOrWhiteSpace(Path.GetFileName(normalizedTarget)))
        {
            throw new InvalidOperationException("The installed package path is not a safe package directory.");
        }
    }

    // A package with one extension.json is that manifest's directory, even when
    // the archive wraps it in another folder. Several manifests, such as the
    // demos bundle, stay rooted at the archive directory.
    private static string ResolveExtensionContentRoot(string extractedRoot)
    {
        var manifests = Directory.EnumerateFiles(
            extractedRoot, "extension.json", SearchOption.AllDirectories).ToList();
        if (manifests.Count != 1)
        {
            return extractedRoot;
        }

        var directory = Path.GetDirectoryName(manifests[0]);
        return string.IsNullOrEmpty(directory) ? extractedRoot : directory;
    }

    private static string BuildExtensionManifestRecords(
        string contentRoot, string extensionsRoot, string targetDirectory)
    {
        var prefix = Path.GetFullPath(extensionsRoot).TrimEnd(Path.DirectorySeparatorChar) +
                     Path.DirectorySeparatorChar;
        var contentPrefix = contentRoot.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) +
                            Path.DirectorySeparatorChar;
        var records = new List<string>();
        foreach (var manifest in Directory.EnumerateFiles(
                     contentRoot, "extension.json", SearchOption.AllDirectories))
        {
            if (!manifest.StartsWith(contentPrefix, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var relativeToContent = manifest.Substring(contentPrefix.Length);
            var installed = Path.GetFullPath(Path.Combine(targetDirectory, relativeToContent));
            if (!installed.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var record = installed.Substring(prefix.Length)
                .Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
            if (!IsSafeExtensionRelative(record))
            {
                continue;
            }

            records.Add(record);
        }

        records.Sort(StringComparer.OrdinalIgnoreCase);
        return string.Join("\n", records);
    }

    private static bool IsSafeExtensionRelative(string relative)
    {
        if (string.IsNullOrWhiteSpace(relative) || relative.Length > 16000 || relative.IndexOf(':') >= 0)
        {
            return false;
        }

        if (relative[0] == '\\' || relative[0] == '/')
        {
            return false;
        }

        var parts = relative.Split(new[] { '\\', '/' }, StringSplitOptions.None);
        foreach (var part in parts)
        {
            if (part.Length == 0 || part == "." || part == "..")
            {
                return false;
            }
        }

        return relative.EndsWith("extension.json", StringComparison.OrdinalIgnoreCase);
    }

    private static string FindPluginRelativePath(string extractedRoot, string id)
    {
        var modules = Directory.EnumerateFiles(extractedRoot, "*.spl", SearchOption.AllDirectories).ToList();
        var preferred = modules.FirstOrDefault(path =>
            string.Equals(Path.GetFileNameWithoutExtension(path), id, StringComparison.OrdinalIgnoreCase) ||
            string.Equals(Path.GetFileNameWithoutExtension(path), Path.GetFileName(extractedRoot), StringComparison.OrdinalIgnoreCase));
        var selected = preferred ?? (modules.Count == 1
            ? modules[0]
            : throw new InvalidDataException("The package contains multiple .spl modules and the main plugin cannot be identified."));
        return selected.Substring(extractedRoot.Length).TrimStart(Path.DirectorySeparatorChar);
    }

    private static string FindFirstPartyPublisher(string extractedRoot)
    {
        var firstParty = Directory.EnumerateFiles(extractedRoot, "*.spl", SearchOption.AllDirectories)
            .Concat(Directory.EnumerateFiles(extractedRoot, "*.slg", SearchOption.AllDirectories));
        foreach (var file in firstParty)
        {
            if (AuthenticodeVerifier.TryGetPublisher(file, out var publisher) &&
                AuthenticodeVerifier.IsExpectedPublisher(publisher))
            {
                return publisher;
            }
        }
        return string.Empty;
    }

    private static bool IsHexSha256(string? value)
    {
        if (string.IsNullOrWhiteSpace(value) || value!.Length != 64)
        {
            return false;
        }

        foreach (var ch in value)
        {
            var isHex = (ch >= '0' && ch <= '9') ||
                        (ch >= 'a' && ch <= 'f') ||
                        (ch >= 'A' && ch <= 'F');
            if (!isHex)
            {
                return false;
            }
        }
        return true;
    }

    private static string ComputeSha256(string path)
    {
        using var stream = File.OpenRead(path);
        using var sha = SHA256.Create();
        var hash = sha.ComputeHash(stream);
        var builder = new StringBuilder(hash.Length * 2);
        foreach (var value in hash)
        {
            builder.Append(value.ToString("x2", CultureInfo.InvariantCulture));
        }
        return builder.ToString();
    }

    private static string WriteInstallHelper(
        string stagingDirectory,
        string sourceDirectory,
        string targetDirectory,
        string packageRoot,
        string? pluginRelativePath,
        bool appendPluginRecord,
        string? extensionManifests,
        string? receiptsPath)
    {
        var helperPath = Path.Combine(stagingDirectory, "install.ps1");
        var receipts = receiptsPath ?? string.Empty;
        var script = $@"$ErrorActionPreference = 'Stop'
function Decode([string]$value) {{ [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($value)) }}
function Set-ReceiptError([string]$text) {{
    $path = Decode('{Encode(receipts)}')
    if ([string]::IsNullOrWhiteSpace($path)) {{ return }}
    $dir = [IO.Path]::GetDirectoryName($path)
    if ($dir) {{ New-Item -ItemType Directory -Path $dir -Force | Out-Null }}
    $file = @{{ schemaVersion = 1; lastInstallError = $text; receipts = @() }}
    if (Test-Path -LiteralPath $path) {{
        try {{ $file = Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json }} catch {{ }}
    }}
    $file.lastInstallError = $text
    $json = $file | ConvertTo-Json -Depth 6
    $tmp = $path + '.' + [guid]::NewGuid().ToString('N') + '.tmp'
    [IO.File]::WriteAllText($tmp, $json, [Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $tmp -Destination $path -Force
}}
function Use-LongPath([string]$path) {{
    $full = [IO.Path]::GetFullPath($path)
    if ($full.Length -lt 260 -or $full.StartsWith('\\?\')) {{ return $full }}
    if ($full.StartsWith('\\')) {{ return '\\?\UNC\' + $full.Substring(2) }}
    return '\\?\' + $full
}}
function Copy-PackageTree([string]$from, [string]$to) {{
    if (-not (Test-Path -LiteralPath $from)) {{ throw 'The staged package is missing.' }}
    $parent = [IO.Path]::GetDirectoryName($to)
    if ($parent) {{ New-Item -ItemType Directory -Path $parent -Force | Out-Null }}
    New-Item -ItemType Directory -Path $to -Force | Out-Null
    Get-ChildItem -LiteralPath $to -Recurse -Force -ErrorAction SilentlyContinue | ForEach-Object {{
        $_.Attributes = $_.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly)
    }}
    $robocopy = Join-Path $env:SystemRoot 'System32\robocopy.exe'
    if (Test-Path -LiteralPath $robocopy) {{
        & $robocopy (Use-LongPath $from) (Use-LongPath $to) /E /R:2 /W:1 /NFL /NDL /NJH /NJS /NP /IS /IT | Out-Null
        if ($LASTEXITCODE -ge 8) {{ throw ""Copying package files failed (robocopy $LASTEXITCODE)."" }}
        $global:LASTEXITCODE = 0
    }} else {{
        Copy-Item -LiteralPath (Join-Path $from '*') -Destination $to -Recurse -Force
    }}
}}
function Remove-ExtraPackageFiles([string]$from, [string]$to) {{
    $fromFull = [IO.Path]::GetFullPath($from).TrimEnd('\')
    $toFull = [IO.Path]::GetFullPath($to).TrimEnd('\')
    if ($toFull.Length -lt 4) {{ throw 'Refusing to update an unsafe package directory.' }}
    if ($toFull.Equals($fromFull, [StringComparison]::OrdinalIgnoreCase)) {{ throw 'Refusing to clean the staged package.' }}
    $items = @(Get-ChildItem -LiteralPath $toFull -Recurse -Force -ErrorAction SilentlyContinue | Sort-Object {{ $_.FullName.Length }} -Descending)
    foreach ($item in $items) {{
        if (-not (Test-Path -LiteralPath $item.FullName)) {{ continue }}
        $relative = $item.FullName.Substring($toFull.Length).TrimStart('\')
        if ([string]::IsNullOrEmpty($relative)) {{ continue }}
        if (Test-Path -LiteralPath (Join-Path $fromFull $relative)) {{ continue }}
        Remove-Item -LiteralPath $item.FullName -Recurse -Force -ErrorAction SilentlyContinue
    }}
}}
function Invoke-PackageInstallLock([scriptblock]$Action) {{
    $mutex = New-Object System.Threading.Mutex($false, 'Local\OpenSamandarin.PackageInstall')
    $acquired = $false
    try {{
        try {{
            $acquired = $mutex.WaitOne([TimeSpan]::FromMinutes(30))
        }} catch [System.Threading.AbandonedMutexException] {{
            $acquired = $true
        }}
        if (-not $acquired) {{ throw 'Timed out waiting for another package install to finish.' }}
        & $Action
    }} finally {{
        if ($acquired) {{ $mutex.ReleaseMutex() | Out-Null }}
        $mutex.Dispose()
    }}
}}
$sourceProcessId = {Process.GetCurrentProcess().Id}
$source = Decode('{Encode(sourceDirectory)}')
$target = Decode('{Encode(targetDirectory)}')
$pluginsRoot = Decode('{Encode(packageRoot)}')
$staging = Decode('{Encode(stagingDirectory)}')
$pluginRelative = Decode('{Encode(pluginRelativePath ?? string.Empty)}')
$extensionManifests = Decode('{Encode(extensionManifests ?? string.Empty)}')
try {{
    $deadline = (Get-Date).AddMinutes(30)
    do {{
        $running = Get-Process -Id $sourceProcessId -ErrorAction SilentlyContinue
        if (-not $running) {{ break }}
        if ((Get-Date) -ge $deadline) {{ break }}
        Start-Sleep -Milliseconds 250
    }} while ($true)
    Start-Sleep -Milliseconds 500
    Invoke-PackageInstallLock {{
    Copy-PackageTree $source $target
    Remove-ExtraPackageFiles $source $target
    if ($pluginRelative.Length -ne 0) {{
        $versionFile = Join-Path $pluginsRoot 'plugins.ver'
        [string[]]$lines = if (Test-Path -LiteralPath $versionFile) {{ [IO.File]::ReadAllLines($versionFile, [Text.Encoding]::Default) }} else {{ @() }}
        if ($lines.Count -gt 0) {{ $lines[0] = $lines[0].TrimStart([char]0xFEFF) }}
        $oldVersion = 0
        if ($lines.Count -gt 0 -and $lines[0] -match '^\s*(\d+)') {{ $oldVersion = [int]$Matches[1] }}
        $newVersion = $oldVersion + 1
        if ($lines.Count -eq 0) {{ $lines = @([string]$newVersion) }} else {{ $lines[0] = [string]$newVersion }}
        if ({(appendPluginRecord ? "$true" : "$false")}) {{
            $modulePath = Join-Path $target $pluginRelative
            $pluginsPrefix = [IO.Path]::GetFullPath($pluginsRoot).TrimEnd('\') + '\'
            $recordPath = [IO.Path]::GetFullPath($modulePath)
            if ($recordPath.StartsWith($pluginsPrefix, [StringComparison]::OrdinalIgnoreCase)) {{
                $recordPath = $recordPath.Substring($pluginsPrefix.Length)
            }}
            $lines += ([string]$newVersion + ':' + $recordPath)
        }}
        [IO.File]::WriteAllLines($versionFile, [string[]]$lines, [Text.Encoding]::Default)
    }}
    if ($extensionManifests.Length -ne 0) {{
        $versionFile = Join-Path $pluginsRoot 'extensions.ver'
        [string[]]$lines = if (Test-Path -LiteralPath $versionFile) {{ [IO.File]::ReadAllLines($versionFile, [Text.UTF8Encoding]::new($false)) }} else {{ @() }}
        if ($lines.Count -gt 0) {{ $lines[0] = $lines[0].TrimStart([char]0xFEFF) }}
        $oldVersion = 0
        if ($lines.Count -gt 0 -and $lines[0] -match '^\s*(\d+)') {{ $oldVersion = [int]$Matches[1] }}
        $newVersion = $oldVersion + 1
        if ($lines.Count -eq 0) {{ $lines = @([string]$newVersion) }} else {{ $lines[0] = [string]$newVersion }}
        foreach ($relative in ($extensionManifests -split ""`n"")) {{
            $relative = $relative.Trim()
            if ($relative.Length -eq 0) {{ continue }}
            $lines += ([string]$newVersion + ':' + $relative)
        }}
        [IO.File]::WriteAllLines($versionFile, [string[]]$lines, [Text.UTF8Encoding]::new($false))
    }}
    Set-ReceiptError $null
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
    }}
}} catch {{
    Set-ReceiptError ($_ | Out-String)
}}";
        File.WriteAllText(helperPath, script, new UTF8Encoding(encoderShouldEmitUTF8Identifier: true));
        return helperPath;
    }

    private static void StartInstallHelper(string helperPath, bool elevate)
    {
        var startInfo = new ProcessStartInfo
        {
            FileName = "powershell.exe",
            Arguments = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File " + QuoteArgument(helperPath),
            UseShellExecute = true,
            WindowStyle = ProcessWindowStyle.Hidden,
        };
        if (elevate) startInfo.Verb = "runas";
        Process.Start(startInfo);
    }

    private static bool RequiresElevation(string path)
    {
        return IsInside(path, Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles)) ||
               IsInside(path, Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86));
    }

    private static bool IsInside(string path, string root)
    {
        if (string.IsNullOrWhiteSpace(root)) return false;
        var normalizedPath = Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        var normalizedRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        return normalizedPath.StartsWith(normalizedRoot, StringComparison.OrdinalIgnoreCase);
    }

    private static string Encode(string value) =>
        Convert.ToBase64String(Encoding.UTF8.GetBytes(value));

    private static string GetUpdateRoot() =>
        Path.Combine(Path.GetTempPath(), "OpenSalamander", "Samandarin", "Updates");

    private static string QuoteArgument(string value) =>
        "\"" + value.Replace("\"", "\\\"") + "\"";

    private static void TryDeleteDirectory(string path)
    {
        try
        {
            if (Directory.Exists(path)) Directory.Delete(path, recursive: true);
        }
        catch
        {
        }
    }
}
