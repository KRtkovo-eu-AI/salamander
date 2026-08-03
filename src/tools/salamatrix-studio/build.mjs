import * as esbuild from 'esbuild';
import { cp, mkdir, rm } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = dirname(fileURLToPath(import.meta.url));
const dist = join(root, 'dist');
const sdkContracts = join(root, '..', '..', 'salamatrix-sdk', 'contracts');
const repositoryLicense = join(root, '..', '..', '..', 'LICENSE');
const clean = process.argv.includes('--clean');
const watch = process.argv.includes('--watch');

if (clean) {
  await rm(dist, { recursive: true, force: true });
  if (process.argv.length === 3) process.exit(0);
}

await mkdir(join(dist, 'contracts'), { recursive: true });
await cp(sdkContracts, join(dist, 'contracts'), { recursive: true });
await cp(repositoryLicense, join(root, 'LICENSE'));

const builds = [
  {
    entryPoints: [join(root, 'src', 'extension.ts')],
    outfile: join(dist, 'extension.js'),
    platform: 'node',
    format: 'cjs',
    external: ['vscode'],
  },
  {
    entryPoints: [join(root, 'src', 'webview', 'index.tsx')],
    outfile: join(dist, 'webview.js'),
    platform: 'browser',
    format: 'iife',
  },
];

const common = {
  bundle: true,
  sourcemap: true,
  minify: true,
  target: 'es2022',
  logLevel: 'info',
};

if (watch) {
  for (const build of builds) {
    const context = await esbuild.context({ ...common, ...build });
    await context.watch();
  }
  console.log('Salamatrix Studio build is watching for changes.');
} else {
  await Promise.all(builds.map((build) => esbuild.build({ ...common, ...build })));
}

await cp(join(root, 'src', 'webview', 'styles.css'), join(dist, 'webview.css'));
