import type { DialogDocument } from './model.js';

export const studioHostProtocolVersion = 1 as const;

export type StudioHostMethod =
  | 'hello'
  | 'validateManifest'
  | 'validateDialog'
  | 'showPreview'
  | 'updatePreview'
  | 'closePreview';

export interface StudioHostRequest {
  protocol: typeof studioHostProtocolVersion;
  kind: 'request';
  id: number;
  method: StudioHostMethod;
  params: Record<string, unknown>;
}

export interface StudioHostResponse<T = unknown> {
  protocol: typeof studioHostProtocolVersion;
  kind: 'response';
  id: number;
  ok: boolean;
  result?: T;
  error?: string;
}

export interface StudioHostHello {
  hostVersion: string;
  architecture: 'x64' | 'arm64';
  dialogSchemaVersions: number[];
  uiVersions: string[];
}

export interface PreviewParams extends Record<string, unknown> {
  dialog: DialogDocument;
  appearance: 'system' | 'light' | 'dark';
  dpi: number;
}

export function request(
  id: number,
  method: StudioHostMethod,
  params: Record<string, unknown> = {},
): StudioHostRequest {
  return { protocol: studioHostProtocolVersion, kind: 'request', id, method, params };
}
