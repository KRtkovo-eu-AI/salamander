# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Verify actual F5 time-completion loops. | Cover session loss before and during Retry. | #731 | GPT-6 |
"""Extract real worker time loops; dialog, file I/O and connection are stubs."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    worker_path = ROOT / 'src/worker.cpp'
    before = hashlib.sha256(worker_path.read_bytes()).hexdigest()
    worker = read('src/worker.cpp')
    start = worker.index('FILETIME /*creation, lastAccess,*/ lastWrite;', worker.index('DoCopyFileLoopAsync(asyncPar, in, out,'))
    get_phase = worker[start:worker.index('                    if (moveReceipt)', start)]
    start = worker.index('BOOL ignoreSetFileTimeErr = FALSE;', start)
    set_phase = worker[start:worker.index('                        // The newly supported Overwrite Older retry', start)]
    template = (ROOT / 'src/tests/uac_thin_time_finish_tests.cpp.in').read_text(encoding='utf-8')
    source = template.replace('@@GET_PHASE@@', get_phase).replace('@@SET_PHASE@@', set_phase)
    if '@@' in source:
        raise RuntimeError('missing template replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-time-finish-', dir=ROOT.parent / 'builds') as temp:
        temp = Path(temp)
        cpp, exe = temp / 'test.cpp', temp / 'test.exe'
        cpp.write_text(source, encoding='utf-8')
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     str(cpp), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != before:
        raise RuntimeError('worker changed during test')
    print('worker.cpp SHA-256:', before)
    print('Extracted time loops; simulated failure, dialog and connection. Not full F5 or UAC.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
