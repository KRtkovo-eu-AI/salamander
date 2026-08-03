import { describe, expect, it } from 'vitest';
import { parseManifest } from '../src/manifestModel.js';

describe('extension manifest designer model', () => {
  it('retains fields outside the visual editor surface', () => {
    const manifest = parseManifest(JSON.stringify({
      schema: 1,
      id: 'Example.Extension',
      name: 'Example',
      version: '1.0.0',
      runtime: { id: 'PowerShell', customRuntimeField: true },
      entryPoint: 'main.ps1',
      iconDark: 'icon-dark.svg',
      commands: [{ id: 'Example.run', title: 'Run', handler: 'run', customCommandField: 42 }],
    }));
    expect(manifest.iconDark).toBe('icon-dark.svg');
    expect(manifest.runtime).toMatchObject({ customRuntimeField: true });
    expect(manifest.commands?.[0]?.customCommandField).toBe(42);
  });
});
