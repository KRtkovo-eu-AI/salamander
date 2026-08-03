# Salamatrix native UI runtime boundary

The standalone runtime will contain the host-independent dialog model, layout,
Win32 window creation, DPI behavior, dark-mode behavior, and implementations of
the Salamatrix-specific controls required for an exact Studio preview.

Host-specific behavior is injected through a narrow interface for parent
windows, message boxes, file/folder pickers, icons, and shell actions.
`Salamatrix.spl` will provide the Open Salamander adapter; the Studio preview
host will provide a standalone Windows adapter. Neither consumer owns a second
copy of the dialog behavior.

This directory currently establishes the dependency boundary. Moving the
existing `NativeDialog` implementation behind it is the next native slice.
