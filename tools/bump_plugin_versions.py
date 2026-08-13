#!/usr/bin/env python3
"""Interactively bump plugin VERSINFO_* versions after reviewing git changes."""
from __future__ import annotations

import argparse
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


def find_repo_root() -> Path:
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        cwd=SCRIPT_DIR,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode == 0:
        return Path(result.stdout.strip())
    return SCRIPT_DIR.parent


ROOT = find_repo_root()
DEFAULT_PLUGINS_ROOT = ROOT / "src" / "plugins"
DEFINE_RE = re.compile(r"^(?P<prefix>\s*#define\s+{name}\s+)(?P<value>\d+)(?P<suffix>.*)$", re.MULTILINE)


@dataclass
class PluginVersion:
    plugin_id: str
    versinfo: Path
    major: int
    minora: int
    minorb: int

    @property
    def display(self) -> str:
        return f"{self.major}.{self.minora}" if self.minorb == 0 else f"{self.major}.{self.minora}.{self.minorb}"

    def bumped(self) -> "PluginVersion":
        if self.minorb == 0:
            return PluginVersion(self.plugin_id, self.versinfo, self.major, self.minora + 1, 0)
        minor_text = f"{self.minora}{self.minorb}"
        bumped_minor = int(minor_text) + 1
        if str(bumped_minor).endswith("0"):
            return PluginVersion(self.plugin_id, self.versinfo, self.major, bumped_minor, 0)
        return PluginVersion(self.plugin_id, self.versinfo, self.major, bumped_minor // 10, bumped_minor % 10)


def run_git(args: list[str], check: bool = True) -> str:
    result = subprocess.run(["git", *args], cwd=ROOT, text=True, capture_output=True, check=False)
    if check and result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "git command failed")
    return result.stdout


def list_tags() -> list[str]:
    return [tag for tag in run_git(["tag", "--sort=-creatordate"]).splitlines() if tag]


def choose_tag(provided: str | None) -> str | None:
    if provided:
        return provided
    tags = list_tags()
    if not tags:
        print("No git tags found; change detection will be skipped.")
        return None
    print("Select baseline tag for change detection:")
    for index, tag in enumerate(tags[:20], 1):
        print(f"  {index:2}. {tag}")
    answer = input("Tag number/name (empty to skip): ").strip()
    if not answer:
        return None
    if answer.isdigit() and 1 <= int(answer) <= min(len(tags), 20):
        return tags[int(answer) - 1]
    return answer


def find_versinfo_files(plugins_root: Path) -> list[Path]:
    return sorted(plugins_root.rglob("versinfo.rh2"))


def read_macro(text: str, name: str, path: Path, default: int | None = None) -> int:
    pattern = re.compile(DEFINE_RE.pattern.format(name=re.escape(name)), re.MULTILINE)
    match = pattern.search(text)
    if not match:
        if default is not None:
            return default
        raise RuntimeError(f"{path} does not define {name}")
    return int(match.group("value"))


def read_version(path: Path, plugins_root: Path) -> PluginVersion:
    text = path.read_text(encoding="utf-8-sig")
    plugin_id = path.relative_to(plugins_root).parts[0]
    return PluginVersion(
        plugin_id=plugin_id,
        versinfo=path,
        major=read_macro(text, "VERSINFO_MAJOR", path),
        minora=read_macro(text, "VERSINFO_MINORA", path),
        minorb=read_macro(text, "VERSINFO_MINORB", path, default=0),
    )


def replace_macro(text: str, name: str, value: int) -> str:
    pattern = re.compile(DEFINE_RE.pattern.format(name=re.escape(name)), re.MULTILINE)
    if pattern.search(text):
        return pattern.sub(lambda m: f"{m.group('prefix')}{value}{m.group('suffix')}", text, count=1)
    if name == "VERSINFO_MINORB":
        minora_pattern = re.compile(DEFINE_RE.pattern.format(name=re.escape("VERSINFO_MINORA")), re.MULTILINE)
        return minora_pattern.sub(lambda m: f"{m.group(0)}\n#define VERSINFO_MINORB      {value}", text, count=1)
    raise RuntimeError(f"Cannot update missing {name}")


def write_version(version: PluginVersion) -> None:
    text = version.versinfo.read_text(encoding="utf-8-sig")
    text = replace_macro(text, "VERSINFO_MAJOR", version.major)
    text = replace_macro(text, "VERSINFO_MINORA", version.minora)
    text = replace_macro(text, "VERSINFO_MINORB", version.minorb)
    version.versinfo.write_text(text, encoding="utf-8")


def version_at_tag(path: Path, tag: str, plugins_root: Path) -> PluginVersion | None:
    rel = path.relative_to(ROOT).as_posix()
    result = subprocess.run(["git", "show", f"{tag}:{rel}"], cwd=ROOT, text=True, capture_output=True, check=False)
    if result.returncode != 0:
        return None
    tmp = path.with_suffix(path.suffix + ".tagtmp")
    try:
        tmp.write_text(result.stdout, encoding="utf-8")
        return read_version(tmp, plugins_root)
    finally:
        tmp.unlink(missing_ok=True)


def is_plugin_project_file(path: str) -> bool:
    return Path(path).suffix.lower() == ".vcxproj"


def plugin_changes_since(plugin_root: Path, tag: str | None) -> list[tuple[str, str, str]]:
    if not tag:
        return []
    rel = plugin_root.relative_to(ROOT).as_posix()
    changes: list[tuple[str, str, str]] = []
    for line in run_git(["diff", "--numstat", tag, "--", rel], check=False).splitlines():
        parts = line.split("\t")
        if len(parts) >= 3 and not is_plugin_project_file(parts[2]):
            changes.append((parts[0], parts[1], parts[2]))
    return changes


def format_change_count(value: str) -> str:
    return value if value != "-" else "binary"


def print_changes(changes: list[tuple[str, str, str]]) -> None:
    if not changes:
        print("  Changed files since selected tag: none")
        return
    print(f"  Changed files since selected tag ({len(changes)}):")
    for added, removed, path in changes:
        print(f"    +{format_change_count(added)} -{format_change_count(removed)}  {path}")


def prompt(
    version: PluginVersion,
    previous: PluginVersion | None,
    proposed: PluginVersion,
    changes: list[tuple[str, str, str]],
    version_changed: bool,
) -> PluginVersion | None:
    changed = bool(changes)
    marker = "changed without version bump" if changed and not version_changed else "review"
    print(f"\n{version.plugin_id}: {version.display} [{marker}]")
    if previous:
        print(f"  Version in selected tag: {previous.display}")
    else:
        print("  Version in selected tag: unavailable")
    print_changes(changes)
    print(f"  VERSINFO_MAJOR  {version.major} -> {proposed.major}")
    print(f"  VERSINFO_MINORA {version.minora} -> {proposed.minora}")
    print(f"  VERSINFO_MINORB {version.minorb} -> {proposed.minorb}")
    answer = input("Apply bump? [y]es/[n]o/[e]dit/[q]uit: ").strip().lower() or "n"
    if answer == "q":
        raise KeyboardInterrupt
    if answer == "y":
        return proposed
    if answer == "e":
        major = int(input(f"VERSINFO_MAJOR [{proposed.major}]: ").strip() or proposed.major)
        minora = int(input(f"VERSINFO_MINORA [{proposed.minora}]: ").strip() or proposed.minora)
        minorb = int(input(f"VERSINFO_MINORB [{proposed.minorb}]: ").strip() or proposed.minorb)
        return PluginVersion(version.plugin_id, version.versinfo, major, minora, minorb)
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plugins-root", type=Path, default=DEFAULT_PLUGINS_ROOT)
    parser.add_argument("--tag", help="baseline git tag for change detection")
    parser.add_argument("--all", action="store_true", help="prompt for every plugin, not only changed plugins")
    parser.add_argument("--dry-run", action="store_true", help="show prompts but do not write changes")
    args = parser.parse_args()

    tag = choose_tag(args.tag)
    versions = [read_version(path, args.plugins_root) for path in find_versinfo_files(args.plugins_root)]
    applied = 0
    try:
        for version in versions:
            previous = version_at_tag(version.versinfo, tag, args.plugins_root) if tag else None
            changes = plugin_changes_since(args.plugins_root / version.plugin_id, tag)
            version_changed = previous is not None and previous.display != version.display
            if not args.all and not (changes and not version_changed):
                continue
            selected = prompt(version, previous, version.bumped(), changes, version_changed)
            if selected:
                applied += 1
                if not args.dry_run:
                    write_version(selected)
    except KeyboardInterrupt:
        print("\nStopped by user.")
    print(f"Processed {len(versions)} version files; applied {applied} bump(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
