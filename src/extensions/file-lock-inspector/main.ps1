Set-StrictMode -Version 2.0

function Initialize-ExtensionDarkMode {
    if ($null -ne ('OpenSalamander.Extensions.DarkModeNativeMethods' -as [type])) {
        return
    }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace OpenSalamander.Extensions
{
    public static class DarkModeNativeMethods
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int SetPreferredAppModeDelegate(int appMode);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private delegate bool AllowDarkModeForAppDelegate(
            [MarshalAs(UnmanagedType.Bool)] bool allow);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private delegate bool AllowDarkModeForWindowDelegate(
            IntPtr hwnd, [MarshalAs(UnmanagedType.Bool)] bool allow);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate void RefreshImmersiveColorPolicyStateDelegate();

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct RtlOsVersionInfo
        {
            internal uint Size;
            internal uint Major;
            internal uint Minor;
            internal uint Build;
            internal uint Platform;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            internal string ServicePack;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr LoadLibrary(string fileName);

        [DllImport("kernel32.dll", ExactSpelling = true)]
        private static extern IntPtr GetProcAddress(
            IntPtr module, IntPtr ordinal);

        [DllImport("ntdll.dll", CharSet = CharSet.Unicode)]
        private static extern int RtlGetVersion(ref RtlOsVersionInfo version);

        [DllImport("dwmapi.dll")]
        public static extern int DwmSetWindowAttribute(
            IntPtr hwnd, int attribute, ref int value, int valueSize);

        [DllImport("uxtheme.dll", CharSet = CharSet.Unicode)]
        public static extern int SetWindowTheme(
            IntPtr hwnd, string subAppName, string subIdList);

        [DllImport("user32.dll")]
        public static extern IntPtr SendMessage(
            IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);

        private static int GetWindowsBuild()
        {
            RtlOsVersionInfo version = new RtlOsVersionInfo();
            version.Size = (uint)Marshal.SizeOf(typeof(RtlOsVersionInfo));
            return RtlGetVersion(ref version) == 0 ? (int)version.Build : 0;
        }

        public static void EnableImmersiveDarkMode()
        {
            int build = GetWindowsBuild();
            if (build < 17763)
                return;
            IntPtr module = LoadLibrary("uxtheme.dll");
            if (module == IntPtr.Zero)
                return;

            IntPtr preferred = GetProcAddress(module, new IntPtr(135));
            if (preferred != IntPtr.Zero)
            {
                if (build >= 18362)
                {
                    SetPreferredAppModeDelegate call =
                        (SetPreferredAppModeDelegate)Marshal.GetDelegateForFunctionPointer(
                            preferred, typeof(SetPreferredAppModeDelegate));
                    call(1); // PreferredAppMode.AllowDark
                }
                else
                {
                    AllowDarkModeForAppDelegate call =
                        (AllowDarkModeForAppDelegate)Marshal.GetDelegateForFunctionPointer(
                            preferred, typeof(AllowDarkModeForAppDelegate));
                    call(true);
                }
            }

            IntPtr refresh = GetProcAddress(module, new IntPtr(104));
            if (refresh != IntPtr.Zero)
            {
                RefreshImmersiveColorPolicyStateDelegate call =
                    (RefreshImmersiveColorPolicyStateDelegate)Marshal.GetDelegateForFunctionPointer(
                        refresh, typeof(RefreshImmersiveColorPolicyStateDelegate));
                call();
            }
        }

        public static void AllowImmersiveDarkMode(IntPtr hwnd)
        {
            if (hwnd == IntPtr.Zero || GetWindowsBuild() < 17763)
                return;
            IntPtr module = LoadLibrary("uxtheme.dll");
            IntPtr proc = module != IntPtr.Zero
                ? GetProcAddress(module, new IntPtr(133))
                : IntPtr.Zero;
            if (proc == IntPtr.Zero)
                return;
            AllowDarkModeForWindowDelegate call =
                (AllowDarkModeForWindowDelegate)Marshal.GetDelegateForFunctionPointer(
                    proc, typeof(AllowDarkModeForWindowDelegate));
            call(hwnd, true);
        }
    }
}
'@
}

function Set-ExtensionDarkMode {
    param([System.Windows.Forms.Form]$Form)

    if (-not $script:UseWindowsDarkMode) { return }

    Initialize-ExtensionDarkMode
    [OpenSalamander.Extensions.DarkModeNativeMethods]::EnableImmersiveDarkMode()
    $background = [System.Drawing.Color]::FromArgb(32, 32, 32)
    $surface = [System.Drawing.Color]::FromArgb(45, 45, 48)
    $input = [System.Drawing.Color]::FromArgb(37, 37, 38)
    $header = [System.Drawing.Color]::FromArgb(50, 50, 54)
    $border = [System.Drawing.Color]::FromArgb(80, 80, 80)
    $text = [System.Drawing.Color]::FromArgb(241, 241, 241)
    $selection = [System.Drawing.Color]::FromArgb(0, 122, 204)

    $controls = New-Object System.Collections.Stack
    $controls.Push($Form)
    while ($controls.Count -gt 0) {
        $control = $controls.Pop()
        $control.BackColor = $background
        $control.ForeColor = $text

        if ($control -is [System.Windows.Forms.Button]) {
            $control.UseVisualStyleBackColor = $false
            $control.FlatStyle = 'Flat'
            $control.BackColor = $surface
            $control.FlatAppearance.BorderColor = $border
            $control.FlatAppearance.MouseOverBackColor =
                [System.Drawing.Color]::FromArgb(62, 62, 66)
            $control.FlatAppearance.MouseDownBackColor =
                [System.Drawing.Color]::FromArgb(75, 75, 80)
        } elseif (
            $control -is [System.Windows.Forms.CheckBox] -or
            $control -is [System.Windows.Forms.RadioButton]) {
            $control.FlatStyle = 'System'
        } elseif (
            $control -is [System.Windows.Forms.TextBox] -or
            $control -is [System.Windows.Forms.RichTextBox] -or
            $control -is [System.Windows.Forms.ComboBox]) {
            $control.BackColor = $input
        } elseif ($control -is [System.Windows.Forms.DataGridView]) {
            $control.BackgroundColor = $background
            $control.GridColor = $border
            $control.EnableHeadersVisualStyles = $false
            $control.ColumnHeadersDefaultCellStyle.BackColor = $header
            $control.ColumnHeadersDefaultCellStyle.ForeColor = $text
            $control.ColumnHeadersDefaultCellStyle.SelectionBackColor = $header
            $control.ColumnHeadersDefaultCellStyle.SelectionForeColor = $text
            $control.DefaultCellStyle.BackColor = $input
            $control.DefaultCellStyle.ForeColor = $text
            $control.DefaultCellStyle.SelectionBackColor = $selection
            $control.DefaultCellStyle.SelectionForeColor = $text
            $control.AlternatingRowsDefaultCellStyle.BackColor = $surface
            $control.AlternatingRowsDefaultCellStyle.ForeColor = $text
        }

        try {
            [OpenSalamander.Extensions.DarkModeNativeMethods]::AllowImmersiveDarkMode(
                $control.Handle)
            $theme = $null
            if ($control -is [System.Windows.Forms.CheckBox] -or
                $control -is [System.Windows.Forms.RadioButton]) {
                $theme = 'Explorer'
            } elseif (
                $control -is [System.Windows.Forms.TextBox] -or
                $control -is [System.Windows.Forms.RichTextBox] -or
                $control -is [System.Windows.Forms.ComboBox] -or
                $control -is [System.Windows.Forms.DataGridView]) {
                $theme = 'DarkMode_Explorer'
            }
            if ($null -ne $theme) {
                [void][OpenSalamander.Extensions.DarkModeNativeMethods]::SetWindowTheme(
                    $control.Handle, $theme, $null)
                [void][OpenSalamander.Extensions.DarkModeNativeMethods]::SendMessage(
                    $control.Handle, 0x031A, [IntPtr]::Zero, [IntPtr]::Zero)
            }
        } catch {}
        foreach ($child in $control.Controls) {
            $controls.Push($child)
        }
    }

    $enable = 1
    try {
        $result =
            [OpenSalamander.Extensions.DarkModeNativeMethods]::DwmSetWindowAttribute(
                $Form.Handle, 20, [ref]$enable, 4)
        if ($result -ne 0) {
            [void][OpenSalamander.Extensions.DarkModeNativeMethods]::DwmSetWindowAttribute(
                $Form.Handle, 19, [ref]$enable, 4)
        }
        [void][OpenSalamander.Extensions.DarkModeNativeMethods]::SendMessage(
            $Form.Handle, 0x031A, [IntPtr]::Zero, [IntPtr]::Zero)
    } catch {}
}

function Get-InspectorStrings {
    param([string]$Locale)

    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($Locale)) {
        $candidates.Add($Locale)
        $primary = ($Locale -split '-')[0]
        if ($primary -ne $Locale) { $candidates.Add($primary) }
    }
    $candidates.Add('en')

    foreach ($candidate in $candidates) {
        $path = Join-Path $PSScriptRoot "locales\$candidate.json"
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return (Get-Content -LiteralPath $path -Raw -Encoding UTF8 |
                ConvertFrom-Json).strings
        }
    }
    throw 'The English localization resource is missing.'
}

function Initialize-RestartManager {
    if ($null -ne ('OpenSalamander.FileLockInspector.RestartManager' -as [type])) {
        return
    }

    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

namespace OpenSalamander.FileLockInspector
{
    public enum RmAppType
    {
        Unknown = 0,
        MainWindow = 1,
        OtherWindow = 2,
        Service = 3,
        Explorer = 4,
        Console = 5,
        Critical = 1000
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RmUniqueProcess
    {
        public int ProcessId;
        public System.Runtime.InteropServices.ComTypes.FILETIME ProcessStartTime;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct RmProcessInfo
    {
        public RmUniqueProcess Process;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string AppName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string ServiceShortName;
        public RmAppType ApplicationType;
        public uint AppStatus;
        public uint TerminalSessionId;
        [MarshalAs(UnmanagedType.Bool)]
        public bool Restartable;
    }

    public static class RestartManager
    {
        private const int ErrorMoreData = 234;
        private const int SessionKeyLength = 32;

        [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
        private static extern int RmStartSession(
            out uint sessionHandle, int sessionFlags, StringBuilder sessionKey);

        [DllImport("rstrtmgr.dll")]
        private static extern int RmEndSession(uint sessionHandle);

        [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
        private static extern int RmRegisterResources(
            uint sessionHandle,
            uint fileCount,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)]
            string[] fileNames,
            uint applicationCount,
            IntPtr applications,
            uint serviceCount,
            string[] serviceNames);

        [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
        private static extern int RmGetList(
            uint sessionHandle,
            out uint processInfoNeeded,
            ref uint processInfoCount,
            [In, Out] RmProcessInfo[] affectedApplications,
            ref uint rebootReasons);

        public static RmProcessInfo[] GetLockingProcesses(string[] paths)
        {
            if (paths == null || paths.Length == 0)
                return new RmProcessInfo[0];

            uint handle;
            StringBuilder key = new StringBuilder(SessionKeyLength + 1);
            int result = RmStartSession(out handle, 0, key);
            if (result != 0)
                throw new Win32Exception(result);

            try
            {
                result = RmRegisterResources(
                    handle, (uint)paths.Length, paths, 0, IntPtr.Zero,
                    0, null);
                if (result != 0)
                    throw new Win32Exception(result);

                for (int attempt = 0; attempt < 3; ++attempt)
                {
                    uint needed;
                    uint count = 0;
                    uint reasons = 0;
                    result = RmGetList(
                        handle, out needed, ref count, null, ref reasons);
                    if (result == 0)
                        return new RmProcessInfo[0];
                    if (result != ErrorMoreData)
                        throw new Win32Exception(result);

                    RmProcessInfo[] processes = new RmProcessInfo[needed];
                    count = needed;
                    result = RmGetList(
                        handle, out needed, ref count, processes, ref reasons);
                    if (result == 0)
                    {
                        if (count == processes.Length)
                            return processes;
                        Array.Resize(ref processes, (int)count);
                        return processes;
                    }
                    if (result != ErrorMoreData)
                        throw new Win32Exception(result);
                }
                throw new Win32Exception(ErrorMoreData);
            }
            finally
            {
                RmEndSession(handle);
            }
        }
    }
}
'@
}

function Get-ApplicationTypeName {
    param([int]$Type)
    switch ($Type) {
        1 { return $script:Strings.appMainWindow }
        2 { return $script:Strings.appOtherWindow }
        3 { return $script:Strings.appService }
        4 { return $script:Strings.appExplorer }
        5 { return $script:Strings.appConsole }
        1000 { return $script:Strings.appCritical }
        default { return $script:Strings.appUnknown }
    }
}

function Get-LockingProcesses {
    param([string[]]$Paths)

    $result = New-Object System.Collections.Generic.List[object]
    $records =
        [OpenSalamander.FileLockInspector.RestartManager]::GetLockingProcesses(
            $Paths)
    foreach ($record in $records) {
        $pidValue = [int]$record.Process.ProcessId
        $name = [string]$record.AppName
        $executable = ''
        $responding = $true
        try {
            $process = [System.Diagnostics.Process]::GetProcessById($pidValue)
            if ([string]::IsNullOrWhiteSpace($name)) {
                $name = $process.ProcessName
            }
            try { $executable = $process.MainModule.FileName } catch {}
            try { $responding = $process.Responding } catch {}
        }
        catch {
            if ([string]::IsNullOrWhiteSpace($name)) {
                $name = $script:Strings.processEnded
            }
        }
        $result.Add([pscustomobject]@{
            Name = $name
            ProcessId = $pidValue
            Type = Get-ApplicationTypeName -Type ([int]$record.ApplicationType)
            Status = if ($responding) {
                $script:Strings.responding
            } else {
                $script:Strings.notResponding
            }
            Executable = $executable
            Restartable = [bool]$record.Restartable
            Service = [string]$record.ServiceShortName
        })
    }
    return @($result | Sort-Object Name, ProcessId)
}

function Get-SelectedProcess {
    param([System.Windows.Forms.DataGridView]$Grid)
    if ($Grid.SelectedRows.Count -eq 0) { return $null }
    return $Grid.SelectedRows[0].Tag
}

function Copy-InspectorReport {
    param([string[]]$Paths, [object[]]$Processes)

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add($script:Strings.title)
    $lines.Add('')
    $lines.Add($script:Strings.files + ':')
    foreach ($path in $Paths) { $lines.Add('  ' + $path) }
    $lines.Add('')
    if ($Processes.Count -eq 0) {
        $lines.Add($script:Strings.noLocks)
    } else {
        $lines.Add($script:Strings.lockingProcesses + ':')
        foreach ($process in $Processes) {
            $lines.Add((
                '{0} (PID {1}) [{2}] {3}' -f
                $process.Name, $process.ProcessId,
                $process.Type, $process.Executable))
        }
    }
    [void]$Salamander.clipboard.CopyText(($lines -join "`r`n"), $true)
}

function Show-InspectorWindow {
    param([string[]]$Paths)

    $form = New-Object System.Windows.Forms.Form
    $form.Text = $script:Strings.title
    $form.StartPosition = 'CenterScreen'
    $form.MinimumSize = New-Object System.Drawing.Size(760, 420)
    $form.Size = New-Object System.Drawing.Size(980, 560)
    $form.FormBorderStyle = 'Sizable'

    $filesLabel = New-Object System.Windows.Forms.Label
    $filesLabel.AutoEllipsis = $true
    $filesLabel.Text = $script:Strings.files + ': ' + ($Paths -join '; ')
    $filesLabel.SetBounds(12, 12, 940, 22)
    $filesLabel.Anchor = 'Top, Left, Right'
    $form.Controls.Add($filesLabel)

    $statusLabel = New-Object System.Windows.Forms.Label
    $statusLabel.SetBounds(12, 38, 940, 22)
    $statusLabel.Anchor = 'Top, Left, Right'
    $form.Controls.Add($statusLabel)

    $grid = New-Object System.Windows.Forms.DataGridView
    $grid.SetBounds(12, 64, 940, 400)
    $grid.Anchor = 'Top, Bottom, Left, Right'
    $grid.AllowUserToAddRows = $false
    $grid.AllowUserToDeleteRows = $false
    $grid.AllowUserToResizeRows = $false
    $grid.AutoSizeColumnsMode = 'Fill'
    $grid.BackgroundColor = [System.Drawing.SystemColors]::Window
    $grid.MultiSelect = $false
    $grid.ReadOnly = $true
    $grid.RowHeadersVisible = $false
    $grid.SelectionMode = 'FullRowSelect'
    [void]$grid.Columns.Add('process', $script:Strings.process)
    [void]$grid.Columns.Add('pid', $script:Strings.pid)
    [void]$grid.Columns.Add('type', $script:Strings.type)
    [void]$grid.Columns.Add('status', $script:Strings.status)
    [void]$grid.Columns.Add('path', $script:Strings.executable)
    $grid.Columns['pid'].FillWeight = 35
    $grid.Columns['type'].FillWeight = 55
    $grid.Columns['status'].FillWeight = 55
    $grid.Columns['path'].FillWeight = 180
    $form.Controls.Add($grid)

    $buttonPanel = New-Object System.Windows.Forms.FlowLayoutPanel
    $buttonPanel.FlowDirection = 'RightToLeft'
    $buttonPanel.WrapContents = $false
    $buttonPanel.SetBounds(12, 474, 940, 38)
    $buttonPanel.Anchor = 'Bottom, Left, Right'
    $form.Controls.Add($buttonPanel)

    function Add-InspectorButton {
        param([string]$Text)
        $button = New-Object System.Windows.Forms.Button
        $button.AutoSize = $true
        $button.MinimumSize = New-Object System.Drawing.Size(92, 27)
        $button.Text = $Text
        $buttonPanel.Controls.Add($button)
        return $button
    }

    $closeButton = Add-InspectorButton $script:Strings.close
    $refreshButton = Add-InspectorButton $script:Strings.refresh
    $copyButton = Add-InspectorButton $script:Strings.copyReport
    $endButton = Add-InspectorButton $script:Strings.endProcess
    $closeProcessButton = Add-InspectorButton $script:Strings.closeProcess
    $openButton = Add-InspectorButton $script:Strings.openLocation
    $form.AcceptButton = $refreshButton
    $form.CancelButton = $closeButton
    Set-ExtensionDarkMode -Form $form

    $script:InspectorProcesses = @()
    $refresh = {
        try {
            $statusLabel.Text = $script:Strings.inspecting
            $form.UseWaitCursor = $true
            [System.Windows.Forms.Application]::DoEvents()
            $script:InspectorProcesses = @(Get-LockingProcesses -Paths $Paths)
            $grid.Rows.Clear()
            foreach ($process in $script:InspectorProcesses) {
                $row = $grid.Rows.Add(
                    $process.Name,
                    $process.ProcessId,
                    $process.Type,
                    $process.Status,
                    $process.Executable)
                $grid.Rows[$row].Tag = $process
            }
            $statusLabel.Text = if ($script:InspectorProcesses.Count -eq 0) {
                $script:Strings.noLocks
            } else {
                $script:Strings.lockCount -f $script:InspectorProcesses.Count
            }
            if ($grid.Rows.Count -gt 0) {
                $grid.Rows[0].Selected = $true
                $grid.CurrentCell = $grid.Rows[0].Cells[0]
            }
        }
        catch {
            [void]$Salamander.ui.MessageBox(
                $_.Exception.Message, $script:Strings.title, 'OK', 'Error')
            $statusLabel.Text = $script:Strings.inspectFailed
        }
        finally {
            $form.UseWaitCursor = $false
        }
    }

    $openButton.Add_Click({
        $selected = Get-SelectedProcess -Grid $grid
        if ($null -eq $selected -or
            [string]::IsNullOrWhiteSpace($selected.Executable)) {
            [void]$Salamander.ui.Notify(
                $script:Strings.locationUnavailable,
                $script:Strings.title, 4000)
            return
        }
        $directory = Split-Path -LiteralPath $selected.Executable -Parent
        [void]$Salamander.target_side.CreateTab($directory)
    })
    $closeProcessButton.Add_Click({
        $selected = Get-SelectedProcess -Grid $grid
        if ($null -eq $selected) { return }
        try {
            $process =
                [System.Diagnostics.Process]::GetProcessById(
                    $selected.ProcessId)
            if (-not $process.CloseMainWindow()) {
                [void]$Salamander.ui.Notify(
                    $script:Strings.closeUnavailable,
                    $script:Strings.title, 4000)
            } else {
                [void]$process.WaitForExit(2000)
                & $refresh
            }
        }
        catch {
            [void]$Salamander.ui.MessageBox(
                $_.Exception.Message, $script:Strings.title, 'OK', 'Error')
        }
    })
    $endButton.Add_Click({
        $selected = Get-SelectedProcess -Grid $grid
        if ($null -eq $selected) { return }
        $message = $script:Strings.confirmEnd -f
            $selected.Name, $selected.ProcessId, [Environment]::NewLine
        $answer = $Salamander.ui.MessageBox(
            $message, $script:Strings.title, 'YesNo', 'Warning')
        if ($answer -ne 6) { return }
        try {
            $process =
                [System.Diagnostics.Process]::GetProcessById(
                    $selected.ProcessId)
            $process.Kill()
            [void]$process.WaitForExit(2000)
            & $refresh
        }
        catch {
            [void]$Salamander.ui.MessageBox(
                $_.Exception.Message, $script:Strings.title, 'OK', 'Error')
        }
    })
    $copyButton.Add_Click({
        Copy-InspectorReport -Paths $Paths `
            -Processes $script:InspectorProcesses
    })
    $refreshButton.Add_Click({ & $refresh })
    $closeButton.Add_Click({ $form.Close() })
    $grid.Add_CellDoubleClick({ $openButton.PerformClick() })

    try {
        & $refresh
        [void]$form.ShowDialog()
    }
    finally {
        $form.Dispose()
    }
}

if ($null -eq (Get-Variable -Name Salamander -ErrorAction SilentlyContinue)) {
    return
}

if ($Salamander.command_handler -eq 'inspect') {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing

    try {
        $appearance = $Salamander.application.Appearance()
        $darkProperty = $appearance.PSObject.Properties['windowsDarkMode']
        $script:UseWindowsDarkMode =
            $null -ne $darkProperty -and [bool]$darkProperty.Value
        if ($script:UseWindowsDarkMode) {
            Initialize-ExtensionDarkMode
            [OpenSalamander.Extensions.DarkModeNativeMethods]::EnableImmersiveDarkMode()
        }
        [System.Windows.Forms.Application]::EnableVisualStyles()

        $language = $Salamander.application.Language()
        $script:Strings = Get-InspectorStrings -Locale $language.locale

        $context = $Salamander.source_side.Context()
        $items = @($context.selectedItems | Where-Object {
            $null -ne $_ -and -not [bool]$_.isDirectory
        })
        if ($items.Count -eq 0 -and $null -ne $context.focusedItem -and
            -not [bool]$context.focusedItem.isDirectory) {
            $items = @($context.focusedItem)
        }
        $paths = @($items | ForEach-Object { [string]$_.path } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Select-Object -Unique)
        if ($paths.Count -eq 0) {
            [void]$Salamander.ui.MessageBox(
                $script:Strings.selectFile,
                $script:Strings.title,
                'OK',
                'Information')
            return
        }
        Initialize-RestartManager
        Show-InspectorWindow -Paths $paths
    }
    catch {
        $title = if ($null -ne (
            Get-Variable -Name Strings -Scope Script -ErrorAction SilentlyContinue)) {
            $script:Strings.title
        } else {
            'File Lock Inspector'
        }
        [void]$Salamander.ui.MessageBox(
            $_.Exception.Message, $title, 'OK', 'Error')
    }
}
