# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual modal selection guard; assert integration order in the actual controller."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');p.add_argument('--mutation',choices=('tab','generation','paths','cache'));args=p.parse_args()
    source=read('src/fileswn5.cpp');controller=block(source,'void CFilesWindow::ChangeAttr(')
    assert 'setCompress || setEncryption || !branchSelection.Capture(this)' in controller
    assert controller.index('branchSelection.Capture(this)') < controller.index('BeginStopRefresh()')
    assert 'branchAttributes ? FALSE : FileBasedCompression' in controller
    assert 'branchAttributes ? FALSE : FileBasedEncryption' in controller
    assert 'chDlg.Compressed = 2;' in controller and 'chDlg.Encrypted = 2;' in controller
    check=controller.index('if (CheckPath(TRUE) != ERROR_SUCCESS || (branchAttributes && !branchSelection.IsCurrent(this)))')
    assert controller.index('chDlg.Execute()') < check < controller.index('GetSelItems(count, indexes)')
    assert 'BOOL res = (!branchAttributes || branchSelection.IsCurrent(this)) &&' in controller
    assert 'if (script->Count == 0 || (branchAttributes && !branchSelection.IsCurrent(this)))' in controller
    assert 'if (!branchAttributes || branchSelection.IsCurrent(this))\n                        {\n                            SetSel(FALSE' in controller
    assert 'if (!branchAttributes || branchSelection.IsCurrent(this)) UnselectItemWithName(selection)' in controller
    helper=block(source,'struct CBranchAttributeSelection')+';'
    mutations={'tab':('Tab == current.Tab &&','true &&'),'generation':('Generation == current.Generation &&','true &&'),'paths':('Paths == current.Paths','true'),'cache':('panel->GetBranchViewFileByCacheKey(panel->GetItemCacheKeyPtr(file)) != &file','false')}
    if args.mutation:
        old,new=mutations[args.mutation];assert helper.count(old)==1;helper=helper.replace(old,new)
    code=read('src/tests/branch_attribute_selection_tests.cpp.in').replace('@@GUARD@@',helper);assert '@@' not in code
    env=compiler_environment();compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-attribute-selection-') as temp:
        d=Path(temp);source=d/'test.cpp';source.write_text(code,encoding='utf8');exe=d/'test.exe'
        for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration not in ('Both',c):continue
            print('Actual attribute selection guard:',c,flush=True)
            for cmd in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(source),'/Fe:'+str(exe)],[str(exe)]):
                result=run(cmd,cwd=d,env=env,timeout=30)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
