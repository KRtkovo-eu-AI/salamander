import { execFileSync } from 'node:child_process';
import { copyFileSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const outputDirectory = fileURLToPath(
  new URL('../../../build/salamatrix-studio/', import.meta.url),
);
const vsce = fileURLToPath(new URL('./node_modules/@vscode/vsce/vsce', import.meta.url));

mkdirSync(outputDirectory, { recursive: true });
execFileSync(process.execPath, [vsce, 'package', '--target', 'win32-x64', '--out', outputDirectory], {
  cwd: new URL('.', import.meta.url),
  stdio: 'inherit',
});

// Keep the human-readable release notes beside the VSIX for GitHub Releases
// and manual installation. The same README is also included inside the VSIX
// by vsce.
copyFileSync(
  fileURLToPath(new URL('./README.md', import.meta.url)),
  fileURLToPath(new URL('../../../build/salamatrix-studio/README.md', import.meta.url)),
);
