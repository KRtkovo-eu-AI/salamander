import * as vscode from 'vscode';
import { ExtensionManifest, parseManifest, validateManifest } from './manifestModel.js';

interface ManifestMessage {
  type: 'ready' | 'update';
  manifest?: ExtensionManifest;
}

export class ManifestEditorProvider implements vscode.CustomTextEditorProvider {
  static readonly viewType = 'salamatrixStudio.manifestDesigner';

  constructor(private readonly context: vscode.ExtensionContext) {}

  async resolveCustomTextEditor(document: vscode.TextDocument, panel: vscode.WebviewPanel): Promise<void> {
    panel.webview.options = {
      enableScripts: true,
      localResourceRoots: [vscode.Uri.joinPath(this.context.extensionUri, 'dist')],
    };
    panel.webview.html = this.html(panel.webview);

    const sendDocument = (): void => {
      try {
        const manifest = parseManifest(document.getText());
        void panel.webview.postMessage({ type: 'document', manifest });
      } catch (error) {
        void panel.webview.postMessage({ type: 'error', message: error instanceof Error ? error.message : String(error) });
      }
    };
    const messages = panel.webview.onDidReceiveMessage(async (message: ManifestMessage) => {
      if (message.type === 'ready') sendDocument();
      if (message.type === 'update' && message.manifest) {
        try {
          const manifest = validateManifest(message.manifest);
          const edit = new vscode.WorkspaceEdit();
          edit.replace(document.uri, fullRange(document), `${JSON.stringify(manifest, null, 2)}\n`);
          await vscode.workspace.applyEdit(edit);
        } catch (error) {
          void panel.webview.postMessage({ type: 'error', message: error instanceof Error ? error.message : String(error) });
        }
      }
    });
    const changes = vscode.workspace.onDidChangeTextDocument((event) => {
      if (event.document.uri.toString() === document.uri.toString()) sendDocument();
    });
    panel.onDidDispose(() => { messages.dispose(); changes.dispose(); });
  }

  private html(webview: vscode.Webview): string {
    const script = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'manifest-webview.js'));
    const style = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'manifest-webview.css'));
    const nonce = randomNonce();
    return `<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource} 'unsafe-inline'; script-src 'nonce-${nonce}';"><link rel="stylesheet" href="${style}"><title>Salamatrix Extension Designer</title></head><body><div id="root"></div><script nonce="${nonce}" src="${script}"></script></body></html>`;
  }
}

function fullRange(document: vscode.TextDocument): vscode.Range {
  return new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length));
}

function randomNonce(): string {
  const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  return Array.from({ length: 32 }, () => alphabet[Math.floor(Math.random() * alphabet.length)]).join('');
}
