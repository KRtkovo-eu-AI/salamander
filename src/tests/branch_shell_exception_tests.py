# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-06 | Exercise actual Branch Shell exception boundaries and medium ownership. | Preserve complete selections and avoid unsafe medium cleanup when Shell extensions fault. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
"""Build the actual branchshell.h with isolated COM fault injection and real Shell transfer."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = (ROOT / 'src/tests/branch_shell_exception_tests.cpp.in').read_text(encoding='utf-8-sig')
    source = source.replace('@@BRANCH_SHELL_HEADER@@', (ROOT / 'src/branchshell.h').as_posix())
    if '@@' in source:
        raise AssertionError('Unexpanded actual-header fixture')
    environment = compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='branch-shell-exception-') as temporary:
        directory = Path(temporary).resolve(strict=True)
        cpp = directory / 'test.cpp'
        cpp.write_text(source, encoding='utf-8')
        manifest = directory / 'longpaths.manifest'
        manifest.write_text('<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"><application xmlns="urn:schemas-microsoft-com:asm.v3"><windowsSettings><longPathAware xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">true</longPathAware></windowsSettings></application></assembly>', encoding='utf-8')
        for configuration in configurations:
            output = directory / configuration
            output.mkdir()
            exe = output / 'test.exe'
            flags = ['/MDd', '/D_DEBUG', '/Od', '/RTC1'] if configuration == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Actual Branch Shell exception/ownership:', configuration, flush=True)
            commands = ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                         str(cpp), '/Fe:' + str(exe), '/link', '/MANIFEST:EMBED', '/MANIFESTINPUT:' + str(manifest), 'shell32.lib', 'ole32.lib', 'uuid.lib', 'user32.lib'],
                        [str(exe), str(directory)])
            for command in commands:
                result = run(command, cwd=output, env=environment, timeout=40)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
