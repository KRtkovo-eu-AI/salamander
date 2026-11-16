# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise real Branch sharing fallback. | Keep ordinary readable entries under an existing DELETE handle. | #731 | GPT-6.1 Terra |
"""Test real Windows sharing behavior through the Branch worker seam."""
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run

def main():
    env=compiler_environment(); compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-sharing-') as temp:
        folder=Path(temp); exe=folder/'test.exe'
        for mode,flags in [('Debug',['/MTd','/Od','/D_DEBUG']),('Release',['/MT','/O2','/DNDEBUG'])]:
            print('Branch sharing fallback:',mode,flush=True)
            for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/DBRANCH_VIEW_TEST',*flags,
                         str(ROOT/'src/branch_view.cpp'),str(ROOT/'src/tests/branch_sharing_fallback_tests.cpp'),
                         '/Fe:'+str(exe),'/link','user32.lib'],[str(exe),str(folder)]):
                result=run(cmd,cwd=folder,env=env,timeout=30)
                if result.returncode:return result.returncode
    return 0

if __name__=='__main__':raise SystemExit(main())
