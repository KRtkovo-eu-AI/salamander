# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise real Find preparation against denied NTFS attributes. | Bind scoped consent to captured file and directory IDs. | #731 | GPT-6.1 Terra |
"""Run the production PrepareTarget body with an isolated installation-trust seam."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    source = (ROOT / 'src/uacbroker/uacbroker.cpp').read_text(encoding='utf-8-sig')
    pieces = {
        'INCLUDES': str(ROOT / 'src/uacbroker').replace('\\', '/'),
        'OPEN_ROOT': block(source, 'Handle OpenVolumeRoot('),
        'OPEN_PARENT': block(source, 'DWORD OpenRequestParent('),
        'RENAME_KIND': block(source, 'UacRenameLeaf::MoveKind RenameKind('),
        'DESTRUCTOR': block(source, 'PreparedLeaf::~PreparedLeaf('),
        'PREPARE': block(source, 'bool PreparedLeaf::PrepareTarget('),
    }
    template = (ROOT / 'src/tests/find_identity_preparation_tests.cpp.in').read_text(encoding='utf-8')
    for name, value in pieces.items():
        template = template.replace('@@' + name + '@@', value)
    if '@@' in template:
        raise RuntimeError('unexpanded test placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ['/MTd', '/Od', '/D_DEBUG'] if args.configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='uac-find-preparation-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(template, encoding='utf-8')
        exe = folder / 'test.exe'
        result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                      str(generated), '/Fe:' + str(exe), '/link', 'advapi32.lib', 'ole32.lib', 'ntdll.lib'],
                     cwd=folder, env=env, timeout=35)
        if result.returncode:
            return result.returncode
        target = folder / 'junction-target'
        junction = folder / 'junction'
        target.mkdir()
        env['SAMANDARIN_FIND_FIXTURE'] = str(folder)
        created = run(['powershell.exe', '-NoProfile', '-Command',
                       "$f=$env:SAMANDARIN_FIND_FIXTURE; "
                       "New-Item -ItemType Junction -Path (Join-Path $f 'junction') "
                       "-Target (Join-Path $f 'junction-target') -ErrorAction Stop | Out-Null"],
                      cwd=folder, env=env, timeout=15)
        if created.returncode:
            return created.returncode
        try:
            return run([str(exe), str(folder)], cwd=folder, env=env, timeout=20).returncode
        finally:
            junction.rmdir()


if __name__ == '__main__':
    raise SystemExit(main())
