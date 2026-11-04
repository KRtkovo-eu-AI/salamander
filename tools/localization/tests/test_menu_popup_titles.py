"""Source-contract: menu POPUP titles must stay distinct after rebase/AI translation."""
from __future__ import annotations

import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
TRANSLATIONS = REPO_ROOT / "translations"
POPUP_RE = re.compile(r'^0,\d+,"(.*)"$')
MENU_RE = re.compile(r'^\[MENU (\d+)\]$')


def menu_popup_titles(text: str) -> dict[str, list[str]]:
    menus: dict[str, list[str]] = {}
    current: str | None = None
    for line in text.splitlines():
        header = MENU_RE.match(line.strip())
        if header:
            current = header.group(1)
            menus[current] = []
            continue
        if current is None:
            continue
        if line.startswith('['):
            current = None
            continue
        popup = POPUP_RE.match(line)
        if popup:
            menus[current].append(popup.group(1))
    return menus


class MenuPopupTitleTests(unittest.TestCase):
    def test_committed_menus_do_not_collapse_all_popups_to_one_title(self):
        failures = []
        for slt in sorted(TRANSLATIONS.glob('*/*.slt')):
            menus = menu_popup_titles(slt.read_text(encoding='utf-8-sig'))
            for menu_id, titles in menus.items():
                if len(titles) < 2:
                    continue
                if len(set(titles)) == 1:
                    failures.append(f'{slt.relative_to(REPO_ROOT)} MENU {menu_id}: {len(titles)}x {titles[0]!r}')
        self.assertEqual(failures, [], 'collapsed POPUP titles:\n' + '\n'.join(failures))

    def test_viewer_menu_has_expected_czech_top_level_titles(self):
        text = (TRANSLATIONS / 'czech' / 'salamand.slt').read_text(encoding='utf-8-sig')
        self.assertEqual(
            menu_popup_titles(text)['6010'],
            ['&Soubor', '&Jiné soubory', '&Upravit', '&Najít', '&Zobrazit', '&Možnosti', 'Ná&pověda'],
        )


if __name__ == '__main__':
    unittest.main()
