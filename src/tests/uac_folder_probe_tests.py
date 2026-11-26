# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the folder-probe worker with controlled filesystem and message seams."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/fileswn1.cpp')
    panel = read('src/fileswn3.cpp')
    change = read('src/fileswn2.cpp')
    destroy = read('src/fileswnb.cpp')
    assert 'StopUacFolderProbe();' in block(panel, 'BOOL CFilesWindow::ReadDirectory(')
    assert 'StartUacFolderProbe();' in block(panel, 'void CFilesWindow::SortDirectory(')
    assert 'UacFolderProbeNext < static_cast<size_t>(Dirs->Count) + static_cast<size_t>(Files->Count)' in block(panel, 'void CFilesWindow::SortDirectory(')
    assert 'UacFolderProbeNext < static_cast<size_t>(Dirs->Count) + static_cast<size_t>(Files->Count)' in block(panel, 'void CFilesWindow::FinishExplorerSortAsync(')
    disk = block(change, 'BOOL CFilesWindow::ChangePathToDisk(')
    assert disk.index('InvalidateChangesInPanelWeHaveNewListing();') < disk.index('StartUacFolderProbe();')
    assert 'StopUacFolderProbe();' in block(destroy, 'case WM_DESTROY:')
    fixture = read('src/tests/uac_folder_probe_tests.cpp.in')
    for key, text in {
        'STATE': block(source, 'struct CUacFolderProbeState', ';'),
        'NEW_RUN': block(source, 'ULONG_PTR NewUacFolderProbeRun('),
        'LOCAL_PARENT': block(source, 'bool IsLocalUacFolderParent('),
        'READONLY_ACL': block(source, 'bool IsReadonlyFileWriteDenied('),
        'WORKER': block(source, 'DWORD WINAPI RunUacFolderProbe('),
        'STOP': block(source, 'void CFilesWindow::StopUacFolderProbe('),
        'START': block(source, 'void CFilesWindow::StartUacFolderProbe('),
        'CONTINUE': block(source, 'void CFilesWindow::UacFolderProbeContinue('),
        'READY': block(source, 'void CFilesWindow::UacFolderProbeReady('),
        'HAS': block(source, 'bool CFilesWindow::HasUacDeniedItem('),
    }.items():
        fixture = fixture.replace('@@' + key + '@@', text)
    assert '@@' not in fixture
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='uac-folder-probe-') as temp:
        folder = Path(temp)
        source_file, executable = folder / 'test.cpp', folder / 'test.exe'
        source_file.write_text(fixture, encoding='utf-8')
        for config, flags in [('Debug', ['/MTd', '/D_DEBUG', '/Od']), ('Release', ['/MT', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Folder access indicator:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             *flags, str(source_file), '/Fe:' + str(executable), 'advapi32.lib'], [str(executable)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
