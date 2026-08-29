# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WIC = (ROOT / "src/plugins/pictview/wic/WicBackend.cpp").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/plugins/pictview/native/NativeDecoder.cpp").read_text(encoding="utf-8")
SVG = (ROOT / "src/plugins/pictview/native/NativeSvg.cpp").read_text(encoding="utf-8")
RAST = (ROOT / "src/common/dep/nanosvg/nanosvgrast.h").read_text(encoding="utf-8")
CONTAINER = (ROOT / "src/plugins/pictview/native/NativeContainer.cpp").read_text(encoding="utf-8")
PREVIEW = (ROOT / "src/plugins/pictview/native/NativePreview.cpp").read_text(encoding="utf-8")
STL = (ROOT / "src/plugins/pictview/native/NativeStl.cpp").read_text(encoding="utf-8")
INTERNAL = (ROOT / "src/plugins/pictview/native/NativeInternal.h").read_text(encoding="utf-8")
HEADER = (ROOT / "src/plugins/pictview/native/NativeDecoder.h").read_text(encoding="utf-8")
SHELL = (ROOT / "src/plugins/pictview/wic/ShellPreviewHost.cpp").read_text(encoding="utf-8")
THUMBS = (ROOT / "src/plugins/pictview/thumbs.cpp").read_text(encoding="utf-8")
RENDER1 = (ROOT / "src/plugins/pictview/render1.cpp").read_text(encoding="utf-8")
PLUGIN = (ROOT / "src/plugins/pictview/pictview.cpp").read_text(encoding="utf-8")
SDK = (ROOT / "src/plugins/shared/spl_gen.h").read_text(encoding="utf-8")
GAPS = (ROOT / "doc/pictview-wic-support-gaps.md").read_text(encoding="utf-8")
HELP = (ROOT / "src/plugins/pictview/help/hh/pictview/appendix_fileformats.htm").read_text(encoding="utf-8")
RC2 = (ROOT / "src/plugins/pictview/pictview.rc2").read_text(encoding="utf-8")


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
    require(WIC, "PVFF_FAST",
            "shell thumbnail extraction must not run on the icon/thumbnail thread")
    require(WIC, "BHID_ThumbnailHandler",
            "shell fallback must bind IThumbnailProvider, not only cached Explorer bitmaps")
    require(WIC, "e357fccd-a995-4576-b01f-234630154e96",
            "mask enumeration must look up IThumbnailProvider")
    require(WIC, '"pdf"',
            "shell mask enumeration must deny document types such as PDF")
    require(WIC, '"ai", "eps", "ept"',
            "shell thumbnail fallback must deny Adobe AI/EPS handlers that can hang the UI thread")
    require(WIC, "IsDeniedShellExtension(extension)",
            "TryOpenShellThumbnail must skip denylisted extensions before binding a handler")

    if "libwebp" in WIC.lower() or "libraw" in WIC.lower():
        raise AssertionError("WebP and RAW must stay on Microsoft WIC codecs")
    if "libwebp" in NATIVE.lower() or "libraw" in NATIVE.lower():
        raise AssertionError("native decoders must not bundle libwebp or LibRaw")

    required_masks = [
        '"*.tga"', '"*.pcx"', '"*.pnm"', '"*.svg"', '"*.psd"',
        '"*.iff"', '"*.ani"', '"*.eps"', '"*.mov"', '"*.hpi"',
        '"*.dds"', '"*.xcf"', '"*.pdn"', '"*.3dm"', '"*.ai"',
        '"*.dwg"', '"*.wmf"', '"*.emf"', '"*.stl"', '"*.3mf"',
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
    require(WIC, "const bool isStl = IsStlExtension(image->fileName);",
            "STL opening must be identified before any WIC decoder attempt")
    require(WIC, "if (isStl)",
            "STL opening must try the external IPreviewHandler first")
    require(WIC, 'RecordDetailedError(failureCode, failureHr, "STL IPreviewHandler")',
            "STL handler failures must not be reported as a generic WIC error")
    require(WIC, 'RecordDetailedError(failureCode, failureHr, "Native decoder")',
            "native decoder failures must retain their actual diagnostic stage")
    require(WIC, "return failureCode;",
            "native decoder failures must not fall through to WIC or shell diagnostics")
    require(WIC, "GetWicDecoderMasks(wicMasks)",
            "opening must query the actual WIC codec inventory separately from native and shell masks")
    require(WIC, "PictView::Native::GetDecoderMasks(nativeMasks)",
            "opening must identify native implementations before considering WIC")
    require(WIC, "wicCodecAvailable && !nativeDecoderRegistered",
            "WIC must not be attempted for formats handled by a native decoder")
    require(WIC, "starPos = std::string::npos",
            "native extension matching must honor wildcard masks such as *.psp*")
    require(PLUGIN, "GetWicDecoderMasks(currentMasks)",
            "PictView viewer associations must be synchronized from WIC codecs only")
    require(PLUGIN, "currentMasks = SplitViewerMasks(FallbackWicDecoderMasks)",
            "WIC enumeration failure must not restore legacy native or shell masks as WIC associations")
    if "GetDecoderMasks(currentMasks)" in PLUGIN:
        raise AssertionError("PictView WIC associations must not use the native/shell union")
    require(WIC, "if (!opened && wicCodecAvailable && !nativeDecoderRegistered)",
            "WIC decoding must be skipped when no WIC codec is registered for the extension")
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
    require(RAST, "b[0] = b[2];",
            "NanoSVG rasterizer must emit BGRA for GDI, not a second swap in DecodeSvg")
    if "std::swap(pixel[0], pixel[2])" in SVG:
        raise AssertionError("DecodeSvg must not swap R/B after nsvgRasterize already did")
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
    require(SHELL, "IPreviewHandler",
            "STL viewing must host Explorer IPreviewHandler, not a bundled mesh renderer")
    require(PLUGIN, "OleInitialize(nullptr)",
            "the viewer UI thread must keep a persistent STA for preview-handler lifetime")
    require(PLUGIN, "OleUninitialize()",
            "the viewer STA must be released only after its window and preview handles")
    require(SHELL, "CoGetApartmentType",
            "interactive preview activation must verify the persistent viewer STA")
    require(SHELL, "APTTYPE_STA",
            "interactive preview handlers must never activate on an MTA thread")
    require(SHELL, "4834AC27-23F1-420A-888D-85DC70B903C5",
            "STL preview must use the Microsoft 3D Viewer DesktopPreviewHandler CLSID")
    require(SHELL, "stale or broken .stl association must not suppress",
            "STL preview must retry Microsoft 3D Viewer when the registered handler fails")
    require(SHELL, "SystemFileAssociations",
            "STL preview must search the standard SystemFileAssociations fallback")
    require(SHELL, "ReadDefaultRegistryString",
            "STL preview must read direct HKCR preview-handler registrations")
    require(SHELL, "shellex",
            "STL preview must inspect the registered preview-handler shell extension key")
    require(SHELL, "CLSCTX_LOCAL_SERVER",
            "preview handlers must activate out of process through the PreviewHost surrogate")
    if "CLSCTX_ALL" in SHELL or "CLSCTX_INPROC_SERVER" in SHELL:
        raise AssertionError("preview handlers must never be activated in the PictView process")
    require(SHELL, "PictView STL preview candidate",
            "STL handler selection must expose the selected association for diagnosis")
    # The current Windows SDK does not expose ASSOCSTR_PROGID here; handler
    # lookup intentionally covers the extension and SystemFileAssociations scopes.

    require(SHELL, "IsStlExtension",
            "interactive IPreviewHandler hosting is STL-only; do not attach Office/PDF previewers")
    require(SHELL, "PVFF_FAST",
            "interactive 3D preview must not run on the thumbnail/fast-open path")
    require(SHELL, "PreviewHostProtocol.h",
            "elevated STL preview must reuse the shared versioned helper protocol")
    require(SHELL, 'directory + L"\\\\pictview-previewhost.exe"',
            "helper executable must be resolved directly beside pictview.spl")
    if 'L"\\\\pictview\\\\pictview-previewhost.exe"' in SHELL:
        raise AssertionError("helper path must not duplicate the installed pictview directory")
    require(SHELL, "CLSID_ShellWindows",
            "elevated preview helper must launch through Explorer automation")
    require(SHELL, "IShellDispatch2",
            "helper launch must use the medium-integrity Explorer shell")
    require(SHELL, "CreateNamedPipeW",
            "helper transport must use a parent-owned named pipe")
    require(SHELL, "ConvertStringSecurityDescriptorToSecurityDescriptorW",
            "helper pipe must have an explicit user/system/admin DACL and mandatory label")
    require(SHELL, "GetNamedPipeClientProcessId",
            "helper connection must validate the client process")
    require(SHELL, "ProcessIdToSessionId",
            "helper connection must remain in the parent logon session")
    require(SHELL, "kHelperConnectTimeoutMs",
            "missing helper must not block PictView indefinitely")
    require(SHELL, "GetOverlappedResult(pipe, &operation, &ignored, TRUE)",
            "timed-out pipe transfers must drain cancellation before releasing OVERLAPPED storage")
    require(SHELL, "GetOverlappedResult(pipe, &connect, &ignored, TRUE)",
            "timed-out pipe connection must drain cancellation before closing its event or pipe")
    require(SHELL, "ERROR_OPERATION_ABORTED",
            "drained cancellation must accept the expected aborted completion")
    require(SHELL, "Command::Attach",
            "helper preview must implement protocol attach")
    require(SHELL, "Command::Resize",
            "helper preview must implement protocol resize")
    require(SHELL, "Command::Focus",
            "helper preview must implement protocol focus")
    require(SHELL, "Command::Close",
            "helper session release must send protocol close")
    require(SHELL, "MapWindowPoints",
            "helper popup geometry must be converted to screen coordinates")
    if "TryOpenPreviewThumbnail" in WIC or "PumpPreviewMessages" in SHELL:
        raise AssertionError("thumbnail path must not host IPreviewHandler or extract Explorer cache")
    if "CLSID_LocalThumbnailCache" in WIC or "WTS_EXTRACT" in WIC:
        raise AssertionError("IThumbnailCache extraction deadlocks the icon thread")
    require(PLUGIN, "JoinThumbnailMasks",
            "thumbnail loader masks must pack native PictView formats before MAX_GROUPMASK truncation")
    require(PLUGIN, "one extension per persisted viewer item",
            "PictView viewer registrations must not pack multiple extensions into truncatable rows")
    require(PLUGIN, "AddViewer(mask.c_str(), force)",
            "PictView viewer registration must add each mask independently")
    require((ROOT / "src/plugins1.cpp").read_text(encoding="utf-8"), "vector<char> ext2(MAX_GROUPMASK)",
            "viewer migration must inspect full persisted mask rows when removing legacy entries")
    require(PLUGIN, "repairMasks = HistoricPictViewViewerMasks()",
            "older PictView configurations must remove all known masks before rebuilding split viewer rows")
    require(PLUGIN, "repairMasks.insert(repairMasks.end(), previousMasks.begin(), previousMasks.end())",
            "legacy PictView repair must include masks persisted by the old WIC inventory")
    require(PLUGIN, "repairMasks.insert(repairMasks.end(), nativeMasks.begin(), nativeMasks.end())",
            "legacy PictView repair must include native masks, not only WIC masks")
    require(SDK, "#define MAX_GROUPMASK 8192",
            "group masks must be large enough for native+WIC+shell thumbnail extensions")
    require(HELP, "IPreviewHandler",
            "help must document Explorer IPreviewHandler hosting for STL")
    require(GAPS, "IPreviewHandler",
            "support gaps must document Explorer IPreviewHandler hosting for STL")
    require(THUMBS, "LoadSTLThumbnail",
            "panel STL thumbnails must use LoadSTLThumbnail, not DecodeMemory")
    require(THUMBS, "ProcessBuffer returns FALSE when the image is complete",
            "STL thumbs must not treat ProcessBuffer completion as failure")
    require(CONTAINER, ".bmp",
            "ZIP container previews must include BMP thumbnails used by Corel CDR")
    require(CONTAINER, "DecodeRiffPreview",
            "legacy RIFF CDR must decode an embedded raster preview")
    require(NATIVE, "DecodeRiffPreview",
            "DecodeMemory must try the RIFF CDR preview decoder")
    require(HEADER, "RasterizeStlMemory",
            "STL rasterizer must be a thumbnail helper, not a DecodeMemory decoder")
    if "Detail::DecodeStl" in NATIVE:
        raise AssertionError("DecodeMemory must not steal STL from IPreviewHandler")
    require(STL, "RotateIsometric",
            "STL panel thumbnails must use an isometric camera")
    require(STL, "yUp{v.x, v.z, -v.y}",
            "STL thumbs must map printer Z-up onto the isometric Y-up camera")
    require(STL, "GetRValue(albedo)",
            "STL mesh albedo must come from the thumbnail color argument")
    require(THUMBS, "RGB(0xFF, 0xC9, 0x24)",
            "STL thumbs must shade with #FFC924")
    require(THUMBS, "G.rgbPanelBackground",
            "STL thumbs must flatten onto SALCOL_ITEM_BK_NORMAL")
    require(SHELL, "TranslateAccelerator",
            "STL IPreviewHandlerFrame must translate host accelerators including Escape")
    require(SHELL, "VK_SPACE",
            "STL preview must advertise Space as next-file so IPreviewHandler forwards it")
    require(SHELL, "CMD_FILE_NEXT",
            "STL preview must map Space to next file")
    require(SHELL, "CMD_FILE_PREV",
            "STL preview must keep Backspace as previous file")
    require(SHELL, "kPreviewHostAccels",
            "STL preview must use a host accel table, not steal 3D-viewer zoom/pan keys")
    require(SHELL, "VK_ESCAPE",
            "STL preview must close the PictView window on Escape")
    require(RC2, "VK_ESCAPE, CMD_CLOSE",
            "PictView accelerator table must map Escape to close")
    require(PLUGIN, "SALCOL_ITEM_FG_NORMAL",
            "InitGlobalGUIParameters must cache the panel item foreground")
    require(CONTAINER, "ExtractZipPreviewBytes",
            "ZIP containers must share one PNG/JPEG thumbnail extractor")
    require(CONTAINER, "metadata/thumbnail.png",
            "3MF Metadata/thumbnail.png must be treated as a preview")
    require(WIC, 'L"3mf"',
            "PathPrefersNativeDecoder must include 3mf")
    require(WIC, "ExtractZipEmbeddedPreview",
            "WIC embedded preview must inflate ZIP thumbnails for JPEG 3MF/CDR/SKP")
    require(PREVIEW, "RasterizeMetafile",
            "DOS EPS must rasterize the header WMF preview")
    require(PREVIEW, "headerPsOffset",
            "DOS EPS %%BeginPreview must search the PostScript body, not only file offset 0")
    require(NATIVE, "data[20]",
            "FindEmbeddedPreview must read the DOS EPS TIFF offset at byte 20")
    require(HELP, "isometric",
            "help must document isometric STL panel thumbnails")
    require(HELP, "Metadata/thumbnail.png",
            "help must document 3MF ZIP thumbnails")
    require(HELP, "DOS binary header",
            "help must document DOS EPS TIFF/WMF previews")
    require(GAPS, "3MF",
            "support gaps must list 3MF ZIP thumbnails")
    require(GAPS, "isometric native",
            "support gaps must document isometric STL panel thumbnails")
    require(RENDER1, "case 3:",
            "viewer AutoRotate must treat info.Orient as EXIF 1-8, not PVFF bitflags")
    require(RENDER1, "cmd = CMD_MIRROR_VERT;",
            "viewer AutoRotate must map EXIF orientation 4 to a vertical flip")
    require(RENDER1, "cmd2 = CMD_MIRROR_HOR;",
            "viewer AutoRotate must compose EXIF orientation 5 as rotate then horizontal flip")
    if "case PVFF_BOTTOMTOTOP:" in RENDER1:
        raise AssertionError(
            "viewer AutoRotate must not use PVFF_BOTTOMTOTOP as an Orient case "
            "(the flag value is 1, which is EXIF orientation-normal)")

    print("pictview_native_decoder_contract_tests: ok")


if __name__ == "__main__":
    main()
