# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual ZIP comment save control flow and wide title formatting."""
import argparse
from pathlib import Path
import shutil
import tempfile
from disk_selection_snapshot_tests import block,compiler_environment,read,run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args();code=read('src/tests/zip_comment_paths_tests.cpp.in')
    pieces={'WIDE':block(read('src/plugins/shared/spl_diskselection.h'),'inline std::wstring WideFromPath('),'TITLE':block(read('src/plugins/zip/dialogs3.cpp'),'static std::wstring CommentArchiveTitleW('),'SAVE':block(read('src/plugins/zip/add.cpp'),'int CZipPack::SaveComment(')}
    for name,text in pieces.items():code=code.replace('@@'+name+'@@',text)
    init=block(read('src/plugins/zip/dialogs3.cpp'),'BOOL CCommentDialog::OnInit(')
    assert 'DialogBoxParamW(HLanguage, MAKEINTRESOURCEW(IDD_COMMENT)' in block(read('src/plugins/zip/dialogs3.cpp'),'CCommentDialog::Proceed(')
    assert 'SetWindowTextW(Dlg, title.c_str())' in init and 'catch (const std::bad_alloc&)' in init and 'char title[' not in init
    environment=compiler_environment();compiler=shutil.which('cl',path=environment.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='zip-comment-paths-') as directory:
        directory=Path(directory);cpp=directory/'test.cpp';exe=directory/'test.exe';cpp.write_text(code,encoding='utf8')
        for configuration in ('Debug','Release') if args.configuration=='Both' else (args.configuration,):
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('ZIP actual comment functions:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
                result=run(command,cwd=directory,env=environment,timeout=40)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
