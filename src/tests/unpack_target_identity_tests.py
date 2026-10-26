# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual directory snapshot; explicit registry doubles and controller placement contracts."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def controller_contract():
 c=block(read('src/fileswn7.cpp'),'void CFilesWindow::UnpackZIPArchive(')
 live=c.index('CFilesActionDiskTarget::FindLive(');first=c.index('target->IsBranchView()');capture=c.index('diskTarget.Capture(MainWindow, target, TRUE)');update=c.index('MainWindow->UpdateDefaultDir(')
 assert live<first<capture<update
 assert c[update:c.index('BeginStopRefresh();')].count('validateDiskTarget()')==1
 dialog=c.index('(int)dlg.Execute()');classify=c.index('const bool useDefaultDiskTarget =');normalize=c.index('SlashesToBackslashesAndRemoveDups(path)');parse=c.index('const BOOL parsed = ParsePath(');preflight=c.index('!diskTarget.MatchesParsedDirectoryDefault(');confirm=c.index('SalMessageBoxEx(&params)');create=c.index('SalCreateDirectoryExW(');backend=c.index('const BOOL unpacked = PackUncompress(');post=c.index('const bool targetCurrent = validateDiskTarget();');clear=c.index('if (unpacked && targetCurrent)')
 assert dialog<classify<normalize<parse<preflight<confirm<create<backend<post<clear
 assert 'tgtPath == NULL' in c[classify:normalize]
 for start,end in ((dialog,classify),(classify,normalize),(parse,preflight),(confirm,create),(create,backend)):
  assert 'if (!validateDiskTarget())' in c[start:end],(start,end)
 loop=c.index('while (1)',confirm);assert loop<c.index('if (!validateDiskTarget())',loop)<create
 assert 'createdAny = true;' in c[create:backend]
 assert 'ReadUnpackDiskDestination(diskTarget.Root.c_str()' in c
 assert 'watchDiskTarget ? &diskTarget : NULL' in c[backend:post]
 assert 'else if (!unpacked)' in c[clear:]
 assert 'PackGetVerifiedBranchUnpacker(this, GetZIPArchive(), PluginData.GetInterface()) == NULL' in c
 assert '(deleteOp || !target->Is(ptDisk) ||' in c
 reject=c.index('if (watchDiskTarget && diskTarget.Branch && !useDefaultDiskTarget)')
 assert classify<reject<normalize<parse
 assert 'IDS_PATHISINVALID' in c[reject:normalize]
 validate=c[c.index('auto validateDiskTarget'):update]
 assert 'diskTarget.IsCurrent(MainWindow)' in validate
 assert 'PackGetVerifiedBranchUnpacker(this, GetZIPArchive(), PluginData.GetInterface()) != NULL' in validate
 assert '#define PATH_TYPE_WINDOWS 1 ' in read('src/plugins/shared/spl_gen.h')
 print('Archive directory target controller placement PASS',flush=True)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args();controller_contract()
 wide=read('src/common/widepath.cpp');text=read('src/tests/disk_target_identity_tests.cpp.in').split('int main()',1)[0]+read('src/tests/unpack_target_identity_tests.cpp.in')
 pieces={'ACTION':block(read('src/fileswnd.h'),'enum CActionType')+';','WIDE':'\n'.join(block(wide,m) for m in ('BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')),'SNAPSHOT':block(read('src/fileactiondisktarget.h'),'struct CFilesActionDiskTarget')+';'}
 for k,v in pieces.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text;env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='unpack-target-identity-') as temp:
  d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual unpack directory identity:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
