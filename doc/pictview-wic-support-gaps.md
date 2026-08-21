# PictView WIC support and backfill list

## Current behaviour

PictView no longer uses the historical `PVW32DLL` decoder set.  On every
plug-in connection it enumerates the local WIC decoder inventory through
`IWICBitmapDecoderInfo::GetFileExtensions` and uses those masks for both the
viewer and thumbnail loader.

Consequences:

- A codec installed by the user or Windows (for example WebP, HEIF/HEIC, DNG
  or a camera RAW codec) is picked up automatically at the next Salamander
  start.  It is not necessary to add its extension to a static list.
- If that codec is uninstalled, the corresponding PictView association is
  removed on the next start.
- The built-in WIC baseline is BMP (`.bmp`, `.dib`, `.rle`), GIF, ICO, JPEG (`.jpg`, `.jpe`, `.jpeg`,
  `.jfif`), PNG, TIFF (`.tif`, `.tiff`), JPEG XR / HD Photo (`.jxr`, `.wdp`)
  and WIC-compatible DDS.  The runtime inventory remains the source
  of truth for the exact extensions on an individual computer.

## Legacy formats requiring an implementation or a WIC codec

The old static registration advertised 114 masks.  The masks below are not
part of the guaranteed native WIC baseline, so the current backend must not
claim them unless a locally installed WIC decoder advertises the mask.  To
restore platform-independent PictView support, implement a decoder/rasterizer
in PictView (or bundle and maintain an appropriate codec), add test files, and
then add the capability to the comparison page.

```text
2BP, 82I, 83I, 85I, 86I, 89I, 92I,
AI, ANI, ARW, AWD,
BLP, BMI, BW,
CAL, CALS, CDR, CDT, CEL, CIT, CLK, CLP, CMX, COT, CPT, CR2, CRW, CUR, CUT,
DCX, DNG, DTX,
EPS, EPT,
FLC, FLI,
GEM,
HAM, HMR, HPI, HRZ,
ICN, IFF, IMG, ITIFF,
JFF, JIF, JMX,
LBM,
MAC, MACP, MBM, MIL, MNG, MOV, MPNT, MSP,
NEF,
OFX, ORF,
PAINT, PAN, PAT, PBM, PC2, PCD, PCT, PCX, PEF, PGM, PIC, PICT, PNM, PNTG,
PPM, PSP*, PSD, PYX,
QFX,
RAF, RAS, RGB,
SAM, SCX, SEP, SGI, SKA, ST, STW, SUN,
TGA, THM, THUMB,
UDI,
WBMP, WEB, WPG,
XAR,
ZBR, ZMF, ZNO.
```

## Formats that need special treatment

| Format family | Status and required work |
| --- | --- |
| WebP | Supported automatically only when the Windows WebP WIC codec is installed. Do not advertise it as universal support. A bundled decoder would make it guaranteed. |
| HEIF/HEIC, AVIF | Not historically registered. They become available automatically if their WIC extension codec exposes a mask. Add test files before describing them as a product guarantee. |
| DNG and camera RAW | May be exposed by a WIC Raw Image Extension or vendor codec. Native decoding is required for a consistent guarantee across machines. |
| SVG | Not a WIC bitmap decoder. Restore thumbnails/viewing only through an SVG rasterizer or a dedicated thumbnail implementation. |
| DDS | The WIC decoder supports only WIC-compatible DDS variants. Test each required DXGI compression format before promising full DDS support. |
| EPS/EPT/AI, MOV | The previous implementation relied on embedded previews. Proper rendering needs a PostScript/PDF/vector rasterizer and a video-frame decoder, respectively. |

## Maintenance rule

Do not add a new PictView suffix by hand merely because an old table listed it.
Either let WIC enumerate it from an installed decoder, or add a tested native
implementation and document its exact feature limits.
