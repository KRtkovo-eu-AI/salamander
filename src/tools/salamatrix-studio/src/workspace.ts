/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-04 | Keep the workspace path of each discovered extension. | The project view and extension picker need that path to separate identical names. | https://github.com/KRtkovo-eu-AI/salamander/issues/776 | Grok 4.7 |
*/
import * as path from 'node:path';
import * as fs from 'node:fs';
import * as vscode from 'vscode';
import { formatProjectLocation } from './projectLocation.js';

export interface ExtensionProject {
  root: vscode.Uri;
  manifest: vscode.Uri;
  name: string;
  runtime: string;
  entryPoint: string;
  location: string;
}

export async function discoverProjects(): Promise<ExtensionProject[]> {
  const manifests = await vscode.workspace.findFiles(
    '**/extension.json',
    '**/{node_modules,.git,build,output,out,.localbuild}/**',
    100,
  );
  const folders = vscode.workspace.workspaceFolders ?? [];
  const projects: ExtensionProject[] = [];
  const projectAt = (
    manifest: vscode.Uri,
    rootDir: string,
    name: string,
    runtime: string,
    entryPoint: string,
  ): ExtensionProject => {
    const folder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(rootDir));
    return {
      root: vscode.Uri.file(rootDir),
      manifest,
      name,
      runtime,
      entryPoint,
      location: formatProjectLocation(rootDir, folder?.uri.fsPath, folder?.name, folders.length),
    };
  };
  for (const manifest of manifests) {
    const rootDir = path.dirname(manifest.fsPath);
    try {
      const bytes = await vscode.workspace.fs.readFile(manifest);
      const json = JSON.parse(new TextDecoder().decode(bytes)) as {
        name?: string;
        title?: string;
        runtime?: string | { id?: string };
        entryPoint?: string;
      };
      const runtime = typeof json.runtime === 'string' ? json.runtime : json.runtime?.id ?? 'Unknown';
      projects.push(projectAt(
        manifest,
        rootDir,
        json.name ?? json.title ?? path.basename(rootDir),
        runtime,
        typeof json.entryPoint === 'string' ? json.entryPoint : 'main',
      ));
    } catch {
      projects.push(projectAt(manifest, rootDir, path.basename(rootDir), 'Invalid manifest', 'extension.json'));
    }
  }
  return projects.sort((a, b) => a.name.localeCompare(b.name) || a.location.localeCompare(b.location));
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
    projects.map((project) => ({
      label: project.name,
      description: project.location,
      detail: project.runtime,
      project,
    })),
    { placeHolder: 'Select the Salamatrix extension project' },
  );
  return picked?.project;
}

export async function readJson(uri: vscode.Uri): Promise<unknown> {
  const bytes = await vscode.workspace.fs.readFile(uri);
  return JSON.parse(new TextDecoder().decode(bytes));
}
