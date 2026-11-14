# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Check the protected directory Recycle Bin explanation. | Keep all locales and the placeholder-free message aligned. | #731 | GPT-6.1 Terra |
"""Validate the protected directory Recycle Bin message in every shipped locale."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCALES = {"chinesesimplified", "czech", "dutch", "french", "german",
           "hungarian", "italian", "romanian", "russian", "slovak", "spanish"}
RESOURCE = "IDS_UAC_RECYCLE_DIRECTORY_UNSUPPORTED"
NUMBER = 14703
ENGLISH = "Moving a protected folder to the Recycle Bin with administrator privileges is not supported yet."


class RecycleDirectoryLocalizationTests(unittest.TestCase):
    def test_resource_id_and_english(self):
        header = (ROOT / "src/texts.rh2").read_text(encoding="utf-8-sig")
        resources = (ROOT / "src/lang/texts.rc2").read_text(encoding="utf-8-sig")
        self.assertEqual(re.findall(r"^#define\s+(\w+)\s+14703\s*$", header, re.M), [RESOURCE])
        rows = re.findall(r"^\s*" + RESOURCE + r',\s*"([^"\r\n]*)"\s*$', resources, re.M)
        self.assertEqual(rows, [ENGLISH])
        self.assertNotIn("%", rows[0])

    def test_translations_and_no_placeholders(self):
        paths = {path.parent.name: path for path in (ROOT / "translations").glob("*/salamand.slt")}
        self.assertEqual(set(paths), LOCALES)
        for locale, path in paths.items():
            with self.subTest(locale=locale):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf[EXPORTINFO]"))
                self.assertEqual(raw.count(b"\n"), raw.count(b"\r\n"))
                text = raw.decode("utf-8-sig")
                section = text.split("[STRINGTABLE 197]", 1)[1].split("[STRINGTABLE 198]", 1)[0]
                pattern = r'^14703,1,"([^"\r\n]+)"\s*$'
                rows = re.findall(pattern, text, re.M)
                self.assertEqual(len(rows), 1)
                self.assertEqual(len(re.findall(pattern, section, re.M)), 1)
                self.assertNotIn("%", rows[0])
                if locale == "czech":
                    self.assertEqual(rows[0], "Přesun chráněné složky do Koše s oprávněními správce zatím není podporován.")


if __name__ == "__main__":
    unittest.main()
