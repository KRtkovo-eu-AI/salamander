# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-26 | Check shared author, web and comment on translated languages. | Select Language details must stay consistent across language archives. | https://github.com/KRtkovo-eu-AI/salamander/issues/857 | Grok 4.7 |
"""Header contract for shipped translation archives."""

from __future__ import annotations

from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
AUTHOR_SUFFIX = "Open Samandarin Authors"
PLACEHOLDERS = {
    "open samandarin",
    "open salamander",
    "open samandarin authors",
    "open salamander authors",
}


def _translation_header(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8-sig")
    section = ""
    found: dict[str, str] = {}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            if section == "TRANSLATION":
                break
            section = stripped[1:-1].split()[0]
            continue
        if section != "TRANSLATION":
            continue
        for key in ("AUTHOR", "WEB", "COMMENT"):
            prefix = key + ","
            if stripped.startswith(prefix) and key not in found:
                value = stripped[len(prefix) :]
                if len(value) < 2 or not value.startswith('"') or not value.endswith('"'):
                    raise AssertionError(f"{path} has a malformed {key} line: {stripped}")
                found[key] = value[1:-1]
    missing = [key for key in ("AUTHOR", "WEB", "COMMENT") if key not in found]
    if missing:
        raise AssertionError(f"{path} is missing {', '.join(missing)}")
    return found


def test_translated_language_headers_use_the_shared_details():
    archives = sorted((REPO / "translations").rglob("*.slt"))
    assert archives, "expected shipped translation archives"
    for path in archives:
        language = path.parent.name
        header = _translation_header(path)
        assert header["WEB"] == "open.samandarin.net", path
        assert header["COMMENT"] == f"{language} (automatic AI translation)", path
        assert '"' not in header["AUTHOR"], path
        parts = [part.strip() for part in header["AUTHOR"].split(",") if part.strip()]
        assert parts[-1] == AUTHOR_SUFFIX, path
        assert all(part.casefold() not in PLACEHOLDERS for part in parts[:-1]), path
