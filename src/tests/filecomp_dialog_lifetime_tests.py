# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the actual Compare dialog drop callback with real Win32 subclass chains."""
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
    source = native.read('src/plugins/filecomp/dialogs.cpp', args.source_ref)
    start = source.index('CCompareFilesDialog::DragDropEditProc(')
    start = source.rfind('\n', 0, start) + 1
    callback = native.block(source, source[start:source.index('\n', start)])
    signature = callback[:callback.index('{')].replace('CCompareFilesDialog::', '')
    install_start = source.index('        // I believe OldEditProc1') if 'OldEditProc1 =' in source else source.index('        SetWindowSubclass(hWnd1, DragDropEditProc')
    install_end = source.index('        DragAcceptFiles(hWnd1', install_start)
    generated = native.read('src/tests/filecomp_dialog_lifetime_tests.cpp.in')
    for key, value in {'CALLBACK': callback, 'SIGNATURE': signature,
                       'INSTALL': source[install_start:install_end]}.items():
        generated = generated.replace('@@' + key + '@@', value)
    if '@@' in generated:
        raise AssertionError('Unexpanded File Comparator lifecycle placeholder')
    environment = native.compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='filecomp-dialog-lifetime-') as directory:
        cpp = Path(directory) / 'test.cpp'
        cpp.write_text(generated, encoding='utf-8')
        exe = Path(directory) / 'test.exe'
        for configuration in configurations:
            flags = ['/MDd', '/D_DEBUG', '/Od'] if configuration == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Actual File Comparator drop callback lifecycle:', configuration, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             *flags, str(cpp), '/Fe:' + str(exe), 'user32.lib', 'comctl32.lib', 'shell32.lib'], [str(exe)]):
                result = native.run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
