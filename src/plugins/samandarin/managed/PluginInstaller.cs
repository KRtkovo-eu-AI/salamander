// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Reflection;
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
               IsCompatibleWithInstalledPackage(row, package!);
    }

    public static bool TryTakeLastError(out string error)
    {
        error = string.Empty;
        var path = Path.Combine(GetUpdateRoot(), "last-update-error.txt");
        try
        {
            if (!File.Exists(path)) return false;
            error = File.ReadAllText(path, Encoding.UTF8);
            if (error.Length > 4096) error = error.Substring(0, 4096);
            File.Delete(path);
            return !string.IsNullOrWhiteSpace(error);
        }
        catch
        {
            return false;
        }
    }

    public static async Task<string> StageAsync(PluginUpdateRow row)
    {
        if (!OfficialPackageDescriptor.TryParse(row.WebUrl, out var package) ||
            !IsCompatibleWithInstalledPackage(row, package!))
        {
            throw new InvalidOperationException("Only official plugin and extension .7z packages can be installed automatically.");
        }

        var executableDirectory = PluginMetadata.GetExecutableDirectory()
            ?? throw new InvalidOperationException("The Salamander installation directory could not be determined.");
        var packageRoot = package!.Kind == OfficialPackageKind.Plugin
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
            await DownloadAsync(package.Uri, archivePath).ConfigureAwait(true);
            var extractedRoot = ExtractAndValidate(archivePath, payloadDirectory, package.Kind);
            var installed = !string.IsNullOrWhiteSpace(row.InstallDirectory);
            var targetDirectory = installed
                ? Path.GetFullPath(row.InstallDirectory)
                : Path.Combine(packageRoot, Path.GetFileName(extractedRoot));
            ValidateTargetDirectory(targetDirectory, executableDirectory, packageRoot);
            var pluginRelativePath = package.Kind == OfficialPackageKind.Plugin
                ? FindPluginRelativePath(extractedRoot, row.Id)
                : null;
            var helperPath = WriteInstallHelper(
                stagingDirectory,
                extractedRoot,
                targetDirectory,
                packageRoot,
                pluginRelativePath,
                appendPluginRecord: package.Kind == OfficialPackageKind.Plugin && !installed);
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

    private static bool IsCompatibleWithInstalledPackage(PluginUpdateRow row, OfficialPackageDescriptor package)
    {
        return row.InstalledKind == InstalledPackageKind.Unknown ||
               (row.InstalledKind == InstalledPackageKind.Plugin && package.Kind == OfficialPackageKind.Plugin) ||
               (row.InstalledKind == InstalledPackageKind.Extension && package.Kind == OfficialPackageKind.Extension);
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
        if (kind == OfficialPackageKind.Extension && !File.Exists(Path.Combine(root, "extension.json")))
        {
            throw new InvalidDataException("The extension package does not contain extension.json in its top-level directory.");
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

    private static string WriteInstallHelper(
        string stagingDirectory,
        string extractedRoot,
        string targetDirectory,
        string pluginsRoot,
        string? pluginRelativePath,
        bool appendPluginRecord)
    {
        var helperPath = Path.Combine(stagingDirectory, "install.ps1");
        var errorLog = Path.Combine(Path.GetDirectoryName(stagingDirectory)!, "last-update-error.txt");
        var script = $@"$ErrorActionPreference = 'Stop'
function Decode([string]$value) {{ [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($value)) }}
$sourceProcessId = {Process.GetCurrentProcess().Id}
$source = Decode('{Encode(extractedRoot)}')
$target = Decode('{Encode(targetDirectory)}')
$pluginsRoot = Decode('{Encode(pluginsRoot)}')
$staging = Decode('{Encode(stagingDirectory)}')
$pluginRelative = Decode('{Encode(pluginRelativePath ?? string.Empty)}')
$errorLog = Decode('{Encode(errorLog)}')
$backup = $target + '.samandarin-backup-{Guid.NewGuid():N}'
try {{
    Wait-Process -Id $sourceProcessId -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null
    if (Test-Path -LiteralPath $target) {{ Move-Item -LiteralPath $target -Destination $backup -Force }}
    try {{
        Move-Item -LiteralPath $source -Destination $target -Force
    }} catch {{
        if ((Test-Path -LiteralPath $backup) -and -not (Test-Path -LiteralPath $target)) {{
            Move-Item -LiteralPath $backup -Destination $target -Force
        }}
        throw
    }}
    if (Test-Path -LiteralPath $backup) {{ Remove-Item -LiteralPath $backup -Recurse -Force }}
    if ($pluginRelative.Length -ne 0) {{
        $versionFile = Join-Path $pluginsRoot 'plugins.ver'
        [string[]]$lines = if (Test-Path -LiteralPath $versionFile) {{ [IO.File]::ReadAllLines($versionFile, [Text.Encoding]::Default) }} else {{ @() }}
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
    if (Test-Path -LiteralPath $errorLog) {{ Remove-Item -LiteralPath $errorLog -Force -ErrorAction SilentlyContinue }}
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
}} catch {{
    [IO.File]::WriteAllText($errorLog, ($_ | Out-String), [Text.Encoding]::UTF8)
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
        Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "OpenSalamander",
            "Samandarin",
            "Updates");

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
