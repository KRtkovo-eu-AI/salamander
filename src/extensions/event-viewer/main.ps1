Set-StrictMode -Version 2.0

function Get-EventViewerStrings {
    param([string]$Locale)
    foreach ($candidate in @($Locale, ($Locale -split '-')[0], 'en')) {
        if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
        $path = Join-Path $PSScriptRoot "locales\$candidate.json"
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return (Get-Content -LiteralPath $path -Raw -Encoding UTF8 |
                ConvertFrom-Json)
        }
    }
    throw 'The English localization resource is missing.'
}

function ConvertTo-EventItemId {
    param([string]$LogName, [long]$RecordId)
    $text = $LogName + "`n" + $RecordId.ToString(
        [Globalization.CultureInfo]::InvariantCulture)
    return 'event-' + [Convert]::ToBase64String(
        [Text.Encoding]::UTF8.GetBytes($text)).TrimEnd('=').
        Replace('+', '-').Replace('/', '_')
}

function ConvertFrom-EventItemId {
    param([string]$ItemId)
    if (-not $ItemId.StartsWith('event-')) { return $null }
    $encoded = $ItemId.Substring(6).Replace('-', '+').Replace('_', '/')
    while (($encoded.Length % 4) -ne 0) { $encoded += '=' }
    $parts = [Text.Encoding]::UTF8.GetString(
        [Convert]::FromBase64String($encoded)) -split "`n", 2
    if ($parts.Count -ne 2) { return $null }
    return @{logName=$parts[0]; recordId=[long]$parts[1]}
}

function ConvertTo-SafeEventName {
    param([string]$Name)
    $safe = $Name -replace '[\\/]', [char]0x2215
    if ([string]::IsNullOrWhiteSpace($safe)) { return 'Event' }
    return $safe
}

function Get-EventLevelText {
    param($Event, $Strings)
    switch ([int]$Event.Level) {
        1 { return [string]$Strings.levels.critical }
        2 { return [string]$Strings.levels.error }
        3 { return [string]$Strings.levels.warning }
        4 { return [string]$Strings.levels.information }
        5 { return [string]$Strings.levels.verbose }
        default { return [string]$Strings.levels.information }
    }
}

function Get-EventRecord {
    param([string]$LogName, [long]$recordId)
    # Event Log can transiently reject a point query while its channel is
    # being updated. The old one-shot query terminated before Dialog.Show(),
    # leaving the user with neither a window nor an explanation.
    $lastError = $null
    foreach ($attempt in 1..5) {
        try {
            $event = Get-WinEvent -LogName $LogName `
                -FilterXPath "*[System[EventRecordID=$recordId]]" `
                -MaxEvents 1 -ErrorAction Stop | Select-Object -First 1
            if ($null -ne $event) { return $event }
        } catch {
            $lastError = $_
        }
        if ($attempt -lt 5) { Start-Sleep -Milliseconds 75 }
    }
    if ($null -ne $lastError) { throw $lastError }
    throw "Event record $recordId was not found in log '$LogName'."
}

function Limit-EventDialogText {
    param([string]$Text, $Strings)
    # Each control is transported as one JSON protocol frame. Keep enough
    # headroom for UTF-8 expansion and JSON escaping, and do not split a UTF-16
    # surrogate pair at the boundary.
    $limit = 30000
    if ($null -eq $Text -or $Text.Length -le $limit) { return [string]$Text }
    if ([char]::IsHighSurrogate($Text[$limit - 1])) { --$limit }
    return $Text.Substring(0, $limit) + "`r`n`r`n" +
        [string]$Strings.textTruncated
}

function Add-EventDialogControl {
    param($Dialog, [object[]]$Arguments)
    $controlId = if ($Arguments.Count -gt 1) {
        [string]$Arguments[1]
    } else { '<unknown>' }
    try {
        [void]$Dialog.AddControl.Invoke($Arguments)
    } catch {
        throw "Event Properties control '$controlId' failed: " +
            [string]$_.Exception.Message
    }
}

function Add-EventRows {
    param([object[]]$Events, [string]$LogName, $Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    foreach ($event in $Events) {
        $provider = [string]$event.ProviderName
        $eventId = [string]$event.Id
        $recordId = [long]$event.RecordId
        $title = [string]::Format([string]$Strings.eventName, $eventId, $provider, $recordId)
        $task = try { [string]$event.TaskDisplayName } catch { '' }
        $items.Add(@{
            id=(ConvertTo-EventItemId $LogName $recordId)
            name=(ConvertTo-SafeEventName $title)
            compactName=(ConvertTo-SafeEventName $title)
            directory=$false; enabled=$true
            columns=@{
                level=(Get-EventLevelText $event $Strings)
                logged=([DateTimeOffset]$event.TimeCreated).ToUnixTimeMilliseconds().ToString(
                    [Globalization.CultureInfo]::InvariantCulture)
                source=$provider; eventId=$eventId; task=$task
            }
        })
    }
    [void]$Salamander.file_system.AddItems($items.ToArray())
}

function Show-EventDetails {
    param($Event, [string]$LogName, $Strings)
    $current = $Event
    $orderedIds = @($Salamander.invocation.panelItemIds)
    $currentIndex = try {
        [int]$Salamander.invocation.panelItemIndex
    } catch { -1 }
    while ($null -ne $current) {
        $message = try { [string]$current.FormatDescription() } catch {
            try { [string]$current.Message } catch {
                [string]$Strings.messageUnavailable
            }
        }
        if ([string]::IsNullOrWhiteSpace($message)) {
            $message = [string]$Strings.messageUnavailable
        }
        $xml = try { [string]$current.ToXml() } catch { '' }
        $message = Limit-EventDialogText $message $Strings
        $xml = Limit-EventDialogText $xml $Strings
        $metadata = [string]::Format([string]$Strings.metadata,
            $LogName, [string]$current.ProviderName, [string]$current.Id,
            (Get-EventLevelText $current $Strings),
            $current.TimeCreated.ToString('g'),
            [string]$current.TaskDisplayName, [string]$current.UserId,
            [string]$current.MachineName, [string]$current.RecordId)
        $title = [string]::Format([string]$Strings.windowTitle,
            [string]$current.Id, [string]$current.ProviderName)
        $dialog = $Salamander.ui.Dialog($title, 420, 340, $true)
        try {
            # Framework-owned toolbar header uses Salamander's existing
            # MoveItemUp/MoveItemDown SVGs, including dark-scheme variants.
            Add-EventDialogControl $dialog @('toolbarheader', 'eventNavigation',
                [string]$Strings.general, $false, $false, 1,
                @{x=10;y=8;width=400;height=12}, $false, $false,
                @{alignControlId='metadata';buttonMask=0x30})
            Add-EventDialogControl $dialog @('button', 'close',
                [string]$Strings.close,
                $false, $false, 1, @{x=350;y=312;width=60;height=20})
            # A multiline label preserves the field rows. StaticText is a
            # value-display helper and intentionally flattens CR/LF.
            Add-EventDialogControl $dialog @('label', 'metadata', $metadata,
                $true, $false, 0, @{x=10;y=22;width=400;height=70})
            Add-EventDialogControl $dialog @('label', 'descriptionLabel',
                [string]$Strings.descriptionLabel, $false, $false, 0,
                @{x=10;y=98;width=120;height=12})
            Add-EventDialogControl $dialog @('textbox', 'message', $message,
                $true, $false,
                0, @{x=10;y=112;width=400;height=100}, $false, $true)
            Add-EventDialogControl $dialog @('label', 'detailsLabel',
                [string]$Strings.details, $false, $false, 0,
                @{x=10;y=218;width=120;height=12})
            Add-EventDialogControl $dialog @('textbox', 'xml', $xml, $true,
                $false, 0,
                @{x=10;y=232;width=400;height=72}, $false, $true)
            $result = [int]$dialog.Show()
        } finally { $dialog.Close() }
        # TLBHDR_UP=5 and TLBHDR_DOWN=6. Only this script resolves the
        # adjacent record in the panel's current ordering.
        if (($result -eq 5 -or $result -eq 6) -and
            $orderedIds.Count -gt 0 -and $currentIndex -ge 0) {
            $offset = if ($result -eq 5) { -1 } else { 1 }
            $nextIndex = $currentIndex + $offset
            if ($nextIndex -ge 0 -and $nextIndex -lt $orderedIds.Count) {
                $identity = ConvertFrom-EventItemId `
                    ([string]$orderedIds[$nextIndex])
                if ($null -ne $identity) {
                    $recordId = [long]$identity.recordId
                    $adjacent = try {
                        Get-EventRecord ([string]$identity.logName) $recordId
                    } catch { $null }
                    if ($null -ne $adjacent) {
                        $currentIndex = $nextIndex
                        $current = $adjacent
                        $LogName = [string]$identity.logName
                        continue
                    }
                }
            }
        }
        break
    }
}

$handler = [string]$Salamander.command_handler
$locale = try { [string]$Salamander.application.Language() } catch { 'en' }
$strings = Get-EventViewerStrings $locale

if ($handler -eq 'openEvent') {
    $identity = ConvertFrom-EventItemId ([string]$Salamander.invocation.item.id)
    if ($null -ne $identity) {
        $recordId = [long]$identity.recordId
        try {
            $event = Get-EventRecord ([string]$identity.logName) $recordId
            Show-EventDetails $event $identity.logName $strings
        } catch {
            $detail = [string]$_.Exception.Message
            [void]$Salamander.ui.MessageBox(
                ([string]::Format([string]$strings.openError, $detail)),
                [string]$strings.openErrorTitle, 'OK', 'Error')
        }
    }
    return
}
if ($handler -ne 'listEvents') { return }

$path = try { [string]$Salamander.invocation.path } catch { '' }
$parts = @($path -split '[\\/]' | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$section = if ($parts.Count -gt 1) { [string]$parts[1] } else { '' }
$subPath = @()
if ($parts.Count -gt 2) {
    $subPath = @($parts[2..($parts.Count - 1)])
}

if ($section -eq 'windows-logs') {
    $logs = @('Application','Security','Setup','System','ForwardedEvents')
    if ($subPath.Count -eq 0) {
        $items = @($logs | ForEach-Object {
            @{id=('log-' + $_); name=$_; directory=$true; enabled=$true;
              columns=@{level='';logged='';source='';eventId='';task=''}}
        })
        [void]$Salamander.file_system.AddItems($items)
    } else {
        $logName = ([string]$subPath[0]) -replace '^log-', ''
        Add-EventRows @(Get-WinEvent -LogName $logName -MaxEvents 250 -ErrorAction Stop) $logName $strings
    }
    return
}

if ($section -eq 'custom-views') {
    if ($subPath.Count -eq 0) {
        [void]$Salamander.file_system.AddItems(@(@{id='administrative-events';
            name=[string]$strings.administrativeEvents; directory=$true; enabled=$true;
            columns=@{level='';logged='';source='';eventId='';task=''}}))
    } else {
        $events = @(Get-WinEvent -FilterHashtable @{
            LogName=@('Application','System'); Level=@(1,2,3)
        } -MaxEvents 250 -ErrorAction Stop)
        foreach ($group in ($events | Group-Object LogName)) {
            Add-EventRows @($group.Group) ([string]$group.Name) $strings
        }
    }
    return
}

if ($section -eq 'applications-services') {
    $allLogs = @(Get-WinEvent -ListLog * -ErrorAction SilentlyContinue |
        Where-Object { -not $_.IsClassicLog -and $_.LogName -like '*/*' } |
        Select-Object -ExpandProperty LogName)
    $decodedPath = @($subPath | ForEach-Object {
        ([string]$_) -replace '^log-node-', ''
    })
    $prefix = if ($decodedPath.Count -gt 0) { $decodedPath -join '/' } else { '' }
    if ($allLogs -contains $prefix) {
        Add-EventRows @(Get-WinEvent -LogName $prefix -MaxEvents 250 -ErrorAction Stop) $prefix $strings
        return
    }
    $childNames = @($allLogs | Where-Object {
        [string]::IsNullOrEmpty($prefix) -or $_.StartsWith($prefix + '/',
            [StringComparison]::OrdinalIgnoreCase)
    } | ForEach-Object {
        $remaining = if ([string]::IsNullOrEmpty($prefix)) { $_ } else {
            $_.Substring($prefix.Length + 1)
        }
        ($remaining -split '/')[0]
    } | Sort-Object -Unique)
    $items = @($childNames | ForEach-Object {
        @{id=('log-node-' + $_); name=(ConvertTo-SafeEventName $_);
          directory=$true; enabled=$true;
          columns=@{level='';logged='';source='';eventId='';task=''}}
    })
    [void]$Salamander.file_system.AddItems($items)
}
