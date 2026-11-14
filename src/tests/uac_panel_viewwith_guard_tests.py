# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Test the protected panel viewer guard. | Exercise production guard logic against stale panel fixtures. | #731 | GPT-6 (Codex) |
"""Compile the production panel guard with explicit fake tab registry seams."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/fileswn5.cpp')
    fixture = read('src/tests/uac_panel_viewwith_guard_tests.cpp.in')
    fixture = fixture.replace('@@GUARD@@', block(source, 'struct ProtectedPanelViewerGuard', ';'))
    if '@@' in fixture:
        raise AssertionError('unexpanded fixture placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='panel-viewwith-guard-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(fixture, encoding='utf-8')
        for configuration in (('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)):
            flags = ['/MTd', '/Od', '/D_DEBUG'] if configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
            exe = folder / (configuration + '.exe')
            print('Panel viewer guard:', configuration, flush=True)
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(generated), '/Fe:' + str(exe), '/link', 'user32.lib'], cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            result = run([str(exe)], cwd=folder, env=env, timeout=20)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
