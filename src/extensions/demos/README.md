# Salamatrix extension package demos

These five directories are complete, standalone extension packages. Each one
contains an `extension.json`, a runtime-specific entry point, and package-owned
SVG artwork. They are deliberately outside `plugins/automation`: Automation
provides only the legacy script bridge, while package metadata and modern
runtime selection are owned by the Salamatrix Framework and surfaced by the
native Plugin Manager.

Install a demo by copying its directory below one of the configured
Salamatrix extension roots, then refresh the extension catalog. The package is
shown as a registered extension row, its icon is taken from `icon.svg`, and its
command contributes to the Plugin menu, panel context menu, and toolbar.

All manifests use the canonical public `schema` field and canonical capability
names. The framework also accepts `schemaVersion` as a compatibility alias. All
five demos use schema 2 and exercise both native v1 roles:

| Runtime | Viewer sample and mask | `salamatrix:` provider |
| --- | --- | --- |
| Node.js | `javascript-node/sample.smxview` (`*.smxview`) | `Salamatrix.Demo.JavaScriptNode!demo-machines` |
| Python | `python/sample.smxpyview` (`*.smxpyview`) | `Salamatrix.Demo.Python!demo-machines` |
| PowerShell | `powershell/sample.smxpsview` (`*.smxpsview`) | `Salamatrix.Demo.PowerShell!demo-machines` |
| PHP | `php/sample.smxphpview` (`*.smxphpview`) | `Salamatrix.Demo.PHP!demo-machines` |
| Lua | `lua/sample.smxluaview` (`*.smxluaview`) | `Salamatrix.Demo.Lua!demo-machines` |

Restart Salamander after installing or changing a Viewer package so the native
association list can be rebuilt. Configuration > Viewers shows the package and
the optional Viewer `name`, rather than only `Salamatrix Framework (plugin)`.
Each sample handler reads UTF-8 text and displays it through the runtime-neutral
UI facade.

Change a panel to `salamatrix:` and open any provider listed above. Each flat
provider publishes SVG-icon items, a default Inspect action, a Toggle state
action backed by extension storage, and a five-second refresh interval. The
provider level contains `..`, Directory Line exposes clickable root/provider
segments, and Salamander's full-path, shortened-path, and directory-only title
modes are honored.
