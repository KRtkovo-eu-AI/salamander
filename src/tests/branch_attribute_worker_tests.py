# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual attribute worker; real wide file/time I/O, deterministic failure and UI facades."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run, ROOT

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');p.add_argument('--source-ref');args=p.parse_args()
    code=read('src/tests/branch_attribute_worker_tests.cpp.in')
    pieces={'WORKER':block(read('src/worker.cpp',args.source_ref),'BOOL DoChangeAttrs('),'HEADER':str(ROOT/'src/workerpath.h').replace(chr(92),'/')}
    for k,v in pieces.items():code=code.replace('@@'+k+'@@',v)
    assert '@@' not in code
    env=compiler_environment();compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-attribute-worker-') as temp:
        d=Path(temp);source=d/'test.cpp';source.write_text(code,encoding='utf8');exe=d/'test.exe'
        for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration not in ('Both',c):continue
            print('Actual attribute worker:',c,flush=True)
            for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(source),'/Fe:'+str(exe)],[str(exe),str(d)]):
                result=run(cmd,cwd=d,env=env,timeout=35)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
