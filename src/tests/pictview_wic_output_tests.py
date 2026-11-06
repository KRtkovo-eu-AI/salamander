# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the actual WIC prefix and stream/callback adapter with a deterministic encoder dependency."""
import argparse
from pathlib import Path
import shutil
import tempfile
from disk_selection_snapshot_tests import ROOT, block, compiler_environment, run

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');args=parser.parse_args()
    source=(ROOT/'src/plugins/pictview/wic/WicBackend.cpp').read_text(encoding='utf-8-sig')
    code=(ROOT/'src/tests/pictview_wic_output_tests.cpp.in').read_text(encoding='utf-8-sig')
    code=code.replace('@@PV_HEADER@@',(ROOT/'src/plugins/pictview/lib/pvw32dll.h').as_posix()).replace('@@PREFIX@@',block(source,'std::wstring AddExtendedPrefixIfNeeded(')).replace('@@ADAPTER@@',block(source,'static PVCODE SaveFrameToCallback('))
    assert '@@' not in code
    environment=compiler_environment();compiler=shutil.which('cl',path=environment['PATH'])
    with tempfile.TemporaryDirectory(prefix='pictview-wic-output-') as temporary:
        directory=Path(temporary);cpp=directory/'test.cpp';cpp.write_text(code,encoding='utf-8');exe=directory/'test.exe'
        for configuration in ('Debug','Release'):
            if args.configuration!='Both' and configuration!=args.configuration:continue
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('Actual WIC output adapter:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/wd4100',*flags,str(cpp),'/Fe:'+str(exe),'ole32.lib'],[str(exe)]):
                result=run(command,cwd=directory,env=environment)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
