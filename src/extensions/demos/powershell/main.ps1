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
}
