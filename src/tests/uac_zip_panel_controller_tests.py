# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise protected ZIP read session setup. | Verify consent and session lifetime without a system UAC prompt. | #731 | GPT-6.1 Terra |
"""Compile the actual protected ZIP session setup with isolated consent seams."""
import argparse
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/fileswn2.cpp')
    first = source.index('DWORD ReadProtectedZip(')
    last = source.index('    ProtectedZipListing listing{*session};', first)
    setup = source[first:last] + '    (void)directory; (void)pluginData; (void)size; (void)date;\n    return ERROR_SUCCESS;\n}\n'
    fixture = read('src/tests/uac_zip_panel_controller_tests.cpp.in').replace('@@SETUP@@', setup)
    if '@@' in fixture:
        raise AssertionError('unexpanded fixture')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='uac-zip-panel-') as temporary:
        directory = Path(temporary)
        cpp = directory / 'test.cpp'
        cpp.write_text(fixture, encoding='utf-8')
        exe = directory / 'test.exe'
        for config in (('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)):
            flags = ['/MDd', '/D_DEBUG', '/Od'] if config == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Protected ZIP session setup:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             *flags, str(cpp), '/Fe:' + str(exe)], [str(exe)]):
                if run(command, cwd=directory, env=env, timeout=35).returncode:
                    return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
