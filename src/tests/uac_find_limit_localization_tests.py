# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Check all typed Find omission messages. | Preserve every locale, resource block and format contract. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Check protected Find limit translations. | Keep every locale and the format argument consistent. | #731 | GPT-6.1 Terra |
"""Validate exact IDs, the protected-item placeholder and all shipped SLT locales."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCALES = {"chinesesimplified", "czech", "dutch", "french", "german",
           "hungarian", "italian", "romanian", "russian", "slovak", "spanish"}
IDS = {"IDS_UAC_FIND_SCOPE_LIMIT": 14700, "IDS_FIND_ACTIVITY_INCOMPLETE": 14701,
       "IDS_UAC_FIND_SCOPE_LIMIT_CARRIED": 14702,
       "IDS_FIND_ACTIVITY_INCOMPLETE_OTHER": 14704,
       "IDS_FIND_INCOMPLETE_DIRECTORY_CAPACITY": 14705,
       "IDS_FIND_INCOMPLETE_SCAN_BUDGET": 14706,
       "IDS_FIND_INCOMPLETE_DEPTH": 14707,
       "IDS_FIND_INCOMPLETE_REPARSE": 14708,
       "IDS_FIND_INCOMPLETE_PROTECTED_FAILURE": 14709}


class FindLimitLocalizationTests(unittest.TestCase):
    def test_resource_ids_and_english(self):
        header = (ROOT / "src/texts.rh2").read_text(encoding="utf-8-sig")
        resources = (ROOT / "src/lang/texts.rc2").read_text(encoding="utf-8-sig")
        for name, number in IDS.items():
            self.assertEqual(re.findall(r"^#define\s+(\w+)\s+" + str(number) + r"\s*$", header, re.M), [name])
            matches = re.findall(r"^\s*" + name + r',\s*"([^"\r\n]*)"\s*$', resources, re.M)
            self.assertEqual(len(matches), 1, name)
            if name == "IDS_FIND_ACTIVITY_INCOMPLETE":
                self.assertEqual(matches[0], "Incomplete (admin limit)")
            self.assertEqual(matches[0].count("%"), name in ("IDS_UAC_FIND_SCOPE_LIMIT", "IDS_UAC_FIND_SCOPE_LIMIT_CARRIED"), name)
            if name in ("IDS_UAC_FIND_SCOPE_LIMIT", "IDS_UAC_FIND_SCOPE_LIMIT_CARRIED"):
                self.assertEqual(matches[0].count("%s"), 1)

    def test_all_locales_and_placeholder(self):
        translations = ROOT / "translations"
        paths = {path.parent.name: path for path in translations.glob("*/salamand.slt")}
        self.assertEqual(set(paths), LOCALES)
        for locale, path in paths.items():
            with self.subTest(locale=locale):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf[EXPORTINFO]"))
                self.assertEqual(raw.count(b"\n"), raw.count(b"\r\n"))
                text = raw.decode("utf-8-sig")
                section = text.split("[STRINGTABLE 197]", 1)[1].split("[STRINGTABLE 198]", 1)[0]
                typed_section = text.split("[STRINGTABLE 198]", 1)[1].split("[STRINGTABLE 199]", 1)[0]
                headers = [int(n) for n in re.findall(r"^\[STRINGTABLE (\d+)\]$", text, re.M)]
                self.assertEqual(headers, list(range(len(headers))))
                for number in IDS.values():
                    pattern = r'^' + str(number) + r',1,"([^"\r\n]+)"\s*$'
                    rows = re.findall(pattern, text, re.M)
                    self.assertEqual(len(rows), 1, (locale, number))
                    if locale == "czech" and number == 14701:
                        self.assertEqual(rows[0], "Neúplné (limit správce)")
                    expected_section = typed_section if number >= 14704 else section
                    self.assertEqual(len(re.findall(pattern, expected_section, re.M)), 1, (locale, number))
                    self.assertEqual(rows[0].count("%"), number in (14700, 14702), (locale, number))
                    if number in (14700, 14702):
                        self.assertEqual(rows[0].count("%s"), 1, locale)


if __name__ == "__main__":
    unittest.main()
