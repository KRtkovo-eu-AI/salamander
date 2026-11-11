# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Test protected Find result provenance across publication and refinement. | Prevent unverified pathname reads when identity preparation fails. | #731 | GPT-6.1 Terra |
"""Exercise production publication, F3 guards and protected Refine branches."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source, dialog = read('src/find.cpp'), read('src/finddlg1.cpp')
    dialog2 = read('src/finddlg2.cpp')
    refine = block(source, 'void RefineData(')
    start = refine.index('        if (ok && refineData->ProtectedOrigin')
    end = refine.index('        if (openDenied)', start)
    on_view = block(dialog, 'void CFindDialog::OnViewFile(')
    on_with = block(dialog, 'void CFindDialog::OnViewFileWith(')
    fixture = read('src/tests/find_protected_provenance_tests.cpp.in')
    pieces = {
        'ADD': block(source, 'BOOL AddFoundItem('),
        'PUBLISH': block(source, 'void PublishCandidate('),
        'REFINE': refine[start:end],
        'REFINE_RESULT': block(source, 'bool IncludeRefinedFindResult('),
        'MERGE': block(dialog2, 'static bool CanMergeFindDuplicate('),
        'DEDUPE': block(dialog2, 'void CFindDialog::OnHideDuplicateNamesSorted('),
        'REFINE_PUBLISH': block(refine, 'if (!data->StopSearch && IncludeRefinedFindResult('),
        'F3_GUARD': block(on_view, 'if (result->ProtectedOrigin && !result->FoundView)'),
        'WITH_GUARD': block(on_with, 'if (FoundFilesListView->At(viewedIndex)->ProtectedOrigin)'),
    }
    for key, value in pieces.items():
        fixture = fixture.replace('@@' + key + '@@', value)
    assert '@@' not in fixture
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='find-provenance-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(fixture, encoding='utf-8')
        for config in (('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)):
            flags = ['/MTd', '/Od', '/D_DEBUG'] if config == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
            exe = folder / (config + '.exe')
            print('Find protected provenance:', config, flush=True)
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(generated), '/Fe:' + str(exe)], cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            result = run([str(exe)], cwd=folder, env=env, timeout=20)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
