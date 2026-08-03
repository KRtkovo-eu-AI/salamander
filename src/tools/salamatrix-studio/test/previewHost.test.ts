import { describe, expect, it } from 'vitest';
import { encodePreview } from '../src/previewTransport.js';
import { createDialogDocument, parseDialogDocument } from '../src/model.js';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';

describe('native preview transport', () => {
  it('encodes UTF-8 text and geometry without delimiter ambiguity', () => {
    const dialog = createDialogDocument('settings', 'Nastavení\tŽluťoučký');
    const encoded = encodePreview(dialog);
    expect(encoded).toMatch(/^SMXPREVIEW1\n/);
    expect(encoded).toContain(Buffer.from(dialog.title, 'utf8').toString('hex'));
    expect(encoded).toContain('\n420\t240\n');
    expect(encoded).toContain(`O\t${Buffer.from('dialogResult').toString('hex')}\tnumber\t31`);
  });

  it('keeps the complete UI capabilities preview payload reproducible', () => {
    const design = parseDialogDocument(readFileSync(resolve(
      'examples/salamatrix-ui-capabilities/.salamatrix/dialogs/ui-capabilities.salamatrix-dialog.json',
    ), 'utf8'));
    const fixture = readFileSync(resolve('test/fixtures/ui-capabilities.smxp'), 'utf8').replaceAll('\r\n', '\n');
    expect(encodePreview(design)).toBe(fixture);
    expect(fixture).toContain('696e64657465726d696e6174654475726174696f6e\tnumber\t2d31');
    expect(fixture).toContain('746f6f6c546970\tstring\t546f6f6c546970');
  });
});
