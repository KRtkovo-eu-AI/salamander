# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual ZIP volume recorder and shared array Add with allocation instrumentation."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 common=read('src/plugins/zip/common2.cpp');extract=read('src/plugins/zip/extract.cpp');main=read('src/plugins/zip/main.cpp')
 assert 'return RecordArchiveVolume();' in block(common,'int CZipCommon::ChangeDisk(')
 assert 'ErrorID = RecordArchiveVolume();' in block(extract,'int CZipUnpack::UnpackWholeArchive(')
 assert 'ArchiveVolumes = archiveVolumes;\n    ArchiveVolumesComplete = true;' in read('src/plugins/zip/common.cpp')
 wrapper=block(main,'BOOL CPluginInterfaceForArchiver::UnpackWholeArchive(')
 assert wrapper.index('!unpack.ArchiveVolumesComplete')<wrapper.index('QuickSortArcVolumes(')
 assert 'ArchiveVolumes->Add(_strdup' not in common+extract
 text=read('src/tests/zip_volume_collection_tests.cpp.in').replace('@@ARRAY_ADD@@',block(read('src/plugins/shared/lukas/array2.h'),'int TDirectArray2<DATA_TYPE>::Add(')).replace('@@RECORDER@@',block(common,'int CZipCommon::RecordArchiveVolume('));assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='zip-volume-collection-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual ZIP internal collection:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
