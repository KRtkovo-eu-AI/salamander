# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Copy/Move path Transfer and history against real Unicode HWND controls."""
import argparse
from pathlib import Path
import re
import shutil
import tempfile
from branch_panel_getters_tests import block, read, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug','Release','Both'), default='Both')
    args = parser.parse_args()
    source = read('src/dialogs3.cpp')
    wide = read('src/common/widepath.cpp')
    actual = '\n'.join(block(source, marker) for marker in (
        'HWND ResolveComboEditControl(', 'static BOOL ReadUnpackDialogPath(',
        'CCopyMoveMoreDialog::CCopyMoveMoreDialog(', 'CCopyMoveMoreDialog::~CCopyMoveMoreDialog(',
        'void CCopyMoveMoreDialog::Transfer('))
    option_enums = block(read('src/cfgdlg.h'), 'enum CCopyMoveConflictMode') + ';'
    scheduler = read('src/storagesched.h')
    for marker in ('CMS_SEQUENTIAL', 'COSP_STORAGE_AWARE', 'COSO_DEFAULT'):
        start = scheduler.rfind('enum', 0, scheduler.index(marker))
        option_enums += '\n' + block(scheduler[start:], 'enum') + ';'
    tokens = sorted(set(re.findall(r'(?:IDS|IDE|IDC|IDD|CMS|CMCM|COSP|COSO)_[A-Z0-9_]+',actual)))
    pieces = {
        'IDS':option_enums+'\nenum {'+', '.join(name+'='+str(100+i) for i,name in enumerate(name for name in tokens if not name.startswith(('CMS_', 'CMCM_', 'COSP_', 'COSO_'))))+'};',
        'CONVERSIONS':'\n'.join(block(wide,marker) for marker in (
            'BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(',
            'std::string SalWideToMultiBytePath(')),
        'HISTORY':block(read('src/salamdr6.cpp'),'void AddValueToStdHistoryValues('),
        'DECL':block(read('src/dialogs.h'),'class CCopyMoveMoreDialog')+';',
        'ACTUAL':actual,
    }
    text = read('src/tests/copy_move_dialog_path_tests.cpp.in')
    for key,value in pieces.items(): text=text.replace('@@'+key+'@@',value)
    assert '@@' not in text
    base=block(read('src/common/winlib.cpp'),'BOOL CDialog::TransferData(')
    assert 'if (!ti.IsGood())' in base and 'return FALSE;' in base
    env=compiler_environment()
    compiler=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='copy-move-dialog-') as temp:
        directory=Path(temp);cpp=directory/'test.cpp';exe=directory/'test.exe'
        cpp.write_text(text,encoding='utf8')
        for config,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration not in ('Both',config): continue
            print('Actual atomic Copy/Move dialog:',config,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/J','/W4','/WX',
                            *flags,str(cpp),'/Fe:'+str(exe),'user32.lib'],*([str(exe), codepage] for codepage in ('system', '1250', '1252', '65001'))):
                result=run(command,cwd=directory,env=env,timeout=40)
                if result.returncode:return result.returncode
    return 0


if __name__=='__main__':raise SystemExit(main())
