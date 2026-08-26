# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WIC = (ROOT / "src/plugins/pictview/wic/WicBackend.cpp").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/plugins/pictview/native/NativeDecoder.cpp").read_text(encoding="utf-8")
SVG = (ROOT / "src/plugins/pictview/native/NativeSvg.cpp").read_text(encoding="utf-8")
CONTAINER = (ROOT / "src/plugins/pictview/native/NativeContainer.cpp").read_text(encoding="utf-8")
INTERNAL = (ROOT / "src/plugins/pictview/native/NativeInternal.h").read_text(encoding="utf-8")
HEADER = (ROOT / "src/plugins/pictview/native/NativeDecoder.h").read_text(encoding="utf-8")
GAPS = (ROOT / "doc/pictview-wic-support-gaps.md").read_text(encoding="utf-8")
HELP = (ROOT / "src/plugins/pictview/help/hh/pictview/appendix_fileformats.htm").read_text(encoding="utf-8")


def require(source: str, snippet: str, message: str) -> None:
    if snippet not in source:
        raise AssertionError(message)


def main() -> None:
    require(WIC, "PictView::Native::GetDecoderMasks(masks);",
            "WIC mask enumeration must union native guaranteed masks")
    require(WIC, "CreateDecoderFromStream",
            "WIC open must sniff the stream when the filename decoder fails")
    require(WIC, "TryOpenNative",
            "WIC open must fall back to the native decoder registry")
    require(WIC, "TryOpenEmbeddedPreview",
            "WIC open must try an embedded preview slice after native decode fails")
    require(WIC, "TryOpenShellThumbnail",
            "WIC open must fall back to Explorer IShellItemImageFactory thumbnails")
    require(WIC, "BHID_ThumbnailHandler",
            "shell fallback must bind IThumbnailProvider, not only cached Explorer bitmaps")
    require(WIC, "e357fccd-a995-4576-b01f-234630154e96",
            "mask enumeration must look up IThumbnailProvider")
    require(WIC, '"pdf"',
            "shell mask enumeration must deny document types such as PDF")
    require(WIC, '"ai", "eps", "ept"',
            "shell thumbnail fallback must deny Adobe AI/EPS handlers that can hang the UI thread")
    require(WIC, "IsDeniedShellExtension(AsciiExtensionFromPath(handle.fileName))",
            "TryOpenShellThumbnail must skip denylisted extensions before binding a handler")

    if "libwebp" in WIC.lower() or "libraw" in WIC.lower():
        raise AssertionError("WebP and RAW must stay on Microsoft WIC codecs")
    if "libwebp" in NATIVE.lower() or "libraw" in NATIVE.lower():
        raise AssertionError("native decoders must not bundle libwebp or LibRaw")

    required_masks = [
        '"*.tga"', '"*.pcx"', '"*.pnm"', '"*.svg"', '"*.psd"',
        '"*.iff"', '"*.ani"', '"*.eps"', '"*.mov"', '"*.hpi"',
        '"*.dds"', '"*.xcf"', '"*.pdn"', '"*.3dm"', '"*.ai"',
        '"*.dwg"', '"*.wmf"', '"*.emf"',
    ]
    for mask in required_masks:
        require(NATIVE, mask, f"native mask list must include {mask}")

    require(HEADER, "FindEmbeddedPreview",
            "preview extraction must expose a WIC-decodable slice")
    require(GAPS, "WebP Image Extension",
            "support gaps must document the official Microsoft WebP codec")
    require(GAPS, "Raw Image Extension",
            "support gaps must document the official Microsoft RAW codec")
    require(HELP, "Native PictView decoders",
            "help must list native guaranteed formats separately from optional WIC codecs")
    require(HELP, "XCF",
            "help must document GIMP XCF support")
    require(HELP, "PDN",
            "help must document Paint.NET preview support")
    require(HELP, "3DM",
            "help must document Rhinoceros preview support")
    require(HELP, "IThumbnailProvider",
            "help must mention Explorer thumbnail handlers")
    require(GAPS, "DX10",
            "support gaps must document native DX10 DDS decoding")
    require(WIC, "PathPrefersNativeDecoder",
            "WIC open must prefer native decode for DDS/DWG/WMF/AI")
    require(WIC, "return PVC_UNSUP_FILE_TYPE",
            "unmapped WIC/shell HRESULT must not surface as Unknown WIC error")
    require(NATIVE, "return preview.size >= 8;",
            "PDF/AI preview extraction must not fall through into a full-file binary scan")
    require(NATIVE, "if (Detail::LooksLikePsOrPdf(data, size))",
            "DecodeMemory must send PDF/PS/AI files only to the preview decoder")
    require(SVG, "reader.Size() > 2ull * 1024ull * 1024ull",
            "SVG sniff must refuse oversized buffers before NanoSVG parse")
    require(SVG, "a == 's' && b == 'v' && c == 'g'",
            "SVG sniff must require an <svg tag, not a lone < from PDF dictionaries")
    require(HELP, "Windows Metafile",
            "help must document WMF/EMF rasterization")
    require(HELP, "BC4/BC5",
            "help must document native BC4/BC5 DDS decode")
    require(HELP, "FlateDecode",
            "help must document PDF/AI FlateDecode image extraction")
    require(CONTAINER, "Decode3dmCompressedPreview",
            "3DM must decode OpenNURBS compressed preview bitmaps, not raw inflate of the chunk")
    require(CONTAINER, "FrameFromRasterPreviewBytes",
            "3DM whole-file fallback must not treat WMF/TIFF noise as the model preview")
    require(NATIVE, "rhino3dm",
            "embedded preview scan must ignore coincidental WMF/TIFF inside Rhinoceros 3DM files")
    require(INTERNAL, "ForceUnusedAlphaOpaque",
            "packed DIBs with unused zero alpha must stay opaque for display")
    require(CONTAINER, ">= 50",
            "Rhino 5+ 3DM archives use 8-byte chunk values")
    require(CONTAINER, "0x40008000",
            "OpenNURBS zlib preview is wrapped in TCODE_ANONYMOUS_CHUNK")
    require(WIC, "!anyOpaqueAlpha",
            "WIC/shell 32-bpp frames with an unused alpha plane must stay visible")

    print("pictview_native_decoder_contract_tests: ok")


if __name__ == "__main__":
    main()
