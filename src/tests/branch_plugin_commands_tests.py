# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual private Branch plugin policy and dispatch with explicit metadata/callback facades."""
import argparse
from pathlib import Path
import re
import shutil
import tempfile
from disk_selection_snapshot_tests import ROOT, block, compiler_environment, read, run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args()
    source=read('src/plugins1.cpp')
    assert re.search(r'#define\s+MID_TEST\s+3\b', read('src/plugins/zip/main.cpp'))
    assert re.search(r'#define\s+CMD_REGENERATE_THUMBNAIL\s+188\b', read('src/plugins/pictview/pictview.rh2'))
    command=block(read('src/mainwnd3.cpp'),'static bool BranchReadOnlyCommandAllowed(')
    names=sorted(set(re.findall(r'\bCM_[A-Z0-9_]+\b',command))-{'CM_PLUGINCMD_MIN','CM_PLUGINCMD_MAX'}|{'CM_AFOCUSSHORTCUT','CM_EDITPROPERTIES','CM_CHANGEATTR','CM_COMPRESS','CM_UNCOMPRESS','CM_ENCRYPT','CM_DECRYPT'})
    generated=read('src/tests/branch_plugin_commands_tests.cpp.in')
    generated=generated.replace('@@CONSTANTS@@','enum { '+','.join(names)+', CM_PLUGINCMD_MIN=50000,CM_PLUGINCMD_MAX=51000 };')
    generated=generated.replace('@@POLICY@@',block(source,'BOOL CPluginData::IsBranchCommandAllowed('))
    generated=generated.replace('@@SUID_GATE@@',command)
    generated=generated.replace('@@DISPATCH@@','\n'.join(block(source,marker) for marker in (
        'BOOL CPluginData::ExecutePostedMenuItem(', 'BOOL CPluginData::ExecuteMenuItem(', 'BOOL CPluginData::ExecuteMenuItem2(', 'BOOL CPluginData::ExecuteToolbarCommand(')))
    generated=generated.replace('@@HOTKEY@@','\n'.join(block(read('src/plugins2.cpp'),marker) for marker in ('BOOL CPlugins::ExecuteCommand(', 'BOOL CPlugins::FindLastCommand(', 'BOOL CPlugins::OnLastCommand(')))
    auxiliary='\n'.join(block(read('src/plugins/salamatrix/salamatrix_api_docs.h'),m) for m in ('inline BOOL GetAutomationApiReferencePath(', 'inline BOOL OpenAutomationApiReference('))
    generated=generated.replace('@@AUXILIARY@@',auxiliary)
    generated=generated.replace('@@PICT_REFRESH@@',block(read('src/plugins/pictview/pictview.cpp'),'case CMD_INTERNAL_REREADTHUMBS:'))
    if '@@' in generated: raise AssertionError('unexpanded fixture marker')
    assert 'data->ExecutePostedMenuItem(GetActivePanel(), HWindow, (int)lParam, FALSE)' in read('src/mainwnd3.cpp')
    assert 'data->ExecutePostedMenuItem(MainWindow->GetActivePanel(), MainWindow->HWindow, id, TRUE)' in read('src/salamdr1.cpp')
    environment=compiler_environment();compiler=shutil.which('cl',path=environment.get('PATH'))
    configurations=('Debug','Release') if args.configuration=='Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='branch-plugin-commands-') as directory:
        directory=Path(directory);source_file=directory/'test.cpp';exe=directory/'test.exe'
        source_file.write_text(generated,encoding='utf-8')
        for configuration in configurations:
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('Branch plugin policy native:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/wd4100',*flags,str(source_file),'/Fe:'+str(exe)],[str(exe)]):
                r=run(command,cwd=directory,env=environment,timeout=40)
                if r.returncode:return r.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
