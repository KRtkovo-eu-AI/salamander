# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Include the real explicit Stop controller. | Exercise reentrant partial publication. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Exercise actual Branch poll and cancellation methods. | Check reentrant lifetime and snapshot handling before changing UAC. | #731 | GPT-6.1 Terra |
"""Compile the actual Branch polling controller with isolated worker and UI seams."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    header = (ROOT / 'src/branch_view.h').read_text(encoding='utf-8-sig')
    source = (ROOT / 'src/branch_view.cpp').read_text(encoding='utf-8-sig')
    pieces = {
        'ENTRY': block(header, 'struct Entry', ';'),
        'PROGRESS': block(header, 'struct Progress', ';'),
        'REFRESH': block(header, 'class RefreshSchedule', ';'),
        'ASSOCIATION': block(header, 'class AssociationPreparation', ';'),
        'LIFETIME': block(header, 'struct PreparationLifetime', ';'),
        'SESSION': block(header, 'struct PreparationSession', ';'),
        'PREVIEW': block(source, 'size_t InitialPreviewCount('),
        'CANCEL': block(source, 'void CFilesWindow::CancelBranchViewScan('),
        'STOP': block(source, 'void CFilesWindow::StopBranchViewScan('),
        'SHUTDOWN': block(source, 'void CFilesWindow::ShutdownBranchView('),
        'START': block(source, 'static void StartBranchViewScan('),
        'REFRESH_VIEW': block(source, 'void CFilesWindow::RefreshBranchView('),
        'POLL': block(source, 'void CFilesWindow::PollBranchView('),
    }
    template = (ROOT / 'src/tests/branch_poll_lifetime_tests.cpp.in').read_text(encoding='utf-8')
    for name, value in pieces.items():
        template = template.replace('@@' + name + '@@', value)
    template = template.replace('state.Worker.SetCaptureDenied(state.UseProtectedListing ? nullptr : CaptureBranchDenied);', '')
    template = template.replace('RetryBranchDenied(this);', '/* separately tested retry controller */')
    template = template.replace('auto preparation = hadPreparation ?',
                                'if (testOomBeforeDrain) throw std::bad_alloc(); auto preparation = hadPreparation ?')
    template = template.replace('BranchView->Worker.Drain(BranchView->StoppedBatch, progress);',
                                'BranchView->Worker.Drain(BranchView->StoppedBatch, progress); if (testOomAfterDrain) throw std::bad_alloc();')
    template = template.replace('preparation->Queue.Add(extension);',
                                'if (testOomQueue) throw std::bad_alloc(); preparation->Queue.Add(extension);')
    if '@@' in template:
        raise RuntimeError('unexpanded test placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-poll-lifetime-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(template, encoding='utf-8')
        exe = folder / 'test.exe'
        modes = [('Debug', ['/MTd', '/Od', '/D_DEBUG']), ('Release', ['/MT', '/O2', '/DNDEBUG'])]
        for mode, flags in modes:
            if args.configuration not in ('Both', mode):
                continue
            print('Actual Branch poll and cancellation:', mode, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', '/DBRANCH_VIEW_TEST', *flags,
                             str(generated), str(ROOT / 'src/branch_view.cpp'),
                             str(ROOT / 'src/tests/branch_poll_worker_adapter.cpp'),
                             '/Fe:' + str(exe), '/link', 'user32.lib'], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
