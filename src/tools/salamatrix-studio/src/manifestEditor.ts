import * as path from 'node:path';
import * as vscode from 'vscode';
import {
  extensionRuntimeIds, generateMenuActions, generatedMenuPath, integrateMenuDispatch, menuDispatchMarker,
  type ExtensionScaffoldSpec,
} from './extensionScaffold.js';
import { ExtensionManifest, parseManifest, validateManifest } from './manifestModel.js';
import { t, tf } from './localize.js';
import {
  createMenuDocument, parseMenuDocument, serializeMenuDocument, synchronizeMenuDocument, type MenuDocument,
} from './menuModel.js';

type DesignerSection = 'overview' | 'menus';

interface ManifestMessage {
  type: 'ready' | 'update' | 'pickIcon' | 'enableGeneratedActions';
  manifest?: ExtensionManifest;
  menu?: MenuDocument;
  commandIndex?: number;
  dark?: boolean;
}

const menuMigrationPreviewScheme = 'salamatrix-studio-menu-migration';
const menuMigrationPreviews = new Map<string, string>();

export class ManifestEditorProvider implements vscode.CustomTextEditorProvider {
  static readonly viewType = 'salamatrixStudio.manifestDesigner';
  private readonly pendingSections = new Map<string, DesignerSection>();
  private readonly panels = new Map<string, vscode.WebviewPanel>();

  constructor(private readonly context: vscode.ExtensionContext) {
    context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider(menuMigrationPreviewScheme, {
      provideTextDocumentContent: (uri) => menuMigrationPreviews.get(uri.toString()) ?? '',
    }));
  }

  async open(uri: vscode.Uri, section: DesignerSection): Promise<void> {
    const key = uri.toString();
    const panel = this.panels.get(key);
    if (panel) {
      panel.reveal();
      await panel.webview.postMessage({ type: 'revealSection', section });
      return;
    }
    this.pendingSections.set(key, section);
    await vscode.commands.executeCommand('vscode.openWith', uri, ManifestEditorProvider.viewType);
  }

  async resolveCustomTextEditor(document: vscode.TextDocument, panel: vscode.WebviewPanel): Promise<void> {
    const key = document.uri.toString();
    const projectRoot = vscode.Uri.file(path.dirname(document.uri.fsPath));
    const menuUri = vscode.Uri.joinPath(projectRoot, '.salamatrix', 'menu.json');
    const initialSection = this.pendingSections.get(key) ?? 'overview';
    this.pendingSections.delete(key);
    this.panels.set(key, panel);
    panel.webview.options = {
      enableScripts: true,
      localResourceRoots: [vscode.Uri.joinPath(this.context.extensionUri, 'dist'), projectRoot],
    };
    panel.webview.html = this.html(panel.webview);

    const load = async (): Promise<{ manifest: ExtensionManifest; menu: MenuDocument; generatedEnabled: boolean }> => {
      const manifest = parseManifest(document.getText());
      let menu = createMenuDocument(manifest);
      try {
        const bytes = await vscode.workspace.fs.readFile(menuUri);
        menu = parseMenuDocument(new TextDecoder().decode(bytes), manifest);
      } catch { /* Existing extensions start in non-invasive custom-handler mode. */ }
      let generatedEnabled = false;
      try {
        const entry = await vscode.workspace.fs.readFile(vscode.Uri.joinPath(projectRoot, manifest.entryPoint));
        generatedEnabled = new TextDecoder().decode(entry).includes(menuDispatchMarker);
      } catch { /* Invalid/missing entry point is reported when generation is enabled. */ }
      return { manifest, menu, generatedEnabled };
    };
    const sendDocument = async (): Promise<void> => {
      try {
        const model = await load();
        const iconSources = model.manifest.commands?.map((command) => ({
          light: iconSource(panel.webview, projectRoot, command.icon),
          dark: iconSource(panel.webview, projectRoot, command.iconDark ?? command.icon),
        })) ?? [];
        await panel.webview.postMessage({ type: 'document', ...model, initialSection, iconSources });
      } catch (error) {
        await panel.webview.postMessage({ type: 'error', message: error instanceof Error ? error.message : String(error) });
      }
    };
    const messages = panel.webview.onDidReceiveMessage(async (message: ManifestMessage) => {
      try {
        if (message.type === 'ready') await sendDocument();
        if (message.type === 'update' && message.manifest && message.menu) {
          const manifest = validateManifest(message.manifest);
          const menu = synchronizeMenuDocument(message.menu, manifest);
          await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(projectRoot, '.salamatrix'));
          await vscode.workspace.fs.writeFile(menuUri, new TextEncoder().encode(serializeMenuDocument(menu)));
          const edit = new vscode.WorkspaceEdit();
          edit.replace(document.uri, fullRange(document), `${JSON.stringify(manifest, null, 2)}\n`);
          await vscode.workspace.applyEdit(edit);
          if (await generatedActionsEnabled(projectRoot, manifest)) await writeGeneratedActions(projectRoot, manifest, menu);
        }
        if (message.type === 'pickIcon' && message.commandIndex !== undefined) {
          await pickCommandIcon(document, projectRoot, message.commandIndex, Boolean(message.dark));
        }
        if (message.type === 'enableGeneratedActions') {
          const { manifest, menu } = await load();
          await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(projectRoot, '.salamatrix'));
          await vscode.workspace.fs.writeFile(menuUri, new TextEncoder().encode(serializeMenuDocument(menu)));
          await enableGeneratedActions(projectRoot, manifest, menu);
          await sendDocument();
        }
      } catch (error) {
        await panel.webview.postMessage({ type: 'error', message: error instanceof Error ? error.message : String(error) });
      }
    });
    const changes = vscode.workspace.onDidChangeTextDocument((event) => {
      if (event.document.uri.toString() === key) void sendDocument();
    });
    panel.onDidDispose(() => { messages.dispose(); changes.dispose(); this.panels.delete(key); });
  }

  private html(webview: vscode.Webview): string {
    const script = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'manifest-webview.js'));
    const style = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'manifest-webview.css'));
    const nonce = randomNonce();
    const language = vscode.env.language.toLowerCase().startsWith('cs') ? 'cs' : 'en';
    return `<!DOCTYPE html><html lang="${language}"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src ${webview.cspSource}; style-src ${webview.cspSource} 'unsafe-inline'; script-src 'nonce-${nonce}';"><link rel="stylesheet" href="${style}"><title>Salamatrix Extension Designer</title></head><body><div id="root"></div><script nonce="${nonce}" src="${script}"></script></body></html>`;
  }
}

async function enableGeneratedActions(projectRoot: vscode.Uri, manifest: ExtensionManifest, menu: MenuDocument): Promise<void> {
  const runtime = runtimeId(manifest);
  const entryUri = vscode.Uri.joinPath(projectRoot, manifest.entryPoint);
  const generated = generatedMenuPath(runtime);
  const generatedUri = vscode.Uri.joinPath(projectRoot, ...generated.split('/'));
  try {
    const existingGenerated = new TextDecoder().decode(await vscode.workspace.fs.readFile(generatedUri));
    if (!existingGenerated.includes('Generated by Salamatrix Studio. Changes will be overwritten.')) {
      throw new Error(tf('Studio will not overwrite the existing unowned file {0}. Move or rename it before enabling generated actions.', generated));
    }
  } catch (error) {
    if (!(error instanceof vscode.FileSystemError) || error.code !== 'FileNotFound') throw error;
  }
  const current = new TextDecoder().decode(await vscode.workspace.fs.readFile(entryUri));
  const integrated = integrateMenuDispatch(runtime, current);
  const previewUri = vscode.Uri.from({
    scheme: menuMigrationPreviewScheme,
    path: `/${path.basename(manifest.entryPoint)}`,
    query: `${Date.now()}`,
  });
  menuMigrationPreviews.set(previewUri.toString(), integrated);
  await vscode.commands.executeCommand(
    'vscode.diff', entryUri, previewUri,
    tf('{0} - Salamatrix Studio menu dispatch preview', path.basename(manifest.entryPoint)),
  );
  const answer = await vscode.window.showWarningMessage(
    tf('Review the open diff. Enable generated menu actions? Studio will insert one marked dispatch block into {0} and will own {1}. Your existing code outside that block will not be changed.', manifest.entryPoint, generated),
    { modal: true }, t('Enable Generated Actions'),
  );
  if (!answer) return;
  await vscode.workspace.fs.writeFile(entryUri, new TextEncoder().encode(integrated));
  await writeGeneratedActions(projectRoot, manifest, menu);
}

async function writeGeneratedActions(projectRoot: vscode.Uri, manifest: ExtensionManifest, menu: MenuDocument): Promise<void> {
  const runtime = runtimeId(manifest);
  const parts = generatedMenuPath(runtime).split('/');
  const target = vscode.Uri.joinPath(projectRoot, ...parts);
  await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(projectRoot, ...parts.slice(0, -1)));
  await vscode.workspace.fs.writeFile(target, new TextEncoder().encode(generateMenuActions(runtime, menu)));
}

async function generatedActionsEnabled(projectRoot: vscode.Uri, manifest: ExtensionManifest): Promise<boolean> {
  try {
    const entry = await vscode.workspace.fs.readFile(vscode.Uri.joinPath(projectRoot, manifest.entryPoint));
    return new TextDecoder().decode(entry).includes(menuDispatchMarker);
  } catch { return false; }
}

async function pickCommandIcon(document: vscode.TextDocument, projectRoot: vscode.Uri, index: number, dark: boolean): Promise<void> {
  const manifest = parseManifest(document.getText());
  const command = manifest.commands?.[index];
  if (!command) throw new Error(t('The selected command no longer exists.'));
  const selected = (await vscode.window.showOpenDialog({
    title: t(dark ? 'Select Dark Command SVG' : 'Select Light Command SVG'), canSelectFiles: true,
    canSelectFolders: false, canSelectMany: false, filters: { [t('SVG images')]: ['svg'] },
  }))?.[0];
  if (!selected) return;
  const safeHandler = (command.handler || 'command').replace(/[^A-Za-z0-9_-]+/g, '_');
  const relative = `icons/${safeHandler}${dark ? '-dark' : ''}.svg`;
  const target = vscode.Uri.joinPath(projectRoot, ...relative.split('/'));
  await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(projectRoot, 'icons'));
  await vscode.workspace.fs.copy(selected, target, { overwrite: true });
  const commands = [...(manifest.commands ?? [])];
  commands[index] = { ...command, [dark ? 'iconDark' : 'icon']: relative };
  const edit = new vscode.WorkspaceEdit();
  edit.replace(document.uri, fullRange(document), `${JSON.stringify({ ...manifest, commands }, null, 2)}\n`);
  await vscode.workspace.applyEdit(edit);
}

function runtimeId(manifest: ExtensionManifest): ExtensionScaffoldSpec['runtime'] {
  const id = typeof manifest.runtime === 'string' ? manifest.runtime : manifest.runtime.id;
  if (!id || !extensionRuntimeIds.includes(id as ExtensionScaffoldSpec['runtime'])) throw new Error(`Unsupported extension runtime: ${id ?? 'missing'}`);
  return id as ExtensionScaffoldSpec['runtime'];
}

function iconSource(webview: vscode.Webview, root: vscode.Uri, relative: unknown): string | undefined {
  if (typeof relative !== 'string' || !relative) return undefined;
  const resolved = path.resolve(root.fsPath, relative);
  const inside = path.relative(root.fsPath, resolved);
  if (inside.startsWith('..') || path.isAbsolute(inside)) return undefined;
  return webview.asWebviewUri(vscode.Uri.file(resolved)).toString();
}

function fullRange(document: vscode.TextDocument): vscode.Range {
  return new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length));
}

function randomNonce(): string {
  const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  return Array.from({ length: 32 }, () => alphabet[Math.floor(Math.random() * alphabet.length)]).join('');
}
