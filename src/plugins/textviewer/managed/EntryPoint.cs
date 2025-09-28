// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Globalization;
using System.Windows.Forms;

namespace OpenSalamander.TextViewer;

public static class EntryPoint
{
    private static bool _initialized;

    [STAThread]
    public static int Dispatch(string? argument)
    {
        IntPtr parent = IntPtr.Zero;

        try
        {
            EnsureApplicationInitialized();

            var parts = (argument ?? string.Empty).Split(new[] { ';' }, 3);
            var command = parts.Length > 0 ? parts[0] : string.Empty;
            parent = ParseHandle(parts.Length > 1 ? parts[1] : string.Empty);
            var payload = parts.Length > 2 ? parts[2] : string.Empty;

            return command switch
            {
                "View" => ViewerHost.Launch(parent, payload, asynchronous: true),
                "ViewSync" => ViewerHost.Launch(parent, payload, asynchronous: false),
                "Release" => ViewerHost.ReleaseSessions(ShouldForceRelease(payload)),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            if (parent != IntPtr.Zero)
            {
                MessageBox.Show(new WindowHandleWrapper(parent),
                    $"Unexpected managed exception:\n{ex.Message}",
                    "Text Viewer Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            else
            {
                MessageBox.Show($"Unexpected managed exception:\n{ex.Message}",
                    "Text Viewer Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            return -1;
        }
    }

    private static void EnsureApplicationInitialized()
    {
        if (_initialized)
        {
            return;
        }

        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        _initialized = true;
    }

    private static IntPtr ParseHandle(string text)
    {
        if (ulong.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value))
        {
            return new IntPtr(unchecked((long)value));
        }

        return IntPtr.Zero;
    }

    private static bool ShouldForceRelease(string? payload)
    {
        var payloadText = payload?.Trim() ?? string.Empty;

        if (payloadText.Length == 0)
        {
            return false;
        }

        var segments = payloadText.Split('|');
        foreach (var segment in segments)
        {
            if (string.IsNullOrWhiteSpace(segment))
            {
                continue;
            }

            var kv = segment.Split(new[] { '=' }, 2);
            if (kv.Length == 0)
            {
                continue;
            }

            var key = kv[0].Trim();
            if (!key.Equals("force", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            if (kv.Length == 1)
            {
                return true;
            }

            var value = kv[1].Trim();
            return value.Equals("1", StringComparison.OrdinalIgnoreCase) ||
                   value.Equals("true", StringComparison.OrdinalIgnoreCase);
        }

        return false;
    }
}
