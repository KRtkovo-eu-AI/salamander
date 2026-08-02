#!/usr/bin/env python3
"""Generate source-code growth reports between adjacent release tags."""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from typing import Any

import report_code_growth as growth


def release(repository: Path, tag: str, prefix: str) -> dict[str, str]:
    pattern = re.compile(rf"^{re.escape(prefix)}(?P<version>\d+(?:\.\d+)*)$")
    match = pattern.fullmatch(tag)
    if match is None:
        raise ValueError(f"Release tag does not match {prefix}<numeric version>: {tag}")
    commit = str(growth.git(repository, "rev-list", "-n", "1", tag)).strip().lower()
    if not commit:
        raise ValueError(f"Release tag was not found: {tag}")
    return {"tag": tag, "version": match.group("version"), "commit": commit}


def report_name(before: dict[str, str], after: dict[str, str]) -> str:
    return f"{before['tag']}-to-{after['version']}.md"


def generate_reports(
    repository: Path, output_directory: Path, tags: list[str], prefix: str
) -> list[Path]:
    releases = [release(repository, tag, prefix) for tag in tags]
    lizard_module = growth.load_lizard()
    snapshots: list[dict[str, Any]] = []
    for item in releases:
        snapshots.append(
            growth.analyze_snapshot(
                growth.baseline_sources(repository, item["commit"]),
                item["tag"],
                lizard_module,
            )
        )

    output_directory.mkdir(parents=True, exist_ok=True)
    outputs: list[Path] = []
    for index in range(len(releases) - 1):
        before = releases[index]
        current_snapshot = snapshots[index + 1]
        report = {
            "schemaVersion": 1,
            "metric": "source-nloc",
            "lizardAvailable": lizard_module is not None,
            "baseline": before,
            "current": current_snapshot,
            "baselineSnapshot": snapshots[index],
        }
        report["comparison"] = growth.compare(snapshots[index], current_snapshot)
        output = output_directory / report_name(before, releases[index + 1])
        output.write_text(growth.markdown(report), encoding="utf-8")
        outputs.append(output)
        summary = report["comparison"]["summary"]
        print(
            f"[code-growth] {before['tag']} -> {releases[index + 1]['tag']}: "
            f"{summary['currentNloc']:,} NLOC ({summary['deltaNloc']:+,})"
        )
    return outputs


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("tags", nargs="+", help="Two or more adjacent release tags")
    parser.add_argument("--repository-root", type=Path, default=Path.cwd())
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--tag-prefix", default="5.0-samandarin-")
    args = parser.parse_args()
    if len(args.tags) < 2:
        parser.error("at least two release tags are required")

    generate_reports(
        args.repository_root.resolve(),
        args.output_directory.resolve(),
        args.tags,
        args.tag_prefix,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
