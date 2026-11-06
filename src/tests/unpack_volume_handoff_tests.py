# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual bundled ZIP/7Zip whole wrappers and ZIP sort with explicit engine/SDK sink doubles."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 text=read('src/tests/unpack_volume_handoff_tests.cpp.in');zip=read('src/plugins/zip/main.cpp');seven=read('src/plugins/7zip/7zip.cpp')
 for key,value in {'ZIP_SORT':block(zip,'void QuickSortArcVolumes('),'ZIP':block(zip,'BOOL CPluginInterfaceForArchiver::UnpackWholeArchive('),'SEVEN':block(seven,'BOOL CPluginInterfaceForArchiver::UnpackWholeArchive(')}.items():text=text.replace('@@'+key+'@@',value)
 assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-volume-handoff-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual whole-archive volume handoff:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
