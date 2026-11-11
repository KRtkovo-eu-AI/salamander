# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Check protected Find viewer identity explanations. | Preserve the F3 choices and retry guidance in all locales. | #731 | GPT-6.1 Terra |
"""Check protected Find identity messages in the source and every shipped locale."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCALES = {"chinesesimplified", "czech", "dutch", "french", "german",
           "hungarian", "italian", "romanian", "russian", "slovak", "spanish"}
IDS = {"IDS_UAC_FIND_VIEW_IDENTITY_UNAVAILABLE": 14712, "IDS_UAC_FIND_VIEW_WITH_UNAVAILABLE": 14713,
       "IDS_UAC_FIND_REFINE_IDENTITY_UNAVAILABLE": 14714}
ENGLISH = {14712: "The verified identity of this protected result is unavailable. The file cannot be opened safely; try running the search again.",
           14713: "View With cannot yet safely preserve the verified identity of this protected result. Use F3 instead.",
           14714: "Content and tags of this protected result cannot currently be read safely using its previously verified identity during refinement. This item was not verified."}


class FindViewIdentityLocalizationTests(unittest.TestCase):
    def test_resources(self):
        header = (ROOT / "src/texts.rh2").read_text(encoding="utf-8-sig")
        resources = (ROOT / "src/lang/texts.rc2").read_text(encoding="utf-8-sig")
        for name, number in IDS.items():
            self.assertEqual(re.findall(r"^#define\s+(\w+)\s+" + str(number) + r"\s*$", header, re.M), [name])
            rows = re.findall(r"^\s*" + name + r',\s*"([^"\r\n]+)"\s*$', resources, re.M)
            self.assertEqual(rows, [ENGLISH[number]])
            self.assertNotIn("%", rows[0])

    def test_all_locales(self):
        paths = {path.parent.name: path for path in (ROOT / "translations").glob("*/salamand.slt")}
        self.assertEqual(set(paths), LOCALES)
        for locale, path in paths.items():
            with self.subTest(locale=locale):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf[EXPORTINFO]"))
                self.assertEqual(raw.count(b"\n"), raw.count(b"\r\n"))
                text = raw.decode("utf-8-sig")
                section = text.split("[STRINGTABLE 198]", 1)[1].split("[STRINGTABLE 199]", 1)[0]
                for number in IDS.values():
                    pattern = r'^' + str(number) + r',1,"([^"\r\n]+)"\s*$'
                    rows = re.findall(pattern, text, re.M)
                    self.assertEqual(len(rows), 1, (locale, number))
                    self.assertEqual(len(re.findall(pattern, section, re.M)), 1, (locale, number))
                    self.assertNotIn("%", rows[0])
                    if number == 14713:
                        self.assertIn("F3", rows[0])
                    if locale == "czech" and number == 14712:
                        self.assertIn("zkuste hledání zopakovat", rows[0])
                    if locale == "czech" and number == 14714:
                        self.assertIn("Tato položka nebyla ověřena", rows[0])


if __name__ == "__main__":
    unittest.main()
