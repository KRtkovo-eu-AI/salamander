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
  for allowed in ('CM_OPEN','CM_VIEW','CM_ALTVIEW','CM_EDIT','CM_BRANCH_OPENPARENT','CM_RENAMEFILE','CM_COPYFILES','CM_MOVEFILES','CM_DELETEFILES','CM_PROPERTIES','CM_CLIPCOPY','CM_CLIPCUT','CM_CLIPPASTE','CM_PACK','CM_UNPACK','CM_CONTEXTMENU'):
   self.assertIn('case '+allowed+':',gate)
  self.assertIn('return false;',gate)
 def test_branch_direct_unpack_requires_qualified_backend_and_owned_deletion_sink(self):
  controller=function('fileswn7.cpp','void CFilesWindow::Unpack(')
  self.assertIn('((IsBranchView() || target->IsBranchView()) && !target->Is(ptDisk))) return;',controller)
  self.assertIn('&delArchiveWhenDone, TRUE)',controller)
  self.assertLess(controller.index('!backendSnapshot.IsVerifiedBranchSourceUnpacker()'),controller.index('CheckAndCreateDirectoryW('))
  self.assertIn('if (contextIsCurrent() && !sourceSnapshot.Branch)',controller)
  policy=function('packers.cpp','bool CWholeArchiveUnpackerSnapshot::IsVerifiedBranchSourceUnpacker(')
  self.assertIn('IsCurrent()',policy)
  self.assertIn('"zip\\\\zip.spl"',policy)
  self.assertIn('"7zip\\\\7zip.spl"',policy)
  for name,marker in (('packers.cpp','BOOL CUnpackerConfig::ExecuteUnpacker('),('plugins1.cpp','BOOL CPluginData::UnpackWholeArchive(')):
   guarded=function(name,marker)
   self.assertIn('diskTarget == NULL || !diskTarget->IsCurrent(MainWindow)',guarded)
   self.assertIn('(delArchiveWhenDone && archiveVolumes == NULL) || backendSnapshot == NULL || !backendSnapshot->IsVerifiedBranchSourceUnpacker()',guarded)
  dialog=function('dialogs3.cpp','void CUnpackDialog::EnableDelArcCheckbox(')
  self.assertIn('AllowDeleteArchive && IsUnpackerChoiceCurrent(i)',dialog)
  self.assertIn('EnablerFileOnDisk, file && onDisk',source('mainwnd1.cpp'))
  self.assertIn('onDisk = activePanel->Is(ptDisk)',source('mainwnd1.cpp'))
 def test_direct_unpack_branch_target_is_bound_to_existing_offered_directory(self):
  controller=function('fileswn7.cpp','void CFilesWindow::Unpack(')
  self.assertIn('!useOfferedDiskTarget || strcmp(path, offeredDiskTarget.c_str()) != 0',controller)
  self.assertLess(controller.index('!backendSnapshot.BindBranchTarget('),controller.index('CheckAndCreateDirectoryW('))
  self.assertIn('(!branchTargetBound || backendSnapshot.ValidateBranchTarget(&diskTarget, path))',controller)
  validate=function('packers.cpp','bool CWholeArchiveUnpackerSnapshot::ValidateBranchTarget(')
  for part in ('!target->DirectoryDefault','!target->IsCurrent(MainWindow)','BranchTargetDirectory != directory','SalPathAddExtendedPrefixW(','GetFileAttributesW(','FILE_ATTRIBUTE_DIRECTORY'):
   self.assertIn(part,validate)
  for name,marker in (('packers.cpp','BOOL CUnpackerConfig::ExecuteUnpacker('),('plugins1.cpp','BOOL CPluginData::UnpackWholeArchive(')):
   self.assertIn('!backendSnapshot->ValidateBranchTarget(diskTarget, targetDir)',function(name,marker))
 def test_path_display_preference_defaults_absolute_and_is_persisted(self):
  self.assertIn('BranchViewRelativePaths = FALSE;',source('dialogs4.cpp'))
  config=source('mainwnd2.cpp')
  self.assertIn('"Branch View Relative Paths"',config)
  for call in ('SetValue(actKey, CONFIG_BRANCHVIEW_RELATIVEPATHS_REG, REG_DWORD,','GetValue(actKey, CONFIG_BRANCHVIEW_RELATIVEPATHS_REG, REG_DWORD,'):
   self.assertIn(call,config)
  self.assertIn('Configuration.BranchViewRelativePaths = Configuration.BranchViewRelativePaths ? TRUE : FALSE;',config)
  appearance=function('dialogs5.cpp','void CCfgPagePanels::Transfer(')
  self.assertIn('if (GetDlgItem(HWindow, IDC_BRANCH_RELATIVE_PATHS) != NULL)',appearance)
  self.assertIn('ti.CheckBox(IDC_BRANCH_RELATIVE_PATHS, Configuration.BranchViewRelativePaths)',appearance)
 def test_path_preference_is_display_only(self):
  display=function('branch_view.cpp','std::wstring CFilesWindow::GetBranchViewDisplayDirectoryW(')
  self.assertIn('std::lock_guard<std::mutex> lock(BranchView->ItemsMutex)',display)
  self.assertIn('item->second.Directory',display)
  self.assertIn('item->second.RelativePath, Configuration.BranchViewRelativePaths',display)
  for getter in ('GetItemDirectoryW','GetItemFullPathW','GetItemIdentityW','GetItemRelativePathW'):
   self.assertNotIn('BranchViewRelativePaths',function('branch_view.cpp','std::wstring CFilesWindow::'+getter+'('))
  paint=source('fileswn4.cpp')
  self.assertIn('const std::wstring parent = GetBranchViewDisplayDirectoryW(*f)',paint)
  transfer=paint.split('TransferPanelWindow = this;',1)[1].split('TransferFileData = f;',1)[0]
  self.assertIn('GetItemDirectoryW(*f)',transfer)
  self.assertNotIn('GetBranchViewDisplayDirectoryW',transfer)
  self.assertIn('GetBranchViewDisplayDirectoryW',function('fileswn9.cpp','static void WINAPI InternalGetBranchPath()'))
 def test_path_preference_lives_in_configuration_not_panel_menus(self):
  self.assertNotIn('BRANCHRELATIVEPATHS',source('menu4.cpp'))
  self.assertIn('case CM_CONFIGURATION:',function('mainwnd3.cpp','static bool BranchReadOnlyCommandAllowed('))
  onconfig=function('mainwnd3.cpp','void CMainWindow::OnConfiguration(')
  self.assertIn('oldBranchViewRelativePaths != Configuration.BranchViewRelativePaths',onconfig)
  self.assertIn('InvalidateRect(LeftPanel->GetListBoxHWND(), NULL, FALSE)',onconfig)
  self.assertIn('InvalidateRect(RightPanel->GetListBoxHWND(), NULL, FALSE)',onconfig)
  self.assertLess(onconfig.index('BeginStopRefresh()'),onconfig.index('dlg.Execute('))
  self.assertGreater(onconfig.index('EndStopRefresh()'),onconfig.index('dlg.Execute('))
  for path in (ROOT.parent/'translations').glob('*/salamand.slt'):
   text=path.read_text(encoding='utf-8-sig');page=text.split('[DIALOG 1105]',1)[1].split('\n[',1)[0]
   self.assertEqual(page.count('14528,'),1,str(path))
 def test_tab_and_history_restores_keep_experimental_gate_and_exact_location(self):
  restore=function('branch_view.cpp','void CFilesWindow::RestoreBranchViewState(')
  self.assertIn('!Salamander::BranchView::ExperimentEnabled() || !state.Enabled || !Is(ptDisk)',restore)
  gate=function('mainwnd3.cpp','static bool BranchReadOnlyCommandAllowed(')
  for command in ('CM_DUPLICATETAB','CM_LEFT_DUPLICATETAB','CM_RIGHT_DUPLICATETAB','CM_LEFT_DUPLICATETABTORIGHT','CM_RIGHT_DUPLICATETABTOLEFT','CM_REOPENTAB','CM_LEFT_REOPENTAB','CM_RIGHT_REOPENTAB','CM_ACTIVEBACK','CM_ACTIVEFORWARD','CM_LBACK','CM_LFORWARD','CM_RBACK','CM_RFORWARD'):
   self.assertIn('case '+command+':',gate)
  self.assertIn('return false;',gate)
  for signature,condition in (('CFilesWindow* CMainWindow::CreateDuplicatePanelTab(','sourceDiskPath == newPanel->GetPathW()'),('bool CMainWindow::CommandReopenClosedTab(','entry.DiskPathW == panel->GetPathW()')):
   route=function('mainwnd3.cpp',signature)
   self.assertLess(route.index(condition),route.index('RestoreBranchViewState('))
  capture=function('fileswn2.cpp','void CFilesWindow::RefreshPathHistoryData(')
  self.assertIn('Files->Count + Dirs->Count == 0',capture)
  self.assertIn('const CBranchViewRestoreState branch = CaptureBranchViewState()',capture)
  same=function('salamdr3.cpp','BOOL CPathHistoryItem::IsTheSamePath(')
  self.assertIn('::IsTheSamePath(PathOrArchiveOrFSName, item.PathOrArchiveOrFSName)',same)
  self.assertNotIn('MAX_PATH',same)
  save=function('salamdr3.cpp','void CPathHistory::SaveToRegistry(')
  load=function('salamdr3.cpp','void CPathHistory::LoadFromRegistry(')
  self.assertIn('saveNavigationState && item->Type == 0',save)
  self.assertIn('type == 0 && loadNavigationState',load)
  self.assertIn('width >= 40 && width <= 10000',load)
  self.assertNotIn('RestoreBranchViewState',load)
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
  self.assertIn('(type != atCopy && type != atMove && type != atDelete && type != atChangeAttrs) || GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(*oneFile)) != oneFile',builder)
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
 def test_pack_binds_qualified_disk_panels_and_backend(self):
  pack=function('fileswn7.cpp','void CFilesWindow::Pack(')
  self.assertIn('CFilesActionDiskTarget::FindLive(MainWindow, this) == NULL',pack)
  self.assertIn('CFilesActionDiskTarget::FindLive(MainWindow, target) == NULL',pack)
  self.assertIn('qualifiedPack = IsBranchView() || target->IsBranchView()',pack)
  self.assertIn('packContext.Capture(MainWindow, this, target)',pack)
  self.assertLess(pack.index('packContext.Capture('),pack.index('dlg.Execute()'))
  self.assertLess(pack.index('!packContext.BindBackend('),pack.index('if (SalGetFullName(fileBuf'))
  self.assertIn('packContext.Target.DefaultPath.c_str()',pack)
  self.assertIn('data.ContextIsCurrent, data.Context',pack)
  self.assertIn('!Is(ptDisk) || !target->Is(ptDisk)',pack)
  self.assertNotIn('delFilesAfterPacking == 1',pack[:pack.index('CALL_STACK_MESSAGE4')])
  self.assertLess(pack.index('!IsVerifiedBranchPacker()'),pack.index('if (SalGetFullName(fileBuf'))
  self.assertIn('data.SourcePanel = this',pack)
  self.assertIn('data.ExactFileSelection = Is(ptDisk)',pack)
 def test_move_cleanup_follows_success_and_preserves_cancel(self):
  zip_source=source('plugins/zip/add.cpp')
  self.assertIn('if (Move && !ErrorID && !UserBreak)',zip_source)
  self.assertIn('ErrorID = CleanUpSource();',zip_source)
  seven=source('plugins/7zip/7zip.cpp')
  self.assertLess(seven.index('BOOL ret = client.Update('),seven.index('if (move && ret)'))
  self.assertIn('if (fi->CanDelete && !fi->IsDir)',seven)
  self.assertIn('if (!SafeDeleteFile(name2, fileSilent))',seven)
 def test_stop_is_present_in_the_actual_menu(self):
  self.assertIn("CM_BRANCH_STOP,",source("menu4.cpp"))
 def test_scheduled_refresh_preserves_selected_layout_and_stop(self):
  refresh=function('branch_view.cpp','void CFilesWindow::RefreshBranchView(')
  self.assertNotIn('if (automatic) return;',refresh)
  self.assertIn('BranchView->Refresh.Request(now, automatic != FALSE)',refresh)
  self.assertIn('if (BranchView->Refresh.Due(now)) StartBranchViewScan(this)',refresh)
  self.assertIn('PostMessage(HWindow, WM_USER_REFRESH_DIR, TRUE, stamp)',function('fileswn7.cpp','void CFilesWindow::AcceptChangeOnPathNotification('))
  queue=function('mainwnd3.cpp','void CMainWindow::PostChangeOnPathNotification(')
  self.assertIn('ChangeNotifArray.push_back(CChangeNotifData{path, includingSubdirs})',queue)
  self.assertIn('catch (const std::bad_alloc&)',queue)
  self.assertNotIn('lstrcpyn(',queue)
  self.assertIn('pathStorage.swap(item.Path)',source('mainwnd3.cpp'))
  self.assertIn('owned until every subscriber callback returns',source('mainwnd3.cpp'))
  toggle=function('branch_view.cpp','void CFilesWindow::ToggleBranchView()')
  self.assertNotIn('SelectViewTemplate(',toggle)
  self.assertIn('OriginalViewTemplate',toggle)
  self.assertIn('StopRefresh || SnooperSuspended || FilesActionInProgress',toggle)
 def test_display_and_property_sort_commands_use_only_existing_ranges(self):
  gate=function('mainwnd3.cpp','static bool BranchReadOnlyCommandAllowed(')
  for first,last in (('ACTIVEMODE_1','ACTIVEMODE_10'),('LEFTMODE_1','LEFTMODE_10'),('RIGHTMODE_1','RIGHTMODE_10'),('ACTIVEEXTRAMODE_MIN','ACTIVEEXTRAMODE_MAX'),('LEFTEXTRAMODE_MIN','LEFTEXTRAMODE_MAX'),('RIGHTEXTRAMODE_MIN','RIGHTEXTRAMODE_MAX'),('LEFTSORTBY_MIN','LEFTSORTBY_MAX'),('RIGHTSORTBY_MIN','RIGHTSORTBY_MAX')):
   self.assertIn('command >= CM_'+first+' && command <= CM_'+last,gate)
  self.assertNotIn('command >= CM_VIEWWITH_MIN',gate)
  self.assertNotIn('command >= CM_EDITWITH_MIN',gate)
  self.assertIn('return false;',gate)
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
