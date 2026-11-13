# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test scoped pre-probe retry state. | Preserve consent without reviving a possibly started broker. | #731 | GPT-6 |
"""Test the extracted worker Open method using stubbed broker outcomes."""
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
    transport_path = ROOT / 'src/uacbroker/uacbroker.cpp'
    digest = hashlib.sha256(worker_path.read_bytes()).hexdigest()
    transport_digest = hashlib.sha256(transport_path.read_bytes()).hexdigest()
    worker = read('src/worker.cpp')
    transport = read('src/uacbroker/uacbroker.cpp')
    start = worker.index('HANDLE CThinCopyWorker::Open(')
    end = worker.index('HANDLE CThinCopyWorker::CancelReceivedHandle(', start)
    transport_start = transport.index('ThinOpenResult ThinCopySession::Open(')
    transport_end = transport.index('\n}\n\nResult PreparedTransfer::Execute(', transport_start)
    transport_method = transport[transport_start:transport_end]
    catch_start = transport_method.rindex('    catch (const std::bad_alloc&)')
    before_probe = transport_method.index('preProbeEligible = false;')
    first_enter = transport_method.index('transportEntered = true;', before_probe)
    first_probe = transport_method.index('const int launched = RunProbe(', first_enter)
    second_enter = transport_method.index('transportEntered = true;', first_probe)
    next_request = transport_method.index('SendOperationRequest(', second_enter)
    if not before_probe < first_enter < first_probe < second_enter < next_request < catch_start:
        raise RuntimeError('Thin transport boundary no longer matches the test')
    if transport_method.count('output.preProbeFailure = preProbeEligible;') != 3:
        raise RuntimeError('Unexpected pre-probe result paths')
    template = (ROOT / 'src/tests/uac_thin_preprobe_retry_tests.cpp.in').read_text(encoding='utf-8')
    code = (template.replace('@@ACTUAL_OPEN@@', worker[start:end])
            .replace('@@ACTUAL_TRANSPORT_CATCH@@', transport_method[catch_start:]))
    if '@@' in code:
        raise RuntimeError('Missing source replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-preprobe-', dir=ROOT.parent / 'builds') as temp:
        source = Path(temp) / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', '/DSAM_UAC_THIN_COPY_TEST', *flags,
                     str(source), '/Fe:' + str(executable)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != digest:
        raise RuntimeError('worker.cpp changed during test')
    if hashlib.sha256(transport_path.read_bytes()).hexdigest() != transport_digest:
        raise RuntimeError('uacbroker.cpp changed during test')
    print('worker.cpp SHA-256:', digest)
    print('uacbroker.cpp SHA-256:', transport_digest)
    print('Actual CThinCopyWorker::Open and transport catch; boundary checked structurally; broker outcomes are stubs.')
    print('This does not exercise complete DoCopyFile, real IPC, UAC, OTS or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
