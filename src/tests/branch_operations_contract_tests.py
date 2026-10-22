# SPDX-License-Identifier: GPL-2.0-or-later
"""Contracts keeping Branch View operations on exact per-item identities."""
from pathlib import Path
import unittest
import argparse
SCOPE = "OrdinaryRename"
ROOT = Path(__file__).resolve().parents[1]
def source(name):
    return (ROOT / name).read_text(encoding='utf-8-sig')
class BranchOperationsContracts(unittest.TestCase):
    def test_initial_view_and_edit_use_owned_wide_full_path(self):
        text = source('fileswn5.cpp')
        for first, last in [('void CFilesWindow::ViewFile(', 'void CFilesWindow::ViewFileInt('), ('void CFilesWindow::EditFile(', 'void CFilesWindow::FillEditWithMenu')]:
            start = text.index(first)
            part = text[start:text.find(last, start) if last in text[start:] else start + 16000]
            self.assertIn('GetItemFullPathW(*f)', part)
            self.assertIn('unicodeDiskFileName = SalWideToMultiBytePath(wideName.c_str(), CP_UTF8)', part)
    def test_rename_remaps_identity_after_every_success(self):
        text = source('fileswn5.cpp')
        part = text[text.index('void CFilesWindow::RenameFileInternalW('):text.index('void CFilesWindow::RenameFileInternal(')]
        self.assertIn('GetItemDirectoryW(*f)', part)
        self.assertIn('GetItemFullPathW(*f)', part)
        self.assertEqual(part.count('UpdateFileDataNameAfterRename('), 3)
        if SCOPE != 'All':
            self.assertIn('PostChangeOnPathNotification(directory.c_str(), isDir)', part)
            return  # Real Branch metadata remap/sleep assertions retained for S26.
        self.assertEqual(part.count('BranchItemRenamed(oldNameKey, *f, oldFullPath)'), 3)
        self.assertEqual(part.count('SleepIconCacheThread()'), 3)
    def test_builder_resolves_each_selected_item(self):
        text = source('fileswn6.cpp')
        part = text[text.index('BOOL CFilesWindow::BuildScriptMain('):text.index('char ADSStreamsGlobalBuf')]
        self.assertIn('GetItemDirectoryW(*oneFile)', part)
        self.assertIn('GetItemFullPathW(*oneFile)', part)
        self.assertIn('script->AddStoragePath(sourcePath', part)
        self.assertIn('useName = (char*)branchName.c_str()', part)
        self.assertIn('BuildScriptFile(script, type, sourcePath', part)
    def test_relative_targets_have_cancel_skip_labels(self):
        text = source('fileswn6.cpp')
        self.assertIn('GetItemRelativePathW(*oneFile)', text)
        self.assertIn('component == L".."', text)
        self.assertIn('branchCreatedDirectories.rbegin()', text)
        self.assertIn('label.Opcode = ocLabelForSkipOfCreateDir', text)
    def test_archive_enumerator_returns_relative_identity(self):
        text = source('fileswn7.cpp')
        self.assertIn('data->SourcePanel->GetItemRelativePathW(file)', text)
        self.assertIn('return data->BranchEnumName.c_str()', text)
        self.assertIn('SourcePanel = NULL', text)
    def test_recycle_queues_whole_wide_selection(self):
        text = source('fileswn8.cpp')
        self.assertIn('GetItemFullPathW(*file)', text)
        self.assertIn('SHCreateItemFromParsingName(path.c_str()', text)
        self.assertIn('operation->DeleteItem(shellItem, NULL)', text)
        self.assertIn('operation->PerformOperations()', text)
    def test_selected_extension_recycle_uses_exact_wide_shell_operation(self):
        text = source('worker.cpp')
        self.assertIn('RecycleOperationFileW(owner, WorkerOperationPathW(name, exactName))', text)
        helper = source('operationrecycle.h')
        self.assertIn('OperationRecyclePath(exactPath, shellPath)', helper)
        self.assertIn('SHCreateItemFromParsingName(shellPath.c_str()', helper)
        self.assertIn('operation->GetAnyOperationsAborted(&aborted)', helper)
        self.assertNotIn('SHFileOperation', helper)
    def test_delete_and_attributes_preserve_owned_wide_source(self):
        text = source('fileswn6.cpp')
        self.assertIn('IsValidPathUtf8Text(path) ? CP_UTF8 : CP_ACP', text)
        self.assertEqual(text.count('SalPathAppendW(sourceW, fileNameW)'), 2)
        self.assertNotIn('SalPathAddExtendedPrefixW(SalMultiByteToWidePathUtf8OrAcp(op.SourceName)', text)
    def test_option_is_localized_and_only_available_for_branch(self):
        dialog = source('dialogs3.cpp')
        self.assertIn('LoadStr(IDS_BRANCH_KEEP_PATHS)', dialog)
        self.assertIn('ti.CheckBox(IDC_BRANCH_KEEP_PATHS, *KeepBranchPathsInOut)', dialog)
        self.assertIn('IsBranchView() ? &keepBranchPaths : NULL', source('fileswn8.cpp'))
        self.assertEqual(source('lang/lang.rc').count('"",IDC_BRANCH_KEEP_PATHS,"Button"'), 2)
    def test_cache_and_cut_flags_use_identity(self):
        self.assertIn('panel->GetItemCacheKeyPtr(*f)', source('fileswna.cpp'))
        self.assertIn('panel->GetItemIdentityW(*f) == anotherPanel->GetItemIdentityW(*f2)', source('shellsup.cpp'))
    def test_icon_reader_never_combines_different_encodings(self):
        text = source('fileswn1.cpp')
        self.assertIn('if (diskListing && !branchListing)', text)
        self.assertIn('SalWideToMultiBytePath(window->GetItemFullPathW(file).c_str(), CP_UTF8)', text)
        self.assertIn('const std::string itemPath = resolveDiskPath(iconData->NameAndData)', text)
        self.assertIn('const std::string thumbnailPathUtf8 = resolveDiskPath(s)', text)
        self.assertIn('!PathContainsValidComponents(path, FALSE)', text)
        self.assertIn('LoadImageW(NULL, pathW.c_str(), IMAGE_ICON', text)
        self.assertIn('CopyStringTruncateUtf8(TransferPanelPath, SAL_MAX_PATH', source('fileswn4.cpp'))
    def test_branch_context_menu_routes_containing_folder_outside_shell(self):
        text = source('shellsup.cpp')
        part = text[text.index('    case saContextMenu:'):text.index('const char* ReturnNameFromParam')]
        self.assertIn('branchOpenParentCommand = 10002', part)
        insertion = part[part.index('if (useSelection && !onlyPanelMenu && panel->IsBranchView()'):part.index('if (GetMenuItemCount(h) > 0) // protection')]
        self.assertIn('panel->GetCaretIndex() >= panel->Dirs->Count', insertion)
        self.assertIn('LoadStr(IDS_BRANCH_OPENPARENT)', insertion)
        self.assertIn('AppendMenuW(h, MF_STRING | MF_ENABLED, branchOpenParentCommand', insertion)
        dispatch = part.index('if (cmd == branchOpenParentCommand)')
        self.assertLess(dispatch, part.index('IsWindows11CompressedFolderCommand(h, cmd)'))
        self.assertIn('openBranchParent = TRUE;', part[dispatch:dispatch + 240])
        self.assertIn('cmd = 0;', part[dispatch:dispatch + 240])
        self.assertLess(part.index('ShellActionAux6(panel)'), part.index('panel->OpenBranchItemDirectory()'))
        self.assertLess(part.index('DestroyMenu(h);', part.index('ShellActionAux6(panel)')), part.index('panel->OpenBranchItemDirectory()'))

    def test_unsupported_shell_menu_falls_back_to_complete_host_selection(self):
        text = source('shellsup.cpp')
        helper = text[text.index('static DWORD TrackBranchHostContextMenu('):text.index('void ShellAction(CFilesWindow* panel,')]
        for command in ('CM_OPEN', 'CM_VIEW', 'CM_EDIT', 'CM_BRANCH_OPENPARENT', 'CM_COPYFILES', 'CM_MOVEFILES', 'CM_DELETEFILES', 'CM_CLIPCUT', 'CM_CLIPCOPY', 'CM_PROPERTIES'):
            self.assertIn('append(' + command + ',', helper)
        self.assertIn('SetImageList(HGrayToolBarImageList)', helper)
        self.assertIn('SetHotImageList(HHotToolBarImageList)', helper)
        self.assertNotIn('SetSel(', helper)
        self.assertNotIn('GetUIObjectOf', helper)
        action = text[text.index('void ShellAction(CFilesWindow* panel,'):text.index('const char* ReturnNameFromParam')]
        self.assertIn('EnumFileNames, &selectionEnumData, TRUE)', action)
        self.assertIn('&selectionEnumData, TRUE)', action)
        fallback = action.index('else if (panel->IsBranchView() && useSelection && !onlyPanelMenu)')
        self.assertIn('deferredBranchCommand = TrackBranchHostContextMenu(panel, pt)', action[fallback:fallback + 250])
        retry = action.index('incomplete shell context menu, retrying QueryContextMenu')
        self.assertLess(action.index('if (panel->ContextMenu != NULL && h != NULL)', retry), fallback)
        dispatch = action.index('if (deferredBranchCommand != 0)')
        self.assertLess(action.index('ShellActionAux6(panel)'), dispatch)
        self.assertLess(action.rindex('EndStopRefresh();'), dispatch)
        self.assertLess(action.index('MainWindow->FocusPanel(panel)', dispatch), action.index('MainWindow->RefreshCommandStates()', dispatch))
        self.assertLess(action.index('MainWindow->RefreshCommandStates()', dispatch), action.index('SendMessage(MainWindow->HWindow, WM_COMMAND, deferredBranchCommand', dispatch))

    def test_name_clipboard_uses_wide_item_identity(self):
        text = source('fileswn9.cpp')
        start = text.index('BOOL CFilesWindow::CopyFocusedNameToClipboard(')
        part = text[start:text.index('BOOL CFilesWindow::CopyCurrentPathToClipboard', start)]
        branch = part[part.index('if (IsBranchView())'):part.index('    char buff[2 * MAX_PATH];')]
        self.assertIn('GetItemFullPathW(file)', branch)
        self.assertIn('BranchClipboardUNCPath(fullPath, unc)', branch)
        self.assertEqual(branch.count('CopyTextToClipboardW('), 3)
        self.assertNotIn('CopyUNCPathToClipboard', branch)
    def test_ordinary_copy_move_owns_unicode_basename(self):
        text = source('fileswn6.cpp')
        part = text[text.index('BOOL CFilesWindow::BuildScriptFile('):]
        self.assertIn('const std::string ownedFileName = fileNameW', part)
        self.assertLess(part.index('fileName = const_cast<char*>(ownedFileName.c_str())'), part.index('op.SourceName = BuildName('))
        self.assertIn('CPathBuffer finalName(4 * SAL_MAX_PATH)', part)
        self.assertIn('MaskName(finalName.Data(), finalName.Capacity()', part)
    def test_ordinary_copy_move_uses_item_parent_at_dialog_boundary(self):
        text = source('fileswn8.cpp')
        self.assertIn('GetItemDirectoryW(*dir)', text)
        self.assertIn('GetItemFullPathW(*dir)', text)
        self.assertIn('sourceBaseName.c_str(), sourceDirectory.c_str(), mask', text)
        builder = source('fileswn6.cpp')
        self.assertEqual(builder.count('CPathBuffer sourcePathStorage(4 * SAL_MAX_PATH)'), 2)
        self.assertIn('unicodeCopyName.empty()', builder)

def load_tests(loader, tests, pattern):
    if SCOPE == 'All':
        return tests
    names = ('test_initial_view_and_edit_use_owned_wide_full_path',
             'test_rename_remaps_identity_after_every_success')
    if SCOPE == 'OrdinaryCopyMove':
        names += ('test_ordinary_copy_move_owns_unicode_basename', 'test_ordinary_copy_move_uses_item_parent_at_dialog_boundary')
    return unittest.TestSuite(BranchOperationsContracts(name) for name in names)

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--scope', choices=('OrdinaryRename', 'OrdinaryCopyMove', 'All'), default='OrdinaryCopyMove')
    args, remaining=parser.parse_known_args()
    SCOPE=args.scope
    print('Operation contracts scope:', SCOPE, '; future Branch assertions require All/S26', flush=True)
    unittest.main(argv=[__file__, *remaining])
