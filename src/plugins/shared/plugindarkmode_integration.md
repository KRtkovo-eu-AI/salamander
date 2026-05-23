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
   - first call `PluginDarkMode_HandleThemeMessage(hwnd, uMsg, lParam)`
   - then update plugin-specific owner-draw state (if any)
   - finally invalidate/repaint as needed
3. On `WM_CTLCOLOR*`:
   - call `PluginDarkMode_HandleCtlColor(uMsg, wParam, lParam, &brush)` first
   - if it returns `TRUE`, return `brush` immediately
   - only if it returns `FALSE`, continue with plugin-local/default handling

This keeps the call order consistent with host dialogs and avoids one handler rewriting
DC colors after another already selected a brush.

## Explicit plugin test case (radio + mask-hints link)

Add a small test dialog page containing:

- three radio buttons (`BS_AUTORADIOBUTTON`)
- one static label with `SS_NOTIFY` style used as a "mask hints" link
- (optionally) one `SysLink` control

Minimal procedure pattern:

```cpp
case WM_THEMECHANGED:
case WM_SETTINGCHANGE:
    PluginDarkMode_HandleThemeMessage(HWindow, uMsg, lParam);
    return 0;

case WM_CTLCOLORSTATIC:
case WM_CTLCOLORBTN:
case WM_CTLCOLOREDIT:
case WM_CTLCOLORLISTBOX:
case WM_CTLCOLORDLG:
case WM_CTLCOLORMSGBOX:
{
    LRESULT brush = 0;
    if (PluginDarkMode_HandleCtlColor(uMsg, wParam, lParam, &brush))
        return brush;
    break; // plugin/default path
}
```

Expected behavior in dark mode:

- radio labels remain readable (light text) and radios use fallback theme path
- SS_NOTIFY / SysLink "mask hints" style text uses a dark-safe light-blue link color
- no double-processing flicker when switching theme (single brush decision path)

Host configuration (`Windows Dark Mode (experimental)`) has priority.  
If host info is unavailable, the module safely falls back to system detection.
