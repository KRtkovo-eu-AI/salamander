# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from __future__ import annotations

import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from langpack_format import (  # noqa: E402
    LANGPACK_FILENAME,
    LANGPACK_MODULE_SALAMAND,
    discover_slg_modules,
    language_base_name,
    make_resource_name,
    parse_resource_name,
)


def test_language_base_name_strips_slg():
    assert language_base_name("czech.slg") == "czech"
    assert language_base_name("CZECH.SLG") == "CZECH"
    assert language_base_name("english") == "english"


def test_make_resource_name():
    assert make_resource_name("czech", "salamand") == "LP_CZECH_SALAMAND"
    assert make_resource_name("ChineseSimplified.slg", "zip") == "LP_CHINESESIMPLIFIED_ZIP"


def test_parse_resource_name():
    assert parse_resource_name("LP_CZECH_SALAMAND", "salamand") == "czech"
    assert parse_resource_name("LP_GERMAN_ZIP", "zip") == "german"
    assert parse_resource_name("LP_GERMAN_ZIP", "salamand") is None
    assert parse_resource_name("OTHER", "zip") is None


def test_make_resource_name_rejects_bad_tokens():
    with pytest.raises(ValueError):
        make_resource_name("czech-extra", "salamand")
    with pytest.raises(ValueError):
        make_resource_name("czech", "zip/extra")


def test_discover_slg_modules(tmp_path: Path):
    (tmp_path / "lang").mkdir()
    (tmp_path / "lang" / "english.slg").write_bytes(b"MZ")
    (tmp_path / "lang" / "czech.slg").write_bytes(b"MZ")
    plugin_lang = tmp_path / "plugins" / "zip" / "lang"
    plugin_lang.mkdir(parents=True)
    (plugin_lang / "english.slg").write_bytes(b"MZ")
    (plugin_lang / "german.slg").write_bytes(b"MZ")

    found = discover_slg_modules(tmp_path)
    assert LANGPACK_MODULE_SALAMAND in found
    assert "czech" in found[LANGPACK_MODULE_SALAMAND]
    assert "english" not in found[LANGPACK_MODULE_SALAMAND]
    assert "zip" in found
    assert "german" in found["zip"]
    assert LANGPACK_FILENAME == "langpack.dll"
