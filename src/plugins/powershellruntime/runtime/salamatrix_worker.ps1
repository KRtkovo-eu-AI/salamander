# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EntryPoint,
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
        if ($frame.Kind -eq 'error' -or $frame.Payload.ok -eq $false) { throw [string]$frame.Payload.error }
        if ($frame.Kind -ne 'result') { throw "Unexpected SMX1 response: $($frame.Kind)" }
        return $frame.Payload
    }
}

Send-Frame -Kind 'hello' -Id 0 -Payload @{ protocol = 1; runtime = 'powershell' }
do { $hello = Read-Frame } while ($hello.Kind -ne 'result' -or $hello.Id -ne 0)
if ($hello.Payload.ok -eq $false) { throw 'Salamander host rejected the worker' }

$commands = [pscustomobject]@{}
$commands | Add-Member ScriptMethod Execute {
    param([string]$CommandId)
    (Invoke-Host -Method 'salamander.commands.execute' -Arguments @{ commandId = $CommandId }).result
}
$commands | Add-Member ScriptMethod Register {
    param([string]$CommandId, [string]$Title, [bool]$PluginMenu = $true, [bool]$ContextMenu = $false)
    (Invoke-Host -Method 'salamander.commands.register' -Arguments @{ commandId = $CommandId; title = $Title; pluginMenu = $PluginMenu; contextMenu = $ContextMenu }).registered
}
$commands | Add-Member ScriptMethod Unregister {
    param([string]$CommandId)
    (Invoke-Host -Method 'salamander.commands.unregister' -Arguments @{ commandId = $CommandId }).unregistered
}
$storage = [pscustomobject]@{}
$storage | Add-Member ScriptMethod Get {
    param([string]$Key, [object]$Default = $null)
    $result = Invoke-Host -Method 'salamander.storage.get' -Arguments @{ key = $Key }
    if ($result.type -eq 'string') { return $result.value }
    return $Default
}
$storage | Add-Member ScriptMethod Set {
    param([string]$Key, [string]$Value)
    [void](Invoke-Host -Method 'salamander.storage.set' -Arguments @{ key = $Key; value = $Value })
}
$fileOperations = [pscustomobject]@{}
$fileOperations | Add-Member ScriptMethod Rename { (Invoke-Host -Method 'salamander.fileOperations.rename' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Copy { (Invoke-Host -Method 'salamander.fileOperations.copy' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Move { (Invoke-Host -Method 'salamander.fileOperations.move' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Delete { (Invoke-Host -Method 'salamander.fileOperations.delete' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod CreateDirectory { (Invoke-Host -Method 'salamander.fileOperations.createDirectory' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Refresh { (Invoke-Host -Method 'salamander.fileOperations.refresh' -Arguments @{}).result }
$fileOperations | Add-Member ScriptMethod Properties { (Invoke-Host -Method 'salamander.fileOperations.properties' -Arguments @{}).result }
$sides = [pscustomobject]@{}
$sides | Add-Member ScriptMethod ActiveTab {
    param([string]$Side = 'source')
    Invoke-Host -Method 'salamander.sides.activeTab' -Arguments @{ side = $Side }
}
$sides | Add-Member ScriptMethod Context {
    param([string]$Side = 'source')
    Invoke-Host -Method 'salamander.sides.context' -Arguments @{ side = $Side }
}
function New-SalamatrixSideView([string]$SideName) {
    $view = [pscustomobject]@{ Side = $SideName }
    $view | Add-Member ScriptMethod ActiveTab { Invoke-Host -Method 'salamander.sides.activeTab' -Arguments @{ side = $this.Side } }
    $view | Add-Member ScriptMethod Context { Invoke-Host -Method 'salamander.sides.context' -Arguments @{ side = $this.Side } }
    return $view
}
$leftSide = New-SalamatrixSideView 'left'
$rightSide = New-SalamatrixSideView 'right'
$sourceSide = New-SalamatrixSideView 'source'
$targetSide = New-SalamatrixSideView 'target'
$ui = [pscustomobject]@{}
$ui | Add-Member ScriptMethod MessageBox {
    param([string]$Message, [string]$Title = 'Salamander')
    (Invoke-Host -Method 'salamander.ui.messageBox' -Arguments @{ message = $Message; title = $Title }).result
}
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
$ui | Add-Member ScriptMethod Dialog {
    param([string]$Title = 'Salamander')
    $created = Invoke-Host -Method 'salamander.ui.dialog.create' -Arguments @{ title = $Title }
    $dialog = [pscustomobject]@{ DialogId = [string]$created.dialogId }
    $dialog | Add-Member ScriptMethod AddControl {
        param([string]$Kind, [string]$Id, [string]$Text = '', [bool]$ReadOnly = $false, [bool]$Checked = $false, [int]$DialogResult = 0, [hashtable]$Layout = $null)
        $arguments = @{ dialogId = $this.DialogId; kind = $Kind; controlId = $Id; text = $Text; readOnly = $ReadOnly; checked = $Checked; dialogResult = $DialogResult }
        if ($null -ne $Layout) {
            foreach ($name in @('x', 'y', 'width', 'height')) {
                if ($Layout.ContainsKey($name)) { $arguments[$name] = [int]$Layout[$name] }
            }
        }
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
    $dialog | Add-Member ScriptMethod AddTextBox { param([string]$Id, [string]$Text = '', [bool]$ReadOnly = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'textbox'; controlId = $Id; text = $Text; readOnly = $ReadOnly }) }
    $dialog | Add-Member ScriptMethod AddCheckBox { param([string]$Id, [string]$Text, [bool]$Checked = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'checkbox'; controlId = $Id; text = $Text; checked = $Checked }) }
    $dialog | Add-Member ScriptMethod AddRadioButton { param([string]$Id, [string]$Text, [bool]$Checked = $false) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'radio'; controlId = $Id; text = $Text; checked = $Checked }) }
    $dialog | Add-Member ScriptMethod AddComboBox { param([string]$Id, [string]$Text = '') [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'combobox'; controlId = $Id; text = $Text }) }
    $dialog | Add-Member ScriptMethod AddListView { param([string]$Id) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'listview'; controlId = $Id }) }
    $dialog | Add-Member ScriptMethod AddTreeView { param([string]$Id) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'treeview'; controlId = $Id }) }
    $dialog | Add-Member ScriptMethod AddTabControl { param([string]$Id) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'tabcontrol'; controlId = $Id }) }
    $dialog | Add-Member ScriptMethod AddItem { param([string]$ControlId, [string]$Text, [int]$ParentIndex = -1) (Invoke-Host -Method 'salamander.ui.dialog.item' -Arguments @{ dialogId = $this.DialogId; controlId = $ControlId; text = $Text; parentIndex = $ParentIndex }).itemCount }
    $dialog | Add-Member ScriptMethod ClearItems { param([string]$ControlId) [void](Invoke-Host -Method 'salamander.ui.dialog.clearItems' -Arguments @{ dialogId = $this.DialogId; controlId = $ControlId }) }
    $dialog | Add-Member ScriptMethod AddButton { param([string]$Id, [string]$Text, [int]$DialogResult = 1) [void](Invoke-Host -Method 'salamander.ui.dialog.add' -Arguments @{ dialogId = $this.DialogId; kind = 'button'; controlId = $Id; text = $Text; dialogResult = $DialogResult }) }
    $dialog | Add-Member ScriptMethod Show { (Invoke-Host -Method 'salamander.ui.dialog.show' -Arguments @{ dialogId = $this.DialogId }).result }
    $dialog | Add-Member ScriptMethod Get { param([string]$Id) Invoke-Host -Method 'salamander.ui.dialog.get' -Arguments @{ dialogId = $this.DialogId; controlId = $Id } }
    $dialog | Add-Member ScriptMethod Close { [void](Invoke-Host -Method 'salamander.ui.dialog.destroy' -Arguments @{ dialogId = $this.DialogId }) }
    return $dialog
}
$clipboard = [pscustomobject]@{}
$clipboard | Add-Member ScriptMethod CopyText {
    param([string]$Text, [bool]$ShowEcho = $false)
    (Invoke-Host -Method 'salamander.clipboard.copyText' -Arguments @{ text = $Text; showEcho = $ShowEcho }).copied
}
$ai = [pscustomobject]@{}
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

$Salamander = [pscustomobject]@{
    commands = $commands
    storage = $storage
    file_operations = $fileOperations
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
}

& $EntryPoint
if ($OneShot) { exit 0 }
while ($true) {
    $frame = Read-Frame
    if ($frame.Kind -eq 'event') { Invoke-Event $frame.Payload; continue }
    if ($frame.Kind -eq 'shutdown') { break }
}


