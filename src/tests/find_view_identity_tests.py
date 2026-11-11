# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise Find F3 identity routing against a real NTFS replacement. | Prevent the readable path probe from bypassing a captured identity. | #731 | GPT-6.1 Terra |
"""Run the production Find-view controller with real NTFS IDs and read primitive."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/fileswn5.cpp')
    fixture = read('src/tests/find_view_identity_tests.cpp.in')
    fixture = fixture.replace('@@INCLUDES@@', (ROOT / 'src/uacbroker').as_posix())
    fixture = fixture.replace('@@CONTROLLER@@', block(source, 'static bool PrepareProtectedViewerSnapshot('))
    fixture = fixture.replace('@@CANDIDATE@@', block(source, 'static bool AllowFindViewerCandidate('))
    fixture = fixture.replace('@@HANDLER_GUARD@@', block(source, 'if (foundView && (!viewer || viewer->ViewerType != VIEWER_INTERNAL))'))
    assert '@@' not in fixture
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='find-view-identity-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(fixture, encoding='utf-8')
        for configuration in (('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)):
            flags = ['/MTd', '/Od', '/D_DEBUG'] if configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
            exe = folder / (configuration + '.exe')
            print('Find viewer identity:', configuration, flush=True)
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(generated), '/Fe:' + str(exe), '/link', 'advapi32.lib', 'ole32.lib', 'ntdll.lib'],
                         cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            fixture_dir = folder / (configuration + '-fixture')
            fixture_dir.mkdir()
            result = run([str(exe), str(fixture_dir)], cwd=folder, env=env, timeout=20)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
