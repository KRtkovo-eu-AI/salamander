# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual ZIP whole/mask/extract-loop methods with explicit no-I/O engine facades."""
import argparse,re,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 source=read('src/plugins/zip/extract.cpp');text=read('src/tests/zip_unpack_completion_tests.cpp.in')
 methods={key:block(source,sig) for key,sig in [('WHOLE','int CZipUnpack::UnpackWholeArchive('),('MATCH','int CZipUnpack::MatchFilesToMask('),('EXTRACT','int CZipUnpack::ExtractFiles(')]}
 single=block(source,'int CZipUnpack::ExtractSingleFile(');creation=[]
 for hit in re.finditer(r'if \(!SalamanderGeneral->CheckAndCreateDirectory\(targetDir,',single):
  start=single.rfind('do',0,hit.start());end=single.index('while (retry);',hit.end())+len('while (retry);');creation.append(single[start:end])
 assert len(creation)==2
 methods.update({'CREATE_DIRECTORY':creation[0],'CREATE_PARENT':creation[1]})
 ids=sorted(set(re.findall(r'\bIDS_[A-Z0-9_]+\b',text+'\n'.join(methods.values()))));text=text.replace('@@IDS@@','enum {'+','.join(k+'='+str(100+i) for i,k in enumerate(ids))+'};')
 for key,value in methods.items():text=text.replace('@@'+key+'@@',value)
 assert '@@' not in text
 assert 'WholeArchiveExtracted = false;' in block(source,'CZipUnpack::CZipUnpack(')
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='zip-unpack-completion-') as tmp:
  d=Path(tmp);cpp=d/'test.cpp';cpp.write_text(text,encoding='utf8');exe=d/'test.exe'
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual ZIP whole-extraction completion:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe)]):
    r=run(cmd,cwd=d,env=env,timeout=40)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
