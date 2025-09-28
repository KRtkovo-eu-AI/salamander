// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Globalization;
using System.Windows.Forms;

namespace OpenSalamander.TreeViewBrowser;

public static class EntryPoint
{
    private static bool _visualsEnabled;

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
                "ShowBrowser" => BrowserHost.Show(parent, payload),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            ShowError(parent, ex);
            return -1;
        }
    }

    private static void EnsureApplicationInitialized()
    {
        if (_visualsEnabled)
        {
            return;
        }

        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        _visualsEnabled = true;
    }

    private static IntPtr ParseHandle(string text)
    {
        if (ulong.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value))
        {
            return new IntPtr(unchecked((long)value));
        }

        return IntPtr.Zero;
    }

    private static void ShowError(IntPtr parent, Exception ex)
    {
        var message = $"Unexpected managed exception:\n{ex.Message}";
        var caption = "Tree View Browser Plugin";

        if (parent != IntPtr.Zero)
        {
            MessageBox.Show(new WindowHandleWrapper(parent), message, caption,
                MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        else
        {
            MessageBox.Show(message, caption, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
