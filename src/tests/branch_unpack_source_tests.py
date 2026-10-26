# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual unpack source capture/conversions and real owned filesystem identity checks; no unpack engine facade."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
    source=read('src/fileswn7.cpp');wide=read('src/common/widepath.cpp')
    controller=block(source,'void CFilesWindow::Unpack(')
    assert 'if (target == NULL || !Is(ptDisk) || IsBranchView() || target->IsBranchView()) return;' in controller
    assert controller.index('PrepareUnpackArchiveSource(')<controller.index('CUnpackDialog(')
    assert 'ExecuteUnpacker(MainWindow->HWindow, this, archiveSource.c_str(), mask,' in controller
    after=controller[controller.index('CUnpackDialog('):]
    assert 'file->' not in after and 'sprintf(subject + l' not in after
    assert 'sourceErrorTextID, &sourceDirectory)' in controller and 'sourceDirectoryW = GetPathW()' not in controller
    text=read('src/tests/branch_unpack_source_tests.cpp.in')
    text=text.replace('@@ACTUAL@@',block(source,'static BOOL PrepareUnpackArchiveSource('))
    text=text.replace('@@CONVERSIONS@@','\n'.join(block(wide,m) for m in ('BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(', 'const wchar_t* SalPathFindFileNameW(')))
    assert '@@' not in text
    env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-unpack-source-') as temp:
        d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
        for config,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if a.configuration not in ('Both',config):continue
            print('Actual unpack archive source:',config,flush=True)
            for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe),str(d/'owned-fixtures')]):
                r=run(cmd,cwd=d,env=env,timeout=40)
                if r.returncode:return r.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
