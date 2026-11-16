# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test prompt errors in F5/F6 worker branches. | Exercise actual extracted transfer, overwrite and tree paths. | #731 | GPT-6 |
"""Test extracted worker branches with stubs; not complete F5/F6 or UAC."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    config = parser.parse_args().configuration
    worker_path = ROOT / 'src/worker.cpp'
    digest = hashlib.sha256(worker_path.read_bytes()).hexdigest()
    worker = read('src/worker.cpp')
    transfer = worker.index('static bool ExecuteWorkerTransfer(')
    transfer_start = worker.index('    if (!planned && existing == jobs.end())\n', transfer)
    transfer_end = worker.index('    const HANDLE cancel =', transfer_start)
    overwrite = worker.index('static SourceOverwriteResult TrySingleProbedOverwrite(')
    overwrite_start = worker.index('        if (!planned && existing == jobs.end())\n', overwrite)
    overwrite_end = worker.index('        auto* job =', overwrite_start)
    tree = worker.index('            case ocCopyProtectedTree:')
    tree_start = worker.index('                UacF7::TransferPrompt prompt{', tree)
    tree_end = worker.index('                Error = prompt.result.unknown', tree_start)
    template = (ROOT / 'src/tests/uac_transfer_prompt_worker_oom_tests.cpp.in').read_text(encoding='utf-8')
    code = (template.replace('@@TRANSFER_BLOCK@@', worker[transfer_start:transfer_end])
            .replace('@@OVERWRITE_BLOCK@@', worker[overwrite_start:overwrite_end])
            .replace('@@TREE_BLOCK@@', worker[tree_start:tree_end]))
    if '@@' in code:
        raise RuntimeError('Missing source replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-prompt-worker-', dir=ROOT.parent / 'builds') as temp:
        test_source = Path(temp) / 'test.cpp'
        test_source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(test_source), '/Fe:' + str(executable)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('worker.cpp changed during test')
    print('worker.cpp SHA-256:', digest)
    print('Actual extracted transfer, overwrite and tree branches; jobs, events, dialog and broker are stubs.')
    print('This does not exercise complete F5/F6, actual UAC, native dialogs or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
