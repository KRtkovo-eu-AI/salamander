# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-06 | Bootstrap stub languages from english.slg PE resources. | Empty translation folders need English state=0 archives before manual/AI translation. | | Composer |
#!/usr/bin/env python3
"""Overlay English texts from english.slg onto a reference .slt structure.

Creates a new-language seed archive: same layout/IDs as the reference SLT,
English strings from the matching english.slg, LANGID for the target language,
and state=0 for every non-empty translatable string.
"""
from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path

import pefile

LINE_RE = re.compile(r'^(?P<prefix>.*?,)(?P<state>[01]),"(?P<text>.*)"(?P<ending>\r?\n)?$')
SECTION_RE = re.compile(r"^\[(?P<kind>DIALOG|MENU|STRINGTABLE|TRANSLATION|EXPORTINFO|RELAYOUT)(?:\s+(?P<id>\d+))?\]$")


def escape_slt(text: str) -> str:
    out: list[str] = []
    for ch in text:
        if ch == "\\":
            out.append("\\\\")
        elif ch == "\a":
            out.append("\\a")
        elif ch == "\b":
            out.append("\\b")
        elif ch == "\f":
            out.append("\\f")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif ch == "\v":
            out.append("\\v")
        elif ch in ("\ufeff", "\ufffe", "\uffff"):
            out.append(f"\\x{ord(ch):04X}")
        else:
            out.append(ch)
    return "".join(out)


def get_resource(pe: pefile.PE, type_id: int, name_id: int) -> bytes | None:
    if not hasattr(pe, "DIRECTORY_ENTRY_RESOURCE"):
        return None
    for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        if entry.id != type_id or not hasattr(entry, "directory"):
            continue
        for name_entry in entry.directory.entries:
            if name_entry.id != name_id:
                continue
            lang_entry = name_entry.directory.entries[0]
            offset = pe.get_offset_from_rva(lang_entry.data.struct.OffsetToData)
            size = lang_entry.data.struct.Size
            return pe.__data__[offset : offset + size]
    return None


def list_resource_ids(pe: pefile.PE, type_id: int) -> list[int]:
    ids: list[int] = []
    if not hasattr(pe, "DIRECTORY_ENTRY_RESOURCE"):
        return ids
    for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        if entry.id != type_id or not hasattr(entry, "directory"):
            continue
        for name_entry in entry.directory.entries:
            if name_entry.id is not None:
                ids.append(name_entry.id)
    return sorted(ids)


def align4(pos: int) -> int:
    return (pos + 3) & ~3


def read_utf16z(data: bytes, pos: int) -> tuple[str, int]:
    if pos + 2 > len(data):
        return "", pos
    if data[pos : pos + 2] == b"\x00\x00":
        return "", pos + 2
    end = pos
    while end + 2 <= len(data):
        if data[end : end + 2] == b"\x00\x00":
            return data[pos:end].decode("utf-16le", "replace"), end + 2
        end += 2
    return data[pos:].decode("utf-16le", "replace"), len(data)


def skip_menu_or_class(data: bytes, pos: int) -> int:
    if pos + 2 > len(data):
        return pos
    word = struct.unpack_from("<H", data, pos)[0]
    if word == 0:
        return pos + 2
    if word == 0xFFFF:
        return pos + 4
    _, pos = read_utf16z(data, pos)
    return pos


def parse_strings(pe: pefile.PE) -> dict[int, str]:
    strings: dict[int, str] = {}
    for block_id in list_resource_ids(pe, 6):
        data = get_resource(pe, 6, block_id)
        if not data:
            continue
        pos = 0
        for index in range(16):
            if pos + 2 > len(data):
                break
            length = struct.unpack_from("<H", data, pos)[0]
            pos += 2
            text = data[pos : pos + 2 * length].decode("utf-16le", "replace")
            pos += 2 * length
            if length:
                strings[((block_id - 1) << 4) | index] = text
    return strings


def parse_dialog_texts(pe: pefile.PE, dialog_id: int) -> dict[str, str]:
    """Return map of 'title' and control-id keys to English (escaped later)."""
    data = get_resource(pe, 5, dialog_id)
    if not data:
        return {}

    pos = 0
    style_or_sig = struct.unpack_from("<I", data, pos)[0]
    pos += 4
    is_ex = style_or_sig == 0xFFFF0001
    if is_ex:
        pos += 4  # helpID
        pos += 4  # exStyle
        style = struct.unpack_from("<I", data, pos)[0]
        pos += 4
    else:
        style = style_or_sig
        pos += 4  # exStyle

    count = struct.unpack_from("<H", data, pos)[0]
    pos += 2
    pos += 8  # x,y,cx,cy
    pos = skip_menu_or_class(data, pos)
    pos = skip_menu_or_class(data, pos)
    title, pos = read_utf16z(data, pos)
    texts: dict[str, str] = {"title": title}

    DS_SETFONT = 0x40
    if style & DS_SETFONT:
        pos += 2  # pointsize
        if is_ex:
            pos += 2  # weight
            pos += 2  # italic + charset
        _, pos = read_utf16z(data, pos)

    for _ in range(count):
        pos = align4(pos)
        if is_ex:
            pos += 4  # helpID
            pos += 4  # exStyle
            pos += 4  # style
            pos += 8  # x,y,cx,cy
            control_id = struct.unpack_from("<I", data, pos)[0] & 0xFFFF
            pos += 4
        else:
            pos += 4  # style
            pos += 4  # exStyle
            pos += 8  # x,y,cx,cy
            control_id = struct.unpack_from("<H", data, pos)[0]
            pos += 2

        if pos + 2 <= len(data) and struct.unpack_from("<H", data, pos)[0] == 0xFFFF:
            pos += 4
        else:
            _, pos = read_utf16z(data, pos)

        if pos + 2 <= len(data) and struct.unpack_from("<H", data, pos)[0] == 0xFFFF:
            pos += 4
            window_name = ""
        else:
            window_name, pos = read_utf16z(data, pos)
        texts[str(control_id)] = window_name

        extra = struct.unpack_from("<H", data, pos)[0]
        pos += 2 + ((extra + 1) // 2) * 2

    return texts


def parse_menu_texts_ordered(pe: pefile.PE, menu_id: int) -> list[str]:
    """Non-empty menu texts in SLT export order (items with original text)."""
    data = get_resource(pe, 4, menu_id)
    if not data:
        return []
    if len(data) >= 4 and struct.unpack_from("<H", data, 0)[0] == 1:
        return parse_menuex_texts_ordered(data)
    return parse_menu_classic_texts_ordered(data)


def parse_menu_classic_texts_ordered(data: bytes) -> list[str]:
    texts: list[str] = []
    pos = 0
    while pos + 2 <= len(data):
        flags = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        if not (flags & 0x0010):  # not popup
            if pos + 2 > len(data):
                break
            pos += 2  # id
        text, pos = read_utf16z(data, pos)
        if text:
            texts.append(text)
        if pos >= len(data):
            break
    return texts


def parse_menuex_texts_ordered(data: bytes) -> list[str]:
    texts: list[str] = []
    pos = 4  # version + offset
    pos += 4  # help id
    pos = align4(pos)

    def read_items() -> None:
        nonlocal pos
        while pos + 12 <= len(data):
            pos = align4(pos)
            pos += 4  # type
            pos += 4  # state
            pos += 4  # id
            flags = struct.unpack_from("<H", data, pos)[0]
            pos += 2
            text, pos = read_utf16z(data, pos)
            pos = align4(pos)
            if flags & 0x0010:
                if pos + 4 <= len(data):
                    pos += 4  # helpID
            if text:
                texts.append(text)
            if flags & 0x0010:
                read_items()
            if flags & 0x0080:
                break

    read_items()
    return texts


def load_pe_maps(
    slg_path: Path,
) -> tuple[dict[int, str], dict[int, dict[str, str]], dict[int, list[str]]]:
    pe = pefile.PE(str(slg_path))
    strings = parse_strings(pe)
    dialogs = {dialog_id: parse_dialog_texts(pe, dialog_id) for dialog_id in list_resource_ids(pe, 5)}
    menus = {menu_id: parse_menu_texts_ordered(pe, menu_id) for menu_id in list_resource_ids(pe, 4)}
    return strings, dialogs, menus


def rewrite_header(lines: list[str], language: str, langid: int, ending: str) -> list[str]:
    out: list[str] = []
    section = ""
    for line in lines:
        stripped = line.strip()
        header = SECTION_RE.match(stripped)
        if header:
            section = header.group("kind")
            out.append(line)
            continue
        if section == "TRANSLATION":
            if stripped.startswith("LANGID,"):
                out.append(f"LANGID,{langid}{ending}")
                continue
            if stripped.startswith("AUTHOR,"):
                out.append(f'AUTHOR,"Open Samandarin"{ending}')
                continue
            if stripped.startswith("COMMENT,"):
                out.append(f'COMMENT,"{language} (automatic AI translation)"{ending}')
                continue
            if stripped.startswith("HELPDIR,"):
                out.append(f'HELPDIR,"ENGLISH"{ending}')
                continue
        out.append(line)
    return out


def bootstrap_slt(
    reference: Path,
    slg: Path,
    output: Path,
    language: str,
    langid: int,
) -> dict[str, int]:
    raw = reference.read_bytes()
    text = raw.decode("utf-8-sig")
    ending = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    strings, dialogs, menus = load_pe_maps(slg)

    section = ""
    section_id = 0
    dialog_control_index = -1
    menu_text_index = 0
    stats = {"replaced": 0, "kept": 0, "marked0": 0, "missing": 0}

    new_lines: list[str] = []
    for line in lines:
        header = SECTION_RE.match(line.strip())
        if header:
            section = header.group("kind")
            section_id = int(header.group("id") or 0)
            dialog_control_index = -1
            menu_text_index = 0
            new_lines.append(line)
            continue

        match = LINE_RE.match(line)
        if not match or section not in {"DIALOG", "MENU", "STRINGTABLE"}:
            new_lines.append(line)
            continue

        prefix = match.group("prefix")
        text_value = match.group("text")
        line_ending = match.group("ending") or ending
        english: str | None = None

        if section == "STRINGTABLE":
            resource_id = int(prefix.split(",", 1)[0])
            english = strings.get(resource_id)
        elif section == "DIALOG":
            dialog_control_index += 1
            dialog_map = dialogs.get(section_id, {})
            if dialog_control_index == 0:
                english = dialog_map.get("title")
            else:
                control_id = prefix.split(",", 1)[0]
                english = dialog_map.get(control_id)
        elif section == "MENU":
            menu_list = menus.get(section_id, [])
            if menu_text_index < len(menu_list):
                english = menu_list[menu_text_index]
            menu_text_index += 1

        if english is None:
            stats["missing"] += 1
            # keep reference text but mark untranslated if non-empty
            state = "0" if text_value else "1"
            if text_value:
                stats["marked0"] += 1
            else:
                stats["kept"] += 1
            new_lines.append(f'{prefix}{state},"{text_value}"{line_ending}')
            continue

        escaped = escape_slt(english)
        state = "0" if escaped else "1"
        if escaped:
            stats["replaced"] += 1
            stats["marked0"] += 1
        else:
            stats["kept"] += 1
        new_lines.append(f'{prefix}{state},"{escaped}"{line_ending}')

    new_lines = rewrite_header(new_lines, language, langid, ending)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8-sig", newline="") as handle:
        handle.write("".join(new_lines))
    return stats


LANG_IDS = {
    "arabic": 1025,
    "bulgarian": 1026,
    "chinesetraditional": 1028,
    "croatian": 1050,
    "greek": 1032,
    "hebrew": 1037,
    "indonesian": 1057,
    "japanese": 1041,
    "korean": 1042,
    "polish": 1045,
    "portuguese": 1046,
    "swedish": 1053,
    "turkish": 1055,
    "ukrainian": 1058,
    "vietnamese": 1066,
}


def resolve_slg(release_root: Path, module: str) -> Path | None:
    if module == "salamand":
        path = release_root / "lang" / "english.slg"
    else:
        path = release_root / "plugins" / module / "lang" / "english.slg"
    return path if path.is_file() else None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-language", default="italian")
    parser.add_argument("--release-root", type=Path, required=True)
    parser.add_argument("--languages", default="polish")
    parser.add_argument("--modules", default="")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()

    languages = [part.strip().lower() for part in args.languages.split(",") if part.strip()]
    ref_dir = args.repo_root / "translations" / args.reference_language
    modules = (
        [part.strip().lower() for part in args.modules.split(",") if part.strip()]
        if args.modules
        else sorted(path.stem for path in ref_dir.glob("*.slt"))
    )

    for language in languages:
        langid = LANG_IDS[language]
        out_dir = args.repo_root / "translations" / language
        print(f"=== {language} (LANGID {langid}) ===")
        for module in modules:
            ref = ref_dir / f"{module}.slt"
            if not ref.is_file():
                print(f"  skip {module}: no reference")
                continue
            slg = resolve_slg(args.release_root, module)
            if slg is None:
                print(f"  skip {module}: no english.slg in release")
                continue
            stats = bootstrap_slt(ref, slg, out_dir / f"{module}.slt", language, langid)
            print(
                f"  {module}: replaced={stats['replaced']} missing={stats['missing']} "
                f"state0={stats['marked0']}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
