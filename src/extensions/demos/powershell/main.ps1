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

    # Keep the controls showcase last so it is the final, user-controlled step.
    $dialog = $Salamander.ui.Dialog('Salamatrix UI capabilities', 520, 315)
    try {
        $dialog.AddControl('label', 'intro', 'Controls provided by Salamatrix', $false, $false, 0, @{ x = 10; y = 8; width = 500; height = 12 })
        $dialog.AddControl('label', 'text-heading', 'Text and picker controls', $false, $false, 0, @{ x = 10; y = 28; width = 240; height = 12 })
        $dialog.AddControl('textbox', 'description', "Native controls are shared by every Salamatrix runtime.`r`nThe dialog follows the current Salamander theme and DPI.", $true, $false, 0, @{ x = 10; y = 42; width = 240; height = 42 }, $false, $true)
        $dialog.AddControl('filepicker', 'file', 'C:\Example\document.txt', $false, $false, 0, @{ x = 10; y = 94; width = 240; height = 18 })
        $dialog.AddControl('folderpicker', 'folder', 'Choose a folder...', $false, $false, 0, @{ x = 10; y = 118; width = 240; height = 18 })
        $dialog.AddControl('checkbox', 'checkbox', 'Check box', $false, $true, 0, @{ x = 10; y = 146; width = 110; height = 14 })
        $dialog.AddControl('radio', 'radio', 'Radio button', $false, $true, 0, @{ x = 130; y = 146; width = 120; height = 14 })
        $dialog.AddControl('tabcontrol', 'tabs', '', $false, $false, 0, @{ x = 10; y = 174; width = 240; height = 70 })
        [void]$dialog.AddItem('tabs', 'Overview')
        [void]$dialog.AddItem('tabs', 'Details')
        [void]$dialog.SetSelectedIndex('tabs', 0)

        $dialog.AddControl('label', 'collection-heading', 'Choice and collection controls', $false, $false, 0, @{ x = 270; y = 28; width = 240; height = 12 })
        $dialog.AddControl('combobox', 'choice', '', $false, $false, 0, @{ x = 270; y = 42; width = 240; height = 80 })
        foreach ($item in @('Salamatrix UI', 'Native Win32 controls', 'Runtime-neutral API')) { [void]$dialog.AddItem('choice', $item) }
        [void]$dialog.SetSelectedIndex('choice', 0)
        $dialog.AddControl('listview', 'list', '', $false, $false, 0, @{ x = 270; y = 70; width = 240; height = 78 })
        $dialog.AddColumn('list', 'Capability', 210)
        foreach ($item in @('Explicit layouts', 'Validation and events', 'Accessible metadata')) { [void]$dialog.AddItem('list', $item) }
        [void]$dialog.SetSelectedIndex('list', 0)
        $dialog.AddControl('treeview', 'tree', '', $false, $false, 0, @{ x = 270; y = 158; width = 240; height = 86 })
        [void]$dialog.AddItem('tree', 'Salamatrix UI')
        [void]$dialog.AddItem('tree', 'Dialogs', 0)
        [void]$dialog.AddItem('tree', 'Controls', 0)
        $dialog.AddControl('button', 'close', 'Close', $false, $false, 1, @{ x = 440; y = 276; width = 70; height = 22 })
        [void]$dialog.Show()
    }
    finally {
        $dialog.Close()
    }
}
