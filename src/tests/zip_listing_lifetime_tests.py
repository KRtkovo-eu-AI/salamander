# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the actual ZIP listing/metadata lifetime and legacy host copy boundary."""
import argparse
from pathlib import Path
import shutil
import tempfile

import safe_file_paths_tests as native


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    parser.add_argument('--source-ref', help='Production revision for a negative control')
    args = parser.parse_args()
    listing = native.read('src/plugins/zip/list.cpp', args.source_ref)
    common = native.read('src/plugins/zip/common.h', args.source_ref)
    sdk = native.read('src/plugins/shared/spl_com.h')
    host = native.read('src/zip.cpp')
    pieces = {
        'SDK': native.block(sdk, 'struct CQuadWord', ';') + '\n' + native.block(sdk, 'struct CFileData //', ';'),
        'PLUGIN_DATA': native.block(common, 'class CZIPFileData', ';'),
        'FILE_INFO': native.block(native.read('src/plugins/zip/typecons.h'), 'struct CFileInfo', ';'),
        'DUP_NAME': native.block(listing, 'static wchar_t* DupZipUtf8NameW('),
        'LIST': native.block(listing, 'int CZipList::List('),
        'PACK_UNCOMPRESS': native.block(native.read('src/pack1.cpp', args.source_ref), '\nBOOL PackUncompress(HWND'),
        'PACK_OWNER_LOOKUP': native.block(native.read('src/pack1.cpp', args.source_ref), 'CPluginData* PackGetPluginForArchiveFile('),
        'CONTAINS': native.block(native.read('src/plugins.h'), 'BOOL Contains(CPluginDataInterfaceAbstract const* iface)'),
    }
    # Compile the real field-copy boundary which deliberately ignores the SDK
    # extension when a legacy binary plugin passes a shorter CFileData layout.
    for kind in ('File', 'Dir'):
        function = native.block(host, 'BOOL CSalamanderDirectory::Add' + kind + '(')
        start = function.index('    CFileData ' + kind.lower() + 'Copy;')
        end = function.index('CFileData& add' + kind + ' = ' + kind.lower() + 'Copy;', start)
        end += len('CFileData& add' + kind + ' = ' + kind.lower() + 'Copy;')
        pieces['COPY_' + kind.upper()] = function[start:end] + '\n    return add' + kind + ';'
    generated = native.read('src/tests/zip_listing_lifetime_tests.cpp.in')
    for key, value in pieces.items():
        generated = generated.replace('@@' + key + '@@', value)
    if '@@' in generated:
        raise AssertionError('Unexpanded ZIP lifetime placeholder')
    environment = native.compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='zip-list-lifetime-') as directory:
        source = Path(directory) / 'test.cpp'
        source.write_text(generated, encoding='utf-8')
        exe = Path(directory) / 'test.exe'
        for configuration in configurations:
            flags = ['/MDd', '/D_DEBUG', '/Od'] if configuration == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Actual ZIP listing lifetime:', configuration, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             '/D_CRT_SECURE_NO_WARNINGS', *flags, str(source), '/Fe:' + str(exe)], [str(exe)]):
                result = native.run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
