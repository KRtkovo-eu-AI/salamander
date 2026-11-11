# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Preserve typed incomplete Find reasons across searches. | Keep different omissions distinct without changing current reports. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Test actual protected Find queue capacity. | Distinguish Stop and failure from incomplete results. | #731 | GPT-6.1 Terra |
"""Compile the actual protected queue and final activity with controlled seams."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source, header = read('src/find.cpp'), read('src/find.h')
    dialog = read('src/finddlg1.cpp')
    log_dialog = read('src/finddlg2.cpp')
    start = block(dialog, 'void CFindDialog::StartSearch(')
    state_start = start.index('    const unsigned previousIncomplete =')
    state_end = start.index('    GrepData.FindDuplicates = FALSE;', state_start)
    before_thread = start.index('    if (!retainIncomplete)', state_end)
    before_thread_end = start.index('    DWORD threadId;', before_thread)
    failed_start = start.index('        GrepData.IncompleteReasons = previousIncomplete;', before_thread_end)
    failed_end = start.index('        if (GrepData.StopEvent)', failed_start)
    refine = block(source, 'void RefineData(')
    digest = block(source, 'BOOL CDuplicateCandidates::GetMD5Digest(')
    fixture = read('src/tests/find_scope_capacity_tests.cpp.in')
    pieces = {
        'LOCALIZED_HEADER': (ROOT / 'src/common/localizedtext.h').as_posix(),
        'INCOMPLETE_REASONS': block(header, 'enum FindIncompleteReason', ';'),
        'ACTIVITY': block(header, 'enum class CFindActivity', ';'),
        'FINAL_ACTIVITY': block(header, 'inline CFindActivity FindFinalActivity('),
        'LIMIT_TEST': block(source, 'static bool FindFileConsentLimit('),
        'LIMIT_TEXT': block(source, 'void FindProtectedScopeLimitText('),
        'STATUS_ID': block(source, 'int FindIncompleteStatusStringID('),
        'REASON_ID': block(source, 'int FindIncompleteReasonStringID('),
        'REASON_TEXT': block(source, 'void FindIncompleteReasonText('),
        'SELECT_LOG_ITEM': block(log_dialog, 'CFindLogDialog::GetSelectedItem()'),
        'SHOW_LOG': block(log_dialog, 'void CFindDialog::OnShowLog()'),
        'SUMMARY_ROWS': log_dialog[log_dialog.index('        const unsigned reasons[] = {FindIncompleteScopeCapacity'):log_dialog.index('        if (Log->GetSkippedCount() > 0)', log_dialog.index('        const unsigned reasons[] = {FindIncompleteScopeCapacity'))],
        'REPORT': block(source, 'static void ReportFindScopeLimit('),
        'RETAIN_INCOMPLETE': block(dialog, 'static unsigned RetainFindIncomplete('),
        'FINISH_ITEMS': dialog[dialog.index('DWORD items = FoundFilesListView->GetCount();', dialog.index('void CFindDialog::FinishSearch(')):dialog.index('UpdateStatusBar = TRUE;', dialog.index('DWORD items = FoundFilesListView->GetCount();', dialog.index('void CFindDialog::FinishSearch('))) + len('UpdateStatusBar = TRUE;')],
        'START_STATE': start[state_start:state_end],
        'BEFORE_THREAD': start[before_thread:before_thread_end],
        'THREAD_FAILURE': start[failed_start:failed_end],
        'QUEUE': block(source, 'class FindProtectedQueue', ';'),
        'DIGEST_DENIED': block(digest, 'if (error == ERROR_ACCESS_DENIED &&'),
        'REFINE_HELPER': block(source, 'bool IncludeRefinedFindResult('),
        'REFINE_DENIED': block(refine, 'if (openDenied)'),
    }
    for key, value in pieces.items():
        fixture = fixture.replace('@@' + key + '@@', value)
    assert '@@' not in fixture
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='find-capacity-') as temp:
        folder = Path(temp)
        source_file, exe = folder / 'test.cpp', folder / 'test.exe'
        source_file.write_text(fixture, encoding='utf-8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual Find scope capacity:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                             str(source_file), '/Fe:' + str(exe), 'user32.lib'], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
