import re
import unittest
from pathlib import Path

SCRIPT = Path(__file__).parents[1] / "rebase_text_archive.ps1"

class RebaseTextArchiveTests(unittest.TestCase):
    def test_new_current_items_are_marked_untranslated(self):
        text = SCRIPT.read_text(encoding="utf-8")
        states = re.findall(r"\$state = if \(\$null -ne \$Legacy\) \{ \$Legacy\.State \} else \{ ([^}]+) \}", text)
        self.assertEqual(states, ["0", "0", "0"])

    def test_new_sections_are_converted_to_untranslated(self):
        text = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("Convert-SectionToUntranslated -Section $currentSection", text)
        self.assertNotIn("# New section — emit current (English) as-is", text)

    def test_section_order_fallback_requires_same_section_count(self):
        text = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("$sameKindCurrentSections.Count -ne $sameKindLegacySections.Count", text)
        self.assertIn("SectionOrdinalFallbacks", text)

    def test_batch_dry_run_still_writes_translated_candidates(self):
        script = (Path(__file__).parents[1] / "localize_all_openai.ps1").read_text(encoding="utf-8")
        self.assertNotIn("$args+='--dry-run'", script)
        self.assertIn("if(-not $DryRun)", script)

if __name__ == "__main__":
    unittest.main()
