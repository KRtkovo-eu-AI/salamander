# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual controller/Apply/guard/formatter; explicit UI and property-store recording facades."""
import argparse, shutil, tempfile, re, ast, json
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run

def main():
    p=argparse.ArgumentParser();p.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=p.parse_args()
    source=read('src/fileswn5.cpp');strings=read('src/salamdr4.cpp');wide=read('src/common/widepath.cpp')
    pieces={'CLASS':block(read('src/salamand.h'),'class CTruncatedString')+';','STRING':'\n'.join(block(strings,m) for m in ['CTruncatedString::CTruncatedString()', 'CTruncatedString::~CTruncatedString()', 'static char* DupWideAsUtf8(', 'static wchar_t* DupWideString(', 'BOOL CTruncatedString::SetW(', 'const wchar_t*\nCTruncatedString::GetW()']),'CONVERSIONS':'\n'.join(block(wide,m) for m in ['BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(']),'FORMAT':'\n'.join(block(source,m) for m in ['static std::wstring EditPropertiesLocalizedToWide(', 'static std::wstring FormatEditPropertiesLocalizedArguments(']),'CTOR':block(source,'CEditWindowsPropertiesDialog(HWND'),'CHECK':block(source,'BOOL CheckSelection()'),'APPLY':block(source,'BOOL Apply()'),'GUARD':block(source,'struct CBranchAttributeSelection')+';','CONTROLLER':block(source,'void CFilesWindow::EditWindowsProperties()')}
    resources=read('src/lang/texts.rc2')
    names=['IDS_EDPROP_RESULT','IDS_EDPROP_RESULT_DETAIL','IDS_EDPROP_REASON_OTHER','IDS_EDPROP_REASON_READONLY','IDS_EDPROP_REASON_UNSUPPORTED','IDS_EDPROP_TITLE','IDS_FFA_TAGS']
    values=[]
    for name in names:
        match=re.search(r'\b'+name+r',\s*"((?:""|[^"])*)"',resources);assert match,name
        values.append(ast.literal_eval('"'+match.group(1).replace('""',r'\"')+'"'))
    pieces['RESOURCES']='static const std::vector<std::string> ActualResources={'+','.join(json.dumps(v) for v in values)+'};'
    code=read('src/tests/branch_edit_properties_dialog_tests.cpp.in')
    for k,v in pieces.items():code=code.replace('@@'+k+'@@',v)
    assert '@@' not in code
    env=compiler_environment();cl=shutil.which('cl',path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='branch-edit-properties-dialog-') as temp:
        d=Path(temp);cpp=d/'test.cpp';cpp.write_text(code,encoding='utf8');exe=d/'test.exe'
        for c,flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if a.configuration not in ('Both',c):continue
            print('Actual edit properties dialog:',c,flush=True)
            for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe),'user32.lib'],[str(exe)]):
                r=run(cmd,cwd=d,env=env,timeout=40)
                if r.returncode:return r.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
