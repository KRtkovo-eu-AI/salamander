# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Keep the optional drive-space controls and localized freshness text in sync."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(relative):
    return (ROOT / relative).read_text(encoding="utf-8-sig")


def dialog_rows(text):
    block = re.search(r"\[DIALOG 476\]\n(.*?)(?=\n\[)", text, re.S).group(1)
    rows = {}
    for line in block.splitlines()[1:]:
        match = re.fullmatch(r'(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),"(.*)"', line)
        if match:
            resource, x, y, width, height, state = map(int, match.groups()[:6])
            rows[resource] = (x, y, width, height, state, match[7])
    return rows


class DriveFreeSpaceUIContracts(unittest.TestCase):
    def test_independent_defaults_and_persistence(self):
        defaults = read("src/dialogs4.cpp")
        persistence = read("src/mainwnd2.cpp")
        transfer = read("src/dialogs5.cpp")
        for field, key in (("RemovableFreeSpacePolicy", "CONFIG_REMOVABLE_FREE_SPACE_POLICY"),
                           ("RemoteFreeSpacePolicy", "CONFIG_REMOTE_FREE_SPACE_POLICY")):
            self.assertIn(f"{field} = 0;", defaults)
            self.assertRegex(persistence, rf"SetValue\(actSubKey, {key}, REG_DWORD,\s*&Configuration\.{field}, sizeof\(DWORD\)\)")
            self.assertRegex(persistence, rf"GetValue\(actSubKey, {key}, REG_DWORD,\s*&Configuration\.{field}, sizeof\(DWORD\)\)")
            self.assertRegex(persistence, rf"if \(\(unsigned\)Configuration\.{field} > 2\)\s*Configuration\.{field} = 0;")
        self.assertIn("DriveFreeSpaceSetPolicies(", transfer)

    def test_localized_resources_and_format_arguments(self):
        translations = sorted((ROOT / "translations").glob("*/salamand.slt"))
        self.assertEqual(len(translations), 11)
        for path in translations:
            with self.subTest(language=path.parent.name):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf"))
                self.assertEqual(raw.count(b"\r\n"), raw.count(b"\n"))
                text = raw.decode("utf-8-sig").replace("\r\n", "\n")
                rows = dialog_rows(text)
                for resource in range(6296, 6301):
                    self.assertIn(resource, rows)
                    self.assertEqual(rows[resource][4], 1)
                for resource in range(15000, 15006):
                    matches = re.findall(rf'^{resource},1,"(.*)"$', text, re.M)
                    self.assertEqual(len(matches), 1)
                    expected_arguments = 2 if resource >= 15003 else 0
                    self.assertEqual(matches[0].count("%s"), expected_arguments)
                    self.assertNotIn("\ufffd", matches[0])

    def test_compact_translated_controls_do_not_overlap(self):
        for path in (ROOT / "translations").glob("*/salamand.slt"):
            with self.subTest(language=path.parent.name):
                rows = dialog_rows(path.read_text(encoding="utf-8-sig"))
                for label, combo in ((6297, 6298), (6299, 6300)):
                    lx, ly, lw, lh, _, _ = rows[label]
                    cx, cy, cw, _, _, _ = rows[combo]
                    self.assertLessEqual(lx + lw, cx)
                    self.assertEqual(ly, cy + 2)
                    self.assertLessEqual(cx + cw, 299)
                # COMBOBOX height includes its dropdown; only the closed 14-DLU box is visible.
                self.assertLessEqual(rows[6298][1] + 14, rows[6300][1])
                self.assertLessEqual(rows[489][1] + rows[489][3], rows[488][1])
                self.assertLessEqual(rows[1158][0] + rows[1158][2], rows[488][0])
                self.assertEqual(rows[1158][1], rows[488][1] + 2)
                self.assertEqual(rows[488][1], rows[360][1])
                self.assertLessEqual(rows[488][1] + rows[488][3], 231)

    def test_menu_hover_uses_cached_free_space_only(self):
        source = read("src/drivelst.cpp")
        callback = source.split("static void GetDriveMenuFreeSpaceToolTip(", 1)[1].split("BOOL CDrivesList::LoadMenuFromData()", 1)[0]
        self.assertIn("itemID == 0 || itemID > static_cast<DWORD>(drives->Count)", callback)
        self.assertIn("drvtRemovable", callback)
        self.assertIn("drvtRemote", callback)
        self.assertIn("AppendOptionalDriveFreeSpace(drive, text)", callback)
        for blocking_call in ("GetDriveBarToolTip", "GetVolumeInformation", "MyGetDiskFreeSpace", "GetDiskFreeSpaceEx"):
            self.assertNotIn(blocking_call, callback)
        self.assertIn("MenuPopup->SetRightTextToolTip(GetDriveMenuFreeSpaceToolTip, this);", source)

    def test_description_template_is_expanded_without_printf(self):
        source = read("src/drivelst.cpp")
        append = source.split("static void AppendOptionalDriveFreeSpace(", 1)[1].split("// Menu hover", 1)[0]
        self.assertIn('#include "drivefreespace_text.h"', source)
        self.assertRegex(append, r"FormatDescription\(\s*(?:pattern|LoadStr\(format\)),\s*size,\s*timestamp\)")
        self.assertIn("CopyStringTruncateUtf8", append)
        # Locale data is text, never a variadic format string. The timestamp's
        # separate constant wide format remains legitimate.
        self.assertNotRegex(append, r"\b(?:_scprintf|_snprintf_s|snprintf|sprintf)\s*\(")
        helper = read("src/drivefreespace_text.h")
        self.assertNotRegex(helper, r"\b(?:_scprintf|_snprintf_s|snprintf|sprintf|vsnprintf|vsprintf)\s*\(")

    def test_english_modes_and_timestamp_formats(self):
        resources = read("src/lang/texts.rc2")
        for key in ("FREE", "CACHED", "STALE"):
            value = re.search(rf'IDS_DRIVE_SPACE_{key}, "([^"]*)"', resources).group(1)
            self.assertEqual(value.count("%s"), 2)
        dialog = read("src/lang/lang.rc").split("IDD_CFGPAGE_DRIVES DIALOGEX", 1)[1].split("\nEND", 1)[0]
        for combo in ("REMOVABLESPACE", "REMOTESPACE"):
            self.assertRegex(dialog, rf"COMBOBOX\s+IDC_DRVSPEC_{combo},.*CBS_DROPDOWNLIST")


if __name__ == "__main__":
    unittest.main()
