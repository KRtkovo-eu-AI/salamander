# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Run thin streamed identity tests. | Verify real NTFS and interruption beyond 4096 entries. | #731 | GPT-6 |
import argparse
import shutil
import tempfile
from pathlib import Path

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-stream-') as temp:
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     str(ROOT / 'src/tests/uac_thin_streamed_child_id_tests.cpp'), '/Fe:' + str(exe),
                     '/link', 'ntdll.lib', 'advapi32.lib'], cwd=temp, env=env, timeout=35)
        if built.returncode:
            return built.returncode
        return run([str(exe), temp], cwd=temp, env=env, timeout=60).returncode


if __name__ == '__main__':
    raise SystemExit(main())
