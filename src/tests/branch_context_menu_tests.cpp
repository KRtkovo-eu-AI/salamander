// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../branchshell.h"
#include "native_path_test_support.h"
#include <cstdio>
#include <filesystem>
#include <set>

static int Failures = 0;
static void Check(bool value, const char* name)
{
    printf("%s %s\n", value ? "PASS" : "FAIL", name);
    if (!value) ++Failures;
}
static bool MakeFile(const std::wstring& path)
{
    std::wstring full = L"\\\\?\\" + path;
    HANDLE file = CreateFileW(full.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    CloseHandle(file);
    return true;
}
static std::set<std::wstring> Verbs(IContextMenu* context)
{
    std::set<std::wstring> verbs;
    HMENU popup = CreatePopupMenu();
    HRESULT result = context->QueryContextMenu(popup, 0, 1, 0x7fff, CMF_NORMAL);
    if (SUCCEEDED(result))
    {
        for (UINT i = 0; i < (UINT)HRESULT_CODE(result); ++i)
        {
            wchar_t name[256] = {};
            if (SUCCEEDED(context->GetCommandString(i, GCS_VERBW, NULL, (LPSTR)name, _countof(name))) && name[0] != 0)
            {
                CharLowerBuffW(name, (DWORD)wcslen(name));
                // GUID-named third-party handlers can appear only after lazy
                // initialization during the first menu query. Compare named
                // canonical verbs; these identify the actual file association.
                if (name[0] != L'{') verbs.insert(name);
            }
        }
    }
    DestroyMenu(popup);
    return verbs;
}
static IContextMenu* Reference(const std::vector<std::wstring>& paths)
{
    std::vector<PIDLIST_ABSOLUTE> full;
    std::vector<PCUITEMID_CHILD> children;
    IContextMenu* context = NULL;
    IShellFolder* parent = NULL;
    HRESULT result = S_OK;
    for (const auto& path : paths)
    {
        PIDLIST_ABSOLUTE id = NULL;
        result = SHParseDisplayName(path.c_str(), NULL, &id, 0, NULL);
        if (FAILED(result)) break;
        full.push_back(id);
        children.push_back(ILFindLastID(id));
    }
    if (SUCCEEDED(result)) result = SHBindToParent(full[0], IID_PPV_ARGS(&parent), NULL);
    if (SUCCEEDED(result)) result = parent->GetUIObjectOf(NULL, (UINT)children.size(), children.data(), IID_IContextMenu, NULL, (void**)&context);
    if (parent != NULL) parent->Release();
    for (auto id : full) CoTaskMemFree(id);
    return context;
}
static void CheckMenu(const std::vector<std::wstring>& paths, const char* label)
{
    IContextMenu* reference = Reference(paths);
    IContextMenu* actual = NULL;
    HRESULT result = CreateShellObjectForPaths(NULL, paths, IID_IContextMenu, (void**)&actual, TRUE);
    Check(reference != NULL && SUCCEEDED(result) && actual != NULL, label);
    if (reference != NULL && actual != NULL)
    {
        const auto expected = Verbs(reference), found = Verbs(actual);
        Check(!expected.empty(), "real parent menu supplies canonical file verbs");
        if (found != expected)
        {
            printf("Reference verbs:"); for (const auto& verb : expected) printf(" [%ls]", verb.c_str()); printf("\nBranch verbs:");
            for (const auto& verb : found) printf(" [%ls]", verb.c_str()); printf("\n");
        }
        Check(found == expected, "Branch menu canonical verbs equal actual parent menu");
        Check(found.count(L"manage") == 0, "file context never exposes computer management");
    }
    if (reference != NULL) reference->Release();
    if (actual != NULL) actual->Release();
}
int wmain(int argc, wchar_t** argv)
{
    HRESULT initialized = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) return 2;
    const std::filesystem::path parent = NativePathTestParent(argc, argv);
    if (parent.empty()) { CoUninitialize(); return 2; }
    const std::filesystem::path root = parent / (L"samandarin-branch-context-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    std::error_code error;
    std::filesystem::create_directory(root, error);
    const auto unicode = root / L"\u017dlu\u0165ou\u010dk\u00fd-\u6771\u4eac-\U0001f600";
    std::filesystem::create_directory(unicode, error);
    const std::wstring ascii = (root / L"same.txt").wstring();
    const std::wstring name = (root / L"\u010d\u0159.txt").wstring();
    const std::wstring duplicate = (unicode / L"same.txt").wstring();
    Check(MakeFile(ascii) && MakeFile(name) && MakeFile(duplicate), "create independent disposable text fixtures");
    CheckMenu({ascii}, "single ASCII file menu");
    CheckMenu({name}, "Unicode filename menu");
    CheckMenu({duplicate}, "Unicode parent menu");
    CheckMenu({ascii, name}, "same-parent multiple file menu");
    const std::wstring byteLong = (root / (std::wstring(100, L'\u6f22') + L".txt")).wstring();
    Check(byteLong.size() < 260 && MakeFile(byteLong), "create UTF8-over-260 fixture");
    CheckMenu({byteLong}, "UTF8-over-260 file menu");
    std::filesystem::path longDirectory = unicode;
    for (int i = 0; i < 5; ++i) longDirectory /= std::wstring(60, L'\u6f22');
    std::filesystem::create_directories(std::filesystem::path(L"\\\\?\\" + longDirectory.wstring()), error);
    const std::wstring longFile = (longDirectory / L"same.txt").wstring();
    Check(longFile.size() > 260 && MakeFile(longFile), "create full Win32 long-path fixture");
    CheckMenu({longFile}, "Win32 long-path file menu");
    IContextMenu* mixed = NULL;
    const HRESULT mixedResult = CreateShellObjectForPaths(NULL, {ascii, duplicate}, IID_IContextMenu, (void**)&mixed, TRUE);
    Check(mixedResult == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) && mixed == NULL,
          "mixed-parent selection cannot silently produce Desktop or first-subset menu");
    if (mixed != NULL) mixed->Release();
    // Only this newly created direct child of the supplied test parent is removed.
    if (root.parent_path() == parent && root.filename().wstring().find(L"samandarin-branch-context-") == 0)
        std::filesystem::remove_all(std::filesystem::path(L"\\\\?\\" + root.wstring()), error);
    CoUninitialize();
    return Failures == 0 ? 0 : 1;
}
