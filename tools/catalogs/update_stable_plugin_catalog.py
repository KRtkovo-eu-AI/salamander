#!/usr/bin/env python3
"""Update shipped plugin and extension entries in the base catalogs.

Existing catalog membership is authoritative: shipped packages already present
in a base catalog stay in that catalog, while new packages default to the stable
catalog. The script keeps catalog metadata/translations intact, refreshes
versions from plugin versinfo.rh2 files and extension manifests, synchronizes
installer plugin-selection versions, and updates generatedAt.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
from pathlib import Path
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CATALOG = ROOT / "doc" / "catalogs-base" / "plugins-stable.json"
DEFAULT_INSTALLER = ROOT / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss"
DEFAULT_PLUGINS_ROOT = ROOT / "src" / "plugins"
DEFAULT_EXTENSIONS_ROOT = ROOT / "src" / "extensions"
DEFAULT_RELEASE_URL = "https://github.com/KRtkovo-eu-AI/salamander/releases"
DEFAULT_PACKAGE_RELEASE_URL = "https://github.com/KRtkovo-eu-AI/salamander-plugins/releases"
DEFAULT_PLUGIN_ICON_URL = "https://samandarin.net/catalogs/img/plugin.png"
CATALOG_SCHEMA_VERSION = 5
PACKAGE_TYPE_PLUGIN = "plugin"
PACKAGE_TYPE_EXTENSION = "extension"
EXTENSION_BUNDLES = {
    "salamatrixdemos": {
        "directory": "demos",
        "name": "Salamatrix Demo Sample Scripts",
        "description": "Demo sample scripts for the Salamatrix framework.",
        "runtime_id": "salamatrix",
    }
}

SOURCE_SPL_RE = re.compile(
    r'^\s*Source:\s*"\{#PayloadDir\}\\plugins\\(?P<path>[^"]+\.spl)"',
    re.IGNORECASE | re.MULTILINE,
)
SOURCE_EXTENSION_RE = re.compile(
    r'^\s*Source:\s*"\{#PayloadDir\}\\extensions\\(?P<directory>[^\\"]+)\\[^"]*".*?'
    r"Check:\s*IsPluginSelected\('(?P<installer_id>[^']+)'\)",
    re.IGNORECASE | re.MULTILINE,
)
DEFINE_RE = re.compile(
    r"^\s*#define\s+"
    r"(VERSINFO_(?:MAJOR|MINORA|MINORB|DESCRIPTION|BETAVERSION_TXT_NO_PLATFORM))"
    r"\s+(.+?)\s*$",
    re.MULTILINE,
)
INSTALLER_ADD_PLUGIN_RE = re.compile(
    r"^(?P<prefix>\s*AddPlugin\('(?P<id>[^']+)',\s*'(?P<name>(?:''|[^'])*)',\s*)"
    r"'(?P<version>[^']*)'(?P<suffix>,\s*(?:True|False)\);\s*)$",
    re.MULTILINE,
)
SET_BASIC_PLUGIN_DATA_VERSION_RE = re.compile(
    r'SetBasicPluginData\([^;]*?,\s*"(?P<version>\d+(?:\.\d+)*)"', re.DOTALL
)


def _unique(values: Iterable[str]) -> list[str]:
    result: list[str] = []
    seen: set[str] = set()
    for value in values:
        if value not in seen:
            result.append(value)
            seen.add(value)
    return result


def parse_installer_plugins(installer: Path) -> list[str]:
    """Return unique plugin ids in the order their .spl files appear in [Files]."""
    text = installer.read_text(encoding="utf-8-sig")
    ids = _unique(
        Path(match.group("path").replace("\\", "/")).stem
        for match in SOURCE_SPL_RE.finditer(text)
    )
    if not ids:
        raise RuntimeError(f"No .spl plugin entries found in {installer}")
    return ids


def parse_installer_extensions(
    installer: Path, extensions_root: Path
) -> list[tuple[str, str]]:
    """Return (catalog id, installer id) for shipped manifest extensions."""
    text = installer.read_text(encoding="utf-8-sig")
    extensions: list[tuple[str, str]] = []
    seen: set[str] = set()
    for match in SOURCE_EXTENSION_RE.finditer(text):
        directory = match.group("directory")
        installer_id = match.group("installer_id")
        bundle_id = next(
            (
                package_id
                for package_id, bundle in EXTENSION_BUNDLES.items()
                if bundle["directory"].casefold() == directory.casefold()
            ),
            None,
        )
        extension_id = bundle_id or directory
        if extension_id in seen:
            continue
        extension_root = extensions_root / directory
        has_manifest = (extension_root / "extension.json").is_file()
        has_bundle_manifests = bundle_id is not None and any(extension_root.rglob("extension.json"))
        if has_manifest or has_bundle_manifests:
            extensions.append((extension_id, installer_id))
            seen.add(extension_id)
    return extensions


def _unquote_define_value(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] == '"':
        return bytes(value[1:-1], "utf-8").decode("unicode_escape")
    return value.split("//", 1)[0].strip()


def find_plugin_versinfo(plugin_id: str, plugins_root: Path) -> Path | None:
    """Return the best versinfo.rh2 path for a plugin."""
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
    version += defines.get("VERSINFO_BETAVERSION_TXT_NO_PLATFORM", "")
    if include_platform:
        version = f"{version} (x64)"
    return version, defines.get("VERSINFO_DESCRIPTION")


def read_extension_manifest(extension_id: str, extensions_root: Path) -> dict[str, Any]:
    manifest_path = extensions_root / extension_id / "extension.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"Cannot read extension manifest {manifest_path}") from exc
    for field in ("name", "version", "description"):
        if not isinstance(manifest.get(field), str) or not manifest[field].strip():
            raise RuntimeError(f"Extension manifest {manifest_path} has no string {field}")
    return manifest


def read_extension_metadata(
    extension_id: str, extensions_root: Path, include_platform: bool = True
) -> tuple[str, str, str, str | None]:
    """Return (version, name, description, runtime catalog id)."""
    bundle = EXTENSION_BUNDLES.get(extension_id)
    if bundle:
        manifest_paths = sorted(
            (extensions_root / bundle["directory"]).rglob("extension.json")
        )
        if not manifest_paths:
            raise RuntimeError(
                f"Extension bundle {extension_id!r} has no extension manifests"
            )
        manifests = [
            json.loads(path.read_text(encoding="utf-8-sig")) for path in manifest_paths
        ]
        versions = {
            manifest.get("version", "").strip()
            for manifest in manifests
            if isinstance(manifest.get("version"), str)
        }
        if len(versions) != 1:
            raise RuntimeError(
                f"Extension bundle {extension_id!r} has inconsistent versions: "
                + ", ".join(sorted(versions))
            )
        version = next(iter(versions))
        if not version:
            raise RuntimeError(f"Extension bundle {extension_id!r} has an empty version")
        if include_platform:
            version = f"{version} (x64)"
        return (
            version,
            bundle["name"],
            bundle["description"],
            bundle["runtime_id"],
        )

    manifest = read_extension_manifest(extension_id, extensions_root)
    version = manifest["version"].strip()
    if include_platform:
        version = f"{version} (x64)"
    runtime = manifest.get("runtime")
    runtime_id = f"{runtime.lower()}runtime" if isinstance(runtime, str) and runtime else None
    return version, manifest["name"].strip(), manifest["description"].strip(), runtime_id


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
        "packageType": PACKAGE_TYPE_PLUGIN,
        "name": {"english": display_name},
        "author": "Open Salamander Authors",
        "description": {"english": description or f"{display_name} plugin for Open Salamander"},
        "latestVersion": version or "unknown (x64)",
        "versionScheme": "fileversion",
        "homepageUrl": DEFAULT_RELEASE_URL,
        "downloadPageUrl": DEFAULT_RELEASE_URL,
    }


def new_extension_entry(extension_id: str, extensions_root: Path) -> dict[str, Any]:
    version, name, description, runtime_id = read_extension_metadata(extension_id, extensions_root)
    entry = new_plugin_entry(extension_id, version, description)
    entry["packageType"] = PACKAGE_TYPE_EXTENSION
    entry["name"] = {"english": name}
    archive_version = version.removesuffix(" (x64)")
    archive_name = f"plugin_5.0_{extension_id}_{archive_version}_x64"
    entry["downloadPageUrl"] = (
        f"{DEFAULT_PACKAGE_RELEASE_URL}/download/{archive_name}/{archive_name}.7z"
    )
    if runtime_id:
        entry["dependencies"] = [runtime_id]
    entry["icon"] = DEFAULT_PLUGIN_ICON_URL
    return entry


def load_catalogs(catalog: Path) -> dict[Path, dict[str, Any]]:
    """Load the selected catalog and its sibling base catalogs."""
    paths = sorted(catalog.parent.glob("*.json"))
    if catalog not in paths:
        paths.append(catalog)
    catalogs: dict[Path, dict[str, Any]] = {}
    for path in paths:
        data = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(data.get("plugins"), list):
            catalogs[path] = data
    if catalog not in catalogs:
        raise RuntimeError(f"Catalog {catalog} has no plugins array")
    return catalogs


def assign_packages_to_catalogs(
    catalogs: dict[Path, dict[str, Any]],
    stable_catalog: Path,
    package_ids: list[str],
    *,
    extension_ids: set[str] | None = None,
    extension_catalog: Path | None = None,
) -> dict[Path, list[str]]:
    """Assign packages using existing membership, defaulting new ids to stable."""
    extension_ids = extension_ids or set()
    owners: dict[str, list[Path]] = {}
    for path, catalog in catalogs.items():
        for entry in catalog["plugins"]:
            owners.setdefault(entry["id"], []).append(path)

    assignments: dict[Path, list[str]] = {}
    for package_id in package_ids:
        if package_id in extension_ids:
            if extension_catalog is None or extension_catalog not in catalogs:
                raise RuntimeError("Manifest extensions require extensions-stable.json")
            assignments.setdefault(extension_catalog, []).append(package_id)
            continue
        candidates = owners.get(package_id, [])
        if stable_catalog in candidates:
            target = stable_catalog
        elif len(candidates) == 1:
            target = candidates[0]
        elif not candidates:
            target = stable_catalog
        else:
            names = ", ".join(str(path) for path in candidates)
            raise RuntimeError(f"Package {package_id!r} belongs to multiple non-stable catalogs: {names}")
        assignments.setdefault(target, []).append(package_id)
    return assignments


def update_catalog(
    catalog: dict[str, Any],
    package_ids: list[str],
    plugins_root: Path,
    generated_at: str | None = None,
    *,
    extension_ids: set[str] | None = None,
    extensions_root: Path = DEFAULT_EXTENSIONS_ROOT,
) -> dict[str, Any]:
    """Synchronize one catalog with the shipped packages assigned to it."""
    extension_ids = extension_ids or set()
    existing = {plugin["id"]: plugin for plugin in catalog.get("plugins", [])}
    package_id_set = set(package_ids)
    ordered_ids = [plugin["id"] for plugin in catalog.get("plugins", []) if plugin["id"] in package_id_set]
    ordered_ids.extend(plugin_id for plugin_id in package_ids if plugin_id not in existing)

    updated_plugins: list[dict[str, Any]] = []
    for package_id in ordered_ids:
        if package_id in extension_ids:
            version, _, description, _ = read_extension_metadata(package_id, extensions_root)
            entry = dict(existing.get(package_id) or new_extension_entry(package_id, extensions_root))
            entry["packageType"] = PACKAGE_TYPE_EXTENSION
        else:
            version, description = read_plugin_metadata(package_id, plugins_root)
            entry = dict(existing.get(package_id) or new_plugin_entry(package_id, version, description))
            entry["packageType"] = PACKAGE_TYPE_PLUGIN
        if version is not None:
            entry["latestVersion"] = version
        elif "latestVersion" not in entry:
            entry["latestVersion"] = "unknown (x64)"
        updated_plugins.append(entry)

    updated = dict(catalog)
    updated["schemaVersion"] = CATALOG_SCHEMA_VERSION
    updated["generatedAt"] = generated_at or now_utc()
    updated["plugins"] = updated_plugins
    return updated


def update_catalog_schema(
    catalog: dict[str, Any], extension_ids: set[str], *, extension_catalog: bool = False
) -> dict[str, Any]:
    """Upgrade catalog metadata without changing its entry membership or ordering."""
    updated = dict(catalog)
    updated["schemaVersion"] = CATALOG_SCHEMA_VERSION
    updated_entries: list[dict[str, Any]] = []
    for package in catalog.get("plugins", []):
        entry = dict(package)
        entry["packageType"] = (
            PACKAGE_TYPE_EXTENSION
            if extension_catalog or entry.get("id") in extension_ids
            else PACKAGE_TYPE_PLUGIN
        )
        updated_entries.append(entry)
    updated["plugins"] = updated_entries
    return updated


def update_installer_plugin_versions(
    installer_text: str,
    shipped_ids: list[str],
    plugins_root: Path,
    extension_installer_ids: dict[str, str] | None = None,
    extensions_root: Path = DEFAULT_EXTENSIONS_ROOT,
) -> str:
    """Update AddPlugin(..., version, ...) calls in the installer script."""
    versions: dict[str, str] = {}
    for plugin_id in shipped_ids:
        version, _ = read_plugin_metadata(plugin_id, plugins_root, include_platform=True)
        if version is not None:
            versions[plugin_id] = version
    for extension_id, installer_id in (extension_installer_ids or {}).items():
        version, _, _, _ = read_extension_metadata(extension_id, extensions_root, include_platform=True)
        versions[installer_id] = version

    seen: set[str] = set()

    def replace(match: re.Match[str]) -> str:
        plugin_id = match.group("id")
        seen.add(plugin_id)
        version = versions.get(plugin_id, match.group("version"))
        return f"{match.group('prefix')}'{version}'{match.group('suffix')}"

    updated = INSTALLER_ADD_PLUGIN_RE.sub(replace, installer_text)
    missing = sorted((set(shipped_ids) | set((extension_installer_ids or {}).values())) - seen)
    if missing:
        raise RuntimeError("Installer plugin selection is missing AddPlugin entries for: " + ", ".join(missing))
    return updated


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--installer", type=Path, default=DEFAULT_INSTALLER)
    parser.add_argument("--plugins-root", type=Path, default=DEFAULT_PLUGINS_ROOT)
    parser.add_argument("--extensions-root", type=Path, default=DEFAULT_EXTENSIONS_ROOT)
    parser.add_argument("--check", action="store_true", help="fail if a catalog or installer would change")
    args = parser.parse_args()

    catalogs = load_catalogs(args.catalog)
    shipped_plugin_ids = parse_installer_plugins(args.installer)
    shipped_extensions = parse_installer_extensions(args.installer, args.extensions_root)
    extension_ids = [extension_id for extension_id, _ in shipped_extensions]
    package_ids = shipped_plugin_ids + extension_ids
    extension_catalog = args.catalog.parent / "extensions-stable.json"
    assignments = assign_packages_to_catalogs(
        catalogs,
        args.catalog,
        package_ids,
        extension_ids=set(extension_ids),
        extension_catalog=extension_catalog,
    )
    generated_at = None if args.check else now_utc()

    rendered_catalogs: dict[Path, str] = {}
    extension_id_set = set(extension_ids)
    for path, catalog_data in catalogs.items():
        if path in assignments:
            timestamp = catalog_data.get("generatedAt") if args.check else generated_at
            updated = update_catalog(
                catalog_data,
                assignments[path],
                args.plugins_root,
                timestamp,
                extension_ids=extension_id_set,
                extensions_root=args.extensions_root,
            )
        else:
            updated = catalog_data
        updated = update_catalog_schema(
            updated,
            extension_id_set,
            extension_catalog=path.name.casefold() == "extensions-stable.json",
        )
        rendered_catalogs[path] = json.dumps(updated, ensure_ascii=False, indent=2) + "\n"

    installer_current = args.installer.read_text(encoding="utf-8-sig")
    installer_rendered = update_installer_plugin_versions(
        installer_current,
        shipped_plugin_ids,
        args.plugins_root,
        dict(shipped_extensions),
        args.extensions_root,
    )

    if args.check:
        stale = [
            str(path)
            for path, rendered in rendered_catalogs.items()
            if rendered != path.read_text(encoding="utf-8")
        ]
        if stale:
            raise SystemExit(
                f"{', '.join(stale)} {'is' if len(stale) == 1 else 'are'} not up to date; "
                f"run {Path(__file__).as_posix()}"
            )
        if installer_rendered != installer_current:
            raise SystemExit(
                f"{args.installer} plugin versions are not up to date; "
                f"run {Path(__file__).as_posix()}"
            )
        return 0

    for path, rendered in rendered_catalogs.items():
        path.write_text(rendered, encoding="utf-8")
    args.installer.write_text(installer_rendered, encoding="utf-8")
    counts = ", ".join(
        f"{path.name}: {len(assignments.get(path, []))}" for path in sorted(catalogs)
    )
    print(f"Updated base catalogs ({counts}) and synchronized installer plugin versions.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
