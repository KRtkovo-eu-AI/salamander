import { describe, expect, it } from 'vitest';
import {
  createDialogDocument,
  parseDialogDocument,
  serializeDialogDocument,
} from '../src/model.js';

describe('dialog model', () => {
  it('round-trips a new dialog', () => {
    const original = createDialogDocument('settings', 'Settings');
    expect(parseDialogDocument(serializeDialogDocument(original))).toEqual(original);
  });

  it('rejects duplicate control identifiers', () => {
    const dialog = createDialogDocument('settings', 'Settings');
    dialog.controls[1]!.id = dialog.controls[0]!.id;
    expect(() => parseDialogDocument(serializeDialogDocument(dialog))).toThrow(/Duplicate control id/);
  });

  it('rejects unknown control kinds', () => {
    const dialog = createDialogDocument('settings', 'Settings');
    const value = JSON.parse(serializeDialogDocument(dialog));
    value.controls[0].kind = 'unknown';
    expect(() => parseDialogDocument(JSON.stringify(value))).toThrow(/Unsupported control kind/);
  });
});
