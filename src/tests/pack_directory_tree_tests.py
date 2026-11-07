# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual recursive Pack scanner over owned files, including long Unicode paths."""
import argparse
import shutil
from pathlib import Path
from branch_panel_getters_tests import read, block, compiler_environment, run
from zip_pack_preview_tests import owned_temporary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    wide = read('src/common/widepath.cpp')
    converters = '\n'.join(block(wide, marker) for marker in (
        'BOOL IsValidPathUtf8Text(', 'BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(',
        'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath('))
    source = read('src/tests/pack_directory_tree_tests.cpp.in')
    source = source.replace('@@WIDE@@', converters).replace('@@SCANNER@@', block(read('src/fileswn7.cpp'), 'BOOL _ReadDirectoryTree('))
    source = source.replace("#define SAL_MAX_PATH ", '#include "' + str(Path(__file__).resolve().parents[2] / "src/common/localizedtext.h").replace("\\", "/") + '"\n#define SAL_MAX_PATH ', 1)
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with owned_temporary() as directory:
        cpp, exe = directory / 'scanner.cpp', directory / 'scanner.exe'
        cpp.write_text(source, encoding='utf8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od', '/RTC1']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print(config, flush=True)
            for cmd in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags, str(cpp), '/Fe:' + str(exe)], [str(exe), str(directory / config)]):
                result = run(cmd, cwd=directory, env=env, timeout=40)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
