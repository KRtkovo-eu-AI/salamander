<?php
if ($Salamander->command_handler === 'run') {
    $Salamander->ui->notify('PHP CLI extension package is running through Salamatrix.', 'Salamatrix PHP Demo', 2500);
    $progress = $Salamander->ui->progress('Salamatrix PHP Progress Demo', 5);
    try {
        for ($step = 1; $step <= 5; $step++) {
            $progress->update($step, null, "Step {$step} of 5");
            usleep(150000);
            if ($progress->isCancelled()) {
                break;
            }
        }
    } finally {
        $progress->close();
    }
    $Salamander->storage->set('lastRun', 'PHP.CLI');

    // Keep the controls showcase last so it is the final, user-controlled step.
    $dialog = $Salamander->ui->dialog('Salamatrix UI capabilities', 520, 315);
    try {
        $dialog->addControl('label', 'intro', 'Controls provided by Salamatrix', false, false, 0, array('x' => 10, 'y' => 8, 'width' => 500, 'height' => 12));
        $dialog->addControl('label', 'text-heading', 'Text and picker controls', false, false, 0, array('x' => 10, 'y' => 28, 'width' => 240, 'height' => 12));
        $dialog->addControl('textbox', 'description', "Native controls are shared by every Salamatrix runtime.\r\nThe dialog follows the current Salamander theme and DPI.", true, false, 0, array('x' => 10, 'y' => 42, 'width' => 240, 'height' => 42), false, true);
        $dialog->addControl('filepicker', 'file', 'C:\\Example\\document.txt', false, false, 0, array('x' => 10, 'y' => 94, 'width' => 240, 'height' => 18));
        $dialog->addControl('folderpicker', 'folder', 'Choose a folder...', false, false, 0, array('x' => 10, 'y' => 118, 'width' => 240, 'height' => 18));
        $dialog->addControl('checkbox', 'checkbox', 'Check box', false, true, 0, array('x' => 10, 'y' => 146, 'width' => 110, 'height' => 14));
        $dialog->addControl('radio', 'radio', 'Radio button', false, true, 0, array('x' => 130, 'y' => 146, 'width' => 120, 'height' => 14));
        $dialog->addControl('tabcontrol', 'tabs', '', false, false, 0, array('x' => 10, 'y' => 174, 'width' => 240, 'height' => 70));
        $dialog->addItem('tabs', 'Overview');
        $dialog->addItem('tabs', 'Details');
        $dialog->setSelectedIndex('tabs', 0);

        $dialog->addControl('label', 'collection-heading', 'Choice and collection controls', false, false, 0, array('x' => 270, 'y' => 28, 'width' => 240, 'height' => 12));
        $dialog->addControl('combobox', 'choice', '', false, false, 0, array('x' => 270, 'y' => 42, 'width' => 240, 'height' => 80));
        foreach (array('Salamatrix UI', 'Native Win32 controls', 'Runtime-neutral API') as $item) $dialog->addItem('choice', $item);
        $dialog->setSelectedIndex('choice', 0);
        $dialog->addControl('listview', 'list', '', false, false, 0, array('x' => 270, 'y' => 70, 'width' => 240, 'height' => 78));
        $dialog->addColumn('list', 'Capability', 210);
        foreach (array('Explicit layouts', 'Validation and events', 'Accessible metadata') as $item) $dialog->addItem('list', $item);
        $dialog->setSelectedIndex('list', 0);
        $dialog->addControl('treeview', 'tree', '', false, false, 0, array('x' => 270, 'y' => 158, 'width' => 240, 'height' => 86));
        $dialog->addItem('tree', 'Salamatrix UI');
        $dialog->addItem('tree', 'Dialogs', 0);
        $dialog->addItem('tree', 'Controls', 0);
        $dialog->addControl('button', 'close', 'Close', false, false, 1, array('x' => 440, 'y' => 276, 'width' => 70, 'height' => 22));
        $dialog->show();
    } finally {
        $dialog->close();
    }
}
