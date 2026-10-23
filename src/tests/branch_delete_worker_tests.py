# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual delete worker over an owned temporary tree; UI and progress are facades."""
from pathlib import Path
import argparse, shutil, tempfile
from branch_panel_getters_tests import block, read, compiler_environment, run, ROOT

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');args=p.parse_args()
    worker=read('src/worker.cpp');validation=read('src/salamdr5.cpp')
    code=read('src/tests/branch_delete_worker_tests.cpp.in')
    pieces={'DELETE':block(worker,'BOOL DoDeleteFile('),'INVALID':block(validation,'BOOL FileNameIsInvalid('),'WORKER_HEADER':str(ROOT/'src/workerpath.h').replace(chr(92),'/'),'RECYCLE_HEADER':str(ROOT/'src/operationrecycle.h').replace(chr(92),'/')}
    for k,v in pieces.items():code=code.replace('@@'+k+'@@',v)
    assert '@@' not in code
    env=compiler_environment();compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-delete-worker-') as directory:
        directory=Path(directory);source=directory/'test.cpp';source.write_text(code,encoding='utf-8');exe=directory/'test.exe'
        for config,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration not in ('Both',config):continue
            print('Actual DoDeleteFile:',config,flush=True)
            for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(source),'/Fe:'+str(exe),'user32.lib','shell32.lib','ole32.lib','uuid.lib'],[str(exe),str(directory)]):
                result=run(cmd,cwd=directory,env=env,timeout=30)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
