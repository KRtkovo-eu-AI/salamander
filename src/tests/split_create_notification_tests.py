# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Windows/general split and wide directory creation; UI/notification facades."""
import argparse
from pathlib import Path
import re
import shutil
import tempfile
from branch_panel_getters_tests import block, read, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug','Release','Both'), default='Both')
    args = parser.parse_args()
    sdk = read('src/zip.cpp')
    assert 'pathIsDir, backslashAtEnd, dirName, curDiskPath, mask, 2 * MAX_PATH);' in sdk
    assert 'isTheSamePathF, 2 * MAX_PATH, MAX_PATH);' in sdk
    assert 'sourceBaseName.c_str(), sourceDirectory.c_str(), mask, SAL_MAX_PATH))' in read('src/fileswn8.cpp')
    assert 'backslashAtEnd || mustBePath, NULL, NULL, mask, SAL_MAX_PATH))' in read('src/fileswna.cpp')
    source = read('src/salamdr5.cpp')
    bounds = source[source.index('// S31b5b3b: capacities'):source.index('BOOL SalSplitWindowsPath(')]
    bounds += block(source, 'static BOOL ReadParsePathRoot(')
    windows = block(source, 'BOOL SalSplitWindowsPath(')
    general = block(source, 'BOOL SalSplitGeneralPath(')
    wide = read('src/common/widepath.cpp')
    utility = read('src/salamdr3.cpp')
    generated = read('src/tests/split_create_notification_tests.cpp.in')
    pieces = {
        'IDS': 'enum {' + ','.join(sorted(set(re.findall(r'IDS_[A-Z0-9_]+', windows+general+bounds)))) + '};',
        'WIDE': '\n'.join(block(wide, marker) for marker in (
            'BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(',
            'BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(',
            'std::string SalWideToMultiBytePath(')),
        'UTILS': block(read('src/salamdr1.cpp'),'int GetRootPath(')+'\n'+
                 '\n'.join(block(utility, marker) for marker in (
                     'void SalPathRemoveBackslash(', 'BOOL SalPathAddBackslash(',
                     'BOOL SalPathAppend(', 'BOOL SalCreateDirectoryExW('))+'\n'+
                 block(read('src/fileswn5.cpp'),'BOOL CutSpacesFromBothSides('),
        'SPLIT': bounds+'\n'+general+'\n'+windows,
    }
    for key, value in pieces.items(): generated = generated.replace('@@'+key+'@@', value)
    assert '@@' not in generated
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='split-create-') as temp:
        directory = Path(temp)
        cpp = directory/'test.cpp'
        cpp.write_text(generated, encoding='utf8')
        executable = directory/'test.exe'
        for config, flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration not in ('Both',config): continue
            print('Actual split/create notification:', config, flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/J','/W4','/WX',
                             *flags,str(cpp),'/Fe:'+str(executable)], [str(executable),str(directory/'owned-fixtures')]):
                result = run(command, cwd=directory, env=env, timeout=40)
                if result.returncode: return result.returncode
    return 0


if __name__ == '__main__': raise SystemExit(main())
