// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Windows.Forms;

namespace OpenSalamander.HyperVM;

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

            return command switch
            {
                "About" => ShowAbout(parentHandle),
                "Configure" => ShowConfiguration(parentHandle),
                "Menu" => ShowMachines(parentHandle),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            MessageBox.Show(ex.ToString(), "Hyper-V Machines Plugin", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return -1;
        }
    }

    private static int ShowMachines(IntPtr parent)
    {
        var machines = GetHyperVMachines();
        var text = machines.Count == 0 ? "No Hyper-V virtual machines found." : string.Join(Environment.NewLine, machines);
        MessageBox.Show(new WindowHandleWrapper(parent), text, "Hyper-V Machines", MessageBoxButtons.OK, MessageBoxIcon.Information);
        return 0;
    }

    private static List<string> GetHyperVMachines()
    {
        var psi = new ProcessStartInfo
        {
            FileName = GetPreferredPowerShellPath(),
            Arguments = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"$ErrorActionPreference = 'Stop'; Import-Module Hyper-V -ErrorAction Stop; Get-VM -ComputerName localhost -ErrorAction Stop | Select-Object -ExpandProperty Name\"",
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8,
        };

        using var process = Process.Start(psi) ?? throw new InvalidOperationException("Failed to run PowerShell.");
        var output = process.StandardOutput.ReadToEnd();
        var error = process.StandardError.ReadToEnd();
        process.WaitForExit();

        if (process.ExitCode != 0)
        {
            throw new InvalidOperationException("Get-VM failed: " + error);
        }

        if (!string.IsNullOrWhiteSpace(error))
        {
            throw new InvalidOperationException("PowerShell reported an error while querying Hyper-V: " + error);
        }

        var result = new List<string>();
        foreach (var line in output.Split(new[] { "\r\n", "\n" }, StringSplitOptions.RemoveEmptyEntries))
        {
            var trimmed = line.Trim();
            if (!string.IsNullOrEmpty(trimmed))
                result.Add(trimmed);
        }
        return result;
    }


    private static string GetPreferredPowerShellPath()
    {
        var windowsDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Windows);

        var sysnativePath = System.IO.Path.Combine(windowsDirectory, "sysnative", "WindowsPowerShell", "v1.0", "powershell.exe");
        if (System.IO.File.Exists(sysnativePath))
            return sysnativePath;

        var system32Path = System.IO.Path.Combine(windowsDirectory, "System32", "WindowsPowerShell", "v1.0", "powershell.exe");
        if (System.IO.File.Exists(system32Path))
            return system32Path;

        return "powershell.exe";
    }

    private static void EnsureApplicationInitialized()
    {
        if (_visualsEnabled) return;
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        _visualsEnabled = true;
    }

    private static int ShowAbout(IntPtr parent)
    {
        const string description = "Show local Hyper-V virtual machines in panel.";
        const string copyright = "Copyleft 2026 Ondřej Kotas, KRtkovo.eu";
        MessageBox.Show(new WindowHandleWrapper(parent),
            description + Environment.NewLine + copyright,
            "About",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);
        return 0;
    }
    private static int ShowConfiguration(IntPtr parent) { MessageBox.Show(new WindowHandleWrapper(parent), "No configuration yet.", "Configuration", MessageBoxButtons.OK, MessageBoxIcon.Information); return 0; }

    private static IntPtr ParseHandle(string value) => ulong.TryParse(value, out var parsed) ? new IntPtr(unchecked((long)parsed)) : IntPtr.Zero;

    private sealed class WindowHandleWrapper : IWin32Window
    {
        public WindowHandleWrapper(IntPtr handle) => Handle = handle;
        public IntPtr Handle { get; }
    }
}
