if ($Salamander.command_handler -eq 'viewDemo') {
    $path = [string]$Salamander.invocation.path
    try {
        $contents = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
        $preview = if ($contents.Length -gt 3000) { $contents.Substring(0, 3000) + "`n`n[preview truncated]" } else { $contents }
        if ([string]::IsNullOrEmpty($preview)) { $preview = '[empty file]' }
        [void]$Salamander.ui.MessageBox($preview, "Salamatrix PowerShell Viewer demo — $path", 'OK', 'Information')
    }
    catch {
        [void]$Salamander.ui.MessageBox($_.Exception.Message, 'Salamatrix PowerShell Viewer demo', 'OK', 'Error')
    }
    return
}

if ($Salamander.command_handler -eq 'listDemoMachines') {
    $machines = @(
        @{id='development'; name='Development VM'; running=$true},
        @{id='test-lab'; name='Test lab'; running=$false},
        @{id='build-agent'; name='Build agent'; running=$true})
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    foreach ($machine in $machines) {
        $running = $Salamander.storage.Get("machine.$($machine.id).running", $machine.running)
        $state = if ($running) { 'Running' } else { 'Stopped' }
        $items.Add(@{
            id=$machine.id; name="$($machine.name) — $state"
            icon='icon.svg'; directory=$false; enabled=$true
            columns=@{state=$state}})
    }
    [void]$Salamander.file_system.AddItems($items.ToArray())
    return
}

if ($Salamander.command_handler -eq 'inspectDemoMachine' -or $Salamander.command_handler -eq 'toggleDemoMachine') {
    $item = $Salamander.invocation.item
    if ($Salamander.command_handler -eq 'inspectDemoMachine') {
        [void]$Salamander.ui.MessageBox("Id: $($item.id)`nName: $($item.name)", 'Salamatrix FS item', 'OK', 'Information')
    }
    else {
        $itemId = if ($item.id) { [string]$item.id } else { 'unknown' }
        $key = "machine.$itemId.running"
        $defaultRunning = @{
            development=$true
            'test-lab'=$false
            'build-agent'=$true
        }[$itemId]
        if ($null -eq $defaultRunning) { $defaultRunning = $false }
        $running = [bool]$Salamander.storage.Get($key, $defaultRunning)
        $Salamander.storage.Set($key, -not $running)
        $state = if (-not $running) { 'Running' } else { 'Stopped' }
        [void]$Salamander.ui.Notify("$($item.name): $state", 'Salamatrix FS demo', 2500)
    }
    return
}

if ($Salamander.command_handler -eq 'run') {
    $Salamander.ui.Notify('PowerShell extension package is running through Salamatrix.', 'Salamatrix PowerShell Demo', 2500)
    $progress = $Salamander.ui.Progress('Salamatrix PowerShell Progress Demo', 5)
    try {
        for ($step = 1; $step -le 5; $step++) {
            [void]$progress.Update($step, -1, "Step $step of 5")
            Start-Sleep -Milliseconds 150
            if ($progress.IsCancelled()) { break }
        }
    }
    finally {
        $progress.Close()
    }
    $null = $Salamander.storage.Set('lastRun', 'PowerShell')

    # Build the complete gallery here, through the public runtime-neutral API.
    $dialog = $Salamander.ui.Dialog('Salamatrix UI capabilities', 463, 236)
    function Add-CapabilityControl($kind, $id, $text, $x, $y, $width, $height, $options = @{}) {
        $dialog.AddControl($kind, $id, $text, $false, $false, 0,
            @{x=$x; y=$y; width=$width; height=$height}, $false, $false, $options)
    }
    $uptime = "System was started $($Salamander.ui.Uptime()) ms ago."
    Add-CapabilityControl groupbox static-group 'CGUIStaticTextAbstract' 6 4 254 108
    Add-CapabilityControl label not-attached-label 'Not attached static text' 14 17 80 8
    Add-CapabilityControl label uptime-plain $uptime 102 17 152 8
    $rows = @(
        @('static-none','0 (no flags)',$uptime,27,0),
        @('static-cache','STF_CACHED_PAINT',$uptime,37,1),
        @('static-bold','STF_BOLD','Bold &text',47,0x10082),
        @('static-underline','STF_UNDERLINE','Underlined text',56,0x20004),
        @('static-end','STF_END_ELLIPSIS','Long long long long long long long long long string.',66,0x20),
        @('static-path','STF_PATH_ELLIPSIS','C:\Program Files\Some Application With Long Path\example.exe',76,0x40),
        @('static-path-url','STF_PATH_ELLIPSIS','ftp://ftp.altap.cz/pub/salamander/example.exe',87,0x40))
    foreach ($row in $rows) {
        Add-CapabilityControl label "$($row[0])-label" $row[1] 14 $row[3] 75 8
        $extra = @{styleFlags=$row[4]}; if ($row[0] -eq 'static-path-url') { $extra.pathSeparator='/' }
        Add-CapabilityControl statictext $row[0] $row[2] 102 $row[3] 152 8 $extra
    }
    Add-CapabilityControl label drag-hint 'Drag texts to change their size.' 151 97 103 8
    Add-CapabilityControl groupbox progress-group 'CGUIProgressBarAbstract' 6 118 254 66
    Add-CapabilityControl label progress-label 'Progress label' 15 129 60 8
    Add-CapabilityControl progressbar progress '' 15 138 235 12 @{progress=120}
    Add-CapabilityControl label unknown-label 'Unknown progress' 15 154 67 8
    Add-CapabilityControl progressbar unknown-progress '' 15 163 235 12 @{progress=-1;indeterminateDuration=-1;indeterminateInterval=100}
    Add-CapabilityControl groupbox buttons-group 'Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract' 6 188 254 40
    Add-CapabilityControl button more '...' 15 204 15 14 @{keepOpen=$true}
    Add-CapabilityControl arrowbutton arrow '' 37 204 15 14
    Add-CapabilityControl textarrowbutton choose '&Choose' 60 204 50 14 @{styleFlags=8}
    Add-CapabilityControl textarrowbutton drop '&Drop' 117 204 50 14 @{styleFlags=2}
    Add-CapabilityControl colorarrowbutton color '' 174 204 33 14 @{textColor=0xff8000;backgroundColor=0xff8000}
    Add-CapabilityControl colorarrowbutton color-text ABC 215 204 33 14 @{textColor=0;backgroundColor=0xffff}
    Add-CapabilityControl groupbox hyperlink-group CGUIHyperLinkAbstract 269 4 185 48
    Add-CapabilityControl label open-label SetActionOpen 277 17 75 8
    Add-CapabilityControl hyperlink open-link www.altap.cz 365 17 47 8 @{styleFlags=0x14;actionOpen='https://www.altap.cz'}
    Add-CapabilityControl label command-label SetActionPostCommand 277 27 81 8
    Add-CapabilityControl hyperlink command-link 'Say something!' 365 27 55 8 @{styleFlags=0x14;actionCommand=0x7f01}
    Add-CapabilityControl label hint-label SetActionShowHint 277 37 81 8
    Add-CapabilityControl hyperlink hint-link 'mask hints' 365 37 40 8 @{styleFlags=8;actionHint="text 1 text 1 text 1 text 1`ntext 2 text 2 text 2"}
    Add-CapabilityControl groupbox tooltip-group SetCurrentToolTip 269 59 185 31
    Add-CapabilityControl statictext tooltip 'Pause the mouse pointer over this text.' 278 73 130 8 @{styleFlags=0x40000;toolTip='ToolTip'}
    Add-CapabilityControl listview header-list '' 269 113 185 50 @{styleFlags=0x01e00000}
    Add-CapabilityControl toolbarheader toolbar-header CGUIToolbarHeaderAbstract 269 102 96 8 @{alignControlId='header-list';buttonMask=0x31}
    Add-CapabilityControl groupbox origin-group 'Created by' 269 169 185 38
    Add-CapabilityControl label runtime-label 'Runtime:' 277 181 42 8
    Add-CapabilityControl statictext runtime-value PowerShell 323 181 122 8 @{styleFlags=2}
    Add-CapabilityControl label extension-label 'Extension:' 277 192 42 8
    Add-CapabilityControl statictext extension-value 'Salamatrix PowerShell Demo' 323 192 122 8 @{styleFlags=2}
    Add-CapabilityControl button close Close 403 213 50 14 @{dialogResult=1;styleFlags=0x100000}
    try { [void]$dialog.Show() } finally { $dialog.Close() }
}
