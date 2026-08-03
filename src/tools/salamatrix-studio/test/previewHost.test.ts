import { describe, expect, it } from 'vitest';
import { encodePreview } from '../src/previewTransport.js';
import { createDialogDocument } from '../src/model.js';

describe('native preview transport', () => {
  it('encodes UTF-8 text and geometry without delimiter ambiguity', () => {
    const dialog = createDialogDocument('settings', 'Nastavení\tŽluťoučký');
    const encoded = encodePreview(dialog);
    expect(encoded).toMatch(/^SMXPREVIEW1\n/);
    expect(encoded).toContain(Buffer.from(dialog.title, 'utf8').toString('hex'));
    expect(encoded).toContain('\n420\t240\n');
    expect(encoded).toContain(`O\t${Buffer.from('dialogResult').toString('hex')}\tnumber\t31`);
  });
});
