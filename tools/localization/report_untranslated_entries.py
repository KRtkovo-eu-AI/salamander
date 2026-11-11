# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Add a report for untranslated SLT entries. | Identify languages and archives that still contain state=0 records after batch translation. | | openai/gpt-5.6-luna-pro |
#!/usr/bin/env python3
"""Report untranslated entries in committed SLT translation archives."""
from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass(frozen=True)
class ArchiveReport:
    language: str
    file: str
    untranslated: int
    total_records: int
    first_lines: list[int]


def count_records(path: Path) -> tuple[int, int, list[int]]:
    total_records = 0
    untranslated = 0
    first_lines: list[int] = []

    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        for line_number, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line or line.startswith("["):
                continue

            try:
                fields = next(csv.reader([line]))
            except csv.Error as error:
                raise ValueError(f"cannot parse CSV at line {line_number}: {error}") from error

            # SLT resource rows end with <state>,<translated text>. Metadata
            # rows such as AUTHOR or LANGID do not have a numeric state field.
            if len(fields) < 2 or fields[-2] not in {"0", "1"}:
                continue

            total_records += 1
            if fields[-2] == "0":
                untranslated += 1
                if len(first_lines) < 10:
                    first_lines.append(line_number)

    return untranslated, total_records, first_lines


def collect_reports(translations_dir: Path) -> tuple[list[ArchiveReport], int]:
    reports: list[ArchiveReport] = []
    malformed = 0

    for language_dir in sorted(path for path in translations_dir.iterdir() if path.is_dir()):
        for archive in sorted(language_dir.glob("*.slt")):
            try:
                untranslated, total_records, first_lines = count_records(archive)
            except (OSError, ValueError) as error:
                print(f"warning: {archive}: {error}", file=sys.stderr)
                malformed += 1
                continue

            if untranslated:
                reports.append(
                    ArchiveReport(
                        language=language_dir.name,
                        file=archive.name,
                        untranslated=untranslated,
                        total_records=total_records,
                        first_lines=first_lines,
                    )
                )

    return reports, malformed


def print_text_report(reports: list[ArchiveReport], malformed: int) -> None:
    languages = sorted({report.language for report in reports})
    total_untranslated = sum(report.untranslated for report in reports)
    total_archives = len(reports)

    print(f"Languages with state=0: {len(languages)}")
    print(f"Archives with state=0: {total_archives}")
    print(f"Untranslated records (state=0): {total_untranslated}")
    if malformed:
        print(f"Archives with parse errors: {malformed}")

    if not reports:
        print("No state=0 records found.")
        return

    print()
    for language in languages:
        language_reports = [report for report in reports if report.language == language]
        language_total = sum(report.untranslated for report in language_reports)
        print(f"{language}: {language_total} untranslated record(s) in {len(language_reports)} file(s)")
        for report in language_reports:
            lines = ",".join(str(line) for line in report.first_lines)
            print(
                f"  {report.file}: {report.untranslated} state=0 record(s)"
                f"/{report.total_records} total, first lines: {lines}"
            )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Report SLT archives and records whose translation state is 0."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repository root (default: inferred from this script)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="write machine-readable JSON instead of the text report",
    )
    parser.add_argument(
        "--fail-on-untranslated",
        action="store_true",
        help="return exit code 1 when any state=0 record is found",
    )
    args = parser.parse_args()

    translations_dir = args.repo_root.resolve() / "translations"
    if not translations_dir.is_dir():
        parser.error(f"translations directory not found: {translations_dir}")

    reports, malformed = collect_reports(translations_dir)
    if args.json:
        payload = {
            "languages_with_state0": sorted({report.language for report in reports}),
            "archives_with_state0": len(reports),
            "untranslated_records": sum(report.untranslated for report in reports),
            "parse_errors": malformed,
            "archives": [asdict(report) for report in reports],
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print_text_report(reports, malformed)

    if args.fail_on_untranslated and (reports or malformed):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
