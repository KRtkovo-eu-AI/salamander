// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Security.Principal;
using System.Windows.Forms;

namespace OpenSalamander.HyperVM;

internal sealed class VirtualDiskInitializationDialog : Form
{
    private readonly string _path;
    private readonly RadioButton _gpt;
    private readonly RadioButton _mbr;
    private readonly ComboBox _fileSystem;
    private readonly TextBox _label;
    private readonly CheckBox _quickFormat;

    private VirtualDiskInitializationDialog(string path)
    {
        _path = path;
        Text = "Initialize Virtual Hard Disk";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        ClientSize = new Size(520, 330);

        var intro = new Label
        {
            Text = "The virtual hard disk is attached but does not contain initialized partitions yet.",
            AutoSize = false,
            Location = new Point(12, 12),
            Size = new Size(496, 34)
        };
        Controls.Add(intro);

        var diskMap = new DiskMapControl { Location = new Point(12, 54), Size = new Size(496, 70) };
        Controls.Add(diskMap);

        var partitionStyleGroup = new GroupBox { Text = "Partition style", Location = new Point(12, 136), Size = new Size(240, 86) };
        _gpt = new RadioButton { Text = "GPT (recommended)", Location = new Point(12, 24), Size = new Size(200, 20), Checked = true };
        _mbr = new RadioButton { Text = "MBR", Location = new Point(12, 50), Size = new Size(200, 20) };
        partitionStyleGroup.Controls.Add(_gpt);
        partitionStyleGroup.Controls.Add(_mbr);
        Controls.Add(partitionStyleGroup);

        var partitionGroup = new GroupBox { Text = "Primary partition", Location = new Point(268, 136), Size = new Size(240, 116) };
        partitionGroup.Controls.Add(new Label { Text = "Size:", Location = new Point(12, 26), Size = new Size(70, 18) });
        partitionGroup.Controls.Add(new Label { Text = "Use maximum size", Location = new Point(92, 26), Size = new Size(130, 18) });
        partitionGroup.Controls.Add(new Label { Text = "File system:", Location = new Point(12, 54), Size = new Size(70, 18) });
        _fileSystem = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Location = new Point(92, 50), Size = new Size(130, 21) };
        _fileSystem.Items.AddRange(new object[] { "NTFS", "exFAT" });
        _fileSystem.SelectedIndex = 0;
        partitionGroup.Controls.Add(_fileSystem);
        _quickFormat = new CheckBox { Text = "Quick format", Location = new Point(92, 80), Size = new Size(130, 20), Checked = true };
        partitionGroup.Controls.Add(_quickFormat);
        Controls.Add(partitionGroup);

        Controls.Add(new Label { Text = "Volume label:", Location = new Point(12, 236), Size = new Size(100, 18) });
        _label = new TextBox { Location = new Point(116, 232), Size = new Size(136, 21), Text = "New Volume" };
        Controls.Add(_label);

        var initialize = new Button { Text = "Initialize", DialogResult = DialogResult.OK, Location = new Point(324, 292), Size = new Size(88, 26) };
        var cancel = new Button { Text = Texts.Cancel, DialogResult = DialogResult.Cancel, Location = new Point(420, 292), Size = new Size(88, 26) };
        Controls.Add(initialize);
        Controls.Add(cancel);
        AcceptButton = initialize;
        CancelButton = cancel;

        ThemeHelper.ApplyTheme(this);
    }

    public static void ShowForNewDisk(IWin32Window owner, string path)
    {
        using var dialog = new VirtualDiskInitializationDialog(path);
        if (dialog.ShowDialog(owner) != DialogResult.OK) return;
        RunElevatedPowerShell(dialog.BuildScript());
    }

    private string BuildScript()
    {
        var partitionStyle = _gpt.Checked ? "GPT" : "MBR";
        var fileSystem = _fileSystem.SelectedItem?.ToString() == "exFAT" ? "exFAT" : "NTFS";
        var full = _quickFormat.Checked ? string.Empty : " -Full";
        var label = _label.Text.Trim();
        if (label.Length == 0) label = "New Volume";

        return "$ErrorActionPreference='Stop'; " +
            $"$image = Get-DiskImage -ImagePath {Quote(_path)}; " +
            "if (-not $image.Attached) { Mount-DiskImage -ImagePath $image.ImagePath | Out-Null; $image = Get-DiskImage -ImagePath $image.ImagePath }; " +
            "$disk = $image | Get-Disk; " +
            $"if ($disk.PartitionStyle -eq 'RAW') {{ Initialize-Disk -Number $disk.Number -PartitionStyle {partitionStyle} -ErrorAction Stop }}; " +
            "$partition = New-Partition -DiskNumber $disk.Number -UseMaximumSize -AssignDriveLetter -ErrorAction Stop; " +
            $"$partition | Format-Volume -FileSystem {fileSystem} -NewFileSystemLabel {Quote(label)} -Confirm:$false{full} -ErrorAction Stop";
    }

    private static void RunElevatedPowerShell(string script)
    {
        var psi = new ProcessStartInfo
        {
            FileName = GetPreferredPowerShellPath(),
            Arguments = "-NoProfile -ExecutionPolicy Bypass -Command \"" + script.Replace("\"", "`\"") + "\"",
            UseShellExecute = true
        };
        if (!IsAdministrator()) psi.Verb = "runas";
        using var process = Process.Start(psi) ?? throw new InvalidOperationException("Failed to start elevated PowerShell.");
        process.WaitForExit();
        if (process.ExitCode != 0) throw new InvalidOperationException("Disk initialization failed or was canceled.");
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

    private sealed class DiskMapControl : Control
    {
        public DiskMapControl()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw | ControlStyles.UserPaint, true);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            using var border = new Pen(ForeColor);
            using var background = new SolidBrush(BackColor);
            using var text = new SolidBrush(ForeColor);
            using var unallocated = new SolidBrush(Color.Black);
            using var primary = new SolidBrush(Color.Navy);
            e.Graphics.FillRectangle(background, ClientRectangle);
            e.Graphics.DrawRectangle(border, 0, 0, Width - 1, Height - 1);
            e.Graphics.FillRectangle(unallocated, 10, 10, Width - 20, 18);
            e.Graphics.DrawString("Unallocated", Font, text, 10, 34);
            e.Graphics.FillRectangle(primary, 110, 48, 12, 12);
            e.Graphics.DrawString("Primary partition will use maximum size", Font, text, 128, 46);
        }
    }
}
