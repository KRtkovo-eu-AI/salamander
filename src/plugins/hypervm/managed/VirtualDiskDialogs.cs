// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.IO;
using System.Security.Principal;

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
    private const int ErrorPrivilegeNotHeld = 1314;
    private const uint OpenVirtualDiskRwDepthDefault = 1;

    private static readonly Guid VirtualStorageTypeVendorMicrosoft = new("EC984AEC-A0F9-47E9-901F-71415A66345B");

    public static void AttachVhd(string path, bool readOnly)
    {
        var storageType = GetStorageType(path, null);
        var accessMask = readOnly ? VirtualDiskAccessMask.AttachReadOnly : VirtualDiskAccessMask.AttachReadWrite;
        var openParameters = new OpenVirtualDiskParameters { Version = OpenVirtualDiskVersion.Version1, RWDepth = OpenVirtualDiskRwDepthDefault };
        var result = NativeMethods.OpenVirtualDisk(ref storageType, path, accessMask, OpenVirtualDiskFlags.None, ref openParameters, out var handle);
        ThrowIfFailed(result, "open", path);

        using (handle)
        {
            var attachParameters = new AttachVirtualDiskParameters { Version = AttachVirtualDiskVersion.Version1 };
            result = NativeMethods.AttachVirtualDisk(handle, IntPtr.Zero, AttachVirtualDiskFlags.PermanentLifetime, 0, ref attachParameters, IntPtr.Zero);
        }

        if (result == ErrorPrivilegeNotHeld && !IsAdministrator())
        {
            RunElevatedPowerShell($"Mount-DiskImage -ImagePath {Quote(path)} -Access {(readOnly ? "ReadOnly" : "ReadWrite")} -ErrorAction Stop");
            return;
        }

        ThrowIfFailed(result, "attach", path);
    }

    public static void DetachVhd(string path)
    {
        var storageType = GetStorageType(path, null);
        var openParameters = new OpenVirtualDiskParameters { Version = OpenVirtualDiskVersion.Version1, RWDepth = OpenVirtualDiskRwDepthDefault };
        var result = NativeMethods.OpenVirtualDisk(ref storageType, path, VirtualDiskAccessMask.Detach, OpenVirtualDiskFlags.None, ref openParameters, out var handle);
        ThrowIfFailed(result, "open", path);

        using (handle)
        {
            result = NativeMethods.DetachVirtualDisk(handle, DetachVirtualDiskFlags.None, 0);
            ThrowIfFailed(result, "detach", path);
        }
    }

    public static void CreateVhd(string path, ulong sizeBytes, string format, bool fixedSize)
    {
        var storageType = GetStorageType(path, format);
        var parameters = new CreateVirtualDiskParameters
        {
            Version = CreateVirtualDiskVersion.Version2,
            Version2 = new CreateVirtualDiskParametersVersion2
            {
                UniqueId = Guid.NewGuid(),
                MaximumSize = sizeBytes,
                BlockSizeInBytes = 0,
                SectorSizeInBytes = 0,
                ParentPath = null,
                SourcePath = null,
                OpenFlags = OpenVirtualDiskFlags.None,
                ParentVirtualStorageType = new VirtualStorageType(),
                SourceVirtualStorageType = new VirtualStorageType(),
                ResiliencyGuid = Guid.Empty
            }
        };
        var flags = fixedSize ? CreateVirtualDiskFlags.FullPhysicalAllocation : CreateVirtualDiskFlags.None;
        var result = NativeMethods.CreateVirtualDisk(ref storageType, path, VirtualDiskAccessMask.None, IntPtr.Zero, flags, 0, ref parameters, IntPtr.Zero, out var handle);
        ThrowIfFailed(result, "create", path);
        handle.Dispose();
    }

    private static VirtualStorageType GetStorageType(string path, string? format)
    {
        var normalizedFormat = !string.IsNullOrWhiteSpace(format) ? format! : (Path.GetExtension(path) ?? string.Empty).TrimStart('.');
        var deviceId = string.Equals(normalizedFormat, "vhd", StringComparison.OrdinalIgnoreCase) ? VirtualStorageTypeDevice.Vhd : VirtualStorageTypeDevice.Vhdx;
        return new VirtualStorageType { DeviceId = deviceId, VendorId = VirtualStorageTypeVendorMicrosoft };
    }

    private static void RunElevatedPowerShell(string command)
    {
        var script = "$ErrorActionPreference='Stop'; " + command;
        var psi = new ProcessStartInfo
        {
            FileName = GetPreferredPowerShellPath(),
            Arguments = "-NoProfile -ExecutionPolicy Bypass -Command \"" + script.Replace("\"", "`\"") + "\"",
            UseShellExecute = true,
            Verb = "runas"
        };
        using var process = Process.Start(psi) ?? throw new InvalidOperationException("Failed to start elevated PowerShell.");
        process.WaitForExit();
        if (process.ExitCode != 0) throw new InvalidOperationException("Elevated virtual hard disk operation failed or was canceled.");
    }

    private static bool IsAdministrator()
    {
        using var identity = WindowsIdentity.GetCurrent();
        return new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator);
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

    private static void ThrowIfFailed(int error, string operation, string path)
    {
        if (error == 0) return;
        throw new InvalidOperationException($"Failed to {operation} virtual hard disk '{path}'. {new System.ComponentModel.Win32Exception(error).Message} (0x{error:X8})");
    }

    private enum VirtualStorageTypeDevice : uint
    {
        Vhd = 2,
        Vhdx = 3
    }

    [Flags]
    private enum VirtualDiskAccessMask : uint
    {
        None = 0,
        AttachReadOnly = 0x00010000,
        AttachReadWrite = 0x00020000,
        Detach = 0x00040000
    }

    [Flags]
    private enum CreateVirtualDiskFlags : uint
    {
        None = 0,
        FullPhysicalAllocation = 1
    }

    [Flags]
    private enum OpenVirtualDiskFlags : uint
    {
        None = 0
    }

    [Flags]
    private enum AttachVirtualDiskFlags : uint
    {
        PermanentLifetime = 0x00000004
    }

    [Flags]
    private enum DetachVirtualDiskFlags : uint
    {
        None = 0
    }

    private enum CreateVirtualDiskVersion
    {
        Version2 = 2
    }

    private enum OpenVirtualDiskVersion
    {
        Version1 = 1
    }

    private enum AttachVirtualDiskVersion
    {
        Version1 = 1
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private struct VirtualStorageType
    {
        public VirtualStorageTypeDevice DeviceId;
        public Guid VendorId;
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private struct CreateVirtualDiskParameters
    {
        public CreateVirtualDiskVersion Version;
        public CreateVirtualDiskParametersVersion2 Version2;
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    private struct CreateVirtualDiskParametersVersion2
    {
        public Guid UniqueId;
        public ulong MaximumSize;
        public uint BlockSizeInBytes;
        public uint SectorSizeInBytes;
        [System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)] public string? ParentPath;
        [System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)] public string? SourcePath;
        public OpenVirtualDiskFlags OpenFlags;
        public VirtualStorageType ParentVirtualStorageType;
        public VirtualStorageType SourceVirtualStorageType;
        public Guid ResiliencyGuid;
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private struct OpenVirtualDiskParameters
    {
        public OpenVirtualDiskVersion Version;
        public uint RWDepth;
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private struct AttachVirtualDiskParameters
    {
        public AttachVirtualDiskVersion Version;
        public uint Reserved;
    }

    private static class NativeMethods
    {
        [System.Runtime.InteropServices.DllImport("virtdisk.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int CreateVirtualDisk(ref VirtualStorageType virtualStorageType, string path, VirtualDiskAccessMask virtualDiskAccessMask, IntPtr securityDescriptor, CreateVirtualDiskFlags flags, uint providerSpecificFlags, ref CreateVirtualDiskParameters parameters, IntPtr overlapped, out Microsoft.Win32.SafeHandles.SafeFileHandle handle);

        [System.Runtime.InteropServices.DllImport("virtdisk.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int OpenVirtualDisk(ref VirtualStorageType virtualStorageType, string path, VirtualDiskAccessMask virtualDiskAccessMask, OpenVirtualDiskFlags flags, ref OpenVirtualDiskParameters parameters, out Microsoft.Win32.SafeHandles.SafeFileHandle handle);

        [System.Runtime.InteropServices.DllImport("virtdisk.dll")]
        public static extern int AttachVirtualDisk(Microsoft.Win32.SafeHandles.SafeFileHandle virtualDiskHandle, IntPtr securityDescriptor, AttachVirtualDiskFlags flags, uint providerSpecificFlags, ref AttachVirtualDiskParameters parameters, IntPtr overlapped);

        [System.Runtime.InteropServices.DllImport("virtdisk.dll")]
        public static extern int DetachVirtualDisk(Microsoft.Win32.SafeHandles.SafeFileHandle virtualDiskHandle, DetachVirtualDiskFlags flags, uint providerSpecificFlags);
    }
}
