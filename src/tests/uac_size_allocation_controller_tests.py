# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-15 | Cover skipped links in batch size consent. | Preserve eligible siblings and native link policy. | #731 | GPT-6 |
# | 2026-11-15 | Test general size branch errors. | Verify partial results without masking terminal failures. | #731 | GPT-6 |
# | 2026-11-15 | Test the partial-size status markup. | Catch unescaped prefix characters in the real parser. | #731 | GPT-6 |
# | 2026-11-15 | Explain denied size scans and total partial selections. | Report administrator denials and preserve lower-bound sums. | #731 | GPT-6 |
# | 2026-11-11 | Exercise protected multi-file selection. | Reuse only the approved panel without new prompts. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Compile the shared batch refusal state. | Test actual controller behavior across distinct scopes. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Exercise the actual size-allocation controller. | Verify consent choices and exact totals without interactive elevation. | #731 | GPT-6.1 Terra |
"""Compile the production atCountSize branch with controlled Win32 and broker results."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/fileswn6.cpp')
    status = read('src/fileswnb.cpp')
    assert 'const int prefix = partialSelectedSize ? 4 : 0;' in status
    assert 'ExpandPluralBytesFilesDirs(text + prefix, 200 - prefix, selectedSize' in status
    notice = block(read('src/fileswn8.cpp'), 'static void ShowSizeInventoryIncomplete(')
    assert 'error == ERROR_ACCESS_DENIED && deniedAsAdmin' in notice
    assert 'LoadStr(IDS_UAC_INVENTORY_ADMIN_DENIED)' in notice
    refresh = read('src/fileswn8.cpp')
    start = refresh.index('if (type == atCountSize) // additional directory sizes have been calculated')
    end = refresh.index('// if Salamander is active', start)
    assert 'PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0)' in refresh[start:end]
    for catalog in (ROOT / 'translations').glob('*/salamand.slt'):
        rows = [line for line in catalog.read_text(encoding='utf-8-sig').splitlines() if line.startswith('14722,')]
        assert len(rows) == 1, f'missing or duplicate administrator-denied text in {catalog}'
        if catalog.parent.name == 'czech':
            assert rows[0].startswith('14722,1,') and 'i s oprávněním správce' in rows[0]
    start = source.index('    case atCountSize:', source.index('BOOL CFilesWindow::BuildScriptFile('))
    end = source.index('    case atRecursiveConvert:', start)
    branch = source[start:end]
    helper = 'template<class Query>\n' + block(read('src/uacbroker/f7pilot.h'), 'DWORD QueryCompressedSize(')
    generated = read('src/tests/uac_size_allocation_controller_tests.cpp.in')
    generated = generated.replace('@@SIZE_LINK_POLICY@@', block(source, 'static bool SkipSizeDirectoryLink('))
    batch_start = source.index('    if (countSize && selCount > 1 && !IsBranchView())')
    batch_end = source.index('    char* useName', batch_start)
    batch_setup = source[batch_start:batch_end]
    assert 'SkipSizeDirectoryLink(item.Attr, verifyListedAlias, Configuration.CountSizeStayOnFileSystem, compatibilityLink)' in batch_setup
    assert batch_setup.index('if (SkipSizeDirectoryLink(') < batch_setup.index('sizeBatchConsent.Selected.emplace_back(')
    generated = generated.replace('@@BRANCH_ERRORS@@', block(read('src/uacbroker/f7pilot.h'), 'inline bool IsSkippableSizeBranchError('))
    start = status.index('const int prefix = partialSelectedSize')
    end = status.index('ExpandPluralBytesFilesDirs(text + prefix', start)
    generated = generated.replace('@@STATUS_PREFIX@@', status[start:end])
    generated = generated.replace('@@STATUS_PARSER@@', block(read('src/salamdr4.cpp'), 'BOOL LookForSubTexts('))
    generated = generated.replace('@@SELECTION_ACCUMULATOR@@', block(read('src/fileswnb.cpp'), 'static void AccumulateSelectedSize('))
    generated = generated.replace('@@QUERY@@', helper).replace('@@COUNT_SIZE@@', branch)
    batch = block(source, 'struct CSizeBatchConsent', ';')
    scope = block(source, 'struct CSizeScanScope', ';')
    scope = scope.replace('std::wstring Root;', 'std::wstring Root = L"C:\\\\scope";')
    generated = generated.replace('@@BATCH_SCOPE@@', batch + '\n' + scope)
    generated = generated.replace('@@DIRECTORY_SIZE@@', block(source, 'if (wideSizeScan && err == ERROR_ACCESS_DENIED'))
    selection = source.index('                    std::optional<CSizeScanScope> selectedSizeScope;')
    selection_end = source.index('                    if (!BuildScriptFile(', selection)
    generated = generated.replace('@@SELECTED_SCOPE@@', source[selection:selection_end])
    if '@@' in generated:
        raise AssertionError('unexpanded fixture')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='uac-size-controller-') as temporary:
        folder = Path(temporary)
        cpp = folder / 'test.cpp'
        cpp.write_text(generated, encoding='utf-8')
        exe = folder / 'test.exe'
        for configuration in configurations:
            flags = ('/MTd', '/Od', '/D_DEBUG') if configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
            print(f'Actual allocation controller: {configuration}', flush=True)
            build = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                         str(cpp), '/Fe:' + str(exe), 'user32.lib'], cwd=folder, env=env, timeout=35)
            if build.returncode:
                return build.returncode
            test = run([str(exe)], cwd=folder, env=env, timeout=15)
            if test.returncode:
                return test.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
