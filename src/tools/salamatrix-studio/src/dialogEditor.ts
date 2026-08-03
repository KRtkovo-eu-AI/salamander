import * as vscode from 'vscode';
import { ControlCatalog, DialogDocument, parseDialogDocument, serializeDialogDocument } from './model.js';

interface WebviewMessage {
  type: 'ready' | 'update' | 'generate' | 'generateForRuntime' | 'preview';
  dialog?: DialogDocument;
}

export class DialogEditorProvider implements vscode.CustomTextEditorProvider {
  static readonly viewType = 'salamatrixStudio.dialogDesigner';
  activeDocument: vscode.Uri | undefined;

  constructor(
    private readonly context: vscode.ExtensionContext,
    private readonly catalog: ControlCatalog,
  ) {}

  async resolveCustomTextEditor(
    document: vscode.TextDocument,
    webviewPanel: vscode.WebviewPanel,
  ): Promise<void> {
    this.activeDocument = document.uri;
    webviewPanel.webview.options = {
      enableScripts: true,
      localResourceRoots: [vscode.Uri.joinPath(this.context.extensionUri, 'dist')],
    };
    webviewPanel.webview.html = this.html(webviewPanel.webview);

    const sendDocument = (): void => {
      try {
        const dialog = parseDialogDocument(document.getText());
        void webviewPanel.webview.postMessage({ type: 'document', dialog, catalog: this.catalog });
      } catch (error) {
        void webviewPanel.webview.postMessage({
          type: 'error',
          message: error instanceof Error ? error.message : String(error),
        });
      }
    };

    const messageSubscription = webviewPanel.webview.onDidReceiveMessage(
      async (message: WebviewMessage) => {
        if (message.type === 'ready') {
          sendDocument();
        } else if (message.type === 'update' && message.dialog) {
          const edit = new vscode.WorkspaceEdit();
          edit.replace(document.uri, fullRange(document), serializeDialogDocument(message.dialog));
          await vscode.workspace.applyEdit(edit);
        } else if (message.type === 'generate') {
          await vscode.commands.executeCommand('salamatrixStudio.generateDialog', document.uri);
        } else if (message.type === 'generateForRuntime') {
          await vscode.commands.executeCommand('salamatrixStudio.generateDialogForRuntime', document.uri);
        } else if (message.type === 'preview') {
          await vscode.commands.executeCommand('salamatrixStudio.previewDialog', document.uri);
        }
      },
    );
    const documentSubscription = vscode.workspace.onDidChangeTextDocument((event) => {
      if (event.document.uri.toString() === document.uri.toString()) sendDocument();
    });
    webviewPanel.onDidChangeViewState((event) => {
      if (event.webviewPanel.active) this.activeDocument = document.uri;
    });
    webviewPanel.onDidDispose(() => {
      messageSubscription.dispose();
      documentSubscription.dispose();
      if (this.activeDocument?.toString() === document.uri.toString()) this.activeDocument = undefined;
    });
  }

  private html(webview: vscode.Webview): string {
    const script = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'webview.js'));
    const style = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'webview.css'));
    const nonce = randomNonce();
    return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource} 'unsafe-inline'; script-src 'nonce-${nonce}';">
  <link rel="stylesheet" href="${style}">
  <title>Salamatrix Dialog Designer</title>
</head>
<body>
  <div id="root"></div>
  <script nonce="${nonce}" src="${script}"></script>
</body>
</html>`;
  }
}

function fullRange(document: vscode.TextDocument): vscode.Range {
  return new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length));
}

function randomNonce(): string {
  const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  return Array.from({ length: 32 }, () => alphabet[Math.floor(Math.random() * alphabet.length)]).join('');
}
