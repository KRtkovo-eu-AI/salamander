# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
"""Exercise actual CompareList scope and root identity checks on NTFS."""
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run

def main():
    source = (ROOT / 'src/uacbroker/uacbroker.cpp').read_text(encoding='utf-8-sig')
    assert 'UacListing::ValidVerifiedListingScope(scope, length)' in source
    assert 'UacListing::MatchesListingIdentity(identity, expected)' in source
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-root-') as temp:
        folder = Path(temp)
        for mode, flags in [('Debug', ['/MTd', '/Od', '/D_DEBUG']), ('Release', ['/MT', '/O2', '/DNDEBUG'])]:
            exe = folder / (mode + '.exe')
            print('Verified Branch root identity:', mode, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                             str(ROOT / 'src/tests/branch_root_identity_tests.cpp'), '/Fe:' + str(exe), '/link', 'advapi32.lib'],
                            [str(exe), str(folder)]):
                result = run(command, cwd=folder, env=env, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
