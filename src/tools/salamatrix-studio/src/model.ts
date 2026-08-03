export const controlKinds = [
  'label', 'statictext', 'textbox', 'checkbox', 'radio', 'combobox',
  'button', 'listview', 'treeview', 'tabcontrol', 'folderpicker',
  'filepicker', 'groupbox', 'hyperlink', 'progressbar', 'arrowbutton',
  'textarrowbutton', 'colorarrowbutton', 'toolbarheader',
] as const;

export type ControlKind = typeof controlKinds[number];
export type RuntimeId =
  | 'PowerShell'
  | 'Python.CPython'
  | 'JavaScript.Node'
  | 'PHP.CLI'
  | 'Lua'
  | 'Automation.JScript'
  | 'Native.Cpp';

export interface Bounds {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface DialogColumn {
  title: string;
  width: number;
}

export type OptionValue = string | number | boolean;

export interface DialogControl {
  kind: ControlKind;
  id: string;
  text: string;
  bounds: Bounds;
  options?: Record<string, OptionValue>;
  items?: string[];
  columns?: DialogColumn[];
  selectedIndex?: number;
  validation?: { required?: boolean; message?: string };
  events?: Record<string, string>;
}

export interface DialogDocument {
  schema: 1;
  targetUiVersion: string;
  id: string;
  title: string;
  width: number;
  height: number;
  controls: DialogControl[];
}

export interface ControlCatalogEntry {
  kind: ControlKind;
  title: string;
  defaultWidth: number;
  defaultHeight: number;
}

export interface ControlCatalog {
  schema: 1;
  uiVersion: string;
  controls: ControlCatalogEntry[];
}

const identifierPattern = /^[A-Za-z][A-Za-z0-9_-]*$/;

export function createDialogDocument(id: string, title: string): DialogDocument {
  return {
    schema: 1,
    targetUiVersion: '1.4',
    id,
    title,
    width: 420,
    height: 240,
    controls: [
      {
        kind: 'button',
        id: 'ok',
        text: 'OK',
        bounds: { x: 304, y: 210, width: 50, height: 14 },
        options: { dialogResult: 1 },
      },
      {
        kind: 'button',
        id: 'cancel',
        text: 'Cancel',
        bounds: { x: 360, y: 210, width: 50, height: 14 },
        options: { dialogResult: 2 },
      },
    ],
  };
}

export function parseDialogDocument(text: string): DialogDocument {
  const value: unknown = JSON.parse(text);
  if (!value || typeof value !== 'object') throw new Error('Dialog document must be a JSON object.');
  const dialog = value as Partial<DialogDocument>;
  if (dialog.schema !== 1) throw new Error('Unsupported dialog schema.');
  if (!dialog.id || !identifierPattern.test(dialog.id)) throw new Error('Dialog id is invalid.');
  if (typeof dialog.title !== 'string') throw new Error('Dialog title is required.');
  if (!Number.isInteger(dialog.width) || !Number.isInteger(dialog.height)) {
    throw new Error('Dialog width and height must be integers.');
  }
  if (!Array.isArray(dialog.controls)) throw new Error('Dialog controls must be an array.');

  const ids = new Set<string>();
  for (const control of dialog.controls) {
    if (!controlKinds.includes(control.kind)) throw new Error(`Unsupported control kind: ${control.kind}`);
    if (!identifierPattern.test(control.id)) throw new Error(`Invalid control id: ${control.id}`);
    if (ids.has(control.id)) throw new Error(`Duplicate control id: ${control.id}`);
    ids.add(control.id);
    for (const name of ['x', 'y', 'width', 'height'] as const) {
      if (!Number.isInteger(control.bounds?.[name])) {
        throw new Error(`Control ${control.id} has invalid ${name}.`);
      }
    }
  }

  return {
    schema: 1,
    targetUiVersion: dialog.targetUiVersion ?? '1.4',
    id: dialog.id,
    title: dialog.title,
    width: dialog.width!,
    height: dialog.height!,
    controls: dialog.controls,
  };
}

export function serializeDialogDocument(dialog: DialogDocument): string {
  return `${JSON.stringify(dialog, null, 2)}\n`;
}

export function generatedFunctionName(id: string): string {
  return `Show${id.charAt(0).toUpperCase()}${id.slice(1).replace(/[-_](.)/g, (_, c: string) => c.toUpperCase())}Dialog`;
}
