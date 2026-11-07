# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Pack context, dispatcher, post-init wrapper and owned wide overwrite; explicit registry/plugin facade."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');p.add_argument('--negative-control',action='store_true');a=p.parse_args()
 h=read('src/zip.h');plugins=read('src/plugins.h');files=read('src/fileswn7.cpp');wide=read('src/common/widepath.cpp')
 def decl(s,m):
  i=s.index(m);return s[i:s.index(';',i)+1]
 wrapper=block(read('src/plugins1.cpp'),'BOOL CPluginData::PackToArchive(')
 if a.negative_control:
  first=wrapper.index('if (validateContext != NULL && !validateContext(context))');second=wrapper.index('if (validateContext != NULL && !validateContext(context))',first+1)
  guard=block(wrapper[second:],'if (validateContext != NULL && !validateContext(context))');wrapper=wrapper[:second]+wrapper[second:].replace(guard,'',1)
 text=read('src/tests/pack_context_tests.cpp.in')
 parts={'ACTION':block(read('src/fileswnd.h'),'enum CActionType')+';', 'WIDE':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathRemoveExtendedPrefixW(', 'std::wstring SalPathAddExtendedPrefixW(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')), 'DISK_TARGET':block(read('src/fileactiondisktarget.h'),'struct CFilesActionDiskTarget')+';', 'PACK_CONTEXT':block(read('src/fileactionpackcontext.h'),'struct CFilesActionPackContext')+';', 'GETTERS':'\n'.join(block(h,m) for m in ('int GetPreferedPacker()', 'int GetPackersCount()', 'int GetPackerType(int index)')), 'PLUGIN_DECL':decl(plugins,'    BOOL PackToArchive(CFilesWindow* panel,'), 'DISPATCH_DECL':decl(h,'    BOOL ExecutePacker(CFilesWindow* panel,'), 'WRAPPER':wrapper, 'DISPATCH':block(read('src/packers.cpp'),'BOOL CPackerConfig::ExecutePacker('), 'REMOVE_ARCHIVE':block(files,'static BOOL RemoveQualifiedPackArchive(')}
 # The overwrite helper is a function-try-block; retain its allocation failure handler.
 start=files.index('static BOOL RemoveQualifiedPackArchive(');finish=files.index('// S27e2 staged activation',start);parts['REMOVE_ARCHIVE']=files[start:finish]
 for k,v in parts.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='pack-context-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for config,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',config):continue
   print('Actual qualified Pack pipeline:',config,flush=True);fixtures=d/config;fixtures.mkdir()
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe),str(fixtures)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
