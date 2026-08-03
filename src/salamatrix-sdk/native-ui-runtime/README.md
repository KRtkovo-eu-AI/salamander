# Salamatrix native UI runtime boundary

The standalone runtime will contain the host-independent dialog model, layout,
Win32 window creation, DPI behavior, dark-mode behavior, and implementations of
the Salamatrix-specific controls required for an exact Studio preview.

Host-specific behavior is injected through a narrow interface for parent
windows, message boxes, file/folder pickers, icons, and shell actions.
`Salamatrix.spl` will provide the Open Salamander adapter; the Studio preview
host will provide a standalone Windows adapter. Neither consumer owns a second
copy of the dialog behavior.

The shared layout helpers in this directory are now consumed by both
`Salamatrix.spl` and the standalone x64 Studio host. The host can render and
validate complete dialog documents without `salamand.exe`.

The current host uses standard Win32 implementations as fallbacks for the
Salamander-owned static text, arrow button, color button, progress, and toolbar
header controls. Moving those implementations, dark-mode handling, and the
remaining `NativeDialog` code behind this boundary is still required before
all preview controls are pixel-identical to the in-process Salamatrix UI.
