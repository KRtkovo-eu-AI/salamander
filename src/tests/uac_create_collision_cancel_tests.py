# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise collision concurrent with cancellation. | Keep receipt and job lifetime safe. | #731 | GPT-6 (Codex) |
"""Run the extracted production worker controller with a deterministic transfer seam."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    config = parser.parse_args().configuration
    production = ROOT / 'src/worker.cpp'
    before = hashlib.sha256(production.read_bytes()).hexdigest()
    worker = read('src/worker.cpp')
    template = (ROOT / 'src/tests/uac_create_collision_cancel_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@RECEIPT@@', block(worker, 'struct CreateOnlySourceReceipt', ';'))
    code = code.replace('@@CONTROLLER@@', block(worker, 'static bool ExecuteWorkerTransfer('))
    assert '@@' not in code
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG', '/D_ITERATOR_DEBUG_LEVEL=2') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-create-cancel-') as temp:
        source = Path(temp) / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags, str(source),
                     '/Fe:' + str(exe), '/link', 'user32.lib'], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    after = hashlib.sha256(production.read_bytes()).hexdigest()
    if before != after:
        raise RuntimeError('worker.cpp changed during test')
    print('worker.cpp SHA-256:', before)
    print('Actual extracted controller; transfer/session/dialog are deterministic stubs; real Windows events.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
