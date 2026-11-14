# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Exercise actual executable-derived location checks. | Verify trusted arbitrary local directories and rejected ancestors. | #731 | GPT-6 |
# | 2026-11-13 | Test InstallLocation COM ownership. | Exercise actual known-folder handling under allocation failure. | #731 | GPT-6 |
"""Exercise extracted InstallLocation::Open with controlled image paths and directories."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    production = ROOT / 'src/uacbroker/uacbroker.cpp'
    before = hashlib.sha256(production.read_bytes()).hexdigest()
    text = read('src/uacbroker/uacbroker.cpp')
    begin = text.index('    bool Open()\n', text.index('class InstallLocation'))
    end = text.index('\n    Handle OpenImage(', begin)
    method = text[begin:end]
    if 'GetModuleFileNameW(nullptr' not in method:
        raise RuntimeError('Unexpected InstallLocation::Open implementation')
    template = (ROOT / 'src/tests/uac_install_location_ownership_tests.cpp.in').read_text(encoding='utf-8')
    image_begin = text.index('    std::wstring ImagePath(', end)
    image_end = text.index('\n    bool MatchesProcess(', image_begin)
    code = template.replace('@@OPEN_METHOD@@', method).replace('@@IMAGE_PATH_METHOD@@', text[image_begin:image_end])
    if '@@' in code:
        raise RuntimeError('Missing test template replacement')
    environment = compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MDd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MD', '/O2', '/DNDEBUG')
    base = ROOT.parent / 'builds'
    with tempfile.TemporaryDirectory(prefix='uac-install-location-ownership-', dir=base) as temp:
        source = Path(temp) / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        for thin in (False, True):
            exe = Path(temp) / ('test-thin.exe' if thin else 'test-ordinary.exe')
            thin_flags = ('/DSAM_UAC_THIN_COPY_TEST',) if thin else ()
            print(f'Actual InstallLocation::Open: {args.configuration}, thin={thin}', flush=True)
            built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                         *thin_flags, str(source), '/Fe:' + str(exe), '/link',
                         'ole32.lib', 'uuid.lib', 'advapi32.lib', 'shell32.lib'], cwd=temp, env=environment, timeout=60)
            if built.returncode:
                return built.returncode
            tested = run([str(exe)], cwd=temp, env=environment, timeout=20)
            if tested.returncode:
                return tested.returncode
    if hashlib.sha256(production.read_bytes()).hexdigest() != before:
        raise RuntimeError('uacbroker.cpp changed during test')
    print('uacbroker.cpp SHA-256:', before)
    print('Actual extracted InstallLocation::Open; image path and directory protection are controlled stubs.')
    print('This is not a complete build, real UAC, or isolated VM acceptance.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
