# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise the protected panel transition controller. | Preserve old sessions on denial while committing only verified new listings. | #731 | GPT-6.1 Terra |
"""Compile the production panel transition with simulated filesystem and consent seams."""
import argparse
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    code = read('src/fileswn2.cpp')
    close = block(code, 'void CFilesWindow::CloseCurrentPath(')
    assert 'if (!cancel && !KeepProtectedListing)' in close
    assert 'if (!cancel && (!newPathIsTheSame || !isRefresh)) LeaveBranchView();' in close
    archive = close[close.index('if (Is(ptZIPArchive))'):]
    assert archive.index('if (!cancel)') < archive.index('ProtectedZipSession.reset();')
    scope = code[code.index('BOOL CFilesWindow::ChangePathToDisk('):]
    listing = scope[scope.index('        bool protectedPath = false;'):scope.index('        if (protectedPath)\n        {\n            err =')]
    commit = scope[scope.index('                KeepProtectedListing = protectedPath'):scope.index('                if (samePath)', scope.index('                KeepProtectedListing = protectedPath'))]
    generated = read('src/tests/uac_panel_transition_tests.cpp.in').replace('@@LISTING@@', listing).replace('@@COMMIT@@', commit)
    if '@@' in generated:
        raise AssertionError('unexpanded fixture')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='uac-panel-transition-') as temp:
        folder = Path(temp)
        source = folder / 'test.cpp'
        source.write_text(generated, encoding='utf-8')
        executable = folder / 'test.exe'
        for config in (('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)):
            flags = ['/MDd', '/D_DEBUG', '/Od'] if config == 'Debug' else ['/MD', '/DNDEBUG', '/O2']
            print('Protected panel transition:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                             *flags, str(source), '/Fe:' + str(executable)], [str(executable)]):
                if run(command, cwd=folder, env=env, timeout=35).returncode:
                    return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
