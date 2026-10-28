# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual Branch capture/restore/history methods with deterministic panel dependencies."""
import argparse, pathlib, runpy, tempfile, shutil
ROOT=pathlib.Path(__file__).resolve().parents[2]
helpers=runpy.run_path(str(ROOT/'src/tests/disk_selection_snapshot_tests.py'))
read,block,run=helpers['read'],helpers['block'],helpers['run']
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
 args=parser.parse_args()
 header=read('src/branch_view.h'); source=read('src/branch_view.cpp')
 pieces={'STATE':block(header,'struct CBranchViewRestoreState',';'),
 'METHODS':'\n'.join(block(source,s) for s in ('CBranchViewRestoreState CFilesWindow::CaptureBranchViewState(', 'void CFilesWindow::RestoreBranchViewState(')),
 'HISTORY':block(read('src/salamdr3.cpp'),'void CPathHistory::ChangeActualPathData(')}
 generated=read('src/tests/branch_restore_state_tests.cpp.in')
 for name,value in pieces.items():generated=generated.replace('@@'+name+'@@',value)
 assert '@@' not in generated
 env=helpers['compiler_environment']();compiler=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='branch-restore-state-') as temp:
  temp=pathlib.Path(temp);src=temp/'test.cpp';exe=temp/'test.exe';src.write_text(generated,encoding='utf-8')
  for config in ('Debug','Release') if args.configuration=='Both' else (args.configuration,):
   print('Branch restoration native configuration:',config,flush=True)
   flags=['/MDd','/D_DEBUG','/Od'] if config=='Debug' else ['/MD','/DNDEBUG','/O2']
   for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/wd4100','/wd4505',*flags,str(src),'/Fe:'+str(exe)],[str(exe)]):
    result=run(cmd,cwd=temp,env=env)
    if result.returncode:return result.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
