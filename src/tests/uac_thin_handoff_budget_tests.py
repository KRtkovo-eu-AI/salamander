# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Run bounded handoff reservation tests. | Cover concurrency and uncertain outcomes in Debug and Release. | #731 | gpt-6.1-terra |
"""Test the real reservation class; this does not run UAC or F5."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-budget-', dir=ROOT.parent / 'builds') as temp:
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(ROOT / 'src/tests/uac_thin_handoff_budget_tests.cpp'), '/Fe:' + str(exe)],
                    cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        return run([str(exe)], cwd=temp, env=env, timeout=20).returncode


if __name__ == '__main__':
    raise SystemExit(main())
