# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EntryPoint,
    [string]$CommandId = '',
    [string]$CommandHandler = '',
    [string]$InvocationJson = '{}',
    [switch]$OneShot
)

$ErrorActionPreference = 'Stop'
$global:SalamatrixNextId = 1
$global:SalamatrixEventHandlers = @{}

function Send-Frame {
    param([string]$Kind, [UInt64]$Id, [hashtable]$Payload)
    $json = $Payload | ConvertTo-Json -Compress -Depth 20
    $frame = "SMX1`t$Kind`t$Id`t$json"
    if ([Text.Encoding]::UTF8.GetByteCount($frame) + 1 -gt 1048576) { throw 'SMX1 frame exceeds the 1 MiB limit' }
    [Console]::Out.WriteLine($frame)
    [Console]::Out.Flush()
}

function Read-Frame {
    $line = [Console]::In.ReadLine()
    if ($null -eq $line) { throw 'Salamander host closed the worker channel' }
    if ([Text.Encoding]::UTF8.GetByteCount($line) + 1 -gt 1048576) { throw 'SMX1 frame exceeds the 1 MiB limit' }
    $parts = $line.Split("`t", 4)
    if ($parts.Count -ne 4 -or $parts[0] -ne 'SMX1') { throw 'Invalid SMX1 frame' }
    [pscustomobject]@{
        Kind = $parts[1]
        Id = [UInt64]::Parse($parts[2], [Globalization.CultureInfo]::InvariantCulture)
        Payload = $parts[3] | ConvertFrom-Json
    }
}

function Invoke-Event {
    param($Payload)
    $name = [string]$Payload.event
    if ($global:SalamatrixEventHandlers.ContainsKey($name)) {
        foreach ($handler in @($global:SalamatrixEventHandlers[$name])) { & $handler $Payload }
    }
}

function Invoke-Host {
    param([string]$Method, [hashtable]$Arguments)
    $payload = @{ method = $Method }
    foreach ($key in $Arguments.Keys) { $payload[$key] = $Arguments[$key] }
    $id = $global:SalamatrixNextId++
    Send-Frame -Kind 'call' -Id $id -Payload $payload
    while ($true) {
        $frame = Read-Frame
        if ($frame.Kind -eq 'event') { Invoke-Event $frame.Payload; continue }
        if ($frame.Id -ne $id) { continue }
        if ($frame.Kind -eq 'error') { throw [string]$frame.Payload.error }
        $okProperty = $frame.Payload.PSObject.Properties['ok']
        if ($null -ne $okProperty -and $okProperty.Value -eq $false) {
            throw [string]$frame.Payload.error
        }
        if ($frame.Kind -ne 'result') { throw "Unexpected SMX1 response: $($frame.Kind)" }
        return $frame.Payload
    }
}

Send-Frame -Kind 'hello' -Id 0 -Payload @{ protocol = 1; runtime = 'powershell' }
do { $hello = Read-Frame } while ($hello.Kind -ne 'result' -or $hello.Id -ne 0)
if ($hello.Payload.ok -eq $false) { throw 'Salamander host rejected the worker' }

function Initialize-SalamatrixWindowIcon {
    try {
        $response = Invoke-Host -Method 'salamander.host.windowIcon' -Arguments @{}
        if ($null -eq $response -or
            [string]::IsNullOrWhiteSpace([string]$response.icon)) {
            return
        }

        Add-Type -AssemblyName System.Drawing
        Add-Type -AssemblyName System.Windows.Forms
        if ($null -eq ('OpenSalamander.Salamatrix.ExtensionWindowIconFilter' -as [type])) {
            $filterSource = @'
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace OpenSalamander.Salamatrix
{
    public sealed class ExtensionWindowIconFilter : IMessageFilter, IDisposable
    {
        private const int GA_ROOT = 2;
        private const int WM_SETICON = 0x0080;
        private const int ICON_SMALL = 0;
        private const int ICON_BIG = 1;

        private readonly Icon icon;
        private IntPtr[] initializedWindows = new IntPtr[8];
        private int initializedWindowCount;

        [DllImport("user32.dll")]
        private static extern IntPtr GetAncestor(IntPtr hwnd, uint flags);

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(
            IntPtr hwnd, out uint processId);

        [DllImport("kernel32.dll")]
        private static extern uint GetCurrentProcessId();

        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(
            IntPtr hwnd, int message, IntPtr wParam, IntPtr lParam);

        public ExtensionWindowIconFilter(Icon windowIcon)
        {
            icon = (Icon)windowIcon.Clone();
        }

        public bool PreFilterMessage(ref Message message)
        {
            IntPtr root = GetAncestor(message.HWnd, GA_ROOT);
            if (root == IntPtr.Zero || IsInitialized(root))
                return false;

            uint processId;
            GetWindowThreadProcessId(root, out processId);
            if (processId != GetCurrentProcessId())
                return false;

            SendMessage(root, WM_SETICON, new IntPtr(ICON_BIG), icon.Handle);
            SendMessage(root, WM_SETICON, new IntPtr(ICON_SMALL), icon.Handle);
            if (initializedWindowCount == initializedWindows.Length)
                Array.Resize(ref initializedWindows, initializedWindows.Length * 2);
            initializedWindows[initializedWindowCount++] = root;
            return false;
        }

        private bool IsInitialized(IntPtr hwnd)
        {
            for (int index = 0; index < initializedWindowCount; ++index)
                if (initializedWindows[index] == hwnd)
                    return true;
            return false;
        }

        public void Dispose()
        {
            Application.RemoveMessageFilter(this);
            icon.Dispose();
        }
    }
}
'@
            if ($PSVersionTable.PSEdition -eq 'Core') {
                $references = @(
                    [System.Drawing.Icon].Assembly.Location
                    [System.Windows.Forms.Form].Assembly.Location
                    [System.Windows.Forms.Message].Assembly.Location
                ) | Select-Object -Unique
                Add-Type -ReferencedAssemblies $references -TypeDefinition $filterSource
            } else {
                Add-Type -ReferencedAssemblies 'System.Drawing', 'System.Windows.Forms' -TypeDefinition $filterSource
            }
        }

        $bytes = [Convert]::FromBase64String([string]$response.icon)
        $stream = New-Object System.IO.MemoryStream -ArgumentList @(,$bytes)
        try {
            $sourceIcon = New-Object System.Drawing.Icon -ArgumentList $stream
            try {
                $filter = New-Object OpenSalamander.Salamatrix.ExtensionWindowIconFilter -ArgumentList $sourceIcon
                [System.Windows.Forms.Application]::AddMessageFilter($filter)
                $global:SalamatrixWindowIconFilter = $filter
            } finally {
                $sourceIcon.Dispose()
            }
        } finally {
            $stream.Dispose()
        }
    } catch {
        # Window icon decoration is optional and must never block an extension.
    }
}

Initialize-SalamatrixWindowIcon

$commands = [pscustomobject]@{}
$commands | Add-Member ScriptMethod Execute {
    param([string]$CommandId)
    (Invoke-Host -Method 'salamander.commands.execute' -Arguments @{ commandId = $CommandId }).result
}
$commands | Add-Member ScriptMethod Register {
    param([string]$CommandId, [string]$Title, [bool]$PluginMenu = $true, [bool]$ContextMenu = $false, [int]$HotKey = 0, [bool]$Toolbar = $false, [string]$Handler = '', [bool]$Enabled = $true, [bool]$Visible = $true)
    (Invoke-Host -Method 'salamander.commands.register' -Arguments @{ commandId = $CommandId; title = $Title; pluginMenu = $PluginMenu; contextMenu = $ContextMenu; hotKey = $HotKey; toolbar = $Toolbar; handler = $Handler; enabled = $Enabled; visible = $Visible }).registered
}
$commands | Add-Member ScriptMethod Unregister {
    param([string]$CommandId)
    (Invoke-Host -Method 'salamander.commands.unregister' -Arguments @{ commandId = $CommandId }).unregistered
}
$commands | Add-Member ScriptMethod SetState {
    param([string]$CommandId, [Nullable[bool]]$Enabled = $null, [Nullable[bool]]$Visible = $null)
    $arguments = @{ commandId = $CommandId }
    if ($null -ne $Enabled) { $arguments.enabled = [bool]$Enabled }
    if ($null -ne $Visible) { $arguments.visible = [bool]$Visible }
    (Invoke-Host -Method 'salamander.commands.setState' -Arguments $arguments).updated
}
$storage = [pscustomobject]@{}
$storage | Add-Member ScriptMethod Get {
    param([string]$Key, [object]$Default = $null)
    $result = Invoke-Host -Method 'salamander.storage.get' -Arguments @{ key = $Key }
    if ($result.type -eq 'string' -or $result.type -eq 'integer' -or $result.type -eq 'boolean') { return $result.value }
    return $Default
}
$storage | Add-Member ScriptMethod Set {
    param([string]$Key, [object]$Value)
    [void](Invoke-Host -Method 'salamander.storage.set' -Arguments @{ key = $Key; value = $Value })
}
$storage | Add-Member ScriptMethod Remove {
    param([string]$Key)
    (Invoke-Host -Method 'salamander.storage.remove' -Arguments @{ key = $Key }).removed
}
$storage | Add-Member ScriptMethod Clear {
    (Invoke-Host -Method 'salamander.storage.clear' -Arguments @{}).ok
}
$storage | Add-Member ScriptMethod Schema {
    @((Invoke-Host -Method 'salamander.storage.schema' -Arguments @{}).settings)
}
$storage | Add-Member ScriptMethod Keys {
    $result = Invoke-Host -Method 'salamander.storage.keys' -Arguments @{}
    if ($null -ne $result -and $result.keys -is [array]) { return $result.keys }
    return @()
}
$fileOperations = [pscustomobject]@{}
$fileOperations | Add-Member ScriptMethod Rename { (Invoke-Host -Method 'salamander.fileOperations.rename' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Copy { (Invoke-Host -Method 'salamander.fileOperations.copy' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Move { (Invoke-Host -Method 'salamander.fileOperations.move' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Delete { (Invoke-Host -Method 'salamander.fileOperations.delete' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod CreateDirectory { (Invoke-Host -Method 'salamander.fileOperations.createDirectory' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Refresh { (Invoke-Host -Method 'salamander.fileOperations.refresh' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Properties { (Invoke-Host -Method 'salamander.fileOperations.properties' -Arguments @{}).result }
$fileSystem = [pscustomobject]@{}
$fileSystem | Add-Member ScriptMethod AddItem {
    param([string]$Id, [string]$Name, [hashtable]$Options = @{})
    $arguments = @{ id = $Id; name = $Name }
    foreach ($key in $Options.Keys) { $arguments[$key] = $Options[$key] }
    [bool](Invoke-Host -Method 'salamander.fileSystem.addItem' -Arguments $arguments).added
}
$sides = [pscustomobject]@{}
$sides | Add-Member ScriptMethod ActiveTab {
    param([string]$Side = 'source')
    Invoke-Host -Method 'salamander.sides.activeTab' -Arguments @{ side = $Side }
}
$sides | Add-Member ScriptMethod Context {
    param([string]$Side = 'source')
    Invoke-Host -Method 'salamander.sides.context' -Arguments @{ side = $Side }
}
$sides | Add-Member ScriptMethod Tabs {
    param([string]$Side = 'source')
    (Invoke-Host -Method 'salamander.sides.tabs' -Arguments @{ side = $Side }).tabs
}
$sides | Add-Member ScriptMethod ActivateTab {
    param([string]$TabId, [bool]$Focus = $true)
    (Invoke-Host -Method 'salamander.sides.activateTab' -Arguments @{ tabId = $TabId; focus = $Focus }).activated
}
$sides | Add-Member ScriptMethod ChangePath {
    param([string]$Path, [string]$Side = 'source')
    Invoke-Host -Method 'salamander.sides.changePath' -Arguments @{ side = $Side; path = $Path }
}
$sides | Add-Member ScriptMethod Refresh {
    param([string]$Side = 'source', [bool]$Force = $false, [bool]$FocusFirstNewItem = $false)
    (Invoke-Host -Method 'salamander.sides.refresh' -Arguments @{ side = $Side; force = $Force; focusFirstNewItem = $FocusFirstNewItem }).ok
}
$sides | Add-Member ScriptMethod SelectItem {
    param([int]$Index, [bool]$Select = $true, [string]$Side = 'source', [bool]$Repaint = $true)
    (Invoke-Host -Method 'salamander.sides.selectItem' -Arguments @{ side = $Side; index = $Index; select = $Select; repaint = $Repaint }).changed
}
$sides | Add-Member ScriptMethod SelectAll {
    param([bool]$Select = $true, [string]$Side = 'source', [bool]$Repaint = $true)
    (Invoke-Host -Method 'salamander.sides.selectAll' -Arguments @{ side = $Side; select = $Select; repaint = $Repaint }).changed
}
$sides | Add-Member ScriptMethod FocusItem {
    param([int]$Index, [string]$Side = 'source', [bool]$PartVisible = $true)
    (Invoke-Host -Method 'salamander.sides.focusItem' -Arguments @{ side = $Side; index = $Index; partVisible = $PartVisible }).changed
}
$sides | Add-Member ScriptMethod CreateTab {
    param([string]$Side = 'source', [AllowNull()][string]$Path = $null, [Nullable[int]]$Index = $null)
    $args = @{ side = $Side; path = $Path }; if ($null -ne $Index) { $args.index = $Index.Value }
    Invoke-Host -Method 'salamander.sides.createTab' -Arguments $args
}
$sides | Add-Member ScriptMethod CloseTab { param([string]$TabId); (Invoke-Host -Method 'salamander.sides.closeTab' -Arguments @{ tabId = [string]$TabId }).ok }
$sides | Add-Member ScriptMethod ReorderTab { param([string]$TabId, [int]$Index); (Invoke-Host -Method 'salamander.sides.reorderTab' -Arguments @{ tabId = [string]$TabId; index = $Index }).ok }
$sides | Add-Member ScriptMethod MoveTab { param([string]$TabId, [string]$Side = 'source', [Nullable[int]]$Index = $null); $args = @{ tabId = [string]$TabId; side = $Side }; if ($null -ne $Index) { $args.index = $Index.Value }; (Invoke-Host -Method 'salamander.sides.moveTab' -Arguments $args).ok }
$sides | Add-Member ScriptMethod SetDetached { param([bool]$Detached); (Invoke-Host -Method 'salamander.sides.setDetached' -Arguments @{ detached = $Detached }).ok }
function New-SalamatrixSideView([string]$SideName) {
    $view = [pscustomobject]@{ Side = $SideName }
    $view | Add-Member ScriptMethod ActiveTab { Invoke-Host -Method 'salamander.sides.activeTab' -Arguments @{ side = $this.Side } }
    $view | Add-Member ScriptMethod Context { Invoke-Host -Method 'salamander.sides.context' -Arguments @{ side = $this.Side } }
    $view | Add-Member ScriptMethod Tabs { (Invoke-Host -Method 'salamander.sides.tabs' -Arguments @{ side = $this.Side }).tabs }
    $view | Add-Member ScriptMethod ActivateTab { param([string]$TabId, [bool]$Focus = $true); (Invoke-Host -Method 'salamander.sides.activateTab' -Arguments @{ tabId = $TabId; focus = $Focus }).activated }
    $view | Add-Member ScriptMethod ChangePath { param([string]$Path); Invoke-Host -Method 'salamander.sides.changePath' -Arguments @{ side = $this.Side; path = $Path } }
    $view | Add-Member ScriptMethod Refresh { param([bool]$Force = $false, [bool]$FocusFirstNewItem = $false); (Invoke-Host -Method 'salamander.sides.refresh' -Arguments @{ side = $this.Side; force = $Force; focusFirstNewItem = $FocusFirstNewItem }).ok }
    $view | Add-Member ScriptMethod SelectItem { param([int]$Index, [bool]$Select = $true, [bool]$Repaint = $true); (Invoke-Host -Method 'salamander.sides.selectItem' -Arguments @{ side = $this.Side; index = $Index; select = $Select; repaint = $Repaint }).changed }
    $view | Add-Member ScriptMethod SelectAll { param([bool]$Select = $true, [bool]$Repaint = $true); (Invoke-Host -Method 'salamander.sides.selectAll' -Arguments @{ side = $this.Side; select = $Select; repaint = $Repaint }).changed }
    $view | Add-Member ScriptMethod FocusItem { param([int]$Index, [bool]$PartVisible = $true); (Invoke-Host -Method 'salamander.sides.focusItem' -Arguments @{ side = $this.Side; index = $Index; partVisible = $PartVisible }).changed }
    $view | Add-Member ScriptMethod CreateTab { param([AllowNull()][string]$Path = $null, [Nullable[int]]$Index = $null); $args = @{ side = $this.Side; path = $Path }; if ($null -ne $Index) { $args.index = $Index.Value }; Invoke-Host -Method 'salamander.sides.createTab' -Arguments $args }
    $view | Add-Member ScriptMethod CloseTab { param([string]$TabId); (Invoke-Host -Method 'salamander.sides.closeTab' -Arguments @{ tabId = [string]$TabId }).ok }
    $view | Add-Member ScriptMethod ReorderTab { param([string]$TabId, [int]$Index); (Invoke-Host -Method 'salamander.sides.reorderTab' -Arguments @{ tabId = [string]$TabId; index = $Index }).ok }
    $view | Add-Member ScriptMethod MoveTab { param([string]$TabId, [string]$Side = $this.Side, [Nullable[int]]$Index = $null); $args = @{ tabId = [string]$TabId; side = $Side }; if ($null -ne $Index) { $args.index = $Index.Value }; (Invoke-Host -Method 'salamander.sides.moveTab' -Arguments $args).ok }
    $view | Add-Member ScriptMethod SetDetached { param([bool]$Detached); (Invoke-Host -Method 'salamander.sides.setDetached' -Arguments @{ detached = $Detached }).ok }
    return $view
}
$leftSide = New-SalamatrixSideView 'left'
$rightSide = New-SalamatrixSideView 'right'
$sourceSide = New-SalamatrixSideView 'source'
$targetSide = New-SalamatrixSideView 'target'
$ui = [pscustomobject]@{}
$ui | Add-Member ScriptMethod MessageBox {
    param(
        [string]$Message,
        [string]$Title = 'Salamander',
        [string]$Buttons = 'OK',
        [string]$Icon = 'Information'
    )
    (Invoke-Host -Method 'salamander.ui.messageBox' -Arguments @{
        message = $Message
        title = $Title
        buttons = $Buttons
        icon = $Icon
    }).result
}
$ui | Add-Member ScriptMethod Notify {
    param([string]$Message, [string]$Title = 'Salamander', [int]$TimeoutMs = 5000)
    (Invoke-Host -Method 'salamander.ui.notify' -Arguments @{ message = $Message; title = $Title; timeoutMs = [Math]::Max(0, $TimeoutMs) }).shown
}
$ui | Add-Member ScriptMethod Controls {
    (Invoke-Host -Method 'salamander.ui.controls' -Arguments @{}).shown
}
$ui | Add-Member ScriptMethod Uptime { [string](Invoke-Host -Method 'salamander.host.uptime' -Arguments @{}).milliseconds }
$ui | Add-Member ScriptMethod InputBox {
    param([string]$Prompt, [string]$Title = 'Salamander', [string]$Initial = '')
    Invoke-Host -Method 'salamander.ui.inputBox' -Arguments @{ prompt = $Prompt; title = $Title; initial = $Initial }
}
$ui | Add-Member ScriptMethod PickFile {
    param([bool]$Save = $false, [string]$Title = '', [string]$Filter = '', [string]$Initial = '')
    Invoke-Host -Method 'salamander.ui.pickFile' -Arguments @{ save = $Save; title = $Title; filter = $Filter; initial = $Initial }
}
$ui | Add-Member ScriptMethod PickFolder {
    param([string]$Title = '', [string]$Initial = '')
    Invoke-Host -Method 'salamander.ui.pickFolder' -Arguments @{ title = $Title; initial = $Initial }
}
$ui | Add-Member ScriptMethod Progress {
    param([string]$Title = 'Salamatrix', [int]$Total = 0, [bool]$TwoProgressBars = $false, [bool]$FileProgress = $false, [bool]$CancelEnabled = $true, [int]$Total2 = -1)
    $arguments = @{ title = $Title; total = $Total; twoProgressBars = $TwoProgressBars; fileProgress = $FileProgress; cancelEnabled = $CancelEnabled }
    if ($Total2 -ge 0) { $arguments['total2'] = $Total2 }
    $created = Invoke-Host -Method 'salamander.ui.progress.create' -Arguments $arguments
    $progress = [pscustomobject]@{ ProgressId = [string]$created.progressId; Closed = $false }
    $progress | Add-Member ScriptMethod Update {
        param([int]$Position, [int]$Total = -1, [string]$Text = '', [bool]$DelayedPaint = $true, [int]$Position2 = -1, [int]$Total2 = -1)
        $arguments = @{ progressId = $this.ProgressId; position = $Position; text = $Text; delayedPaint = $DelayedPaint }
        if ($Total -ge 0) { $arguments['total'] = $Total }
        if ($Position2 -ge 0) { $arguments['position2'] = $Position2 }
        if ($Total2 -ge 0) { $arguments['total2'] = $Total2 }
        (Invoke-Host -Method 'salamander.ui.progress.update' -Arguments $arguments).continued
    }
    $progress | Add-Member ScriptMethod Step {
        param([int]$Amount = 1, [bool]$DelayedPaint = $true)
        (Invoke-Host -Method 'salamander.ui.progress.step' -Arguments @{ progressId = $this.ProgressId; amount = $Amount; delayedPaint = $DelayedPaint }).continued
    }
    $progress | Add-Member ScriptMethod SetTotals {
        param([int]$Total, [int]$Total2)
        [void](Invoke-Host -Method 'salamander.ui.progress.setTotals' -Arguments @{ progressId = $this.ProgressId; total = $Total; total2 = $Total2 })
    }
    $progress | Add-Member ScriptMethod SetPositions {
        param([int]$Position, [int]$Position2, [bool]$DelayedPaint = $true)
        (Invoke-Host -Method 'salamander.ui.progress.setPositions' -Arguments @{ progressId = $this.ProgressId; position = $Position; position2 = $Position2; delayedPaint = $DelayedPaint }).continued
    }
    $progress | Add-Member ScriptMethod SetTitle {
        param([string]$Title)
        [void](Invoke-Host -Method 'salamander.ui.progress.setTitle' -Arguments @{ progressId = $this.ProgressId; title = $Title })
    }
    $progress | Add-Member ScriptMethod SetCancelEnabled {
        param([bool]$Enabled)
        [void](Invoke-Host -Method 'salamander.ui.progress.setCancelEnabled' -Arguments @{ progressId = $this.ProgressId; enabled = $Enabled })
    }
    $progress | Add-Member ScriptMethod IsCancelled {
        (Invoke-Host -Method 'salamander.ui.progress.cancelled' -Arguments @{ progressId = $this.ProgressId }).cancelled
    }
    $progress | Add-Member ScriptMethod Close {
        if (-not $this.Closed) {
            [void](Invoke-Host -Method 'salamander.ui.progress.close' -Arguments @{ progressId = $this.ProgressId })
            $this.Closed = $true
        }
    }
    return $progress
}
$ui | Add-Member ScriptMethod Dialog {
    param([string]$Title = 'Salamander', [int]$Width = 320, [int]$Height = 180)
    $created = Invoke-Host -Method 'salamander.ui.dialog.create' -Arguments @{ title = $Title; width = $Width; height = $Height }
    $dialog = [pscustomobject]@{ DialogId = [string]$created.dialogId }
    $dialog | Add-Member ScriptMethod AddControl {
        param([string]$Kind, [string]$Id, [string]$Text = '', [bool]$ReadOnly = $false, [bool]$Checked = $false, [int]$DialogResult = 0, [hashtable]$Layout = $null, [bool]$KeepOpen = $false, [bool]$Multiline = $false, [hashtable]$Options = $null)
        $arguments = @{ dialogId = $this.DialogId; kind = $Kind; controlId = $Id; text = $Text; readOnly = $ReadOnly; checked = $Checked; dialogResult = $DialogResult; keepOpen = $KeepOpen; multiline = $Multiline }
        if ($null -ne $Layout) {
            foreach ($name in @('x', 'y', 'width', 'height')) {
                if ($Layout.ContainsKey($name)) { $arguments[$name] = [int]$Layout[$name] }
            }
        }
        if ($null -ne $Options) { foreach ($name in $Options.Keys) { $arguments[$name] = $Options[$name] } }
        [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments $arguments)
    }
    $dialog | Add-Member ScriptMethod SetValidation {
        param([string]$Id, [bool]$Required = $false, [string]$Message = '')
        [void](Invoke-Host -Method 'salamander.ui.dialog.validation' -Arguments @{ dialogId = $this.DialogId; controlId = $Id; required = $Required; message = $Message })
    }
    $dialog | Add-Member ScriptMethod OnChange {
        param([scriptblock]$Handler)
        $eventName = "salamander.ui.dialog.$($this.DialogId).changed"
        if (-not $global:SalamatrixEventHandlers.ContainsKey($eventName)) { $global:SalamatrixEventHandlers[$eventName] = @() }
        $global:SalamatrixEventHandlers[$eventName] += $Handler
        [void](Invoke-Host -Method 'salamander.ui.dialog.events' -Arguments @{ dialogId = $this.DialogId; enabled = $true; event = $eventName })
        return $eventName
    }
    $dialog | Add-Member ScriptMethod AddColumn {
        param([string]$ControlId, [string]$Title, [int]$Width = 180)
        [void](Invoke-Host -Method 'salamander.ui.dialog.column' -Arguments @{ dialogId = $this.DialogId; controlId = $ControlId; title = $Title; width = $Width })
    }
    $dialog | Add-Member ScriptMethod SetSelectedIndex {
        param([string]$ControlId, [int]$Index = -1)
        (Invoke-Host -Method 'salamander.ui.dialog.selection' -Arguments @{ dialogId = $this.DialogId; controlId = $ControlId; index = $Index }).selectedIndex
    }
    $dialog | Add-Member ScriptMethod OffChange {
        param([string]$EventName = '')
        if ([string]::IsNullOrEmpty($EventName)) { $EventName = "salamander.ui.dialog.$($this.DialogId).changed" }
        [void](Invoke-Host -Method 'salamander.ui.dialog.events' -Arguments @{ dialogId = $this.DialogId; enabled = $false; event = $EventName })
        $global:SalamatrixEventHandlers.Remove($EventName)
    }
    $dialog | Add-Member ScriptMethod AddLabel { param([string]$Id, [string]$Text) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'label'; controlId = $Id; text = $Text }) }
    $dialog | Add-Member ScriptMethod AddTextBox { param([string]$Id, [string]$Text = '', [bool]$ReadOnly = $false, [bool]$Multiline = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'textbox'; controlId = $Id; text = $Text; readOnly = $ReadOnly; multiline = $Multiline }) }
    $dialog | Add-Member ScriptMethod AddFolderPicker { param([string]$Id, [string]$Path = '') [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'folderpicker'; controlId = $Id; text = $Path }) }
    $dialog | Add-Member ScriptMethod AddFilePicker { param([string]$Id, [string]$Path = '', [string]$Filter = '', [bool]$Save = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'filepicker'; controlId = $Id; text = $Path; filter = $Filter; save = $Save }) }
    $dialog | Add-Member ScriptMethod AddCheckBox { param([string]$Id, [string]$Text, [bool]$Checked = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'checkbox'; controlId = $Id; text = $Text; checked = $Checked }) }
    $dialog | Add-Member ScriptMethod AddRadioButton { param([string]$Id, [string]$Text, [bool]$Checked = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'radio'; controlId = $Id; text = $Text; checked = $Checked }) }
    $dialog | Add-Member ScriptMethod AddComboBox { param([string]$Id, [string]$Text = '') [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'combobox'; controlId = $Id; text = $Text }) }
    $dialog | Add-Member ScriptMethod AddListView { param([string]$Id) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'listview'; controlId = $Id }) }
    $dialog | Add-Member ScriptMethod AddTreeView { param([string]$Id) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'treeview'; controlId = $Id }) }
    $dialog | Add-Member ScriptMethod AddTabControl { param([string]$Id) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'tabcontrol'; controlId = $Id }) }
    $dialog | Add-Member ScriptMethod AddItem { param([string]$ControlId, [string]$Text, [int]$ParentIndex = -1) (Invoke-Host -Method 'salamander.ui.dialog.item' -Arguments @{ dialogId = $this.DialogId; controlId = $ControlId; text = $Text; parentIndex = $ParentIndex }).itemCount }
    $dialog | Add-Member ScriptMethod ClearItems { param([string]$ControlId) [void](Invoke-Host -Method 'salamander.ui.dialog.clearItems' -Arguments @{ dialogId = $this.DialogId; controlId = $ControlId }) }
    $dialog | Add-Member ScriptMethod AddButton { param([string]$Id, [string]$Text, [int]$DialogResult = 1, [bool]$KeepOpen = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'button'; controlId = $Id; text = $Text; dialogResult = $DialogResult; keepOpen = $KeepOpen }) }
    $dialog | Add-Member ScriptMethod Show { (Invoke-Host -Method 'salamander.ui.dialog.show' -Arguments @{ dialogId = $this.DialogId }).result }
    $dialog | Add-Member ScriptMethod Get { param([string]$Id) Invoke-Host -Method 'salamander.ui.dialog.get' -Arguments @{ dialogId = $this.DialogId; controlId = $Id } }
    $dialog | Add-Member ScriptMethod Set { param([string]$Id, [string]$Value) [void](Invoke-Host -Method 'salamander.ui.dialog.set' -Arguments @{ dialogId = $this.DialogId; controlId = $Id; value = $Value }) }
    $dialog | Add-Member ScriptMethod Close { [void](Invoke-Host -Method 'salamander.ui.dialog.destroy' -Arguments @{ dialogId = $this.DialogId }) }
    return $dialog
}
$clipboard = [pscustomobject]@{}
$clipboard | Add-Member ScriptMethod CopyText {
    param([string]$Text, [bool]$ShowEcho = $false)
    (Invoke-Host -Method 'salamander.clipboard.copyText' -Arguments @{ text = $Text; showEcho = $ShowEcho }).copied
}
$ai = [pscustomobject]@{}
$ai | Add-Member ScriptMethod Api {
    param([string]$Topic = $null)
    $arguments = @{}
    if (-not [string]::IsNullOrEmpty($Topic)) { $arguments['topic'] = $Topic }
    Invoke-Host -Method 'salamander.ai.api' -Arguments $arguments
}
$ai | Add-Member ScriptMethod ApiDescription {
    param([string]$Topic = $null)
    $this.Api($Topic)
}
$ai | Add-Member ScriptMethod Generate {
    param([string]$Prompt, [object]$Context = $null, [string]$Provider = $null, [string]$Runtime = $null, [string]$ExistingScript = $null, [string]$Feedback = $null)
    $arguments = @{ prompt = $Prompt }
    if ($null -ne $Context) { $arguments['context'] = $Context }
    if (-not [string]::IsNullOrEmpty($Provider)) { $arguments['provider'] = $Provider }
    if (-not [string]::IsNullOrEmpty($Runtime)) { $arguments['runtime'] = $Runtime }
    if ($null -ne $ExistingScript) { $arguments['existingScript'] = $ExistingScript }
    if ($null -ne $Feedback) { $arguments['feedback'] = $Feedback }
    Invoke-Host -Method 'salamander.ai.generate' -Arguments $arguments
}
$ai | Add-Member ScriptMethod Preview {
    param([string]$Prompt, [object]$Context = $null, [string]$Provider = $null, [string]$Runtime = $null, [string]$ExistingScript = $null, [string]$Feedback = $null)
    $arguments = @{ prompt = $Prompt }
    if ($null -ne $Context) { $arguments['context'] = $Context }
    if (-not [string]::IsNullOrEmpty($Provider)) { $arguments['provider'] = $Provider }
    if (-not [string]::IsNullOrEmpty($Runtime)) { $arguments['runtime'] = $Runtime }
    if ($null -ne $ExistingScript) { $arguments['existingScript'] = $ExistingScript }
    if ($null -ne $Feedback) { $arguments['feedback'] = $Feedback }
    Invoke-Host -Method 'salamander.ai.preview' -Arguments $arguments
}
$events = [pscustomobject]@{}
$events | Add-Member ScriptMethod Subscribe {
    param([string]$Event, [scriptblock]$Handler)
    if (-not $global:SalamatrixEventHandlers.ContainsKey($Event)) { $global:SalamatrixEventHandlers[$Event] = @() }
    $global:SalamatrixEventHandlers[$Event] += $Handler
    [string](Invoke-Host -Method 'salamander.events.subscribe' -Arguments @{ event = $Event }).subscriptionId
}
$events | Add-Member ScriptMethod Unsubscribe {
    param([string]$SubscriptionId)
    [void](Invoke-Host -Method 'salamander.events.unsubscribe' -Arguments @{ subscriptionId = $SubscriptionId })
}
$runtimes = [pscustomobject]@{}
$runtimes | Add-Member ScriptMethod List {
    (Invoke-Host -Method 'salamander.runtimes.list' -Arguments @{}).runtimes
}
$application = [pscustomobject]@{}
$application | Add-Member ScriptMethod Language {
    Invoke-Host -Method 'salamander.host.language' -Arguments @{}
}
$application | Add-Member ScriptMethod Appearance {
    Invoke-Host -Method 'salamander.host.appearance' -Arguments @{}
}

$invocation = $InvocationJson | ConvertFrom-Json
if ($null -eq $invocation -or $invocation -is [System.Array]) {
    throw '-InvocationJson must contain a JSON object'
}

$Salamander = [pscustomobject]@{
    command_id = $CommandId
    command_handler = $CommandHandler
    invocation = $invocation
    commands = $commands
    storage = $storage
    file_operations = $fileOperations
    file_system = $fileSystem
    sides = $sides
    left_side = $leftSide
    right_side = $rightSide
    source_side = $sourceSide
    target_side = $targetSide
    ui = $ui
    clipboard = $clipboard
    ai = $ai
    events = $events
    runtimes = $runtimes
    application = $application
}

# stdout is reserved for SMX1 frames; discard unassigned PowerShell return
# values from extension scripts so they cannot corrupt the transport.
& $EntryPoint | Out-Null
if ($OneShot) { exit 0 }
while ($true) {
    $frame = Read-Frame
    if ($frame.Kind -eq 'event') { Invoke-Event $frame.Payload; continue }
    if ($frame.Kind -eq 'shutdown') { break }
}
