# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

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
    registration = (ROOT / "src/plugins/mmviewer/mmviewer.cpp").read_text(encoding="utf-8")
    props = (ROOT / "src/plugins/mmviewer/vcxproj/mmviewer.props").read_text(encoding="utf-8")

    for extension in (".ogg", ".oga", ".opus", ".flac", ".m4a", ".aac",
                      ".aiff", ".ape", ".mpc", ".wv", ".tta"):
        require(parser, f'"{extension}"', f"TagLib dispatch for {extension}")
        require(registration, f"*{extension}", f"viewer registration for {extension}")

    require(parser_impl, "value.to8Bit(true)", "explicit UTF-8 TagLib output")
    require(parser_impl, "ReadOggTextProperties(FileName.c_str(), legacyProperties)",
            "legacy comment fallback in Multimedia Viewer")
    require(parser_impl, "!ContainsKey(emittedKeys, it->first)",
            "deduplication of TagLib and fallback metadata")
    require(parser_impl, "MB_ERR_INVALID_CHARS", "UTF-8-first path decoding")
    require(parser_impl, "codePage = CP_ACP", "legacy path fallback")
    require(parser_impl, 'return L"\\\\\\\\?\\\\UNC\\\\"', "extended UNC path support")
    require(parser_impl, 'return L"\\\\\\\\?\\\\"', "extended local path support")
    require(props, "TAGLIB_STATIC", "static TagLib linking")
    require(props, "vcpkg_installed_mmviewer_taglib", "isolated TagLib install root")

    print("Multimedia Viewer audio metadata source-contract tests passed.")


if __name__ == "__main__":
    main()
