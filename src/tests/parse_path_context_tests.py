# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual ParsePath wrapper/shared parser; real directory attributes, explicit UI/plugin/absolute-name facades."""
import argparse,shutil,tempfile
from pathlib import Path
from branch_panel_getters_tests import block,read,compiler_environment,run
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
 wide=read('src/common/widepath.cpp');source=read('src/salamdr5.cpp');text=read('src/tests/parse_path_context_tests.cpp.in')
 parts=[block(read('src/common/widepath.h'),'class CPathBuffer')+';']+[block(wide,m) for m in ('CPathBuffer::CPathBuffer()', 'CPathBuffer::CPathBuffer(int', 'char* CPathBuffer::Data()', 'int CPathBuffer::Capacity()', 'BOOL SalIsExtendedLengthPathW(', 'std::wstring SalPathAddExtendedPrefixW(', 'BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')]
 text=text.replace('@@WIDE@@','\n'.join(parts))
 parts=[block(read('src/salamdr1.cpp'),'int GetRootPath('),block(read('src/salamdr3.cpp'),'BOOL SalPathAddBackslash(')]
 parts += [block(read('src/salamdr4.cpp'),m) for m in ('BOOL IsPluginFSPath(const char*','BOOL IsPluginFSPath(char*')]
 parts += [block(read('src/fileswn5.cpp'),'BOOL CutSpacesFromBothSides(')]
 text=text.replace('@@UTILS@@','\n'.join(parts))
 # Preserve the complete production function-try-block including bad_alloc handler.
 show=source[source.index('static void ShowParsePathError('):source.index('// GetRootPath and drive reconnect')]
 text=text.replace('@@PARSER@@',show+block(source,'static BOOL ReadParsePathRoot(')+'\n'+block(source,'BOOL SalParsePath('))
 text=text.replace('@@WRAPPER@@',block(read('src/fileswn9.cpp'),'BOOL CFilesWindow::ParsePath('))
 assert '@@' not in text
 env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='parse-path-context-') as temp:
  d=Path(temp);cpp=d/'test.cpp';exe=d/'test.exe';cpp.write_text(text,encoding='utf8')
  for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
   if a.configuration not in ('Both',c):continue
   print('Actual path parser context:',c,flush=True)
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/J','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe),'user32.lib'],[str(exe),str(d/'owned-fixtures')]):
    result=run(cmd,cwd=d,env=env,timeout=40)
    if result.returncode:return result.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
