# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Observe WM_DESTROY on a private Windows desktop. | Check visibility without activating or switching the user's desktop. | #731 | GPT-6 |
"""Real Win32 private-desktop probe; does not exercise UAC or the active desktop."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release'),default='Debug')
    args=parser.parse_args()
    env=compiler_environment(); compiler=shutil.which('cl',path=env.get('PATH'))
    if not compiler: raise RuntimeError('MSVC cl.exe unavailable')
    flags=('/MTd','/Od','/D_DEBUG') if args.configuration=='Debug' else ('/MT','/O2','/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-focus-private-desktop-',dir=ROOT.parent/'builds') as temp:
        exe=Path(temp)/'probe.exe'; source=ROOT/'src/tests/uac_progress_destroy_desktop_probe.cpp'
        built=run([compiler,'/nologo','/std:c++17','/EHsc','/W4','/WX',*flags,str(source),'ole32.lib','user32.lib','/Fe:'+str(exe)],cwd=temp,env=env,timeout=60)
        if built.returncode: return built.returncode
        result=run([str(exe)],cwd=temp,env=env,timeout=20)
        return result.returncode

if __name__=='__main__': raise SystemExit(main())
