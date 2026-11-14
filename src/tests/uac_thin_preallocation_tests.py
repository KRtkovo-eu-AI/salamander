# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test scoped preallocation failure recovery. | Exercise the actual worker branch with write-only Windows handles and injected failures. | #731 | GPT-6 |
"""Test the extracted preallocation branch; this is not whole F5 or actual UAC."""
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
    production = ROOT / 'src/worker.cpp'
    before = hashlib.sha256(production.read_bytes()).hexdigest()
    dialog_file = ROOT / 'src/dialogs.cpp'
    dialogs_before = hashlib.sha256(dialog_file.read_bytes()).hexdigest()
    worker = read('src/worker.cpp')
    helper_start = worker.index('static bool ThinCopyResetFailedPreallocation(HANDLE file)')
    helper_end = worker.index('\n}\n', helper_start) + 2
    accepted_start = worker.index('static void ThinCopyAcceptedTarget(')
    accepted_end = worker.index('\n}\n', accepted_start) + 2
    cancel_start = worker.index('bool CThinCopyWorker::CancelBeforeElevation()')
    cancel_end = worker.index('\n}\n', cancel_start) + 2
    report_start = worker.index('bool CThinCopyWorker::ReportTargetFailure(')
    report_end = worker.index('\n}\n', report_start) + 2
    dialogs = read('src/dialogs.cpp')
    dispatch_start = dialogs.index('        case WORKER_DIALOG_UAC_THIN_TERMINAL:')
    dispatch_end = dialogs.index('\n#endif', dispatch_start)
    macro_start = worker.index('#define THIN_DELETE_TARGET(ctx, local)')
    macro_end = worker.index('\n#else', macro_start)
    region = worker.index('// if possible, allocate the required space for the file (prevents disk fragmentation')
    fatal_start = worker.index('                        if (fatal)\n', region)
    fatal_end = worker.index('\n                    }\n\n                    script->SetFileStartParams();', fatal_start)
    fatal = worker[fatal_start:fatal_end]
    if len(fatal) > 5000 or 'ThinCopyResetFailedPreallocation(out)' not in fatal:
        raise RuntimeError('Unexpected worker preallocation region')
    operation_start = worker.index('BOOL DoCopyFile(COperation* op,')
    sticky_declaration = worker.index('CThinCopyWorker thin(', operation_start)
    first_mark = worker.index('thin.dataTransferStarted = true;', fatal_end)
    first_loop = worker.index('DoCopyFileLoopAsync(', first_mark)
    retry_label = worker.index('COPY_AGAIN:', sticky_declaration, region)
    if not operation_start < sticky_declaration < retry_label < fatal_start < fatal_end < first_mark < first_loop:
        raise RuntimeError('Transfer-start guard is not in scope before actual copy loops')
    open_start = worker.index('HANDLE CThinCopyWorker::Open(')
    open_end = worker.index('HANDLE CThinCopyWorker::CancelReceivedHandle(', open_start)
    opened = worker[open_start:open_end]
    if opened.count('ThinCopyAcceptedTarget(dataTransferStarted, kind);') != 1 or not (
            opened.rindex('return CancelReceivedHandle(') < opened.index('ThinCopyAcceptedTarget(dataTransferStarted, kind);') <
            opened.rindex('return result.file;')):
        raise RuntimeError('New target must be accepted only after successful handoff and cancellation check')
    guarded = ' '.join(fatal.split())
    if ('if (!thin.dataTransferStarted && !cancelled) { reset = ThinCopyResetFailedPreallocation(out);' not in guarded
            or 'cancelled = thin.CancelBeforeElevation(); } if (reset && !cancelled)' not in guarded
            or 'bool cancelled = thin.CancelBeforeElevation();' not in guarded):
        raise RuntimeError('Scoped reset lacks transfer and cancellation guards')
    template = (ROOT / 'src/tests/uac_thin_preallocation_tests.cpp.in').read_text(encoding='utf-8')
    code = (template.replace('@@ACCEPTED@@', worker[accepted_start:accepted_end])
            .replace('@@HELPER@@', worker[helper_start:helper_end])
            .replace('@@CANCEL@@', worker[cancel_start:cancel_end])
            .replace('@@REPORT@@', worker[report_start:report_end])
            .replace('@@DISPATCH@@', dialogs[dispatch_start:dispatch_end])
            .replace('@@DELETE_MACRO@@', worker[macro_start:macro_end])
            .replace('@@FATAL_BRANCH@@', fatal))
    if '@@' in code:
        raise RuntimeError('Missing test template replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    base = ROOT.parent / 'builds'
    with tempfile.TemporaryDirectory(prefix='uac-thin-preallocation-', dir=base) as temp:
        source = Path(temp) / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', *flags,
                     str(source), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        fixture = Path(temp) / '\u6771-preallocation'
        fixture.mkdir()
        result = run([str(exe), str(fixture)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(production.read_bytes()).hexdigest() != before:
        raise RuntimeError('worker.cpp changed during test')
    print('worker.cpp SHA-256:', before)
    print('Actual extracted reset helper, cancellation guard, terminal method, dialog dispatch, cleanup macro and preallocation branch.')
    print('Real local NTFS write-only sync/overlapped handles; fault injection and broker cleanup are stubs.')
    print('This does not exercise the complete DoCopyFile worker, actual protected ACLs, UAC, SMB or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
