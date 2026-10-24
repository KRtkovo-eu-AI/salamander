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
  for forbidden in ('CM_VIEW_WITH','CM_EDIT_WITH','CM_CLIPPASTELINKS'):
   self.assertNotIn('case '+forbidden+':',gate)
  for allowed in ('CM_OPEN','CM_VIEW','CM_ALTVIEW','CM_EDIT','CM_BRANCH_OPENPARENT','CM_RENAMEFILE','CM_COPYFILES','CM_MOVEFILES','CM_DELETEFILES','CM_PROPERTIES','CM_CLIPCOPY','CM_CLIPCUT','CM_CLIPPASTE','CM_CONTEXTMENU'):
   self.assertIn('case '+allowed+':',gate)
  self.assertIn('return false;',gate)
 def test_incoming_paths_require_real_root(self):
  self.assertIn('IsBranchView() && !Salamander::BranchView::IsRootDropTarget(GetPathW(), targetPath)',function('fileswn6.cpp','void CFilesWindow::DropCopyMove('))
  paste=function('fileswn9.cpp','BOOL CFilesWindow::ClipboardPaste(')
  for guard in ('onlyLinks ||','!Salamander::BranchView::IsRootDropTarget(GetPathW(), pastePath)','IsBranchView() && IsFakeDataObject','IsBranchView() && !ownRutine'):
   self.assertIn(guard,paste)
  self.assertLess(paste.index('IsBranchView() && !ownRutine'),paste.index('CreateIContextMenu2('))
  root=function('branchdroptarget.h','inline bool IsRootDropTarget(')
  self.assertIn('target == root',root)
  receive=function('shellsup.cpp','const char* GetCurrentDir(')
  for guard in ('index >= 0 && index < panel->Dirs->Count + panel->Files->Count','DropSourcePanel == NULL','DropSourcePanel == panel','!DropSourcePanel->Is(ptDisk)','srcType != 0','root.size() >= sizeof(panel->DropPath)'):
   self.assertIn(guard,receive)
 def test_copy_is_flat_disk_only_and_requires_current_metadata(self):
  action=function('fileswn8.cpp','void CFilesWindow::FilesAction(')
  self.assertIn('(type != atCopy && type != atMove) || target == NULL || !target->Is(ptDisk)',action)
  self.assertIn('countSizeMode != 0 || copyToSelectedDirs',action)
  self.assertIn('target != NULL && target->IsBranchView()',action)
  self.assertLess(action.index('pathType != PATH_TYPE_WINDOWS'),action.index('if (pathType == PATH_TYPE_ARCHIVE)'))
  builder=function('fileswn6.cpp','BOOL CFilesWindow::BuildScriptMain(')
  self.assertIn('(type != atCopy && type != atMove && type != atDelete) || GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(*oneFile)) != oneFile',builder)
  self.assertIn('script->AddStoragePath(sourcePath, type == atMove ? SACCESS_READWRITE : SACCESS_READ)',builder)
  self.assertIn('keepBranchPaths',builder)
  self.assertIn('BOOL keepBranchPaths = FALSE',action)
 def test_delete_uses_exact_confirmation_and_no_branch_dos_fallback(self):
  action=function('fileswn8.cpp','void CFilesWindow::FilesAction(')
  self.assertIn('type != atDelete && ((type != atCopy && type != atMove)',action)
  self.assertIn('const std::wstring relative = GetItemRelativePathW(*f)',action)
  self.assertIn('str.SetW(messageText.c_str(), relative.c_str())',action)
  confirm=function('fileswn8.cpp','static void AppendConfirmDeleteExtInfo(')
  self.assertIn('panel->GetItemRelativePathW(*file)',confirm)
  builder=function('fileswn6.cpp','BOOL CFilesWindow::BuildScriptMain(')
  self.assertIn('!IsBranchView() && type == atDelete && selCount <= 1',builder)
 def test_branch_recycle_keeps_force_policy_and_checks_setup_failures(self):
  part=function('fileswn8.cpp','BOOL CFilesWindow::DeleteThroughRecycleBin(')
  self.assertIn('result = operation->SetOperationFlags(FOF_ALLOWUNDO | FOFX_RECYCLEONDELETE | FOFX_ADDUNDORECORD | FOFX_EARLYFAILURE)',part)
  self.assertIn('result = operation->SetOwnerWindow',part)
  self.assertIn('result = operation->GetAnyOperationsAborted',part)
  self.assertIn('GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(*file)) != file',part)
  self.assertLess(part.index('operation->DeleteItem'),part.index('operation->PerformOperations'))
  self.assertIn('!IsBranchView() && OperationRecycleDriveTypeW',source('fileswn8.cpp'))
  self.assertNotIn('S27c3: recycle modes',source('fileswn8.cpp'))
 def test_properties_only_shell_gate_and_whole_selection_fallback(self):
  shell=source('shellsup.cpp');action=shell[shell.index('void ShellAction('):shell.index('const char* ReturnNameFromParam')]
  self.assertIn('(action != saProperties && action != saCopyToClipboard && action != saCutToClipboard && action != saContextMenu && action != saLeftDragFiles && action != saRightDragFiles) || !useSelection || onlyPanelMenu',action)
  self.assertLess(action.index('action != saProperties'),action.index('if (panel->QuickSearchMode)'))
  properties=action[action.index('    case saProperties:'):action.index('    case saCopyToClipboard:')]
  self.assertIn('EnumFileNames, &data, panel->IsBranchView()',properties)
  self.assertIn('SHMultiFileProperties(object, 0)',properties)
  self.assertLess(properties.index('SHMultiFileProperties(object, 0)'),properties.index('object->Release()'))
  paths=function('shellsup.cpp','static std::vector<std::wstring> GetPanelShellPaths(')
  self.assertIn('GetBranchViewFileByCacheKey(data->Panel->GetItemCacheKeyPtr(file)) != &file',paths)
  self.assertIn('if (path.empty())',paths)
 def test_rename_requires_current_metadata_and_preserves_modal_identity(self):
  for marker in ('void CFilesWindow::RenameFile(', 'void CFilesWindow::QuickRenameBegin(', 'void CFilesWindow::RenameFileInternalW(', 'BOOL CFilesWindow::HandeQuickRenameWindowKey('):
   part=function('fileswn5.cpp',marker)
   self.assertIn('GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(*f)) != f',part)
  part=function('fileswn5.cpp','void CFilesWindow::RenameFile(')
  self.assertIn('GetBranchViewFileByCacheKey(renameIdentity.c_str()) != f',part)
  rename=function('fileswn5.cpp','void CFilesWindow::RenameFileInternalW(')
  self.assertEqual(rename.count('BranchItemRenamed(oldNameKey, *f, oldFullPath)'),3)
  self.assertEqual(rename.count('SleepIconCacheThread()'),3)
  self.assertEqual(rename.count('WakeupIconCacheThread()'),3)
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
   if file == 'fileswn7.cpp': self.assertIn('IsBranchView() ||',text)
   else: self.assertIn('IsBranchView() && (countSizeMode != 0 || copyToSelectedDirs',text)
   self.assertIn('target->IsBranchView()',text)
 def test_stop_is_present_in_the_actual_menu(self):
  self.assertIn("CM_BRANCH_STOP,",source("menu4.cpp"))
 def test_manual_only_refresh_and_detailed_layout(self):
  self.assertIn('if (automatic) return;',function('branch_view.cpp','void CFilesWindow::RefreshBranchView('))
  toggle=function('branch_view.cpp','void CFilesWindow::ToggleBranchView()')
  self.assertIn('SelectViewTemplate(2',toggle)
  self.assertIn('OriginalViewTemplate',toggle)
  self.assertIn('StopRefresh || SnooperSuspended || FilesActionInProgress',toggle)
 def test_program_broker_keeps_windows_execution_and_no_security_retry(self):
  execute=function('fileswn2.cpp','void CFilesWindow::Execute(')
  self.assertIn('BranchProgram::IsProgram(source.c_str())',execute)
  self.assertIn('LaunchBranchProgram(GetListBoxHWND(), source, directory)',execute)
  broker=source('branchprogram.h')
  self.assertIn('MUTZ_ISFILE | MUTZ_REQUIRESAVEDFILECHECK',broker)
  self.assertIn('zone != URLZONE_LOCAL_MACHINE',broker)
  self.assertIn('attachment->CheckPolicy()',broker)
  self.assertIn('attachment->Execute(owner, L"open", &child)',broker)
  self.assertIn('CreateProcessW(helper.c_str()',broker)
  self.assertNotIn('CreateProcessW(source',broker)
  self.assertNotIn('SEE_MASK_NOZONECHECKS',broker)
  helper=source('salopen/salopen.cpp')
  self.assertLess(helper.index('if (branchProgram)'),helper.index('// get file-name from file-mapping'))
 def test_broker_uses_pinned_handles_not_packet_paths_for_file_opens(self):
  execute=function('branchprogram.h','inline HRESULT Execute(')
  self.assertNotIn('CreateFileW(native',execute)
  self.assertNotIn('CreateFileW(parent',execute)
  self.assertIn('GetFinalPathNameByHandleW(file',execute)
  self.assertIn('GetFinalPathNameByHandleW(expectedDirectory',execute)
  self.assertIn('fileParentEnd - 1 != parentEnd',execute)
  self.assertIn('SameObject(expectedDirectory, actualDirectory)',execute)
  client=function('branchprogram.h','inline HRESULT Launch(')
  self.assertIn('PROC_THREAD_ATTRIBUTE_HANDLE_LIST',client)
  self.assertIn('CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT',client)
  self.assertLess(client.index('file.Reset(); folder.Reset();'),client.index('ResumeThread'))
  receive=function('branchprogram.h','inline DWORD Receive(')
  self.assertIn('output[i] = input[i]',receive)
  self.assertIn('Execute((HWND)packet->Owner, source',receive)
 def test_longpath_registry_is_diagnostic_only_and_help_is_local(self):
  query=function('fileswn2.cpp','static BranchLongPathSetting ReadBranchLongPathSetting()')
  self.assertIn('KEY_QUERY_VALUE | KEY_WOW64_64KEY',query)
  self.assertIn('type != REG_DWORD',query)
  self.assertNotIn('RegSet',query)
  self.assertNotIn('LongPathsEnabled',source('branchprogram.h'))
  dialog=function('fileswn2.cpp','static void ShowBranchLaunchFailure(')
  self.assertIn('MSGBOXEX_DEFBUTTON2',dialog)
  self.assertIn('== DIALOG_OK && longPath',dialog)
  self.assertIn('IDS_FILELAUNCH_LONGPATH_CAVEAT',dialog)
  self.assertIn('IDS_FILELAUNCH_LONGPATH_GUIDE',dialog)
  self.assertNotIn('ShellExecute',dialog)
 def test_launch_resource_fallback_and_all_language_records(self):
  import re
  ids=dict((name,int(value)) for name,value in re.findall(r'#define\s+(IDS_FILELAUNCH_\w+)\s+(\d+)',source('texts.rh2')))
  self.assertEqual(set(ids.values()),set(range(14516,14528)))
  english=dict(re.findall(r'\b(IDS_FILELAUNCH_\w+),\s*"([^"\r\n]*)"',source('lang/texts.rc2')))
  embedded=dict(re.findall(r'\b(IDS_FILELAUNCH_\w+),\s*"([^"\r\n]*)"',source('salamand.rc')))
  self.assertEqual(english,embedded)
  self.assertEqual(set(english),set(ids))
  self.assertIn('not the established cause',english['IDS_FILELAUNCH_LONGPATH_CAVEAT'])
  self.assertIn('does not change this setting',english['IDS_FILELAUNCH_LONGPATH_GUIDE'])
  for path in (ROOT.parent/'translations').glob('*/salamand.slt'):
   records=set(int(sid) for sid in re.findall(r'^(145\d+),[01],',path.read_text(encoding='utf-8-sig'),re.MULTILINE))
   self.assertTrue(set(ids.values()).issubset(records),str(path))
if __name__=='__main__': unittest.main()
