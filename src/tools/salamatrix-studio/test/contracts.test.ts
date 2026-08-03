import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { controlKinds } from '../src/model.js';

const contracts = resolve(process.cwd(), '..', '..', 'salamatrix-sdk', 'contracts');

describe('shared SDK contracts', () => {
  it('keeps the control catalog aligned with the TypeScript model', () => {
    const catalog = JSON.parse(readFileSync(resolve(contracts, 'control-catalog.json'), 'utf8')) as {
      controls: Array<{ kind: string }>;
    };
    expect(catalog.controls.map((control) => control.kind)).toEqual([...controlKinds]);
  });

  it('ships parseable JSON schemas', () => {
    for (const name of ['salamatrix-dialog.schema.json', 'studio-host-protocol.schema.json']) {
      expect(() => JSON.parse(readFileSync(resolve(contracts, name), 'utf8'))).not.toThrow();
    }
  });
});
