# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual 7zip listing ownership with explicit archive/host facades."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    client = read('src/plugins/7zip/7zclient.cpp')
    host = read('src/zip.cpp')
    def projection(method, variable):
        body = block(host, 'BOOL CSalamanderDirectory::' + method + '(')
        start = body.index('    CFileData ' + variable + 'Copy;')
        end = body.index('    CFileData& add' + method[3:] + ' = ' + variable + 'Copy;')
        return body[start:end]
    pieces = {
        'DUP': block(client, 'static wchar_t* DupUtf8NameW('),
        'GUARD': block(client, 'class CListedNameWGuard') + ';',
        'FREE': block(client, 'static void FreeListedFileDataName('),
        'ADD': block(client, 'BOOL C7zClient::AddFileDir('),
        'FILE_PROJECTION': projection('AddFile', 'file'),
        'DIR_PROJECTION': projection('AddDir', 'dir'),
    }
    generated = read('src/tests/sevenzip_listing_ownership_tests.cpp.in')
    for key, value in pieces.items():
        generated = generated.replace('@@' + key + '@@', value)
    assert '@@' not in generated
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='sevenzip-listing-ownership-') as temp:
        directory = Path(temp)
        source = directory / 'test.cpp'
        source.write_text(generated, encoding='utf-8')
        exe = directory / 'test.exe'
        for configuration in configurations:
            flags = ['/MDd', '/D_DEBUG', '/Od'] if configuration == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Actual 7zip listing ownership:', configuration, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             '/D_CRT_SECURE_NO_WARNINGS', *flags, str(source), '/Fe:' + str(exe)], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
