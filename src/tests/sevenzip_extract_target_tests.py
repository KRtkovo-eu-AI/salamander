# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual 7-Zip callbacks and vendor stream I/O; host UI facades are explicit."""
import argparse,shutil,tempfile,re
from pathlib import Path
from branch_panel_getters_tests import ROOT,block,read,compiler_environment,run

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args();source=read('src/plugins/7zip/extract.cpp');structs=read('src/plugins/7zip/structs.h');host=read('src/plugins/7zip/7zip.cpp')
 data=read('src/tests/sevenzip_extract_target_tests.cpp.in');methods='\n'.join(block(source,m) for m in ('STDMETHODIMP CExtractCallbackImp::GetStream(', 'void CExtractCallbackImp::Cleanup(', 'STDMETHODIMP CExtractCallbackImp::PrepareOperation('))
 start=source.index('BOOL CExtractCallbackImp::OnDataError(');end=source.index('LRESULT CExtractCallbackImp::Error(',start);methods+='\n'+source[start:end]
 start=source.index('STDMETHODIMP CExtractCallbackImp::SetOperationResult(');end=source.index('STDMETHODIMP CExtractCallbackImp::CryptoGetTextPassword(',start);methods+='\n'+source[start:end]
 ids=sorted(set(re.findall(r'\bIDS_[A-Z0-9_]+\b',methods+' IDS_DELETE_ERROR ')));parts={'COMPLETION':block(read('src/plugins/7zip/extract.h'),'    bool IsSelectionComplete() const'),'STOP_EXTRACTION':next(line for line in read('src/plugins/7zip/7zclient.h').splitlines() if line.startswith('#define E_STOPEXTRACTION ')),'HELPERS':'\n'.join(block(structs,m) for m in ('inline UString GetSalamanderUnicodeString(', 'inline UString GetSalamanderLongPath(')),'GUARD':block(source,'static DWORD SevenZipExtractTargetError('),'DELETE':block(host,'BOOL SafeDeleteFileW(')+'\n'+block(host,'BOOL SafeDeleteFile('),'CALLBACKS':methods,'IDS':'enum {'+','.join(n+'='+str(100+i) for i,n in enumerate(ids))+'};'}
 for k,v in parts.items():data=data.replace('@@'+k+'@@',v)
 assert '@@' not in data
 get=block(source,'STDMETHODIMP CExtractCallbackImp::GetStream(');assert get.index('SevenZipExtractTargetError(')<get.index('_tcscpy(TargetFileName, TargetDir)')<get.index('WM_7ZIP_CREATEFILE')<get.index('OutFileStreamSpec->Open(')
 assert 'GetUnicodeString(GetAnsiString(ProcessedFileInfo.FileName))' not in source
 assert 'SafeDeleteFile(GetAnsiString(ProcessedFileInfo.FileName)' not in source
 assert "*lastComp = '\\0'" not in get
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'));vendor=ROOT/'src/plugins/7zip/7za'
 files=['CPP/Common/MyString.cpp','CPP/Common/MyVector.cpp','CPP/Common/IntToString.cpp','CPP/Common/StringConvert.cpp','CPP/7zip/Common/FileStreams.cpp','CPP/Windows/FileIO.cpp','CPP/Windows/FileName.cpp','CPP/Windows/FileDir.cpp','CPP/Windows/FileFind.cpp','CPP/Windows/PropVariant.cpp','CPP/Windows/TimeUtils.cpp']
 with tempfile.TemporaryDirectory(prefix='sevenzip-output-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(data,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual 7-Zip extraction target:',c,flush=True)
   cmd=[cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/Gy','/D_CRT_SECURE_NO_WARNINGS',*flags,'/I'+str(vendor/'CPP'),str(cpp),*[str(vendor/f) for f in files],'/Fe:'+str(exe),'/link','/OPT:REF','oleaut32.lib','user32.lib']
   for call in (cmd,[str(exe),str(d/'owned-fixtures')]):
    r=run(call,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
