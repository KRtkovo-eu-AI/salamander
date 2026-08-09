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
names. The framework also accepts `schemaVersion` as a compatibility alias. The
Node demo uses schema 2 to exercise the two native v1 roles:

- Open `javascript-node/sample.smxview` after installing the package and
  restarting Salamander. The registered `*.smxview` Viewer handler reads the
  file and displays its contents through the runtime-neutral UI facade.
- Change a panel to `salamatrix:`, enter
  `Salamatrix.Demo.JavaScriptNode!demo-machines`, and use Enter or the context
  menu on a demo machine. The flat provider publishes SVG-icon items, a default
  Inspect action, a Toggle state action backed by extension storage, and a
  five-second refresh interval.

The remaining four packages keep focused examples for Python, PowerShell, PHP,
and Lua so the same command, progress, dialog, and storage surfaces remain easy
to compare across all five runtimes.
