# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Run two files through one real planned client job. | Verify batch reuse without claiming UAC acceptance. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Run the real client's connection-loss test. | Exercise an idle pipe after a dispatcher process exits. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Run a cross-process crash before the thin handle reply. | Measure a real broker-process exit after duplication. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Exercise a bounded budget on the real client pipe. | Verify repeated lost responses stop further handle requests. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Run the post-duplication lost-reply case. | Measure repeated unclaimed handles on the real dispatcher pipe. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Add isolated native overwrite dispatcher cases. | Compare actual NTFS behavior without claiming complete F5 or UAC acceptance. | #731 | GPT-6 |
# | 2026-11-14 | Run identity-anchored cleanup through the real dispatcher. | Verify own-object deletion, replacement refusal and failed creation handoff. | #731 | GPT-6 |
# | 2026-11-14 | Run an isolated host-process crash case. | Check that the real dispatcher exits when its peer dies. | #731 | GPT-6 |
# | 2026-11-14 | Run the external dispatcher crash case. | Test real cross-process handle lifetime separately from UAC. | #731 | GPT-6 |
# | 2026-11-12 | Test the experimental thin source-parent fallback. | Run real NTFS and pipe cases without pretending the token is elevated. | #731 | GPT-6 |
"""Build and run the real job dispatcher with experimental thin pinning."""

import argparse
from pathlib import Path
import shutil
import sys
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run
from uac_thin_sync_loop_tests import build_source as build_sync_loop_source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    crash_mode = parser.add_mutually_exclusive_group()
    crash_mode.add_argument('--process-crash-only', action='store_true')
    crash_mode.add_argument('--host-crash-only', action='store_true')
    crash_mode.add_argument('--owned-cleanup-only', action='store_true')
    crash_mode.add_argument('--overwrite-only', action='store_true')
    crash_mode.add_argument('--lost-reply-only', action='store_true')
    crash_mode.add_argument('--external-lost-reply-only', action='store_true')
    crash_mode.add_argument('--client-reopen-only', action='store_true')
    crash_mode.add_argument('--client-lost-budget-only', action='store_true')
    crash_mode.add_argument('--client-external-lost-budget-only', action='store_true')
    crash_mode.add_argument('--client-connection-only', action='store_true')
    crash_mode.add_argument('--client-planned-only', action='store_true')
    crash_mode.add_argument('--client-planned-loop-only', action='store_true')
    crash_mode.add_argument('--client-planned-mismatch-only', action='store_true')
    crash_mode.add_argument('--client-planned-grow-only', action='store_true')
    crash_mode.add_argument('--client-planned-shrink-only', action='store_true')
    crash_mode.add_argument('--client-planned-ea-only', action='store_true')
    crash_mode.add_argument('--client-planned-async-only', action='store_true')
    crash_mode.add_argument('--client-planned-async-read-retry-only', action='store_true')
    crash_mode.add_argument('--client-planned-async-mismatch-only', action='store_true')
    crash_mode.add_argument('--client-planned-preallocation-only', action='store_true')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    async_loop = args.client_planned_async_only or args.client_planned_async_mismatch_only or args.client_planned_async_read_retry_only
    linked_loop = args.client_planned_loop_only or args.client_planned_mismatch_only or args.client_planned_grow_only or args.client_planned_shrink_only or args.client_planned_ea_only or args.client_planned_preallocation_only or async_loop
    client_mode = args.client_reopen_only or args.client_lost_budget_only or args.client_external_lost_budget_only or args.client_connection_only or args.client_planned_only or linked_loop
    if args.owned_cleanup_only or client_mode or args.overwrite_only or args.lost_reply_only or args.external_lost_reply_only:
        flags += ('/DSAM_UAC_THIN_REOPEN_PROTOTYPE',)
    if client_mode:
        flags += ('/DSAM_UAC_CLIENT_PIPE_HARNESS', '/DSAM_UAC_THIN_PLANNED_PROTOTYPE')
    if linked_loop:
        flags += ('/DSAM_UAC_LOOP_LINK_HARNESS', '/wd4201')
    if async_loop:
        flags += ('/DSAM_UAC_LOOP_LINK_ASYNC', '/wd4100')
    if args.client_planned_preallocation_only:
        flags += ('/DSAM_UAC_LINK_PREALLOC', '/wd4505')
    if args.client_lost_budget_only or args.client_external_lost_budget_only:
        flags += ('/DSAM_UAC_THIN_HANDOFF_TEST_CAP=3',)
    with tempfile.TemporaryDirectory(prefix='uac-thin-parent-') as temp:
        exe = Path(temp) / ('salamand.exe' if client_mode else 'uacbroker_thin_parent_job_tests.exe')
        source = ROOT / 'src/uacbroker/uacbroker_thin_parent_job_tests.cpp'
        sources = [str(source)]
        if client_mode:
            client = Path(temp) / 'thin_client.cpp'
            client.write_text('#define SAM_UAC_F7_HOST\n#define SAM_UAC_THIN_COPY_TEST\n'
                              f'#include "{(ROOT / "src/uacbroker/uacbroker.cpp").as_posix()}"\n',
                              encoding='utf-8')
            sources.append(str(client))
            if linked_loop:
                worker = (ROOT / 'src/worker.cpp').read_text(encoding='utf-8-sig')
                header = (ROOT / 'src/worker.h').read_text(encoding='utf-8-sig')
                loop = Path(temp) / 'native_worker_loop.cpp'
                if async_loop:
                    emitted = run([sys.executable, '-B', str(ROOT / 'src/tests/uac_thin_async_loop_tests.py'),
                                   '--emit-linked', str(loop)], cwd=temp, env=env, timeout=20)
                    if emitted.returncode:
                        return emitted.returncode
                else:
                    loop.write_text(build_sync_loop_source(worker, header), encoding='utf-8')
                sources.append(str(loop))
                if args.client_planned_preallocation_only:
                    preallocation = Path(temp) / 'native_preallocation.cpp'
                    emitted = run([sys.executable, '-B', str(ROOT / 'src/tests/uac_thin_preallocation_tests.py'),
                                   '--emit-linked', str(preallocation)], cwd=temp, env=env, timeout=20)
                    if emitted.returncode:
                        return emitted.returncode
                    sources.append(str(preallocation))
        try:
            if client_mode:
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
            if args.overwrite_only:
                command.append('--thin-overwrite-only')
            if args.lost_reply_only:
                command.append('--thin-lost-reply-only')
            if args.external_lost_reply_only:
                command.append('--thin-external-lost-reply-only')
            if args.client_reopen_only:
                command.append('--thin-client-reopen-only')
            if args.client_lost_budget_only:
                command.append('--thin-client-lost-budget-only')
            if args.client_external_lost_budget_only:
                command.append('--thin-client-external-lost-budget-only')
            if args.client_connection_only:
                command.append('--thin-client-connection-only')
            if args.client_planned_only or args.client_planned_loop_only:
                command.append('--thin-client-planned-only')
            if args.client_planned_mismatch_only:
                command.append('--thin-client-planned-mismatch-only')
            if args.client_planned_grow_only:
                command.append('--thin-client-planned-grow-only')
            if args.client_planned_shrink_only:
                command.append('--thin-client-planned-shrink-only')
            if args.client_planned_ea_only:
                command.append('--thin-client-planned-ea-only')
            if args.client_planned_async_only:
                command.append('--thin-client-planned-only')
            if args.client_planned_async_read_retry_only:
                command.append('--thin-client-planned-read-retry-only')
            if args.client_planned_async_mismatch_only:
                command.append('--thin-client-planned-mismatch-only')
            if args.client_planned_preallocation_only:
                command.append('--thin-client-planned-only')
            return run(command, cwd=temp, env=env, timeout=25).returncode
        finally:
            root = Path(temp).resolve(strict=True)
            target = root / 'target'
            if target.exists() and target.resolve(strict=True) == target:
                shutil.rmtree('\\\\?\\' + str(target))


if __name__ == '__main__':
    raise SystemExit(main())
