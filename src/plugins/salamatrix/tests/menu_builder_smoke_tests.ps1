$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$builderRoot = Join-Path $root 'src\extensions\extension-menu-builder'
. (Join-Path $builderRoot 'main.ps1')

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
        $project.generatedBy -ne 'OpenSalamander.ExtensionMenuBuilder') {
        throw 'Generated extension files do not match the builder project.'
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
