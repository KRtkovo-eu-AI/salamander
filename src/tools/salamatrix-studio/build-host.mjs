import { execFileSync } from 'node:child_process';
import { existsSync } from 'node:fs';
import { join } from 'node:path';

const programFilesX86 = process.env['ProgramFiles(x86)'] ?? 'C:\\Program Files (x86)';
const vswhere = join(programFilesX86, 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
let msbuild = 'msbuild.exe';
if (existsSync(vswhere)) {
  const installation = execFileSync(vswhere, ['-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild', '-property', 'installationPath'], { encoding: 'utf8' }).trim();
  const candidate = join(installation, 'MSBuild', 'Current', 'Bin', 'MSBuild.exe');
  if (installation && existsSync(candidate)) msbuild = candidate;
}
execFileSync(msbuild, [
  'preview-host/SalamatrixStudio.Host.vcxproj', '/m', '/t:Build',
  '/p:Configuration=Release', '/p:Platform=x64', '/nr:false', '/v:minimal',
], { cwd: new URL('.', import.meta.url), stdio: 'inherit' });
