// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Globalization;
using System.Windows.Forms;

namespace OpenSalamander.CSDemo;

public static class EntryPoint
{
    private static bool _visualsEnabled;

    [STAThread]
    public static int Dispatch(string? argument)
    {
        try
        {
            EnsureApplicationInitialized();

            var parts = (argument ?? string.Empty).Split(new[] { ';' }, 3);
            var command = parts.Length > 0 ? parts[0] : string.Empty;
            var parentHandle = ParseHandle(parts.Length > 1 ? parts[1] : string.Empty);
            var payload = parts.Length > 2 ? parts[2] : string.Empty;

            return command switch
            {
                "About" => ShowAbout(parentHandle),
                "Configure" => ShowConfiguration(parentHandle),
                "Menu" => ShowHello(parentHandle, payload),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            ShowError("Unexpected managed exception:", ex);
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

    private static int ShowAbout(IntPtr parent)
    {
        using var dialog = new Form
        {
            Text = "C# Demo Plugin",
            StartPosition = FormStartPosition.CenterParent,
            FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false,
            MinimizeBox = false,
            ClientSize = new System.Drawing.Size(380, 160),
        };

        var description = new Label
        {
            Text = "This dialog is implemented in managed C# code.\n\n" +
                   "It demonstrates how a WinForms UI can be displayed from a native " +
                   "Open Salamander plugin.",
            Dock = DockStyle.Fill,
            AutoSize = false,
            Padding = new Padding(12),
        };

        var okButton = new Button
        {
            Text = "Close",
            DialogResult = DialogResult.OK,
            Anchor = AnchorStyles.Bottom | AnchorStyles.Right,
            Size = new System.Drawing.Size(85, 26),
            Location = new System.Drawing.Point(dialog.ClientSize.Width - 97, dialog.ClientSize.Height - 38),
        };

        dialog.Controls.Add(description);
        dialog.Controls.Add(okButton);
        dialog.AcceptButton = okButton;

        ThemeHelper.ApplyTheme(dialog);
        ShowDialog(dialog, parent);
        return 0;
    }

    private static int ShowConfiguration(IntPtr parent)
    {
        using var dialog = new Form
        {
            Text = "C# Demo Configuration",
            StartPosition = FormStartPosition.CenterParent,
            FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false,
            MinimizeBox = false,
            ClientSize = new System.Drawing.Size(420, 210),
        };

        var info = new Label
        {
            Text = "Managed configuration window.\n" +
                   "In a real plugin you could load and save settings " +
                   "through Salamander's registry APIs.",
            AutoSize = false,
            Dock = DockStyle.Top,
            Padding = new Padding(12),
            Height = 80,
        };

        var textLabel = new Label
        {
            Text = "Sample option:",
            Location = new System.Drawing.Point(16, 96),
            AutoSize = true,
        };

        var textBox = new TextBox
        {
            Location = new System.Drawing.Point(16, 118),
            Width = 270,
            Text = "Hello from managed code!",
        };

        var saveButton = new Button
        {
            Text = "OK",
            DialogResult = DialogResult.OK,
            Anchor = AnchorStyles.Bottom | AnchorStyles.Right,
            Size = new System.Drawing.Size(85, 26),
            Location = new System.Drawing.Point(dialog.ClientSize.Width - 185, dialog.ClientSize.Height - 42),
        };

        var cancelButton = new Button
        {
            Text = "Cancel",
            DialogResult = DialogResult.Cancel,
            Anchor = AnchorStyles.Bottom | AnchorStyles.Right,
            Size = new System.Drawing.Size(85, 26),
            Location = new System.Drawing.Point(dialog.ClientSize.Width - 94, dialog.ClientSize.Height - 42),
        };

        dialog.Controls.Add(info);
        dialog.Controls.Add(textLabel);
        dialog.Controls.Add(textBox);
        dialog.Controls.Add(saveButton);
        dialog.Controls.Add(cancelButton);

        dialog.AcceptButton = saveButton;
        dialog.CancelButton = cancelButton;

        ThemeHelper.ApplyTheme(dialog);
        var result = ShowDialog(dialog, parent);
        if (result == DialogResult.OK)
        {
            // In a real plugin the value would be forwarded to the native layer
            // or persisted via Salamander's registry helpers.
            ThemeHelper.ShowMessageBox(new WindowHandleWrapper(parent),
                $"Managed sample saved:\n{textBox.Text}",
                "C# Demo Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        return 0;
    }

    private static int ShowHello(IntPtr parent, string payload)
    {
        using var dialog = new Form
        {
            Text = "Managed Window",
            StartPosition = FormStartPosition.CenterParent,
            ClientSize = new System.Drawing.Size(360, 180),
            FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false,
            MinimizeBox = false,
        };

        var label = new Label
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(16),
            AutoSize = false,
            TextAlign = System.Drawing.ContentAlignment.MiddleCenter,
            Text = string.IsNullOrEmpty(payload)
                ? "Hello from the managed world!"
                : string.Format(CultureInfo.CurrentCulture, "Managed payload: {0}", payload),
        };

        dialog.Controls.Add(label);

        var okButton = new Button
        {
            Text = "Close",
            DialogResult = DialogResult.OK,
            Anchor = AnchorStyles.Bottom | AnchorStyles.Right,
            Size = new System.Drawing.Size(85, 26),
            Location = new System.Drawing.Point(dialog.ClientSize.Width - 97, dialog.ClientSize.Height - 38),
        };

        dialog.Controls.Add(okButton);
        dialog.AcceptButton = okButton;

        ThemeHelper.ApplyTheme(dialog);
        ShowDialog(dialog, parent);
        return 0;
    }

    private static DialogResult ShowDialog(Form dialog, IntPtr parent)
    {
        IWin32Window? owner = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;
        return owner is null ? dialog.ShowDialog() : dialog.ShowDialog(owner);
    }

    private static IntPtr ParseHandle(string text)
    {
        if (ulong.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value))
        {
            return new IntPtr(unchecked((long)value));
        }

        return IntPtr.Zero;
    }

    private static void ShowError(string caption, Exception ex)
    {
        ThemeHelper.ShowMessageBox(
            null,
            $"{caption}\n{ex.Message}",
            "C# Demo Plugin",
            MessageBoxButtons.OK,
            MessageBoxIcon.Error);
    }

    private sealed class WindowHandleWrapper : IWin32Window
    {
        public WindowHandleWrapper(IntPtr handle)
        {
            Handle = handle;
        }

        public IntPtr Handle { get; }
    }
}
