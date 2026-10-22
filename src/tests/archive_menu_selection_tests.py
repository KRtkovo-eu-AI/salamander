# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual ZIP/7-Zip menu commands with a captured-selection and archive-engine harness."""
import argparse
from pathlib import Path
import shutil
import tempfile
from disk_selection_snapshot_tests import ROOT, block, compiler_environment, read, run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    parser.add_argument('--scope',choices=('ZIP','All'),default='ZIP',help='7-Zip commands are deferred to S14')
    args=parser.parse_args()
    zip_source=read('src/plugins/zip/main.cpp')
    seven_source=read('src/plugins/7zip/7zip.cpp') if args.scope=='All' else ''
    generated=read('src/tests/archive_menu_selection_tests.cpp.in')
    pieces={
        'FORMAT':block(zip_source,'static std::string ArchiveNameMessage('),
        'ZIP_COMMAND':block(zip_source,'BOOL CPluginInterfaceForMenuExt::ExecuteMenuItem('),
        'SEVEN_COMMAND':block(seven_source,'static BOOL TestArchive(') if args.scope=='All' else '',
    }
    generated=generated.replace('@@SEVEN_ENABLED@@','1' if args.scope=='All' else '0')
    for key,value in pieces.items():generated=generated.replace('@@'+key+'@@',value)
    if '@@' in generated:raise AssertionError('unexpanded fixture marker')
    environment=compiler_environment();compiler=shutil.which('cl',path=environment.get('PATH'))
    configurations=('Debug','Release') if args.configuration=='Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='archive-menu-selection-') as directory:
        directory=Path(directory);source=directory/'test.cpp';executable=directory/'test.exe'
        source.write_text(generated,encoding='utf-8')
        for configuration in configurations:
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('Archive menu native configuration:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/wd4100',*flags,
                             str(source),'/Fe:'+str(executable)], [str(executable)]):
                result=run(command,cwd=directory,env=environment,timeout=45)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
