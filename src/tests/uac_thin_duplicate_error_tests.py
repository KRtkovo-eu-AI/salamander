# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Test actual thin duplication result handling. | Distinguish dead host and failed duplication from successful transfer. | #731 | GPT-6 |
"""Compile the actual thin transfer helper with deterministic Win32 substitutes."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    configuration = parser.parse_args().configuration
    source = read('src/uacbroker/uacbroker.cpp')
    start = source.index('DWORD DuplicateThinToHost(')
    end = source.index('\n\n// Request framing', start)
    template = (ROOT / 'src/tests/uac_thin_duplicate_error_tests.cpp.in').read_text(encoding='utf-8')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-duplicate-', dir=ROOT.parent / 'builds') as temp:
        test = Path(temp) / 'test.cpp'
        exe = Path(temp) / 'test.exe'
        test.write_text(template.replace('@@METHOD@@', source[start:end]), encoding='utf-8')
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(test), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    print('Actual helper; host liveness and duplication are simulated, not a full IPC/UAC test.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
