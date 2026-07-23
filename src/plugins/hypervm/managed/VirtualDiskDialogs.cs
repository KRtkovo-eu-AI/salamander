// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.IO;

namespace OpenSalamander.HyperVM;

internal static class Texts
{
    private const int IDS_PLUGINNAME = 46;
    private const int IDS_VHD_CREATE_TITLE = 60;
    private const int IDS_VHD_ATTACH_TITLE = 61;
    private const int IDS_VHD_DETACH_TITLE = 62;
    private const int IDS_VHD_LOCATION = 63;
    private const int IDS_VHD_BROWSE = 64;
    private const int IDS_VHD_SIZE = 65;
    private const int IDS_VHD_FORMAT = 66;
    private const int IDS_VHD_TYPE = 67;
    private const int IDS_VHD_VHD_HELP = 68;
    private const int IDS_VHD_VHDX_HELP = 69;
    private const int IDS_VHD_FIXED = 70;
    private const int IDS_VHD_DYNAMIC = 71;
    private const int IDS_VHD_FIXED_HELP = 72;
    private const int IDS_VHD_DYNAMIC_HELP = 73;
    private const int IDS_VHD_READONLY = 74;
    private const int IDS_VHD_CREATE_INTRO = 75;
    private const int IDS_VHD_ATTACH_INTRO = 76;
    private const int IDS_VHD_DETACH_QUESTION = 77;
    private const int IDS_VHD_ATTACHED = 78;
    private const int IDS_VHD_NO_MACHINES = 79;
    private const int IDS_CONFIG_TITLE = 80;
    private const int IDS_NO_CONFIGURATION = 81;
    private const int IDS_OK = 82;
    private const int IDS_CANCEL = 83;
    private const int IDS_VHD_FILTER = 84;

    public static string PluginName => Load(IDS_PLUGINNAME);
    public static string CreateTitle => Load(IDS_VHD_CREATE_TITLE);
    public static string AttachTitle => Load(IDS_VHD_ATTACH_TITLE);
    public static string DetachTitle => Load(IDS_VHD_DETACH_TITLE);
    public static string Location => Load(IDS_VHD_LOCATION);
    public static string Browse => Load(IDS_VHD_BROWSE);
    public static string Size => Load(IDS_VHD_SIZE);
    public static string Format => Load(IDS_VHD_FORMAT);
    public static string Type => Load(IDS_VHD_TYPE);
    public static string VhdHelp => Load(IDS_VHD_VHD_HELP);
    public static string VhdxHelp => Load(IDS_VHD_VHDX_HELP);
    public static string Fixed => Load(IDS_VHD_FIXED);
    public static string Dynamic => Load(IDS_VHD_DYNAMIC);
    public static string FixedHelp => Load(IDS_VHD_FIXED_HELP);
    public static string DynamicHelp => Load(IDS_VHD_DYNAMIC_HELP);
    public static string ReadOnly => Load(IDS_VHD_READONLY);
    public static string CreateIntro => Load(IDS_VHD_CREATE_INTRO);
    public static string AttachIntro => Load(IDS_VHD_ATTACH_INTRO);
    public static string DetachQuestion => Load(IDS_VHD_DETACH_QUESTION);
    public static string Attached => Load(IDS_VHD_ATTACHED);
    public static string NoMachines => Load(IDS_VHD_NO_MACHINES);
    public static string AboutTitle => Load(47);
    public static string ConfigurationTitle => Load(IDS_CONFIG_TITLE);
    public static string AboutText => Load(48);
    public static string NoConfiguration => Load(IDS_NO_CONFIGURATION);
    public static string OK => Load(IDS_OK);
    public static string Cancel => Load(IDS_CANCEL);
    public static string VhdFilter => Load(IDS_VHD_FILTER);

    private static string Load(int id)
    {
        var buffer = new System.Text.StringBuilder(1024);
        return NativeMethods.HyperVM_LoadString(id, buffer, buffer.Capacity) > 0 ? buffer.ToString() : string.Empty;
    }

    private static class NativeMethods
    {
        [System.Runtime.InteropServices.DllImport("HyperVM.Spl", CallingConvention = System.Runtime.InteropServices.CallingConvention.StdCall, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int HyperVM_LoadString(int stringId, System.Text.StringBuilder buffer, int bufferLength);
    }
}


internal static class VirtualDiskManager
{
    public static void AttachVhd(string path, bool readOnly)
    {
        var access = readOnly ? "ReadOnly" : "ReadWrite";
        RunPowerShell($"Mount-DiskImage -ImagePath {Quote(path)} -Access {access} -ErrorAction Stop");
    }

    public static void DetachVhd(string path) => RunPowerShell($"Dismount-DiskImage -ImagePath {Quote(path)} -ErrorAction Stop");
    public static void CreateVhd(string path, ulong sizeBytes, string format, bool fixedSize)
    {
        var type = fixedSize ? "Fixed" : "Dynamic";
        RunPowerShell($"New-VHD -Path {Quote(path)} -SizeBytes {sizeBytes} -{type} -ErrorAction Stop");
    }
    private static void RunPowerShell(string command)
    {
        var psi = new ProcessStartInfo { FileName = GetPreferredPowerShellPath(), Arguments = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"$ErrorActionPreference='Stop'; " + command.Replace("\"", "`\"") + "\"", UseShellExecute = false, CreateNoWindow = true, RedirectStandardError = true };
        using var p = Process.Start(psi) ?? throw new InvalidOperationException("Failed to run PowerShell."); var error = p.StandardError.ReadToEnd(); p.WaitForExit(); if (p.ExitCode != 0) throw new InvalidOperationException(error);
    }
    private static string Quote(string s) => "'" + s.Replace("'", "''") + "'";
    private static string GetPreferredPowerShellPath()
    {
        var windowsDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Windows);
        var sysnativePath = Path.Combine(windowsDirectory, "sysnative", "WindowsPowerShell", "v1.0", "powershell.exe");
        if (File.Exists(sysnativePath)) return sysnativePath;
        var system32Path = Path.Combine(windowsDirectory, "System32", "WindowsPowerShell", "v1.0", "powershell.exe");
        return File.Exists(system32Path) ? system32Path : "powershell.exe";
    }
}
