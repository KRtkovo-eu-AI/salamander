# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise a private file-list workspace. | Use the actual native copy primitive without elevation. | #731 | GPT-6.1 Sol |
"""Test workspace and broker copy primitive; does not exercise UAC or the UI."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ['/MTd', '/Od', '/D_DEBUG'] if args.configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='uac-filelist-workspace-') as folder:
        directory = Path(folder)
        exe = directory / 'test.exe'
        command = [compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                   str(ROOT / 'src/tests/uac_filelist_workspace_tests.cpp'), '/Fe:' + str(exe),
                   '/link', 'ntdll.lib', 'ole32.lib', 'advapi32.lib', 'bcrypt.lib']
        result = run(command, cwd=directory, env=env, timeout=40)
        if result.returncode:
            return result.returncode
        return run([str(exe), str(directory / 'target')], cwd=directory, env=env, timeout=20).returncode


if __name__ == '__main__':
    raise SystemExit(main())
