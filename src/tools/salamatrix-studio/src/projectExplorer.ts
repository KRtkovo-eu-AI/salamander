import * as path from 'node:path';
import * as vscode from 'vscode';
import { ExtensionProject, discoverProjects } from './workspace.js';

type Node = ProjectNode | FileNode | GroupNode;

class ProjectNode extends vscode.TreeItem {
  constructor(public readonly project: ExtensionProject) {
    super(project.name, vscode.TreeItemCollapsibleState.Expanded);
    this.description = project.runtime;
    this.iconPath = new vscode.ThemeIcon('package');
    this.contextValue = 'salamatrixProject';
  }
}

class GroupNode extends vscode.TreeItem {
  constructor(public readonly project: ExtensionProject, public readonly group: 'dialogs') {
    super('Dialogs', vscode.TreeItemCollapsibleState.Expanded);
    this.iconPath = new vscode.ThemeIcon('window');
  }
}

class FileNode extends vscode.TreeItem {
  constructor(label: string, public readonly uri: vscode.Uri, icon: string, contextValue?: string) {
    super(label, vscode.TreeItemCollapsibleState.None);
    this.resourceUri = uri;
    this.iconPath = new vscode.ThemeIcon(icon);
    this.command = { command: 'vscode.open', title: 'Open', arguments: [uri] };
    this.contextValue = contextValue;
  }
}

export class StudioProjectExplorer implements vscode.TreeDataProvider<Node> {
  private readonly changed = new vscode.EventEmitter<Node | undefined | void>();
  readonly onDidChangeTreeData = this.changed.event;
  private projects: ExtensionProject[] = [];

  async refresh(): Promise<void> {
    this.projects = await discoverProjects();
    this.changed.fire();
  }

  getTreeItem(element: Node): vscode.TreeItem {
    return element;
  }

  async getChildren(element?: Node): Promise<Node[]> {
    if (!element) {
      if (this.projects.length === 0) this.projects = await discoverProjects();
      return this.projects.map((project) => new ProjectNode(project));
    }
    if (element instanceof ProjectNode) {
      return [
        new FileNode('extension.json', element.project.manifest, 'json', 'salamatrixManifest'),
        new GroupNode(element.project, 'dialogs'),
      ];
    }
    if (element instanceof GroupNode) {
      const pattern = new vscode.RelativePattern(
        vscode.Uri.joinPath(element.project.root, '.salamatrix', 'dialogs'),
        '*.salamatrix-dialog.json',
      );
      const dialogs = await vscode.workspace.findFiles(pattern);
      return dialogs
        .sort((a, b) => a.fsPath.localeCompare(b.fsPath))
        .map((uri) => new FileNode(path.basename(uri.fsPath, '.salamatrix-dialog.json'), uri, 'symbol-class'));
    }
    return [];
  }
}
