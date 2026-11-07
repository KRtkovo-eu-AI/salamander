# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the actual bounded volume plan/wide delete helper; own Win32 fixtures only."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 wide=read('src/common/widepath.cpp');text=read('src/tests/unpack_volume_paths_tests.cpp.in')
 text=text.replace('@@WIDE@@','\n'.join(block(wide,m) for m in ('BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(', 'std::wstring SalPathRemoveExtendedPrefixW(', 'BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')))
 text=text.replace('@@ACTUAL@@',read('src/unpackvolumes.h').replace('#pragma once',''));assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-volume-paths-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual unpack volume paths:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe),str(d/('owned-'+c))]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
