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


def fill_catalog(catalog: dict[str, Any], archives: dict[str, Path]) -> int:
    updated = 0
    for entry in catalog.get("plugins", []):
        name = archive_name_from_url(entry.get("downloadPageUrl"))
        if not name or name not in archives:
            continue
        entry["packageSha256"] = sha256_file(archives[name])
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
