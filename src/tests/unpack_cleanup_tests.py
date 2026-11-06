# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual scoped empty-directory cleanup on owned real long/Unicode trees and NTFS junctions."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 source=read('src/fileswn7.cpp');wide=read('src/common/widepath.cpp');text=read('src/tests/unpack_cleanup_tests.cpp.in')
 text=text.replace('@@ACTUAL@@',block(source,'static void RemoveEmptyUnpackDirectoriesW('))
 text=text.replace('@@CONVERSIONS@@','\n'.join(block(wide,m) for m in ('BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(', 'BOOL SalPathAppendW(')))
 assert '@@' not in text
 controller=block(source,'void CFilesWindow::Unpack(')
 for needle in ('RemoveEmptyUnpackDirectoriesW(newDirW);','nextFocus, NULL, pathBuffer.Capacity()', 'strlen(nextFocus) < sizeof(NextFocusName)', 'str.SetW(promptW.c_str(), fileNameW.c_str());'):
  assert needle in controller,needle
 assert 'RemoveEmptyDirs(newDir)' not in controller and 'lstrcpyn(newDir' not in controller and 'char fileName[MAX_PATH]' not in controller and 'char pathAlt[MAX_PATH]' not in controller
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-cleanup-') as temp:
  d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual Unpack cleanup:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe),str(d/'owned-fixtures')]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
