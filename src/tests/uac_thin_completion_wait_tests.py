# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Exercise the scoped worker pause wait. | Verify broker-loss precedence over cancellation. | #731 | GPT-6 |
"""Run the actual wait using Windows events and a simulated transport."""
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
    path = ROOT / 'src/worker.cpp'
    before = hashlib.sha256(path.read_bytes()).hexdigest()
    worker = read('src/worker.cpp')
    start = worker.index('DWORD CThinCopyWorker::WaitTransferResume() const')
    actual = worker[start:worker.index('bool CThinCopyWorker::CancelBeforeElevation()', start)]
    start = worker.index('    bool WaitCompletionResume(int caption)')
    completion = worker[start:worker.index('    bool NativeOverwrite()', start)]
    template = (ROOT / 'src/tests/uac_thin_completion_wait_tests.cpp.in').read_text(encoding='utf-8')
    source = template.replace('@@WAIT_METHOD@@', actual).replace('@@COMPLETION_METHOD@@', completion)
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-completion-wait-', dir=ROOT.parent / 'builds') as temp:
        temp = Path(temp)
        cpp, exe = temp / 'test.cpp', temp / 'test.exe'
        cpp.write_text(source, encoding='utf-8')
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(cpp), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(path.read_bytes()).hexdigest() != before:
        raise RuntimeError('worker changed during test')
    print('worker.cpp SHA-256:', before)
    print('Extracted wait; simulated transport. Not full F5, real broker, or UAC.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
