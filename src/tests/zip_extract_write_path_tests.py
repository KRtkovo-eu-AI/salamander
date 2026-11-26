# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise the common ZIP extraction write path. | Check the real output control flow without implying full decoder coverage. | #731 | GPT-6.1 Terra |
"""Compile actual ZIP output, flush, and STORE methods against explicit test facades."""
import argparse
import re
import shutil
import tempfile
from pathlib import Path

from branch_panel_getters_tests import block, read, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/plugins/zip/extract.cpp')
    header = read('src/plugins/zip/extract.h')
    text = read('src/tests/zip_extract_write_path_tests.cpp.in')
    methods = {
        'WRITE': block(source, 'int CZipUnpack::WriteExtractBytes('),
        'FLUSH': block(source, 'int ExtractFlush('),
        'STORE': block(source, 'int CZipUnpack::UnStoreFile('),
    }
    assert 'WriteExtractBytes' in header
    assert 'WriteExtractBytes(buf, bytes)' in methods['FLUSH']
    assert 'WriteExtractBytes(InputBuffer, readBytes)' in methods['STORE']
    for method in ('InflateFile', 'ExplodeFile', 'UnShrinkFile', 'UnReduceFile', 'UnBZIP2File'):
        assert 'output.Flush = ExtractFlush;' in block(source, 'int CZipUnpack::' + method + '('), method
    ids = sorted(set(re.findall(r'\bIDS_[A-Z0-9_]+\b', text + '\n'.join(methods.values()))))
    text = text.replace('@@IDS@@', 'enum {' + ','.join(key + '=' + str(100 + i) for i, key in enumerate(ids)) + '};')
    for key, value in methods.items():
        text = text.replace('@@' + key + '@@', value)
    assert '@@' not in text
    env = compiler_environment()
    cl = shutil.which('cl', path=env.get('PATH'))
    if not cl:
        raise RuntimeError('MSVC compiler not found')
    with tempfile.TemporaryDirectory(prefix='zip-extract-write-') as tmp:
        directory = Path(tmp)
        cpp = directory / 'test.cpp'
        cpp.write_text(text, encoding='utf8')
        exe = directory / 'test.exe'
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual ZIP output control flow with simulated I/O, CRC and AES:', config, flush=True)
            for command in ([cl, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                             str(cpp), '/Fe:' + str(exe)], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=40)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
