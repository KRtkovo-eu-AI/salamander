# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Naming helpers for the official Samandarin language pack (langpack.dll).

Resource names inside the pack are RT_RCDATA entries:
  LP_<LANGUAGE>_<MODULE>
e.g. LP_CZECH_SALAMAND, LP_GERMAN_ZIP

Language and module tokens are uppercased ASCII. The runtime lowers the language
base name again when enumerating.
"""

from __future__ import annotations

import re

LANGPACK_FILENAME = "langpack.dll"
LANGPACK_RC_PREFIX = "LP_"
LANGPACK_MODULE_SALAMAND = "salamand"

_TOKEN_RE = re.compile(r"^[A-Za-z0-9]+$")


def language_base_name(slg_name: str) -> str:
    name = slg_name.strip()
    if name.lower().endswith(".slg"):
        name = name[:-4]
    return name


def make_resource_name(language_base: str, module_key: str) -> str:
    lang = language_base_name(language_base).upper()
    module = module_key.strip().upper()
    if not _TOKEN_RE.match(lang):
        raise ValueError(f"invalid language token: {language_base!r}")
    if not _TOKEN_RE.match(module):
        raise ValueError(f"invalid module token: {module_key!r}")
    return f"{LANGPACK_RC_PREFIX}{lang}_{module}"


def parse_resource_name(resource_name: str, module_key: str) -> str | None:
    """Return lowercase language base if resource_name matches module_key."""
    prefix = LANGPACK_RC_PREFIX
    if not resource_name.upper().startswith(prefix):
        return None
    rest = resource_name[len(prefix) :]
    suffix = "_" + module_key.strip().upper()
    if not rest.upper().endswith(suffix):
        return None
    lang = rest[: -len(suffix)]
    if not lang or not _TOKEN_RE.match(lang):
        return None
    return lang.lower()


def discover_slg_modules(build_root) -> dict[str, dict[str, object]]:
    """Map module_key -> {language_base: Path} for non-english .slg under build_root."""
    from pathlib import Path

    root = Path(build_root)
    found: dict[str, dict[str, object]] = {}

    main_lang = root / "lang"
    if main_lang.is_dir():
        for slg in main_lang.glob("*.slg"):
            base = language_base_name(slg.name)
            if base.lower() == "english":
                continue
            found.setdefault(LANGPACK_MODULE_SALAMAND, {})[base.lower()] = slg

    plugins = root / "plugins"
    if plugins.is_dir():
        for slg in plugins.glob("*/lang/*.slg"):
            base = language_base_name(slg.name)
            if base.lower() == "english":
                continue
            module = slg.parent.parent.name.lower()
            found.setdefault(module, {})[base.lower()] = slg

    return found
