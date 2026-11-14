# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Run an isolated host-process crash case. | Check that the real dispatcher exits when its peer dies. | #731 | GPT-6 |
# | 2026-11-14 | Run the external dispatcher crash case. | Test real cross-process handle lifetime separately from UAC. | #731 | GPT-6 |
# | 2026-11-12 | Test the experimental thin source-parent fallback. | Run real NTFS and pipe cases without pretending the token is elevated. | #731 | GPT-6 |
"""Build and run the real job dispatcher with experimental thin pinning."""

import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    crash_mode = parser.add_mutually_exclusive_group()
    crash_mode.add_argument('--process-crash-only', action='store_true')
    crash_mode.add_argument('--host-crash-only', action='store_true')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-parent-') as temp:
        exe = Path(temp) / 'uacbroker_thin_parent_job_tests.exe'
        source = ROOT / 'src/uacbroker/uacbroker_thin_parent_job_tests.cpp'
        try:
            build = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                         *flags, str(source), '/Fe:' + str(exe), '/link', '/MANIFEST:EMBED',
                         'advapi32.lib', 'ole32.lib', 'shell32.lib', 'uuid.lib', 'ntdll.lib'],
                        cwd=temp, env=env, timeout=55)
            if build.returncode:
                return build.returncode
            command = [str(exe)]
            if args.process_crash_only:
                command.append('--thin-process-crash-only')
            if args.host_crash_only:
                command.append('--thin-host-crash-only')
            return run(command, cwd=temp, env=env, timeout=25).returncode
        finally:
            root = Path(temp).resolve(strict=True)
            target = root / 'target'
            if target.exists() and target.resolve(strict=True) == target:
                shutil.rmtree('\\\\?\\' + str(target))


if __name__ == '__main__':
    raise SystemExit(main())
