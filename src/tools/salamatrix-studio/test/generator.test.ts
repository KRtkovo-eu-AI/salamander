import { describe, expect, it } from 'vitest';
import { generateDialog } from '../src/generator.js';
import { createDialogDocument } from '../src/model.js';

const dialog = createDialogDocument('settings', 'Settings');

describe('dialog generators', () => {
  it('generates a PowerShell module using the public facade', () => {
    const generated = generateDialog(dialog, 'PowerShell');
    expect(generated.fileName).toBe('settings-dialog.generated.ps1');
    expect(generated.content).toContain("$Salamander.ui.Dialog('Settings', 420, 240)");
    expect(generated.content).toContain("$dialog.AddControl('button', 'ok'");
    expect(generated.content).toContain('$dialog.Close()');
  });

  it('generates an asynchronous Node module', () => {
    const generated = generateDialog(dialog, 'JavaScript.Node');
    expect(generated.fileName).toBe('settings-dialog.generated.mjs');
    expect(generated.content).toContain('await Salamander.ui.dialog("Settings"');
    expect(generated.content).toContain('await dialog.addControl("button", "ok"');
  });

  it('generates a Python module', () => {
    const generated = generateDialog(dialog, 'Python.CPython');
    expect(generated.fileName).toBe('settings_dialog_generated.py');
    expect(generated.content).toContain('dialog = Salamander.ui.dialog("Settings", 420, 240)');
    expect(generated.content).toContain('dialog.add_control("button", "ok"');
  });
});
