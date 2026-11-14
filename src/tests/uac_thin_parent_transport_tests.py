# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Run identity-anchored cleanup through the real dispatcher. | Verify own-object deletion, replacement refusal and failed creation handoff. | #731 | GPT-6 |
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
    crash_mode.add_argument('--owned-cleanup-only', action='store_true')
    crash_mode.add_argument('--client-reopen-only', action='store_true')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    if args.owned_cleanup_only or args.client_reopen_only:
        flags += ('/DSAM_UAC_THIN_REOPEN_PROTOTYPE',)
    if args.client_reopen_only:
        flags += ('/DSAM_UAC_CLIENT_PIPE_HARNESS', '/DSAM_UAC_THIN_PLANNED_PROTOTYPE')
    with tempfile.TemporaryDirectory(prefix='uac-thin-parent-') as temp:
        exe = Path(temp) / ('salamand.exe' if args.client_reopen_only else 'uacbroker_thin_parent_job_tests.exe')
        source = ROOT / 'src/uacbroker/uacbroker_thin_parent_job_tests.cpp'
        sources = [str(source)]
        if args.client_reopen_only:
            client = Path(temp) / 'thin_client.cpp'
            client.write_text('#define SAM_UAC_F7_HOST\n#define SAM_UAC_THIN_COPY_TEST\n'
                              f'#include "{(ROOT / "src/uacbroker/uacbroker.cpp").as_posix()}"\n',
                              encoding='utf-8')
            sources.append(str(client))
        try:
            if args.client_reopen_only:
                # PreparedTransfer pins the named host/broker pair even when
                # this fixture supplies an already-authenticated test pipe.
                # Build a real broker image; it is not launched by these cases.
                broker = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                              *flags, '/DSAM_UAC_BROKER', '/DSAM_UAC_THIN_COPY_TEST',
                              str(ROOT / 'src/uacbroker/uacbroker.cpp'),
                              '/Fe:' + str(Path(temp) / 'uacbroker.exe'), '/link', '/MANIFEST:EMBED',
                              'advapi32.lib', 'ole32.lib', 'shell32.lib', 'uuid.lib', 'ntdll.lib', 'user32.lib'],
                             cwd=temp, env=env, timeout=35)
                if broker.returncode:
                    return broker.returncode
            build = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                         *flags, *sources, '/Fe:' + str(exe), '/link', '/MANIFEST:EMBED',
                         'advapi32.lib', 'ole32.lib', 'shell32.lib', 'uuid.lib', 'ntdll.lib', 'user32.lib'],
                        cwd=temp, env=env, timeout=55)
            if build.returncode:
                return build.returncode
            command = [str(exe)]
            if args.process_crash_only:
                command.append('--thin-process-crash-only')
            if args.host_crash_only:
                command.append('--thin-host-crash-only')
            if args.owned_cleanup_only:
                command.append('--thin-owned-cleanup-only')
            if args.client_reopen_only:
                command.append('--thin-client-reopen-only')
            return run(command, cwd=temp, env=env, timeout=25).returncode
        finally:
            root = Path(temp).resolve(strict=True)
            target = root / 'target'
            if target.exists() and target.resolve(strict=True) == target:
                shutil.rmtree('\\\\?\\' + str(target))


if __name__ == '__main__':
    raise SystemExit(main())
