# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-04 | Require the shell header for the property-store call. | propsys.h does not declare it for the Windows 7 plugin target. | https://github.com/KRtkovo-eu-AI/salamander/issues/732 | Grok 4.7 |
# | 2026-11-04 | Cover video container registration and Media Foundation metadata. | The viewer contract now includes video streams as well as audio. | https://github.com/KRtkovo-eu-AI/salamander/issues/732 | Grok 4.7 |

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    files_window = (ROOT / "src/fileswn9.cpp").read_text(encoding="utf-8")
    shell_property = files_window[
        files_window.index("static BOOL AppendShellPropertyLine"):
        files_window.index("static BOOL AppendCategoryProperties")
    ]
    require(shell_property, "WideTextToUtf8(display, valueText)",
            "UTF-16 to UTF-8 conversion for shell property values")
    require(shell_property, "WideTextToUtf8(name, nameText)",
            "UTF-16 to UTF-8 conversion for shell property names")
    if "CP_ACP" in shell_property:
        raise AssertionError("panel shell properties must not round-trip through ACP")
    require(files_window, "CopyStringTruncateUtf8(text + len, textSize - len, line)",
            "UTF-8-safe tooltip truncation")
    require(shell_property, "WideTextContainsReplacementCharacter(display)",
            "rejection of already-corrupted shell metadata")
    require(files_window, "AppendLegacyOggProperties", "legacy OGG tooltip fallback")

    legacy_decoder = (ROOT / "src/audio_metadata_legacy.h").read_text(encoding="utf-8")
    require(legacy_decoder, "MB_ERR_INVALID_CHARS", "strict UTF-8 validation of Vorbis comments")
    require(legacy_decoder, "MultiByteToWideChar(1250", "CP1250 fallback for malformed legacy comments")
    require(legacy_decoder, 'memcmp(&packet[1], "vorbis", 6)', "Vorbis comment packet parser")
    require(legacy_decoder, 'memcmp(&packet[0], "OpusTags", 8)', "Opus comment packet parser")

    parser = (ROOT / "src/plugins/mmviewer/parser.cpp").read_text(encoding="utf-8")
    parser_impl = (ROOT / "src/plugins/mmviewer/taglibparser.cpp").read_text(encoding="utf-8")
    media_info = (ROOT / "src/plugins/mmviewer/mfinfo.cpp").read_text(encoding="utf-8")
    registration = (ROOT / "src/plugins/mmviewer/mmviewer.cpp").read_text(encoding="utf-8")
    props = (ROOT / "src/plugins/mmviewer/vcxproj/mmviewer.props").read_text(encoding="utf-8")
    english = (ROOT / "src/plugins/mmviewer/lang/lang.rc2").read_text(encoding="utf-8")

    for extension in (".ogg", ".oga", ".opus", ".flac", ".m4a", ".aac",
                      ".aiff", ".ape", ".mpc", ".wv", ".tta"):
        require(parser, f'"{extension}"', f"TagLib dispatch for {extension}")
        require(registration, f"*{extension}", f"viewer registration for {extension}")

    for extension in (".mkv", ".m4v", ".mov", ".qt", ".3gp", ".3g2", ".asf",
                      ".avi", ".divx", ".wmv", ".mpg", ".mpeg", ".m2ts", ".mts"):
        require(parser, f'"{extension}"', f"video dispatch for {extension}")
        require(registration, f"*{extension}", f"viewer registration for {extension}")
    if '" .ts"' in parser or '".ts"' in parser:
        raise AssertionError("the .ts extension collides with TypeScript and stays unregistered")

    require(parser, "CreateCheckedParser<CParserMF>()", "checked allocation of the video parser")
    require(registration, "CURRENT_CONFIG_VERSION 3", "viewer config version for video containers")
    require(registration, "ConfigVersion < 3", "upgrade registration for existing installations")

    require(parser_impl, "value.to8Bit(true)", "explicit UTF-8 TagLib output")
    require(parser_impl, "ReadOggTextProperties(FileName.c_str(), legacyProperties)",
            "legacy comment fallback in Multimedia Viewer")
    require(parser_impl, "!ContainsKey(emittedKeys, it->first)",
            "deduplication of TagLib and fallback metadata")
    require(parser_impl, "MmViewerPathToWide", "shared UTF-8-first path decoding")
    require(parser_impl, "MmViewerMakeExtendedPath", "shared extended path prefix")
    require(parser_impl, "MmViewerMayContainVideo", "Media Foundation is limited to containers that can hold video")
    require(parser_impl, "ReadMediaFoundationInfo", "video stream lookup for TagLib files")
    require(parser_impl, "AppendMfVideoSection", "video section beside TagLib audio")
    require(parser_impl, "if (audio != NULL)", "audio header only when audio properties exist")

    require(media_info, "MB_ERR_INVALID_CHARS", "UTF-8-first path decoding")
    require(media_info, "codePage = CP_ACP", "legacy path fallback")
    require(media_info, 'return L"\\\\\\\\?\\\\UNC\\\\"', "extended UNC path support")
    require(media_info, 'return L"\\\\\\\\?\\\\"', "extended local path support")
    require(media_info, 'strstr(pattern, "%u")', "indexed labels do not use a translated format string")
    require(media_info, "#include <shobjidl.h>", "shell property-store declaration")
    require(media_info, "GetNativeMediaType", "native media type without decoding")
    require(media_info, "MFCreateSourceReaderFromURL", "wide-path media source open")
    require(media_info, "MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, FALSE",
            "video processing stays disabled")
    if "ReadSample" in media_info or "SetCurrentMediaType" in media_info:
        raise AssertionError("media metadata must not select or pull decoded samples")
    if "[MAX_PATH]" in media_info:
        raise AssertionError("media metadata must not allocate a MAX_PATH path buffer")

    require(english, "IDS_VIDEO_CODEC", "localized video codec label")
    require(english, "IDS_VIDEO_RESOLUTION", "localized resolution label")
    require(english, "IDS_MEDIA_UNKNOWN", "localized unknown codec label")
    for language in ("czech", "slovak", "german", "french", "italian", "spanish",
                     "dutch", "hungarian", "romanian", "russian", "chinesesimplified"):
        translated = (ROOT / "translations" / language / "mmviewer.slt").read_text(encoding="utf-8-sig")
        require(translated, "12624,1,", f"{language} video information label")
        require(translated, "12642,1,", f"{language} unknown codec label")
        require(translated, "*.mkv;", f"{language} open-dialog video extensions")

    require(props, "TAGLIB_STATIC", "static TagLib linking")
    require(props, "vcpkg_installed_mmviewer_taglib", "isolated TagLib install root")
    for library in ("mfplat.lib", "mfreadwrite.lib", "mfuuid.lib", "propsys.lib"):
        require(props, library, f"Media Foundation link dependency {library}")

    print("Multimedia Viewer audio metadata source-contract tests passed.")


if __name__ == "__main__":
    main()
