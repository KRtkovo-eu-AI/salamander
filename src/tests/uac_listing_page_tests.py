# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Run bounded native directory-page tests. | Validate identity and short names on NTFS. | #731 | GPT-6 (Codex) |
import argparse
import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--arch', choices=('x64', 'x86'), default='x64')
    arch = parser.parse_args().arch
    env = compiler_environment()
    if arch == 'x86':
        vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        installation = run([str(vswhere), '-latest', '-products', '*', '-requires',
                            'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
                           check=True, stdout=subprocess.PIPE, timeout=10).stdout.decode('utf-8-sig').strip()
        if not installation:
            raise RuntimeError('MSVC C++ tools were not found')
        setup = Path(installation) / 'Common7/Tools/VsDevCmd.bat'
        command = f'cmd /d /s /c "call "{setup}" -arch=x86 -host_arch=x64 >nul && set"'
        output = run(command, check=True, stdout=subprocess.PIPE, timeout=30).stdout.decode(errors='replace')
        for line in output.splitlines():
            if '=' in line and not line.startswith('='):
                key, value = line.split('=', 1)
                env[key.upper()] = value
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='uac-listing-page-') as temp:
        folder = Path(temp)
        for mode, flags in [('Debug', ['/MTd', '/Od', '/D_DEBUG']), ('Release', ['/MT', '/O2', '/DNDEBUG'])]:
            print('Native listing pages:', arch, mode, flush=True)
            exe = folder / (mode + '.exe')
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(ROOT / 'src/tests/uac_listing_page_tests.cpp'), '/Fe:' + str(exe),
                          '/link', 'ntdll.lib', 'advapi32.lib'], cwd=folder, env=env, timeout=45)
            if result.returncode:
                return result.returncode
            result = run([str(exe), str(folder)], cwd=folder, env=env, timeout=60)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
