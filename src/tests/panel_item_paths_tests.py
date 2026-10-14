# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual ordinary panel getters and host service; adapted from PR #769 tests."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import tempfile
import safe_file_paths_tests as native


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    parser.add_argument('--verify-abi-source', action='store_true', help='Verify the frozen ABI hashes against the pinned release (requires history)')
    parser.add_argument('--query-source-ref', help='Negative control: earlier QueryService with current resolver')
    args = parser.parse_args()
    sdk = native.read('src/plugins/shared/spl_gen.h')
    # Frozen from the released base, so normal CI needs no historical Git object.
    # Full class bodies/layout remain protected, not just a count of virtuals.
    abi = json.loads(native.read('src/tests/panel_item_paths_abi.json'))
    assert abi['source_commit'] == 'a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449'
    def digest(text):
        return hashlib.sha256(text.replace('\r\n', '\n').encode('utf-8')).hexdigest()
    classes = abi['spl_gen_abstract_classes']
    for name, expected in classes.items():
        assert digest(native.block(sdk, 'class ' + name + '\n', ';')) == expected, name
    assert digest(native.read('src/plugins/shared/spl_com.h')) == abi['spl_com_sha256']
    if args.verify_abi_source:
        released = native.read('src/plugins/shared/spl_gen.h', abi['source_commit'])
        names = re.findall(r'class (\w+Abstract)\s*\n\{', released)
        assert set(names) == set(classes)
        for name, expected in classes.items():
            assert digest(native.block(released, 'class ' + name + '\n', ';')) == expected, name
        assert digest(native.read('src/plugins/shared/spl_com.h', abi['source_commit'])) == abi['spl_com_sha256']
        print('ABI fixture verified against pinned release:', abi['source_commit'], flush=True)
    print('Released SDK classes/layout unchanged:', len(classes), flush=True)
    common = native.read('src/plugins/shared/spl_com.h')
    wide = native.read('src/common/widepath.cpp')
    panel = native.read('src/fileswn1.cpp')
    host = native.read('src/zip.cpp')
    pieces = {
        'SDK': native.block(common, 'struct CQuadWord', ';') + '\n' + native.block(common, 'struct CFileData //', ';') + '\n' + '\n'.join(native.block(sdk, marker, ';') for marker in ('struct CSalamanderServiceQuery', 'struct CSalamanderServiceResult', 'class CSalamanderPanelItemPathsAbstract')),
        'CONSTANTS': '\n'.join(line for line in sdk.splitlines() if re.match(r'#define (PANEL_(SOURCE|TARGET|LEFT|RIGHT)|SALAMANDER_(SERVICE_PANEL_ITEM_PATHS|PANEL_ITEM_PATHS_VERSION_1_0|SERVICE_DISK_SELECTION|DISK_SELECTION_VERSION_1_0))\b', line)),
        'CONVERSION': native.block(wide, 'BOOL IsValidPathUtf8Text(') + '\n' + native.block(wide, 'std::wstring SalMultiByteToWidePath('),
        'JOIN': native.block(wide, 'BOOL SalPathAppendW('),
        'GETTERS': '\n'.join(native.block(panel, 'std::wstring CFilesWindow::' + name + '(') for name in ('GetItemDirectoryW', 'GetItemFullPathW', 'GetItemIdentityW', 'GetItemRelativePathW')),
        'SERVICE': native.block(host, 'class CPanelItemPathsService', ';'),
        'FACTORY': native.block(host, 'CSalamanderPanelItemPathsAbstract* GetPanelItemPathsService()'),
        'QUERY': native.block(native.read('src/zip.cpp', args.query_source_ref), 'BOOL CSalamanderGeneral::QueryService('),
    }
    generated = native.read('src/tests/panel_item_paths_tests.cpp.in')
    for name, value in pieces.items():
        generated = generated.replace('@@' + name + '@@', value)
    assert '@@' not in generated
    environment = native.compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='panel-item-paths-') as directory:
        source = Path(directory) / 'test.cpp'
        source.write_text(generated, encoding='utf-8')
        exe = Path(directory) / 'test.exe'
        for configuration in configurations:
            flags = ['/MDd', '/D_DEBUG', '/Od'] if configuration == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Actual ordinary panel paths and service:', configuration, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', '/D_CRT_SECURE_NO_WARNINGS', *flags, str(source), '/Fe:' + str(exe)], [str(exe)]):
                result = native.run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())