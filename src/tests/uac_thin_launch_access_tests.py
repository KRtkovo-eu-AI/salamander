# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the test-only launch access observer on a real restricted process handle."""
import argparse
import csv
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run
from uac_thin_launch_mask_events import LaunchMaskEvents


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-launch-access-', dir=ROOT.parent / 'builds') as temp:
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     str(ROOT / 'src/tests/uac_thin_launch_access_tests.cpp'), '/Fe:' + str(exe)],
                    cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
        who = subprocess.run(['whoami', '/user', '/fo', 'csv', '/nh'], capture_output=True, text=True,
                             encoding='utf-8', errors='replace', check=True, timeout=10)
        sid = next(csv.reader(who.stdout.splitlines()))[1]
        for argument, expected in (('--external', {'status': 'ok', 'granted_access': 0x101000}),
                                   ('--external-invalid', {'status': 'error'})):
            with LaunchMaskEvents(sid, sid) as collector:
                if collector.read(0) != {'status': 'pending'}:
                    raise RuntimeError('New event collection was not initially pending')
                child_env = os.environ.copy()
                child_env['SAM_UAC_TEST_LAUNCH_MASK_BASE'] = collector.base
                child = subprocess.run([str(exe), argument], cwd=temp, env=child_env, timeout=10)
                if child.returncode or collector.read(1000) != expected:
                    raise RuntimeError('Cross-process mask observation did not match')
        print('PASS precreated events collect exact mask and query failure across processes')
    print('Restricted same-process handle; not ShellExecute, UAC, a different administrator, or VM acceptance.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
