# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual host archive identity/epoch with explicit panel registry doubles."""
import argparse,shutil,tempfile,re
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');p.add_argument('--negative-control',action='store_true',help='Remove only the generated enumerator guards to prove regression detection');a=p.parse_args()
 h=read('src/fileswnd.h');release=block(read('src/fileswn1.cpp'),'void CFilesWindowAncestor::ReleaseListing(')
 assert release.index('AdvanceListingIdentity();')<release.index('ReleaseListingBody(')
 for marker in ('void CFilesWindowAncestor::SetZIPPath(', 'void CFilesWindowAncestor::SetZIPArchive('):assert 'AdvanceListingIdentity();' in block(read('src/fileswn1.cpp'),marker)
 assert 'AdvanceListingIdentity();' in block(read('src/fileswn2.cpp'),'void CFilesWindow::InvalidateChangesInPanelWeHaveNewListing(')
 for marker in ('void SetPanelType(', 'void SetArchiveDir('):assert 'AdvanceListingIdentity();' in block(h,marker)
 text=read('src/tests/archive_source_identity_tests.cpp.in')
 parts={'EPOCH':'\n'.join(block(h,x) for x in ('ULONGLONG GetListingIdentityGeneration(', 'void AdvanceListingIdentity(')), 'FINDLIVE':block(read('src/fileactiondisktarget.h'),'static CFilesWindow* FindLive('), 'SOURCE':block(read('src/fileactionarchivesource.h'),'struct CFilesActionArchiveSource')+';'}
 enum_source=read('src/fileswn7.cpp')
 current=read('src/fileswn7.cpp')
 parts.update({'CONSTANTS':'\n'.join(re.findall(r'^#define SALENUM_(?:SUCCESS|ERROR|CANCEL)\b[^\n]*',read('src/plugins/shared/spl_com.h'),re.M)),
  'ENUM_STATE':block(h,'struct CPanelTmpEnumData')+';',
  'ENUM_METHODS':'\n'.join(block(current,x) for x in ('CPanelTmpEnumData::CPanelTmpEnumData(', 'CPanelTmpEnumData::~CPanelTmpEnumData(', 'void CPanelTmpEnumData::Reset(')),
  'ENUMERATOR':block(enum_source,'const char* _PanelSalEnumSelection('),
  'PUBLIC_ENUMERATOR':block(enum_source,'const char* WINAPI PanelSalEnumSelection(')})
 if a.negative_control:
  assert parts['ENUMERATOR'].count('if (!data->HasCurrentContext())')==2
  for _ in range(2):
   guard=block(parts['ENUMERATOR'],'if (!data->HasCurrentContext())')
   parts['ENUMERATOR']=parts['ENUMERATOR'].replace(guard,'',1)
 for k,v in parts.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='archive-source-identity-') as temp:
  d=Path(temp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual archive source identity:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
