# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Run actual parallel-copy preflight on isolated NTFS fixtures. | Verify allocation failure preserves existing targets and links. | #731 | GPT-6 (Codex) |
"""Compile the production worker function with isolated scheduler seams."""
import argparse
import os
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    parser.add_argument('--source-ref', help='compare the worker function from a prior git revision')
    args = parser.parse_args()
    worker = read('src/worker.cpp', args.source_ref)
    function = block(worker, 'static BOOL ParallelCopyFileW(')
    code = read('src/tests/parallel_copy_preflight_tests.cpp.in').replace('@@PARALLEL_COPY@@', function)
    code = code.replace('@@PARALLEL_THREAD@@', block(worker, 'static DWORD WINAPI ParallelCopyThreadProc('))
    if '@@PARALLEL_COPY@@' in code:
        raise AssertionError('production worker function was not inserted')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='parallel-copy-preflight-') as temp:
        root = Path(temp).resolve(strict=True)
        source = root / 'fixture.cpp'
        exe = root / 'fixture.exe'
        source.write_text(code, encoding='utf-8')
        result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                      str(source), '/Fe:' + str(exe)], cwd=root, env=env, timeout=35)
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
