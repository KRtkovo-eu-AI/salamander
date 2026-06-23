#!/usr/bin/env python3
"""Verify that importing and re-exporting an SLT archive preserved translations.

The language-pack build uses Translator as the authority for producing .slg
resources.  This check compares the committed/source SLT with a fresh SLT export
from the generated .slg so encoding/code-page regressions are caught before a
broken language pack is copied into the runtime tree.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SECTION_RE = re.compile(r"^\[(?P<section>[^]]+)\]$")
LINE_RE = re.compile(r'^(?P<prefix>.*?,)(?P<state>[01]),"(?P<text>.*)"\r?\n?$')
MOJIBAKE_RE = re.compile(r'[ÃÂÅ][\u0080-\u00BF\u00A0-\u00BF\u0100-\u017F\u2122]')
REPLACEMENT_CHARS = {"\ufffd", "\u25a1", "\u25a0"}


def parse(path: Path) -> dict[str, tuple[int, str]]:
    section = ""
    items: dict[str, tuple[int, str]] = {}
    for line_no, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(keepends=True), 1):
        header = SECTION_RE.match(line.rstrip("\r\n"))
        if header:
            section = header.group("section")
            continue
        if not section:
            continue
        match = LINE_RE.match(line)
        if not match:
            continue
        key = f"{section}:{match.group('prefix')}"
        items[key] = (line_no, match.group("text"))
    return items


def validate_text(path: Path, line_no: int, text: str) -> list[str]:
    errors = []
    if any(ch in text for ch in REPLACEMENT_CHARS):
        errors.append(f"{path}:{line_no}: text contains a replacement/box glyph: {text}")
    if MOJIBAKE_RE.search(text):
        errors.append(f"{path}:{line_no}: text looks mojibaked: {text}")
    if "??" in text:
        errors.append(f"{path}:{line_no}: text contains repeated question marks: {text}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Source/committed SLT archive")
    parser.add_argument("roundtrip", type=Path, help="SLT exported back from generated SLG")
    parser.add_argument("--max-examples", type=int, default=20)
    args = parser.parse_args()

    source = parse(args.source)
    roundtrip = parse(args.roundtrip)
    mismatches: list[str] = []

    for _, (line_no, source_text) in source.items():
        mismatches.extend(validate_text(args.source, line_no, source_text))
    for _, (line_no, roundtrip_text) in roundtrip.items():
        mismatches.extend(validate_text(args.roundtrip, line_no, roundtrip_text))

    for key, (line_no, source_text) in source.items():
        if key not in roundtrip:
            mismatches.append(f"{args.source}:{line_no}: missing in roundtrip: {key}")
            continue
        roundtrip_text = roundtrip[key][1]
        if source_text != roundtrip_text:
            mismatches.append(
                f"{args.source}:{line_no}: text changed after import/export for {key}\n"
                f"  source:    {source_text}\n"
                f"  roundtrip: {roundtrip_text}"
            )

    for key, (line_no, _) in roundtrip.items():
        if key not in source:
            mismatches.append(f"{args.roundtrip}:{line_no}: extra roundtrip item: {key}")

    if mismatches:
        for mismatch in mismatches[: args.max_examples]:
            print(mismatch, file=sys.stderr)
        if len(mismatches) > args.max_examples:
            print(f"... {len(mismatches) - args.max_examples} more mismatch(es)", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
