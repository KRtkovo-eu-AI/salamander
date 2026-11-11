# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Guard batch-wide refusal and per-root size scopes. | Keep denied consent isolated to one user operation. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Check the private partial size marker. | Keep incomplete directory size coverage without changing the SDK. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check the shared modal UAC consent owner. | Retain coverage of size calculation after sharing the dialog. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check continuation after a live broker denies access. | Avoid a separate dialog for each omitted subtree. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Compile the production protected size skip branch. | Verify readable totals after declined elevation. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Guard the wide size scan and dangling-link classification. | Keep ordinary read failures on the existing error path. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check a single size consent and automatic reuse. | Keep the broker scoped to the selected tree without separate scope controls. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify live owner and per-tree size scope. | Prevent repeated elevation and stale incomplete sizes. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify the size scan foreground handoff. | Record consent before disabling the owner and restore before showing results. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify scoped continuation of protected size batches. | Keep refused or incomplete rows out of published sizes. | #731 | Codex |
"""Compile the actual BuildScriptDir error/cancel branch, including its message storage."""
import argparse
import json
import re
import shutil
import tempfile
from pathlib import Path
from disk_file_size_paths_tests import read, run, compiler_environment, ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    parser.add_argument('--source-ref', help='demonstrate the original overrun with the same native harness')
    args = parser.parse_args()
    production = read('src/fileswn6.cpp', args.source_ref)
    if not args.source_ref:
        consent = production[production.index('class CProtectedOperationConsentDialog'):
                             production.index('BOOL CFilesWindow::BuildScriptDir(')]
        for field in ('IDS_MSGBOXBTN_SKIP', 'IDS_MSGBOXBTN_SKIPALL', 'IDS_AS_ADMIN_TITLE',
                      'MSGBOXEX_YESNOOKCANCEL', 'MSGBOXEX_SHIELDONOKBTN'):
            assert field in consent
        modal_end = consent.index('return CMessageBox::DialogProc(')
        assert consent.index('Error = Session->Start(HWindow)') < modal_end
        assert 'IDCONTINUE' not in consent
        start = production.index('if (wideSizeScan && err == ERROR_ACCESS_DENIED')
        protected = production[start:production.index('if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES)', start)]
        assert 'if (!session && canElevate && temporary.PrepareInventory(path))' in protected
        assert 'UacF7::CanOfferElevation()' in protected
        assert 'sizeScope->Session.PrepareInventory(sizeScope->Root)' in protected
        assert protected.index('if (scoped && sizeScope->StartResult)') < protected.index('choice = ConfirmProtectedSize')
        assert 'sizeScope->StartResult = inventoryError;' in protected
        assert 'session->IsOpen() ? session->Inventory' in protected
        scan_start = production.index('const bool wideSizeScan = type == atCountSize;')
        assert 'FindFirstFileW(pattern.c_str(), &sizeRow)' in production[scan_start:start]
        assert 'SalPathAddExtendedPrefixIfNeededW' in production[scan_start:start]
        assert 'err == ERROR_PATH_NOT_FOUND && (sourceDirAttr & FILE_ATTRIBUTE_REPARSE_POINT)' in production[scan_start:start]
        assert 'IsDanglingDirectoryLinkForSize(sourcePath)' in production[scan_start:start]
        action = read('src/fileswn8.cpp')
        assert 'disableMainWindow = type != atCountSize' in action
        direct_denial = protected.index('if (inventoryError == ERROR_ACCESS_DENIED && session->IsOpen())')
        assert direct_denial < protected.index('inventory.firstOmission = inventoryError;', direct_denial)
        assert direct_denial < protected.index('inventoryError = ERROR_SUCCESS;', direct_denial)
        assert direct_denial < protected.index('err = inventoryError;')
        assert 'script->CountSizeFirstOmissionPath = path;' in protected
        skip = protected[protected.index('if (choice != IDOK || inventoryError == ERROR_CANCELLED)'):]
        assert skip.index('if (choice == IDCANCEL)') < skip.index('return FALSE;')
        assert skip.index('if (choice == IDNO)') < skip.index('ErrListDirSkipAll = TRUE;')
        assert skip.index('script->CountSizeIncomplete = true;') < skip.index('return TRUE;')
        scan = production[production.index('BOOL CFilesWindow::BuildScriptMain('):
                          production.index('class CProtectedOperationConsentDialog')]
        assert scan.count('CSizeBatchConsent sizeBatchConsent;') == 1
        assert scan.index('CSizeBatchConsent sizeBatchConsent;') < scan.index('sizeScope->Batch = &sizeBatchConsent;')
        assert scan.index('sizeScope->Batch = &sizeBatchConsent;') < scan.index('if (!BuildScriptDir(')
        assert 'selectedSizeScope->Batch = &sizeBatchConsent;' in scan
        assert 'std::optional<CSizeScanScope> sizeScope;' in scan
        assert 'if (consentDeclined && sizeScope)' in protected
        assert 'sizeScope->DeclineElevation();' in protected
        assert scan.index('script->CountSizeIncomplete = false;') < scan.index('if (!BuildScriptDir(')
        assert scan.index('oneFile->SizeValid = 0;') < scan.index('if (!BuildScriptDir(')
        assert 'oneFile->SizeValid = !script->CountSizeIncomplete;' in scan
        assert 'const bool partial = script->CountSizeIncomplete && sizeScope->Scanned;' in scan
        assert 'oneFile->PluginData = DiskDirectoryPartialSizeMarker;' in scan
        assert 'script->CountSizeAnyIncomplete |= script->CountSizeIncomplete;' in scan
    protected_skip = ''
    if not args.source_ref:
        start = production.index('                    if (choice != IDOK || inventoryError == ERROR_CANCELLED)')
        end = production.index('                    if (inventoryError)', start)
        protected_skip = production[start:end]
    production = production[production.index('BOOL CFilesWindow::BuildScriptDir('):production.index('\nBOOL GetLinkTgtFileSize(')]
    start = production.index('    CPathBuffer textBuffer(') if '    CPathBuffer textBuffer(' in production else production.index('    char text[')
    storage = production[start:production.index('    char finalName[', start)]
    enumeration = production[production.index('//---  build the path to sourceDirName and start searching for contained files'):]
    marker = 'if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES)'
    match = re.search(r'(?m)^ {12}' + re.escape(marker) + r'\n[ 	]*\{', enumeration)
    assert match, 'read error dialog branch not found'
    start = match.start()
    opening = match.end() - 1
    # Menu-export comments contain unmatched C++ braces; ignore comments and literals.
    tokens = re.finditer(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|[{}]', enumeration[opening:], re.S)
    depth = 0
    for token in tokens:
        if token.group() == '{': depth += 1
        elif token.group() == '}':
            depth -= 1
            if not depth:
                error = enumeration[start:opening + token.end()]
                break
    else: raise AssertionError('unterminated production error branch')
    header = '#include "' + (ROOT / 'src/common/localizedtext.h').as_posix() + '"\n'
    header += '#include "' + (ROOT / 'src/common/widepath.h').as_posix() + '"'
    wide = read('src/common/widepath.cpp', args.source_ref)
    buffer = wide[wide.index('CPathBuffer::CPathBuffer()'):wide.index('CWidePathBuffer::CWidePathBuffer()')]
    names = ('IDS_CANNOTREADDIR', 'IDS_NAMEISTOOLONG', 'IDS_TOOLONGNAME2', 'IDS_DELETESHDIR', 'IDS_NONEMPTYDIRDELCONFIRM')
    patterns = []
    english = read('src/lang/texts.rc2')
    ids = read('src/texts.rh2')
    for name in names:
        text = re.search(r'^\s*' + name + r', "(.*)"\s*$', english, re.M).group(1)
        patterns.append((name, json.loads('"' + text.replace('""', r'\"') + '"')))
        number = re.search(r'^#define\s+' + name + r'\s+(\d+)', ids, re.M).group(1)
        for translation in sorted((ROOT / 'translations').glob('*/salamand.slt')):
            text = re.search(r'^' + number + r',[01],"(.*)"\s*$', translation.read_text(encoding='utf-8-sig'), re.M).group(1)
            patterns.append((name, re.sub(r'\\([nrt\\])', lambda m: {'n': '\n', 'r': '\r', 't': '\t', '\\': '\\'}[m[1]], text)))
    golden = []
    for name, pattern in patterns:
        values = ['PATH', 'ERROR'] if name in ('IDS_CANNOTREADDIR', 'IDS_NAMEISTOOLONG', 'IDS_TOOLONGNAME2') else ['PATH']
        it = iter(values)
        expected = ''.join(next(it) if token == '%s' else '%' if token == '%%' else token for token in re.split(r'(%s|%%)', pattern))
        golden.append('{' + ','.join(json.dumps(v, ensure_ascii=False) for v in (pattern, expected)) + ',' + str(len(values)) + '}')
    source = read('src/tests/directory_size_cancel_tests.cpp.in')
    for key, value in {'HEADER': header, 'BUFFER': buffer, 'STORAGE': storage, 'ERROR': error, 'PROTECTED_SKIP': protected_skip, 'HAS_PROTECTED_SKIP': str(int(bool(protected_skip))), 'PATTERNS': ',\n'.join(golden)}.items():
        source = source.replace('@@' + key + '@@', value)
    assert '@@' not in source
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='directory-size-cancel-') as temp:
        directory = Path(temp)
        cpp, exe = directory / 'test.cpp', directory / 'test.exe'
        cpp.write_text(source, encoding='utf8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual BuildScriptDir error/cancel:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', '/GS', *flags, str(cpp), '/Fe:' + str(exe), 'user32.lib'], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    print('Native exit:', hex(result.returncode & 0xffffffff), flush=True)
                    return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
