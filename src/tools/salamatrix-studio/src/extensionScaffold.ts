import type { ExtensionManifest } from './manifestModel.js';
import { serializeMenuDocument, type MenuDocument } from './menuModel.js';
import type { RuntimeId } from './model.js';

export interface ExtensionScaffoldSpec {
  id: string;
  name: string;
  description: string;
  runtime: Exclude<RuntimeId, 'Native.Cpp'>;
}

export interface ScaffoldFile {
  path: string;
  content: string;
}

interface RuntimeTemplate {
  entryPoint: string;
  generatedPath: string;
  entry(spec: ExtensionScaffoldSpec): string;
  generated(actionsJson: string): string;
}

export const extensionRuntimeIds: ExtensionScaffoldSpec['runtime'][] = [
  'PowerShell', 'Python.CPython', 'JavaScript.Node', 'PHP.CLI', 'Lua', 'Automation.JScript',
];

export function validateExtensionId(value: string): string | undefined {
  return /^[A-Za-z][A-Za-z0-9_-]*(\.[A-Za-z0-9][A-Za-z0-9_-]*)+$/.test(value)
    ? undefined
    : 'Use a dotted identifier such as MyCompany.MyExtension.';
}

export function validateExtensionFolderName(value: string): string | undefined {
  return value && !/[\\/:*?"<>|]/.test(value) && value !== '.' && value !== '..'
    ? undefined
    : 'Enter one safe folder name.';
}

export function createExtensionScaffold(spec: ExtensionScaffoldSpec): ScaffoldFile[] {
  const invalid = validateExtensionId(spec.id);
  if (invalid) throw new Error(invalid);
  if (!spec.name.trim()) throw new Error('Extension name is required.');
  const template = runtimeTemplates[spec.runtime];
  const command = {
    id: `${spec.id}.run`, title: `Run ${spec.name}`, handler: 'run', menu: 'plugin' as const,
    contextMenu: false, toolbar: false, requires: 'any', enabled: true, visible: true,
  };
  const manifest: ExtensionManifest = {
    schema: 1, id: spec.id, name: spec.name, version: '1.0.0', description: spec.description,
    runtime: spec.runtime, entryPoint: template.entryPoint, icon: 'icon.svg', iconDark: 'icon-dark.svg',
    capabilities: ['ui.notify'], commands: [command],
  };
  const menu: MenuDocument = {
    schema: 1, generatedBy: 'SalamatrixStudio', commands: [{ handler: 'run', action: 'custom' }],
  };
  return [
    { path: 'extension.json', content: `${JSON.stringify(manifest, null, 2)}\n` },
    { path: template.entryPoint, content: template.entry(spec) },
    { path: template.generatedPath, content: template.generated(JSON.stringify(menu.commands)) },
    { path: '.salamatrix/menu.json', content: serializeMenuDocument(menu) },
    { path: 'icon.svg', content: icon('#2979c7', '#ffffff') },
    { path: 'icon-dark.svg', content: icon('#62aef7', '#18212a') },
  ];
}

export async function findScaffoldConflicts(
  files: ScaffoldFile[], exists: (relativePath: string) => Promise<boolean>,
): Promise<string[]> {
  const results = await Promise.all(files.map(async (file) => ({ file, exists: await exists(file.path) })));
  return results.filter((result) => result.exists).map((result) => result.file.path);
}

export function generatedMenuPath(runtime: ExtensionScaffoldSpec['runtime']): string {
  return runtimeTemplates[runtime].generatedPath;
}

export function generateMenuActions(runtime: ExtensionScaffoldSpec['runtime'], document: MenuDocument): string {
  const actions = document.commands.map((action) => ({
    ...action,
    target: action.target ?? '',
    arguments: action.arguments ?? '',
    workingDirectory: action.workingDirectory ?? '',
  }));
  return runtimeTemplates[runtime].generated(JSON.stringify(actions));
}

export const menuDispatchMarker = 'SALAMATRIX-STUDIO-MENU-DISPATCH';

export function integrateMenuDispatch(runtime: ExtensionScaffoldSpec['runtime'], current: string): string {
  if (current.includes(menuDispatchMarker)) return current;
  const snippet = integrationSnippets[runtime];
  if (runtime === 'PHP.CLI') {
    if (!current.trimStart().startsWith('<?php')) throw new Error('The PHP entry point must start with <?php before generated menu actions can be enabled.');
    const marker = current.indexOf('<?php') + '<?php'.length;
    return `${current.slice(0, marker)}\n${snippet}${current.slice(marker)}`;
  }
  return `${snippet}${current}`;
}

const runtimeTemplates: Record<ExtensionScaffoldSpec['runtime'], RuntimeTemplate> = {
  PowerShell: {
    entryPoint: 'main.ps1', generatedPath: 'generated/menu-actions.generated.ps1',
    entry: (spec) => `${integrationSnippets.PowerShell}if ($Salamander.command_handler -eq 'run') {\n    $Salamander.ui.Notify('Hello from ${ps(spec.name)}.', '${ps(spec.name)}', 2500)\n}\n`,
    generated: generatePowerShell,
  },
  'Python.CPython': {
    entryPoint: 'main.py', generatedPath: 'generated/menu_actions_generated.py',
    entry: (spec) => `${integrationSnippets['Python.CPython']}if Salamander.command_handler == "run":\n    Salamander.ui.notify("Hello from ${py(spec.name)}.", "${py(spec.name)}", 2500)\n`,
    generated: generatePython,
  },
  'JavaScript.Node': {
    entryPoint: 'main.mjs', generatedPath: 'generated/menu-actions.generated.mjs',
    entry: (spec) => `${integrationSnippets['JavaScript.Node']}if (!(await handleGeneratedMenuAction(Salamander)) && Salamander.command_handler === "run") {\n  await Salamander.ui.notify("Hello from ${js(spec.name)}.", "${js(spec.name)}", 2500);\n}\n`,
    generated: generateNode,
  },
  'PHP.CLI': {
    entryPoint: 'main.php', generatedPath: 'generated/menu-actions.generated.php',
    entry: (spec) => `<?php\n${integrationSnippets['PHP.CLI']}if (!$handledByStudio && $Salamander->command_handler === 'run') {\n    $Salamander->ui->notify('Hello from ${php(spec.name)}.', '${php(spec.name)}', 2500);\n}\n`,
    generated: generatePhp,
  },
  Lua: {
    entryPoint: 'main.lua', generatedPath: 'generated/menu-actions.generated.lua',
    entry: (spec) => `${integrationSnippets.Lua}if not handled_by_studio and Salamander.command_handler == "run" then\n    Salamander.ui.notify("Hello from ${lua(spec.name)}.", "${lua(spec.name)}", 2500)\nend\n`,
    generated: generateLua,
  },
  'Automation.JScript': {
    entryPoint: 'main.js', generatedPath: 'generated/menu-actions.generated.js',
    entry: (spec) => `${integrationSnippets['Automation.JScript']}if (!handledByStudio) {\n    Salamander.MsgBox("Hello from ${js(spec.name)}.", 64, "${js(spec.name)}");\n}\n`,
    generated: generateJScript,
  },
};

const integrationSnippets: Record<ExtensionScaffoldSpec['runtime'], string> = {
  PowerShell: `# ${menuDispatchMarker}\n. (Join-Path $PSScriptRoot 'generated/menu-actions.generated.ps1')\nif (Invoke-SalamatrixStudioMenuAction $Salamander) { return }\n\n`,
  'Python.CPython': `# ${menuDispatchMarker}\nfrom generated.menu_actions_generated import handle_generated_menu_action\nif handle_generated_menu_action(Salamander):\n    raise SystemExit(0)\n\n`,
  'JavaScript.Node': `// ${menuDispatchMarker}\nimport { handleGeneratedMenuAction } from "./generated/menu-actions.generated.mjs";\n\n`,
  'PHP.CLI': `// ${menuDispatchMarker}\nrequire_once __DIR__ . '/generated/menu-actions.generated.php';\n$handledByStudio = handle_salamatrix_studio_menu_action($Salamander);\n\n`,
  Lua: `-- ${menuDispatchMarker}\nlocal studio_source = debug.getinfo(1, "S").source:sub(2)\nlocal studio_root = studio_source:match("^(.*)[/\\\\]") or "."\nlocal handle_generated_menu_action = dofile(studio_root .. "/generated/menu-actions.generated.lua")\nlocal handled_by_studio = handle_generated_menu_action(Salamander)\n\n`,
  'Automation.JScript': `// ${menuDispatchMarker}\nvar studioFso = new ActiveXObject("Scripting.FileSystemObject");\nvar studioGeneratedPath = studioFso.BuildPath(studioFso.GetParentFolderName(Salamander.Script.Path), "generated\\\\menu-actions.generated.js");\nvar studioGeneratedFile = new ActiveXObject("ADODB.Stream");\nstudioGeneratedFile.Type = 2;\nstudioGeneratedFile.Charset = "utf-8";\nstudioGeneratedFile.Open();\nstudioGeneratedFile.LoadFromFile(studioGeneratedPath);\neval(studioGeneratedFile.ReadText());\nstudioGeneratedFile.Close();\nvar handledByStudio = handleSalamatrixStudioMenuAction(Salamander);\n\n`,
};

function generatePowerShell(json: string): string {
  return `# Generated by Salamatrix Studio. Changes will be overwritten.\n$script:SalamatrixStudioActions = '${ps(json)}' | ConvertFrom-Json\nfunction Invoke-SalamatrixStudioMenuAction {\n param($Salamander)\n $a = @($script:SalamatrixStudioActions | Where-Object { $_.handler -eq $Salamander.command_handler } | Select-Object -First 1)\n if ($a.Count -eq 0 -or $a[0].action -eq 'custom') { return $false }\n $a = $a[0]; $target = [Environment]::ExpandEnvironmentVariables([string]$a.target); $args = [Environment]::ExpandEnvironmentVariables([string]$a.arguments); $parameters = @{}; if (-not [string]::IsNullOrWhiteSpace([string]$a.workingDirectory)) { $parameters.WorkingDirectory = [string]$a.workingDirectory }\n if ($a.action -eq 'open') { Start-Process $target @parameters } elseif ($a.action -eq 'command') { Start-Process $env:ComSpec -ArgumentList @('/d','/c',($target + ' ' + $args)) @parameters } elseif ($a.action -eq 'powershell') { Start-Process 'powershell.exe' -ArgumentList ('-NoProfile -ExecutionPolicy Bypass -File "' + $target + '" ' + $args) @parameters } else { Start-Process $target -ArgumentList $args @parameters }\n return $true\n}\n`;
}

function generatePython(json: string): string {
  return `# Generated by Salamatrix Studio. Changes will be overwritten.\nimport json, os, subprocess\nACTIONS = json.loads(${JSON.stringify(json)})\ndef handle_generated_menu_action(Salamander):\n    action = next((x for x in ACTIONS if x.get("handler") == Salamander.command_handler), None)\n    if not action or action.get("action") == "custom": return False\n    target=os.path.expandvars(action.get("target", "")); args=os.path.expandvars(action.get("arguments", "")); cwd=action.get("workingDirectory") or None\n    if action["action"] == "open": os.startfile(target)\n    elif action["action"] == "command": subprocess.Popen(target + " " + args, cwd=cwd, shell=True)\n    elif action["action"] == "powershell": subprocess.Popen('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + target + '" ' + args, cwd=cwd, shell=True)\n    else: subprocess.Popen(target + " " + args, cwd=cwd, shell=True)\n    return True\n`;
}

function generateNode(json: string): string {
  return `// Generated by Salamatrix Studio. Changes will be overwritten.\nimport { spawn } from "node:child_process";\nconst actions = JSON.parse(${JSON.stringify(json)});\nexport async function handleGeneratedMenuAction(Salamander) {\n  const a = actions.find((item) => item.handler === Salamander.command_handler); if (!a || a.action === "custom") return false;\n  const target = expand(a.target || ""), args = expand(a.arguments || ""), cwd = a.workingDirectory || undefined;\n  if (a.action === "open") spawn("cmd.exe", ["/d", "/c", "start", "", target], { detached: true, cwd });\n  else if (a.action === "command") spawn(target + " " + args, { shell: true, detached: true, cwd });\n  else if (a.action === "powershell") spawn('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + target + '" ' + args, { shell: true, detached: true, cwd });\n  else spawn(target + " " + args, { shell: true, detached: true, cwd }); return true;\n}\nfunction expand(value) { return value.replace(/%([^%]+)%/g, (_, key) => process.env[key] || ""); }\n`;
}

function generatePhp(json: string): string {
  return `<?php\n// Generated by Salamatrix Studio. Changes will be overwritten.\nfunction handle_salamatrix_studio_menu_action($Salamander) {\n $actions=json_decode('${php(json)}', true); $a=null; foreach($actions as $item) if($item['handler']===$Salamander->command_handler){$a=$item;break;} if(!$a || $a['action']==='custom') return false;\n $target=$a['target']??''; $args=$a['arguments']??''; $command=$a['action']==='open'?'start "" "'.$target.'"':($a['action']==='powershell'?'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "'.$target.'" '.$args:$target.' '.$args); pclose(popen($command,'r')); return true;\n}\n`;
}

function generateLua(json: string): string {
  const actions = JSON.parse(json) as Array<Record<string, string>>;
  const rows = actions.map((action) => `  { handler = "${lua(action.handler ?? '')}", action = "${lua(action.action ?? '')}", target = "${lua(action.target ?? '')}", arguments = "${lua(action.arguments ?? '')}", workingDirectory = "${lua(action.workingDirectory ?? '')}" }`).join(',\n');
  return `-- Generated by Salamatrix Studio. Changes will be overwritten.\nlocal actions = {\n${rows}\n}\nreturn function(Salamander)\n  local selected = nil\n  for _, item in ipairs(actions) do if item.handler == Salamander.command_handler then selected = item; break end end\n  if not selected or selected.action == "custom" then return false end\n  local command = selected.action == "open" and ('start "" "' .. selected.target .. '"') or selected.action == "powershell" and ('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' .. selected.target .. '" ' .. selected.arguments) or (selected.target .. ' ' .. selected.arguments)\n  os.execute(command); return true\nend\n`;
}

function generateJScript(json: string): string {
  return `// Generated by Salamatrix Studio. Changes will be overwritten.\nvar salamatrixStudioActions = ${json};\nfunction handleSalamatrixStudioMenuAction(Salamander) {\n var currentHandler = ""; try { currentHandler = Salamander.command_handler; } catch (ignored) {}\n var a = null; for (var i=0;i<salamatrixStudioActions.length;i++){if(salamatrixStudioActions[i].handler===currentHandler){a=salamatrixStudioActions[i];break;}}\n if(!a && !currentHandler && salamatrixStudioActions.length===1)a=salamatrixStudioActions[0];if(!a || a.action==='custom')return false;var shell=new ActiveXObject('WScript.Shell');if(a.workingDirectory)shell.CurrentDirectory=a.workingDirectory;var line=a.action==='open'?'cmd.exe /d /c start "" "'+a.target+'"':a.action==='powershell'?'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "'+a.target+'" '+(a.arguments||''):(a.target+' '+(a.arguments||''));shell.Run(line,1,false);return true;\n}\n`;
}

function icon(background: string, foreground: string): string {
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64"><rect x="7" y="8" width="50" height="48" rx="5" fill="${background}"/><path fill="${foreground}" d="M16 18h8v8h-8zm13 1h19v6H29zM16 31h8v8h-8zm13 1h19v6H29zM16 44h8v4h-8zm13 0h12v4H29z"/></svg>\n`;
}
function ps(value: string): string { return value.replaceAll("'", "''"); }
function py(value: string): string { return value.replaceAll('\\', '\\\\').replaceAll('"', '\\"'); }
function js(value: string): string { return value.replaceAll('\\', '\\\\').replaceAll('"', '\\"'); }
function php(value: string): string { return value.replaceAll('\\', '\\\\').replaceAll("'", "\\'"); }
function lua(value: string): string { return value.replaceAll('\\', '\\\\').replaceAll('"', '\\"').replaceAll('\n', '\\n'); }
