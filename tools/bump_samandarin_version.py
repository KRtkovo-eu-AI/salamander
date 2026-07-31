#!/usr/bin/env python3
"""Bump the Samandarin version and matching configuration metadata.

Run from the repository root (or anywhere inside the repository):

    python tools/bump_samandarin_version.py

The script increments the Samandarin minor version by one and updates the
registry/configuration metadata that must move in lockstep with a release.
"""

from __future__ import annotations

import re
import subprocess
from pathlib import Path


def repo_root() -> Path:
    output = subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True)
    return Path(output.strip())


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", newline="")


def replace_one(text: str, pattern: str, replacement: str, *, file_name: str) -> str:
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise RuntimeError(f"Expected exactly one match for {pattern!r} in {file_name}, found {count}.")
    return updated


def main() -> None:
    root = repo_root()

    spl_vers = root / "src/plugins/shared/spl_vers.h"
    consts = root / "src/consts.h"
    mainwnd2 = root / "src/mainwnd2.cpp"
    inno = root / "doc/runbook-setup/inno_setup_salamander_x64.iss"

    spl_text = read(spl_vers)
    match = re.search(r"^#define VERSINFO_SAMANDARIN_MAJOR (\d+)\n#define VERSINFO_SAMANDARIN_MINORA (\d+)$", spl_text, re.MULTILINE)
    if not match:
        raise RuntimeError(f"Could not find Samandarin version defines in {spl_vers}.")

    major = int(match.group(1))
    old_minor = int(match.group(2))
    new_minor = old_minor + 1
    old_version = f"{major}.{old_minor}"
    new_version = f"{major}.{new_minor}"

    build_match = re.search(r"^#define VERSINFO_BUILDNUMBER (\d+)$", spl_text, re.MULTILINE)
    if not build_match:
        raise RuntimeError(f"Could not find VERSINFO_BUILDNUMBER in {spl_vers}.")
    old_build_number = int(build_match.group(1))
    new_build_number = old_build_number + 1

    spl_text = replace_one(
        spl_text,
        r"^(#define VERSINFO_SAMANDARIN_MINORA )\d+$",
        rf"\g<1>{new_minor}",
        file_name=str(spl_vers),
    )
    spl_text = replace_one(
        spl_text,
        rf"^(// {old_build_number} = 5\.0-samandarin-{re.escape(old_version)}\n)",
        rf"\g<1>// {new_build_number} = 5.0-samandarin-{new_version}\n",
        file_name=str(spl_vers),
    )
    spl_text = replace_one(
        spl_text,
        r"^(#define VERSINFO_BUILDNUMBER )\d+$",
        rf"\g<1>{new_build_number}",
        file_name=str(spl_vers),
    )
    write(spl_vers, spl_text)

    consts_text = read(consts)
    count_match = re.search(r"^#define SALCFG_ROOTS_COUNT (\d+)$", consts_text, re.MULTILINE)
    if not count_match:
        raise RuntimeError(f"Could not find SALCFG_ROOTS_COUNT in {consts}.")
    old_roots_count = int(count_match.group(1))
    consts_text = replace_one(
        consts_text,
        r"^(#define SALCFG_ROOTS_COUNT )\d+$",
        rf"\g<1>{old_roots_count + 1}",
        file_name=str(consts),
    )
    write(consts, consts_text)

    main_text = read(mainwnd2)
    config_match = re.search(r"^const DWORD THIS_CONFIG_VERSION = (\d+);$", main_text, re.MULTILINE)
    if not config_match:
        raise RuntimeError(f"Could not find THIS_CONFIG_VERSION in {mainwnd2}.")
    old_config_version = int(config_match.group(1))
    new_config_version = old_config_version + 1

    config_history_match = re.search(
        rf"^// {old_config_version} = 5\.0-samandarin-([^\s]+).*$",
        main_text,
        re.MULTILINE,
    )
    if not config_history_match:
        raise RuntimeError(f"Could not find configuration history entry {old_config_version} in {mainwnd2}.")
    old_config_version_name = config_history_match.group(1)

    main_text = replace_one(
        main_text,
        rf"^(// {old_config_version} = 5\.0-samandarin-{re.escape(old_config_version_name)}.*\n)",
        rf"\g<1>// {new_config_version} = 5.0-samandarin-{new_version}\n",
        file_name=str(mainwnd2),
    )
    main_text = replace_one(
        main_text,
        r"^(const DWORD THIS_CONFIG_VERSION = )\d+(;)$",
        rf"\g<1>{new_config_version}\2",
        file_name=str(mainwnd2),
    )
    main_text = replace_one(
        main_text,
        rf"^(\s*)\"Software\\\\Open Salamander Samandarin\\\\5\.0-samandarin-{re.escape(old_config_version_name)}\",$",
        rf'\g<1>"Software\\\\Open Salamander Samandarin\\\\5.0-samandarin-{new_version}",\n\g<0>',
        file_name=str(mainwnd2),
    )
    main_text = replace_one(
        main_text,
        rf"^(\s*)\"5\.0 Samandarin {re.escape(old_config_version_name)}\",$",
        rf'\g<1>"5.0 Samandarin {new_version}",\n\g<0>',
        file_name=str(mainwnd2),
    )
    write(mainwnd2, main_text)

    inno_text = read(inno)
    inno_text = replace_one(
        inno_text,
        r'^(#define SamandarinVersion ")\d+\.\d+("$)',
        rf"\g<1>{new_version}\2",
        file_name=str(inno),
    )
    write(inno, inno_text)

    print(f"Bumped Samandarin {old_version} -> {new_version}")
    print(f"Bumped VERSINFO_BUILDNUMBER {old_build_number} -> {new_build_number}")
    print(f"Bumped THIS_CONFIG_VERSION {old_config_version} -> {new_config_version}")
    print(f"Bumped SALCFG_ROOTS_COUNT {old_roots_count} -> {old_roots_count + 1}")


if __name__ == "__main__":
    main()
