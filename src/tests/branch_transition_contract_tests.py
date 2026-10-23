# SPDX-License-Identifier: GPL-2.0-or-later
"""Transition gates expose only accepted Branch integrations."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
def source(name): return (ROOT/name).read_text(encoding='utf-8-sig')
def function(name, marker):
 text=source(name); start=text.index(marker); opening=text.index('{',start); depth=1; end=opening+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
class BranchTransitionContracts(unittest.TestCase):
 def test_activation_and_restore_require_process_gate(self):
  self.assertIn('SAMANDARIN_EXPERIMENTAL_BRANCH_VIEW',source('branch_view.h'))
  self.assertIn("value[0] == L'1'",source('branch_view.h'))
  for marker in ('void CFilesWindow::ToggleBranchView()', 'void CFilesWindow::RestoreBranchViewState('):
   self.assertIn('ExperimentEnabled()',function('branch_view.cpp',marker))
 def test_command_gate_precedes_plugin_and_user_callbacks(self):
  text=source('mainwnd3.cpp');start=text.index('CFilesWindow* branchCommandPanel');part=text[start:]
  self.assertLess(part.index('!BranchReadOnlyCommandAllowed(LOWORD(wParam))'),part.index('if (HelpMode'))
  gate=function('mainwnd3.cpp','static bool BranchReadOnlyCommandAllowed(')
  for forbidden in ('CM_COPYFILES','CM_MOVEFILES','CM_DELETEFILES','CM_RENAMEFILE','CM_VIEW_WITH','CM_EDIT_WITH'):
   self.assertNotIn('case '+forbidden+':',gate)
  for allowed in ('CM_OPEN','CM_VIEW','CM_ALTVIEW','CM_EDIT','CM_BRANCH_OPENPARENT'):
   self.assertIn('case '+allowed+':',gate)
  self.assertIn('return false;',gate)
 def test_readonly_direct_paths_are_guarded(self):
  cases={'fileswn5.cpp':['void CFilesWindow::RenameFile(', 'void CFilesWindow::QuickRenameBegin('], 'fileswn6.cpp':['void CFilesWindow::DropCopyMove('], 'fileswn9.cpp':['BOOL CFilesWindow::ClipboardPaste(']}
  for file, markers in cases.items():
   for marker in markers:
    part=function(file,marker)
    self.assertIn('if (IsBranchView()' if file == 'fileswn2.cpp' else 'if (IsBranchView()) return',part)
 def test_view_and_edit_require_current_metadata_before_full_path(self):
  for marker in ('void CFilesWindow::ViewFile(', 'void CFilesWindow::EditFile('):
   part=function('fileswn5.cpp',marker)
   self.assertLess(part.index('GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(*f)) != f'),part.index('GetItemFullPathW(*f)'))
 def test_plugin_viewer_policy_precedes_the_dispatch(self):
  part=function('fileswn5.cpp','void CFilesWindow::ViewFile(')
  self.assertLess(part.index('// S27a transition:'),part.index('ViewFileInt('))
  self.assertIn('viewer->ViewerType == VIEWER_INTERNAL || viewer->ViewerType == VIEWER_EXTERNAL',part)
  self.assertIn('if (!allowed) return;',part)
  self.assertNotIn('CanViewFile(',part)
 def test_parent_navigation_rejects_missing_identity(self):
  part=function('branch_view.cpp','BOOL CFilesWindow::OpenBranchItemDirectory()')
  self.assertLess(part.index('GetBranchViewFileByCacheKey('),part.index('ChangePathToDiskW('))
  self.assertIn('if (path.empty() || name.empty()) return FALSE;',part)
  self.assertLess(part.index('GetFileAttributesW('),part.index('if (!ChangePathToDiskW('))
 def test_parent_focus_uses_owned_wide_basename(self):
  part=function('branch_view.cpp','BOOL CFilesWindow::OpenBranchItemDirectory()')
  self.assertLess(part.index('std::wstring name ='),part.index('ChangePathToDiskW('))
  self.assertLess(part.index('ChangePathToDiskW('),part.index('SetCaretIndex('))
  self.assertIn('currentName == name',part)
  self.assertNotIn('focus.c_str()',part)
 def test_enter_uses_current_identity_and_preserves_shell_policy(self):
  part=function('fileswn2.cpp','void CFilesWindow::Execute(')
  self.assertIn('GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(*file)) != file',part)
  self.assertIn('SalPathRemoveExtendedPrefixW(source.c_str())',part)
  self.assertIn('if (shellFile.size() >= MAX_PATH)',part)
  self.assertNotIn('SEE_MASK_NOZONECHECKS',part)
 def test_archive_preflight_is_wide_at_both_call_sites(self):
  part=function('fileswn2.cpp','BOOL CFilesWindow::ChangePathToArchive(')
  self.assertEqual(part.count('OpenArchiveFileForRead(archive,'),2)
  self.assertNotIn('CreateFile(archive,',part)
  read=function('fileswn2.cpp','static HANDLE OpenArchiveFileForRead(')
  self.assertIn('SalPathAddExtendedPrefixW',read)
  self.assertIn('HANDLES_Q(CreateFileW(',read)
 def test_operations_guard_source_and_destination(self):
  for file in ('fileswn7.cpp','fileswn8.cpp'):
   text=source(file)
   self.assertIn('IsBranchView() ||',text)
   self.assertIn('target->IsBranchView()',text)
 def test_stop_is_present_in_the_actual_menu(self):
  self.assertIn("CM_BRANCH_STOP,",source("menu4.cpp"))
 def test_manual_only_refresh_and_detailed_layout(self):
  self.assertIn('if (automatic) return;',function('branch_view.cpp','void CFilesWindow::RefreshBranchView('))
  toggle=function('branch_view.cpp','void CFilesWindow::ToggleBranchView()')
  self.assertIn('SelectViewTemplate(2',toggle)
  self.assertIn('OriginalViewTemplate',toggle)
  self.assertIn('StopRefresh || SnooperSuspended || FilesActionInProgress',toggle)
if __name__=='__main__': unittest.main()
