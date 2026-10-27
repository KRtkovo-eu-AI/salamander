# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual 7Zip Decompress and completion predicate; engine/SDK facades are explicit."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import read, block, compiler_environment, run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 data=read('src/tests/sevenzip_unpack_completion_tests.cpp.in')
 for k,v in {'COMPLETION':block(read('src/plugins/7zip/extract.h'),'    bool IsSelectionComplete() const'),'DECOMPRESS':block(read('src/plugins/7zip/7zclient.cpp'),'int C7zClient::Decompress(')}.items():data=data.replace('@@'+k+'@@',v)
 assert '@@' not in data
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='sevenzip-completion-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(data,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual 7Zip completion:',c,flush=True)
   for call in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(call,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
