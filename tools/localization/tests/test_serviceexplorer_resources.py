import re
import unittest
from pathlib import Path

ROOT = Path(__file__).parents[3]
PLUGIN = ROOT / "src/plugins/serviceexplorer"

class ServiceExplorerResourceTests(unittest.TestCase):
    def test_dialogs_used_by_code_exist_in_language_resources(self):
        code = (PLUGIN / "dialogs.cpp").read_text(encoding="utf-8", errors="replace")
        used_dialogs = set(re.findall(r"\bIDD_[A-Z0-9_]+\b", code))
        rc2 = (PLUGIN / "lang/lang.rc2").read_text(encoding="utf-8", errors="replace")
        defined_dialogs = set(re.findall(r"^(IDD_[A-Z0-9_]+)\s+DIALOG(?:EX)?\b", rc2, re.MULTILINE))
        self.assertTrue(used_dialogs)
        self.assertEqual(used_dialogs - defined_dialogs, set())

    def test_plugin_has_standard_root_resource_files(self):
        self.assertTrue((PLUGIN / "serviceexplorer.rc").is_file())
        self.assertTrue((PLUGIN / "serviceexplorer.rc2").is_file())
        self.assertTrue((PLUGIN / "serviceexplorer.rh").is_file())
        self.assertTrue((PLUGIN / "versinfo.rh2").is_file())
        project = (PLUGIN / "vcxproj/serviceexplorer.vcxproj").read_text(encoding="utf-8", errors="replace")
        self.assertIn(r'ResourceCompile Include="..\serviceexplorer.rc"', project)
        self.assertIn(r'ClInclude Include="..\serviceexplorer.rh"', project)
        self.assertNotIn("plugin.rc", project)
        self.assertNotIn("plugin.rh", project)

    def test_language_resource_uses_standard_version_include(self):
        lang_rc2 = (PLUGIN / "lang/lang.rc2").read_text(encoding="utf-8", errors="replace")
        self.assertIn('#include "versinfo.rc2"', lang_rc2)
        self.assertNotIn('..\\shared\\versinfo.rc2', lang_rc2)

    def test_language_symbols_do_not_use_negative_ids(self):
        lang_rh = (PLUGIN / "lang/lang.rh").read_text(encoding="utf-8", errors="replace")
        self.assertNotRegex(lang_rh, r"^#define\s+\S+\s+-\d+", msg="Translator include files must not contain negative symbolic IDs")
        self.assertNotRegex(lang_rh, r"^#define\s+IDC_STATIC\s+-1\b", msg="Do not expose IDC_STATIC=-1 to Translator")

if __name__ == "__main__":
    unittest.main()
