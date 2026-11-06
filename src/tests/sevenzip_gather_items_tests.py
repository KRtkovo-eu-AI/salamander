# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual whole-archive gather/append/record code; directory, mask, array and UI facades are explicit."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import ROOT,read,block,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 host=read('src/plugins/7zip/7zip.cpp');whole=block(host,'BOOL CPluginInterfaceForArchiver::UnpackWholeArchive(')
 assert 'malloc(SAL_MAX_PATH)' in whole and 'free(archivePath);' in whole
 assert 'if (GatherItems(dir, modmask, &archiveItems, archivePath, SAL_MAX_PATH) != OPER_CANCEL)' in whole
 assert whole.index('if (GatherItems(')<whole.index('const int result = client2->Decompress(')<whole.index('free(archivePath);')
 data=read('src/tests/sevenzip_gather_items_tests.cpp.in')
 for k,v in {'RECORD':block(read('src/plugins/7zip/7zclient.h'),'struct CArchiveItemInfo')+';','APPEND':block(read('src/salamdr3.cpp'),'BOOL SalPathAppend('),'GATHER':block(host,'int GatherItems(')}.items():data=data.replace('@@'+k+'@@',v)
 assert '@@' not in data
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'));vendor=ROOT/'src/plugins/7zip/7za/CPP'
 with tempfile.TemporaryDirectory(prefix='sevenzip-gather-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(data,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual 7-Zip whole-archive collection:',c,flush=True)
   cmd=[cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,'/I'+str(vendor),str(cpp),str(vendor/'Common/MyString.cpp'),str(vendor/'Common/MyVector.cpp'),str(vendor/'Common/IntToString.cpp'),str(vendor/'Common/StringConvert.cpp'),'/Fe:'+str(exe),'/link','user32.lib','oleaut32.lib']
   for call in (cmd,[str(exe)]):
    r=run(call,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
