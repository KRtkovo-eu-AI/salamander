# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise paged partial Branch retries. | Preserve siblings and reject substituted identities. | #731 | GPT-6.1 Terra |
"""Compile the real Branch workers with deterministic paged I/O seams."""
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run

def main():
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-paged-partial-') as temp:
        folder = Path(temp)
        for mode, flags in [('Debug', ['/MTd', '/Od', '/D_DEBUG']), ('Release', ['/MT', '/O2', '/DNDEBUG'])]:
            exe = folder / (mode + '.exe')
            print('Branch paged partial worker:', mode, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', '/DBRANCH_VIEW_TEST', *flags,
                             str(ROOT / 'src/branch_view.cpp'), str(ROOT / 'src/tests/branch_paged_partial_worker_tests.cpp'),
                             '/Fe:' + str(exe), '/link', 'user32.lib'], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
