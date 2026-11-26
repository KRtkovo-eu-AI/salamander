# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual Ctrl+M host generators and enumeration; formatter is stubbed, no UAC is exercised."""
import argparse
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    wide = read('src/common/widepath.cpp')
    enumeration = read('src/fileswn7.cpp')
    generator = read('src/fileswn6.cpp')
    template = read('src/tests/uac_makefilelist_generation_tests.cpp.in')
    converters = '\n'.join(block(wide, marker) for marker in (
        'BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(',
        'std::wstring SalPathRemoveExtendedPrefixW(', 'std::wstring SalMultiByteToWidePath(',
        'std::string SalWideToMultiBytePath(', 'BOOL SalPathAppendW('))
    structs = enumeration[enumeration.index('struct CDiskDirEnumEntry'):enumeration.index('static std::wstring DiskDirFileNameW(')]
    actual = structs + '\n' + '\n'.join(block(enumeration, marker) for marker in (
        'static std::wstring DiskDirFileNameW(', 'static std::string DiskDirFileNameUtf8(',
        'static bool DiskDirSamePath(', 'static bool DiskDirAddEnumEntry(',
        'static bool DiskDirReportEnumerationError(', 'static bool DiskDirExcludeWorkspace(',
        'static bool DiskDirEnumDirectory(',
        'static bool DiskDirBuildEnumeration(', 'static void DiskDirPrepareFileListEntry(',
        'BOOL CFilesWindow::MakeFileListRecursive('))
    actual += '\n' + block(generator, 'BOOL CFilesWindow::MakeFileList(')
    text = template.replace('@@CONVERTERS@@', converters).replace('@@ACTUAL@@', actual)
    assert '@@' not in text
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC compiler unavailable')
    with tempfile.TemporaryDirectory(prefix='makefilelist-generation-') as temp:
        directory = Path(temp)
        cpp, exe = directory / 'test.cpp', directory / 'test.exe'
        cpp.write_text(text, encoding='utf-8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od', '/RTC1']),
                              ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual host generators; stub formatter; no UAC:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             *flags, str(cpp), '/Fe:' + str(exe)], [str(exe), str(directory / config)]):
                result = run(command, cwd=directory, env=env, timeout=40)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
