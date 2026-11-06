# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual host metadata I/O against real Windows property handlers and owned JPEGs."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
    source=read('src/salamdr4.cpp')
    markers=['static void TrimFileTag(', 'static BOOL ContainsFileTag(', 'HRESULT ReadFileTagsW(', 'BOOL IsFilePropertyWritableW(', 'HRESULT WriteFileStringVectorPropertyW(', 'HRESULT WriteFileTagsW(', 'HRESULT ReadFilePropertyTextW(', 'static HRESULT InitFilePropertyValueFromText(', 'HRESULT WriteFilePropertyTextW(']
    code=read('src/tests/branch_edit_properties_io_tests.cpp.in').replace('@@ACTUAL@@','\n'.join(block(source,m) for m in markers));assert '@@' not in code
    env=compiler_environment();compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-edit-properties-test-') as temp:
        directory=Path(temp);cpp=directory/'test.cpp';cpp.write_text(code,encoding='utf8');exe=directory/'test.exe'
        for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if a.configuration not in ('Both',c):continue
            print('Actual edit properties I/O:',c,flush=True)
            for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe),'ole32.lib','shell32.lib','propsys.lib','windowscodecs.lib','uuid.lib'],[str(exe)]):
                result=run(cmd,cwd=directory,env=env,timeout=40)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
