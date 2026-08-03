import { existsSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { generateMenuActions } from '../src/extensionScaffold.js';
import type { MenuDocument } from '../src/menuModel.js';

const menu: MenuDocument = {
  schema: 1,
  generatedBy: 'SalamatrixStudio',
  commands: [{
    handler: 'run-special', action: 'program', target: 'tool "quoted".exe',
    arguments: '--name "Příliš žluťoučký kůň"', workingDirectory: 'C:\\Work Folder',
  }],
};

describe('generated menu dispatcher syntax', () => {
  it('parses Automation JScript as ECMAScript', () => {
    expect(() => new Function(generateMenuActions('Automation.JScript', menu))).not.toThrow();
  });

  it('parses every generated dispatcher with an available runtime', () => {
    const checks: Array<{ runtime: Parameters<typeof generateMenuActions>[0]; command: string; args: string[] }> = [
      {
        runtime: 'PowerShell', command: 'powershell.exe', args: ['-NoProfile', '-Command',
          '$text=[Console]::In.ReadToEnd();$tokens=$null;$errors=$null;[System.Management.Automation.Language.Parser]::ParseInput($text,[ref]$tokens,[ref]$errors)|Out-Null;if($errors.Count){$errors|ForEach-Object{[Console]::Error.WriteLine($_)};exit 1}'],
      },
      { runtime: 'Python.CPython', command: 'python.exe', args: ['-c', 'import sys; compile(sys.stdin.read(), "generated.py", "exec")'] },
      { runtime: 'JavaScript.Node', command: process.execPath, args: ['--check', '--input-type=module'] },
      { runtime: 'PHP.CLI', command: 'php.exe', args: ['-l'] },
    ];
    const lua = resolve('../../../build/vcpkg_installed_third_party/x64-windows/tools/lua/lua.exe');
    if (existsSync(lua)) checks.push({ runtime: 'Lua', command: lua, args: ['-e', 'assert(load(io.read("*a")))'] });

    for (const check of checks) {
      if (check.command !== process.execPath && spawnSync('where.exe', [check.command]).status !== 0 && !existsSync(check.command)) continue;
      const result = spawnSync(check.command, check.args, {
        input: generateMenuActions(check.runtime, menu), encoding: 'utf8', timeout: 10_000,
        env: { ...process.env, PYTHONUTF8: '1' },
      });
      expect(result.status, `${check.runtime}: ${result.stderr || result.stdout}`).toBe(0);
    }
  });
});
