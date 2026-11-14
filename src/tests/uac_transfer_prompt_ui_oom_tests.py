# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test transfer UI allocation failure dispatch. | Verify cancellation and errors using actual extracted branches. | #731 | GPT-6 |
"""Extracted UI branches with fake dialogs; not a real GUI or F5/UAC test."""
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
    source_path = ROOT / 'src/dialogs.cpp'
    digest = hashlib.sha256(source_path.read_bytes()).hexdigest()
    source = read('src/dialogs.cpp')
    begin = source.index('        case WORKER_DIALOG_UAC_TRANSFER:')
    error_case = source.index('        case WORKER_DIALOG_UAC_TRANSFER_PROMPT_ERROR:', begin)
    end = source.index('        case WORKER_DIALOG_UAC_TRANSFER_UNKNOWN:', error_case)
    template = (ROOT / 'src/tests/uac_transfer_prompt_ui_oom_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@TRANSFER_CASE@@', source[begin:error_case]).replace('@@PROMPT_ERROR_CASE@@', source[error_case:end])
    if '@@' in code:
        raise RuntimeError('Missing source replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-prompt-ui-', dir=ROOT.parent / 'builds') as temp:
        test_source = Path(temp) / 'test.cpp'
        test_source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', '/DSAM_UAC_THIN_COPY_TEST', *flags,
                     str(test_source), '/Fe:' + str(executable)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(source_path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('dialogs.cpp changed during test')
    print('dialogs.cpp SHA-256:', digest)
    print('Actual extracted UI branches; UI, cancellation, transfer request and allocation faults are stubs.')
    print('This does not exercise real GUI, DoCopyFile, UAC or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
