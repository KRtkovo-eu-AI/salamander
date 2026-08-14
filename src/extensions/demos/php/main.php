<?php
if ($Salamander->command_handler === 'viewDemo') {
    $path = isset($Salamander->invocation['path']) ? (string)$Salamander->invocation['path'] : '';
    $contents = @file_get_contents($path);
    if ($contents === false) {
        $error = error_get_last();
        $Salamander->ui->messageBox(isset($error['message']) ? $error['message'] : 'Unable to read file.', 'Salamatrix PHP Viewer demo', 'OK', 'Error');
    } else {
        $preview = strlen($contents) > 3000 ? substr($contents, 0, 3000) . "\n\n[preview truncated]" : $contents;
        $Salamander->ui->messageBox($preview === '' ? '[empty file]' : $preview, 'Salamatrix PHP Viewer demo — ' . $path);
    }
    return;
}

if ($Salamander->command_handler === 'listDemoMachines') {
    $machines = array(
        array('id'=>'development', 'name'=>'Development VM', 'running'=>true),
        array('id'=>'test-lab', 'name'=>'Test lab', 'running'=>false),
        array('id'=>'build-agent', 'name'=>'Build agent', 'running'=>true));
    $items = array();
    foreach ($machines as $machine) {
        $running = $Salamander->storage->get('machine.' . $machine['id'] . '.running', $machine['running']);
        $items[] = array(
            'id'=>$machine['id'],
            'name'=>$machine['name'] . ' — ' . ($running ? 'Running' : 'Stopped'),
            'icon'=>'icon.svg', 'directory'=>false, 'enabled'=>true,
            'columns'=>array('state'=>$running ? 'Running' : 'Stopped'));
    }
    $Salamander->file_system->add_items($items);
    return;
}

if ($Salamander->command_handler === 'inspectDemoMachine' || $Salamander->command_handler === 'toggleDemoMachine') {
    $item = isset($Salamander->invocation['item']) && is_array($Salamander->invocation['item']) ? $Salamander->invocation['item'] : array();
    $itemId = isset($item['id']) ? (string)$item['id'] : 'unknown';
    $itemName = isset($item['name']) ? (string)$item['name'] : $itemId;
    if ($Salamander->command_handler === 'inspectDemoMachine') {
        $Salamander->ui->messageBox("Id: {$itemId}\nName: {$itemName}", 'Salamatrix FS item');
    } else {
        $key = 'machine.' . $itemId . '.running';
        $defaults = array('development'=>true, 'test-lab'=>false, 'build-agent'=>true);
        $defaultRunning = isset($defaults[$itemId]) ? $defaults[$itemId] : false;
        $running = (bool)$Salamander->storage->get($key, $defaultRunning);
        $Salamander->storage->set($key, !$running);
        $Salamander->ui->notify($itemName . ': ' . (!$running ? 'Running' : 'Stopped'), 'Salamatrix FS demo', 2500);
    }
    return;
}

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

    // Build the complete gallery here, through the public runtime-neutral API.
    $dialog = $Salamander->ui->dialog('Salamatrix UI capabilities', 463, 236);
    $add = function ($kind, $id, $text, $x, $y, $width, $height, $options = array()) use ($dialog) {
        $dialog->addControl($kind, $id, $text, false, false, 0,
            array('x'=>$x, 'y'=>$y, 'width'=>$width, 'height'=>$height), false, false, $options);
    };
    $uptime = 'System was started ' . $Salamander->ui->uptime() . ' ms ago.';
    $add('groupbox','static-group','CGUIStaticTextAbstract',6,4,254,108);
    $add('label','not-attached-label','Not attached static text',14,17,80,8);
    $add('label','uptime-plain',$uptime,102,17,152,8);
    $rows = array(
        array('static-none','0 (no flags)',$uptime,27,0),
        array('static-cache','STF_CACHED_PAINT',$uptime,37,1),
        array('static-bold','STF_BOLD','Bold &text',47,0x10082),
        array('static-underline','STF_UNDERLINE','Underlined text',56,0x20004),
        array('static-end','STF_END_ELLIPSIS','Long long long long long long long long long string.',66,0x20),
        array('static-path','STF_PATH_ELLIPSIS','C:\\Program Files\\Some Application With Long Path\\example.exe',76,0x40),
        array('static-path-url','STF_PATH_ELLIPSIS','ftp://ftp.altap.cz/pub/salamander/example.exe',87,0x40));
    foreach ($rows as $row) {
        $add('label',$row[0].'-label',$row[1],14,$row[3],75,8);
        $extra = array('styleFlags'=>$row[4]);
        if ($row[0] === 'static-path-url') $extra['pathSeparator'] = '/';
        $add('statictext',$row[0],$row[2],102,$row[3],152,8,$extra);
    }
    $add('label','drag-hint','Drag texts to change their size.',151,97,103,8);
    $add('groupbox','progress-group','CGUIProgressBarAbstract',6,118,254,66);
    $add('label','progress-label','Progress label',15,129,60,8);
    $add('progressbar','progress','',15,138,235,12,array('progress'=>120));
    $add('label','unknown-label','Unknown progress',15,154,67,8);
    $add('progressbar','unknown-progress','',15,163,235,12,array('progress'=>-1,'indeterminateDuration'=>-1,'indeterminateInterval'=>100));
    $add('groupbox','buttons-group','Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract',6,188,254,40);
    $add('button','more','...',15,204,15,14,array('keepOpen'=>true));
    $add('arrowbutton','arrow','',37,204,15,14);
    $add('textarrowbutton','choose','&Choose',60,204,50,14,array('styleFlags'=>8));
    $add('textarrowbutton','drop','&Drop',117,204,50,14,array('styleFlags'=>2));
    $add('colorarrowbutton','color','',174,204,33,14,array('textColor'=>0xff8000,'backgroundColor'=>0xff8000));
    $add('colorarrowbutton','color-text','ABC',215,204,33,14,array('textColor'=>0,'backgroundColor'=>0xffff));
    $add('groupbox','hyperlink-group','CGUIHyperLinkAbstract',269,4,185,48);
    $add('label','open-label','SetActionOpen',277,17,75,8);
    $add('hyperlink','open-link','www.altap.cz',365,17,47,8,array('styleFlags'=>0x14,'actionOpen'=>'https://www.altap.cz'));
    $add('label','command-label','SetActionPostCommand',277,27,81,8);
    $add('hyperlink','command-link','Say something!',365,27,55,8,array('styleFlags'=>0x14,'actionCommand'=>0x7f01));
    $add('label','hint-label','SetActionShowHint',277,37,81,8);
    $add('hyperlink','hint-link','mask hints',365,37,40,8,array('styleFlags'=>8,'actionHint'=>"text 1 text 1 text 1 text 1\ntext 2 text 2 text 2"));
    $add('groupbox','tooltip-group','SetCurrentToolTip',269,59,185,31);
    $add('statictext','tooltip','Pause the mouse pointer over this text.',278,73,130,8,array('styleFlags'=>0x40000,'toolTip'=>'ToolTip'));
    $add('listview','header-list','',269,113,185,50,array('styleFlags'=>0x01e00000));
    $add('toolbarheader','toolbar-header','CGUIToolbarHeaderAbstract',269,102,96,8,array('alignControlId'=>'header-list','buttonMask'=>0x31));
    $add('groupbox','origin-group','Created by',269,169,185,38);
    $add('label','runtime-label','Runtime:',277,181,42,8);
    $add('statictext','runtime-value','PHP.CLI',323,181,122,8,array('styleFlags'=>2));
    $add('label','extension-label','Extension:',277,192,42,8);
    $add('statictext','extension-value','Salamatrix PHP Demo',323,192,122,8,array('styleFlags'=>2));
    $add('button','close','Close',403,213,50,14,array('dialogResult'=>1,'styleFlags'=>0x100000));
    try { $dialog->show(); } finally { $dialog->close(); }
}
