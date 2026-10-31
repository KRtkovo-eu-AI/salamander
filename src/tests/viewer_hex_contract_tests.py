# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

"""Verify the host routes binary and text operations to their intended cache.

The native companion tests exercise byte conversion, overlapping cache moves,
Unicode cell construction, and real GDI drawing. These checks cover the host
integration, whose full GUI class cannot be linked into that small executable.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def section(text, start, end):
    begin = text.index(start)
    return text[begin:text.index(end, begin + len(start))]


class ViewerHexIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / 'src/viewer.h').read_text(encoding='utf-8-sig')
        cls.viewer = (ROOT / 'src/viewer.cpp').read_text(encoding='utf-8-sig')
        cls.buffer = (ROOT / 'src/viewer2.cpp').read_text(encoding='utf-8-sig')
        cls.commands = (ROOT / 'src/viewer3.cpp').read_text(encoding='utf-8-sig')

    def test_raw_cache_has_same_allocation_and_valid_range(self):
        self.assertIn('malloc(2 * VIEW_BUFFER_SIZE)', self.viewer)
        self.assertIn('RawBuffer = Buffer != NULL ? Buffer + VIEW_BUFFER_SIZE : NULL;', self.viewer)
        self.assertIn('BOOL IsGood() { return Buffer != NULL', self.header)
        destructor = section(self.viewer, 'CViewerWindow::~CViewerWindow()',
                             'void CViewerWindow::DestroyViewerMenuControls')
        self.assertEqual(destructor.count('free(Buffer)'), 1)
        self.assertNotIn('free(RawBuffer)', destructor)

    def test_complete_reads_capture_raw_bytes_before_conversion(self):
        capture = section(self.buffer, 'void CViewerWindow::CodeCharacters',
                          'BOOL CViewerWindow::LoadBefore')
        self.assertIn('CaptureViewerBytes(RawBuffer + (start - Buffer), start,', capture)
        self.assertIn('UseCodeTable ? CodeTable : NULL', capture)
        before = section(self.buffer, 'BOOL CViewerWindow::LoadBefore',
                         'BOOL CViewerWindow::LoadBehind')
        behind = section(self.buffer, 'BOOL CViewerWindow::LoadBehind',
                         'void CViewerWindow::OpenFile')
        for method in (before, behind):
            self.assertRegex(method, r'if \(readed != \(DWORD\)read\)')
            self.assertRegex(method, r'else\s*\{\s*CodeCharacters\([^;]+;\s*Loaded \+= readed;')
            self.assertIn('MoveViewerBytes(RawBuffer, Buffer,', method)
            self.assertIn('Seek = Loaded = 0;', method)
        self.assertEqual(self.buffer.count('MoveViewerBytes('), 2)
        self.assertNotRegex(self.buffer, r'memmove\(Buffer')

    def test_conversion_and_mode_changes_invalidate_both_views(self):
        convert = section(self.commands, 'void CViewerWindow::SetCodeType',
                          'void CViewerWindow::OnVScroll')
        self.assertIn('Seek = 0;', convert)
        self.assertIn('Loaded = 0;', convert)
        change = section(self.buffer, 'void CViewerWindow::ChangeType',
                         'void CViewerWindow::')
        self.assertIn('Type = type;', change)
        self.assertIn('FileChanged(NULL, FALSE, fatalErr, FALSE)', change)
        self.assertLess(change.index('Type = type;'), change.index('FileChanged('))

    def test_hex_paint_uses_raw_numbers_and_fixed_unicode_cells(self):
        paint = self.viewer[self.viewer.index('void CViewerWindow::Paint'):]
        hex_paint = section(paint, 'case vtHex:', 'case vtText:')
        self.assertIn('FormatViewerHexBytes(RawBuffer + (lineOffset - Seek),', hex_paint)
        self.assertIn('Buffer + (lineOffset - Seek), (int)len,', hex_paint)
        self.assertEqual(hex_paint.count('DrawViewerHexCells('), 5)
        self.assertNotIn('MyTextOut(', hex_paint)
        self.assertNotIn('ViewerFontMapping', hex_paint)

    def test_binary_search_and_word_boundaries_use_raw_source(self):
        self.assertIn('return FindDialog.HexMode ? RawBuffer : Buffer;', self.header)
        search = section(self.commands, 'SearchData.SetFlags(flags);',
                         'FindingSoDonotSwitchToHex = FALSE;')
        self.assertIn('SearchData.SearchForward((char*)(GetSearchBuffer()', search)
        self.assertIn('SearchData.SearchBackward((char*)(GetSearchBuffer()', search)
        self.assertEqual(search.count('char c = *(GetSearchBuffer()'), 4)
        self.assertNotIn('(Buffer +', search)
        self.assertEqual(self.commands.count('RegExp.SetLine((char*)(Buffer +'), 2)

    def test_find_prefill_matches_search_mode_and_unsigned_hex_bytes(self):
        prefill = section(self.buffer, 'BOOL CViewerWindow::GetFindText',
                          'BOOL CViewerWindow::CheckSelectionIsNotTooBig')
        self.assertIn('HasDecodedTextMode() && !hexMode', prefill)
        self.assertIn('(hexMode ? RawBuffer : Buffer) + (off - Seek)', prefill)
        self.assertIn('endSel = startSel + FIND_TEXT_LEN - 1;', prefill)
        dialog = section(self.viewer, 'void CFindSetDialog::Transfer',
                         'CFindSetDialog::DialogProc')
        self.assertIn('view->GetFindText(buf, len, HexMode)', dialog)
        self.assertIn('(unsigned)(unsigned char)buf[i]', dialog)
        self.assertIn('len = (FIND_TEXT_LEN - 1) / 3;', dialog)

    def test_copy_export_drag_and_regex_keep_converted_text(self):
        copy = section(self.buffer, 'CViewerWindow::GetSelectedText(BOOL& fatalErr)',
                       'CViewerWindow::GetSelectedTextW(')
        self.assertIn('memcpy(s, Buffer + (off - Seek), (int)len);', copy)
        self.assertNotIn('RawBuffer', copy)
        export = section(self.commands, 'case CM_COPYTOFILE:', 'case CM_SELECTALLTEXT:')
        self.assertIn('WriteFile(file, Buffer + (off - Seek)', export)
        self.assertNotIn('RawBuffer', export)
        self.assertIn('HasDecodedTextMode() ? GetSelectedTextW(fatalErr, NULL) : GetSelectedText(fatalErr)',
                      self.commands)


if __name__ == '__main__':
    unittest.main()
