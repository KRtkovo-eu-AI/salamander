# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual NTFS helpers and attribute worker; real compression/EFS capability, fault/UI facades."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run, ROOT

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');p.add_argument('--source-ref');a=p.parse_args()
    worker=read('src/worker.cpp',a.source_ref);code=read('src/tests/branch_ntfs_worker_tests.cpp.in')
    pieces={'WORKER':block(worker,'BOOL DoChangeAttrs('),'HEADER':str(ROOT/'src/workerpath.h').replace(chr(92),'/'),'NTFS':'\n'.join(block(worker,m) for m in ['DWORD CompressFile(', 'DWORD UncompressFile(', 'DWORD MyEncryptFile(', 'DWORD MyDecryptFile('])}
    for k,v in pieces.items():code=code.replace('@@'+k+'@@',v)
    assert '@@' not in code
    env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-ntfs-worker-') as temp:
        d=Path(temp);source=d/'test.cpp';source.write_text(code,encoding='utf8');exe=d/'test.exe'
        for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if a.configuration not in ('Both',c):continue
            print('Actual NTFS worker:',c,flush=True)
            for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(source),'/Fe:'+str(exe),'advapi32.lib'],[str(exe),str(d)]):
                result=run(cmd,cwd=d,env=env,timeout=38)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
