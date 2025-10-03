#!/usr/bin/env python3
"""Update setup.inf with copy instructions for managed plugins.

This helper scans the staged plugin outputs produced by the Salamander
build system and injects the necessary [CopyFiles] and [CreateDirs]
entries for the managed plugins that do not ship with the historical
installer template (jsonviewer, textviewer, webview2renderviewer and
samandarin).

The script keeps the rest of setup.inf intact: it removes any existing
entries related to these plugins and appends the freshly generated
content at the end of the relevant sections.  This makes the script
idempotent – it can be re-run every time the managed dependencies change
without creating duplicate entries or forcing manual edits.

Typical usage (after building both architectures):

    python tools/setup/update_setup_inf.py \
        --build-root "%OPENSAL_BUILD_DIR%" \
        --setup-inf "%OPENSAL_BUILD_DIR%\\setup\\Release_x64\\setup.inf"

If you keep separate setup.inf files per architecture, invoke the script
for each file and point --build-root to the directory that contains the
matching staged Salamander tree.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
from typing import Dict, Iterable, List, Mapping, Sequence, Tuple

PluginName = str
Architecture = str

# Flags used for specific files.  The majority of payload files use
# COPY_FLAG_SKIP_SAME (0x20), but the documentation files are marked with
# 0x01 to ensure they are always refreshed.
SPECIAL_FILE_FLAGS: Dict[str, str] = {
    "dependencies.md": "0x01",
    "supported_file_types.md": "0x01",
}

PLUGINS: Sequence[PluginName] = (
    "jsonviewer",
    "textviewer",
    "webview2renderviewer",
    "samandarin",
)

ARCHITECTURES: Sequence[Architecture] = ("Release_x64", "Release_x86")

COMMENT_COPYFILES_BEGIN = "; BEGIN auto-generated plugin copy entries\n"
COMMENT_COPYFILES_END = "; END auto-generated plugin copy entries\n"
COMMENT_CREATEDIRS_BEGIN = "; BEGIN auto-generated plugin directories\n"
COMMENT_CREATEDIRS_END = "; END auto-generated plugin directories\n"


class SetupInfError(Exception):
    """Raised when the setup.inf file cannot be processed."""


def detect_line_ending(lines: Sequence[str]) -> str:
    for line in lines:
        if line.endswith("\r\n"):
            return "\r\n"
        if line.endswith("\n"):
            return "\n"
    return os.linesep


def gather_plugin_files(build_root: Path) -> Dict[Architecture, Dict[PluginName, List[Tuple[Path, str]]]]:
    """Collect files to copy for each plugin and architecture.

    Returns a nested mapping from architecture to plugin name to a list
    of tuples ``(relative_path, flag)``.  ``relative_path`` is relative
    to the plugin directory and uses Windows path separators.
    """

    result: Dict[Architecture, Dict[PluginName, List[Tuple[Path, str]]]] = {}

    for arch in ARCHITECTURES:
        arch_plugins: Dict[PluginName, List[Tuple[Path, str]]] = {}
        for plugin in PLUGINS:
            plugin_dir = build_root / "salamander" / arch / "plugins" / plugin
            if not plugin_dir.exists():
                continue
            files: List[Tuple[Path, str]] = []
            for path in sorted(plugin_dir.rglob("*")):
                if path.is_file():
                    rel_path = path.relative_to(plugin_dir)
                    flag = SPECIAL_FILE_FLAGS.get(path.name.lower(), "0x20")
                    files.append((rel_path, flag))
            if files:
                arch_plugins[plugin] = files
        if arch_plugins:
            result[arch] = arch_plugins

    if not result:
        raise SetupInfError(
            "No plugin payloads were found under the build root. Make sure the"
            " Salamander solution has been built before running this script."
        )

    return result


def make_copy_line(arch: Architecture, plugin: PluginName, rel_path: Path, flag: str, newline: str) -> str:
    rel = str(rel_path).replace("/", "\\")
    src = f"%0\\salamander\\{arch}\\plugins\\{plugin}\\{rel}"
    dst = f"%1\\plugins\\{plugin}\\{rel}"
    return f"{src},{dst},{flag}{newline}"


def accumulate_directories(entries: Iterable[str]) -> List[str]:
    dirs = set()
    prefix = "%1\\"
    for entry in entries:
        if not entry.lower().startswith("%1\\plugins\\"):
            continue
        rel = entry[len(prefix):]
        parts = [p for p in rel.split("\\") if p]
        if len(parts) <= 1:
            continue
        # Remove the file name.
        parts.pop()
        # Skip the leading "plugins" component, the rest are plugin-specific.
        if parts and parts[0].lower() == "plugins":
            parts = parts[1:]
        current: List[str] = []
        for part in parts:
            current.append(part)
            dirs.add("\\".join(current))
    ordered = sorted(dirs, key=lambda x: x.lower())
    return [f"%1\\plugins\\{path}" for path in ordered]


def find_section(lines: Sequence[str], section_name: str) -> Tuple[int, int]:
    target = f"[{section_name.lower()}]"
    start = -1
    for idx, line in enumerate(lines):
        if line.strip().lower() == target:
            start = idx
            break
    if start == -1:
        raise SetupInfError(f"Section [{section_name}] not found in setup.inf")

    end = len(lines)
    for idx in range(start + 1, len(lines)):
        if lines[idx].startswith("["):
            end = idx
            break

    return start, end


def remove_plugin_lines(lines: List[str], start: int, end: int, patterns: Sequence[str]) -> None:
    idx = start + 1
    lower_patterns = tuple(p.lower() for p in patterns)
    while idx < end:
        stripped = lines[idx].strip().lower()
        if any(pattern in stripped for pattern in lower_patterns) or stripped in {
            COMMENT_COPYFILES_BEGIN.strip().lower(),
            COMMENT_COPYFILES_END.strip().lower(),
            COMMENT_CREATEDIRS_BEGIN.strip().lower(),
            COMMENT_CREATEDIRS_END.strip().lower(),
        }:
            del lines[idx]
            end -= 1
        else:
            idx += 1


def insert_lines(lines: List[str], start: int, end: int, new_lines: Sequence[str], line_ending: str) -> Tuple[int, int]:
    insertion = end
    if insertion > start + 1 and lines[insertion - 1].strip():
        lines.insert(insertion, line_ending)
        insertion += 1
        end += 1

    for offset, text in enumerate(new_lines):
        lines.insert(insertion + offset, text)

    end += len(new_lines)
    return start, end


def update_copyfiles_section(
    lines: List[str],
    payloads: Mapping[Architecture, Mapping[PluginName, List[Tuple[Path, str]]]],
    line_ending: str,
) -> None:
    start, end = find_section(lines, "CopyFiles")
    patterns = [f"\\plugins\\{plugin}" for plugin in PLUGINS]
    remove_plugin_lines(lines, start, end, patterns)
    start, end = find_section(lines, "CopyFiles")

    generated: List[str] = [COMMENT_COPYFILES_BEGIN.replace("\n", line_ending)]

    for arch in ARCHITECTURES:
        arch_payload = payloads.get(arch)
        if not arch_payload:
            continue
        for plugin in PLUGINS:
            files = arch_payload.get(plugin)
            if not files:
                continue
            generated.append(f"; {plugin} ({arch}){line_ending}")
            for rel_path, flag in files:
                generated.append(
                    make_copy_line(arch, plugin, rel_path, flag, line_ending)
                )
    generated.append(COMMENT_COPYFILES_END.replace("\n", line_ending))

    insert_lines(lines, start, end, generated, line_ending)


def update_createdirs_section(
    lines: List[str],
    payloads: Mapping[Architecture, Mapping[PluginName, List[Tuple[Path, str]]]],
    line_ending: str,
) -> None:
    start, end = find_section(lines, "CreateDirs")
    patterns = [f"%1\\plugins\\{plugin}" for plugin in PLUGINS]
    remove_plugin_lines(lines, start, end, patterns)
    start, end = find_section(lines, "CreateDirs")

    dest_entries: List[str] = []
    for arch_payload in payloads.values():
        for plugin, files in arch_payload.items():
            for rel_path, _ in files:
                dest_entries.append(
                    f"%1\\plugins\\{plugin}\\{str(rel_path).replace('/', '\\')}"
                )

    directories = accumulate_directories(dest_entries)
    if not directories:
        return

    generated = [COMMENT_CREATEDIRS_BEGIN.replace("\n", line_ending)]
    for directory in directories:
        generated.append(f"{directory}{line_ending}")
    generated.append(COMMENT_CREATEDIRS_END.replace("\n", line_ending))

    insert_lines(lines, start, end, generated, line_ending)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Update setup.inf for managed plugins.")
    parser.add_argument(
        "--build-root",
        type=Path,
        default=os.environ.get("OPENSAL_BUILD_DIR"),
        help="Path to the OPENSAL build root containing the staged Salamander tree.",
    )
    parser.add_argument(
        "--setup-inf",
        type=Path,
        required=True,
        help="Path to the setup.inf file that should be updated.",
    )
    parser.add_argument(
        "--no-backup",
        action="store_true",
        help="Do not create a .bak file next to setup.inf before writing changes.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not args.build_root:
        raise SetupInfError(
            "--build-root was not provided and OPENSAL_BUILD_DIR is not set."
        )

    build_root = args.build_root
    if not build_root.exists():
        raise SetupInfError(f"Build root '{build_root}' does not exist.")

    setup_inf_path = args.setup_inf
    if not setup_inf_path.exists():
        raise SetupInfError(f"setup.inf file '{setup_inf_path}' does not exist.")

    original_content = setup_inf_path.read_text(encoding="utf-8-sig")
    lines = original_content.splitlines(keepends=True)
    line_ending = detect_line_ending(lines)

    payloads = gather_plugin_files(build_root)

    update_copyfiles_section(lines, payloads, line_ending)
    update_createdirs_section(lines, payloads, line_ending)

    updated_content = "".join(lines)

    if updated_content == original_content:
        return

    if not args.no_backup:
        backup_path = setup_inf_path.with_suffix(setup_inf_path.suffix + ".bak")
        backup_path.write_text(original_content, encoding="utf-8")

    setup_inf_path.write_text(updated_content, encoding="utf-8")


if __name__ == "__main__":
    try:
        main()
    except SetupInfError as exc:
        raise SystemExit(str(exc))
