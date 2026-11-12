# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Run isolated thin progress foreground decision tests. | Test live source helper without changing the interactive desktop. | #731 | GPT-6 |
"""Compile the actual inline foreground helper with deterministic Win32 stubs."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, run


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release'),default='Debug')
    parser.add_argument('--source-root',type=Path,default=ROOT)
    args=parser.parse_args()
    header=args.source_root/'src/uac_thin_foreground.h'
    before=hashlib.sha256(header.read_bytes()).hexdigest()
    source=ROOT/'src/tests/uac_thin_foreground_helpers_tests.cpp'
    env=compiler_environment(); compiler=shutil.which('cl',path=env.get('PATH'))
    if not compiler: raise RuntimeError('MSVC cl.exe unavailable')
    flags=('/MTd','/Od','/D_DEBUG') if args.configuration=='Debug' else ('/MT','/O2','/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-progress-focus-',dir=ROOT.parent/'builds') as temp:
        exe=Path(temp)/'focus.exe'
        built=run([compiler,'/nologo','/std:c++17','/EHsc','/W4','/WX',*flags,'/I'+str(args.source_root/'src'),str(source),'/Fe:'+str(exe)],cwd=temp,env=env,timeout=60)
        if built.returncode: return built.returncode
        result=run([str(exe)],cwd=temp,env=env,timeout=20)
        if result.returncode: return result.returncode
    if hashlib.sha256(header.read_bytes()).hexdigest()!=before: raise RuntimeError('focus helper changed during test')
    print('Actual inline helper; Windows APIs are deterministic stubs. No desktop was touched.')
    print('Windows foreground restrictions, queue dispatch, and actual UAC require separate tests.')
    return 0

if __name__=='__main__': raise SystemExit(main())
