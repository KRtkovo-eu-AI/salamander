# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Include the planned-custody preprocessor boundary. | Keep the extracted prompt block syntactically complete. | #731 | GPT-6 |
# | 2026-11-13 | Test thin-copy prompt allocation failures. | Exercise the actual session-selection block with deterministic allocation faults. | #731 | GPT-6 |
"""Test extracted scoped-session selection, not whole F5 or UAC."""
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
    method_start = worker.index('HANDLE CThinCopyWorker::Open(')
    block_start = worker.index('    if (!session)\n', method_start)
    prototype = worker.rfind('#ifdef SAM_UAC_THIN_PLANNED_PROTOTYPE\n', method_start, block_start)
    if prototype < 0:
        raise RuntimeError('Missing prototype guard')
    block_start = prototype
    block_end = worker.index('    // Pause or cancellation may arrive', block_start)
    block = worker[block_start:block_end]
    if len(block) > 6000 or 'catch (const std::bad_alloc&)' not in block:
        raise RuntimeError('Unexpected scoped session block')
    template = (ROOT / 'src/tests/uac_thin_prompt_alloc_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@SESSION_BLOCK@@', block)
    if '@@' in code:
        raise RuntimeError('Missing template replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-prompt-', dir=ROOT.parent / 'builds') as temp:
        source = Path(temp) / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(source), '/Fe:' + str(executable)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('worker.cpp changed during test')
    print('worker.cpp SHA-256:', digest)
    print('Actual extracted scoped-session block; conversions, session, prompt and allocation faults are stubs.')
    print('This does not exercise the complete DoCopyFile worker, actual UAC or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
