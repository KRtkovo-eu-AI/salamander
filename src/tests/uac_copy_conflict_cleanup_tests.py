# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify actual worker conflict cleanup with real local handles, not UAC or the full worker."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    worker_path = ROOT / 'src/worker.cpp'
    before = hashlib.sha256(worker_path.read_bytes()).hexdigest()
    worker = worker_path.read_text(encoding='utf-8-sig')
    start = worker.index('static BOOL ReturnCopyConflictChanged(')
    end = worker.index('\n}\n', start) + 2
    copy_start = worker.index('BOOL DoCopyFile(COperation* op,')
    copy_end = worker.index('BOOL DoMoveFile(COperation* op,', copy_start)
    body = worker[copy_start:copy_end]
    if body.count('return ReturnCopyConflictChanged(in, dlgData);') != 11 or 'dlgData.ConflictTargetChanged = TRUE;' in body:
        raise RuntimeError('DoCopyFile conflict returns changed; reassess source-handle ownership')
    template = (ROOT / 'src/tests/uac_copy_conflict_cleanup_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@HELPER@@', worker[start:end])
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-conflict-cleanup-', dir=ROOT.parent / 'builds') as temp:
        source, exe = Path(temp) / 'test.cpp', Path(temp) / 'test.exe'
        source.write_text(code, encoding='utf-8')
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     str(source), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe), temp], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != before:
        raise RuntimeError('worker.cpp changed during test')
    print('worker.cpp SHA-256:', before)
    print('Extracted conflict cleanup with real handles; full worker, broker and UAC are not exercised.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
