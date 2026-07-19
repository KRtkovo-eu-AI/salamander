// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Management;
using System.Windows.Forms;

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


internal sealed class CreateVhdDialog : Form
{
    private readonly TextBox _path = new() { Width = 275 };
    private readonly NumericUpDown _size = new() { Minimum = 1, Maximum = 1024 * 1024, Value = 64, Width = 74 };
    private readonly ComboBox _unit = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 54 };
    private readonly RadioButton _vhd = new() { Text = "VHD", Checked = true, AutoSize = true };
    private readonly RadioButton _vhdx = new() { Text = "VHDX", AutoSize = true };
    private readonly RadioButton _fixed = new() { Text = Texts.Fixed, Checked = true, AutoSize = true };
    private readonly RadioButton _dynamic = new() { Text = Texts.Dynamic, AutoSize = true };
    private readonly Button _ok = new() { Text = Texts.OK, DialogResult = DialogResult.OK, Enabled = false };
    public CreateVhdDialog()
    {
        Text = Texts.CreateTitle; FormBorderStyle = FormBorderStyle.FixedDialog; StartPosition = FormStartPosition.CenterParent; MaximizeBox = MinimizeBox = false; ClientSize = new Size(380, 470);
        _unit.Items.AddRange(new object[] { "MB", "GB", "TB" }); _unit.SelectedIndex = 0; _path.TextChanged += (_, _) => _ok.Enabled = !string.IsNullOrWhiteSpace(_path.Text);
        var browse = new Button { Text = Texts.Browse, Width = 74 }; browse.Click += (_, _) => { using var s = new SaveFileDialog { Filter = Texts.VhdFilter, DefaultExt = _vhd.Checked ? "vhd" : "vhdx" }; if (s.ShowDialog(this) == DialogResult.OK) _path.Text = s.FileName; };
        var cancel = new Button { Text = Texts.Cancel, DialogResult = DialogResult.Cancel };
        Controls.AddRange(new Control[] { L(Texts.CreateIntro,12,12,350), L(Texts.Location,12,74,100), _path, browse, L(Texts.Size,12,138,170), _size, _unit, Group(Texts.Format,12,164,356,134,_vhd,L(Texts.VhdHelp,32,34,315),_vhdx,L(Texts.VhdxHelp,32,78,315)), Group(Texts.Type,12,306,356,118,_fixed,L(Texts.FixedHelp,32,34,315),_dynamic,L(Texts.DynamicHelp,32,78,315)), _ok, cancel });
        _path.Location = new Point(12,93); browse.Location = new Point(295,91); _size.Location = new Point(236,131); _unit.Location = new Point(315,130); _ok.SetBounds(215,438,72,24); cancel.SetBounds(295,438,72,24); AcceptButton = _ok; CancelButton = cancel; ThemeHelper.ApplyTheme(this);
    }
    public string VhdPath => _path.Text; public string Format => _vhd.Checked ? "VHD" : "VHDX"; public bool IsFixed => _fixed.Checked; public bool AttachAfterCreate => true; public ulong SizeBytes => (ulong)_size.Value * (_unit.Text == "TB" ? 1024UL*1024*1024*1024 : _unit.Text == "GB" ? 1024UL*1024*1024 : 1024UL*1024);
    private static Label L(string t,int x,int y,int w)=>new(){Text=t,Location=new Point(x,y),MaximumSize=new Size(w,0),AutoSize=true};
    private static GroupBox Group(string text,int x,int y,int w,int h,params Control[] c){var g=new GroupBox{Text=text,Location=new Point(x,y),Size=new Size(w,h)}; foreach(var cc in c) g.Controls.Add(cc); c[0].Location=new Point(8,18); c[2].Location=new Point(8,62); return g;}
}

internal sealed class AttachVhdDialog : Form
{
    private readonly TextBox _path = new() { Width = 275 };
    private readonly CheckBox _ro = new() { Text = Texts.ReadOnly, AutoSize = true };
    private readonly Button _ok = new() { Text = Texts.OK, DialogResult = DialogResult.OK, Enabled = false };
    public AttachVhdDialog(){Text=Texts.AttachTitle; FormBorderStyle=FormBorderStyle.FixedDialog; StartPosition=FormStartPosition.CenterParent; MaximizeBox=MinimizeBox=false; ClientSize=new Size(380,148); _path.TextChanged+=(_,_)=>_ok.Enabled=!string.IsNullOrWhiteSpace(_path.Text); var browse=new Button{Text=Texts.Browse,Width=74}; browse.Click+=(_,_)=>{using var o=new OpenFileDialog{Filter=Texts.VhdFilter}; if(o.ShowDialog(this)==DialogResult.OK)_path.Text=o.FileName;}; var cancel=new Button{Text=Texts.Cancel,DialogResult=DialogResult.Cancel}; Controls.AddRange(new Control[]{new Label{Text=Texts.AttachIntro,Location=new Point(12,12),AutoSize=true},new Label{Text=Texts.Location,Location=new Point(12,74),AutoSize=true},_path,browse,_ro,_ok,cancel}); _path.Location=new Point(12,94); browse.Location=new Point(295,92); _ro.Location=new Point(12,121); _ok.SetBounds(215,116,72,24); cancel.SetBounds(295,116,72,24); AcceptButton=_ok; CancelButton=cancel; ThemeHelper.ApplyTheme(this);}
    public string VhdPath=>_path.Text; public bool ReadOnly=>_ro.Checked;
}

internal static class VirtualDiskManager
{
    public static void AttachVhd(string path, bool readOnly) => InvokeDiskImage(path, "Attach", readOnly);
    public static void DetachVhd(string path) => InvokeDiskImage(path, "Detach", false);
    private static void InvokeDiskImage(string path, string method, bool readOnly)
    {
        using var image = new ManagementObject(@"root\Microsoft\Windows\Storage", $"MSFT_DiskImage.ImagePath='{path.Replace("\\", "\\\\").Replace("'", "\\'")}'", null);
        var inParams = image.GetMethodParameters(method);
        if (method == "Attach") inParams["Access"] = readOnly ? 1U : 0U;
        image.InvokeMethod(method, inParams, null);
    }
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
