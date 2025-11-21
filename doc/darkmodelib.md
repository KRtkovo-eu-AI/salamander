# Dark mode library adapter

The optional `darkmodelib.dll` runtime can extend Salamander's existing dark-mode
pipeline. The adapter is loaded on-demand; builds continue to work even when the
library is absent.

## Responsibilities mapped

* **Palette building:** `DarkModeQueryPreferredPalette` asks the library for
  dialog text, background, and accent colors. When present, the palette feeds
  the Windows dark scheme created in `BuildWindowsDarkPalette` so list views and
  dialogs inherit accent-aware colors.
* **Window registration:** `DarkModeSetEnabled`, `DarkModeApplyWindow`, and
  `DarkModeApplyTree` prefer `darkmodelib` APIs to opt windows into immersive
  title bars and automatic control theming. The in-tree helpers remain as
  fallbacks.
* **Title bar refresh:** `DarkModeRefreshTitleBar` delegates to the library
  first so non-client rendering benefits from any additional heuristics.

## Fallbacks and toggles

* Set the environment variable `SALAMANDER_DISABLE_DARKMODELIB=1` to skip the
  library entirely and use the built-in helpers.
* When the DLL is missing or incomplete, Salamander reverts to the existing
  native dark-mode shim without functional regressions.

## Runtime expectation

If you want the extended behaviors (automatic control theming, accent-aware
palettes), place `darkmodelib.dll` next to `salamander.exe` (or anywhere in the
standard DLL search path). No build-time dependency is required.
