import { describe, expect, it } from 'vitest';
import { request } from '../src/hostProtocol.js';

describe('native host protocol', () => {
  it('creates a versioned JSON-line request', () => {
    expect(request(7, 'hello')).toEqual({
      protocol: 1,
      kind: 'request',
      id: 7,
      method: 'hello',
      params: {},
    });
  });
});
