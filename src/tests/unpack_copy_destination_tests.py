# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual archive-copy destination helpers and controller placement; no archive engine facade."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 source=read('src/fileswn7.cpp');wide=read('src/common/widepath.cpp');text=read('src/tests/unpack_copy_destination_tests.cpp.in')
 text=text.replace('@@ACTUAL@@','\n'.join(block(source,m) for m in ('static BOOL IsUnpackDirectoryComponentInvalid(', 'static BOOL CopyUnpackDestination(', 'static BOOL ReadUnpackDiskDestination(')))
 text=text.replace('@@CONVERSIONS@@','\n'.join(block(wide,m) for m in ('std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')))
 assert '@@' not in text
 controller=block(source,'void CFilesWindow::UnpackZIPArchive(')
 for needle in ('CPathBuffer changesRootBuffer;', 'CPathBuffer newDirsBuffer;', 'CopyUnpackDestination(tgtPath, path, pathBuffer.Capacity())', 'ReadUnpackDiskDestination(target->GetPathW()', 'NULL, NULL, pathBuffer.Capacity()', 'SalCreateDirectoryExW(newDirsW.c_str()', 'RemoveEmptyUnpackDirectoriesW(', 'if (strlen(path) >= MAX_PATH)', 'if (IsBranchView() || target != NULL && target->IsBranchView()) return;'):
  assert needle in controller,needle
 for forbidden in ('lstrcpyn(path, tgtPath', 'char newDirs[MAX_PATH]', 'char changesRoot[MAX_PATH]', 'SalPathAddBackslash(path, MAX_PATH + 200)', 'CreateDirectory(newDirs', 'RemoveEmptyDirs(newDirs'):
  assert forbidden not in controller,forbidden
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-copy-destination-') as temp:
  d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual unpack copy destination:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
