# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Check uncertain thin job retirement. | Prevent reuse after ambiguous results. | #731 | GPT-6 |
"""Exercise the actual thin result classification and retirement guard."""
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
    path = ROOT / 'src/uacbroker/uacbroker.cpp'
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    source = read('src/uacbroker/uacbroker.cpp')
    function = source.index('ThinOpenResult ThinCopySession::Open(')
    start = source.index('    struct RetireUnknownJob\n', function)
    end = source.index('    bool preProbeEligible', start)
    guard = source[start:end]
    start = source.index('        output.error = result.error;', end)
    end = source.index('\n    }\n    catch (const std::bad_alloc&)', start)
    classification = source[start:end]
    template = (ROOT / 'src/tests/uac_thin_unknown_retire_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@ACTUAL_GUARD@@', guard).replace('@@ACTUAL_CLASSIFICATION@@', classification)
    if '@@' in code:
        raise RuntimeError('Missing source replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-unknown-', dir=ROOT.parent / 'builds') as temp:
        test_source = Path(temp) / 'test.cpp'
        test_source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX',
                     '/DSAM_UAC_THIN_COPY_TEST', *flags, str(test_source), '/Fe:' + str(executable)],
                    cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('uacbroker.cpp changed during test')
    print('uacbroker.cpp SHA-256:', digest)
    print('Actual guard and response classification; pipe and file handles are real, job/Close are stubs and outcomes are injected.')
    print('Does not run the complete Open method, real broker IPC, UAC or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
