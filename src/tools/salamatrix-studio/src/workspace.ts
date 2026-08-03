import * as path from 'node:path';
import * as fs from 'node:fs';
import * as vscode from 'vscode';

export interface ExtensionProject {
  root: vscode.Uri;
  manifest: vscode.Uri;
  name: string;
  runtime: string;
}

export async function discoverProjects(): Promise<ExtensionProject[]> {
  const manifests = await vscode.workspace.findFiles(
    '**/extension.json',
    '**/{node_modules,.git,build,output,out,.localbuild}/**',
    100,
  );
  const projects: ExtensionProject[] = [];
  for (const manifest of manifests) {
    try {
      const bytes = await vscode.workspace.fs.readFile(manifest);
      const json = JSON.parse(new TextDecoder().decode(bytes)) as {
        name?: string;
        title?: string;
        runtime?: string | { id?: string };
      };
      const runtime = typeof json.runtime === 'string' ? json.runtime : json.runtime?.id ?? 'Unknown';
      projects.push({
        root: vscode.Uri.file(path.dirname(manifest.fsPath)),
        manifest,
        name: json.name ?? json.title ?? path.basename(path.dirname(manifest.fsPath)),
        runtime,
      });
    } catch {
      projects.push({
        root: vscode.Uri.file(path.dirname(manifest.fsPath)),
        manifest,
        name: path.basename(path.dirname(manifest.fsPath)),
        runtime: 'Invalid manifest',
      });
    }
  }
  return projects.sort((a, b) => a.name.localeCompare(b.name));
}

export function findProjectRoot(uri: vscode.Uri): vscode.Uri | undefined {
  const folder = vscode.workspace.getWorkspaceFolder(uri);
  let current = fs.statSync(uri.fsPath).isDirectory() ? uri.fsPath : path.dirname(uri.fsPath);
  const stop = folder?.uri.fsPath;
  while (true) {
    if (fs.existsSync(path.join(current, 'extension.json'))) return vscode.Uri.file(current);
    if (current === stop) return undefined;
    const parent = path.dirname(current);
    if (parent === current) return undefined;
    current = parent;
  }
}

export async function chooseProject(): Promise<ExtensionProject | undefined> {
  const projects = await discoverProjects();
  if (projects.length === 0) {
    void vscode.window.showErrorMessage('No extension.json was found in the current workspace.');
    return undefined;
  }
  if (projects.length === 1) return projects[0];
  const picked = await vscode.window.showQuickPick(
    projects.map((project) => ({ label: project.name, description: project.runtime, project })),
    { placeHolder: 'Select the Salamatrix extension project' },
  );
  return picked?.project;
}

export async function readJson(uri: vscode.Uri): Promise<unknown> {
  const bytes = await vscode.workspace.fs.readFile(uri);
  return JSON.parse(new TextDecoder().decode(bytes));
}
