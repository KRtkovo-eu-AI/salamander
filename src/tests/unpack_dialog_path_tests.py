# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Unpack target transfer against real Unicode HWND controls; transfer/config scaffolding is explicit."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 source=read('src/dialogs3.cpp');wide=read('src/common/widepath.cpp');text=read('src/tests/unpack_dialog_path_tests.cpp.in')
 pieces={'DECL':block(read('src/dialogs.h'),'class CUnpackDialog')+';','ACTUAL':'\n'.join(block(source,m) for m in ('static BOOL ReadUnpackDialogPath(', 'CUnpackDialog::CUnpackDialog(', 'void CUnpackDialog::EnableDelArcCheckbox(', 'void CUnpackDialog::Transfer(')),'CONVERSIONS':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath('))}
 for k,v in pieces.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text
 controller=block(read('src/fileswn7.cpp'),'void CFilesWindow::Unpack(');assert 'CUnpackDialog(HWindow, path, pathBuffer.Capacity(), pathAlt, mask,' in controller
 base=block(read('src/common/winlib.cpp'),'BOOL CDialog::TransferData(');assert 'if (!ti.IsGood())' in base and 'return FALSE;' in base
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-dialog-path-') as temp:
  d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual Unicode Unpack dialog transfer:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe),'user32.lib'],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
