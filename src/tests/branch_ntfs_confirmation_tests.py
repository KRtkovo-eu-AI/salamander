# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual NTFS confirmation builder and typed string storage; explicit panel/resource facades."""
import argparse, shutil, tempfile, re, ast, json
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
    source=read('src/salamdr4.cpp');wide=read('src/common/widepath.cpp')
    pieces={'CONFIRM':block(read('src/fileswn5.cpp'),'static BOOL PrepareNtfsConfirmation('),'CLASS':block(read('src/salamand.h'),'class CTruncatedString')+';','CONVERSIONS':'\n'.join(block(wide,m) for m in ['BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(']),'STRING':'\n'.join(block(source,m) for m in ['CTruncatedString::CTruncatedString()', 'CTruncatedString::~CTruncatedString()', 'static char* DupWideAsUtf8(', 'static wchar_t* DupWideString(', 'BOOL CTruncatedString::SetW(', 'const wchar_t*\nCTruncatedString::GetW()'])}
    resources=read('src/lang/texts.rc2')
    def resource(name):
        match=re.search(r'\b'+name+r',\s*"((?:""|[^"])*)"',resources)
        assert match,name
        return ast.literal_eval('"'+match.group(1).replace('""',r'\"')+'"')
    pieces['RESOURCES']='static const std::vector<std::string> ActualPrefixes={'+','.join(json.dumps(resource(n)) for n in ['IDS_CONFIRM_NTFSCOMPRESS','IDS_CONFIRM_NTFSUNCOMPRESS','IDS_CONFIRM_NTFSENCRYPT','IDS_CONFIRM_NTFSDECRYPT'])+'};\nstatic const std::string ActualFileQuestion='+json.dumps(resource('IDS_QUESTION_FILE'))+';'
    code=read('src/tests/branch_ntfs_confirmation_tests.cpp.in')
    for k,v in pieces.items():code=code.replace('@@'+k+'@@',v)
    assert '@@' not in code
    env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-ntfs-confirmation-') as temp:
        d=Path(temp);source=d/'test.cpp';source.write_text(code,encoding='utf8');exe=d/'test.exe'
        for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if a.configuration not in ('Both',c):continue
            print('Actual NTFS confirmation:',c,flush=True)
            for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(source),'/Fe:'+str(exe)],[str(exe)]):
                r=run(cmd,cwd=d,env=env,timeout=30)
                if r.returncode:return r.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
