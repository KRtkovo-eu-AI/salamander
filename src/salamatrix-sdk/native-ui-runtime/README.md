# Salamatrix native UI runtime boundary

The standalone runtime will contain the host-independent dialog model, layout,
Win32 window creation, DPI behavior, dark-mode behavior, and implementations of
the Salamatrix-specific controls required for an exact Studio preview.

Host-specific behavior is injected through a narrow interface for parent
windows, message boxes, file/folder pickers, icons, and shell actions.
`Salamatrix.spl` will provide the Open Salamander adapter; the Studio preview
host will provide a standalone Windows adapter. Neither consumer owns a second
copy of the dialog behavior.

This directory now owns the public UI contract, `NativeDialog`, layout/DPI
helpers, and the Salamatrix-specific static text, hyperlink, progress, arrow,
text-arrow, color-arrow, and toolbar-header control implementations.
`Salamatrix.spl` and the standalone x64 Studio host compile these same sources;
neither consumer contains a second renderer or a control fallback.

Environment behavior is supplied through `INativeDialogHost`. Salamatrix's
adapter integrates the selected Salamander dark-mode policy and message-box
parenting. The standalone adapter uses the current Windows appearance and has
no dependency on `salamand.exe`.
