# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Exercise the actual thin host-handle privilege boundary. | Check scoped privilege and token restoration paths with deterministic failures. | #731 | GPT-6 |
"""Exercise the actual OpenThinHostForDup body with simulated Win32 calls."""
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
    start = source.index('Handle OpenThinHostForDup(')
    end = source.index('\n\nDWORD CheckThinParentPath', start)
    template = (ROOT / 'src/tests/uac_thin_host_dup_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@METHOD@@', source[start:end])
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-host-dup-', dir=ROOT.parent / 'builds') as temp:
        test = Path(temp) / 'test.cpp'
        exe = Path(temp) / 'test.exe'
        test.write_text(code, encoding='utf-8')
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(test), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    print('Actual OpenThinHostForDup body; Windows process and token operations are simulated.')
    print('This does not establish real cross-account access, UAC, or security of the full protocol.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
