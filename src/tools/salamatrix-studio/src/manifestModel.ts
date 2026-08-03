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

export interface ExtensionManifest {
  schema?: number;
  id: string;
  name: string;
  version: string;
  description?: string;
  runtime: string | { id?: string; [key: string]: unknown };
  entryPoint: string;
  capabilities?: string[];
  commands?: ExtensionCommand[];
  [key: string]: unknown;
}

export function parseManifest(text: string): ExtensionManifest {
  return validateManifest(JSON.parse(text));
}

export function validateManifest(value: unknown): ExtensionManifest {
  if (!value || typeof value !== 'object') throw new Error('extension.json must contain a JSON object.');
  const manifest = value as Partial<ExtensionManifest>;
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
  return manifest as ExtensionManifest;
}
