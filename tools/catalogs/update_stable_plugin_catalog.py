#!/usr/bin/env python3
"""Update the stable plugin catalog from the Inno Setup installer manifest.

The script keeps existing catalog metadata/translations intact, synchronizes the
plugin list with the .spl plugins shipped by the installer, refreshes versions
from each plugin's versinfo.rh2, keeps installer plugin-selection versions
in sync, and updates generatedAt.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CATALOG = ROOT / "doc" / "catalogs-base" / "plugins-stable.json"
DEFAULT_INSTALLER = ROOT / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss"
DEFAULT_PLUGINS_ROOT = ROOT / "src" / "plugins"
DEFAULT_RELEASE_URL = "https://github.com/KRtkovo-eu-AI/salamander/releases"
STABLE_CATALOG_EXCLUDED_PLUGIN_IDS = {"demoplug", "salamatrix"}

SOURCE_SPL_RE = re.compile(
    r'^\s*Source:\s*"\{#PayloadDir\}\\plugins\\(?P<id>[^\\"]+)\\(?P<file>[^\\"]+\.spl)"',
    re.IGNORECASE | re.MULTILINE,
)
DEFINE_RE = re.compile(
    r"^\s*#define\s+(VERSINFO_(?:MAJOR|MINORA|MINORB|DESCRIPTION))\s+(.+?)\s*$",
    re.MULTILINE,
)
INSTALLER_ADD_PLUGIN_RE = re.compile(
    r"^(?P<prefix>\s*AddPlugin\('(?P<id>[^']+)',\s*'(?P<name>(?:''|[^'])*)',\s*)'(?P<version>[^']*)'(?P<suffix>,\s*(?:True|False)\);\s*)$",
    re.MULTILINE,
)
SET_BASIC_PLUGIN_DATA_VERSION_RE = re.compile(
    r"SetBasicPluginData\([^;]*?,\s*\"(?P<version>\d+(?:\.\d+)*)\"", re.DOTALL
)


def parse_installer_plugins(installer: Path) -> list[str]:
    """Return unique plugin ids in the order their .spl files appear in [Files]."""
    text = installer.read_text(encoding="utf-8-sig")
    ids: list[str] = []
    seen: set[str] = set()
    for match in SOURCE_SPL_RE.finditer(text):
        plugin_id = match.group("id")
        if plugin_id not in seen:
            ids.append(plugin_id)
            seen.add(plugin_id)
    if not ids:
        raise RuntimeError(f"No .spl plugin entries found in {installer}")
    return ids


def _unquote_define_value(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] == '"':
        return bytes(value[1:-1], "utf-8").decode("unicode_escape")
    return value.split("//", 1)[0].strip()


def find_plugin_versinfo(plugin_id: str, plugins_root: Path) -> Path | None:
    """Return the best versinfo.rh2 path for a plugin, including nested plugin projects."""
    plugin_root = plugins_root / plugin_id
    direct = plugin_root / "versinfo.rh2"
    if direct.exists():
        return direct
    candidates = sorted(plugin_root.rglob("versinfo.rh2"))
    return candidates[0] if candidates else None


def read_plugin_source_version(plugin_id: str, plugins_root: Path) -> str | None:
    """Return a source-declared plugin version when no versinfo.rh2 exists."""
    plugin_root = plugins_root / plugin_id
    for source in sorted(plugin_root.glob("*.cpp")):
        text = source.read_text(encoding="utf-8-sig", errors="ignore")
        match = SET_BASIC_PLUGIN_DATA_VERSION_RE.search(text)
        if match:
            return match.group("version")
    return None


def read_plugin_metadata(
    plugin_id: str, plugins_root: Path, include_platform: bool = True
) -> tuple[str | None, str | None]:
    """Return (version, English description) from plugin metadata."""
    versinfo = find_plugin_versinfo(plugin_id, plugins_root)
    if not versinfo:
        version = read_plugin_source_version(plugin_id, plugins_root)
        return (f"{version} (x64)" if include_platform and version else version), None
    text = versinfo.read_text(encoding="utf-8-sig")
    defines = {name: _unquote_define_value(value) for name, value in DEFINE_RE.findall(text)}
    try:
        major = int(defines["VERSINFO_MAJOR"])
        minora = int(defines["VERSINFO_MINORA"])
        minorb = int(defines.get("VERSINFO_MINORB", "0"))
    except (KeyError, ValueError) as exc:
        raise RuntimeError(f"Cannot parse version macros in {versinfo}") from exc

    version = f"{major}.{minora}" if minorb == 0 else f"{major}.{minora}{minorb}"
    if include_platform:
        version = f"{version} (x64)"
    return version, defines.get("VERSINFO_DESCRIPTION")


def now_utc() -> str:
    epoch = os.environ.get("SOURCE_DATE_EPOCH")
    if epoch is not None:
        instant = dt.datetime.fromtimestamp(int(epoch), tz=dt.timezone.utc)
    else:
        instant = dt.datetime.now(dt.timezone.utc)
    return instant.replace(microsecond=0).isoformat().replace("+00:00", "Z")


def new_plugin_entry(plugin_id: str, version: str | None, description: str | None) -> dict[str, Any]:
    display_name = plugin_id.replace("-", " ").replace("_", " ").title()
    return {
        "id": plugin_id,
        "name": {"english": display_name},
        "author": "Open Salamander Authors",
        "description": {"english": description or f"{display_name} plugin for Open Salamander"},
        "latestVersion": version or "unknown (x64)",
        "versionScheme": "fileversion",
        "homepageUrl": DEFAULT_RELEASE_URL,
        "downloadPageUrl": DEFAULT_RELEASE_URL,
    }


def update_catalog(
    catalog: dict[str, Any], shipped_ids: list[str], plugins_root: Path, generated_at: str | None = None
) -> dict[str, Any]:
    existing = {plugin["id"]: plugin for plugin in catalog.get("plugins", [])}
    updated_plugins: list[dict[str, Any]] = []
    stable_ids = [plugin_id for plugin_id in shipped_ids if plugin_id not in STABLE_CATALOG_EXCLUDED_PLUGIN_IDS]
    for plugin_id in stable_ids:
        version, description = read_plugin_metadata(plugin_id, plugins_root)
        entry = dict(existing.get(plugin_id) or new_plugin_entry(plugin_id, version, description))
        if version is not None:
            entry["latestVersion"] = version
        elif "latestVersion" not in entry:
            entry["latestVersion"] = "unknown (x64)"
        updated_plugins.append(entry)

    updated = dict(catalog)
    updated["generatedAt"] = generated_at or now_utc()
    updated["plugins"] = updated_plugins
    return updated


def update_installer_plugin_versions(installer_text: str, shipped_ids: list[str], plugins_root: Path) -> str:
    """Update AddPlugin(..., version, ...) calls in the installer script."""
    versions: dict[str, str] = {}
    for plugin_id in shipped_ids:
        version, _ = read_plugin_metadata(plugin_id, plugins_root, include_platform=True)
        if version is not None:
            versions[plugin_id] = version

    seen: set[str] = set()

    def replace(match: re.Match[str]) -> str:
        plugin_id = match.group("id")
        seen.add(plugin_id)
        version = versions.get(plugin_id, match.group("version"))
        return f"{match.group('prefix')}'{version}'{match.group('suffix')}"

    updated = INSTALLER_ADD_PLUGIN_RE.sub(replace, installer_text)
    missing = sorted(set(shipped_ids) - seen)
    if missing:
        raise RuntimeError("Installer plugin selection is missing AddPlugin entries for: " + ", ".join(missing))
    return updated



def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--installer", type=Path, default=DEFAULT_INSTALLER)
    parser.add_argument("--plugins-root", type=Path, default=DEFAULT_PLUGINS_ROOT)
    parser.add_argument("--check", action="store_true", help="fail if the catalog would change")
    args = parser.parse_args()

    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    shipped_ids = parse_installer_plugins(args.installer)
    generated_at = catalog.get("generatedAt") if args.check else None
    updated = update_catalog(catalog, shipped_ids, args.plugins_root, generated_at)
    rendered = json.dumps(updated, ensure_ascii=False, indent=2) + "\n"
    installer_current = args.installer.read_text(encoding="utf-8-sig")
    installer_rendered = update_installer_plugin_versions(installer_current, shipped_ids, args.plugins_root)

    current = args.catalog.read_text(encoding="utf-8")
    if args.check:
        if rendered != current:
            raise SystemExit(f"{args.catalog} is not up to date; run {Path(__file__).as_posix()}")
        if installer_rendered != installer_current:
            raise SystemExit(f"{args.installer} plugin versions are not up to date; run {Path(__file__).as_posix()}")
        return 0

    args.catalog.write_text(rendered, encoding="utf-8")
    args.installer.write_text(installer_rendered, encoding="utf-8")
    print(
        f"Updated {args.catalog} with {len(updated['plugins'])} stable plugins "
        "and synchronized installer plugin versions."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
