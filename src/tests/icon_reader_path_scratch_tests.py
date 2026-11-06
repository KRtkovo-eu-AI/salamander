# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the actual icon-reader path snapshot and heap scratch preparation."""
import argparse
from pathlib import Path
import shutil
import tempfile

import safe_file_paths_tests as native


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    worker = native.read('src/fileswn1.cpp')
    worker = native.block(worker, 'unsigned IconThreadThreadFBody(void* parameter)')
    start = worker.index('CPathBuffer iconPathBuffer(')
    end = worker.index('BOOL readOnlyVisibleItems =', start)
    buffers = native.read('src/common/widepath.cpp')
    definitions = '\n'.join(native.block(buffers, marker) for marker in (
        'CPathBuffer::CPathBuffer(int reserveChars)', 'char* CPathBuffer::Data()',
        'int CPathBuffer::Capacity() const', 'CWidePathBuffer::CWidePathBuffer(int reserveChars)',
        'wchar_t* CWidePathBuffer::Data()', 'int CWidePathBuffer::Capacity() const',
        'std::string SalWideToMultiBytePath(const wchar_t* path, UINT codePage)'))
    generated = native.read('src/tests/icon_reader_path_scratch_tests.cpp.in')
    for key, value in {'PREPARE': worker[start:end], 'BUFFERS': definitions,
                       'HEADER': str(native.ROOT / 'src/common/widepath.h').replace('\\', '/')}.items():
        generated = generated.replace('@@' + key + '@@', value)
    if '@@' in generated:
        raise AssertionError('Unexpanded icon-reader fixture placeholder')
    environment = native.compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='icon-reader-path-scratch-') as directory:
        source = Path(directory) / 'test.cpp'
        source.write_text(generated, encoding='utf-8')
        exe = Path(directory) / 'test.exe'
        for configuration in configurations:
            flags = ['/MDd', '/D_DEBUG', '/Od', '/RTC1'] if configuration == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Actual icon-reader path snapshot:', configuration, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             *flags, str(source), '/Fe:' + str(exe)], [str(exe)]):
                result = native.run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
