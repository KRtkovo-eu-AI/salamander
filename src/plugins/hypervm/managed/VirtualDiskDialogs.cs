// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Management;
using System.Windows.Forms;

namespace OpenSalamander.HyperVM;

internal static class Texts
{
    private static bool Cs => CultureInfo.CurrentUICulture.TwoLetterISOLanguageName is "cs" or "sk";
    public static string PluginName => Cs ? "Stroje Hyper-V" : "Hyper-V Machines";
    public static string CreateMenu => Cs ? "Vytvořit VHD" : "Create VHD";
    public static string AttachMenu => Cs ? "Připojit VHD" : "Attach VHD";
    public static string DetachMenu => Cs ? "Odpojit VHD" : "Detach VHD";
    public static string CreateTitle => Cs ? "Vytvořit a připojit virtuální pevný disk" : "Create and Attach Virtual Hard Disk";
    public static string AttachTitle => Cs ? "Připojit virtuální pevný disk" : "Attach Virtual Hard Disk";
    public static string DetachTitle => Cs ? "Odpojit virtuální pevný disk" : "Detach Virtual Hard Disk";
    public static string Location => Cs ? "Umístění:" : "Location:";
    public static string Browse => Cs ? "Procházet..." : "Browse...";
    public static string Size => Cs ? "Velikost virtuálního disku:" : "Virtual hard disk size:";
    public static string Format => Cs ? "Formát virtuálního disku" : "Virtual hard disk format";
    public static string Type => Cs ? "Typ virtuálního disku" : "Virtual hard disk type";
    public static string VhdHelp => Cs ? "Podporuje virtuální disky do velikosti 2040 GB." : "Supports virtual disks up to 2040 GB in size.";
    public static string VhdxHelp => Cs ? "Podporuje virtuální disky větší než 2040 GB a je odolnější proti výpadkům napájení." : "Supports virtual disks larger than 2040 GB and is resilient to power failure events.";
    public static string Fixed => Cs ? "Pevná velikost (doporučeno)" : "Fixed size (Recommended)";
    public static string Dynamic => Cs ? "Dynamicky se zvětšující" : "Dynamically expanding";
    public static string FixedHelp => Cs ? "Soubor disku se při vytvoření alokuje na maximální velikost." : "The virtual hard disk file is allocated to its maximum size when created.";
    public static string DynamicHelp => Cs ? "Soubor disku roste podle zapisovaných dat." : "The virtual hard disk file grows as data is written to it.";
    public static string ReadOnly => Cs ? "Jen pro čtení." : "Read-only.";
    public static string CreateIntro => Cs ? "Zadejte umístění virtuálního pevného disku v počítači." : "Specify the virtual hard disk location on the machine.";
    public static string AttachIntro => Cs ? "Zadejte umístění virtuálního pevného disku v počítači." : "Specify the virtual hard disk location on the computer.";
    public static string DetachQuestion => Cs ? "Odpojit virtuální disk {0}?" : "Detach virtual hard disk {0}?";
    public static string Attached => Cs ? "Virtuální disk byl připojen: {0}" : "Virtual hard disk attached: {0}";
    public static string NoMachines => Cs ? "Nebyly nalezeny žádné virtuální počítače Hyper-V." : "No Hyper-V virtual machines found.";
    public static string AboutTitle => Cs ? "O pluginu" : "About";
    public static string ConfigurationTitle => Cs ? "Konfigurace" : "Configuration";
    public static string AboutText => Cs ? "Zobrazí lokální virtuální počítače Hyper-V v panelu." : "Show local Hyper-V virtual machines in panel.";
    public static string NoConfiguration => Cs ? "Zatím žádná konfigurace." : "No configuration yet.";
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
    private readonly Button _ok = new() { Text = "OK", DialogResult = DialogResult.OK, Enabled = false };
    public CreateVhdDialog()
    {
        Text = Texts.CreateTitle; FormBorderStyle = FormBorderStyle.FixedDialog; StartPosition = FormStartPosition.CenterParent; MaximizeBox = MinimizeBox = false; ClientSize = new Size(380, 470);
        _unit.Items.AddRange(new object[] { "MB", "GB", "TB" }); _unit.SelectedIndex = 0; _path.TextChanged += (_, _) => _ok.Enabled = !string.IsNullOrWhiteSpace(_path.Text);
        var browse = new Button { Text = Texts.Browse, Width = 74 }; browse.Click += (_, _) => { using var s = new SaveFileDialog { Filter = "Virtual hard disks (*.vhd;*.vhdx)|*.vhd;*.vhdx", DefaultExt = _vhd.Checked ? "vhd" : "vhdx" }; if (s.ShowDialog(this) == DialogResult.OK) _path.Text = s.FileName; };
        var cancel = new Button { Text = "Cancel", DialogResult = DialogResult.Cancel };
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
    private readonly Button _ok = new() { Text = "OK", DialogResult = DialogResult.OK, Enabled = false };
    public AttachVhdDialog(){Text=Texts.AttachTitle; FormBorderStyle=FormBorderStyle.FixedDialog; StartPosition=FormStartPosition.CenterParent; MaximizeBox=MinimizeBox=false; ClientSize=new Size(380,185); _path.TextChanged+=(_,_)=>_ok.Enabled=!string.IsNullOrWhiteSpace(_path.Text); var browse=new Button{Text=Texts.Browse,Width=74}; browse.Click+=(_,_)=>{using var o=new OpenFileDialog{Filter="Virtual hard disks (*.vhd;*.vhdx)|*.vhd;*.vhdx"}; if(o.ShowDialog(this)==DialogResult.OK)_path.Text=o.FileName;}; var cancel=new Button{Text="Cancel",DialogResult=DialogResult.Cancel}; Controls.AddRange(new Control[]{new Label{Text=Texts.AttachIntro,Location=new Point(12,12),AutoSize=true},new Label{Text=Texts.Location,Location=new Point(12,74),AutoSize=true},_path,browse,_ro,_ok,cancel}); _path.Location=new Point(12,94); browse.Location=new Point(295,92); _ro.Location=new Point(12,121); _ok.SetBounds(215,153,72,24); cancel.SetBounds(295,153,72,24); AcceptButton=_ok; CancelButton=cancel; ThemeHelper.ApplyTheme(this);} 
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
