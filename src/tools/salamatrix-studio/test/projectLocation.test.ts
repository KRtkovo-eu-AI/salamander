import * as path from 'node:path';
import { describe, expect, it } from 'vitest';
import { formatProjectLocation } from '../src/projectLocation.js';

describe('extension project location', () => {
  it('shows the path inside a single workspace', () => {
    const root = path.join('workspace', 'plugins', 'demo');
    expect(formatProjectLocation(root, 'workspace', 'workspace', 1)).toBe('plugins/demo');
  });

  it('uses the workspace folder name when the extension is the workspace root', () => {
    expect(formatProjectLocation('workspace', 'workspace', 'salamatrix-ui-capabilities', 1))
      .toBe('salamatrix-ui-capabilities');
  });

  it('prefixes the workspace name when several folders are open', () => {
    const root = path.join('left', 'demo');
    expect(formatProjectLocation(root, 'left', 'left', 2)).toBe('left/demo');
  });

  it('keeps the full path when the extension is outside the workspace', () => {
    const root = path.join('other', 'demo');
    expect(formatProjectLocation(root, 'workspace', 'workspace', 1)).toBe(root);
    expect(formatProjectLocation(root, undefined, undefined, 0)).toBe(root);
  });
});
