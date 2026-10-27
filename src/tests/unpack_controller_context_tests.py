# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the entire actual Unpack controller and backend dispatch with explicit callback/I/O doubles."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 text=read('src/tests/unpack_controller_context_tests.cpp.in');wide=read('src/common/widepath.cpp');header=read('src/zip.h');packers=read('src/packers.cpp');files=read('src/fileswn7.cpp');plugins=read('src/plugins.h')
 def decl(t,m):
  i=t.index(m);return t[i:t.index(';',i)+1]
 attached=block(read('src/mainwnd3.cpp'),'void CMainWindow::ClosePanelTab(CFilesWindow* panel,').split('    CPanelSide side =')[0]+'    (void)storeForReopen;RemovePanel(*this,panel);\n}'
 detached=block(read('src/mainwnd1.cpp'),'void CMainWindow::CloseDetachedTab(CFilesWindow* panel)').split('    CPanelSide side =')[0]+'    ++DetachedTransitions;ClosePanelTab(panel);\n}'
 move=block(read('src/fileswn6.cpp'),'BOOL CFilesWindow::MoveFiles(').split('        SetCurrentDirectory(source);')[0]+'        (void)target;(void)remapNameFrom;(void)remapNameTo;\n        ++MoveCalls;if(OnMove)OnMove();FilesActionInProgress=FALSE;return TRUE;\n    }\n    return FALSE;\n}'
 # Move prefix occurs before the counter/hook definitions: forward declarations in generated fixture.
 text=text.replace('@@MOVE_PREFIX@@','')
 text=text.replace('@@CONTROLLER@@',move+'\n@@CONTROLLER@@')
 parts={'GET_PREFERRED':block(header,'int GetPreferedUnpacker()'),'ACTION':block(read('src/fileswnd.h'),'enum CActionType')+';','WIDE':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathRemoveExtendedPrefixW(', 'std::wstring SalPathAddExtendedPrefixW(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(', 'const wchar_t* SalPathFindFileNameW(')),'SNAPSHOT':block(read('src/fileactiondisktarget.h'),'struct CFilesActionDiskTarget')+';','ROW':block(header,'class CUnpackerConfigData')+';','BACKEND':block(header,'struct CWholeArchiveUnpackerSnapshot')+';','DISPATCH_DECL':decl(header,'    BOOL ExecuteUnpacker('),'WRAPPER_DECL':decl(plugins,'    BOOL UnpackWholeArchive(CFilesWindow* panel,'),'METHODS':'\n'.join(block(packers,m) for m in ('bool CWholeArchiveUnpackerSnapshot::Capture(', 'bool CWholeArchiveUnpackerSnapshot::IsCurrent(', 'bool CWholeArchiveUnpackerSnapshot::IsVerifiedBranchSourceUnpacker(', 'bool CWholeArchiveUnpackerSnapshot::BindBranchTarget(', 'bool CWholeArchiveUnpackerSnapshot::ValidateBranchTarget(', 'bool CWholeArchiveUnpackerSnapshot::ValidateExternalPaths(', 'BOOL CUnpackerConfig::ExecuteUnpacker(')),'WRAPPER':block(read('src/plugins1.cpp'),'BOOL CPluginData::UnpackWholeArchive('),'VOLUMES':read('src/unpackvolumes.h').replace('#pragma once',''),'SOURCE':block(files,'static BOOL PrepareUnpackArchiveSource('),'CONTROLLER':block(files,'void CFilesWindow::Unpack('),'CLOSE_ATTACHED':attached,'CLOSE_DETACHED':detached}
 for k,v in parts.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-controller-context-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual whole-archive controller:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    result=run(cmd,cwd=d,env=env,timeout=40)
    if result.returncode:return result.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
