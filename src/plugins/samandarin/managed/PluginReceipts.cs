// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;

namespace OpenSalamander.Samandarin;

internal sealed class PluginReceiptStoreFile
{
    public int schemaVersion { get; set; } = 1;
    public string? lastInstallError { get; set; }
    public PluginReceiptRecord[]? receipts { get; set; }
}

internal sealed class PluginReceiptRecord
{
    public string? id { get; set; }
    public string? packageType { get; set; }
    public string? sourceUrl { get; set; }
    public string? packageSha256 { get; set; }
    public string? signer { get; set; }
    public string? version { get; set; }
    public string? verifiedAt { get; set; }
}

internal static class PluginReceiptStore
{
    private const string FileName = "plugin-receipts.json";
    private static readonly object SyncRoot = new();
    private static readonly JavaScriptSerializer Serializer = new() { MaxJsonLength = 4 * 1024 * 1024 };

    public static string? GetStorePath()
    {
        var directory = PluginMetadata.GetExecutableDirectory();
        return string.IsNullOrWhiteSpace(directory) ? null : Path.Combine(directory!, FileName);
    }

    public static bool TryTakeLastError(out string error)
    {
        error = string.Empty;
        lock (SyncRoot)
        {
            var file = Load();
            if (string.IsNullOrWhiteSpace(file.lastInstallError))
            {
                return false;
            }

            error = file.lastInstallError!;
            if (error.Length > 4096)
            {
                error = error.Substring(0, 4096);
            }
            file.lastInstallError = null;
            Save(file);
            return true;
        }
    }

    public static void SetLastError(string error)
    {
        lock (SyncRoot)
        {
            var file = Load();
            file.lastInstallError = string.IsNullOrWhiteSpace(error) ? null : error;
            Save(file);
        }
    }

    public static void ClearLastError()
    {
        SetLastError(string.Empty);
    }

    public static void Upsert(PluginReceiptRecord receipt)
    {
        if (receipt is null || string.IsNullOrWhiteSpace(receipt.id))
        {
            return;
        }

        lock (SyncRoot)
        {
            var file = Load();
            var receipts = (file.receipts ?? Array.Empty<PluginReceiptRecord>()).ToList();
            receipts.RemoveAll(item =>
                string.Equals(item.id, receipt.id, StringComparison.OrdinalIgnoreCase) &&
                string.Equals(item.packageType ?? "plugin", receipt.packageType ?? "plugin", StringComparison.OrdinalIgnoreCase));
            receipts.Add(receipt);
            file.schemaVersion = 1;
            file.receipts = receipts.ToArray();
            Save(file);
        }
    }

    public static PluginReceiptRecord? Find(string id, string? packageType)
    {
        if (string.IsNullOrWhiteSpace(id))
        {
            return null;
        }

        lock (SyncRoot)
        {
            var file = Load();
            return (file.receipts ?? Array.Empty<PluginReceiptRecord>()).FirstOrDefault(item =>
                string.Equals(item.id, id, StringComparison.OrdinalIgnoreCase) &&
                (string.IsNullOrWhiteSpace(packageType) ||
                 string.Equals(item.packageType ?? "plugin", packageType, StringComparison.OrdinalIgnoreCase)));
        }
    }

    internal static PluginReceiptStoreFile Load()
    {
        var path = GetStorePath();
        if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
        {
            return new PluginReceiptStoreFile { schemaVersion = 1, receipts = Array.Empty<PluginReceiptRecord>() };
        }

        try
        {
            var json = File.ReadAllText(path, Encoding.UTF8);
            var parsed = Serializer.Deserialize<PluginReceiptStoreFile>(json);
            if (parsed is null)
            {
                return new PluginReceiptStoreFile { schemaVersion = 1, receipts = Array.Empty<PluginReceiptRecord>() };
            }
            parsed.receipts ??= Array.Empty<PluginReceiptRecord>();
            return parsed;
        }
        catch
        {
            return new PluginReceiptStoreFile { schemaVersion = 1, receipts = Array.Empty<PluginReceiptRecord>() };
        }
    }

    private static void Save(PluginReceiptStoreFile file)
    {
        var path = GetStorePath();
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        file.schemaVersion = 1;
        file.receipts ??= Array.Empty<PluginReceiptRecord>();
        var json = Serializer.Serialize(file);
        var directory = Path.GetDirectoryName(path);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory!);
        }

        var temporary = path + "." + Guid.NewGuid().ToString("N", CultureInfo.InvariantCulture) + ".tmp";
        File.WriteAllText(temporary, json, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
        if (File.Exists(path))
        {
            File.Replace(temporary, path, destinationBackupFileName: null);
        }
        else
        {
            File.Move(temporary, path);
        }
    }
}
