# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-06 | Test archive cache identity and drop scheduling after upstream integration. | Keep metadata invalidation and complete Unicode paths together. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
"""Compile actual cache-key and drop-admission statements; use real owned archive files."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import block, read, compiler_environment, run, ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    viewer = read('src/fileswn5.cpp')
    drop = block(read('src/fileswn6.cpp'), 'void CFilesWindow::DropCopyMove(')
    first = drop.index('            script->CopyMoveTransferMode =')
    last = drop.index('            script->AddStoragePath(targetPath, SACCESS_WRITE);', first)
    last += len('            script->AddStoragePath(targetPath, SACCESS_WRITE);')
    pieces = {
        'VIEWER_PATH_HEADER': (ROOT / 'src/viewerpath.h').as_posix(),
        'SCHEDULER_HEADER': (ROOT / 'src/storagesched.h').as_posix(),
        'CACHE': block(viewer, 'static DWORD UpdateArchiveCacheHash(') + '\n' + block(viewer, 'static void BuildArchiveCacheKey('),
        'PREFIX': block(read('src/common/widepath.cpp'), 'BOOL SalIsExtendedLengthPathW(') + '\n' + block(read('src/common/widepath.cpp'), 'std::wstring SalPathAddExtendedPrefixW('),
        'OVERRIDE': block(read('src/storagesched.cpp'), 'int CopyMoveGetSchedulingOverride('),
        'DROP': drop[first:last],
    }
    text = read('src/tests/upstream_core_integration_tests.cpp.in')
    for key, value in pieces.items():
        text = text.replace('@@' + key + '@@', value)
    assert '@@' not in text
    # Both controls must be initialized without restoring the removed global-policy checkbox rule.
    dialog = read('src/dialogs3.cpp').split('CCopyMoveMoreDialog::DialogProc(UINT', 1)[1]
    init = dialog[dialog.index('case WM_INITDIALOG:'):dialog.index('EnableWindow(GetDlgItem(HWindow, IDC_CM_SECURITY)')]
    assert 'IDC_BRANCH_KEEP_PATHS' in init and 'UpdateTransferModeControls();' in init
    assert 'EnableWindow(GetDlgItem(HWindow, IDC_CM_STARTONIDLE)' not in init
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='upstream-core-integration-') as temp:
        directory = Path(temp)
        cpp, exe = directory / 'test.cpp', directory / 'test.exe'
        cpp.write_text(text, encoding='utf8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual upstream core integration:', config, flush=True)
            command = [compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                       str(cpp), '/Fe:' + str(exe)]
            result = run(command, cwd=directory, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            for codepage in ('1250', '1252', '65001'):
                runtime = directory / (config + '-' + codepage)
                runtime.mkdir()
                result = run([str(exe), str(runtime), codepage], cwd=directory, env=env, timeout=20)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
