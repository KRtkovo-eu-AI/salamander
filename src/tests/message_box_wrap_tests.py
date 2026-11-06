# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the actual message-box conversion/wrap functions against Windows GDI."""
import argparse
from pathlib import Path
import shutil
import tempfile
from disk_selection_snapshot_tests import block,compiler_environment,read,run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args()
    source=read('src/msgbox.cpp')
    code=read('src/tests/message_box_wrap_tests.cpp.in')
    for marker,signature in (('CONVERT','static std::wstring MessageBoxTextToWide('),('WRAP','char* DuplicateStrAndInsertEOLs(')):
        code=code.replace('@@'+marker+'@@',block(source,signature))
    for rect in ('tR','iterRect'):
        assert f'DrawTextW(hDC, MessageBoxTextToWide(Text.Get()).c_str(), -1, &{rect},' in source
    assert 'DrawTextW(hDC, MessageBoxTextToWide(newText).c_str()' in source
    environment=compiler_environment();compiler=shutil.which('cl',path=environment.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='message-box-wrap-') as directory:
        directory=Path(directory);cpp=directory/'test.cpp';exe=directory/'test.exe';cpp.write_text(code,encoding='utf8')
        for configuration in ('Debug','Release') if args.configuration=='Both' else (args.configuration,):
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('Message box real GDI:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe),'gdi32.lib','user32.lib'],[str(exe)]):
                result=run(command,cwd=directory,env=environment,timeout=40)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
