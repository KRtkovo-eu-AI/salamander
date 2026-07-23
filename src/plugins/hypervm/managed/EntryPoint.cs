// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.Management;
using System.Windows.Forms;

namespace OpenSalamander.HyperVM;

public static class EntryPoint
{
    private static bool _visualsEnabled;

    [STAThread]
    public static int Dispatch(string? argument)
    {
        var parentHandle = IntPtr.Zero;

        try
        {
            EnsureApplicationInitialized();

            var parts = (argument ?? string.Empty).Split(new[] { ';' }, 3);
            var command = parts.Length > 0 ? parts[0] : string.Empty;
            parentHandle = ParseHandle(parts.Length > 1 ? parts[1] : string.Empty);
            var payload = parts.Length > 2 ? parts[2] : string.Empty;

            return command switch
            {
                "About" => ShowAbout(parentHandle),
                "Configure" => ShowConfiguration(parentHandle),
                "Menu" => ExecuteMenu(parentHandle, payload),
                "ColorsChanged" => ColorsChanged(),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            var owner = parentHandle == IntPtr.Zero ? null : new WindowHandleWrapper(parentHandle);
            ThemeHelper.ShowMessageBox(owner, ex.Message, Texts.PluginName, MessageBoxButtons.OK, MessageBoxIcon.Error);
            return -1;
        }
    }

    private static int ExecuteMenu(IntPtr parent, string command)
    {
        return command switch
        {
            "ShowMachines" => ShowMachines(parent),
            var s when s.StartsWith("CreateVhd|", StringComparison.Ordinal) => CreateVhd(parent, s.Substring(10)),
            var s when s.StartsWith("AttachVhd|", StringComparison.Ordinal) => AttachVhd(parent, s.Substring(10)),
            var s when s.StartsWith("DetachVhd|", StringComparison.Ordinal) => DetachVhd(parent, s.Substring(10)),
            _ => 1,
        };
    }

    private static int CreateVhd(IntPtr parent, string payload)
    {
        var parts = payload.Split(new[] { '|' }, 4);
        if (parts.Length != 4 || !ulong.TryParse(parts[1], out var sizeBytes)) return 1;
        VirtualDiskManager.CreateVhd(parts[0], sizeBytes, parts[2], parts[3] == "Fixed");
        VirtualDiskManager.AttachVhd(parts[0], false);
        VirtualDiskInitializationDialog.ShowForNewDisk(new WindowHandleWrapper(parent), parts[0]);
        return 0;
    }

    private static int AttachVhd(IntPtr parent, string payload)
    {
        var parts = payload.Split(new[] { '|' }, 2);
        if (parts.Length != 2) return 1;
        VirtualDiskManager.AttachVhd(parts[0], parts[1] == "1");
        ThemeHelper.ShowMessageBox(new WindowHandleWrapper(parent), string.Format(Texts.Attached, parts[0]), Texts.PluginName, MessageBoxButtons.OK, MessageBoxIcon.Information);
        return 0;
    }

    private static int DetachVhd(IntPtr parent, string path)
    {
        if (ThemeHelper.ShowMessageBox(new WindowHandleWrapper(parent), string.Format(Texts.DetachQuestion, path), Texts.DetachTitle, MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK) return 0;
        VirtualDiskManager.DetachVhd(path);
        return 0;
    }

    private static int ShowMachines(IntPtr parent)
    {
        var machines = GetHyperVMachines();
        var text = machines.Count == 0 ? Texts.NoMachines : string.Join(Environment.NewLine, machines);
        ThemeHelper.ShowMessageBox(new WindowHandleWrapper(parent), text, Texts.PluginName, MessageBoxButtons.OK, MessageBoxIcon.Information);
        return 0;
    }

    private static List<string> GetHyperVMachines()
    {
        using var searcher = new ManagementObjectSearcher(@"root\virtualization\v2", "SELECT ElementName FROM Msvm_ComputerSystem WHERE Caption = 'Virtual Machine'");
        var result = new List<string>();
        foreach (ManagementObject vm in searcher.Get())
        {
            var name = Convert.ToString(vm["ElementName"]);
            if (!string.IsNullOrWhiteSpace(name)) result.Add(name!);
        }
        return result;
    }

    private static int ShowAbout(IntPtr parent)
    {
        ThemeHelper.ShowMessageBox(new WindowHandleWrapper(parent), Texts.AboutText, Texts.AboutTitle, MessageBoxButtons.OK, MessageBoxIcon.Information);
        return 0;
    }
    private static int ShowConfiguration(IntPtr parent)
    {
        ThemeHelper.ShowMessageBox(new WindowHandleWrapper(parent), Texts.NoConfiguration, Texts.ConfigurationTitle, MessageBoxButtons.OK, MessageBoxIcon.Information);
        return 0;
    }

    private static int ColorsChanged()
    {
        ThemeHelper.InvalidatePalette();
        return 0;
    }

    private static void EnsureApplicationInitialized()
    {
        if (_visualsEnabled) return;
        ManagedApplication.Initialize();
        _visualsEnabled = true;
    }

    private static IntPtr ParseHandle(string value) => ulong.TryParse(value, out var parsed) ? new IntPtr(unchecked((long)parsed)) : IntPtr.Zero;

    private sealed class WindowHandleWrapper : IWin32Window
    {
        public WindowHandleWrapper(IntPtr handle) => Handle = handle;
        public IntPtr Handle { get; }
    }
}
