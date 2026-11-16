#!/usr/bin/env python3
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-15 | Emit Pascal comments in the generated Inno code include. | The include is parsed inside the installer [Code] section. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-14 | Generate the Inno web-package lookup from stable package catalogs. | Keep installer download URLs, hashes, dependencies, and component IDs synchronized. | | openrouter/openai/gpt-6-luna-pro |
"""Generate the Inno Pascal package lookup from the stable base catalogs.

Only packages with a syntactically valid SHA-256 are emitted as downloadable.
An empty packageSha256 remains unavailable until a release asset is published
and its digest is recorded in the catalog.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[2]
CATALOG_DIR = ROOT / "doc" / "catalogs-base"
INSTALLER_PATH = ROOT / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss"
OUTPUT_PATH = ROOT / "doc" / "runbook-setup" / "inno_package_catalog.issinc"
CATALOG_FILES = (
    "plugins-stable.json",
    "extensions-stable.json",
    "extension-runtimes.json",
)
ADD_PLUGIN_RE = re.compile(r"^\s*AddPlugin\('([^']+)'", re.MULTILINE)
PLUGIN_CHECK_RE = re.compile(r"IsPluginSelected\('([^']+)'\)", re.IGNORECASE)
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
OFFICIAL_RELEASE_PREFIX = "/KRtkovo-eu-AI/salamander-plugins/releases/download/"

# These IDs are the installer-facing IDs used by IsPluginSelected. Catalog IDs
# remain the public package IDs; all other entries map by exact ID.
INSTALLER_ID_ALIASES = {
    "event-viewer": "eventviewer",
    "extension-menu-builder": "extensionmenubuilder",
    "file-lock-inspector": "filelockinspector",
    "git-worktree-navigator": "gitworktreenavigator",
    "hardware-monitor": "hardwaremonitor",
    "json-viewer-nodejs": "jsonwebviewer",
    "process-explorer": "processexplorer",
}
EXTENSION_INSTALL_PATHS = {
    "event-viewer": "extensions\\event-viewer",
    "extension-menu-builder": "extensions\\extension-menu-builder",
    "file-lock-inspector": "extensions\\file-lock-inspector",
    "git-worktree-navigator": "extensions\\git-worktree-navigator",
    "hardware-monitor": "extensions\\hardware-monitor",
    "json-viewer-nodejs": "extensions\\json-viewer-nodejs",
    "salamatrixdemos": "extensions\\demos",
    "process-explorer": "extensions\\process-explorer",
}
RUNTIME_IDS = {
    "javascriptruntime",
    "luaruntime",
    "phpruntime",
    "powershellruntime",
    "pythonruntime",
}


def pascal_string(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def installer_ids() -> set[str]:
    text = INSTALLER_PATH.read_text(encoding="utf-8-sig")
    return {
        value.casefold()
        for value in ADD_PLUGIN_RE.findall(text) + PLUGIN_CHECK_RE.findall(text)
    }


def load_catalog_entries() -> list[dict[str, object]]:
    entries: list[dict[str, object]] = []
    for catalog_name in CATALOG_FILES:
        catalog_path = CATALOG_DIR / catalog_name
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        packages = catalog.get("plugins")
        if not isinstance(packages, list):
            raise ValueError(f"Catalog has no package list: {catalog_path}")
        for package in packages:
            if not isinstance(package, dict):
                raise ValueError(f"Invalid package entry in {catalog_path}")
            package["_catalog"] = catalog_name
            entries.append(package)
    return entries


def package_installer_id(package: dict[str, object], known_ids: set[str]) -> str | None:
    package_id = package.get("id")
    if not isinstance(package_id, str):
        return None
    installer_id = INSTALLER_ID_ALIASES.get(package_id, package_id)
    return installer_id if installer_id.casefold() in known_ids else None


def install_relative_path(package: dict[str, object]) -> str:
    package_id = str(package["id"])
    package_type = package.get("packageType")
    if package_type == "extension":
        relative = EXTENSION_INSTALL_PATHS.get(package_id)
        if relative is None:
            raise ValueError(f"No Inno destination mapping for extension package {package_id}")
        return relative
    if package_type != "plugin":
        raise ValueError(f"Unknown package type for {package_id}: {package_type!r}")
    if package_id in RUNTIME_IDS:
        return f"plugins\\extension-runtimes\\{package_id}"
    return f"plugins\\{package_id}"


def validate_download(package: dict[str, object]) -> tuple[str, str] | None:
    raw_hash = package.get("packageSha256")
    if not isinstance(raw_hash, str) or raw_hash == "":
        return None
    if not SHA256_RE.fullmatch(raw_hash):
        raise ValueError(f"Invalid packageSha256 for {package.get('id')}")

    raw_url = package.get("downloadPageUrl")
    if not isinstance(raw_url, str):
        raise ValueError(f"Missing downloadPageUrl for {package.get('id')}")
    url = urlparse(raw_url)
    if (url.scheme != "https" or url.hostname != "github.com" or
            not url.path.startswith(OFFICIAL_RELEASE_PREFIX)):
        raise ValueError(f"Non-official package URL for {package.get('id')}: {raw_url}")
    path_parts = [unquote(part) for part in url.path.split("/") if part]
    if len(path_parts) < 5:
        raise ValueError(f"Malformed package release URL for {package.get('id')}: {raw_url}")
    file_name = path_parts[-1]
    package_id = str(package.get("id", ""))
    latest = package.get("latestVersion")
    if not isinstance(latest, str) or not latest:
        raise ValueError(f"Missing latestVersion for {package_id}")
    version = latest.split(" (", 1)[0].replace(" ", "_")
    expected_suffix = f"_{version}_x64.7z"
    expected_prefix = f"plugin_5.0_{package_id}_"
    if (not file_name.casefold().startswith(expected_prefix.casefold()) or
            not file_name.casefold().endswith(expected_suffix.casefold()) or
            path_parts[-2].casefold() != file_name[:-3].casefold()):
        raise ValueError(
            f"Download URL version does not match latestVersion for {package_id}: "
            f"{file_name!r} vs {latest!r}"
        )
    return raw_url, raw_hash.lower()


def generate() -> str:
    known_ids = installer_ids()
    downloadable: dict[str, dict[str, object]] = {}
    dependency_pairs: set[tuple[str, str]] = set()
    catalog_errors: list[str] = []

    for package in load_catalog_entries():
        installer_id = package_installer_id(package, known_ids)
        if installer_id is None:
            continue
        key = installer_id.casefold()
        if key in downloadable:
            raise ValueError(f"Duplicate installer package mapping: {installer_id}")

        dependencies = package.get("dependencies", [])
        if dependencies is not None:
            if not isinstance(dependencies, list):
                raise ValueError(f"Invalid dependencies for {package.get('id')}")
            for dependency in dependencies:
                if not isinstance(dependency, str):
                    raise ValueError(f"Invalid dependency for {package.get('id')}")
                mapped_dependency = INSTALLER_ID_ALIASES.get(dependency, dependency)
                if mapped_dependency.casefold() not in known_ids:
                    raise ValueError(
                        f"Catalog dependency {dependency!r} for {package.get('id')} "
                        "has no installer component"
                    )
                dependency_pairs.add((installer_id, mapped_dependency))

        try:
            valid_download = validate_download(package)
        except ValueError as error:
            catalog_errors.append(str(error))
            continue
        if valid_download is None:
            continue
        url, sha256 = valid_download
        downloadable[key] = {
            "installer_id": installer_id,
            "package_id": str(package["id"]),
            "package_type": str(package.get("packageType", "")),
            "latest_version": str(package["latestVersion"]),
            "url": url,
            "sha256": sha256,
            "destination": install_relative_path(package),
        }

    if catalog_errors:
        raise ValueError("Invalid downloadable package catalog entries:\n- " + "\n- ".join(catalog_errors))

    lines = [
        "// | Date | Summary | Reason | Ticket | Model |",
        "// | 2026-11-14 | Generate the catalog-backed Inno web-package lookup. | Keep hashes, URLs, dependencies, and component mappings synchronized. | | openrouter/openai/gpt-6-luna-pro |",
        "// Generated by tools/catalogs/generate_inno_package_catalog.py; do not edit by hand.",
        "",
        "procedure AddGeneratedInstallerPackageDependencies;",
        "begin",
    ]
    for owner, dependency in sorted(dependency_pairs, key=lambda pair: (pair[0].casefold(), pair[1].casefold())):
        lines.append(
            f"  AddPluginDependency({pascal_string(owner)}, {pascal_string(dependency)});"
        )
    lines.extend(
        [
            "end;",
            "",
            "function GetInstallerWebPackage(const InstallerId: String; var PackageId, PackageType, PackageVersion, PackageUrl, PackageSha256, InstallRelativePath: String): Boolean;",
            "begin",
            "  Result := True;",
        ]
    )
    for key in sorted(downloadable):
        package = downloadable[key]
        lines.extend(
            [
                f"  if CompareText(InstallerId, {pascal_string(str(package['installer_id']))}) = 0 then",
                "  begin",
                f"    PackageId := {pascal_string(str(package['package_id']))};",
                f"    PackageType := {pascal_string(str(package['package_type']))};",
                f"    PackageVersion := {pascal_string(str(package['latest_version']))};",
                f"    PackageUrl := {pascal_string(str(package['url']))};",
                f"    PackageSha256 := {pascal_string(str(package['sha256']))};",
                f"    InstallRelativePath := {pascal_string(str(package['destination']))};",
                "    Exit;",
                "  end;",
            ]
        )
    lines.extend(
        [
            "  Result := False;",
            "end;",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT_PATH)
    parser.add_argument("--check", action="store_true", help="fail if the generated include is stale")
    args = parser.parse_args()
    expected = generate()
    output = args.output.resolve()
    if args.check:
        if not output.is_file() or output.read_text(encoding="utf-8") != expected:
            raise SystemExit(f"Generated Inno package catalog is stale: {output}")
        print(f"Inno package catalog is current: {output}")
        return 0
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(expected, encoding="utf-8", newline="\n")
    print(f"Generated Inno package catalog: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
