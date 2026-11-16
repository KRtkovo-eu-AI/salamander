# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Cover newly added language archives. | Accept explicitly untranslated English fallbacks while retaining existing translation checks. | #731 | GPT-6 |
# | 2026-11-11 | Check verified-result viewer messages. | Keep consent and handler limitations localized without placeholders. | #731 | GPT-6.1 Terra |
"""Check verified-result F3 resources in the source and every shipped locale."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCALES = {"arabic", "bulgarian", "chinesesimplified", "chinesetraditional", "croatian",
           "czech", "dutch", "french", "german", "greek", "hebrew", "hungarian",
           "indonesian", "italian", "japanese", "korean", "polish", "portuguese",
           "romanian", "russian", "slovak", "spanish", "swedish", "turkish", "ukrainian", "vietnamese"}
FALLBACKS = {"catalan", "danish", "finnish", "lithuanian", "norwegian", "slovenian"}
IDS = {"IDS_UAC_VIEW_IDENTITY_HANDLER_UNSUPPORTED": 14710, "IDS_UAC_VIEW_VERIFIED_CONFIRM": 14711}


class VerifiedViewLocalizationTests(unittest.TestCase):
    def test_panel_consent_discloses_read_access(self):
        english = 'Access was denied. Read and list this folder and its subfolders as an administrator?'
        resources = (ROOT / 'src/lang/texts.rc2').read_text(encoding='utf-8-sig')
        self.assertIn('IDS_UAC_LIST_CONFIRM, "' + english + '"', resources)
        for path in (ROOT / 'translations').glob('*/salamand.slt'):
            rows = re.findall(r'^14631,([01]),"([^"\r\n]+)"$', path.read_text(encoding='utf-8-sig'), re.M)
            self.assertEqual(len(rows), 1, path)
            if path.parent.name == 'czech':
                self.assertEqual(rows[0], ('1', 'Přístup byl odepřen. Číst a vypsat tuto složku a její podsložky jako správce?'))
            elif path.parent.name in FALLBACKS:
                self.assertEqual(rows[0], ('0', english))
            else:
                self.assertEqual(rows[0][0], '1', path)

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
        self.assertEqual(set(paths), LOCALES | FALLBACKS)
        resources = (ROOT / 'src/lang/texts.rc2').read_text(encoding='utf-8-sig')
        english = dict(re.findall(r'^\s*(IDS_[A-Z0-9_]+),\s*"(.*)"\s*$', resources, re.M))
        for locale, path in paths.items():
            with self.subTest(locale=locale):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf[EXPORTINFO]"))
                self.assertEqual(raw.count(b"\n"), raw.count(b"\r\n"))
                text = raw.decode("utf-8-sig")
                section = text.split("[STRINGTABLE 198]", 1)[1].split("[STRINGTABLE 199]", 1)[0]
                headers = [int(n) for n in re.findall(r"^\[STRINGTABLE (\d+)\]$", text, re.M)]
                self.assertEqual(headers, list(range(len(headers))))
                for name, number in IDS.items():
                    state = '0' if locale in FALLBACKS else '1'
                    pattern = r'^' + str(number) + ',' + state + r',"([^"\r\n]+)"\s*$'
                    rows = re.findall(pattern, text, re.M)
                    self.assertEqual(len(rows), 1, (locale, number))
                    self.assertEqual(len(re.findall(pattern, section, re.M)), 1, (locale, number))
                    self.assertNotIn("%", rows[0])
                    if locale in FALLBACKS:
                        self.assertEqual(rows[0], english[name])
                    if number == 14711:
                        self.assertIn("16", rows[0])


if __name__ == "__main__":
    unittest.main()
