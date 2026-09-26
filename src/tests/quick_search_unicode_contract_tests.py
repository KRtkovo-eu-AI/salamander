# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

"""Host integration boundaries accompanying the native Unicode input tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def section(text, start, end):
    begin = text.index(start)
    return text[begin:text.index(end, begin + len(start))]


class QuickSearchUnicodeContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def read(name):
            return (ROOT / 'src' / name).read_text(encoding='utf-8-sig')
        cls.box = read('filesbx1.cpp')
        cls.panel = read('fileswn0.cpp')
        cls.create = read('fileswnb.cpp')
        cls.header = read('fileswnd.h')
        cls.startup = read('salamdr1.cpp')
        cls.edit = read('editwnd.cpp')

    def test_panel_window_is_unicode_end_to_end(self):
        self.assertIn('const wchar_t* CFILESBOX_CLASSNAMEW', self.box)
        self.assertIn(': CWindow(ooStatic, TRUE)', self.box)
        self.assertIn('ListBox->CreateExW(WS_EX_WINDOWEDGE,', self.create)
        self.assertIn('RegisterUniversalClassW(CS_DBLCLKS | CS_OWNDC,', self.startup)
        self.assertIn('case WM_UNICHAR:', self.box)
        self.assertIn('wParam == UNICODE_NOCHAR', self.box)
        self.assertIn('wParam <= 0x10FFFF', self.box)
        self.assertIn('SendMessageW(HWindow, WM_CHAR, text[i], lParam)', self.box)

    def test_main_message_pump_preserves_unicode_with_ansi_menu_boundary(self):
        loop = self.startup[self.startup.index('//--- aplikacni smycka'):]
        for call in ('GetMessageW(&msg', 'PeekMessageW(&msg', 'DispatchMessageW(&msg',
                     'IsDialogMessageW(wnd->HWindow, &msg)', 'TranslateAcceleratorW(acceleratorTarget,'):
            self.assertIn(call, loop)
        for call in ('GetMessage(&msg', 'PeekMessage(&msg', 'DispatchMessage(&msg',
                     'IsDialogMessage(wnd->HWindow, &msg)', 'TranslateAccelerator(acceleratorTarget,'):
            self.assertNotIn(call, loop)
        self.assertIn('MSG menuMsg = msg;', loop)
        self.assertIn('length == 1 && !usedDefault', loop)
        self.assertIn('WC_NO_BEST_FIT_CHARS', loop)
        self.assertIn('IsMenuBarMessage(&menuMsg)', loop)

    def test_input_is_panel_local_and_packet_pairs_survive(self):
        self.assertIn('wchar_t QuickSearchHighSurrogate;', self.header)
        self.assertNotIn('GetUtf8QuickSearchText', self.panel)
        self.assertNotIn('static unsigned char pendingUtf8', self.panel)
        end = section(self.panel, 'void CFilesWindow::EndQuickSearch', '// Finds the next/previous item.')
        self.assertIn('QuickSearchHighSurrogate = 0;', end)
        focus = section(self.panel, 'void CFilesWindow::OnKillFocus', '\n}')
        self.assertIn('QuickSearchHighSurrogate = 0;', focus)
        self.assertIn('wParam != VK_PACKET', self.panel)
        self.assertIn('AppendQuickSearchCodeUnit(QuickSearchHighSurrogate,', self.panel)

    def test_alt_uses_original_system_character_and_event_context(self):
        system = section(self.panel, 'BOOL CFilesWindow::OnSysChar', 'BOOL CFilesWindow::OnChar')
        self.assertIn('(lParam & (1L << 29)) != 0', system)
        self.assertIn('OnChar(wParam, lParam, lResult, TRUE)', system)
        self.assertIn('fromSystemChar || (GetKeyState(VK_MENU)', self.panel)
        self.assertIn('MapVirtualKeyExW((UINT)wParam, MAPVK_VK_TO_CHAR, GetKeyboardLayout(0))', self.panel)
        self.assertNotIn('ToAscii(', self.panel)
        self.assertNotIn('ToUnicodeEx(', self.panel)
        self.assertIn('case WM_SYSDEADCHAR:', self.box)

    def test_search_has_one_wide_mask_and_transactional_append(self):
        search = section(self.panel, 'BOOL CFilesWindow::QSFindNext', '// Finds the next/previous selected item.')
        self.assertIn('const wchar_t* newText', search)
        self.assertIn('const size_t previousLength = QuickSearchMaskW.length();', search)
        self.assertIn('QuickSearchMaskW.resize(previousLength);', search)
        self.assertNotIn('MAX_PATH)', search)
        self.assertNotIn('lstrcpyn', search)
        self.assertNotIn('char QuickSearch[', self.header)
        self.assertNotIn('char QuickSearchMask[', self.header)

    def test_editing_uses_complete_characters_and_reconciles_normalization(self):
        keys = section(self.panel, 'case VK_BACK: // remove the last complete', 'case VK_UP:')
        self.assertIn('RemoveLastQuickSearchCharacter(QuickSearchMaskW)', keys)
        self.assertIn('ShortenQuickSearchPrefix(QuickSearchW, QuickSearchMaskW)', keys)
        self.assertIn('ExtendQuickSearchPrefix(QuickSearchW, QuickSearchMaskW, name)', keys)
        self.assertIn('AgreeQSMaskW(QuickSearchW.c_str(), TRUE, mask.c_str(), TRUE, matchedLength)', keys)
        self.assertIn('QuickSearchMaskW = QuickSearchW;', keys)

    def test_command_line_forwarding_never_narrows_unicode_characters(self):
        typed = section(self.panel, 'BOOL CFilesWindow::OnChar', 'void CFilesWindow::GotoSelectedItem')
        self.assertIn('PostMessageW(hEditLine, WM_CHAR, quickSearchText[i], lParam)', typed)
        self.assertNotIn('wParam < 256', typed)
        edit_char = section(self.edit[self.edit.index('CEditLine::WindowProc(UINT'):], 'case WM_CHAR:', 'case WM_KEYDOWN:')
        self.assertIn('switch (wParam)', edit_char)
        self.assertNotIn('switch ((TCHAR)wParam)', edit_char)

    def test_caret_offsets_remain_unicode_and_empty_extension_is_not_separate(self):
        caret = self.panel[self.panel.index('void CFilesWindow::SetQuickSearchCaretPos()'):]
        self.assertIn('GetQuickSearchCaretTextRange(name, QuickSearchW.length(), separateExtension)', caret)
        self.assertIn('file->Ext[0] != 0', caret)
        self.assertIn('GetTextExtentPoint32W', caret)
        self.assertIn('CPathBuffer formattedBuffer;', caret)
        self.assertNotIn('char formatedFileName[MAX_PATH]', caret)


if __name__ == '__main__':
    unittest.main()
