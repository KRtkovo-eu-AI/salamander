# SPDX-License-Identifier: GPL-2.0-or-later
"""S26 deliberately exposes only a process-gated, read-only collection."""
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
  for forbidden in ('CM_COPYFILES','CM_MOVEFILES','CM_DELETEFILES','CM_BRANCH_OPENPARENT','CM_VIEW','CM_EDIT'):
   self.assertNotIn('case '+forbidden+':',gate)
  self.assertIn('return false;',gate)
 def test_readonly_direct_paths_are_guarded(self):
  cases={'fileswn5.cpp':['void CFilesWindow::ViewFile(', 'void CFilesWindow::EditFile(', 'void CFilesWindow::RenameFile(', 'void CFilesWindow::QuickRenameBegin('], 'fileswn6.cpp':['void CFilesWindow::DropCopyMove('], 'fileswn9.cpp':['BOOL CFilesWindow::ClipboardPaste(']}
  for file, markers in cases.items():
   for marker in markers:
    part=function(file,marker)
    self.assertIn('if (IsBranchView()) return',part)
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
