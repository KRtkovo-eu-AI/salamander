export interface ExtensionCommand {
  id: string;
  title: string;
  handler: string;
  menu?: 'plugin' | 'context' | 'both' | 'none';
  contextMenu?: boolean;
  toolbar?: boolean;
  toolbarMenu?: boolean;
  requires?: string;
  icon?: string;
  iconDark?: string;
  requiresExecutable?: string;
  enabled?: boolean;
  visible?: boolean;
  [key: string]: unknown;
}

export interface ExtensionViewer {
  patterns: string[];
  handler: string;
}

export interface ExtensionFileSystemAction {
  id: string;
  title: string;
  handler: string;
  default?: boolean;
}

export interface ExtensionFileSystem {
  id: string;
  name: string;
  listHandler: string;
  openHandler?: string;
  icon?: string;
  iconDark?: string;
  refreshIntervalMs?: number;
  actions?: ExtensionFileSystemAction[];
}

export interface ExtensionManifest {
  schemaVersion?: 1 | 2;
  id: string;
  name: string;
  version: string;
  description?: string;
  runtime: string | { id?: string; [key: string]: unknown };
  entryPoint: string;
  capabilities?: string[];
  commands?: ExtensionCommand[];
  viewers?: ExtensionViewer[];
  fileSystems?: ExtensionFileSystem[];
  [key: string]: unknown;
}

export function parseManifest(text: string): ExtensionManifest {
  return validateManifest(JSON.parse(text));
}

export function validateManifest(value: unknown): ExtensionManifest {
  if (!value || typeof value !== 'object') throw new Error('extension.json must contain a JSON object.');
  const manifest = value as Partial<ExtensionManifest>;
  if (manifest.schemaVersion !== undefined && ![1, 2].includes(manifest.schemaVersion)) throw new Error('Unsupported manifest schemaVersion.');
  for (const field of ['id', 'name', 'version', 'entryPoint'] as const) {
    if (typeof manifest[field] !== 'string' || manifest[field].length === 0) throw new Error(`Manifest field '${field}' is required.`);
  }
  const runtime = typeof manifest.runtime === 'string' ? manifest.runtime : manifest.runtime?.id;
  if (!runtime) throw new Error("Manifest field 'runtime' is required.");
  if (manifest.commands !== undefined && !Array.isArray(manifest.commands)) throw new Error("Manifest field 'commands' must be an array.");
  const ids = new Set<string>();
  for (const command of manifest.commands ?? []) {
    if (!command || typeof command !== 'object') throw new Error('Every manifest command must be an object.');
    if (command.id && ids.has(command.id)) throw new Error(`Duplicate command id: ${command.id}`);
    if (command.id) ids.add(command.id);
    if (command.menu && !['plugin', 'context', 'both', 'none'].includes(command.menu)) throw new Error(`Invalid command menu: ${command.menu}`);
    if (command.requires && !['any', 'disk', 'focused', 'file', 'selection'].includes(command.requires)) throw new Error(`Invalid command requirement: ${command.requires}`);
    if (command.toolbarMenu && !command.toolbar) throw new Error('toolbarMenu requires toolbar to be enabled.');
  }
  if (manifest.viewers !== undefined && (!Array.isArray(manifest.viewers) || manifest.schemaVersion !== 2)) throw new Error('Manifest viewers require schemaVersion 2.');
  if (manifest.fileSystems !== undefined && (!Array.isArray(manifest.fileSystems) || manifest.schemaVersion !== 2)) throw new Error('Manifest fileSystems require schemaVersion 2.');
  return manifest as ExtensionManifest;
}
