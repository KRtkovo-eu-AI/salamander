# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test both thin RunProbe launch boundaries. | Prevent prelaunch consent reuse once launching may begin. | #731 | GPT-6 |
"""Extract RunProbe prelaunch code; endpoint, image and launcher are stubs."""
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
    source_path = ROOT / 'src/uacbroker/uacbroker.cpp'
    digest = hashlib.sha256(source_path.read_bytes()).hexdigest()
    source = read('src/uacbroker/uacbroker.cpp')
    deleter_start = source.index('struct LocalFreeDeleter\n')
    deleter_end = source.index('\n};', deleter_start) + 3
    endpoint = source.index('Handle CreateEndpoint(std::wstring& name)')
    guard = source.index('    std::unique_ptr<void, LocalFreeDeleter> sidGuard(sid);', endpoint)
    sddl = source.index('    std::wstring sddl =', guard)
    reset = source.index('    sidGuard.reset();', sddl)
    if not guard < sddl < reset:
        raise RuntimeError('SID is not guarded across SDDL construction')
    probe = source.index('int RunProbe(')
    start = source.index('    auto pipeName = std::wstring();', probe)
    end = source.index('    if (!launched)\n', start)
    template = (ROOT / 'src/tests/uac_thin_runprobe_prelaunch_tests.cpp.in').read_text(encoding='utf-8')
    code = (template.replace('@@ACTUAL_PREFIX@@', source[start:end])
            .replace('@@ACTUAL_LOCALFREE_DELETER@@', source[deleter_start:deleter_end])
            .replace('@@ACTUAL_SID_GUARD@@', source[guard:source.index('\n', guard)])
            .replace('@@ACTUAL_SDDL@@', source[sddl:source.index('\n', sddl)])
            .replace('@@ACTUAL_SID_RESET@@', source[reset:source.index('\n', reset)]))
    if '@@' in code:
        raise RuntimeError('Missing source replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-runprobe-boundary-', dir=ROOT.parent / 'builds') as temp:
        test_source = Path(temp) / 'test.cpp'
        test_source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', '/DSAM_UAC_F7_HOST',
                     '/DSAM_UAC_THIN_COPY_TEST', *flags, str(test_source), '/Fe:' + str(executable)],
                    cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(source_path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('uacbroker.cpp changed during test')
    print('uacbroker.cpp SHA-256:', digest)
    print('Actual RunProbe prelaunch prefix; endpoint, image, launcher and faults are stubs.')
    print('This does not exercise real IPC, broker launch, UAC, complete F5 or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
