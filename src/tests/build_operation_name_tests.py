# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""S19 acceptance tests; reuse the pinned donor's production-extraction driver."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
from disk_file_size_paths_tests import block, compiler_environment, read

ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--configuration',choices=('Debug','Release'))
    args=parser.parse_args()
    wide=read('src/common/widepath.cpp')
    production=read('src/salamdr1.cpp');start=production.index('char* BuildName(')
    # BuildName contains a Translator example with unbalanced braces in a
    # comment; preserve the actual function up to its explicit section marker.
    build_name=production[start:production.index('\n// ****************************************************************************',start)]
    code=read('src/tests/build_operation_name_tests.cpp.in').replace('@@WIDE_HELPERS@@',block(wide,'BOOL IsValidPathUtf8Text(')+'\n'+block(wide,'std::wstring SalMultiByteToWidePath(')).replace('@@BUILD_NAME@@',build_name)
    assert '@@' not in code
    env=compiler_environment();compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='build-operation-name-') as directory:
        base=Path(directory);source=base/'test.cpp';source.write_text(code,encoding='utf-8');exe=base/'test.exe'
        for config,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration and args.configuration!=config:continue
            print('Actual BuildName',config,flush=True)
            subprocess.run([compiler,'/nologo','/EHsc','/std:c++17','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS','/utf-8',*flags,str(source),'/Fe:'+str(exe)],cwd=base,env=env,check=True,timeout=30)
            subprocess.run([str(exe)],cwd=base,env=env,check=True,timeout=20)
if __name__=='__main__':main()
