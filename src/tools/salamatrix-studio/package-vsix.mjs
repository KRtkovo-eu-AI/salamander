import { execFileSync } from 'node:child_process';
import { mkdirSync } from 'node:fs';
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
