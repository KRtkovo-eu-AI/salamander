# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual backend snapshot/dispatcher/wrapper; plugin and registry doubles are explicit."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 data=read('src/tests/unpack_backend_binding_tests.cpp.in');wide=read('src/common/widepath.cpp');header=read('src/zip.h');plugins=read('src/plugins.h');packers=read('src/packers.cpp')
 def decl(text,marker):
  start=text.index(marker);return text[start:text.index(';',start)+1]
 parts={'ACTION':block(read('src/fileswnd.h'),'enum CActionType')+';','WIDE':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')),'SNAPSHOT':block(read('src/fileactiondisktarget.h'),'struct CFilesActionDiskTarget')+';','ROW':block(header,'class CUnpackerConfigData')+';','BACKEND':block(header,'struct CWholeArchiveUnpackerSnapshot')+';','DECL':decl(plugins,'    BOOL UnpackWholeArchive(CFilesWindow* panel,'),'DISPATCH_DECL':decl(header,'    BOOL ExecuteUnpacker('),'METHODS':'\n'.join(block(packers,m) for m in ('bool CWholeArchiveUnpackerSnapshot::Capture(', 'bool CWholeArchiveUnpackerSnapshot::IsCurrent(', 'bool CWholeArchiveUnpackerSnapshot::ValidateExternalPaths(', 'BOOL CUnpackerConfig::MoveUnpacker(', 'BOOL CUnpackerConfig::ExecuteUnpacker(')),'WRAPPER':block(read('src/plugins1.cpp'),'BOOL CPluginData::UnpackWholeArchive(')}
 for k,v in parts.items():data=data.replace('@@'+k+'@@',v)
 assert '@@' not in data
 assert parts['WRAPPER'].count('backendSnapshot->IsCurrent(this)')==2
 assert parts['METHODS'].index('UnpackerConfig.Unpackers[Index] != Entry')<parts['METHODS'].index('const CUnpackerConfigData* entry = UnpackerConfig.Unpackers[Index]')
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-backend-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(data,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual whole-archive backend binding:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
