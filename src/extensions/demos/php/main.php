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

    // The framework-owned native controls showcase is the final demo step.
    if (!$Salamander->ui->controls()) {
        throw new RuntimeException('Salamatrix UI controls showcase is unavailable');
    }
}
