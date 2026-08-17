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
                logged=$event.TimeCreated.ToString('g')
                source=$provider; eventId=$eventId; task=$task
            }
        })
    }
    [void]$Salamander.file_system.AddItems($items.ToArray())
}

function Show-EventDetails {
    param($Event, [string]$LogName, $Strings)
    $message = try { [string]$Event.FormatDescription() } catch {
        try { [string]$Event.Message } catch { [string]$Strings.messageUnavailable }
    }
    if ([string]::IsNullOrWhiteSpace($message)) {
        $message = [string]$Strings.messageUnavailable
    }
    $xml = try { [string]$Event.ToXml() } catch { '' }
    $level = Get-EventLevelText $Event $Strings
    $metadata = [string]::Format([string]$Strings.metadata,
        $LogName, [string]$Event.ProviderName, [string]$Event.Id,
        $level, $Event.TimeCreated.ToString('g'), [string]$Event.TaskDisplayName,
        [string]$Event.UserId, [string]$Event.MachineName, [string]$Event.RecordId)
    $title = [string]::Format([string]$Strings.windowTitle,
        [string]$Event.Id, [string]$Event.ProviderName)
    $dialog = $Salamander.ui.Dialog($title, 600, 410)
    $dialog.AddControl('label', 'generalLabel', [string]$Strings.general,
        $false, $false, 0, @{x=10;y=8;width=80;height=12})
    $dialog.AddControl('textbox', 'message', $message, $true, $false, 0,
        @{x=10;y=24;width=580;height=180}, $false, $true)
    $dialog.AddControl('textbox', 'metadata', $metadata, $true, $false, 0,
        @{x=10;y=212;width=580;height=82}, $false, $true)
    $dialog.AddControl('label', 'detailsLabel', [string]$Strings.details,
        $false, $false, 0, @{x=10;y=302;width=80;height=12})
    $dialog.AddControl('textbox', 'xml', $xml, $true, $false, 0,
        @{x=10;y=318;width=500;height=80}, $false, $true)
    $dialog.AddControl('button', 'close', [string]$Strings.close,
        $false, $false, 1, @{x=520;y=376;width=70;height=22})
    try { [void]$dialog.Show() } finally { $dialog.Close() }
}

$handler = [string]$Salamander.command_handler
$locale = try { [string]$Salamander.application.Language() } catch { 'en' }
$strings = Get-EventViewerStrings $locale

if ($handler -eq 'openEvent') {
    $identity = ConvertFrom-EventItemId ([string]$Salamander.invocation.item.id)
    if ($null -ne $identity) {
        $recordId = [long]$identity.recordId
        $event = Get-WinEvent -LogName ([string]$identity.logName) `
            -FilterXPath "*[System[EventRecordID=$recordId]]" -MaxEvents 1 `
            -ErrorAction Stop | Select-Object -First 1
        if ($null -ne $event) { Show-EventDetails $event $identity.logName $strings }
    }
    return
}
if ($handler -ne 'listEvents') { return }

$path = try { [string]$Salamander.invocation.path } catch { '' }
$parts = @($path -split '[\\/]' | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$section = if ($parts.Count -gt 1) { [string]$parts[1] } else { '' }
$subPath = if ($parts.Count -gt 2) { @($parts[2..($parts.Count - 1)]) } else { @() }

if ($section -eq 'windows-logs') {
    $logs = @('Application','Security','Setup','System','ForwardedEvents')
    if ($subPath.Count -eq 0) {
        $items = @($logs | ForEach-Object {
            @{id=('log-' + $_); name=$_; directory=$true; enabled=$true;
              columns=@{level='';logged='';source='';eventId='';task=''}}
        })
        [void]$Salamander.file_system.AddItems($items)
    } else {
        $logName = [string]$subPath[0]
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
    $prefix = if ($subPath.Count -gt 0) { $subPath -join '/' } else { '' }
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
