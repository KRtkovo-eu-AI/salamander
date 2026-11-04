/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-04 | Describe an extension by its path inside the workspace. | Extensions that share a display name need a visible source path. | https://github.com/KRtkovo-eu-AI/salamander/issues/776 | Grok 4.7 |
*/
import * as path from 'node:path';

export function formatProjectLocation(
  rootFsPath: string,
  workspaceRootFsPath: string | undefined,
  workspaceName: string | undefined,
  workspaceCount: number,
): string {
  if (!workspaceRootFsPath) return rootFsPath;
  const relative = path.relative(workspaceRootFsPath, rootFsPath);
  if (!relative || relative === '.') return workspaceName || rootFsPath;
  if (relative.startsWith(`..${path.sep}`) || relative === '..' || path.isAbsolute(relative)) return rootFsPath;
  const normalized = relative.split(path.sep).join('/');
  return workspaceCount > 1 && workspaceName ? `${workspaceName}/${normalized}` : normalized;
}
