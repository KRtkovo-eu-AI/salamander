Set-StrictMode -Version 2.0

function Initialize-BuilderDarkMode {
    if ($null -ne ('OpenSalamander.ExtensionMenuBuilder.DarkMode' -as [type])) {
        return
    }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace OpenSalamander.ExtensionMenuBuilder
{
    public static class DarkMode
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int SetPreferredAppModeDelegate(int mode);
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
        private static extern IntPtr GetProcAddress(IntPtr module, IntPtr ordinal);
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

        private static int GetBuild()
        {
            RtlOsVersionInfo version = new RtlOsVersionInfo();
            version.Size = (uint)Marshal.SizeOf(typeof(RtlOsVersionInfo));
            return RtlGetVersion(ref version) == 0 ? (int)version.Build : 0;
        }

        public static void EnableApplication()
        {
            int build = GetBuild();
            if (build < 17763)
                return;
            IntPtr module = LoadLibrary("uxtheme.dll");
            if (module == IntPtr.Zero)
                return;
            IntPtr preferred = GetProcAddress(module, new IntPtr(135));
            if (preferred != IntPtr.Zero && build >= 18362)
            {
                SetPreferredAppModeDelegate call =
                    (SetPreferredAppModeDelegate)Marshal.GetDelegateForFunctionPointer(
                        preferred, typeof(SetPreferredAppModeDelegate));
                call(1);
            }
            IntPtr refresh = GetProcAddress(module, new IntPtr(104));
            if (refresh != IntPtr.Zero)
            {
                RefreshImmersiveColorPolicyStateDelegate call =
                    (RefreshImmersiveColorPolicyStateDelegate)
                    Marshal.GetDelegateForFunctionPointer(
                        refresh, typeof(RefreshImmersiveColorPolicyStateDelegate));
                call();
            }
        }

        public static void EnableWindow(IntPtr hwnd)
        {
            if (hwnd == IntPtr.Zero || GetBuild() < 17763)
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

function Set-BuilderDarkMode {
    param([System.Windows.Forms.Form]$Form)
    if (-not $script:UseWindowsDarkMode) { return }

    Initialize-BuilderDarkMode
    [OpenSalamander.ExtensionMenuBuilder.DarkMode]::EnableApplication()
    $background = [System.Drawing.Color]::FromArgb(32, 32, 32)
    $surface = [System.Drawing.Color]::FromArgb(45, 45, 48)
    $input = [System.Drawing.Color]::FromArgb(37, 37, 38)
    $header = [System.Drawing.Color]::FromArgb(50, 50, 54)
    $border = [System.Drawing.Color]::FromArgb(80, 80, 80)
    $text = [System.Drawing.Color]::FromArgb(241, 241, 241)
    $selection = [System.Drawing.Color]::FromArgb(0, 122, 204)

    $stack = New-Object System.Collections.Stack
    $stack.Push($Form)
    while ($stack.Count -gt 0) {
        $control = $stack.Pop()
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
            $control -is [System.Windows.Forms.TextBox] -or
            $control -is [System.Windows.Forms.ComboBox] -or
            $control -is [System.Windows.Forms.ListBox]) {
            $control.BackColor = $input
        } elseif ($control -is [System.Windows.Forms.GroupBox]) {
            $control.BackColor = $background
        }
        try {
            [OpenSalamander.ExtensionMenuBuilder.DarkMode]::EnableWindow(
                $control.Handle)
            $theme = if (
                $control -is [System.Windows.Forms.TextBox] -or
                $control -is [System.Windows.Forms.ComboBox] -or
                $control -is [System.Windows.Forms.ListBox]) {
                'DarkMode_Explorer'
            } else {
                $null
            }
            if ($null -ne $theme) {
                [void][OpenSalamander.ExtensionMenuBuilder.DarkMode]::SetWindowTheme(
                    $control.Handle, $theme, $null)
            }
            [void][OpenSalamander.ExtensionMenuBuilder.DarkMode]::SendMessage(
                $control.Handle, 0x031A, [IntPtr]::Zero, [IntPtr]::Zero)
        } catch {}
        foreach ($child in $control.Controls) { $stack.Push($child) }
    }
    $enable = 1
    try {
        $result =
            [OpenSalamander.ExtensionMenuBuilder.DarkMode]::DwmSetWindowAttribute(
                $Form.Handle, 20, [ref]$enable, 4)
        if ($result -ne 0) {
            [void][OpenSalamander.ExtensionMenuBuilder.DarkMode]::DwmSetWindowAttribute(
                $Form.Handle, 19, [ref]$enable, 4)
        }
    } catch {}
}

function Get-BuilderStrings {
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

function Get-BuilderString {
    param([string]$Name)
    $property = $script:Strings.PSObject.Properties[$Name]
    if ($null -ne $property) { return [string]$property.Value }
    return $Name
}

function New-BuilderCommand {
    param([string]$Title = '')
    if ([string]::IsNullOrWhiteSpace($Title)) {
        $Title = Get-BuilderString 'newCommand'
    }
    return [pscustomobject][ordered]@{
        Key = 'command' + ($script:Commands.Count + 1)
        Title = $Title
        Action = 'program'
        Target = ''
        Arguments = ''
        WorkingDirectory = ''
        Icon = ''
        IconDark = ''
        PluginMenu = $true
        ContextMenu = $false
        Toolbar = $false
        ToolbarMenu = $false
    }
}

function Get-ActionDisplayName {
    param([string]$Action)
    switch ($Action) {
        'powershell' { return Get-BuilderString 'actionPowerShell' }
        'command' { return Get-BuilderString 'actionCommand' }
        'open' { return Get-BuilderString 'actionOpen' }
        default { return Get-BuilderString 'actionProgram' }
    }
}

function Get-ActionFromDisplayName {
    param([string]$Name)
    if ($Name -eq (Get-BuilderString 'actionPowerShell')) { return 'powershell' }
    if ($Name -eq (Get-BuilderString 'actionCommand')) { return 'command' }
    if ($Name -eq (Get-BuilderString 'actionOpen')) { return 'open' }
    return 'program'
}

function ConvertTo-SafeFileName {
    param([string]$Name)
    $result = $Name
    foreach ($character in [System.IO.Path]::GetInvalidFileNameChars()) {
        $result = $result.Replace([string]$character, '_')
    }
    if ([string]::IsNullOrWhiteSpace($result)) { return 'asset' }
    return $result
}

function Copy-BuilderAsset {
    param(
        [string]$Source,
        [string]$ExtensionFolder,
        [string]$Subdirectory,
        [string]$PreferredName
    )
    if ([string]::IsNullOrWhiteSpace($Source)) { return '' }
    $resolved = [System.IO.Path]::GetFullPath($Source)
    if (-not [System.IO.File]::Exists($resolved)) {
        throw ((Get-BuilderString 'fileMissing') -f $Source)
    }
    $destinationDirectory = Join-Path $ExtensionFolder $Subdirectory
    [System.IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null
    $name = if ([string]::IsNullOrWhiteSpace($PreferredName)) {
        [System.IO.Path]::GetFileName($resolved)
    } else {
        ConvertTo-SafeFileName $PreferredName
    }
    $destination = Join-Path $destinationDirectory $name
    if ([System.StringComparer]::OrdinalIgnoreCase.Compare(
            $resolved, [System.IO.Path]::GetFullPath($destination)) -ne 0) {
        [System.IO.File]::Copy($resolved, $destination, $true)
    }
    return ($Subdirectory + '/' + $name).Replace('\', '/')
}

function Write-Utf8WithoutBom {
    param([string]$Path, [string]$Text)
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Set-BuilderObjectProperty {
    param([object]$Object, [string]$Name, [object]$Value)
    $property = $Object.PSObject.Properties[$Name]
    if ($null -ne $property) {
        $property.Value = $Value
    } else {
        $Object | Add-Member -NotePropertyName $Name -NotePropertyValue $Value
    }
}

function Get-DefaultExtensionIcon {
    param([bool]$Dark)
    $fill = if ($Dark) { '#62aef7' } else { '#2979c7' }
    $foreground = if ($Dark) { '#18212a' } else { '#ffffff' }
    return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <rect x="7" y="8" width="50" height="48" rx="5" fill="$fill"/>
  <path fill="$foreground" d="M16 18h8v8h-8zm13 1h19v6H29zM16 31h8v8h-8zm13 1h19v6H29zM16 44h8v4h-8zm13 0h12v4H29z"/>
</svg>
"@
}

function Get-BuilderPreviewImage {
    param([object]$Command)
    $candidates = if ($script:UseWindowsDarkMode) {
        @([string]$Command.IconDark, [string]$Command.Icon)
    } else {
        @([string]$Command.Icon)
    }
    $path = ''
    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and
            [System.IO.File]::Exists(
                [System.IO.Path]::GetFullPath($candidate))) {
            $path = [System.IO.Path]::GetFullPath($candidate)
            break
        }
    }
    if ([string]::IsNullOrWhiteSpace($path)) { return $null }

    try {
        $rendered = Invoke-Host -Method 'salamander.ui.renderIcon' -Arguments @{
            path = $path
            size = 16
        }
        if ($null -eq $rendered -or
            [string]::IsNullOrWhiteSpace([string]$rendered.icon)) {
            return $null
        }
        $bytes = [System.Convert]::FromBase64String([string]$rendered.icon)
        $stream = New-Object System.IO.MemoryStream -ArgumentList @(,$bytes)
        try {
            $source = New-Object System.Drawing.Icon -ArgumentList $stream
            try { return $source.ToBitmap() } finally { $source.Dispose() }
        } finally {
            $stream.Dispose()
        }
    } catch {
        return $null
    }
}

function Get-GeneratedRuntimeScript {
    return @'
Set-StrictMode -Version 2.0

function Expand-MenuValue {
    param([string]$Value, [object]$Source, [object]$Target)
    if ($null -eq $Value) { return '' }
    $focused = ''
    $sourcePath = ''
    $targetPath = ''
    $selected = @()
    if ($null -ne $Source) {
        if ($null -ne $Source.PSObject.Properties['path']) {
            $sourcePath = [string]$Source.path
        }
        if ($null -ne $Source.PSObject.Properties['focusedItem'] -and
            $null -ne $Source.focusedItem) {
            $focused = [string]$Source.focusedItem.path
        }
        if ($null -ne $Source.PSObject.Properties['selectedItems']) {
            $selected = @($Source.selectedItems | ForEach-Object {
                '"' + ([string]$_.path).Replace('"', '\"') + '"'
            })
        }
    }
    if ($null -ne $Target -and
        $null -ne $Target.PSObject.Properties['path']) {
        $targetPath = [string]$Target.path
    }
    $expanded = $Value.
        Replace('${focusedItem.path}', $focused).
        Replace('${activePanel.path}', $sourcePath).
        Replace('${sourcePanel.path}', $sourcePath).
        Replace('${targetPanel.path}', $targetPath).
        Replace('${selectedItems}', ($selected -join ' '))
    return [System.Environment]::ExpandEnvironmentVariables($expanded)
}

function Resolve-MenuTarget {
    param([string]$Target)
    if ([string]::IsNullOrWhiteSpace($Target)) { return $Target }
    if ($Target -match '^[A-Za-z][A-Za-z0-9+.-]*:') { return $Target }
    if ([System.IO.Path]::IsPathRooted($Target)) { return $Target }
    return Join-Path $PSScriptRoot ($Target.Replace('/', '\'))
}

if ($null -eq (Get-Variable -Name Salamander -ErrorAction SilentlyContinue)) {
    return
}

try {
    $configuration = Get-Content -LiteralPath (
        Join-Path $PSScriptRoot 'actions.json') -Raw -Encoding UTF8 |
        ConvertFrom-Json
    $action = @($configuration.commands | Where-Object {
        $_.handler -eq $Salamander.command_handler
    } | Select-Object -First 1)
    if ($action.Count -eq 0) { throw 'The requested menu action was not found.' }
    $action = $action[0]
    $source = $Salamander.source_side.Context()
    $target = $Salamander.target_side.Context()
    $expandedTarget = Expand-MenuValue ([string]$action.target) $source $target
    $expandedArguments =
        Expand-MenuValue ([string]$action.arguments) $source $target
    $expandedWorkingDirectory =
        Expand-MenuValue ([string]$action.workingDirectory) $source $target
    $resolvedTarget = Resolve-MenuTarget $expandedTarget

    switch ([string]$action.action) {
        'powershell' {
            $arguments = '-NoProfile -ExecutionPolicy Bypass -File "' +
                $resolvedTarget.Replace('"', '\"') + '"'
            if (-not [string]::IsNullOrWhiteSpace($expandedArguments)) {
                $arguments += ' ' + $expandedArguments
            }
            $parameters = @{
                FilePath = 'powershell.exe'
                ArgumentList = $arguments
            }
            if (-not [string]::IsNullOrWhiteSpace($expandedWorkingDirectory)) {
                $parameters.WorkingDirectory = $expandedWorkingDirectory
            }
            Start-Process @parameters
        }
        'command' {
            $line = $expandedTarget
            if (-not [string]::IsNullOrWhiteSpace($expandedArguments)) {
                $line += ' ' + $expandedArguments
            }
            $parameters = @{
                FilePath = $env:ComSpec
                ArgumentList = @('/d', '/c', $line)
            }
            if (-not [string]::IsNullOrWhiteSpace($expandedWorkingDirectory)) {
                $parameters.WorkingDirectory = $expandedWorkingDirectory
            }
            Start-Process @parameters
        }
        'open' {
            Start-Process -FilePath $resolvedTarget
        }
        default {
            $parameters = @{ FilePath = $resolvedTarget }
            if (-not [string]::IsNullOrWhiteSpace($expandedArguments)) {
                $parameters.ArgumentList = $expandedArguments
            }
            if (-not [string]::IsNullOrWhiteSpace($expandedWorkingDirectory)) {
                $parameters.WorkingDirectory = $expandedWorkingDirectory
            }
            Start-Process @parameters
        }
    }
}
catch {
    [void]$Salamander.ui.MessageBox(
        $_.Exception.Message, 'Custom menu', 'OK', 'Error')
}
'@
}

function Save-BuilderProject {
    param(
        [string]$Name,
        [string]$Id,
        [string]$Description,
        [string]$ExtensionFolder
    )
    if ([string]::IsNullOrWhiteSpace($Name) -or
        [string]::IsNullOrWhiteSpace($Id) -or
        [string]::IsNullOrWhiteSpace($ExtensionFolder)) {
        throw (Get-BuilderString 'requiredExtensionFields')
    }
    if ($Id -notmatch '^[A-Za-z][A-Za-z0-9_-]*(\.[A-Za-z0-9][A-Za-z0-9_-]*)+$') {
        throw (Get-BuilderString 'invalidExtensionId')
    }
    if ($script:Commands.Count -eq 0) {
        throw (Get-BuilderString 'atLeastOneCommand')
    }
    $manifestOnly = $null -ne (
        Get-Variable -Name ManifestOnlyMode -Scope Script -ErrorAction SilentlyContinue) -and
        [bool]$script:ManifestOnlyMode
    $keys = @{}
    foreach ($command in $script:Commands) {
        if ([string]::IsNullOrWhiteSpace($command.Title) -or
            $command.Key -notmatch '^[A-Za-z][A-Za-z0-9_-]*$') {
            throw (Get-BuilderString 'invalidCommand')
        }
        if ($keys.ContainsKey($command.Key)) {
            throw ((Get-BuilderString 'duplicateCommand') -f $command.Key)
        }
        $keys[$command.Key] = $true
        if (-not $manifestOnly -and
            [string]::IsNullOrWhiteSpace($command.Target)) {
            throw ((Get-BuilderString 'targetRequired') -f $command.Title)
        }
        if ($command.ToolbarMenu -and -not $command.Toolbar) {
            throw ((Get-BuilderString 'toolbarMenuRequiresToolbar') -f
                $command.Title)
        }
    }

    $folder = [System.IO.Path]::GetFullPath($ExtensionFolder)
    [System.IO.Directory]::CreateDirectory($folder) | Out-Null
    $manifestPath = Join-Path $folder 'extension.json'
    $projectPath = Join-Path $folder 'menu-builder.json'
    if ($manifestOnly) {
        $loadedFolder =
            [System.IO.Path]::GetFullPath($script:ImportedExtensionFolder)
        if ([System.StringComparer]::OrdinalIgnoreCase.Compare(
                $folder, $loadedFolder) -ne 0) {
            throw (Get-BuilderString 'manifestOnlySameFolder')
        }
        if ([System.IO.File]::Exists($manifestPath)) {
            [System.IO.File]::Copy(
                $manifestPath, $manifestPath + '.bak', $true)
        }
        $updatedCommands = New-Object System.Collections.Generic.List[object]
        foreach ($command in $script:Commands) {
            $commandId = $Id + '.' + [string]$command.Key
            $existing = @($script:ImportedManifest.commands | Where-Object {
                $existingId = if ($null -ne $_.PSObject.Properties['id']) {
                    [string]$_.id
                } else {
                    ''
                }
                $existingHandler = if (
                    $null -ne $_.PSObject.Properties['handler']) {
                    [string]$_.handler
                } else {
                    ''
                }
                ($existingId -eq $commandId) -or
                ($existingHandler -eq [string]$command.Key)
            } | Select-Object -First 1)
            $manifestCommand = if ($existing.Count -gt 0) {
                $existing[0]
            } else {
                [pscustomobject][ordered]@{}
            }
            $menu = if ($command.PluginMenu -and $command.ContextMenu) {
                'both'
            } elseif ($command.PluginMenu) {
                'plugin'
            } elseif ($command.ContextMenu) {
                'context'
            } else {
                'none'
            }
            Set-BuilderObjectProperty $manifestCommand 'id' $commandId
            Set-BuilderObjectProperty $manifestCommand 'title' (
                [string]$command.Title)
            Set-BuilderObjectProperty $manifestCommand 'handler' (
                [string]$command.Key)
            Set-BuilderObjectProperty $manifestCommand 'menu' $menu
            Set-BuilderObjectProperty $manifestCommand 'contextMenu' (
                [bool]$command.ContextMenu)
            Set-BuilderObjectProperty $manifestCommand 'toolbar' (
                [bool]$command.Toolbar)
            Set-BuilderObjectProperty $manifestCommand 'toolbarMenu' (
                [bool]$command.ToolbarMenu)
            $icon = Copy-BuilderAsset $command.Icon $folder 'icons' (
                ([string]$command.Key) + '.svg')
            $iconDark = Copy-BuilderAsset $command.IconDark $folder 'icons' (
                ([string]$command.Key) + '-dark.svg')
            if ([string]::IsNullOrWhiteSpace($icon)) {
                $manifestCommand.PSObject.Properties.Remove('icon')
            } else {
                Set-BuilderObjectProperty $manifestCommand 'icon' $icon
            }
            if ([string]::IsNullOrWhiteSpace($iconDark)) {
                $manifestCommand.PSObject.Properties.Remove('iconDark')
            } else {
                Set-BuilderObjectProperty $manifestCommand 'iconDark' $iconDark
            }
            $updatedCommands.Add($manifestCommand)
        }
        Set-BuilderObjectProperty $script:ImportedManifest 'name' $Name
        Set-BuilderObjectProperty $script:ImportedManifest 'id' $Id
        Set-BuilderObjectProperty $script:ImportedManifest 'description' (
            $Description)
        Set-BuilderObjectProperty $script:ImportedManifest 'commands' (
            [object[]]$updatedCommands.ToArray())
        Write-Utf8WithoutBom $manifestPath (
            $script:ImportedManifest | ConvertTo-Json -Depth 12)
        return $true
    }
    if ([System.IO.File]::Exists($manifestPath) -and
        -not [System.IO.File]::Exists($projectPath)) {
        $answer = $Salamander.ui.MessageBox(
            (Get-BuilderString 'overwriteHandwritten'),
            (Get-BuilderString 'title'), 'YesNo', 'Warning')
        if ($answer -ne 6) { return $false }
        foreach ($file in @(
                'extension.json', 'main.ps1', 'actions.json',
                'icon.svg', 'icon-dark.svg')) {
            $existing = Join-Path $folder $file
            if ([System.IO.File]::Exists($existing)) {
                [System.IO.File]::Copy($existing, $existing + '.bak', $true)
            }
        }
    }

    if (-not [System.IO.File]::Exists((Join-Path $folder 'icon.svg'))) {
        Write-Utf8WithoutBom (Join-Path $folder 'icon.svg') (
            Get-DefaultExtensionIcon $false)
    }
    if (-not [System.IO.File]::Exists((Join-Path $folder 'icon-dark.svg'))) {
        Write-Utf8WithoutBom (Join-Path $folder 'icon-dark.svg') (
            Get-DefaultExtensionIcon $true)
    }

    $manifestCommands = New-Object System.Collections.Generic.List[object]
    $runtimeCommands = New-Object System.Collections.Generic.List[object]
    $projectCommands = New-Object System.Collections.Generic.List[object]
    foreach ($command in $script:Commands) {
        $handler = [string]$command.Key
        $commandId = $Id + '.' + $handler
        $menu = if ($command.PluginMenu -and $command.ContextMenu) {
            'both'
        } elseif ($command.PluginMenu) {
            'plugin'
        } elseif ($command.ContextMenu) {
            'context'
        } else {
            'none'
        }
        $icon = Copy-BuilderAsset $command.Icon $folder 'icons' (
            $handler + '.svg')
        $iconDark = Copy-BuilderAsset $command.IconDark $folder 'icons' (
            $handler + '-dark.svg')
        $target = [string]$command.Target
        $projectTarget = [string]$command.Target
        if ($command.Action -eq 'powershell' -and
            [System.IO.File]::Exists($target)) {
            $target = Copy-BuilderAsset $target $folder 'scripts' (
                [System.IO.Path]::GetFileName($target))
            $projectTarget = Join-Path $folder ($target.Replace('/', '\'))
        }
        $manifestCommand = [ordered]@{
            id = $commandId
            title = [string]$command.Title
            handler = $handler
            menu = $menu
            contextMenu = [bool]$command.ContextMenu
            toolbar = [bool]$command.Toolbar
            toolbarMenu = [bool]$command.ToolbarMenu
        }
        if (-not [string]::IsNullOrWhiteSpace($icon)) {
            $manifestCommand.icon = $icon
        }
        if (-not [string]::IsNullOrWhiteSpace($iconDark)) {
            $manifestCommand.iconDark = $iconDark
        }
        $manifestCommands.Add([pscustomobject]$manifestCommand)
        $runtimeCommands.Add([pscustomobject][ordered]@{
            handler = $handler
            action = [string]$command.Action
            target = $target
            arguments = [string]$command.Arguments
            workingDirectory = [string]$command.WorkingDirectory
        })
        $projectCommands.Add([pscustomobject][ordered]@{
            key = $handler
            title = [string]$command.Title
            action = [string]$command.Action
            target = $projectTarget
            arguments = [string]$command.Arguments
            workingDirectory = [string]$command.WorkingDirectory
            icon = if ([string]::IsNullOrWhiteSpace($icon)) {
                ''
            } else {
                Join-Path $folder ($icon.Replace('/', '\'))
            }
            iconDark = if ([string]::IsNullOrWhiteSpace($iconDark)) {
                ''
            } else {
                Join-Path $folder ($iconDark.Replace('/', '\'))
            }
            pluginMenu = [bool]$command.PluginMenu
            contextMenu = [bool]$command.ContextMenu
            toolbar = [bool]$command.Toolbar
            toolbarMenu = [bool]$command.ToolbarMenu
        })
    }

    $manifest = [pscustomobject][ordered]@{
        schema = 1
        id = $Id
        name = $Name
        version = '1.0.0'
        description = $Description
        runtime = 'PowerShell'
        entryPoint = 'main.ps1'
        icon = 'icon.svg'
        iconDark = 'icon-dark.svg'
        capabilities = @('panels.read', 'ui.dialogs')
        commands = [object[]]$manifestCommands.ToArray()
    }
    $actions = [pscustomobject][ordered]@{
        schema = 1
        generatedBy = 'OpenSalamander.ExtensionMenuBuilder'
        commands = [object[]]$runtimeCommands.ToArray()
    }
    $project = [pscustomobject][ordered]@{
        schema = 1
        generatedBy = 'OpenSalamander.ExtensionMenuBuilder'
        name = $Name
        id = $Id
        description = $Description
        commands = [object[]]$projectCommands.ToArray()
    }
    Write-Utf8WithoutBom $manifestPath (
        $manifest | ConvertTo-Json -Depth 8)
    Write-Utf8WithoutBom (Join-Path $folder 'actions.json') (
        $actions | ConvertTo-Json -Depth 8)
    Write-Utf8WithoutBom $projectPath (
        $project | ConvertTo-Json -Depth 8)
    Write-Utf8WithoutBom (Join-Path $folder 'main.ps1') (
        Get-GeneratedRuntimeScript)
    return $true
}

function Load-BuilderProject {
    param([string]$ManifestPath)
    $manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 |
        ConvertFrom-Json
    $folder = Split-Path -Parent $ManifestPath
    $projectPath = Join-Path $folder 'menu-builder.json'
    $project = if (Test-Path -LiteralPath $projectPath -PathType Leaf) {
        Get-Content -LiteralPath $projectPath -Raw -Encoding UTF8 |
            ConvertFrom-Json
    } else {
        $null
    }
    $script:Commands.Clear()
    $sourceCommands = if ($null -ne $project) {
        @($project.commands)
    } else {
        @($manifest.commands)
    }
    foreach ($source in $sourceCommands) {
        $sourceTitle = if ($null -ne $source.PSObject.Properties['title']) {
            [string]$source.title
        } else {
            ''
        }
        $command = New-BuilderCommand $sourceTitle
        $command.Key = if ($null -ne $source.PSObject.Properties['key']) {
            [string]$source.key
        } elseif ($null -ne $source.PSObject.Properties['handler']) {
            [string]$source.handler
        } elseif ($null -ne $source.PSObject.Properties['id']) {
            ([string]$source.id -split '\.')[-1]
        } else {
            'command' + ($script:Commands.Count + 1)
        }
        foreach ($property in @(
                'action', 'target', 'arguments', 'workingDirectory',
                'icon', 'iconDark')) {
            if ($null -ne $source.PSObject.Properties[$property]) {
                $targetName = $property.Substring(0, 1).ToUpperInvariant() +
                    $property.Substring(1)
                $command.$targetName = [string]$source.$property
            }
        }
        if ($null -ne $project) {
            $command.PluginMenu = [bool]$source.pluginMenu
            $command.ContextMenu = [bool]$source.contextMenu
            $command.Toolbar = [bool]$source.toolbar
            $command.ToolbarMenu = [bool]$source.toolbarMenu
        } else {
            $menu = if ($null -ne $source.PSObject.Properties['menu']) {
                [string]$source.menu
            } else {
                'plugin'
            }
            $command.PluginMenu = $menu -eq 'plugin' -or $menu -eq 'both'
            $contextMenu = $null -ne (
                $source.PSObject.Properties['contextMenu']) -and
                [bool]$source.contextMenu
            $command.ContextMenu =
                $menu -eq 'context' -or $menu -eq 'both' -or
                $contextMenu
            $command.Toolbar =
                $null -ne $source.PSObject.Properties['toolbar'] -and
                [bool]$source.toolbar
            $command.ToolbarMenu =
                $null -ne $source.PSObject.Properties['toolbarMenu'] -and
                [bool]$source.toolbarMenu
            if (-not [string]::IsNullOrWhiteSpace($command.Icon) -and
                -not [System.IO.Path]::IsPathRooted($command.Icon)) {
                $command.Icon = Join-Path $folder (
                    $command.Icon.Replace('/', '\'))
            }
            if (-not [string]::IsNullOrWhiteSpace($command.IconDark) -and
                -not [System.IO.Path]::IsPathRooted($command.IconDark)) {
                $command.IconDark = Join-Path $folder (
                    $command.IconDark.Replace('/', '\'))
            }
        }
        $script:Commands.Add($command)
    }
    return [pscustomobject]@{
        Name = [string]$manifest.name
        Id = [string]$manifest.id
        Description = if ($null -ne (
            $manifest.PSObject.Properties['description'])) {
            [string]$manifest.description
        } else {
            ''
        }
        Folder = $folder
        Generated = $null -ne $project
        Manifest = $manifest
    }
}

function Show-BuilderWindow {
    $script:Commands =
        New-Object System.Collections.Generic.List[object]
    $script:Commands.Add((New-BuilderCommand))
    $script:UpdatingEditor = $false
    $script:ManifestOnlyMode = $false
    $script:ImportedManifest = $null
    $script:ImportedExtensionFolder = ''

    $form = New-Object System.Windows.Forms.Form
    $form.Text = Get-BuilderString 'title'
    $form.StartPosition = 'CenterScreen'
    $form.MinimumSize = New-Object System.Drawing.Size(940, 680)
    $form.Size = New-Object System.Drawing.Size(1080, 760)
    $form.FormBorderStyle = 'Sizable'

    $extensionGroup = New-Object System.Windows.Forms.GroupBox
    $extensionGroup.Text = Get-BuilderString 'extension'
    $extensionGroup.SetBounds(12, 10, 1040, 142)
    $extensionGroup.Anchor = 'Top, Left, Right'
    $form.Controls.Add($extensionGroup)

    $nameLabel = New-Object System.Windows.Forms.Label
    $nameLabel.Text = Get-BuilderString 'name'
    $nameLabel.SetBounds(12, 27, 95, 22)
    $extensionGroup.Controls.Add($nameLabel)
    $nameBox = New-Object System.Windows.Forms.TextBox
    $nameBox.Text = Get-BuilderString 'defaultExtensionName'
    $nameBox.SetBounds(112, 24, 310, 24)
    $extensionGroup.Controls.Add($nameBox)

    $idLabel = New-Object System.Windows.Forms.Label
    $idLabel.Text = Get-BuilderString 'identifier'
    $idLabel.SetBounds(438, 27, 85, 22)
    $extensionGroup.Controls.Add($idLabel)
    $idBox = New-Object System.Windows.Forms.TextBox
    $idBox.Text = 'OpenSalamander.CustomMenu'
    $idBox.SetBounds(528, 24, 495, 24)
    $idBox.Anchor = 'Top, Left, Right'
    $extensionGroup.Controls.Add($idBox)

    $descriptionLabel = New-Object System.Windows.Forms.Label
    $descriptionLabel.Text = Get-BuilderString 'description'
    $descriptionLabel.SetBounds(12, 59, 95, 22)
    $extensionGroup.Controls.Add($descriptionLabel)
    $descriptionBox = New-Object System.Windows.Forms.TextBox
    $descriptionBox.SetBounds(112, 56, 911, 24)
    $descriptionBox.Anchor = 'Top, Left, Right'
    $extensionGroup.Controls.Add($descriptionBox)

    $folderLabel = New-Object System.Windows.Forms.Label
    $folderLabel.Text = Get-BuilderString 'extensionFolder'
    $folderLabel.SetBounds(12, 91, 95, 22)
    $extensionGroup.Controls.Add($folderLabel)
    $folderBox = New-Object System.Windows.Forms.TextBox
    $folderBox.Text = Join-Path (Split-Path -Parent $PSScriptRoot) (
        'my-custom-menu')
    $folderBox.SetBounds(112, 88, 805, 24)
    $folderBox.Anchor = 'Top, Left, Right'
    $extensionGroup.Controls.Add($folderBox)
    $folderButton = New-Object System.Windows.Forms.Button
    $folderButton.Text = Get-BuilderString 'browse'
    $folderButton.SetBounds(925, 87, 98, 26)
    $folderButton.Anchor = 'Top, Right'
    $extensionGroup.Controls.Add($folderButton)

    $commandsGroup = New-Object System.Windows.Forms.GroupBox
    $commandsGroup.Text = Get-BuilderString 'commands'
    $commandsGroup.SetBounds(12, 160, 300, 510)
    $commandsGroup.Anchor = 'Top, Bottom, Left'
    $form.Controls.Add($commandsGroup)
    $commandList = New-Object System.Windows.Forms.ListBox
    $commandList.SetBounds(10, 24, 278, 404)
    $commandList.Anchor = 'Top, Bottom, Left, Right'
    $commandsGroup.Controls.Add($commandList)

    $addButton = New-Object System.Windows.Forms.Button
    $addButton.Text = Get-BuilderString 'add'
    $addButton.SetBounds(10, 438, 64, 28)
    $addButton.Anchor = 'Bottom, Left'
    $commandsGroup.Controls.Add($addButton)
    $scriptButton = New-Object System.Windows.Forms.Button
    $scriptButton.Text = Get-BuilderString 'addScript'
    $scriptButton.SetBounds(78, 438, 92, 28)
    $scriptButton.Anchor = 'Bottom, Left'
    $commandsGroup.Controls.Add($scriptButton)
    $removeButton = New-Object System.Windows.Forms.Button
    $removeButton.Text = Get-BuilderString 'remove'
    $removeButton.SetBounds(174, 438, 114, 28)
    $removeButton.Anchor = 'Bottom, Left, Right'
    $commandsGroup.Controls.Add($removeButton)
    $upButton = New-Object System.Windows.Forms.Button
    $upButton.Text = Get-BuilderString 'moveUp'
    $upButton.SetBounds(10, 472, 136, 28)
    $upButton.Anchor = 'Bottom, Left'
    $commandsGroup.Controls.Add($upButton)
    $downButton = New-Object System.Windows.Forms.Button
    $downButton.Text = Get-BuilderString 'moveDown'
    $downButton.SetBounds(152, 472, 136, 28)
    $downButton.Anchor = 'Bottom, Left'
    $commandsGroup.Controls.Add($downButton)

    $editorGroup = New-Object System.Windows.Forms.GroupBox
    $editorGroup.Text = Get-BuilderString 'command'
    $editorGroup.SetBounds(324, 160, 728, 510)
    $editorGroup.Anchor = 'Top, Bottom, Left, Right'
    $form.Controls.Add($editorGroup)

    function Add-EditorLabel {
        param([string]$Text, [int]$Y)
        $label = New-Object System.Windows.Forms.Label
        $label.Text = $Text
        $label.SetBounds(12, $Y, 145, 22)
        $editorGroup.Controls.Add($label)
    }
    Add-EditorLabel (Get-BuilderString 'commandTitle') 28
    $titleBox = New-Object System.Windows.Forms.TextBox
    $titleBox.SetBounds(164, 25, 545, 24)
    $titleBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($titleBox)
    Add-EditorLabel (Get-BuilderString 'commandKey') 60
    $keyBox = New-Object System.Windows.Forms.TextBox
    $keyBox.SetBounds(164, 57, 545, 24)
    $keyBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($keyBox)
    Add-EditorLabel (Get-BuilderString 'action') 92
    $actionBox = New-Object System.Windows.Forms.ComboBox
    $actionBox.DropDownStyle = 'DropDownList'
    $actionBox.SetBounds(164, 89, 545, 24)
    $actionBox.Anchor = 'Top, Left, Right'
    foreach ($name in @(
            Get-BuilderString 'actionProgram',
            Get-BuilderString 'actionPowerShell',
            Get-BuilderString 'actionCommand',
            Get-BuilderString 'actionOpen')) {
        [void]$actionBox.Items.Add($name)
    }
    $editorGroup.Controls.Add($actionBox)
    Add-EditorLabel (Get-BuilderString 'target') 124
    $targetBox = New-Object System.Windows.Forms.TextBox
    $targetBox.SetBounds(164, 121, 438, 24)
    $targetBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($targetBox)
    $targetButton = New-Object System.Windows.Forms.Button
    $targetButton.Text = Get-BuilderString 'browse'
    $targetButton.SetBounds(610, 120, 99, 26)
    $targetButton.Anchor = 'Top, Right'
    $editorGroup.Controls.Add($targetButton)
    Add-EditorLabel (Get-BuilderString 'arguments') 156
    $argumentsBox = New-Object System.Windows.Forms.TextBox
    $argumentsBox.SetBounds(164, 153, 545, 24)
    $argumentsBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($argumentsBox)
    Add-EditorLabel (Get-BuilderString 'workingDirectory') 188
    $workingBox = New-Object System.Windows.Forms.TextBox
    $workingBox.SetBounds(164, 185, 438, 24)
    $workingBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($workingBox)
    $workingButton = New-Object System.Windows.Forms.Button
    $workingButton.Text = Get-BuilderString 'browse'
    $workingButton.SetBounds(610, 184, 99, 26)
    $workingButton.Anchor = 'Top, Right'
    $editorGroup.Controls.Add($workingButton)
    Add-EditorLabel (Get-BuilderString 'icon') 220
    $iconBox = New-Object System.Windows.Forms.TextBox
    $iconBox.SetBounds(164, 217, 438, 24)
    $iconBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($iconBox)
    $iconButton = New-Object System.Windows.Forms.Button
    $iconButton.Text = Get-BuilderString 'browse'
    $iconButton.SetBounds(610, 216, 99, 26)
    $iconButton.Anchor = 'Top, Right'
    $editorGroup.Controls.Add($iconButton)
    Add-EditorLabel (Get-BuilderString 'iconDark') 252
    $iconDarkBox = New-Object System.Windows.Forms.TextBox
    $iconDarkBox.SetBounds(164, 249, 438, 24)
    $iconDarkBox.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($iconDarkBox)
    $iconDarkButton = New-Object System.Windows.Forms.Button
    $iconDarkButton.Text = Get-BuilderString 'browse'
    $iconDarkButton.SetBounds(610, 248, 99, 26)
    $iconDarkButton.Anchor = 'Top, Right'
    $editorGroup.Controls.Add($iconDarkButton)

    $placementLabel = New-Object System.Windows.Forms.Label
    $placementLabel.Text = Get-BuilderString 'placement'
    $placementLabel.SetBounds(12, 290, 145, 22)
    $editorGroup.Controls.Add($placementLabel)
    $pluginCheck = New-Object System.Windows.Forms.CheckBox
    $pluginCheck.Text = Get-BuilderString 'pluginsMenu'
    $pluginCheck.SetBounds(164, 286, 250, 24)
    $editorGroup.Controls.Add($pluginCheck)
    $contextCheck = New-Object System.Windows.Forms.CheckBox
    $contextCheck.Text = Get-BuilderString 'contextMenu'
    $contextCheck.SetBounds(420, 286, 250, 24)
    $editorGroup.Controls.Add($contextCheck)
    $toolbarCheck = New-Object System.Windows.Forms.CheckBox
    $toolbarCheck.Text = Get-BuilderString 'extensionBar'
    $toolbarCheck.SetBounds(164, 316, 250, 24)
    $editorGroup.Controls.Add($toolbarCheck)
    $toolbarMenuCheck = New-Object System.Windows.Forms.CheckBox
    $toolbarMenuCheck.Text = Get-BuilderString 'toolbarOpensMenu'
    $toolbarMenuCheck.SetBounds(420, 316, 280, 24)
    $editorGroup.Controls.Add($toolbarMenuCheck)

    $tokensLabel = New-Object System.Windows.Forms.Label
    $tokensLabel.Text = Get-BuilderString 'tokens'
    $tokensLabel.SetBounds(12, 355, 697, 65)
    $tokensLabel.Anchor = 'Top, Left, Right'
    $editorGroup.Controls.Add($tokensLabel)
    $previewButton = New-Object System.Windows.Forms.Button
    $previewButton.Text = Get-BuilderString 'preview'
    $previewButton.SetBounds(164, 438, 180, 30)
    $previewButton.Anchor = 'Bottom, Left'
    $editorGroup.Controls.Add($previewButton)

    $openButton = New-Object System.Windows.Forms.Button
    $openButton.Text = Get-BuilderString 'openExtension'
    $openButton.SetBounds(12, 682, 170, 30)
    $openButton.Anchor = 'Bottom, Left'
    $form.Controls.Add($openButton)
    $saveButton = New-Object System.Windows.Forms.Button
    $saveButton.Text = Get-BuilderString 'saveExtension'
    $saveButton.SetBounds(690, 682, 180, 30)
    $saveButton.Anchor = 'Bottom, Right'
    $form.Controls.Add($saveButton)
    $closeButton = New-Object System.Windows.Forms.Button
    $closeButton.Text = Get-BuilderString 'close'
    $closeButton.SetBounds(878, 682, 174, 30)
    $closeButton.Anchor = 'Bottom, Right'
    $form.Controls.Add($closeButton)

    function Refresh-CommandList {
        param([int]$SelectedIndex = -1)
        $commandList.BeginUpdate()
        $commandList.Items.Clear()
        foreach ($command in $script:Commands) {
            [void]$commandList.Items.Add([string]$command.Title)
        }
        $commandList.EndUpdate()
        if ($script:Commands.Count -gt 0) {
            if ($SelectedIndex -lt 0) { $SelectedIndex = 0 }
            if ($SelectedIndex -ge $script:Commands.Count) {
                $SelectedIndex = $script:Commands.Count - 1
            }
            $commandList.SelectedIndex = $SelectedIndex
        }
    }

    function Load-CommandEditor {
        $index = $commandList.SelectedIndex
        $enabled = $index -ge 0 -and $index -lt $script:Commands.Count
        $editorGroup.Enabled = $enabled
        $removeButton.Enabled = $enabled
        $upButton.Enabled = $enabled -and $index -gt 0
        $downButton.Enabled =
            $enabled -and $index -lt ($script:Commands.Count - 1)
        if (-not $enabled) { return }
        $actionEditable = -not $script:ManifestOnlyMode
        foreach ($control in @(
                $actionBox, $targetBox, $targetButton, $argumentsBox,
                $workingBox, $workingButton)) {
            $control.Enabled = $actionEditable
        }
        $script:UpdatingEditor = $true
        $command = $script:Commands[$index]
        $titleBox.Text = $command.Title
        $keyBox.Text = $command.Key
        $actionBox.SelectedItem = Get-ActionDisplayName $command.Action
        $targetBox.Text = $command.Target
        $argumentsBox.Text = $command.Arguments
        $workingBox.Text = $command.WorkingDirectory
        $iconBox.Text = $command.Icon
        $iconDarkBox.Text = $command.IconDark
        $pluginCheck.Checked = $command.PluginMenu
        $contextCheck.Checked = $command.ContextMenu
        $toolbarCheck.Checked = $command.Toolbar
        $toolbarMenuCheck.Checked = $command.ToolbarMenu
        $toolbarMenuCheck.Enabled = $command.Toolbar
        $script:UpdatingEditor = $false
    }

    function Save-CommandEditor {
        if ($script:UpdatingEditor) { return }
        $index = $commandList.SelectedIndex
        if ($index -lt 0 -or $index -ge $script:Commands.Count) { return }
        $command = $script:Commands[$index]
        $command.Title = $titleBox.Text
        $command.Key = $keyBox.Text
        $command.Action = Get-ActionFromDisplayName ([string]$actionBox.SelectedItem)
        $command.Target = $targetBox.Text
        $command.Arguments = $argumentsBox.Text
        $command.WorkingDirectory = $workingBox.Text
        $command.Icon = $iconBox.Text
        $command.IconDark = $iconDarkBox.Text
        $command.PluginMenu = $pluginCheck.Checked
        $command.ContextMenu = $contextCheck.Checked
        $command.Toolbar = $toolbarCheck.Checked
        $command.ToolbarMenu =
            $toolbarCheck.Checked -and $toolbarMenuCheck.Checked
        $toolbarMenuCheck.Enabled = $toolbarCheck.Checked
        if (-not $toolbarCheck.Checked) {
            $toolbarMenuCheck.Checked = $false
        }
        $commandList.Items[$index] = if (
            [string]::IsNullOrWhiteSpace($command.Title)) {
            Get-BuilderString 'untitled'
        } else {
            $command.Title
        }
    }

    $commandList.Add_SelectedIndexChanged({ Load-CommandEditor })
    foreach ($control in @(
            $titleBox, $keyBox, $targetBox, $argumentsBox, $workingBox,
            $iconBox, $iconDarkBox)) {
        $control.Add_TextChanged({ Save-CommandEditor })
    }
    $actionBox.Add_SelectedIndexChanged({ Save-CommandEditor })
    foreach ($control in @(
            $pluginCheck, $contextCheck, $toolbarCheck, $toolbarMenuCheck)) {
        $control.Add_CheckedChanged({ Save-CommandEditor })
    }
    $addButton.Add_Click({
        $script:Commands.Add((New-BuilderCommand))
        Refresh-CommandList ($script:Commands.Count - 1)
    })
    $scriptButton.Add_Click({
        $picked = $Salamander.ui.PickFile(
            $false, (Get-BuilderString 'selectScript'),
            'PowerShell (*.ps1)|*.ps1|All files (*.*)|*.*', '')
        if ($picked.selected) {
            $title = [System.IO.Path]::GetFileNameWithoutExtension(
                [string]$picked.path)
            $command = New-BuilderCommand $title
            $command.Action = 'powershell'
            $command.Target = [string]$picked.path
            $script:Commands.Add($command)
            Refresh-CommandList ($script:Commands.Count - 1)
        }
    })
    $removeButton.Add_Click({
        $index = $commandList.SelectedIndex
        if ($index -ge 0) {
            $script:Commands.RemoveAt($index)
            Refresh-CommandList $index
        }
    })
    $upButton.Add_Click({
        $index = $commandList.SelectedIndex
        if ($index -gt 0) {
            $item = $script:Commands[$index]
            $script:Commands.RemoveAt($index)
            $script:Commands.Insert($index - 1, $item)
            Refresh-CommandList ($index - 1)
        }
    })
    $downButton.Add_Click({
        $index = $commandList.SelectedIndex
        if ($index -ge 0 -and $index -lt $script:Commands.Count - 1) {
            $item = $script:Commands[$index]
            $script:Commands.RemoveAt($index)
            $script:Commands.Insert($index + 1, $item)
            Refresh-CommandList ($index + 1)
        }
    })
    $folderButton.Add_Click({
        $picked = $Salamander.ui.PickFolder(
            (Get-BuilderString 'selectExtensionFolder'), $folderBox.Text)
        if ($picked.selected) { $folderBox.Text = [string]$picked.path }
    })
    $workingButton.Add_Click({
        $picked = $Salamander.ui.PickFolder(
            (Get-BuilderString 'selectWorkingDirectory'), $workingBox.Text)
        if ($picked.selected) { $workingBox.Text = [string]$picked.path }
    })
    $targetButton.Add_Click({
        $picked = $Salamander.ui.PickFile(
            $false, (Get-BuilderString 'selectTarget'),
            'Programs and scripts (*.exe;*.cmd;*.bat;*.ps1)|*.exe;*.cmd;*.bat;*.ps1|All files (*.*)|*.*',
            $targetBox.Text)
        if ($picked.selected) { $targetBox.Text = [string]$picked.path }
    })
    foreach ($pair in @(
            @($iconButton, $iconBox),
            @($iconDarkButton, $iconDarkBox))) {
        $button = $pair[0]
        $box = $pair[1]
        $button.Add_Click({
            $picked = $Salamander.ui.PickFile(
                $false, (Get-BuilderString 'selectIcon'),
                'SVG icons (*.svg)|*.svg', $box.Text)
            if ($picked.selected) { $box.Text = [string]$picked.path }
        }.GetNewClosure())
    }
    $previewButton.Add_Click({
        Save-CommandEditor
        $preview = New-Object System.Windows.Forms.ContextMenuStrip
        $preview.ImageScalingSize = New-Object System.Drawing.Size(16, 16)
        $previewImages = New-Object System.Collections.Generic.List[System.Drawing.Image]
        foreach ($command in $script:Commands) {
            if (-not $command.PluginMenu) { continue }
            $item = New-Object System.Windows.Forms.ToolStripMenuItem
            $item.Text = $command.Title
            $item.Enabled = -not [string]::IsNullOrWhiteSpace($command.Target)
            $previewImage = Get-BuilderPreviewImage $command
            if ($null -ne $previewImage) {
                $previewImages.Add($previewImage)
                $item.Image = $previewImage
            }
            [void]$preview.Items.Add($item)
        }
        if ($preview.Items.Count -eq 0) {
            $item = New-Object System.Windows.Forms.ToolStripMenuItem
            $item.Text = Get-BuilderString 'noPreviewItems'
            $item.Enabled = $false
            [void]$preview.Items.Add($item)
        }
        $preview.Add_Closed({
            foreach ($image in $previewImages) { $image.Dispose() }
            $preview.Dispose()
        }.GetNewClosure())
        $preview.Show($previewButton, 0, $previewButton.Height)
    })
    $openButton.Add_Click({
        $picked = $Salamander.ui.PickFile(
            $false, (Get-BuilderString 'selectManifest'),
            'Salamatrix extension (extension.json)|extension.json|JSON files (*.json)|*.json',
            $folderBox.Text)
        if ($picked.selected) {
            try {
                $loaded = Load-BuilderProject ([string]$picked.path)
                $nameBox.Text = $loaded.Name
                $idBox.Text = $loaded.Id
                $descriptionBox.Text = $loaded.Description
                $folderBox.Text = $loaded.Folder
                $script:ManifestOnlyMode = -not $loaded.Generated
                $script:ImportedManifest = $loaded.Manifest
                $script:ImportedExtensionFolder = $loaded.Folder
                Refresh-CommandList 0
                if (-not $loaded.Generated) {
                    [void]$Salamander.ui.MessageBox(
                        (Get-BuilderString 'importedReadOnlyActions'),
                        (Get-BuilderString 'title'), 'OK', 'Information')
                }
            } catch {
                [void]$Salamander.ui.MessageBox(
                    $_.Exception.Message, (Get-BuilderString 'title'),
                    'OK', 'Error')
            }
        }
    })
    $saveButton.Add_Click({
        Save-CommandEditor
        try {
            if (Save-BuilderProject $nameBox.Text $idBox.Text (
                    $descriptionBox.Text) $folderBox.Text) {
                [void]$Salamander.ui.MessageBox(
                    ((Get-BuilderString 'saved') -f $folderBox.Text),
                    (Get-BuilderString 'title'), 'OK', 'Information')
            }
        } catch {
            [void]$Salamander.ui.MessageBox(
                $_.Exception.Message, (Get-BuilderString 'title'),
                'OK', 'Error')
        }
    })
    $closeButton.Add_Click({ $form.Close() })

    Refresh-CommandList 0
    Set-BuilderDarkMode $form
    try { [void]$form.ShowDialog() } finally { $form.Dispose() }
}

if ($null -eq (Get-Variable -Name Salamander -ErrorAction SilentlyContinue)) {
    return
}

if ($Salamander.command_handler -eq 'open') {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    try {
        $appearance = $Salamander.application.Appearance()
        $dark = $appearance.PSObject.Properties['windowsDarkMode']
        $script:UseWindowsDarkMode =
            $null -ne $dark -and [bool]$dark.Value
        if ($script:UseWindowsDarkMode) {
            Initialize-BuilderDarkMode
            [OpenSalamander.ExtensionMenuBuilder.DarkMode]::EnableApplication()
        }
        [System.Windows.Forms.Application]::EnableVisualStyles()
        $language = $Salamander.application.Language()
        $script:Strings = Get-BuilderStrings ([string]$language.locale)
        Show-BuilderWindow
    }
    catch {
        [void]$Salamander.ui.MessageBox(
            $_.Exception.Message, 'Extension Menu Builder', 'OK', 'Error')
    }
}
