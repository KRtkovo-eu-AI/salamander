# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise worker identity helpers with real files and injected Win32 error changes."""
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
    digest = hashlib.sha256(worker_path.read_bytes()).hexdigest()
    worker = worker_path.read_text(encoding='utf-8-sig')
    start = worker.index('static BOOL GetParallelFileIdentity(const std::wstring& path, DWORD& volumeSerial,\n                                    ULONGLONG& fileIndex)\n{')
    end = worker.index('static BOOL CanPrepareParallelDirectory(', start)
    template = (ROOT / 'src/tests/uac_identity_error_tests.cpp.in').read_text(encoding='utf-8')
    source_code = template.replace('@@HELPERS@@', worker[start:end])
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-identity-error-', dir=ROOT.parent / 'builds') as temp:
        source, exe = Path(temp) / 'test.cpp', Path(temp) / 'test.exe'
        source.write_text(source_code, encoding='utf-8')
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     str(source), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe), temp], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('worker.cpp changed during test')
    print('worker.cpp SHA-256:', digest)
    print('Real objects and extracted identity helpers; injected failures, not full F5 or UAC.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
