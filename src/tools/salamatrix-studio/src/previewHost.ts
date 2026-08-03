import { ChildProcess, spawn } from 'node:child_process';
import { mkdir, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import * as vscode from 'vscode';
import type { DialogDocument } from './model.js';
import { encodePreview } from './previewTransport.js';

export class PreviewHost implements vscode.Disposable {
  private child: ChildProcess | undefined;
  private modelPath: string | undefined;

  constructor(private readonly context: vscode.ExtensionContext) {}

  async show(dialog: DialogDocument): Promise<void> {
    this.close();
    const executable = vscode.Uri.joinPath(this.context.extensionUri, 'native', 'win32-x64', 'SalamatrixStudio.Host.exe').fsPath;
    const directory = path.join(tmpdir(), 'SalamatrixStudio');
    await mkdir(directory, { recursive: true });
    this.modelPath = path.join(directory, `preview-${process.pid}-${Date.now()}.smxp`);
    await writeFile(this.modelPath, encodePreview(dialog), 'utf8');
    this.child = spawn(executable, [this.modelPath], { windowsHide: false, stdio: 'ignore' });
    const current = this.child;
    current.once('error', (error) => void vscode.window.showErrorMessage(`Cannot start native preview: ${error.message}`));
    current.once('exit', () => {
      if (this.child === current) this.child = undefined;
      void this.removeModel();
    });
  }

  close(): void {
    if (this.child && !this.child.killed) this.child.kill();
    this.child = undefined;
    void this.removeModel();
  }

  dispose(): void { this.close(); }

  private async removeModel(): Promise<void> {
    const target = this.modelPath;
    this.modelPath = undefined;
    if (target) await rm(target, { force: true });
  }
}
