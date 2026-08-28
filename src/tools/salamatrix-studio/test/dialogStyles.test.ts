import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';

describe('dialog designer preview styling', () => {
  it('uses the native dialog font instead of the VS Code UI font', () => {
    const styles = readFileSync(resolve(process.cwd(), 'src', 'webview', 'styles.css'), 'utf8');
    expect(styles).toContain('font-family: "MS Shell Dlg", "Microsoft Sans Serif", sans-serif;');
    expect(styles).toContain('font-size: 8pt;');
    expect(styles).toContain('.preview-dark .dialog-frame');
    expect(styles).toContain('.preview-light .dialog-frame');
    expect(styles).toContain('font-weight: 700;');
    expect(styles).toContain('text-decoration: underline;');
    expect(styles).toContain('white-space: nowrap;');
    expect(styles).toContain('text-overflow: ellipsis;');
    expect(styles).toContain('overflow: hidden;');
  });
});