# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise actual progress completion branch in isolation. | Verify queue priority and foreground guard without touching other windows. | #731 | GPT-6 |
"""Compile the real WM_USER_PROGRDLGEND case with deterministic OS and queue stubs."""
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
    dialog=args.source_root/'src/dialogs.cpp'; header=args.source_root/'src/uac_thin_foreground.h'
    before={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in (dialog,header)}
    data=dialog.read_text(encoding='utf-8')
    start=data.index('    case WM_USER_PROGRDLGEND:'); end=data.index('\n    case WM_COMMAND:',start)
    case=data[start:end]
    template=(ROOT/'src/tests/uac_progress_end_native_tests.cpp.in').read_text(encoding='utf-8')
    code=template.replace('@@CASE@@',case)
    if '@@' in code: raise RuntimeError('unfilled placeholder')
    env=compiler_environment(); compiler=shutil.which('cl',path=env.get('PATH'))
    if not compiler: raise RuntimeError('MSVC cl.exe unavailable')
    flags=('/MTd','/Od','/D_DEBUG') if args.configuration=='Debug' else ('/MT','/O2','/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-progress-end-',dir=ROOT.parent/'builds') as temp:
        source=Path(temp)/'test.cpp';exe=Path(temp)/'test.exe';source.write_text(code,encoding='utf-8')
        built=run([compiler,'/nologo','/std:c++17','/EHsc','/W4','/WX',*flags,'/I'+str(args.source_root/'src'),str(source),'/Fe:'+str(exe)],cwd=temp,env=env,timeout=60)
        if built.returncode:return built.returncode
        result=run([str(exe)],cwd=temp,env=env,timeout=20)
        if result.returncode:return result.returncode
    if any(hashlib.sha256(p.read_bytes()).hexdigest()!=before[p] for p in before): raise RuntimeError('production source changed during test')
    print('Actual WM_USER_PROGRDLGEND branch and inline helper; OS and queue are deterministic stubs.')
    print('No desktop was touched. Actual Windows dialog-manager behavior and UAC remain unverified.')
    return 0

if __name__=='__main__':raise SystemExit(main())
