import { execFileSync } from 'node:child_process';
import { existsSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const programFilesX86 = process.env['ProgramFiles(x86)'] ?? 'C:\\Program Files (x86)';
const vswhere = join(programFilesX86, 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
let msbuild = 'msbuild.exe';
let installation = '';
if (existsSync(vswhere)) {
  installation = execFileSync(vswhere, ['-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild', '-property', 'installationPath'], { encoding: 'utf8' }).trim();
  const candidate = join(installation, 'MSBuild', 'Current', 'Bin', 'MSBuild.exe');
  if (installation && existsSync(candidate)) msbuild = candidate;
}
execFileSync(msbuild, [
  'preview-host/SalamatrixStudio.Host.vcxproj', '/m', '/t:Build',
  '/p:Configuration=Release', '/p:Platform=x64', '/nr:false', '/v:minimal',
], { cwd: new URL('.', import.meta.url), stdio: 'inherit' });

const toolsRoot = join(installation, 'VC', 'Tools', 'MSVC');
const toolVersions = existsSync(toolsRoot)
  ? readdirSync(toolsRoot, { withFileTypes: true })
      .filter((entry) => entry.isDirectory())
      .map((entry) => entry.name)
      .sort((left, right) => right.localeCompare(left, undefined, { numeric: true }))
  : [];
const dumpbin = toolVersions
  .map((version) => join(toolsRoot, version, 'bin', 'Hostx64', 'x64', 'dumpbin.exe'))
  .find(existsSync);
if (!dumpbin) {
  throw new Error('Unable to locate the x64 MSVC dumpbin.exe required to verify preview host dependencies.');
}

const host = fileURLToPath(new URL('./native/win32-x64/SalamatrixStudio.Host.exe', import.meta.url));
const dependencies = execFileSync(dumpbin, ['/dependents', host], { encoding: 'utf8' });
const forbiddenRuntime = /^\s*((?:concrt|msvcp|vcruntime)\d+(?:_[a-z0-9_]+)?\.dll|ucrtbase\.dll|api-ms-win-crt-[a-z0-9-]+\.dll)\s*$/gim;
const forbiddenImports = [...dependencies.matchAll(forbiddenRuntime)].map((match) => match[1]);
if (forbiddenImports.length > 0) {
  throw new Error(`Preview host must use the static CRT; forbidden imports: ${forbiddenImports.join(', ')}`);
}
