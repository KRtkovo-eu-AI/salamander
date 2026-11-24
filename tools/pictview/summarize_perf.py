#!/usr/bin/env python3
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-21 | Summarize anonymous PictView timings. | Compare exclusive phase costs without image identifiers. | | GPT-6 (Codex) |
"""Validate PictView timing CSV and print anonymous aggregate JSON (stdlib only)."""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import json
import math
from pathlib import Path
import statistics


FIELDS = (
    "pid,tid,sample,action,outcome,file_bytes,width,height,display_width,display_height,"
    "total_ms,navigation_ms,open_other_ms,decode_ms,exif_ms,transform_ms,scale_ms,paint_ms,"
    "other_ms,decode_attempts,scale_attempts,transform_attempts,draw_attempts,paint_calls"
).split(",")
PHASES = FIELDS[11:19]
ATTEMPTS = FIELDS[19:]
OUTCOMES = {
    "painted", "gdi_flush_failed", "superseded", "closed", "unsupported_kind",
    "open_failed", "post_failed", "canceled", "read_failed", "paint_failed",
    "command_skipped", "same_target", "source_busy", "no_target", "transform_failed",
}
TOLERANCE_MS = 0.02


def read_samples(path: Path) -> tuple[list[dict], float]:
    rows, seen, max_error = [], set(), 0.0
    with path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream, strict=True)
        if reader.fieldnames != FIELDS:
            raise ValueError("CSV header does not match the anonymous timing schema")
        for raw in reader:
            line = reader.line_num
            if None in raw or any(value is None or value == "" for value in raw.values()):
                raise ValueError(f"Incomplete CSV row at line {line}")
            row = {}
            for name, value in raw.items():
                if name in ("action", "outcome"):
                    row[name] = value
                    continue
                try:
                    number = float(value) if name.endswith("_ms") else int(value)
                except ValueError:
                    raise ValueError(f"Invalid numeric field {name} at line {line}") from None
                if not math.isfinite(number) or number < 0:
                    raise ValueError(f"Invalid numeric field {name} at line {line}")
                row[name] = number
            if row["action"] not in ("open", "navigation") or row["outcome"] not in OUTCOMES:
                raise ValueError(f"Unsupported action or outcome at line {line}")
            key = tuple(row[name] for name in ("pid", "tid", "sample"))
            if any(value == 0 for value in key) or key in seen:
                raise ValueError(f"Invalid or duplicate sample identity at line {line}")
            seen.add(key)
            error = abs(sum(row[name] for name in PHASES) - row["total_ms"])
            if error > TOLERANCE_MS + 1e-9:
                raise ValueError(f"Exclusive phase sum differs from total at line {line}")
            if row["outcome"] == "painted" and any(row[name] == 0 for name in ("width", "height", "display_width", "display_height")):
                raise ValueError(f"Successful sample has incomplete dimensions at line {line}")
            max_error = max(max_error, error)
            rows.append(row)
    if not rows:
        raise ValueError("CSV contains no complete samples")
    return rows, max_error


def summarize(rows: list[dict]) -> dict:
    painted = [row for row in rows if row["outcome"] == "painted"]
    values = sorted(row["total_ms"] for row in painted)
    total = sum(values)
    return {
        "samples": len(rows),
        "actions": dict(sorted(Counter(row["action"] for row in rows).items())),
        "outcomes": dict(sorted(Counter(row["outcome"] for row in rows).items())),
        "file_bytes_range": [min(row["file_bytes"] for row in rows), max(row["file_bytes"] for row in rows)] if rows else None,
        "attempts_total": {name: sum(row[name] for row in rows) for name in ATTEMPTS},
        "painted_samples": len(painted),
        "painted_total_ms": {
            "median": round(statistics.median(values), 3),
            "p95": round(values[math.ceil(len(values) * 0.95) - 1], 3),
            "mean": round(statistics.mean(values), 3),
        } if painted else None,
        "painted_phases": {
            name: {
                "mean_ms": round(statistics.mean(row[name] for row in painted), 3),
                "percent_total": round(sum(row[name] for row in painted) / total * 100, 3) if total else None,
            } for name in PHASES
        } if painted else None,
        "painted_attempts_mean": {
            name: round(statistics.mean(row[name] for row in painted), 3) for name in ATTEMPTS
        } if painted else None,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="Anonymous timing CSV")
    parser.add_argument("--navigation-only", action="store_true", help="Summarize navigation actions only")
    parser.add_argument("--output-json", type=Path, help="Also write the anonymous summary as JSON")
    args = parser.parse_args()
    try:
        rows, max_error = read_samples(args.csv)
    except (OSError, UnicodeError, csv.Error, ValueError, OverflowError) as error:
        # Never echo raw CSV values or input paths, including in error messages.
        message = str(error) if isinstance(error, ValueError) and not isinstance(error, UnicodeError) else "Cannot read a valid anonymous timing CSV"
        parser.error(message)
    selected = [row for row in rows if not args.navigation_only or row["action"] == "navigation"]
    groups = defaultdict(list)
    for row in selected:
        groups[tuple(sorted((row["width"], row["height"])))].append(row)
    report = {
        "schema_version": 1,
        "navigation_only": args.navigation_only,
        "validated_samples": len(rows),
        "phase_sum_tolerance_ms": TOLERANCE_MS,
        "max_phase_sum_error_ms": round(max_error, 6),
        "latency_population": "painted outcomes only",
        "p95_method": "nearest rank; ceil(0.95 * sample_count)",
        "overall": summarize(selected),
        "by_dimensions": [
            {"short_edge": dimensions[0], "long_edge": dimensions[1], **summarize(group)}
            for dimensions, group in sorted(groups.items())
        ],
    }
    text = json.dumps(report, indent=2, allow_nan=False) + "\n"
    if args.output_json:
        try:
            args.output_json.write_text(text, encoding="utf-8")
        except OSError:
            parser.error("Cannot write the anonymous JSON summary")
    print(text, end="")


if __name__ == "__main__":
    main()
