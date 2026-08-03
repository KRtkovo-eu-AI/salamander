import { describe, expect, it } from 'vitest';
import { dialogClientPixels, dialogFramePixels } from '../src/dialogGeometry.js';

describe('dialog-unit geometry', () => {
  it('matches the 8 pt MS Shell Dlg native template at 96 DPI', () => {
    expect(dialogClientPixels(420, 240)).toEqual({ width: 630, height: 390 });
    expect(dialogFramePixels(420, 240)).toEqual({ width: 630, height: 421 });
  });

  it('keeps the UI capabilities demo geometry exact', () => {
    expect(dialogClientPixels(463, 236)).toEqual({ width: 695, height: 384 });
  });
});
