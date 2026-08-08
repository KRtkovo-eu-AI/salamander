import { describe, expect, it } from 'vitest';
import {
  createExtensionScaffold, extensionRuntimeIds, findScaffoldConflicts, generateMenuActions,
  integrateMenuDispatch, menuDispatchMarker, validateExtensionFolderName,
} from '../src/extensionScaffold.js';
import type { MenuDocument } from '../src/menuModel.js';

describe('extension project scaffolding', () => {
  for (const runtime of extensionRuntimeIds) {
    it(`creates a functional ${runtime} project`, () => {
      const files = createExtensionScaffold({ id: 'Example.Test', name: 'Test', description: 'Demo', runtime });
      const manifest = JSON.parse(files.find((file) => file.path === 'extension.json')!.content);
      const entry = files.find((file) => file.path === manifest.entryPoint)!.content;
      expect(manifest).toMatchObject({ schemaVersion: 2, id: 'Example.Test', runtime, version: '1.0.0' });
      expect(manifest.commands[0]).toMatchObject({ handler: 'run', title: 'Run Test' });
      expect(manifest.capabilities).toContain('ui.dialogs');
      expect(entry).toContain(menuDispatchMarker);
      expect(entry.toLowerCase()).toContain('hello from');
      if (runtime === 'Automation.JScript') {
        expect(entry).toContain('Salamander.Script.Path');
        expect(entry).toContain('Salamander.MsgBox');
        expect(entry).not.toContain('WScript.ScriptFullName');
        expect(entry).toContain('if (!handledByStudio)');
      }
      expect(files.some((file) => file.path === '.salamatrix/menu.json')).toBe(true);
      expect(files.some((file) => file.path.startsWith('generated/'))).toBe(true);
    });
  }

  it('detects every conflict before the writer creates files', async () => {
    const files = createExtensionScaffold({ id: 'Example.Test', name: 'Test', description: '', runtime: 'PowerShell' });
    const conflicts = await findScaffoldConflicts(files, async (relative) => ['extension.json', 'icon.svg'].includes(relative));
    expect(conflicts).toEqual(['extension.json', 'icon.svg']);
  });

  it('validates a single safe destination folder name', () => {
    expect(validateExtensionFolderName('my-extension')).toBeUndefined();
    expect(validateExtensionFolderName('../outside')).toBeDefined();
    expect(validateExtensionFolderName('nested/folder')).toBeDefined();
    expect(validateExtensionFolderName('')).toBeDefined();
  });

  it('generates all simple action kinds without duplicating entry integration', () => {
    const menu: MenuDocument = {
      schema: 1, generatedBy: 'SalamatrixStudio',
      commands: ['program', 'open', 'command', 'powershell', 'custom'].map((action, index) => ({
        handler: `action${index}`, action: action as 'program', target: 'tool.exe', arguments: '--test',
      })),
    };
    for (const runtime of extensionRuntimeIds) {
      const generated = generateMenuActions(runtime, menu);
      for (const action of ['program', 'open', 'command', 'powershell', 'custom']) expect(generated).toContain(action);
      const integrated = integrateMenuDispatch(runtime, runtime === 'PHP.CLI' ? '<?php\nuser_code();\n' : 'user code\n');
      expect(integrateMenuDispatch(runtime, integrated)).toBe(integrated);
      if (runtime === 'Lua') {
        expect(generated).toContain('local actions = {');
        expect(generated).not.toContain('actions_json:match');
      }
      if (runtime === 'Automation.JScript') expect(generated).toContain('salamatrixStudioActions.length===1');
    }
  });
});
