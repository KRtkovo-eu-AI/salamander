# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual SFX command and real Unicode edit-control path readback."""
import argparse
from pathlib import Path
import shutil,tempfile
from disk_selection_snapshot_tests import block,compiler_environment,read,run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args();code=read('src/tests/zip_sfx_paths_tests.cpp.in')
    pieces={'WIDE':block(read('src/plugins/shared/spl_diskselection.h'),'inline std::wstring WideFromPath('),'UTF8':block(read('src/plugins/shared/spl_diskselection.h'),'inline std::string Utf8FromWide('),'READ':block(read('src/plugins/zip/dialogs.cpp'),'static bool ReadSfxDialogPath('),'CREATE':block(read('src/plugins/zip/add_del.cpp'),'int CZipPack::CreateSFX(')}
    for name,text in pieces.items():code=code.replace('@@'+name+'@@',text)
    environment=compiler_environment();compiler=shutil.which('cl',path=environment.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='zip-sfx-paths-') as directory:
        directory=Path(directory);cpp=directory/'test.cpp';exe=directory/'test.exe';cpp.write_text(code,encoding='utf8')
        for configuration in ('Debug','Release') if args.configuration=='Both' else (args.configuration,):
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('ZIP actual SFX functions:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe),'user32.lib'],[str(exe)]):
                result=run(command,cwd=directory,env=environment,timeout=40)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
