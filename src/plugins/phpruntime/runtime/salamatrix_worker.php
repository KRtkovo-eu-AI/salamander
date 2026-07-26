<?php
// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

const SALAMATRIX_MAX_FRAME_BYTES = 1048576;

function smx_send($kind, $id, $payload) {
    $json = json_encode($payload, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    $frame = "SMX1\t{$kind}\t{$id}\t{$json}\n";
    if (strlen($frame) > SALAMATRIX_MAX_FRAME_BYTES) throw new RuntimeException('SMX1 frame exceeds the 1 MiB limit');
    fwrite(STDOUT, $frame);
    fflush(STDOUT);
}

function smx_read() {
    $line = fgets(STDIN, SALAMATRIX_MAX_FRAME_BYTES + 2);
    if ($line === false) throw new RuntimeException('Salamander host closed the worker channel');
    if (strlen($line) > SALAMATRIX_MAX_FRAME_BYTES || substr($line, -1) !== "\n") throw new RuntimeException('Invalid or oversized SMX1 frame');
    $parts = explode("\t", rtrim($line, "\r\n"), 4);
    if (count($parts) !== 4 || $parts[0] !== 'SMX1') throw new RuntimeException('Invalid SMX1 frame');
    $payload = json_decode($parts[3], true);
    if (!is_array($payload)) throw new RuntimeException('Invalid SMX1 JSON payload');
    return array('kind' => $parts[1], 'id' => (int)$parts[2], 'payload' => $payload);
}

class SalamatrixClient {
    public $nextId = 1;
    public $handlers = array();
    public $subscriptions = array();

    public function dispatchEvent($payload) {
        $name = isset($payload['event']) ? $payload['event'] : '';
        if (!empty($this->handlers[$name])) foreach ($this->handlers[$name] as $handler) call_user_func($handler, $payload);
    }

    public function call($method, $arguments) {
        $payload = array('method' => $method);
        foreach ($arguments as $key => $value) $payload[$key] = $value;
        $id = $this->nextId++;
        smx_send('call', $id, $payload);
        while (true) {
            $frame = smx_read();
            if ($frame['kind'] === 'event') { $this->dispatchEvent($frame['payload']); continue; }
            if ($frame['id'] !== $id) continue;
            if ($frame['kind'] === 'error' || (isset($frame['payload']['ok']) && !$frame['payload']['ok'])) throw new RuntimeException(isset($frame['payload']['error']) ? $frame['payload']['error'] : 'host call failed');
            if ($frame['kind'] !== 'result') throw new RuntimeException('Unexpected SMX1 response');
            return $frame['payload'];
        }
    }
}

class SalamatrixCommands {
    private $client; public function __construct($client) { $this->client = $client; }
    public function execute($id) { $r = $this->client->call('salamander.commands.execute', array('commandId' => $id)); return isset($r['result']) ? $r['result'] : 'error'; }
    public function register($id, $title, $pluginMenu = true, $contextMenu = false, $hotKey = 0, $toolbar = false) { $r = $this->client->call('salamander.commands.register', array('commandId' => $id, 'title' => $title, 'pluginMenu' => $pluginMenu, 'contextMenu' => $contextMenu, 'hotKey' => (int)$hotKey, 'toolbar' => $toolbar)); return !empty($r['registered']); }
    public function unregister($id) { $r = $this->client->call('salamander.commands.unregister', array('commandId' => $id)); return !empty($r['unregistered']); }
}
class SalamatrixStorage {
    private $client; public function __construct($client) { $this->client = $client; }
    public function get($key, $default = null) { $r = $this->client->call('salamander.storage.get', array('key' => $key)); return isset($r['type']) && $r['type'] === 'string' ? $r['value'] : $default; }
    public function set($key, $value) { $this->client->call('salamander.storage.set', array('key' => $key, 'value' => $value)); }
    public function remove($key) { $r = $this->client->call('salamander.storage.remove', array('key' => $key)); return !empty($r['removed']); }
    public function clear() { $r = $this->client->call('salamander.storage.clear', array()); return !empty($r['ok']); }
}
class SalamatrixFileOperations {
    private $client; public function __construct($client) { $this->client = $client; }
    private function run($operation) { $r = $this->client->call('salamander.fileOperations.' . $operation, array()); return isset($r['result']) ? $r['result'] : 'error'; }
    public function rename() { return $this->run('rename'); }
    public function copy() { return $this->run('copy'); }
    public function move() { return $this->run('move'); }
    public function delete() { return $this->run('delete'); }
    public function createDirectory() { return $this->run('createDirectory'); }
    public function refresh() { return $this->run('refresh'); }
    public function properties() { return $this->run('properties'); }
}
class SalamatrixSides {
    private $client; public function __construct($client) { $this->client = $client; }
    public function activeTab($side = 'source') { return $this->client->call('salamander.sides.activeTab', array('side' => $side)); }
    public function context($side = 'source') { return $this->client->call('salamander.sides.context', array('side' => $side)); }
    public function tabs($side = 'source') { $r = $this->client->call('salamander.sides.tabs', array('side' => $side)); return isset($r['tabs']) ? $r['tabs'] : array(); }
    public function activateTab($tabId, $focus = true) { $r = $this->client->call('salamander.sides.activateTab', array('tabId' => (string)$tabId, 'focus' => (bool)$focus)); return !empty($r['activated']); }
    public function changePath($path, $side = 'source') { return $this->client->call('salamander.sides.changePath', array('side' => $side, 'path' => $path)); }
    public function refresh($side = 'source', $force = false, $focusFirstNewItem = false) { $r = $this->client->call('salamander.sides.refresh', array('side' => $side, 'force' => (bool)$force, 'focusFirstNewItem' => (bool)$focusFirstNewItem)); return !empty($r['ok']); }
    public function selectItem($index, $select = true, $side = 'source', $repaint = true) { $r = $this->client->call('salamander.sides.selectItem', array('side' => $side, 'index' => (int)$index, 'select' => (bool)$select, 'repaint' => (bool)$repaint)); return !empty($r['changed']); }
    public function selectAll($select = true, $side = 'source', $repaint = true) { $r = $this->client->call('salamander.sides.selectAll', array('side' => $side, 'select' => (bool)$select, 'repaint' => (bool)$repaint)); return !empty($r['changed']); }
    public function focusItem($index, $side = 'source', $partVisible = true) { $r = $this->client->call('salamander.sides.focusItem', array('side' => $side, 'index' => (int)$index, 'partVisible' => (bool)$partVisible)); return !empty($r['changed']); }
}
class SalamatrixSideView {
    private $sides; private $name;
    public function __construct($sides, $name) { $this->sides = $sides; $this->name = $name; }
    public function activeTab() { return $this->sides->activeTab($this->name); }
    public function context() { return $this->sides->context($this->name); }
    public function tabs() { return $this->sides->tabs($this->name); }
    public function activateTab($tabId, $focus = true) { return $this->sides->activateTab($tabId, $focus); }
    public function changePath($path) { return $this->sides->changePath($path, $this->name); }
    public function refresh($force = false, $focusFirstNewItem = false) { return $this->sides->refresh($this->name, $force, $focusFirstNewItem); }
    public function selectItem($index, $select = true, $repaint = true) { return $this->sides->selectItem($index, $select, $this->name, $repaint); }
    public function selectAll($select = true, $repaint = true) { return $this->sides->selectAll($select, $this->name, $repaint); }
    public function focusItem($index, $partVisible = true) { return $this->sides->focusItem($index, $this->name, $partVisible); }
}
class SalamatrixUi {
    private $client; public function __construct($client) { $this->client = $client; }
    public function messageBox($message, $title = 'Salamander') { $r = $this->client->call('salamander.ui.messageBox', array('message' => $message, 'title' => $title)); return isset($r['result']) ? $r['result'] : 0; }
    public function inputBox($prompt, $title = 'Salamander', $initial = '') { return $this->client->call('salamander.ui.inputBox', array('prompt' => $prompt, 'title' => $title, 'initial' => $initial)); }
    public function pickFile($save = false, $title = '', $filter = '', $initial = '') { return $this->client->call('salamander.ui.pickFile', array('save' => (bool)$save, 'title' => $title, 'filter' => $filter, 'initial' => $initial)); }
    public function pickFolder($title = '', $initial = '') { return $this->client->call('salamander.ui.pickFolder', array('title' => $title, 'initial' => $initial)); }
    public function progress($title = 'Salamatrix', $total = 0, $twoProgressBars = false, $fileProgress = false, $cancelEnabled = true, $total2 = null) {
        $args = array('title' => $title, 'total' => (int)$total, 'twoProgressBars' => (bool)$twoProgressBars, 'fileProgress' => (bool)$fileProgress, 'cancelEnabled' => (bool)$cancelEnabled);
        if ($total2 !== null) $args['total2'] = (int)$total2;
        $r = $this->client->call('salamander.ui.progress.create', $args);
        return new SalamatrixProgress($this->client, (string)$r['progressId']);
    }
    public function dialog($title = 'Salamander', $width = 320, $height = 180) { $r = $this->client->call('salamander.ui.dialog.create', array('title' => $title, 'width' => (int)$width, 'height' => (int)$height)); return new SalamatrixDialog($this->client, (string)$r['dialogId']); }
}
class SalamatrixProgress {
    private $client; private $id; private $closed = false;
    public function __construct($client, $id) { $this->client = $client; $this->id = $id; }
    public function update($position, $total = null, $text = '', $delayedPaint = true, $position2 = null, $total2 = null) {
        $args = array('progressId' => $this->id, 'position' => (int)$position, 'text' => $text, 'delayedPaint' => (bool)$delayedPaint);
        if ($total !== null) $args['total'] = (int)$total;
        if ($position2 !== null) $args['position2'] = (int)$position2;
        if ($total2 !== null) $args['total2'] = (int)$total2;
        $r = $this->client->call('salamander.ui.progress.update', $args);
        return !isset($r['continued']) || !empty($r['continued']);
    }
    public function step($amount = 1, $delayedPaint = true) {
        $r = $this->client->call('salamander.ui.progress.step', array('progressId' => $this->id, 'amount' => (int)$amount, 'delayedPaint' => (bool)$delayedPaint));
        return !isset($r['continued']) || !empty($r['continued']);
    }
    public function setTotals($total, $total2) { $this->client->call('salamander.ui.progress.setTotals', array('progressId' => $this->id, 'total' => (int)$total, 'total2' => (int)$total2)); }
    public function setPositions($position, $position2, $delayedPaint = true) {
        $r = $this->client->call('salamander.ui.progress.setPositions', array('progressId' => $this->id, 'position' => (int)$position, 'position2' => (int)$position2, 'delayedPaint' => (bool)$delayedPaint));
        return !isset($r['continued']) || !empty($r['continued']);
    }
    public function setTitle($title) { $this->client->call('salamander.ui.progress.setTitle', array('progressId' => $this->id, 'title' => $title)); }
    public function setCancelEnabled($enabled) { $this->client->call('salamander.ui.progress.setCancelEnabled', array('progressId' => $this->id, 'enabled' => (bool)$enabled)); }
    public function isCancelled() { $r = $this->client->call('salamander.ui.progress.cancelled', array('progressId' => $this->id)); return !empty($r['cancelled']); }
    public function close() { if (!$this->closed) { $this->client->call('salamander.ui.progress.close', array('progressId' => $this->id)); $this->closed = true; } }
}
class SalamatrixClipboard {
    private $client; public function __construct($client) { $this->client = $client; }
    public function copyText($text, $showEcho = false) { $r = $this->client->call('salamander.clipboard.copyText', array('text' => $text, 'showEcho' => $showEcho)); return !empty($r['copied']); }
}
class SalamatrixDialog {
    private $client; private $id;
    public function __construct($client, $id) { $this->client = $client; $this->id = $id; }
    private function add($kind, $controlId, $text = '', $extra = array()) { $args = array('dialogId' => $this->id, 'kind' => $kind, 'controlId' => $controlId, 'text' => $text); foreach ($extra as $key => $value) $args[$key] = $value; $this->client->call('salamander.ui.dialog.add', $args); }
    public function addControl($kind, $id, $text = '', $readOnly = false, $checked = false, $dialogResult = 0, $layout = array(), $keepOpen = false, $multiline = false) {
        $extra = array('readOnly' => (bool)$readOnly, 'checked' => (bool)$checked, 'dialogResult' => (int)$dialogResult, 'keepOpen' => (bool)$keepOpen, 'multiline' => (bool)$multiline);
        foreach (array('x', 'y', 'width', 'height') as $name) if (is_array($layout) && array_key_exists($name, $layout)) $extra[$name] = (int)$layout[$name];
        $this->add($kind, $id, $text, $extra);
    }
    public function setValidation($id, $required = false, $message = '') { $this->client->call('salamander.ui.dialog.validation', array('dialogId' => $this->id, 'controlId' => $id, 'required' => (bool)$required, 'message' => $message)); }
    public function onChange($handler) {
        $event = 'salamander.ui.dialog.' . $this->id . '.changed';
        $this->client->call('salamander.ui.dialog.events', array('dialogId' => $this->id, 'enabled' => true, 'event' => $event));
        if (!isset($this->client->handlers[$event])) $this->client->handlers[$event] = array();
        $this->client->handlers[$event][] = $handler;
        return $event;
    }
    public function offChange($event = '') {
        if ($event === '') $event = 'salamander.ui.dialog.' . $this->id . '.changed';
        $this->client->call('salamander.ui.dialog.events', array('dialogId' => $this->id, 'enabled' => false, 'event' => $event));
        unset($this->client->handlers[$event]);
    }
    public function addLabel($id, $text) { $this->add('label', $id, $text); }
    public function addTextBox($id, $text = '', $readOnly = false, $multiline = false) { $this->add('textbox', $id, $text, array('readOnly' => $readOnly, 'multiline' => (bool)$multiline)); }
    public function addCheckBox($id, $text, $checked = false) { $this->add('checkbox', $id, $text, array('checked' => $checked)); }
    public function addRadioButton($id, $text, $checked = false) { $this->add('radio', $id, $text, array('checked' => $checked)); }
    public function addComboBox($id, $text = '') { $this->add('combobox', $id, $text); }
    public function addListView($id) { $this->add('listview', $id); }
    public function addTreeView($id) { $this->add('treeview', $id); }
    public function addTabControl($id) { $this->add('tabcontrol', $id); }
    public function addItem($controlId, $text, $parentIndex = -1) { $r = $this->client->call('salamander.ui.dialog.item', array('dialogId' => $this->id, 'controlId' => $controlId, 'text' => $text, 'parentIndex' => $parentIndex)); return isset($r['itemCount']) ? $r['itemCount'] : 0; }
    public function addColumn($controlId, $title, $width = 180) { $this->client->call('salamander.ui.dialog.column', array('dialogId' => $this->id, 'controlId' => $controlId, 'title' => $title, 'width' => (int)$width)); }
    public function setSelectedIndex($controlId, $index = -1) { $r = $this->client->call('salamander.ui.dialog.selection', array('dialogId' => $this->id, 'controlId' => $controlId, 'index' => (int)$index)); return isset($r['selectedIndex']) ? $r['selectedIndex'] : -1; }
    public function clearItems($controlId) { $this->client->call('salamander.ui.dialog.clearItems', array('dialogId' => $this->id, 'controlId' => $controlId)); }
    public function addButton($id, $text, $dialogResult = 1, $keepOpen = false) { $this->add('button', $id, $text, array('dialogResult' => $dialogResult, 'keepOpen' => (bool)$keepOpen)); }
    public function show() { $r = $this->client->call('salamander.ui.dialog.show', array('dialogId' => $this->id)); return isset($r['result']) ? $r['result'] : 0; }
    public function get($id) { return $this->client->call('salamander.ui.dialog.get', array('dialogId' => $this->id, 'controlId' => $id)); }
    public function set($id, $value) { $this->client->call('salamander.ui.dialog.set', array('dialogId' => $this->id, 'controlId' => $id, 'value' => $value)); }
    public function close() { $this->client->call('salamander.ui.dialog.destroy', array('dialogId' => $this->id)); }
}
class SalamatrixAi {
    private $client; public function __construct($client) { $this->client = $client; }
    public function api($topic = null) {
        $arguments = array();
        if ($topic !== null && $topic !== '') $arguments['topic'] = $topic;
        return $this->client->call('salamander.ai.api', $arguments);
    }
    public function apiDescription($topic = null) { return $this->api($topic); }
    public function generate($prompt, $context = null, $provider = null, $runtime = null, $existingScript = null, $feedback = null) {
        $arguments = array('prompt' => $prompt);
        if ($context !== null) $arguments['context'] = $context;
        if ($provider !== null) $arguments['provider'] = $provider;
        if ($runtime !== null) $arguments['runtime'] = $runtime;
        if ($existingScript !== null) $arguments['existingScript'] = $existingScript;
        if ($feedback !== null) $arguments['feedback'] = $feedback;
        return $this->client->call('salamander.ai.generate', $arguments);
    }
    public function preview($prompt, $context = null, $provider = null, $runtime = null, $existingScript = null, $feedback = null) {
        $arguments = array('prompt' => $prompt);
        if ($context !== null) $arguments['context'] = $context;
        if ($provider !== null) $arguments['provider'] = $provider;
        if ($runtime !== null) $arguments['runtime'] = $runtime;
        if ($existingScript !== null) $arguments['existingScript'] = $existingScript;
        if ($feedback !== null) $arguments['feedback'] = $feedback;
        return $this->client->call('salamander.ai.preview', $arguments);
    }
}
class SalamatrixEvents {
    private $client; public function __construct($client) { $this->client = $client; }
    public function subscribe($event, $handler) {
        $r = $this->client->call('salamander.events.subscribe', array('event' => $event));
        $id = (string)$r['subscriptionId'];
        if (!isset($this->client->handlers[$event])) $this->client->handlers[$event] = array();
        $this->client->handlers[$event][] = $handler;
        $this->client->subscriptions[$id] = $event;
        return $id;
    }
    public function unsubscribe($id) { $this->client->call('salamander.events.unsubscribe', array('subscriptionId' => (string)$id)); unset($this->client->subscriptions[(string)$id]); }
}

$entry = null;
$commandId = '';
$oneShot = false;
for ($i = 1; $i < count($argv); ++$i) {
    if ($argv[$i] === '--entry' && isset($argv[$i + 1])) $entry = $argv[++$i];
    elseif ($argv[$i] === '--command-id' && isset($argv[$i + 1])) $commandId = $argv[++$i];
    elseif ($argv[$i] === '--one-shot') $oneShot = true;
}
class SalamatrixRuntimes {
    private $client; public function __construct($client) { $this->client = $client; }
    public function list() { $r = $this->client->call('salamander.runtimes.list', array()); return isset($r['runtimes']) ? $r['runtimes'] : array(); }
}
if ($entry === null) throw new RuntimeException('Missing --entry');
$client = new SalamatrixClient();
smx_send('hello', 0, array('protocol' => 1, 'runtime' => 'php'));
do { $hello = smx_read(); } while ($hello['kind'] !== 'result' || $hello['id'] !== 0);
if (isset($hello['payload']['ok']) && !$hello['payload']['ok']) throw new RuntimeException('Salamander host rejected the worker');

$Salamander = new stdClass();
$Salamander->command_id = $commandId;
$Salamander->commands = new SalamatrixCommands($client);
$Salamander->storage = new SalamatrixStorage($client);
$Salamander->file_operations = new SalamatrixFileOperations($client);
$Salamander->sides = new SalamatrixSides($client);
$Salamander->left_side = new SalamatrixSideView($Salamander->sides, 'left');
$Salamander->right_side = new SalamatrixSideView($Salamander->sides, 'right');
$Salamander->source_side = new SalamatrixSideView($Salamander->sides, 'source');
$Salamander->target_side = new SalamatrixSideView($Salamander->sides, 'target');
$Salamander->ui = new SalamatrixUi($client);
$Salamander->clipboard = new SalamatrixClipboard($client);
$Salamander->ai = new SalamatrixAi($client);
$Salamander->events = new SalamatrixEvents($client);
$Salamander->runtimes = new SalamatrixRuntimes($client);
include $entry;

if ($oneShot) exit(0);

while (true) {
    $frame = smx_read();
    if ($frame['kind'] === 'event') { $client->dispatchEvent($frame['payload']); continue; }
    if ($frame['kind'] === 'shutdown') break;
}
