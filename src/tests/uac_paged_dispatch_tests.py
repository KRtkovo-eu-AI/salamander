# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Run identity-bearing paged dispatcher tests. | Verify real NTFS and pipe frames beyond one listing budget. | #731 | GPT-6 (Codex) |
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-paged-dispatch-') as temp:
        job = (ROOT / 'src/uacbroker/uacbroker_job_tests.cpp').read_text(encoding='utf-8-sig')
        if job.count('int wmain()\n{') != 1 or job.count('#include "uacbroker.cpp"') != 1:
            raise AssertionError('unexpected dispatcher fixture layout')
        job = job.replace('int wmain()\n{', 'int ExistingJobFixtureMain()\n{')
        job = job.replace('#include "uacbroker.cpp"',
                          '#include "' + (ROOT / 'src/uacbroker/uacbroker.cpp').as_posix() + '"')
        fixture = (ROOT / 'src/tests/uac_paged_dispatch_tests.cpp.in').read_text(encoding='utf-8-sig')
        if fixture.count('@@JOB_FIXTURE@@') != 1:
            raise AssertionError('missing dispatcher fixture placeholder')
        source = Path(temp) / 'test.cpp'
        source.write_text(fixture.replace('@@JOB_FIXTURE@@', job), encoding='utf-8')
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     str(source), '/Fe:' + str(exe),
                     '/link', '/MANIFEST:EMBED', 'advapi32.lib', 'ole32.lib', 'shell32.lib', 'uuid.lib', 'ntdll.lib'],
                    cwd=temp, env=env, timeout=35)
        if built.returncode:
            return built.returncode
        return run([str(exe)], cwd=temp, env=env, timeout=60).returncode


if __name__ == '__main__':
    raise SystemExit(main())
