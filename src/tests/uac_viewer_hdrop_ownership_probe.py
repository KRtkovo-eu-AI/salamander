# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Probe WM_DROPFILES ownership. | Test real queue and DragFinish after a target is destroyed. | #731 | GPT-6 (Codex) |
"""Run a native ownership probe; this is not CViewerWindow or UAC."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args()
    env=compiler_environment()
    compiler=shutil.which('cl',path=env.get('PATH'))
    wait=block(read('src/viewer3.cpp'),'if (WaitForViewerRefresh &&')
    branch=block(wait,'case WM_DROPFILES:').replace('case WM_DROPFILES:',
        'static LRESULT RefreshDropBranch(WPARAM wParam)',1)
    fixture=read('src/tests/uac_viewer_hdrop_ownership_probe.cpp.in').replace('@@REFRESH_DROP_BRANCH@@',branch)
    if '@@REFRESH_DROP_BRANCH@@' in fixture:
        raise AssertionError('production branch not inserted')
    with tempfile.TemporaryDirectory(prefix='uac-viewer-hdrop-') as temp:
        folder=Path(temp)
        source=folder/'probe.cpp'
        source.write_text(fixture,encoding='utf-8')
        for config in (('Debug','Release') if args.configuration=='Both' else (args.configuration,)):
            flags=('/MTd','/Od','/D_DEBUG') if config=='Debug' else ('/MT','/O2','/DNDEBUG')
            exe=folder/(config+'.exe')
            print('Native WM_DROPFILES ownership:',config,flush=True)
            built=run([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,
                       str(source),'/Fe:'+str(exe),
                       '/link','user32.lib','shell32.lib'],cwd=folder,env=env,timeout=35)
            if built.returncode:
                return built.returncode
            result=run([str(exe)],cwd=folder,env=env,timeout=15)
            if result.returncode:
                return result.returncode
    return 0


if __name__=='__main__':
    raise SystemExit(main())
