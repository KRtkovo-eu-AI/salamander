# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual Branch shortcut dispatch; real ShellLink/wide I/O plus callback doubles."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args()
    wide=read('src/common/widepath.cpp')
    dispatcher=block(read('src/fileswn0.cpp'),'void CFilesWindow::FocusShortcutTarget(')
    dispatcher=dispatcher[:dispatcher.index('    char shortName')]+ '    ++LegacyCalls;\n}'
    pieces={
        'HELPERS':'\n'.join(block(wide,m) for m in ('BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathRemoveExtendedPrefixW(', 'BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')),
        'EXTENDED':block(read('src/branch_view.cpp'),'std::wstring ExtendedPath('),
        'FOCUS':block(read('src/branch_view.cpp'),'void CFilesWindow::FocusBranchShortcutTarget('),
        'DISPATCH':dispatcher,
    }
    generated=read('src/tests/branch_shortcut_target_tests.cpp.in')
    for k,v in pieces.items():generated=generated.replace('@@'+k+'@@',v)
    assert '@@' not in generated
    env=compiler_environment();compiler=shutil.which('cl',path=env.get('PATH'))
    configs=('Debug','Release') if args.configuration=='Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='branch-shortcut-target-') as temp:
        d=Path(temp);source=d/'test.cpp';source.write_text(generated,encoding='utf8');exe=d/'test.exe'
        for c in configs:
            flags=['/MDd','/D_DEBUG','/Od'] if c=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('Actual Branch shortcut target:',c,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,str(source),'/Fe:'+str(exe),'ole32.lib','shell32.lib','uuid.lib','user32.lib'],[str(exe)]):
                r=run(command,cwd=d,env=env,timeout=35)
                if r.returncode:return r.returncode
    return 0


if __name__=='__main__':raise SystemExit(main())
