import { describe, expect, it } from 'vitest';
import { parseManifest } from '../src/manifestModel.js';

describe('extension manifest designer model', () => {
  it('retains fields outside the visual editor surface', () => {
    const manifest = parseManifest(JSON.stringify({
      schema: 2,
      id: 'Example.Extension',
      name: 'Example',
      version: '1.0.0',
      runtime: { id: 'PowerShell', customRuntimeField: true },
      entryPoint: 'main.ps1',
      iconDark: 'icon-dark.svg',
      commands: [{ id: 'Example.run', title: 'Run', handler: 'run', customCommandField: 42 }],
      viewers: [{ name: 'Example Viewer', patterns: ['*.example'], handler: 'viewExample' }],
    }));
    expect(manifest.iconDark).toBe('icon-dark.svg');
    expect(manifest.runtime).toMatchObject({ customRuntimeField: true });
    expect(manifest.commands?.[0]?.customCommandField).toBe(42);
    expect(manifest.viewers?.[0]).toMatchObject({ name: 'Example Viewer', handler: 'viewExample' });
  });

  it('accepts both schema keys and rejects conflicting aliases', () => {
    expect(parseManifest(JSON.stringify({
      schema: 1, id: 'Canonical', name: 'Canonical', version: '1.0.0',
      runtime: 'PowerShell', entryPoint: 'main.ps1',
    })).schema).toBe(1);
    expect(parseManifest(JSON.stringify({
      schemaVersion: 1, id: 'Alias', name: 'Alias', version: '1.0.0',
      runtime: 'PowerShell', entryPoint: 'main.ps1',
    })).schemaVersion).toBe(1);
    expect(() => parseManifest(JSON.stringify({
      schema: 1, schemaVersion: 2, id: 'Conflict', name: 'Conflict', version: '1.0.0',
      runtime: 'PowerShell', entryPoint: 'main.ps1',
    }))).toThrow(/must not conflict/);
  });
});
