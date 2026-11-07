// SPDX-License-Identifier: GPL-2.0-or-later
#include "../branch_view.h"
#include <shlobj.h>
#include <propsys.h>
#include <propvarutil.h>
#include <propkey.h>
#include <cstdio>
#include <map>
#include <thread>

using namespace Salamander::BranchView;
static int Failures = 0;
static void Check(bool ok, const char* label)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    if (!ok) ++Failures;
}
static const unsigned char Png0[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,17,0,0,0,23,8,6,0,0,0,237,52,164,231,0,0,0,30,73,68,65,84,120,156,99,16,9,104,250,79,41,102,24,53,100,212,144,81,67,70,13,25,53,100,4,24,2,0,94,119,228,226,43,34,44,76,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char Png1[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,41,0,0,0,29,8,6,0,0,0,55,139,69,77,0,0,0,54,73,68,65,84,120,156,237,206,33,1,0,32,16,0,177,15,65,48,50,81,28,98,112,98,98,126,179,246,185,117,243,59,32,41,41,41,25,39,41,89,35,41,89,35,41,89,35,41,89,35,41,89,243,0,38,172,205,18,74,36,67,46,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char Png2[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,7,0,0,0,11,8,6,0,0,0,179,144,151,168,0,0,0,20,73,68,65,84,120,156,99,16,9,104,250,143,11,51,140,74,18,144,4,0,120,169,145,226,141,127,243,184,0,0,0,0,73,69,78,68,174,66,96,130};
struct Properties
{
    ULONG Width = 0, Height = 0;
    ULONGLONG Modified = 0;
    std::wstring Dimensions;
    bool operator==(const Properties& other) const
    { return Width == other.Width && Height == other.Height && Modified == other.Modified && Dimensions == other.Dimensions; }
};
static bool Read(const std::wstring& path, Properties& result)
{
    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path.c_str(), NULL, GPS_DEFAULT, IID_PPV_ARGS(&store));
    if (FAILED(hr)) return false;
    PROPVARIANT value;
    PropVariantInit(&value);
    hr = store->GetValue(PKEY_Image_HorizontalSize, &value);
    if (SUCCEEDED(hr)) hr = PropVariantToUInt32(value, &result.Width);
    PropVariantClear(&value);
    if (SUCCEEDED(hr)) hr = store->GetValue(PKEY_Image_VerticalSize, &value);
    if (SUCCEEDED(hr)) hr = PropVariantToUInt32(value, &result.Height);
    PropVariantClear(&value);
    if (SUCCEEDED(hr)) hr = store->GetValue(PKEY_Image_Dimensions, &value);
    PWSTR display = NULL;
    if (SUCCEEDED(hr)) hr = PSFormatForDisplayAlloc(PKEY_Image_Dimensions, value, PDFF_DEFAULT, &display);
    if (SUCCEEDED(hr) && display != NULL) result.Dimensions = display;
    CoTaskMemFree(display);
    PropVariantClear(&value);
    if (SUCCEEDED(hr)) hr = store->GetValue(PKEY_DateModified, &value);
    if (SUCCEEDED(hr) && value.vt == VT_FILETIME)
        result.Modified = ((ULONGLONG)value.filetime.dwHighDateTime << 32) | value.filetime.dwLowDateTime;
    else hr = E_FAIL;
    PropVariantClear(&value);
    store->Release();
    return SUCCEEDED(hr) && !result.Dimensions.empty();
}
int wmain()
{
    HRESULT initialized = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) return 2;
    std::vector<wchar_t> temp(32768);
    if (!GetTempPathW((DWORD)temp.size(), temp.data())) return 2;
    const std::wstring root = Join(temp.data(), L"branch-properties-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    std::vector<std::wstring> directories;
    auto dir = [&](const std::wstring& path) {
        if (!CreateDirectoryW(ExtendedPath(path).c_str(), NULL)) return false;
        directories.push_back(path); return true;
    };
    Check(dir(root), "create owned property fixture root");
    const std::wstring first = Join(root, L"ascii"), second = Join(root, L"\u010desk\u00fd-\u65e5\u672c-\U0001f600");
    Check(dir(first) && dir(second), "create distinct ASCII and Unicode parents");
    std::wstring longParent = root;
    for (int i = 0; i < 4; ++i) { longParent = Join(longParent, std::wstring(75, L'\u6f22')); Check(dir(longParent), "create long Unicode ancestor"); }
    const std::vector<std::wstring> paths = {Join(first, L"same.png"), Join(second, L"same.png"), Join(longParent, L"same.png")};
    const unsigned char* images[] = {Png0, Png1, Png2};
    const DWORD sizes[] = {sizeof(Png0), sizeof(Png1), sizeof(Png2)};
    const ULONG widths[] = {17,41,7}, heights[] = {23,29,11};
    std::map<std::wstring, Properties> ordinary;
    for (size_t i = 0; i < paths.size(); ++i)
    {
        HANDLE file = CreateFileW(ExtendedPath(paths[i]).c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        bool written = false;
        if (file != INVALID_HANDLE_VALUE)
        {
            DWORD done = 0;
            const ULONGLONG ticks = 132000000000000000ULL + i * 864000000000ULL;
            FILETIME date = {(DWORD)ticks, (DWORD)(ticks >> 32)};
            written = WriteFile(file, images[i], sizes[i], &done, NULL) && done == sizes[i] && SetFileTime(file, NULL, NULL, &date);
            CloseHandle(file);
        }
        Check(written, "write actual PNG with independent dimensions and date");
        Properties p;
        Check(Read(paths[i], p) && p.Width == widths[i] && p.Height == heights[i] && p.Modified != 0, "ordinary full path returns actual PNG properties");
        ordinary[paths[i]] = p;
    }
    Scan scanner; Progress progress;
    Check(scanner.Start(root, false), "start actual Branch View recursive scanner");
    std::vector<Entry> entries;
    ULONGLONG deadline = GetTickCount64() + 10000;
    do
    {
        std::vector<Entry> batch; scanner.Drain(batch, progress);
        entries.insert(entries.end(), batch.begin(), batch.end());
        if (!progress.Running) break;
        Sleep(1);
    } while (GetTickCount64() < deadline);
    Check(!progress.Running && progress.Errors == 0 && entries.size() == 3, "Branch snapshot retains all three duplicate basenames");
    std::map<std::wstring, Properties> branch;
    bool workerOk = true;
    std::thread worker([&] {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) { workerOk = false; return; }
        for (const Entry& entry : entries)
        {
            Properties value;
            if (!Read(entry.FullPath(), value)) workerOk = false;
            branch[entry.FullPath()] = value;
        }
        CoUninitialize();
    });
    worker.join();
    Check(workerOk && branch == ordinary, "async Branch properties equal ordinary values for exact identities");
    Check(branch[paths[0]].Dimensions != branch[paths[1]].Dimensions && branch[paths[0]].Modified != branch[paths[1]].Modified,
          "duplicate basename rows retain different formatted Dimensions and dates");
    Check(paths[2].size() > MAX_PATH && branch[paths[2]].Width == 7, "long Unicode Branch row retains actual image width");
    for (const auto& path : paths) DeleteFileW(ExtendedPath(path).c_str());
    for (auto i = directories.rbegin(); i != directories.rend(); ++i) RemoveDirectoryW(ExtendedPath(*i).c_str());
    CoUninitialize();
    return Failures ? 1 : 0;
}
