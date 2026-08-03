import type { ExtensionManifest } from './manifestModel.js';

export type MenuActionKind = 'custom' | 'program' | 'open' | 'command' | 'powershell';

export interface MenuAction {
  handler: string;
  action: MenuActionKind;
  target?: string;
  arguments?: string;
  workingDirectory?: string;
  [key: string]: unknown;
}

export interface MenuDocument {
  schema: 1;
  generatedBy: 'SalamatrixStudio';
  commands: MenuAction[];
  [key: string]: unknown;
}

export function createMenuDocument(manifest: ExtensionManifest): MenuDocument {
  return {
    schema: 1,
    generatedBy: 'SalamatrixStudio',
    commands: (manifest.commands ?? []).map((command) => ({
      handler: command.handler || handlerFromId(command.id),
      action: 'custom',
    })),
  };
}

export function parseMenuDocument(text: string, manifest: ExtensionManifest): MenuDocument {
  const value = JSON.parse(text) as Partial<MenuDocument>;
  if (!value || typeof value !== 'object') throw new Error('menu.json must contain a JSON object.');
  if (value.commands !== undefined && !Array.isArray(value.commands)) throw new Error("Menu field 'commands' must be an array.");
  const document: MenuDocument = {
    ...value,
    schema: 1,
    generatedBy: 'SalamatrixStudio',
    commands: (value.commands ?? []).map(validateAction),
  };
  return synchronizeMenuDocument(document, manifest);
}

export function synchronizeMenuDocument(document: MenuDocument, manifest: ExtensionManifest): MenuDocument {
  const existing = new Map(document.commands.map((command) => [command.handler, command]));
  return {
    ...document,
    schema: 1,
    generatedBy: 'SalamatrixStudio',
    commands: (manifest.commands ?? []).map((command) => {
      const handler = command.handler || handlerFromId(command.id);
      return existing.get(handler) ?? { handler, action: 'custom' };
    }),
  };
}

export function serializeMenuDocument(document: MenuDocument): string {
  return `${JSON.stringify(document, null, 2)}\n`;
}

function validateAction(value: unknown): MenuAction {
  if (!value || typeof value !== 'object') throw new Error('Every menu action must be an object.');
  const action = value as Partial<MenuAction>;
  if (typeof action.handler !== 'string' || !action.handler) throw new Error("Menu action field 'handler' is required.");
  if (!['custom', 'program', 'open', 'command', 'powershell'].includes(action.action ?? '')) {
    throw new Error(`Unsupported menu action: ${String(action.action)}`);
  }
  return action as MenuAction;
}

function handlerFromId(id: string): string {
  return id.split('.').at(-1) || 'run';
}
