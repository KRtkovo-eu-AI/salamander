# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-26 | Require generated launchers to omit a policy override switch. | The shipped menu builder was flagged for embedding that command line. | | Grok 4.7 |
# | 2026-11-06 | Expect ExtensionMenuBuilder generatedBy without OpenSalamander prefix. | Menu Builder package id was shortened; smoke test lagged behind. | | Composer |
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$builderRoot = Join-Path $root 'src\extensions\extension-menu-builder'
. (Join-Path $builderRoot 'main.ps1')

$builderSource = Get-Content -Raw (Join-Path $builderRoot 'main.ps1')
if ($builderSource -notmatch '\$form\.BeginInvoke\(\$disposePreview\)' -or
    $builderSource -match '(?s)\$preview\.Add_Closed\(\{.*?\$preview\.Dispose\(\).*?\}\s*\.GetNewClosure') {
    throw ('Menu preview cleanup must be deferred until after the ' +
        'ContextMenuStrip Closed event has completed.')
}
if ($builderSource -notmatch
    'foreach \(\$resource in \$previewResources\.ToArray\(\)\)') {
    throw ('Menu preview cleanup must use List.ToArray(); Windows PowerShell ' +
        'cannot reliably enumerate this generic list through @(...).')
}

Add-Type -AssemblyName System.Drawing

$iconStream = New-Object System.IO.MemoryStream
try {
    $systemIcon = [System.Drawing.SystemIcons]::Application
    $systemIcon.Save($iconStream)
    $script:MockPreviewIcon = [System.Convert]::ToBase64String(
        $iconStream.ToArray())
} finally {
    $iconStream.Dispose()
}
$script:MockPreviewPath = ''
function Invoke-Host {
    param([string]$Method, [hashtable]$Arguments)
    if ($Method -ne 'salamander.ui.renderIcon') {
        throw "Unexpected mocked host method: $Method"
    }
    $script:MockPreviewPath = [string]$Arguments.path
    return [pscustomobject]@{ icon = $script:MockPreviewIcon }
}

$script:UseWindowsDarkMode = $true
$previewCommand = [pscustomobject]@{
    Icon = Join-Path $builderRoot 'icon.svg'
    IconDark = Join-Path $builderRoot 'icon-dark.svg'
}
$previewImage = Get-BuilderPreviewImage $previewCommand
try {
    if ($null -eq $previewImage -or
        $script:MockPreviewPath -ne [System.IO.Path]::GetFullPath(
            $previewCommand.IconDark)) {
        throw 'Menu preview did not create an image from the dark SVG icon.'
    }
} finally {
    if ($null -ne $previewImage) { $previewImage.Dispose() }
}

$script:Strings = (
    Get-Content -Raw (Join-Path $builderRoot 'locales\en.json') |
    ConvertFrom-Json).strings
$script:Commands = New-Object System.Collections.Generic.List[object]
$command = New-BuilderCommand 'Open Notepad'
$command.Key = 'notepad'
$command.Target = 'notepad.exe'
$command.Toolbar = $true
$script:Commands.Add($command)

$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    'salamatrix-menu-builder-' + [guid]::NewGuid().ToString('N'))
try {
    if (-not (Save-BuilderProject 'Test Menu' 'OpenSalamander.TestMenu' (
                'Generated smoke test') $testRoot)) {
        throw 'Save-BuilderProject returned false.'
    }
    $manifest = Get-Content -Raw (Join-Path $testRoot 'extension.json') |
        ConvertFrom-Json
    $actions = Get-Content -Raw (Join-Path $testRoot 'actions.json') |
        ConvertFrom-Json
    $project = Get-Content -Raw (Join-Path $testRoot 'menu-builder.json') |
        ConvertFrom-Json
    $parseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile(
        (Join-Path $testRoot 'main.ps1'), [ref]$null, [ref]$parseErrors)
    if ($parseErrors.Count -gt 0) {
        throw ($parseErrors.Message -join '; ')
    }
    if ($manifest.commands[0].handler -ne 'notepad' -or
        $actions.commands[0].target -ne 'notepad.exe' -or
        $project.generatedBy -ne 'ExtensionMenuBuilder') {
        throw 'Generated extension files do not match the builder project.'
    }
    $generatedRuntime = Get-Content -Raw (Join-Path $testRoot 'main.ps1')
    if ($generatedRuntime -match '(?i)-executionpolicy\b' -or
        $generatedRuntime -match '(?i)\bbypass\b' -or
        $generatedRuntime -notmatch '-NoProfile -File' -or
        $builderSource -match '(?i)-executionpolicy\b' -or
        $builderSource -match '(?i)\bbypass\b') {
        throw ('Menu builder still embeds a policy override in the ' +
            'shipped script or in the runtime it generates.')
    }
    $marker = Join-Path $testRoot 'marker.txt'
    $payloadPath = Join-Path $testRoot 'payload.ps1'
    Write-Utf8WithoutBom $payloadPath @'
param([string]$Name)
Set-Content -LiteralPath $env:MENU_MARKER -Value $Name -Encoding ASCII
'@
    $script:Commands.Clear()
    $payloadCommand = New-BuilderCommand 'Run payload'
    $payloadCommand.Key = 'payload'
    $payloadCommand.Action = 'powershell'
    $payloadCommand.Target = $payloadPath
    $payloadCommand.Arguments = '-Name marker-ok'
    $payloadCommand.WorkingDirectory = $testRoot
    $script:Commands.Add($payloadCommand)
    if (-not (Save-BuilderProject 'Test Menu' 'OpenSalamander.TestMenu' (
                'Generated smoke test') $testRoot)) {
        throw 'Save-BuilderProject returned false for the script action.'
    }
    $wrapperPath = Join-Path $testRoot 'wrapper.ps1'
    $generatedPath = (Join-Path $testRoot 'main.ps1').Replace("'", "''")
    $markerLiteral = $marker.Replace("'", "''")
    Write-Utf8WithoutBom $wrapperPath @"
`$ErrorActionPreference = 'Stop'
`$env:MENU_MARKER = '$markerLiteral'
`$ui = New-Object psobject
`$ui | Add-Member -MemberType ScriptMethod -Name MessageBox -Value {
    param(`$Message, `$Title, `$Buttons, `$Icon)
    throw `$Message
}
`$side = New-Object psobject
`$side | Add-Member -MemberType ScriptMethod -Name Context -Value { `$null }
`$Salamander = New-Object psobject
`$Salamander | Add-Member -NotePropertyName command_handler -NotePropertyValue 'payload'
`$Salamander | Add-Member -NotePropertyName ui -NotePropertyValue `$ui
`$Salamander | Add-Member -NotePropertyName source_side -NotePropertyValue `$side
`$Salamander | Add-Member -NotePropertyName target_side -NotePropertyValue `$side
. '$generatedPath'
"@
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $wrapperPath
    if ($LASTEXITCODE -ne 0) {
        throw "Generated menu runtime exited with code $LASTEXITCODE."
    }
    $deadline = [datetime]::UtcNow.AddSeconds(20)
    do {
        if (Test-Path -LiteralPath $marker) { break }
        Start-Sleep -Milliseconds 200
    } while ([datetime]::UtcNow -lt $deadline)
    if (-not (Test-Path -LiteralPath $marker)) {
        throw 'Generated PowerShell action did not run the selected script.'
    }
    $markerText = (Get-Content -LiteralPath $marker -Raw).Trim()
    if ($markerText -ne 'marker-ok') {
        throw "Generated PowerShell action wrote unexpected output: $markerText"
    }
    $handwrittenRoot = Join-Path $testRoot 'handwritten'
    [System.IO.Directory]::CreateDirectory($handwrittenRoot) | Out-Null
    $handwrittenManifest = [pscustomobject][ordered]@{
        schema = 1
        id = 'OpenSalamander.Handwritten'
        name = 'Handwritten'
        version = '2.5.0'
        runtime = 'PowerShell'
        entryPoint = 'custom.ps1'
        capabilities = @('ui.dialogs')
        commands = @([pscustomobject][ordered]@{
            id = 'OpenSalamander.Handwritten.keep'
            title = 'Keep me'
            handler = 'keep'
            menu = 'plugin'
            requires = 'disk'
        })
    }
    Write-Utf8WithoutBom (Join-Path $handwrittenRoot 'extension.json') (
        $handwrittenManifest | ConvertTo-Json -Depth 8)
    Write-Utf8WithoutBom (Join-Path $handwrittenRoot 'custom.ps1') (
        "'handwritten entry point'")
    $loaded = Load-BuilderProject (
        Join-Path $handwrittenRoot 'extension.json')
    $script:ManifestOnlyMode = $true
    $script:ImportedManifest = $loaded.Manifest
    $script:ImportedExtensionFolder = $loaded.Folder
    if (-not (Save-BuilderProject $loaded.Name $loaded.Id (
                $loaded.Description) $loaded.Folder)) {
        throw 'Manifest-only save returned false.'
    }
    $updatedHandwritten = Get-Content -Raw (
        Join-Path $handwrittenRoot 'extension.json') | ConvertFrom-Json
    $entryPointText = Get-Content -Raw (
        Join-Path $handwrittenRoot 'custom.ps1')
    if ($updatedHandwritten.version -ne '2.5.0' -or
        $updatedHandwritten.entryPoint -ne 'custom.ps1' -or
        $updatedHandwritten.commands[0].requires -ne 'disk' -or
        $entryPointText -ne "'handwritten entry point'") {
        throw 'Manifest-only editing did not preserve the existing extension.'
    }
    Write-Output 'Extension Menu Builder smoke test passed.'
}
finally {
    $resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
    $temporaryRoot =
        [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
    if ($resolvedTestRoot.StartsWith(
            $temporaryRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedTestRoot)) {
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    }
}
