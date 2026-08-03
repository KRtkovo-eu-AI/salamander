import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { controlKinds } from '../src/model.js';

const contracts = resolve(process.cwd(), '..', '..', 'salamatrix-sdk', 'contracts');
const previewProject = resolve(process.cwd(), 'preview-host', 'SalamatrixStudio.Host.vcxproj');

describe('shared SDK contracts', () => {
  it('keeps the control catalog aligned with the TypeScript model', () => {
    const catalog = JSON.parse(readFileSync(resolve(contracts, 'control-catalog.json'), 'utf8')) as {
      controls: Array<{ kind: string }>;
    };
    expect(catalog.controls.map((control) => control.kind)).toEqual([...controlKinds]);
  });

  it('ships parseable JSON schemas', () => {
    for (const name of ['salamatrix-dialog.schema.json', 'studio-host-protocol.schema.json']) {
      expect(() => JSON.parse(readFileSync(resolve(contracts, name), 'utf8'))).not.toThrow();
    }
  });

  it('builds the native preview with modern controls and bundled darkmodelib', () => {
    const project = readFileSync(previewProject, 'utf8');
    expect(project).toContain("name='Microsoft.Windows.Common-Controls' version='6.0.0.0'");
    expect(project).toContain('USE_DARKMODELIB=1');
    expect(project).toContain('third_party\\darkmodelib\\src\\Darkmodelib.cpp');
    expect(project).toContain('<RuntimeLibrary>MultiThreaded</RuntimeLibrary>');

    const host = readFileSync(resolve(process.cwd(), '..', '..', 'salamatrix-sdk', 'native-ui-runtime', 'salamatrix_ui_win32_host.cpp'), 'utf8');
    expect(host).toContain('SetWin32NativeDialogDarkMode');
    expect(host).toContain('dmlib::setChildCtrlsSubclassAndTheme');
  });

  it('implements the Salamander-specific preview controls instead of placeholder glyphs', () => {
    const controls = readFileSync(resolve(process.cwd(), '..', '..', 'salamatrix-sdk', 'native-ui-runtime', 'salamatrix_ui_controls.cpp'), 'utf8');
    expect(controls).toContain('class NativeToolTip');
    expect(controls).toContain('TTF_IDISHWND | TTF_SUBCLASS');
    expect(controls).toContain('Indeterminate = progress == static_cast<DWORD>(-1)');
    expect(controls).toContain('SetTimer(Window, 1');
    expect(controls).toContain('Flags & STF_HYPERLINK_COLOR');
    expect(controls).toContain('Flags & STF_DOTUNDERLINE');
    expect(controls).toContain('Caption(Wide(WindowText(window).c_str()))');
    expect(controls).not.toContain('SetWindowTextW(window, L"✎  ＋  ×  ↕  ↑  ↓  ⇈  ⌕")');
  });

  it('ships the exact 463 x 236 UI capabilities gallery as a Studio project', () => {
    const demoPath = resolve(process.cwd(), 'examples', 'salamatrix-ui-capabilities');
    const dialog = JSON.parse(readFileSync(resolve(demoPath, '.salamatrix', 'dialogs', 'ui-capabilities.salamatrix-dialog.json'), 'utf8')) as {
      width: number; height: number; controls: Array<{ id: string; bounds: { x: number; y: number; width: number; height: number }; options: Record<string, unknown> }>;
    };
    expect(dialog.width).toBe(463);
    expect(dialog.height).toBe(236);
    expect(dialog.controls).toHaveLength(47);
    expect(dialog.controls.find((control) => control.id === 'static-group')?.bounds).toEqual({ x: 6, y: 4, width: 254, height: 108 });
    expect(dialog.controls.find((control) => control.id === 'header-list')?.options.styleFlags).toBe(0x01e00000);
    expect(dialog.controls.find((control) => control.id === 'toolbar-header')?.options).toMatchObject({ alignControlId: 'header-list', buttonMask: 0x31 });
    expect(dialog.controls.find((control) => control.id === 'close')).toMatchObject({ bounds: { x: 403, y: 213, width: 50, height: 14 }, options: { dialogResult: 1, styleFlags: 0x100000 } });
    expect(() => JSON.parse(readFileSync(resolve(demoPath, 'extension.json'), 'utf8'))).not.toThrow();
    expect(readFileSync(resolve(demoPath, 'main.ps1'), 'utf8')).toContain('generated/ui-capabilities-dialog.generated.ps1');
  });
});
