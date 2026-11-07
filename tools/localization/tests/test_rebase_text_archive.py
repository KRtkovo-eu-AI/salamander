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

    def test_menu_popup_ids_use_fifo_queues(self):
        text = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("New-IdItemQueueMap", text)
        self.assertIn("Add-IdItemToQueueMap", text)
        self.assertIn("Get-IdItemFromQueueMap", text)
        self.assertIn("Queue[object]", text)
        self.assertNotIn("$legacyItems[$item.Id] = $item", text)

    def test_duplicate_menu_popup_ids_preserve_distinct_titles(self):
        import shutil
        import subprocess
        import tempfile
        from pathlib import Path

        pwsh = shutil.which("pwsh") or shutil.which("powershell")
        if pwsh is None:
            self.skipTest("PowerShell is required to execute rebase_text_archive.ps1")

        current = """[EXPORTINFO]
PROJECTNAME,"x"
TEXTVERSION,"1"
VERSION,"1"

[TRANSLATION]
LANGID,1033

[MENU 6010]
0,0,"&File"
6050,0,"&Open"
0,0,"&Edit"
6062,0,"&Copy"
0,0,"&Help"
6091,0,"&Contents"
"""
        legacy = """[EXPORTINFO]
PROJECTNAME,"x"
TEXTVERSION,"1"
VERSION,"1"

[TRANSLATION]
LANGID,1029

[MENU 6010]
0,1,"&Soubor"
6050,1,"&Otevrit"
0,1,"&Upravit"
6062,1,"&Kopirovat"
0,1,"Napoveda"
6091,1,"&Obsah"
"""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            current_path = root / "current.slt"
            legacy_path = root / "legacy.slt"
            output_path = root / "out.slt"
            current_path.write_text(current, encoding="utf-8-sig")
            legacy_path.write_text(legacy, encoding="utf-8-sig")
            completed = subprocess.run(
                [
                    pwsh,
                    "-NoProfile",
                    "-File",
                    str(SCRIPT),
                    "-CurrentArchive",
                    str(current_path),
                    "-LegacyArchive",
                    str(legacy_path),
                    "-OutputArchive",
                    str(output_path),
                ],
                capture_output=True,
                text=True,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr or completed.stdout)
            popups = [
                line
                for line in output_path.read_text(encoding="utf-8-sig").splitlines()
                if line.startswith("0,")
            ]
            self.assertEqual(
                popups,
                ['0,1,"&Soubor"', '0,1,"&Upravit"', '0,1,"Napoveda"'],
            )

if __name__ == "__main__":
    unittest.main()
