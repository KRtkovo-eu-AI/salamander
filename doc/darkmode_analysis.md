# Windows native dark mode integration for Salamander

## Current limitations
- `ColorsChanged()` toggles Salamander's custom palette arrays and repaints toolbars, but it only calls the internal dark palette helpers and never opts windows into the native Explorer theme; even when the configuration flag is set it just repaints local brushes.【F:src/salamdr1.cpp†L3191-L3235】
- The Colors configuration page exposes predefined schemes yet the "Windows dark mode" entry is only a combo-box option with no descriptive control, so users cannot discover or persist a dedicated native-dark preference outside the hard-coded scheme slot.【F:src/dialogs4.cpp†L3335-L3376】【F:src/lang/lang.rc†L798-L826】
- Main-frame creation applies Salamander-specific painting logic without calling Windows dark mode APIs on child windows, rebars, or menus, leaving system-drawn surfaces in light mode even when the palette flips.【F:src/mainwnd3.cpp†L1011-L1108】【F:src/mainwnd3.cpp†L1200-L1286】
- Common dialog helpers in WinLib and the menu subsystem do not invoke any of the undocumented `uxtheme.dll` entry points (e.g., `AllowDarkModeForWindow`, `_FlushMenuThemes`), so popups, plugin dialogs, and legacy controls ignore the OS-wide dark preference.【F:src/common/winlib.cpp†L242-L320】【F:src/menu1.cpp†L1-L120】

## Relevant Windows APIs (from `win32-darkmode` sample)
- The reference project resolves ordinals 132–139 from `uxtheme.dll` to obtain `ShouldAppsUseDarkMode`, `AllowDarkModeForWindow`, `SetPreferredAppMode`, `FlushMenuThemes`, and friends, caching function pointers behind helper wrappers.【F:.3rd-party/win32-darkmode-master.zip/win32-darkmode-master/win32-darkmode/DarkMode.h†L1-L85】
- It also toggles immersive title bars through `SetWindowCompositionAttribute` with the undocumented `WCA_USEDARKMODECOLORS` flag and exposes scrollbar theming hooks by intercepting `OpenNcThemeData` calls before window creation.【F:.3rd-party/win32-darkmode-master.zip/win32-darkmode-master/win32-darkmode/DarkMode.h†L19-L77】【F:.3rd-party/win32-darkmode-master.zip/win32-darkmode-master/win32-darkmode/DarkMode.h†L86-L152】

## Implementation roadmap
1. **Bootstrap helper module**
   - Create `src/darkmode.h/.cpp` that mirrors the sample's loader: resolve ordinals, guard against unsupported builds, expose `DarkModeInitialize`, `DarkModeSetEnabled`, `DarkModeApplyWindow`, `DarkModeRefreshTitleBar`, `DarkModeHandleSettingChange`, and `DarkModeFixScrollbars`. Reuse Salamander's high-contrast detection to short-circuit dark activation.【F:src/salamdr1.cpp†L3191-L3235】【F:.3rd-party/win32-darkmode-master.zip/win32-darkmode-master/win32-darkmode/DarkMode.h†L1-L152】
   - Call the initializer once after configuration import so the helper can set `PreferredAppMode::AllowDark` before any HWNDs appear. Cache whether dark mode is supported to avoid repeated ordinal lookups.【F:src/salamdr1.cpp†L3191-L3235】

2. **Expose configuration toggles**
   - Replace the hard-coded "Windows dark mode" combo entry with an explicit checkbox (and optional "follow system" toggle) stored in `Configuration` and persisted via `SALAMANDER_COLORS_REG`. Update language resources accordingly.【F:src/dialogs4.cpp†L3335-L3376】【F:src/lang/lang.rc†L798-L826】
   - Update `ColorsChanged()` so it reads the checkbox state, calls `DarkModeSetEnabled`, and refreshes Salamander's palette only when native dark mode is active, keeping compatibility with legacy schemes.【F:src/salamdr1.cpp†L3191-L3235】

3. **Opt windows and menus into native dark mode**
   - During `CMainWindow::WM_CREATE`, `WM_THEMECHANGED`, and `WM_SETTINGCHANGE`, invoke the helper to apply dark mode to the frame, rebars, menus, and MDI children, and forward broadcasts to plugin windows. Refresh the title bar each time the immersive color policy changes.【F:src/mainwnd3.cpp†L1011-L1108】
   - Update WinLib's dialog initialisation (`CDialog::DialogProc`/`CWindow::AttachToWindow`) to call `DarkModeApplyWindow`, then send `WM_THEMECHANGED` to controls like `SysListView32` or `SysTreeView32` so they adopt Explorer's dark resources. Repaint Salamander-owned brushes with the helper-supplied colors.【F:src/common/winlib.cpp†L242-L320】
   - Extend the menu subsystem to enable dark popups by calling `AllowDarkModeForWindow` on each menu HWND, tracking them in `CMenuWindowQueue`, and invoking `_FlushMenuThemes` whenever the configuration toggles so system menus redraw with the immersive palette.【F:src/menu1.cpp†L1-L120】

4. **Refresh scrollbars and plugins**
   - Invoke `DarkModeFixScrollbars()` once per process (after COMCTL is loaded) to remap scrollbar classes to `Explorer::ScrollBar`, matching the sample's approach. Ensure panels and the internal viewer receive a final `WM_THEMECHANGED` so their native scrollbars adopt the new theme.【F:.3rd-party/win32-darkmode-master.zip/win32-darkmode-master/win32-darkmode/DarkMode.h†L112-L152】
   - Broadcast a synthetic `WM_THEMECHANGED` to plugin windows after `ColorsChanged()` finishes so third-party UI can respond immediately, preserving existing plugin contract semantics.【F:src/salamdr1.cpp†L3191-L3235】

## Suggested Codex tasks
The following JSON block can be fed to your Codex task runner; each task references concrete source files and concludes with an executable verification command.

```json
{
  "version": 1,
  "tasks": [
    {
      "id": "darkmode-helper",
      "description": "Add uxtheme-based dark mode helper with window/titlebar/app activation",
      "pathHints": ["src/darkmode.cpp", "src/darkmode.h", "src/salamdr1.cpp"],
      "steps": [
        "Port ordinal loader and helper wrappers from the win32-darkmode sample",
        "Expose DarkModeInitialize/SetEnabled/ApplyWindow/RefreshTitleBar/HandleSettingChange",
        "Call DarkModeInitialize() during startup before any top-level HWNDs are created"
      ],
      "run": ["ninja", "-C", "build", "salamander"]
    },
    {
      "id": "darkmode-config",
      "description": "Expose Windows dark mode checkbox on Colors page and persist preference",
      "pathHints": ["src/dialogs4.cpp", "src/lang/lang.rc", "src/mainwnd2.cpp"],
      "steps": [
        "Add UseWindowsDarkMode flag to Configuration defaults and registry IO",
        "Replace scheme slot with checkbox UI and wire Transfer()/Validate()",
        "Ensure ColorsChanged() calls DarkModeSetEnabled() based on the new flag"
      ],
      "run": ["ninja", "-C", "build", "salamander", "config-dialog-tests"]
    },
    {
      "id": "darkmode-window-tree",
      "description": "Apply native dark mode to main window, dialogs, menus, and scrollbars",
      "pathHints": ["src/mainwnd3.cpp", "src/common/winlib.cpp", "src/menu1.cpp"],
      "steps": [
        "Call DarkModeApplyWindow/RefreshTitleBar in WM_CREATE/WM_THEMECHANGED",
        "Hook dialog creation to enable Explorer dark resources on child controls",
        "Flush menu themes and invoke scrollbar fixer when toggling"
      ],
      "run": ["ninja", "-C", "build", "salamander", "ui-smoke-tests"]
    }
  ]
}
```
