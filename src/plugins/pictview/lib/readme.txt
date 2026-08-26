10/2025

The closed-source PVW32Cnv backend has been fully replaced by an in-tree implementation that
uses the Windows Imaging Component (WIC) APIs available on modern versions of Windows. The
legacy PVW32Cnv.lib import library is no longer required to build the plugin.

Native decoders in pictview/native cover formats that WIC does not guarantee (TGA, PCX, Netpbm,
SGI/RAS, IFF, PSD composite, SVG, Autodesk FLI/FLC, DDS/DX10 including BC4/BC5,
XCF, PDN thumbnails, 3DM/DWG/SKP/BLEND previews, WMF/EMF, and embedded EPS/AI/PDF previews). WebP, camera RAW,
and HEIF/AVIF remain optional Microsoft Store WIC codecs.