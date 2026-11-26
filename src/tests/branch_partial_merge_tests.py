# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Test identity-bound partial listing merges. | Preserve prior rows and reject replacements. | #731 | GPT-6.1 Terra |
"""Compile and exercise the actual Branch partial-directory merge model."""
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run

def main():
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-partial-') as temp:
        folder = Path(temp)
        for mode, flags in [('Debug', ['/MTd', '/Od', '/D_DEBUG']), ('Release', ['/MT', '/O2', '/DNDEBUG'])]:
            exe = folder / (mode + '.exe')
            print('Branch partial identity merge:', mode, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', '/DBRANCH_VIEW_TEST', *flags,
                             str(ROOT / 'src/branch_view.cpp'), str(ROOT / 'src/tests/branch_partial_merge_tests.cpp'),
                             '/Fe:' + str(exe), '/link', 'user32.lib'], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
