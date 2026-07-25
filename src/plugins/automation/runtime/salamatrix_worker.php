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
    public function register($id, $title, $pluginMenu = true, $contextMenu = false) { $r = $this->client->call('salamander.commands.register', array('commandId' => $id, 'title' => $title, 'pluginMenu' => $pluginMenu, 'contextMenu' => $contextMenu)); return !empty($r['registered']); }
    public function unregister($id) { $r = $this->client->call('salamander.commands.unregister', array('commandId' => $id)); return !empty($r['unregistered']); }
}
class SalamatrixStorage {
    private $client; public function __construct($client) { $this->client = $client; }
    public function get($key, $default = null) { $r = $this->client->call('salamander.storage.get', array('key' => $key)); return isset($r['type']) && $r['type'] === 'string' ? $r['value'] : $default; }
    public function set($key, $value) { $this->client->call('salamander.storage.set', array('key' => $key, 'value' => $value)); }
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
}
class SalamatrixUi {
    private $client; public function __construct($client) { $this->client = $client; }
    public function messageBox($message, $title = 'Salamander') { $r = $this->client->call('salamander.ui.messageBox', array('message' => $message, 'title' => $title)); return isset($r['result']) ? $r['result'] : 0; }
    public function inputBox($prompt, $title = 'Salamander', $initial = '') { return $this->client->call('salamander.ui.inputBox', array('prompt' => $prompt, 'title' => $title, 'initial' => $initial)); }
    public function dialog($title = 'Salamander') { $r = $this->client->call('salamander.ui.dialog.create', array('title' => $title)); return new SalamatrixDialog($this->client, (string)$r['dialogId']); }
}
class SalamatrixDialog {
    private $client; private $id;
    public function __construct($client, $id) { $this->client = $client; $this->id = $id; }
    private function add($kind, $controlId, $text, $extra = array()) { $args = array('dialogId' => $this->id, 'kind' => $kind, 'controlId' => $controlId, 'text' => $text); foreach ($extra as $key => $value) $args[$key] = $value; $this->client->call('salamander.ui.dialog.add', $args); }
    public function addLabel($id, $text) { $this->add('label', $id, $text); }
    public function addTextBox($id, $text = '', $readOnly = false) { $this->add('textbox', $id, $text, array('readOnly' => $readOnly)); }
    public function addCheckBox($id, $text, $checked = false) { $this->add('checkbox', $id, $text, array('checked' => $checked)); }
    public function addRadioButton($id, $text, $checked = false) { $this->add('radio', $id, $text, array('checked' => $checked)); }
    public function addComboBox($id, $text = '') { $this->add('combobox', $id, $text); }
    public function addListView($id) { $this->add('listview', $id); }
    public function addTreeView($id) { $this->add('treeview', $id); }
    public function addButton($id, $text, $dialogResult = 1) { $this->add('button', $id, $text, array('dialogResult' => $dialogResult)); }
    public function show() { $r = $this->client->call('salamander.ui.dialog.show', array('dialogId' => $this->id)); return isset($r['result']) ? $r['result'] : 0; }
    public function get($id) { return $this->client->call('salamander.ui.dialog.get', array('dialogId' => $this->id, 'controlId' => $id)); }
    public function close() { $this->client->call('salamander.ui.dialog.destroy', array('dialogId' => $this->id)); }
}
class SalamatrixAi {
    private $client; public function __construct($client) { $this->client = $client; }
    public function generate($prompt, $context = null, $provider = null) {
        $arguments = array('prompt' => $prompt);
        if ($context !== null) $arguments['context'] = $context;
        if ($provider !== null) $arguments['provider'] = $provider;
        return $this->client->call('salamander.ai.generate', $arguments);
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
for ($i = 1; $i < count($argv); ++$i) if ($argv[$i] === '--entry' && isset($argv[$i + 1])) $entry = $argv[++$i];
if ($entry === null) throw new RuntimeException('Missing --entry');
$client = new SalamatrixClient();
smx_send('hello', 0, array('protocol' => 1, 'runtime' => 'php'));
do { $hello = smx_read(); } while ($hello['kind'] !== 'result' || $hello['id'] !== 0);
if (isset($hello['payload']['ok']) && !$hello['payload']['ok']) throw new RuntimeException('Salamander host rejected the worker');

$Salamander = new stdClass();
$Salamander->commands = new SalamatrixCommands($client);
$Salamander->storage = new SalamatrixStorage($client);
$Salamander->file_operations = new SalamatrixFileOperations($client);
$Salamander->sides = new SalamatrixSides($client);
$Salamander->left_side = $Salamander->sides;
$Salamander->right_side = $Salamander->sides;
$Salamander->source_side = $Salamander->sides;
$Salamander->target_side = $Salamander->sides;
$Salamander->ui = new SalamatrixUi($client);
$Salamander->ai = new SalamatrixAi($client);
$Salamander->events = new SalamatrixEvents($client);
include $entry;

while (true) {
    $frame = smx_read();
    if ($frame['kind'] === 'event') { $client->dispatchEvent($frame['payload']); continue; }
    if ($frame['kind'] === 'shutdown') break;
}
