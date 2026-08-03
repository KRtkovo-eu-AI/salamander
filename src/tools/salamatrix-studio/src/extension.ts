import * as path from 'node:path';
import * as vscode from 'vscode';
import { DialogEditorProvider } from './dialogEditor.js';
import { ManifestEditorProvider } from './manifestEditor.js';
import { generateDialog, runtimeFromManifest } from './generator.js';
import {
  ControlCatalog,
  createDialogDocument,
  parseDialogDocument,
  serializeDialogDocument,
  RuntimeId,
} from './model.js';
import { StudioProjectExplorer } from './projectExplorer.js';
import { PreviewHost } from './previewHost.js';
import { chooseProject, findProjectRoot, readJson } from './workspace.js';

export async function activate(context: vscode.ExtensionContext): Promise<void> {
  const catalog = await loadCatalog(context);
  const editor = new DialogEditorProvider(context, catalog);
  const explorer = new StudioProjectExplorer();
  const manifestEditor = new ManifestEditorProvider(context);
  const previewHost = new PreviewHost(context);
  const projectWatcher = vscode.workspace.createFileSystemWatcher('**/extension.json');
  const dialogWatcher = vscode.workspace.createFileSystemWatcher('**/*.salamatrix-dialog.json');
  const refreshExplorer = (): void => { void explorer.refresh(); };
  projectWatcher.onDidCreate(refreshExplorer);
  projectWatcher.onDidChange(refreshExplorer);
  projectWatcher.onDidDelete(refreshExplorer);
  dialogWatcher.onDidCreate(refreshExplorer);
  dialogWatcher.onDidChange(refreshExplorer);
  dialogWatcher.onDidDelete(refreshExplorer);

  context.subscriptions.push(
    projectWatcher,
    dialogWatcher,
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
    vscode.commands.registerCommand('salamatrixStudio.refreshExplorer', () => explorer.refresh()),
    vscode.commands.registerCommand('salamatrixStudio.openManifestDesigner', async (uri?: vscode.Uri) => {
      const target = uri ?? vscode.window.activeTextEditor?.document.uri;
      if (!target || path.basename(target.fsPath).toLowerCase() !== 'extension.json') {
        void vscode.window.showErrorMessage('Select or open an extension.json file first.');
        return;
      }
      await vscode.commands.executeCommand('vscode.openWith', target, ManifestEditorProvider.viewType);
    }),
    vscode.commands.registerCommand('salamatrixStudio.addDialog', async () => {
      await addDialog();
      await explorer.refresh();
    }),
    vscode.commands.registerCommand('salamatrixStudio.addStandaloneDialog', async () => {
      await addStandaloneDialog();
    }),
    vscode.commands.registerCommand('salamatrixStudio.generateDialog', async (uri?: vscode.Uri) => {
      await generateActiveDialog(uri ?? editor.activeDocument);
      await explorer.refresh();
    }),
    vscode.commands.registerCommand('salamatrixStudio.generateDialogForRuntime', async (uri?: vscode.Uri) => {
      const picked = await vscode.window.showQuickPick([
        'PowerShell', 'Python.CPython', 'JavaScript.Node', 'PHP.CLI', 'Lua', 'Automation.JScript', 'Native.Cpp',
      ], { title: 'Generate Salamatrix Dialog Code', placeHolder: 'Select the target runtime' });
      if (picked) await generateActiveDialog(uri ?? editor.activeDocument, picked as RuntimeId);
      await explorer.refresh();
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

  await explorer.refresh();
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
