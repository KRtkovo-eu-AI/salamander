# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Run sealed move-recovery exception tests. | Check durable source recovery without touching existing data. | #731 | GPT-6 (Codex) |
"""Exercise the actual recovery helper with isolated NTFS fixtures."""
import argparse
import os
from pathlib import Path
import re
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    move = block(read('src/uacbroker/uacbroker.cpp'), 'DWORD MoveAcrossVolumes(')
    assert 'auto recover = [&] { return Recover(sourceParent, storage.get(), journal.get(), security.get()); };' in move
    assert re.search(r'if\s*\(recover\(\)\)\s*return ERROR_PARTIAL_COPY;', move)
    assert re.search(r'const DWORD rollback = recover\(\);\s*if\s*\(published \|\| rollback\)\s*return ERROR_PARTIAL_COPY;', move)
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-move-recover-exception-') as temp:
        root = Path(temp).resolve(strict=True)
        exe = root / 'test.exe'
        result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                      str(ROOT / 'src/tests/uac_move_recover_exception_tests.cpp'), '/Fe:' + str(exe),
                      '/link', 'ntdll.lib', 'ole32.lib', 'advapi32.lib', 'bcrypt.lib'],
                     cwd=root, env=env, timeout=35)
        if result.returncode:
            return result.returncode
        for relative in ('short-東', 'long-東/' + '/'.join(['dír-日-' + 'p' * 70] * 4)):
            fixture = (root / relative).resolve()
            if not fixture.is_relative_to(root):
                raise ValueError('fixture escaped its owned parent')
            extended = '\\\\?\\' + str(fixture)
            os.makedirs(extended)
            result = run([str(exe), extended], cwd=root, env=env, timeout=25)
            if result.returncode:
                return result.returncode
            child = (root / relative.split('/')[0]).resolve(strict=True)
            if child.parent != root:
                raise ValueError('cleanup target escaped its owned parent')
            shutil.rmtree('\\\\?\\' + str(child))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
