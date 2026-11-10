# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Test the actual verified metadata executor. | Verify that accepted shields apply only to the matching first denied operation. | #731 | GPT-6.1 Sol (Codex) |
# | 2026-11-10 | Exercise actual metadata preview and worker dispatch. | Verify denied-write detection and fail-closed protected requests without UI or elevation. | #731 | GPT-6.1 Sol (Codex) |
"""Compile production metadata preview/worker functions with isolated OS and UI seams."""
import argparse
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    controller = read('src/fileswn5.cpp')
    worker = read('src/worker.cpp')
    change = block(controller, 'void CFilesWindow::ChangeAttr(')
    assert change.index('ProtectedListing->PrepareMetadata(') < change.index('BuildScriptMain(script, atChangeAttrs')
    assert 'script->At(0).SourceNameWValid' in change
    assert 'SalPathRemoveExtendedPrefixW(script->At(0).SourceNameW.c_str()) == metadataPreview.path' in change
    assert 'script->Count == 1 && script->At(0).Opcode == ocChangeAttrs' in change
    assert 'dlgData.MetadataShieldedOk = shieldedOk && sameItem;' in change
    assert 'dlgData.AllowBasicMetadataPilot && chDlg.AcceptedShieldedOk()' in change
    assert '!deleting && !shareJob && prompt.shieldedOk' in read('src/dialogs.cpp')
    assert 'ProtectedListing->IsOpen()' in change
    assert 'find = entry;' in change
    pieces = {
        'HEADER': str(ROOT / 'src/uacbroker/f7pilot.h').replace('\\', '/'),
        'DATA': block(read('src/worker.h'), 'struct CChangeAttrsData', ';'),
        'TIME': block(worker, 'static LONGLONG MetadataFileTime('),
        'CHANGE': block(worker, 'bool BuildBasicMetadataChange('),
        'EXECUTOR': block(worker, 'static BOOL ExecuteVerifiedWorkerLeaf('),
        'WORKER': block(worker, 'static bool TryChangeBasicMetadata('),
        'PREVIEW': block(controller, 'struct MetadataElevationPreview', ';'),
    }
    code = read('src/tests/protected_metadata_controller_tests.cpp.in')
    for key, value in pieces.items():
        code = code.replace('@@' + key + '@@', value)
    assert '@@' not in code
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='protected-metadata-controller-') as temp:
        directory = Path(temp)
        source = directory / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        exe = directory / 'test.exe'
        for configuration, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', configuration):
                continue
            print('Actual protected metadata controller:', configuration, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                             str(source), '/Fe:' + str(exe)], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
