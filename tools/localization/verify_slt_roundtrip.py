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


def parse_langid(path: Path) -> tuple[int, int] | None:
    in_translation = False
    for line_no, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        stripped = line.strip()
        if stripped == "[TRANSLATION]":
            in_translation = True
            continue
        if in_translation and stripped.startswith("["):
            return None
        if in_translation and stripped.startswith("LANGID,"):
            try:
                return line_no, int(stripped.split(",", 1)[1])
            except ValueError:
                return line_no, -1
    return None


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
    parser.add_argument("--expected-langid", type=int, help="Expected SLT LANGID for the language folder")
    args = parser.parse_args()

    source = parse(args.source)
    roundtrip = parse(args.roundtrip)
    mismatches: list[str] = []

    if args.expected_langid is not None:
        for label, path in (("source", args.source), ("roundtrip", args.roundtrip)):
            langid = parse_langid(path)
            if langid is None:
                mismatches.append(f"{path}: missing LANGID in [TRANSLATION] section")
            elif langid[1] != args.expected_langid:
                mismatches.append(f"{path}:{langid[0]}: {label} LANGID is {langid[1]}, expected {args.expected_langid}")

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
