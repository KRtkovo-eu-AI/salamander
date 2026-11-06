# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual entire external unpack function with explicit process/temp I/O doubles."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 text=read('src/tests/external_unpack_context_tests.cpp.in');wide=read('src/common/widepath.cpp');h=read('src/pack.h');start=h.index('BOOL PackUniversalUncompress(')
 parts={'ACTION':block(read('src/fileswnd.h'),'enum CActionType')+';','WIDE':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')),'SNAPSHOT':block(read('src/fileactiondisktarget.h'),'struct CFilesActionDiskTarget')+';','DECL':h[start:h.index(';',start)+1],'ACTUAL':block(read('src/pack1.cpp'),'BOOL PackUniversalUncompress(HWND parent, const char* command')}
 for k,v in parts.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text
 assert parts['ACTUAL'].count('if (!contextIsCurrent())')==3
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='external-unpack-context-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual external unpack lifecycle:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,str(cpp),'/Fe:'+str(exe),'user32.lib'],[str(exe)]):
    result=run(cmd,cwd=d,env=env,timeout=40)
    if result.returncode:return result.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
