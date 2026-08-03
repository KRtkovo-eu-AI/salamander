import * as path from 'node:path';
import * as vscode from 'vscode';
import { DialogEditorProvider } from './dialogEditor.js';
import { ManifestEditorProvider } from './manifestEditor.js';
import { generateDialog, runtimeFromManifest } from './generator.js';
import {
  createExtensionScaffold, extensionRuntimeIds, findScaffoldConflicts, validateExtensionFolderName, validateExtensionId,
  type ExtensionScaffoldSpec,
} from './extensionScaffold.js';
import {
  ControlCatalog,
  createDialogDocument,
  parseDialogDocument,
  serializeDialogDocument,
  RuntimeId,
} from './model.js';
import { StudioProjectExplorer } from './projectExplorer.js';
import { t, tf } from './localize.js';
import { PreviewHost } from './previewHost.js';
import { chooseProject, findProjectRoot, readJson } from './workspace.js';

export async function activate(context: vscode.ExtensionContext): Promise<void> {
  const pendingOverviewKey = 'salamatrixStudio.pendingOverview';
  const catalog = await loadCatalog(context);
  const editor = new DialogEditorProvider(context, catalog);
  const explorer = new StudioProjectExplorer();
  const manifestEditor = new ManifestEditorProvider(context);
  const previewHost = new PreviewHost(context);
  const projectWatcher = vscode.workspace.createFileSystemWatcher('**/extension.json');
  const dialogWatcher = vscode.workspace.createFileSystemWatcher('**/*.salamatrix-dialog.json');
  const refreshExplorer = async (): Promise<void> => {
    const count = await explorer.refresh();
    await vscode.commands.executeCommand('setContext', 'salamatrixStudio.hasProjects', count > 0);
  };
  projectWatcher.onDidCreate(refreshExplorer);
  projectWatcher.onDidChange(refreshExplorer);
  projectWatcher.onDidDelete(refreshExplorer);
  dialogWatcher.onDidCreate(refreshExplorer);
  dialogWatcher.onDidChange(refreshExplorer);
  dialogWatcher.onDidDelete(refreshExplorer);
  const workspaceFoldersChanged = vscode.workspace.onDidChangeWorkspaceFolders(refreshExplorer);

  context.subscriptions.push(
    projectWatcher,
    dialogWatcher,
    workspaceFoldersChanged,
    previewHost,
    vscode.window.registerCustomEditorProvider(DialogEditorProvider.viewType, editor, {
      webviewOptions: { retainContextWhenHidden: false },
      supportsMultipleEditorsPerDocument: false,
    }),
    vscode.window.registerCustomEditorProvider(ManifestEditorProvider.viewType, manifestEditor, {
      webviewOptions: { retainContextWhenHidden: false },
      supportsMultipleEditorsPerDocument: false,
    }),
    vscode.window.registerTreeDataProvider('salamatrixStudio.projectExplorer', explorer),
    vscode.commands.registerCommand('salamatrixStudio.refreshExplorer', refreshExplorer),
    vscode.commands.registerCommand('salamatrixStudio.createExtension', async () => {
      const created = await createExtensionProject();
      if (!created) return;
      if (!vscode.workspace.getWorkspaceFolder(created)) {
        const folders = vscode.workspace.workspaceFolders ?? [];
        if (folders.length === 0) {
          await context.globalState.update(
            pendingOverviewKey, vscode.Uri.joinPath(created, 'extension.json').toString(),
          );
          await vscode.commands.executeCommand('vscode.openFolder', created);
          return;
        }
        vscode.workspace.updateWorkspaceFolders(folders.length, 0, { uri: created, name: path.basename(created.fsPath) });
      }
      await refreshExplorer();
      await manifestEditor.open(vscode.Uri.joinPath(created, 'extension.json'), 'overview');
    }),
    vscode.commands.registerCommand('salamatrixStudio.addExistingExtensionFolder', async () => {
      await addExistingExtensionFolder();
      await refreshExplorer();
    }),
    vscode.commands.registerCommand('salamatrixStudio.openManifestDesigner', async (uri?: vscode.Uri, section: 'overview' | 'menus' = 'overview') => {
      const target = uri ?? vscode.window.activeTextEditor?.document.uri;
      if (!target || path.basename(target.fsPath).toLowerCase() !== 'extension.json') {
        void vscode.window.showErrorMessage(t('Select or open an extension.json file first.'));
        return;
      }
      await manifestEditor.open(target, section);
    }),
    vscode.commands.registerCommand('salamatrixStudio.addDialog', async () => {
      await addDialog();
      await refreshExplorer();
    }),
    vscode.commands.registerCommand('salamatrixStudio.addStandaloneDialog', async () => {
      await addStandaloneDialog();
    }),
    vscode.commands.registerCommand('salamatrixStudio.generateDialog', async (uri?: vscode.Uri) => {
      await generateActiveDialog(uri ?? editor.activeDocument);
      await refreshExplorer();
    }),
    vscode.commands.registerCommand('salamatrixStudio.generateDialogForRuntime', async (uri?: vscode.Uri) => {
      const picked = await vscode.window.showQuickPick([
        'PowerShell', 'Python.CPython', 'JavaScript.Node', 'PHP.CLI', 'Lua', 'Automation.JScript', 'Native.Cpp',
      ], { title: 'Generate Salamatrix Dialog Code', placeHolder: 'Select the target runtime' });
      if (picked) await generateActiveDialog(uri ?? editor.activeDocument, picked as RuntimeId);
      await refreshExplorer();
    }),
    vscode.commands.registerCommand('salamatrixStudio.previewDialog', async (uri?: vscode.Uri) => {
      const target = uri ?? editor.activeDocument;
      if (!target) { void vscode.window.showErrorMessage('Open a Salamatrix dialog design first.'); return; }
      try {
        const bytes = await vscode.workspace.fs.readFile(target);
        await previewHost.show(parseDialogDocument(new TextDecoder().decode(bytes)));
      } catch (error) {
        void vscode.window.showErrorMessage(error instanceof Error ? error.message : String(error));
      }
    }),
    vscode.commands.registerCommand('salamatrixStudio.saveDialogTemplate', async () => {
      await saveDialogTemplate(editor.activeDocument);
    }),
    vscode.commands.registerCommand('salamatrixStudio.newDialogFromTemplate', async () => {
      await newDialogFromTemplate();
      await explorer.refresh();
    }),
  );

  await refreshExplorer();
  const pendingOverview = context.globalState.get<string>(pendingOverviewKey);
  if (pendingOverview) {
    const uri = vscode.Uri.parse(pendingOverview);
    if (vscode.workspace.getWorkspaceFolder(uri)) {
      await context.globalState.update(pendingOverviewKey, undefined);
      await manifestEditor.open(uri, 'overview');
    }
  }
}

export function deactivate(): void {}

async function addDialog(source?: ReturnType<typeof createDialogDocument>): Promise<void> {
  const project = await chooseProject();
  if (!project) return;
  const id = await vscode.window.showInputBox({
    title: 'Add Salamatrix Dialog',
    prompt: 'Dialog identifier',
    value: source?.id ?? 'settings',
    validateInput: (value) => /^[A-Za-z][A-Za-z0-9_-]*$/.test(value)
      ? undefined
      : 'Use letters, digits, underscores, or hyphens; start with a letter.',
  });
  if (!id) return;
  const title = await vscode.window.showInputBox({
    title: 'Add Salamatrix Dialog',
    prompt: 'Window title',
    value: source?.title ?? 'Settings',
  });
  if (title === undefined) return;

  const directory = vscode.Uri.joinPath(project.root, '.salamatrix', 'dialogs');
  const uri = vscode.Uri.joinPath(directory, `${id}.salamatrix-dialog.json`);
  try {
    await vscode.workspace.fs.stat(uri);
    void vscode.window.showErrorMessage(`Dialog '${id}' already exists.`);
    return;
  } catch {
    // Expected for a new dialog.
  }
  const dialog = source
    ? { ...source, id, title }
    : createDialogDocument(id, title);
  await vscode.workspace.fs.createDirectory(directory);
  await vscode.workspace.fs.writeFile(uri, new TextEncoder().encode(serializeDialogDocument(dialog)));
  await vscode.commands.executeCommand('vscode.openWith', uri, DialogEditorProvider.viewType);
}

async function generateActiveDialog(uri: vscode.Uri | undefined, runtimeOverride?: RuntimeId): Promise<void> {
  if (!uri) {
    void vscode.window.showErrorMessage('Open a Salamatrix dialog design first.');
    return;
  }
  const projectRoot = findProjectRoot(uri);
  if (!projectRoot && !runtimeOverride) {
    void vscode.window.showErrorMessage('The dialog is not inside a Salamatrix extension project.');
    return;
  }
  try {
    const dialogBytes = await vscode.workspace.fs.readFile(uri);
    const dialog = parseDialogDocument(new TextDecoder().decode(dialogBytes));
    const runtime = runtimeOverride ?? runtimeFromManifest(await readJson(vscode.Uri.joinPath(projectRoot!, 'extension.json')));
    const generated = generateDialog(dialog, runtime);
    const outputRoot = projectRoot ?? vscode.Uri.file(path.dirname(uri.fsPath));
    const directory = vscode.Uri.joinPath(outputRoot, 'generated');
    await vscode.workspace.fs.createDirectory(directory);
    const targets: vscode.Uri[] = [];
    for (const file of generated.files) {
      const target = vscode.Uri.joinPath(directory, file.fileName);
      await vscode.workspace.fs.writeFile(target, new TextEncoder().encode(file.content));
      targets.push(target);
    }
    const relative = targets.map((target) => path.relative(outputRoot.fsPath, target.fsPath)).join(', ');
    const choice = await vscode.window.showInformationMessage(
      `Generated ${relative} for ${runtime}.`,
      'Open Generated Code',
    );
    if (choice) await vscode.window.showTextDocument(targets[targets.length - 1]!);
  } catch (error) {
    void vscode.window.showErrorMessage(error instanceof Error ? error.message : String(error));
  }
}

async function addStandaloneDialog(): Promise<void> {
  const id = await vscode.window.showInputBox({
    title: 'New Standalone Salamatrix Dialog', prompt: 'Dialog identifier', value: 'dialog',
    validateInput: (value) => /^[A-Za-z][A-Za-z0-9_-]*$/.test(value) ? undefined : 'Enter a valid dialog identifier.',
  });
  if (!id) return;
  const title = await vscode.window.showInputBox({ title: 'New Standalone Salamatrix Dialog', prompt: 'Window title', value: 'Dialog' });
  if (title === undefined) return;
  const base = vscode.workspace.workspaceFolders?.[0]?.uri;
  const target = await vscode.window.showSaveDialog({
    title: 'Save Salamatrix Dialog Design',
    defaultUri: base ? vscode.Uri.joinPath(base, `${id}.salamatrix-dialog.json`) : undefined,
    filters: { 'Salamatrix dialog designs': ['salamatrix-dialog.json'] },
  });
  if (!target) return;
  await vscode.workspace.fs.writeFile(target, new TextEncoder().encode(serializeDialogDocument(createDialogDocument(id, title))));
  await vscode.commands.executeCommand('vscode.openWith', target, DialogEditorProvider.viewType);
}

async function saveDialogTemplate(uri: vscode.Uri | undefined): Promise<void> {
  if (!uri) {
    void vscode.window.showErrorMessage('Open a Salamatrix dialog design first.');
    return;
  }
  const projectRoot = findProjectRoot(uri);
  if (!projectRoot) return;
  const name = await vscode.window.showInputBox({
    title: 'Save Dialog as Template',
    prompt: 'Template name',
    value: path.basename(uri.fsPath, '.salamatrix-dialog.json'),
    validateInput: (value) => /^[A-Za-z][A-Za-z0-9_-]*$/.test(value) ? undefined : 'Enter a safe file name.',
  });
  if (!name) return;
  const directory = vscode.Uri.joinPath(projectRoot, '.salamatrix', 'templates');
  const target = vscode.Uri.joinPath(directory, `${name}.salamatrix-dialog.json`);
  await vscode.workspace.fs.createDirectory(directory);
  await vscode.workspace.fs.copy(uri, target, { overwrite: false });
  void vscode.window.showInformationMessage(`Saved dialog template '${name}'.`);
}

async function newDialogFromTemplate(): Promise<void> {
  const project = await chooseProject();
  if (!project) return;
  const pattern = new vscode.RelativePattern(
    vscode.Uri.joinPath(project.root, '.salamatrix', 'templates'),
    '*.salamatrix-dialog.json',
  );
  const templates = await vscode.workspace.findFiles(pattern);
  if (templates.length === 0) {
    void vscode.window.showInformationMessage('This project has no saved dialog templates.');
    return;
  }
  const picked = await vscode.window.showQuickPick(
    templates.map((uri) => ({
      label: path.basename(uri.fsPath, '.salamatrix-dialog.json'),
      uri,
    })),
    { placeHolder: 'Select a dialog template' },
  );
  if (!picked) return;
  const bytes = await vscode.workspace.fs.readFile(picked.uri);
  await addDialog(parseDialogDocument(new TextDecoder().decode(bytes)));
}

async function loadCatalog(context: vscode.ExtensionContext): Promise<ControlCatalog> {
  const uri = vscode.Uri.joinPath(context.extensionUri, 'dist', 'contracts', 'control-catalog.json');
  const bytes = await vscode.workspace.fs.readFile(uri);
  return JSON.parse(new TextDecoder().decode(bytes)) as ControlCatalog;
}

async function createExtensionProject(): Promise<vscode.Uri | undefined> {
  const name = await vscode.window.showInputBox({
    title: t('Create Salamatrix Extension (1/5)'), prompt: t('Extension display name'), value: 'My Extension',
    validateInput: (value) => value.trim() ? undefined : t('Extension name is required.'),
  });
  if (!name) return undefined;
  const suggestedId = `MyCompany.${name.replace(/[^A-Za-z0-9_-]+/g, '') || 'Extension'}`;
  const id = await vscode.window.showInputBox({
    title: t('Create Salamatrix Extension (2/5)'), prompt: t('Unique dotted extension identifier'), value: suggestedId,
    validateInput: (value) => validateExtensionId(value) ? t('Use a dotted identifier such as MyCompany.MyExtension.') : undefined,
  });
  if (!id) return undefined;
  const runtime = await vscode.window.showQuickPick(extensionRuntimeIds, {
    title: t('Create Salamatrix Extension (3/5)'), placeHolder: t('Select the extension runtime'),
  });
  if (!runtime) return undefined;
  const description = await vscode.window.showInputBox({
    title: t('Create Salamatrix Extension (4/5)'), prompt: t('Description (optional)'),
    value: `${name} extension for Open Salamander.`,
  });
  if (description === undefined) return undefined;
  const root = await chooseExtensionDestination(name);
  if (!root) return undefined;

  const files = createExtensionScaffold({ id, name, description, runtime } as ExtensionScaffoldSpec);
  const conflicts = await findScaffoldConflicts(files, async (relativePath) => {
    const target = vscode.Uri.joinPath(root, ...relativePath.split('/'));
    try { await vscode.workspace.fs.stat(target); return true; } catch { return false; }
  });
  if (conflicts.length > 0) {
    void vscode.window.showErrorMessage(tf('Extension was not created because these files already exist: {0}', conflicts.join(', ')));
    return undefined;
  }
  await vscode.workspace.fs.createDirectory(root);
  for (const file of files) {
    const parts = file.path.split('/');
    const target = vscode.Uri.joinPath(root, ...parts);
    if (parts.length > 1) await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(root, ...parts.slice(0, -1)));
    await vscode.workspace.fs.writeFile(target, new TextEncoder().encode(file.content));
  }
  void vscode.window.showInformationMessage(tf('Created Salamatrix extension {0} for {1}.', `'${name}'`, runtime));
  return root;
}

async function chooseExtensionDestination(name: string): Promise<vscode.Uri | undefined> {
  let base: vscode.Uri | undefined;
  const folders = vscode.workspace.workspaceFolders ?? [];
  if (folders.length === 1) base = folders[0]!.uri;
  else if (folders.length > 1) {
    base = (await vscode.window.showQuickPick(
      folders.map((folder) => ({ label: folder.name, description: folder.uri.fsPath, uri: folder.uri })),
      { title: t('Create Salamatrix Extension (5/5)'), placeHolder: t('Select the workspace folder') },
    ))?.uri;
  } else {
    base = (await vscode.window.showOpenDialog({ title: t('Select the parent folder'), canSelectFolders: true, canSelectFiles: false, canSelectMany: false }))?.[0];
  }
  if (!base) return undefined;
  const placement = await vscode.window.showQuickPick([
    { label: t('Use workspace folder'), description: base.fsPath, value: 'root' },
    { label: t('Create a new subfolder'), description: t('Recommended when the workspace contains other projects'), value: 'subfolder' },
  ], { title: t('Create Salamatrix Extension (5/5)'), placeHolder: t('Choose the project location') });
  if (!placement) return undefined;
  if (placement.value === 'root') return base;
  const folderName = await vscode.window.showInputBox({
    title: t('Extension folder name'), value: name.trim().replace(/[^A-Za-z0-9._-]+/g, '-') || 'extension',
    validateInput: (value) => validateExtensionFolderName(value) ? t('Enter one safe folder name.') : undefined,
  });
  return folderName ? vscode.Uri.joinPath(base, folderName) : undefined;
}

async function addExistingExtensionFolder(): Promise<void> {
  const selected = (await vscode.window.showOpenDialog({
    title: t('Add Existing Salamatrix Extension Folder'), canSelectFolders: true, canSelectFiles: false, canSelectMany: false,
  }))?.[0];
  if (!selected) return;
  try { await vscode.workspace.fs.stat(vscode.Uri.joinPath(selected, 'extension.json')); } catch {
    void vscode.window.showErrorMessage(t('The selected folder does not contain extension.json.'));
    return;
  }
  const folders = vscode.workspace.workspaceFolders ?? [];
  if (folders.some((folder) => containsUri(folder.uri, selected))) return;
  if (folders.length === 0) {
    await vscode.commands.executeCommand('vscode.openFolder', selected);
    return;
  }
  vscode.workspace.updateWorkspaceFolders(folders.length, 0, { uri: selected, name: path.basename(selected.fsPath) });
}

function containsUri(parent: vscode.Uri, child: vscode.Uri): boolean {
  const relative = path.relative(parent.fsPath, child.fsPath);
  return relative === '' || (!relative.startsWith('..') && !path.isAbsolute(relative));
}
