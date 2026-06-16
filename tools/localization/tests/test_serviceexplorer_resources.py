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

if __name__ == "__main__":
    unittest.main()
