# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual plugin wrapper and target snapshot; initialization/backend/registry doubles are explicit."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 text=read('src/tests/unpack_post_init_target_tests.cpp.in');wide=read('src/common/widepath.cpp');plugins=read('src/plugins.h');start=plugins.index('    BOOL UnpackArchive(CFilesWindow* panel,')
 pieces={'ACTION':block(read('src/fileswnd.h'),'enum CActionType')+';', 'WIDE':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')), 'SNAPSHOT':block(read('src/fileactiondisktarget.h'),'struct CFilesActionDiskTarget')+';', 'DECL':plugins[start:plugins.index(';',start)+1], 'WRAPPER':block(read('src/plugins1.cpp'),'BOOL CPluginData::UnpackArchive(')}
 for key,value in pieces.items():text=text.replace('@@'+key+'@@',value)
 assert '@@' not in text
 wrapper=pieces['WRAPPER'];assert wrapper.index('InitDLL(')<wrapper.index('diskTarget->IsCurrent(')<wrapper.index('CSalamanderForOperations sc(')<wrapper.index('PluginIfaceForArchiver.UnpackArchive(')
 dispatch=block(read('src/pack1.cpp'),'BOOL PackUncompress(')
 for owner in ('owner','stored','plugin'):assert 'return '+owner+'->UnpackArchive(panel, archiveFileName, pluginData, targetDir, archiveRoot, nextName, param, diskTarget);' in dispatch
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-post-init-') as temp:
  d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual unpack post-init target:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
