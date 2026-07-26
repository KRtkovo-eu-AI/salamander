<?php
if ($Salamander->command_handler === 'run') {
    $Salamander->ui->notify('PHP CLI extension package is running through Salamatrix.', 'Salamatrix PHP Demo', 2500);
    $Salamander->storage->set('lastRun', 'PHP.CLI');
}
