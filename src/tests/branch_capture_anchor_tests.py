# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Test the actual denied-directory anchor helper. | Exercise ACL denial, name replacement and cancellation. | #731 | GPT-6.1 Terra |
"""Compile the actual host helper with NTFS ACL and a deterministic parent-capture seam."""
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, run

def main():
    source=(ROOT/'src/branch_view.cpp').read_text(encoding='utf-8-sig')
    template=(ROOT/'src/tests/branch_capture_anchor_tests.cpp.in').read_text(encoding='utf-8-sig')
    template=template.replace('@@CAPTURE@@',block(source,'static bool CaptureBranchDenied('))
    env=compiler_environment(); compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-capture-anchor-') as temp:
        folder=Path(temp); generated=folder/'test.cpp'; generated.write_text(template,encoding='utf-8'); exe=folder/'test.exe'
        for mode,flags in [('Debug',['/MTd','/Od','/D_DEBUG']),('Release',['/MT','/O2','/DNDEBUG'])]:
            print('Production denied-anchor helper:',mode,flush=True)
            for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(generated),'/Fe:'+str(exe),'/link','advapi32.lib'],
                        [str(exe),str(folder)]):
                result=run(cmd,cwd=folder,env=env,timeout=30)
                if result.returncode:return result.returncode
    return 0

if __name__=='__main__':raise SystemExit(main())
