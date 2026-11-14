# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Run the actual file-list controller with isolated seams. | Exercise modal selection changes and failure routing. | #731 | GPT-6.1 Sol |
"""Test the actual controller with fake UI/broker; does not test real UAC or generation."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    controller = block(read('src/mainwnd4.cpp'), 'static bool CreateProtectedFileList(')
    # Function-try-block includes its handler after the first compound block.
    start = read('src/mainwnd4.cpp').index('catch (const std::bad_alloc&)', read('src/mainwnd4.cpp').index(controller))
    controller += '\n' + block(read('src/mainwnd4.cpp')[start:], 'catch (const std::bad_alloc&)')
    source = read('src/tests/uac_filelist_controller_tests.cpp.in').replace('@@CONTROLLER@@', controller)
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ['/MTd', '/Od', '/D_DEBUG'] if args.configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='uac-filelist-controller-') as folder:
        directory = Path(folder)
        cpp, exe = directory / 'test.cpp', directory / 'test.exe'
        cpp.write_text(source, encoding='utf-8')
        result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                      str(cpp), '/Fe:' + str(exe), '/link', 'comctl32.lib', 'user32.lib'], cwd=directory, env=env, timeout=35)
        if result.returncode:
            return result.returncode
        return run([str(exe)], cwd=directory, env=env, timeout=10).returncode


if __name__ == '__main__':
    raise SystemExit(main())
