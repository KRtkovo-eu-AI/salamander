# PictView WIC support and backfill list

## Current behaviour

PictView enumerates supported extensions from three sources on every plug-in
connection:

1. The local Windows Imaging Component inventory through
   `IWICBitmapDecoderInfo::GetFileExtensions`.
2. The native PictView decoder registry (`PictView::Native::GetDecoderMasks`).
3. Explorer `IThumbnailProvider` / `IExtractImage` handlers, minus a denylist
   of documents, archives, executables, and media.

A mask is advertised only when one of those sources actually provides a
decoder.  Opening a file tries WIC by filename, then WIC by stream sniff
(needed for JPEG aliases such as `.jff` / `.thm`), then the native registry,
then an embedded preview slice (JPEG/PNG/TIFF/BMP/WMF inside EPS, PDF/AI,
MOV, HPI, Corel, Paint Shop Pro, or Zoner containers, including a ZIP-stored
thumbnail in 3MF/SKP/CDR), then Explorer
`IPreviewHandler` hosting for `.stl` (Microsoft 3D Viewer, skipped on the
thumbnail/`PVFF_FAST` path). Panel `.stl` thumbnails are an isometric native
raster (mesh colored from `SALCOL_ITEM_FG_NORMAL`) instead of that handler.
Interactive viewer opens may then ask Explorer
for a still (`IThumbnailProvider` or `IShellItemImageFactory` with
`SIIGBF_THUMBNAILONLY`). The thumbnail/icon thread never hosts
`IPreviewHandler`, never extracts `IThumbnailCache`, and never binds a shell
thumbnail handler: those block the icon reader and freeze directory changes
and plugin unload. Panel thumbnails therefore come from native or WIC
decode only. Viewer and thumbnail masks share the inventory; thumbnail
registration packs native PictView formats first so `SetThumbnailLoader`
cannot truncate them at `MAX_GROUPMASK`.

If WIC creates a decoder but cannot actually decode the first frame (typical
for DXGI/DX10 DDS), PictView continues with native, embedded, and shell
fallbacks instead of failing immediately.

## Guaranteed native formats

The following masks are always registered, independent of optional Store
codecs: TGA, PCX, DCX, PBM/PGM/PPM/PNM, RAS/SUN, SGI/BW/RGB, WBMP, SVG,
IFF/LBM, ANI, CUR, PSD (flattened 8-bit composite), FLI/FLC, DTX, JPEG
aliases JFF/JIF/THM/THUMB, EPS/EPT/AI (preview), MOV (preview), HPI
(preview), CDR/CDT/CMX/WEB/XAR (preview), PSP* (preview), ZBR/ZMF/ZNO
(preview), DDS (uncompressed, DXT1/3/5, ATI1/ATI2, DX10 BC1/BC2/BC3/BC4/BC5 and common 32-bit
UNORM; cubemaps and BC7 are not native), XCF (8-bit RGB/gray flatten),
PDN (PDN3 PNG thumbnail), 3DM (embedded preview), DWG (embedded BMP/PNG/WMF/EMF preview),
SKP (ZIP thumbnail), 3MF (ZIP `Metadata/thumbnail.png`), BLEND (embedded JPEG/PNG/BMP), WMF/EMF (GDI rasterization),
STL (isometric panel thumbnail; Explorer `IPreviewHandler` / Microsoft 3D Viewer in the viewer).

## Optional Microsoft WIC codecs

Do not advertise these as a PictView guarantee.  When the codec is
installed, WIC enumeration adds the masks automatically.

- **WebP:** [WebP Image Extension](https://apps.microsoft.com/detail/9pg2dk419drg).
- **Camera RAW / DNG:** [Raw Image Extension](https://apps.microsoft.com/detail/9nctdw2w1bh8).
- **HEIF/HEIC / AVIF:** HEIF Image Extensions / AV1 Video Extension.

`libwebp`, LibRaw, and libheif are not bundled.

## Formats still without a native decoder

The historical registration listed many more suffixes (TI calculator images,
AWD, BLP, PhotoCD, PICT, WPG, GEM IMG, MacPaint, CALS, and others).  Those
masks are not advertised until a tested decoder exists.

## Save As

Save As is still WIC-only: BMP, GIF, JPEG, PNG, and TIFF.  Native formats are
decode/view/thumbnail only until a matching encoder is added.

## Maintenance rule

Do not add a new PictView suffix by hand merely because an old table listed
it.  Either let WIC enumerate it from an installed decoder, or add a tested
native implementation and document its exact feature limits.
