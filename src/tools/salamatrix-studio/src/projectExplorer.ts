/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-04 | Show the extension source path beside its name. | Identical extension names are otherwise indistinguishable in the project view. | https://github.com/KRtkovo-eu-AI/salamander/issues/776 | Grok 4.7 |
*/
import * as path from 'node:path';
import * as vscode from 'vscode';
import { ExtensionProject, discoverProjects } from './workspace.js';
import { t } from './localize.js';

type Node = ProjectNode | FileNode | GroupNode | SectionNode | DemoNode;

class ProjectNode extends vscode.TreeItem {
  constructor(public readonly project: ExtensionProject) {
    super(project.name, vscode.TreeItemCollapsibleState.Expanded);
    this.description = `${project.location} · ${project.runtime}`;
    this.tooltip = `${project.name}\n${project.root.fsPath}`;
    this.iconPath = new vscode.ThemeIcon('package');
    this.contextValue = 'salamatrixProject';
  }
}

class GroupNode extends vscode.TreeItem {
  constructor(public readonly project: ExtensionProject, public readonly group: 'dialogs' | 'source') {
    super(t(group === 'dialogs' ? 'Dialogs' : 'Source Files'), vscode.TreeItemCollapsibleState.Expanded);
    this.iconPath = new vscode.ThemeIcon(group === 'dialogs' ? 'window' : 'files');
    this.contextValue = group === 'dialogs' ? 'salamatrixDialogs' : 'salamatrixSourceFiles';
  }
}

class SectionNode extends vscode.TreeItem {
  constructor(project: ExtensionProject, section: 'overview' | 'menus') {
    super(t(section === 'overview' ? 'Overview' : 'Menu Builder'), vscode.TreeItemCollapsibleState.None);
    this.iconPath = new vscode.ThemeIcon(section === 'overview' ? 'project' : 'list-tree');
    this.contextValue = section === 'overview' ? 'salamatrixOverview' : 'salamatrixMenuBuilder';
    this.command = {
      command: 'salamatrixStudio.openManifestDesigner', title: this.label as string,
      arguments: [project.manifest, section],
    };
  }
}

class DemoNode extends vscode.TreeItem {
  constructor() {
    super(t('UI Capabilities Demo'), vscode.TreeItemCollapsibleState.None);
    this.description = t('Open the bundled dialog gallery');
    this.iconPath = new vscode.ThemeIcon('window');
    this.contextValue = 'salamatrixUiCapabilitiesDemo';
    this.command = {
      command: 'salamatrixStudio.openUiCapabilitiesDemo',
      title: t('Open UI Capabilities Demo'),
    };
  }
}

class FileNode extends vscode.TreeItem {
  constructor(label: string, public readonly uri: vscode.Uri, icon: string, contextValue?: string) {
    super(label, vscode.TreeItemCollapsibleState.None);
    this.resourceUri = uri;
    this.iconPath = new vscode.ThemeIcon(icon);
    this.command = { command: 'vscode.open', title: t('Open'), arguments: [uri] };
    this.contextValue = contextValue;
  }
}

export class StudioProjectExplorer implements vscode.TreeDataProvider<Node> {
  private readonly changed = new vscode.EventEmitter<Node | undefined | void>();
  readonly onDidChangeTreeData = this.changed.event;
  private projects: ExtensionProject[] = [];

  async refresh(): Promise<number> {
    this.projects = await discoverProjects();
    this.changed.fire();
    return this.projects.length;
  }

  getTreeItem(element: Node): vscode.TreeItem {
    return element;
  }

  async getChildren(element?: Node): Promise<Node[]> {
    if (!element) {
      if (this.projects.length === 0) this.projects = await discoverProjects();
      const nodes: Node[] = this.projects.map((project) => new ProjectNode(project));
      if (nodes.length > 0) nodes.push(new DemoNode());
      return nodes;
    }
    if (element instanceof ProjectNode) {
      return [
        new SectionNode(element.project, 'overview'),
        new SectionNode(element.project, 'menus'),
        new GroupNode(element.project, 'dialogs'),
        new GroupNode(element.project, 'source'),
      ];
    }
    if (element instanceof GroupNode) {
      if (element.group === 'dialogs') {
        const pattern = new vscode.RelativePattern(
          vscode.Uri.joinPath(element.project.root, '.salamatrix', 'dialogs'), '*.salamatrix-dialog.json',
        );
        const dialogs = await vscode.workspace.findFiles(pattern);
        return dialogs.sort((a, b) => a.fsPath.localeCompare(b.fsPath))
          .map((uri) => new FileNode(path.basename(uri.fsPath, '.salamatrix-dialog.json'), uri, 'symbol-class'));
      }
      const entry = vscode.Uri.joinPath(element.project.root, element.project.entryPoint);
      const generated = await vscode.workspace.findFiles(
        new vscode.RelativePattern(vscode.Uri.joinPath(element.project.root, 'generated'), '**/*'),
      );
      return [new FileNode(element.project.entryPoint, entry, 'code')]
        .concat(generated.sort((a, b) => a.fsPath.localeCompare(b.fsPath))
          .map((uri) => new FileNode(path.relative(element.project.root.fsPath, uri.fsPath), uri, 'symbol-file')));
    }
    return [];
  }
}
