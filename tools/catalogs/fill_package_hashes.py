#!/usr/bin/env python3
"""Fill catalog packageSha256 values from local .7z archives.

Hashes are taken from files, not from GitHub release notes. Archive names are
matched against catalog downloadPageUrl file names.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CATALOGS = ROOT / "doc" / "catalogs-base"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def archive_name_from_url(url: str | None) -> str | None:
    if not url:
        return None
    name = unquote(Path(urlparse(url).path).name)
    return name.casefold() if name else None


def index_archives(directory: Path) -> dict[str, Path]:
    return {path.name.casefold(): path for path in directory.glob("*.7z")}


def archive_name_candidates(package_id: str, archives: dict[str, Path]) -> list[str]:
    prefix = f"plugin_5.0_{package_id}_"
    suffix = "_x64.7z"
    return [
        name
        for name in archives
        if name.startswith(prefix) and name.endswith(suffix)
    ]


def archive_version(name: str, package_id: str) -> str | None:
    prefix = f"plugin_5.0_{package_id}_"
    suffix = "_x64.7z"
    if not name.startswith(prefix) or not name.endswith(suffix):
        return None
    return name[len(prefix) : -len(suffix)]


def fill_catalog(catalog: dict[str, Any], archives: dict[str, Path]) -> int:
    updated = 0
    for entry in catalog.get("plugins", []):
        name = archive_name_from_url(entry.get("downloadPageUrl"))
        package_id = entry.get("id")
        if not isinstance(package_id, str) or not package_id:
            continue

        candidates = archive_name_candidates(package_id, archives)
        latest_version = entry.get("latestVersion")
        if isinstance(latest_version, str):
            latest_version = latest_version.split(" (", 1)[0].casefold()
            latest_version = latest_version.replace(" ", "_")
            version_matches = [
                candidate
                for candidate in candidates
                if archive_version(candidate, package_id) == latest_version
            ]
            if len(version_matches) == 1:
                candidates = version_matches

        archive = archives.get(name) if name else None
        if len(candidates) == 1:
            archive = archives[candidates[0]]
        if archive is None:
            continue
        entry["packageSha256"] = sha256_file(archive)
        updated += 1
    return updated


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sources = parser.add_mutually_exclusive_group(required=True)
    sources.add_argument("--archives", type=Path, help="directory of official .7z packages")
    sources.add_argument("--archive", type=Path, help="single official .7z package")
    parser.add_argument("--catalogs", type=Path, default=DEFAULT_CATALOGS)
    args = parser.parse_args()

    if args.archive is not None:
        archive = args.archive.resolve()
        if not archive.is_file():
            raise SystemExit(f"Archive not found: {archive}")
        archives = {archive.name.casefold(): archive}
    else:
        archives = index_archives(args.archives)
        if not archives:
            raise SystemExit(f"No .7z archives found in {args.archives}")

    total = 0
    for path in sorted(args.catalogs.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data.get("plugins"), list):
            continue
        updated = fill_catalog(data, archives)
        if updated:
            path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            total += updated
            print(f"{path.name}: filled {updated} hash(es)")
    print(f"Filled {total} packageSha256 value(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
