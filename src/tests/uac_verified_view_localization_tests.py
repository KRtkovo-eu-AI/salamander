# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Check verified-result viewer messages. | Keep consent and handler limitations localized without placeholders. | #731 | GPT-6.1 Terra |
"""Check verified-result F3 resources in the source and every shipped locale."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCALES = {"chinesesimplified", "czech", "dutch", "french", "german",
           "hungarian", "italian", "romanian", "russian", "slovak", "spanish"}
IDS = {"IDS_UAC_VIEW_IDENTITY_HANDLER_UNSUPPORTED": 14710, "IDS_UAC_VIEW_VERIFIED_CONFIRM": 14711}


class VerifiedViewLocalizationTests(unittest.TestCase):
    def test_resources(self):
        header = (ROOT / "src/texts.rh2").read_text(encoding="utf-8-sig")
        resources = (ROOT / "src/lang/texts.rc2").read_text(encoding="utf-8-sig")
        for name, number in IDS.items():
            self.assertEqual(re.findall(r"^#define\s+(\w+)\s+" + str(number) + r"\s*$", header, re.M), [name])
            rows = re.findall(r"^\s*" + name + r',\s*"([^"\r\n]+)"\s*$', resources, re.M)
            self.assertEqual(len(rows), 1, name)
            self.assertNotIn("%", rows[0])
            if number == 14711:
                self.assertIn("16 MB", rows[0])
                self.assertNotIn("denied", rows[0].lower())

    def test_locales_and_resource_blocks(self):
        paths = {path.parent.name: path for path in (ROOT / "translations").glob("*/salamand.slt")}
        self.assertEqual(set(paths), LOCALES)
        for locale, path in paths.items():
            with self.subTest(locale=locale):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf[EXPORTINFO]"))
                self.assertEqual(raw.count(b"\n"), raw.count(b"\r\n"))
                text = raw.decode("utf-8-sig")
                section = text.split("[STRINGTABLE 198]", 1)[1].split("[STRINGTABLE 199]", 1)[0]
                headers = [int(n) for n in re.findall(r"^\[STRINGTABLE (\d+)\]$", text, re.M)]
                self.assertEqual(headers, list(range(len(headers))))
                for number in IDS.values():
                    pattern = r'^' + str(number) + r',1,"([^"\r\n]+)"\s*$'
                    rows = re.findall(pattern, text, re.M)
                    self.assertEqual(len(rows), 1, (locale, number))
                    self.assertEqual(len(re.findall(pattern, section, re.M)), 1, (locale, number))
                    self.assertNotIn("%", rows[0])
                    if number == 14711:
                        self.assertIn("16", rows[0])


if __name__ == "__main__":
    unittest.main()
