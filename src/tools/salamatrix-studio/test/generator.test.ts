import { describe, expect, it } from 'vitest';
import { generateDialog } from '../src/generator.js';
import { createDialogDocument } from '../src/model.js';

const dialog = createDialogDocument('settings', 'Settings');

function onlyFile(runtime: Parameters<typeof generateDialog>[1]) {
  return generateDialog(dialog, runtime).files[0]!;
}

describe('dialog generators', () => {
  it('generates a PowerShell module using the public facade', () => {
    const generated = onlyFile('PowerShell');
    expect(generated.fileName).toBe('settings-dialog.generated.ps1');
    expect(generated.content).toContain("$Salamander.ui.Dialog('Settings', 420, 240)");
    expect(generated.content).toContain("$dialog.AddControl('button', 'ok'");
    expect(generated.content).toContain('$dialog.Close()');
  });

  it('generates an asynchronous Node module', () => {
    const generated = onlyFile('JavaScript.Node');
    expect(generated.fileName).toBe('settings-dialog.generated.mjs');
    expect(generated.content).toContain('await Salamander.ui.dialog("Settings"');
    expect(generated.content).toContain('await dialog.addControl("button", "ok"');
  });

  it('generates a Python module', () => {
    const generated = onlyFile('Python.CPython');
    expect(generated.fileName).toBe('settings_dialog_generated.py');
    expect(generated.content).toContain('dialog = Salamander.ui.dialog("Settings", 420, 240)');
    expect(generated.content).toContain('dialog.add_control("button", "ok"');
  });

  it('generates PHP and Lua modules using their public facades', () => {
    const php = onlyFile('PHP.CLI');
    expect(php.fileName).toBe('settings_dialog_generated.php');
    expect(php.content).toContain("$Salamander->ui->dialog('Settings', 420, 240)");
    expect(php.content).toContain("$dialog->addControl('button', 'ok'");

    const lua = onlyFile('Lua');
    expect(lua.fileName).toBe('settings_dialog_generated.lua');
    expect(lua.content).toContain('Salamander.ui.dialog("Settings", 420, 240)');
    expect(lua.content).toContain('dialog.add_control("button", "ok"');
    expect(lua.content).toContain('dialog_result = 1');
  });

  it('generates the supported Automation COM surface and rejects unsupported controls', () => {
    const automation = onlyFile('Automation.JScript');
    expect(automation.content).toContain('Salamander.UI.dialog("Settings", 420, 240)');
    expect(automation.content).toContain('dialog.add("button", "ok"');

    const unsupported = createDialogDocument('input', 'Input');
    unsupported.controls.push({ kind: 'textbox', id: 'value', text: '', bounds: { x: 7, y: 7, width: 100, height: 14 } });
    expect(() => generateDialog(unsupported, 'Automation.JScript')).toThrow(/does not expose.*textbox/);
  });

  it('generates a native C++ header and implementation', () => {
    const nativeDialog = createDialogDocument('settings', 'Settings');
    nativeDialog.controls[0]!.options = {
      styleFlags: 20, pathSeparator: '/', toolTip: 'Tip', actionOpen: 'https://www.altap.cz',
      actionCommand: 32513, actionHint: 'Hint', progress: 120, indeterminateDuration: -1,
      indeterminateInterval: 100, textColor: 1, backgroundColor: 2,
      alignControlId: 'list', buttonMask: 49,
    };
    const generated = generateDialog(nativeDialog, 'Native.Cpp');
    expect(generated.files.map((file) => file.fileName)).toEqual([
      'settings-dialog.generated.h',
      'settings-dialog.generated.cpp',
    ]);
    expect(generated.files[1]!.content).toContain('MinimumVersion = SALAMATRIX_UI_VERSION_1_4');
    expect(generated.files[1]!.content).toContain('Salamatrix::UI::ControlKindButton');
    expect(generated.files[1]!.content).toContain('SetPathSeparator("/"[0])');
    expect(generated.files[1]!.content).toContain('SetActionPostCommand(static_cast<WORD>(32513))');
    expect(generated.files[1]!.content).toContain('SetIndeterminateTiming(-1, 100)');
    expect(generated.files[1]!.content).toContain('SetToolbarHeader("list", 49)');
  });
});
