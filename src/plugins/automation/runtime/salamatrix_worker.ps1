# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EntryPoint
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
$sides = [pscustomobject]@{}
$sides | Add-Member ScriptMethod ActiveTab {
    param([string]$Side = 'source')
    Invoke-Host -Method 'salamander.sides.activeTab' -Arguments @{ side = $Side }
}
$ui = [pscustomobject]@{}
$ui | Add-Member ScriptMethod MessageBox {
    param([string]$Message, [string]$Title = 'Salamander')
    (Invoke-Host -Method 'salamander.ui.messageBox' -Arguments @{ message = $Message; title = $Title }).result
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

$Salamander = [pscustomobject]@{
    commands = $commands
    storage = $storage
    sides = $sides
    left_side = $sides
    right_side = $sides
    source_side = $sides
    target_side = $sides
    ui = $ui
    events = $events
}

& $EntryPoint
while ($true) {
    $frame = Read-Frame
    if ($frame.Kind -eq 'event') { Invoke-Event $frame.Payload; continue }
    if ($frame.Kind -eq 'shutdown') { break }
}
