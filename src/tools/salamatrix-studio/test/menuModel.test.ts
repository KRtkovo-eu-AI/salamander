import { describe, expect, it } from 'vitest';
import { parseManifest } from '../src/manifestModel.js';
import { parseMenuDocument, synchronizeMenuDocument } from '../src/menuModel.js';

const manifest = parseManifest(JSON.stringify({
  id: 'Example.Test', name: 'Test', version: '1.0.0', runtime: 'PowerShell', entryPoint: 'main.ps1',
  commands: [{ id: 'Example.Test.run', title: 'Run', handler: 'run' }],
}));

describe('menu builder model', () => {
  it('retains fields outside the visual menu surface', () => {
    const menu = parseMenuDocument(JSON.stringify({
      schema: 1, generatedBy: 'SalamatrixStudio', customRoot: 42,
      commands: [{ handler: 'run', action: 'program', target: 'tool.exe', customAction: true }],
    }), manifest);
    expect(menu.customRoot).toBe(42);
    expect(menu.commands[0]).toMatchObject({ customAction: true, target: 'tool.exe' });
  });

  it('adds missing custom handlers and removes stale actions', () => {
    const changed = { ...manifest, commands: [...manifest.commands!, { id: 'Example.Test.second', title: 'Second', handler: 'second' }] };
    const result = synchronizeMenuDocument({
      schema: 1, generatedBy: 'SalamatrixStudio',
      commands: [{ handler: 'run', action: 'open', target: 'https://example.test' }, { handler: 'stale', action: 'custom' }],
    }, changed);
    expect(result.commands).toHaveLength(2);
    expect(result.commands[0]).toMatchObject({ handler: 'run', action: 'open' });
    expect(result.commands[1]).toEqual({ handler: 'second', action: 'custom' });
  });
});
