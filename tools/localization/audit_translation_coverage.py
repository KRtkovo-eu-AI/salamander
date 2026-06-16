#!/usr/bin/env python3
"""Report which localizable Samandarin modules have committed SLT archives."""
from __future__ import annotations
import argparse
from pathlib import Path


def discover_modules(root: Path) -> set[str]:
    modules = {"salamand"}
    setup_inf = root / "tools" / "setup_x64.inf"
    for line in setup_inf.read_text(encoding="utf-8", errors="ignore").splitlines():
        marker = "\\plugins\\"
        lowered = line.lower()
        if marker in lowered and "\\lang\\%slg" in lowered:
            modules.add(lowered.split(marker, 1)[1].split("\\", 1)[0])
    return modules


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--fail-on-missing", action="store_true", help="fail if any language lacks any current module archive")
    args = parser.parse_args()
    root = args.repo_root.resolve()
    modules = discover_modules(root)
    translations = root / "translations"
    languages = sorted(path for path in translations.iterdir() if path.is_dir())
    print(f"Current localizable modules ({len(modules)}): {', '.join(sorted(modules))}")
    any_missing = False
    for language in languages:
        archived = {path.stem.lower() for path in language.glob("*.slt")}
        missing = sorted(modules - archived)
        stale = sorted(archived - modules)
        any_missing |= bool(missing)
        print(f"{language.name}: {len(archived & modules)}/{len(modules)} current modules")
        if missing:
            print(f"  missing: {', '.join(missing)}")
        if stale:
            print(f"  archived-only: {', '.join(stale)}")
    return 1 if args.fail_on_missing and any_missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
