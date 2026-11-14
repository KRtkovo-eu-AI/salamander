# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the actual synchronous transfer loop with real local file handles.

This fixture adapts the surrounding operation, dialog and thin-session APIs;
it is not the complete DoCopyFile worker, broker, UAC or VM acceptance.
"""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    config = parser.parse_args().configuration
    worker_path = ROOT / 'src/worker.cpp'
    header_path = ROOT / 'src/worker.h'
    before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in (worker_path, header_path)}
    worker = worker_path.read_text(encoding='utf-8-sig')
    header = header_path.read_text(encoding='utf-8-sig')
    def section(start, end):
        begin = worker.index(start)
        return worker[begin:worker.index(end, begin)]
    buffer = re.search(r'^#define ASYNC_COPY_BUF_SIZE \([^\r\n]*', header, re.MULTILINE)
    if not buffer:
        raise RuntimeError('native async buffer definition missing')
    template = (ROOT / 'src/tests/uac_thin_sync_loop_tests.cpp.in').read_text(encoding='utf-8')
    code = (template.replace('@@BUFFER@@', buffer.group(0))
            .replace('@@ASYNC@@', section('struct CAsyncCopyParams\n{', '// **********************************************************************************\n'))
            .replace('@@TAIL@@', section('BOOL SalSetFilePointer(HANDLE file, const CQuadWord& offset)',
                                         '// copies ADS into the newly created file/directory'))
            .replace('@@LOOP@@', section('void DoCopyFileLoopOrig(', 'enum CCopy_BlkState')))
    if '@@' in code:
        raise RuntimeError('unexpanded test placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='thin-sync-loop-', dir=ROOT.parent / 'builds') as temp:
        source, exe = Path(temp) / 'test.cpp', Path(temp) / 'test.exe'
        source.write_text(code, encoding='utf-8')
        result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                      '/wd4201', '/wd4244', *flags, str(source), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if result.returncode:
            return result.returncode
        result = run([str(exe), temp], cwd=temp, env=env, timeout=30)
        if result.returncode:
            return result.returncode
    for path, digest in before.items():
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise RuntimeError(str(path) + ' changed during test')
    print('worker.cpp SHA-256:', before[worker_path])
    print('Actual extracted synchronous loop and native tail check; local NTFS I/O with adapted surrounding APIs.')
    print('Not complete DoCopyFile, actual privileged broker, UAC or VM acceptance.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
