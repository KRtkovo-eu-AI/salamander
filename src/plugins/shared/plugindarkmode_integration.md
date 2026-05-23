# Plugin dark mode integration (short guide)

Use `plugindarkmode.h` from plugin window/dialog procedures.

## Messages to handle

- `WM_THEMECHANGED`
- `WM_SETTINGCHANGE` (especially `ImmersiveColorSet` / `WindowsThemeElement`)
- `WM_CTLCOLORSTATIC`, `WM_CTLCOLORBTN`, `WM_CTLCOLOREDIT`

## Recommended flow

1. On plugin init (or when host config is known), call:
   - `PluginDarkMode_SetHostPolicyAvailable(...)`
   - `PluginDarkMode_SetHostColors(...)` (optional, preferred when host gives scheme colors)
2. On `WM_THEMECHANGED` / `WM_SETTINGCHANGE`:
   - call `PluginDarkMode_ApplyTitleBar(hwnd)`
   - call `PluginDarkMode_ApplyListTreeThemeRecursive(hwnd)`
   - invalidate/repaint if custom owner-draw controls are used
3. On `WM_CTLCOLOR*`:
   - return `PluginDarkMode_GetDialogCtlColorBrush((HDC)wParam, uMsg)`

Host configuration (`Windows Dark Mode (experimental)`) has priority.  
If host info is unavailable, the module safely falls back to system detection.
