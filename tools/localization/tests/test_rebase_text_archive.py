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

    def test_batch_script_defaults_to_openrouter_gpt(self):
        script = (Path(__file__).parents[1] / "localize_all_openai.ps1").read_text(encoding="utf-8")
        self.assertIn('else{"openrouter"}', script)
        self.assertIn('"openai/gpt-5.4-nano"', script)
        self.assertIn("OPENROUTER_API_KEY", script)
        self.assertIn("'--provider',$Provider", script)

    def test_localize_all_passes_source_archive_and_trim_switch(self):
        script = (Path(__file__).parents[1] / "localize_all_openai.ps1").read_text(encoding="utf-8")
        self.assertIn("[switch]$AutoTrimTranslations", script)
        self.assertIn("'--source-archive',$source", script)
        self.assertIn("if($AutoTrimTranslations){$args+='--trim-translations'}", script)

    def test_stringtables_are_rebased_by_global_string_id(self):
        text = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("$legacyStringItemsById = @{}", text)
        self.assertIn("Merge-StringTableSection -CurrentSection $currentSection -LegacyStringItemsById $legacyStringItemsById", text)
        self.assertNotIn("Merge-KeyedSection -CurrentSection $currentSection -LegacySection $legacySection -ParseLine ${function:Parse-StringItem}", text)

    def test_stringtable_reuse_checks_technical_tokens(self):
        text = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("Test-CanReuseTranslation -CurrentText $currentItem.Text -TranslatedText $legacyItem.Text", text)
        self.assertIn("Get-AcceleratorCount", text)

    def test_literal_spaced_ampersand_is_not_accelerator(self):
        text = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("[char]::IsWhiteSpace($previousChar) -and [char]::IsWhiteSpace($nextChar)", text)

if __name__ == "__main__":
    unittest.main()
