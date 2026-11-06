# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-06 | Apply JSON translation maps onto SLT archives. | Manual/batch translators need a safe way to set state=1 texts without breaking escapes. | | Composer |
# | 2026-11-06 | Allow list values for duplicate MENU resource ids. | Popup titles sharing id 0 need distinct translations in order. | | Composer |
# | 2026-11-06 | Support list-valued keys for duplicate MENU resource IDs. | Popup menu headers reuse id 0 with different captions in one section. | | Composer |
#!/usr/bin/env python3
"""Apply a JSON map of translations onto an SLT archive.

JSON format:
{
  "[STRINGTABLE 0]:1011": "Ścieżka",
  "[DIALOG 100]:1150": "&Zaznacz pliki:",
  "[MENU 600]:0": ["\\x10&Plik", "&Edycja", "&Widok", "&Opcje", "Pomo&c"],
  ...
}

Keys are section:resource_id (first CSV field). Values are already SLT-escaped
texts (use \\n for newlines, \\\\ for backslash). Empty-string entries are
kept as state=1.

When the same resource id appears more than once in a section (common for
MENU popup headers with id 0), the value may be a list of strings applied
in order to successive occurrences. A plain string still applies to every
occurrence (backward compatible).

A JSON list of {"key","text"} objects is also accepted; duplicate keys are
applied in list order to successive matching occurrences.
"""
from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict, deque
from pathlib import Path

LINE_RE = re.compile(r'^(?P<prefix>.*?,)(?P<state>[01]),"(?P<text>.*)"(?P<ending>\r?\n)?$')
SECTION_RE = re.compile(r"^\[(?P<kind>DIALOG|MENU|STRINGTABLE)(?:\s+(?P<id>\d+))?\]$")


def normalize_mapping(raw: dict | list) -> dict[str, deque[str] | str]:
    """Return key -> string (reuse) or deque (consume in order)."""
    if isinstance(raw, list):
        queues: dict[str, deque[str]] = defaultdict(deque)
        for item in raw:
            queues[item["key"]].append(item["text"])
        return dict(queues)
    mapping: dict[str, deque[str] | str] = {}
    for key, value in raw.items():
        if isinstance(value, list):
            mapping[key] = deque(value)
        else:
            mapping[key] = value
    return mapping


def take_translation(slot: deque[str] | str) -> str | None:
    if isinstance(slot, deque):
        if not slot:
            return None
        return slot.popleft()
    return slot


def apply(path: Path, raw_mapping: dict | list) -> dict[str, int]:
    mapping = normalize_mapping(raw_mapping)
    text = path.read_text(encoding="utf-8-sig")
    ending = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    section = ""
    stats = {"applied": 0, "missing_keys": 0, "unchanged": 0}
    for index, line in enumerate(lines):
        header = SECTION_RE.match(line.strip())
        if header:
            section = line.strip()
            continue
        match = LINE_RE.match(line)
        if not match or not section:
            continue
        resource_id = match.group("prefix").split(",", 1)[0]
        key = f"{section}:{resource_id}"
        if key not in mapping:
            if match.group("state") == "0" and match.group("text"):
                stats["missing_keys"] += 1
            else:
                stats["unchanged"] += 1
            continue
        translated = take_translation(mapping[key])
        if translated is None:
            if match.group("state") == "0" and match.group("text"):
                stats["missing_keys"] += 1
            else:
                stats["unchanged"] += 1
            continue
        state = "1"
        line_ending = match.group("ending") or ending
        lines[index] = f'{match.group("prefix")}{state},"{translated}"{line_ending}'
        stats["applied"] += 1
    path.write_text("".join(lines), encoding="utf-8-sig", newline="")
    return stats


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("slt", type=Path)
    parser.add_argument("json_map", type=Path)
    args = parser.parse_args()
    mapping = json.loads(args.json_map.read_text(encoding="utf-8"))
    stats = apply(args.slt, mapping)
    print(json.dumps(stats))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
