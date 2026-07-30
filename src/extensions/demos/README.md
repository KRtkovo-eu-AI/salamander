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
