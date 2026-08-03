. (Join-Path $PSScriptRoot 'generated/ui-capabilities-dialog.generated.ps1')

if ($Salamander.command_handler -eq 'run') {
    [void](ShowUiCapabilitiesDialog)
}
