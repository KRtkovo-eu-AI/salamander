# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Source-contract checks for the English-in-host + langpack.dll localization model."""

from __future__ import annotations

from pathlib import Path

REPO = Path(__file__).resolve().parents[3]


def test_langpack_runtime_sources_exist():
    assert (REPO / "src" / "langpack.h").is_file()
    assert (REPO / "src" / "langpack.cpp").is_file()
    text = (REPO / "src" / "langpack.h").read_text(encoding="utf-8", errors="replace")
    assert 'SAL_LANGPACK_FILENAME "langpack.dll"' in text
    assert "SalLoadLanguageForModule" in text
    assert "SalIsEnglishLanguageName" in text


def test_host_embeds_english_lang_rc():
    sal = (REPO / "src" / "vcxproj" / "salamand.vcxproj").read_text(encoding="utf-8", errors="replace")
    assert "lang\\lang.rc" in sal or "lang/lang.rc" in sal
    assert "SKIP_LANG_VERSIONINFO" in sal
    assert "langpack.cpp" in sal

    props = (REPO / "src" / "plugins" / "shared" / "vcxproj" / "plugin_base.props").read_text(
        encoding="utf-8", errors="replace"
    )
    assert "EmbedEnglishLangRc" in props
    assert "SKIP_LANG_VERSIONINFO" in props


def test_versinfo_can_skip_when_embedding():
    vers = (REPO / "src" / "plugins" / "shared" / "versinfo.rc2").read_text(encoding="utf-8", errors="replace")
    assert "SKIP_LANG_VERSIONINFO" in vers


def test_inno_ships_optional_langpack_not_per_language_slg():
    iss = (REPO / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss").read_text(
        encoding="utf-8", errors="replace"
    )
    assert r"lang\langpack.dll" in iss
    assert "additionallanguages" in iss
    assert r"lang\czech.slg" not in iss
    assert r"plugins\zip\lang\*.slg" not in iss


def test_official_langpack_builder_exists():
    assert (REPO / "tools" / "localization" / "build_official_langpack.ps1").is_file()
    build = (REPO / "tools" / "localization" / "build_language_packs.ps1").read_text(
        encoding="utf-8", errors="replace"
    )
    assert "build_official_langpack.ps1" in build
