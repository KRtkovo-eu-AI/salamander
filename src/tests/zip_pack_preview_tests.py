# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the actual ZIP Pack preview initializer; UI messages are explicit doubles."""
import argparse
from contextlib import contextmanager
import re
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import read, block, compiler_environment, run


@contextmanager
def owned_temporary():
    directory = Path(tempfile.mkdtemp(prefix='zip-pack-preview-')).resolve()
    assert directory.parent == Path(tempfile.gettempdir()).resolve()
    assert directory.name.startswith('zip-pack-preview-')
    try:
        yield directory
    finally:
        # Python's ordinary cleanup cannot traverse these long fixture paths
        # with LongPathsEnabled=0; use the exact verified owned extended root.
        shutil.rmtree('\\\\?\\' + str(directory))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source = read('src/plugins/zip/dialogs.cpp')
    start = source.index('void CPackDialog::ResetControls()')
    initializer = source[start:source.index('\nINT_PTR PackDialog(', start)]
    handlers = '\n'.join(block(source, 'BOOL CPackDialog::' + name + '(') for name in ('OnMultiVol', 'OnSeqName', 'OnSelfExtr'))
    constants = sorted(set(re.findall(r'\b(?:IDC_|IDS_|PD_|PA_)[A-Z_0-9]+\b', initializer + handlers + ' IDS_NODISPLAY')) - {'IDCANCEL'})
    definitions = '\n'.join('#define %s %d' % (name, 1 << i) for i, name in enumerate(constants))
    text = r'''
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <new>
#include <string>
#include <vector>
#define SAL_MAX_PATH 32768
#define CALL_STACK_MESSAGE1(...)
#define CALL_STACK_MESSAGE2(...)
#define IDS_ERRACCESDIR 700002
#define ERR_RETRY 700003
@@CONSTANTS@@
@@PATH_HELPERS@@
struct ModeSettings { int Flags = 0; };
struct ModeOptions { int Action = 0; ModeSettings SfxSettings; };
struct ModeConfig { bool ShowExOptions = true, LastUsedAuto = false; char VolSizeCache[5][16]{}; int VolSizeUnits[5]{}; };
static ModeConfig Config;
static struct { ModeSettings Settings; char Name[64]{}; } LastUsedSfxSet;
static int DefaultsCalls = 0, DialogCalls = 0;
#define MAX_VOL_STR 16
#define SE_AUTO 1
#define SE_NOTALLOWCHANGE 2
struct CZipPack {
    bool SkipAllIOErrors = false;
    int Errors = 0;
    int ProcessError(int, DWORD, const char*, int, bool*) { ++Errors; return 700004; }
    int GetDirInfo(const char*, DWORD*, FILETIME*);
    const char* ZipName = nullptr;
    ModeConfig Config;
    ModeOptions Options;
    int ErrorID = 0;
    bool LoadDefaults() { ++DefaultsCalls; return true; }
    int LoadExPackOptions(unsigned);

};
@@DIRECTORY_INFO@@
static std::string Preview;
static int Ended = 0, LegacyPreviewCalls = 0, Messages = 0;
static bool Multi = false, Self = false;
@@ADVANCED_LIMIT@@
struct General { HWND GetMainWindowHWND() { return nullptr; } void SalMessageBox(HWND, const char*, const char*, UINT) { ++Messages; } } general;
static General* SalamanderGeneral = &general;
static BOOL FixtureEnable(HWND, BOOL) { return TRUE; }
static HWND FixtureItem(HWND, int) { return nullptr; }
static BOOL FixtureInvalidate(HWND, const RECT*, BOOL) { return TRUE; }
static BOOL FixtureUpdate(HWND) { return TRUE; }
static LRESULT PreviewMessage(HWND, int id, UINT message, WPARAM check, LPARAM text) {
    if (message == BM_GETCHECK) return (id == IDC_MULTIVOL ? Multi : id == IDC_SELFEXTR ? Self : false) ? BST_CHECKED : BST_UNCHECKED;
    if (message == BM_SETCHECK) { if(id == IDC_MULTIVOL) Multi = check == BST_CHECKED; if(id == IDC_SELFEXTR) Self = check == BST_CHECKED; }
    if (id == IDC_ARCHIVE && message == WM_SETTEXT) Preview = reinterpret_cast<const char*>(text);
    return 0;
}
static BOOL PreviewTitle(HWND, const char*) { return TRUE; }
static BOOL PreviewEnd(HWND, INT_PTR result) { Ended = static_cast<int>(result); return TRUE; }
static const char* LoadStr(int) { return "owned fixture"; }
// Multi-volume filename generation is outside this ordinary/SFX preview fixture.
static int MakeFileName(int, bool, const char* source, char* target, BOOL) {
    ++LegacyPreviewCalls; if (!ZipPackAdvancedPathFits(source)) throw std::bad_alloc();
    strcpy_s(target, SAL_MAX_PATH, source); return static_cast<int>(strlen(target));
}
@@SPLIT@@
struct CPackDialog {
    struct Options { int Action = 0; bool SeqNames = false, Encrypt = false; } options;
    struct ConfigData { BOOL WinZipNames = TRUE; } config;
    struct PackData { int ErrorID = 0; } pack;
    Options* PackOptions = &options;
    ConfigData* Config = &config;
    PackData* PackObject = &pack;
    const char* ZipFile = nullptr;
    unsigned Flags = PD_NEWARCHIVE;
    HWND Dlg = nullptr;
    void ResetControls();
    BOOL OnMultiVol(WORD, WORD, HWND);
    BOOL OnSeqName(WORD, WORD, HWND);
    BOOL OnSelfExtr(WORD, WORD, HWND);
};
#undef SendDlgItemMessage
#define SendDlgItemMessage PreviewMessage
#undef SetWindowText
#define SetWindowText PreviewTitle
#define EndDialog PreviewEnd
#define EnableWindow FixtureEnable
#define GetDlgItem FixtureItem
#define InvalidateRect FixtureInvalidate
#define UpdateWindow FixtureUpdate
#define CALL_STACK_MESSAGE3(...)
@@HANDLERS@@
@@INITIALIZER@@
static int PackDialog(HWND, CZipPack*, ModeConfig*, ModeOptions*, const char*, unsigned) { ++DialogCalls; return IDOK; }
@@LOAD_OPTIONS@@
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::vector<std::string> names = {"C:\\ascii\\file.zip", u8"C:\\ascii\\東京.zip",
        u8"C:\\東京\\file.zip", u8"C:\\žluťoučký\\東京\\café.zip",
        "C:\\" + std::string(100, 'a') + "\\" + std::string(100, 'b') + u8"\\東京東京東京東京東京東京東京東京東京東京東京東京東京東京東京東京.zip",
        "C:\\" + std::string(200, 'a') + "\\" + std::string(200, 'b') + u8"\\東京.zip",
        std::string(SAL_MAX_PATH - 5, 'x') + ".zip"};
    int checks = 0;
    for (const auto& name : names) {
        for (int action : {0, PA_SELFEXTRACT, PA_MULTIVOL, PA_SELFEXTRACT | PA_MULTIVOL}) {
            CPackDialog dialog;
            dialog.ZipFile = name.c_str();
            dialog.options.Action = action;
            Preview.clear(); Ended = 0; LegacyPreviewCalls = 0;
            if (action && !ZipPackAdvancedPathFits(name.c_str())) {
                dialog.ResetControls();
                if (Ended != IDCANCEL || dialog.pack.ErrorID != IDS_TOOLONGZIPNAME || !Preview.empty() || LegacyPreviewCalls != 0) return 6;
                ++checks; continue;
            }
            const bool sfx = (action & PA_SELFEXTRACT) != 0;
            dialog.ResetControls();
            const std::string expected = sfx ? name.substr(0, name.rfind('.')) + ".exe" : name;
            if (Preview != expected || Ended != 0 || dialog.pack.ErrorID != 0) {
                std::printf("FAIL exact preview length=%zu sfx=%d\n", name.size(), sfx); return 1;
            }
            ++checks;
        }
    }
    for (size_t length : {size_t(MAX_PATH-1), size_t(MAX_PATH), size_t(MAX_PATH+1), size_t(SAL_MAX_PATH-1)}) {
        const std::string path(length, 'x');
        if (ZipPackAdvancedPathFits(path.c_str()) != (length < MAX_PATH)) return 7;
        ++checks;
        if(length < MAX_PATH) continue;
        for (int which : {0,1,2}) {
            CPackDialog dialog; dialog.ZipFile=path.c_str();
            Multi=Self=true; Messages=LegacyPreviewCalls=0; Preview.clear(); Ended=0;
            if(which==0) dialog.OnMultiVol(BN_CLICKED,IDC_MULTIVOL,nullptr);
            if(which==1) dialog.OnSeqName(BN_CLICKED,IDC_SEQNAME,nullptr);
            if(which==2) dialog.OnSelfExtr(BN_CLICKED,IDC_SELFEXTR,nullptr);
            if(Multi || Self || Messages!=1 || LegacyPreviewCalls || !Preview.empty() || Ended) return 8;
            ++checks;
        }
    }
    for (bool showDialog : {false,true}) for (int action : {0,PA_SELFEXTRACT,PA_MULTIVOL,PA_SELFEXTRACT|PA_MULTIVOL}) for (size_t length : {size_t(100),size_t(MAX_PATH),size_t(SAL_MAX_PATH-1)}) {
        std::string name(length,'x'); CZipPack pack; pack.ZipName=name.c_str();
        pack.Config.ShowExOptions=showDialog; pack.Options.Action=action;
        DefaultsCalls=DialogCalls=0; const int result=pack.LoadExPackOptions(0);
        const bool rejected=action && length>=MAX_PATH;
        if(result != (rejected ? IDS_TOOLONGZIPNAME : 0) || DefaultsCalls!=1 || DialogCalls != (!rejected && showDialog ? 1 : 0)) return 9;
        ++checks;
    }
    const auto root = std::filesystem::absolute(argv[1]);
    for (size_t i = 0; i < names.size() - 1; ++i) {
        // The drive-qualified preview names are reused as relative fixture components.
        const std::string path = root.u8string() + "\\" + names[i].substr(3);
        const auto wide = ZipPathToWide(path.c_str());
        std::filesystem::create_directories(wide);
        CZipPack pack;
        DWORD attributes = 0; FILETIME time{};
        WIN32_FILE_ATTRIBUTE_DATA expected{};
        if (!GetFileAttributesExW(wide.c_str(), GetFileExInfoStandard, &expected)) return 3;
        if (pack.GetDirInfo(path.c_str(), &attributes, &time) != 0 || pack.Errors != 0 ||
            attributes != expected.dwFileAttributes ||
            CompareFileTime(&time, &expected.ftLastWriteTime) != 0) return 4;
        ++checks;
        attributes = 123; time = {456,789};
        if (pack.GetDirInfo((path + "\\missing").c_str(), &attributes, &time) != 700004 ||
            pack.Errors != 1 || attributes != 123 || time.dwLowDateTime != 456 || time.dwHighDateTime != 789) return 5;
        ++checks;
    }
    std::printf("Actual ZIP preview and directory metadata: %d checks PASS\n", checks);
    return 0;
}
'''
    split = block(read('src/plugins/zip/common2.cpp'), 'void SplitPath2(')
    advanced_limit = block(read('src/plugins/zip/common.h'), 'inline bool ZipPackAdvancedPathFits(')
    for marker in ('PackMultiVol', 'PackSelfExtract'):
        body = block(read('src/plugins/zip/add.cpp'), 'int CZipPack::' + marker + '(')
        assert body.index('ZipPackAdvancedPathFits(ZipName)') < body.index('OpenProgressDialog' if marker == 'PackMultiVol' else 'SalPathRenameExtension')
    assert block(source, 'BOOL CPackDialog::OnOK(').index('ZipPackAdvancedPathFits(ZipFile)') < block(source, 'BOOL CPackDialog::OnOK(').index('PackOptions->Action = PA_NORMAL')
    load_options = block(read('src/plugins/zip/add_del.cpp'), 'int CZipPack::LoadExPackOptions(')
    text = text.replace('@@LOAD_OPTIONS@@', load_options).replace('@@ADVANCED_LIMIT@@', advanced_limit).replace('@@HANDLERS@@', handlers).replace('@@CONSTANTS@@', definitions).replace('@@SPLIT@@', split).replace('@@INITIALIZER@@', initializer)
    common = read('src/plugins/zip/common.cpp')
    helpers = '\n'.join(block(common, marker) for marker in (
        'static std::wstring ZipPathAddExtendedPrefix(', 'static bool ZipBytesToWide(', 'std::wstring ZipPathToWide('))
    add = read('src/plugins/zip/add.cpp')
    begin = add.index('int CZipPack::GetDirInfo(')
    info = add[begin:add.index('\nint CZipPack::IsDirectoryEmpty(', begin)]
    text = text.replace('@@PATH_HELPERS@@', helpers).replace('@@DIRECTORY_INFO@@', info)
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with owned_temporary() as temporary:
        directory = Path(temporary)
        cpp, exe = directory / 'test.cpp', directory / 'test.exe'
        cpp.write_text(text, encoding='utf8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od', '/RTC1']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print(config, flush=True)
            for cmd in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags, str(cpp), '/Fe:' + str(exe)], [str(exe), str(directory / config)]):
                result = run(cmd, cwd=directory, env=env, timeout=40)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
