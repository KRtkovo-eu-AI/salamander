10/2025

The closed-source PVW32Cnv backend has been fully replaced by an in-tree implementation that
uses the Windows Imaging Component (WIC) APIs available on modern versions of Windows. The
legacy PVW32Cnv.lib import library is no longer required to build the plugin.