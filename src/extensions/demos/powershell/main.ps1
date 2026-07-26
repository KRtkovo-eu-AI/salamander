if ($Salamander.command_handler -eq 'run') {
    $Salamander.ui.Notify('PowerShell extension package is running through Salamatrix.', 'Salamatrix PowerShell Demo', 2500)
    $null = $Salamander.storage.Set('lastRun', 'PowerShell')
}
