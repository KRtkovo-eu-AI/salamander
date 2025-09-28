// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace OpenSalamander.TreeViewBrowser;

internal static class BrowserHost
{
    public static int Show(IntPtr parent, string payload)
    {
        var initialPath = ParseInitialPath(payload);

        using var window = new TreeViewBrowserForm(initialPath);
        if (parent != IntPtr.Zero)
        {
            window.ShowDialog(new WindowHandleWrapper(parent));
        }
        else
        {
            window.ShowDialog();
        }

        return 0;
    }

    private static string ParseInitialPath(string? payload)
    {
        if (string.IsNullOrWhiteSpace(payload))
        {
            return string.Empty;
        }

        var entries = payload.Split('|');
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in entries)
        {
            if (string.IsNullOrWhiteSpace(entry))
            {
                continue;
            }

            var kv = entry.Split(new[] { '=' }, 2);
            if (kv.Length == 0)
            {
                continue;
            }

            var key = kv[0].Trim();
            var value = kv.Length > 1 ? kv[1] : string.Empty;
            if (key.Length == 0)
            {
                continue;
            }

            map[key] = value;
        }

        if (!map.TryGetValue("path", out var encodedPath) || string.IsNullOrWhiteSpace(encodedPath))
        {
            return string.Empty;
        }

        return TryDecodeBase64(encodedPath, out var decoded)
            ? decoded
            : encodedPath.Trim();
    }

    private static bool TryDecodeBase64(string value, out string decoded)
    {
        decoded = string.Empty;
        var trimmed = value?.Trim();
        if (string.IsNullOrEmpty(trimmed))
        {
            return false;
        }

        try
        {
            var bytes = Convert.FromBase64String(trimmed);
            decoded = Encoding.UTF8.GetString(bytes);
            return true;
        }
        catch (FormatException)
        {
            return false;
        }
        catch (DecoderFallbackException)
        {
            return false;
        }
    }
}
