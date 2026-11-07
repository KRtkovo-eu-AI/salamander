// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Test actual common/handles.cpp and its live registry. Fixture ownership and
// cleanup are adapted from PR #769 donor 47661e92 safe_file_paths_tests.cpp.in.
// The donor has no direct regression test for the core A/W wrapper additions.
#include "handles_wide_test_support/precomp.h"
#include <algorithm>
#include <string>
#include <vector>
#include "handles.h"
#include "widepath.h"

HINSTANCE HInstance = NULL;
void Initialize__Messages();
#ifdef TRACE_ENABLE
void Initialize__Trace();
#endif
#ifdef HANDLES_ENABLE
void Initialize__Handles();
struct InspectableHandles : C__Handles
{
    int Count() const { return Handles.Count; }
    bool Contains(HANDLE handle, C__HandlesType type, C__HandlesOrigin origin)
    {
        for (int i = 0; i < Handles.Count; ++i)
            if (Handles[i].Handle.Handle == handle && Handles[i].Handle.Type == type && Handles[i].Handle.Origin == origin)
                return true;
        return false;
    }
};
#define TRACKED(tracker, operation) tracker.SetInfo(__FILE__, __LINE__, __otQuiet).operation
#else
struct InspectableHandles { int Count() const { return 0; } };
#define TRACKED(tracker, operation) ::operation
#endif
static int Checks = 0, Failures = 0, Skipped = 0;
static void Check(bool success, const char* description)
{
    ++Checks;
    if (!success) { ++Failures; std::printf("FAIL: %s (Win32 %lu)\n", description, GetLastError()); }
}
static std::string Utf8(const std::wstring& value)
{
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) std::abort();
    std::string result(count, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1, &result[0], count, nullptr, nullptr);
    result.resize(count - 1);
    return result;
}
static std::wstring Extended(const std::wstring& value) { return SalPathAddExtendedPrefixW(value.c_str()); }
struct Fixtures
{
    std::wstring Root, OldDirectory;
    std::vector<std::wstring> Files, Directories;
    Fixtures()
    {
        CWidePathBuffer buffer;
        DWORD count = GetCurrentDirectoryW(static_cast<DWORD>(buffer.Capacity()), buffer.Data());
        if (!count || count >= static_cast<size_t>(buffer.Capacity())) std::abort();
        OldDirectory.assign(buffer.Data(), count);
        count = GetTempPathW(static_cast<DWORD>(buffer.Capacity()), buffer.Data());
        if (!count || count >= static_cast<size_t>(buffer.Capacity())) std::abort();
        Root.assign(buffer.Data(), count);
        Root += L"Samandarin-size-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        if (!CreateDirectoryW(Root.c_str(), nullptr)) std::abort();
        Directories.push_back(Root);
    }
    bool Owned(const std::wstring& path) const
    {
        return path == Root || (path.size() > Root.size() &&
               path.compare(0, Root.size(), Root) == 0 && path[Root.size()] == L'\\');
    }
    ~Fixtures()
    {
        Check(SetCurrentDirectoryW(OldDirectory.c_str()) != FALSE, "restore working directory");
        for (auto i = Files.rbegin(); i != Files.rend(); ++i)
            Check(Owned(*i) && DeleteFileW(Extended(*i).c_str()) != FALSE, "remove own fixture file");
        for (auto i = Directories.rbegin(); i != Directories.rend(); ++i)
            Check(Owned(*i) && RemoveDirectoryW(Extended(*i).c_str()) != FALSE, "remove own fixture directory");
    }
    std::wstring Directory(const std::wstring& parent, const std::wstring& name)
    {
        const std::wstring path = parent + L"\\" + name;
        if (!Owned(path) || !CreateDirectoryW(Extended(path).c_str(), nullptr)) std::abort();
        Directories.push_back(path);
        return path;
    }
    std::wstring File(const std::wstring& parent, const std::wstring& name, DWORD size)
    {
        const std::wstring path = parent + L"\\" + name;
        if (!Owned(path)) std::abort();
        HANDLE handle = CreateFileW(Extended(path).c_str(), GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) std::abort();
        Files.push_back(path);
        std::string payload(size, 'x');
        DWORD written = 0;
        const BOOL ok = WriteFile(handle, payload.data(), size, &written, nullptr);
        CloseHandle(handle);
        if (!ok || written != size) std::abort();
        return path;
    }
};


static std::wstring BitmapFile(Fixtures& fixtures, const std::wstring& directory, const std::wstring& name)
{
    const auto path = directory + L"\\" + name;
    if (!fixtures.Owned(path)) std::abort();
    BITMAPFILEHEADER header = {}; BITMAPINFOHEADER info = {};
    header.bfType = 0x4d42; header.bfOffBits = sizeof(header) + sizeof(info); header.bfSize = header.bfOffBits + 4;
    info.biSize = sizeof(info); info.biWidth = 1; info.biHeight = 1; info.biPlanes = 1;
    info.biBitCount = 24; info.biCompression = BI_RGB; info.biSizeImage = 4;
    std::vector<BYTE> bytes(header.bfSize, 0);
    memcpy(bytes.data(), &header, sizeof(header)); memcpy(bytes.data() + sizeof(header), &info, sizeof(info));
    bytes[header.bfOffBits] = 0x22; bytes[header.bfOffBits + 1] = 0x66; bytes[header.bfOffBits + 2] = 0xaa;
    HANDLE file = CreateFileW(Extended(path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) std::abort();
    fixtures.Files.push_back(path);
    DWORD written = 0; const BOOL ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    CloseHandle(file);
    if (!ok || written != bytes.size()) std::abort();
    return path;
}
static void CheckBitmap(HANDLE handle)
{
    BITMAP bitmap = {};
    Check(GetObject(handle, sizeof(bitmap), &bitmap) == sizeof(bitmap) && bitmap.bmWidth == 1 && bitmap.bmHeight == 1,
        "loaded exact bitmap dimensions");
    const auto pixels = static_cast<const BYTE*>(bitmap.bmBits);
    Check(pixels != nullptr && pixels[0] == 0x22 && pixels[1] == 0x66 && pixels[2] == 0xaa,
        "loaded exact fixture pixel bytes");
}
static void LoadBitmap(Fixtures& fixtures, const std::wstring& directory, const std::wstring& name, bool ansi)
{
    const auto path = BitmapFile(fixtures, directory, name);
    const auto api = Extended(path);
    InspectableHandles tracker;
    const UINT flags = LR_LOADFROMFILE | LR_CREATEDIBSECTION;
    Check(GetFileAttributesW(api.c_str()) != INVALID_FILE_ATTRIBUTES, "exact bitmap fixture exists before image load");
    HANDLE raw = ::LoadImageW(nullptr, api.c_str(), IMAGE_BITMAP, 0, 0, flags);
    const DWORD rawError = GetLastError();
    const bool backendLongLimit = !raw && path.size() >= MAX_PATH;
    if (backendLongLimit) { ++Skipped; std::printf("SKIP: native LoadImageW backend rejected a long bitmap path (Win32 %lu)\n", rawError); }
    else Check(raw != nullptr, "native wide bitmap backend supports this fixture");
    if (raw) ::DeleteObject(raw);
    HANDLE image = ansi ? TRACKED(tracker, LoadImageA(nullptr, Utf8(path).c_str(), IMAGE_BITMAP, 0, 0, flags))
                        : TRACKED(tracker, LoadImageW(nullptr, api.c_str(), IMAGE_BITMAP, 0, 0, flags));
    Check((image != nullptr) == (raw != nullptr), "wrapper preserves native wide backend success/failure");
#ifdef HANDLES_ENABLE
    Check(tracker.Count() == (image ? 1 : 0), "bitmap acquisition registered exactly once");
    if (image) Check(tracker.Contains(image, __htBitmap, __hoLoadImage), "bitmap registry type and origin");
#endif
    if (image) { CheckBitmap(image); Check(TRACKED(tracker, DeleteObject(image)) != FALSE, "tracked bitmap cleanup"); }
    Check(tracker.Count() == 0, "bitmap registry empty after cleanup or failure");
}
static void WatchDirectory(Fixtures& fixtures, const std::wstring& directory, bool ansi, int identity)
{
    InspectableHandles tracker;
    const auto api = Extended(directory);
    HANDLE watch = ansi ? TRACKED(tracker, FindFirstChangeNotificationA(Utf8(directory).c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME))
                        : TRACKED(tracker, FindFirstChangeNotificationW(api.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME));
    Check(watch != INVALID_HANDLE_VALUE, "open exact directory change notification");
#ifdef HANDLES_ENABLE
    Check(tracker.Count() == (watch != INVALID_HANDLE_VALUE ? 1 : 0), "directory notification registered once");
    if (watch != INVALID_HANDLE_VALUE) Check(tracker.Contains(watch, __htChangeNotification, __hoFindFirstChangeNotification), "notification registry type and origin");
#endif
    if (watch != INVALID_HANDLE_VALUE)
    {
        fixtures.File(directory, L"signal-" + std::to_wstring(identity) + L".txt", 1);
        Check(WaitForSingleObject(watch, 5000) == WAIT_OBJECT_0, "notification observes the correct owned directory");
        Check(TRACKED(tracker, FindCloseChangeNotification(watch)) != FALSE, "tracked notification cleanup");
    }
    Check(tracker.Count() == 0, "notification registry empty after cleanup");
}
static void ImageKinds()
{
    InspectableHandles tracker;
    HICON icon = (HICON)TRACKED(tracker, LoadImageW(HInstance, MAKEINTRESOURCEW(100), IMAGE_ICON, 32, 32, 0));
    Check(icon != nullptr, "load owned fixture icon with W wrapper");
#ifdef HANDLES_ENABLE
    if (icon) Check(tracker.Contains(icon, __htIcon, __hoLoadImage), "icon registry type and origin");
#endif
    if (icon) Check(TRACKED(tracker, DestroyIcon(icon)) != FALSE, "tracked icon cleanup");
    HCURSOR cursor = (HCURSOR)TRACKED(tracker, LoadImageA(HInstance, MAKEINTRESOURCEA(101), IMAGE_CURSOR, 32, 32, 0));
    Check(cursor != nullptr, "load owned fixture cursor with A wrapper");
#ifdef HANDLES_ENABLE
    if (cursor) Check(tracker.Contains(cursor, __htCursor, __hoLoadImage), "cursor registry type and origin");
#endif
    if (cursor) Check(TRACKED(tracker, DestroyCursor(cursor)) != FALSE, "tracked cursor cleanup");
    Check(tracker.Count() == 0, "all image-kind acquisitions balanced");
}
static void FailureAndLegacy(Fixtures& fixtures)
{
    InspectableHandles tracker;
    const auto missing = fixtures.Root + L"\\missing";
    Check(TRACKED(tracker, LoadImageW(nullptr, missing.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)) == nullptr,
        "wide image failure returns null");
    Check(TRACKED(tracker, FindFirstChangeNotificationW(missing.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME)) == INVALID_HANDLE_VALUE,
        "wide notification failure returns invalid handle");
    Check(tracker.Count() == 0, "failed APIs add no registry entries");
    HANDLE watch = HANDLES_Q(FindFirstChangeNotification(Utf8(fixtures.Root).c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME));
    Check(watch != INVALID_HANDLE_VALUE, "legacy generic notification macro remains supported");
    if (watch != INVALID_HANDLE_VALUE) Check(HANDLES_Q(FindCloseChangeNotification(watch)) != FALSE, "legacy notification macro cleanup");
    HICON icon = (HICON)HANDLES_Q(LoadImage(HInstance, MAKEINTRESOURCE(100), IMAGE_ICON, 32, 32, 0));
    Check(icon != nullptr, "legacy generic image macro remains supported");
    if (icon) Check(HANDLES_Q(DestroyIcon(icon)) != FALSE, "legacy image macro cleanup");
}
int main()
{
    HInstance = GetModuleHandleW(nullptr);
#ifdef TRACE_ENABLE
    Initialize__Trace();
#endif
    Initialize__Messages();
#ifdef HANDLES_ENABLE
    Initialize__Handles();
#endif
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    {
        Fixtures fixtures;
        const bool asciiRoot = Utf8(fixtures.Root).size() == fixtures.Root.size() && fixtures.Root.size() + 32 < MAX_PATH;
        Check(asciiRoot, "owned ASCII root fits legacy A API boundary"); if (!asciiRoot) return 1;
        const auto sentinel = fixtures.File(fixtures.Root, L"sentinel.txt", 3);
        LoadBitmap(fixtures, fixtures.Root, L"ascii.bmp", true); WatchDirectory(fixtures, fixtures.Root, true, 1);
        auto unicode = fixtures.Directory(fixtures.Root, L"žluťoučký");
        LoadBitmap(fixtures, unicode, L"ascii.bmp", false); WatchDirectory(fixtures, unicode, false, 2);
        unicode = fixtures.Directory(unicode, L"東京-\U0001F642");
        LoadBitmap(fixtures, unicode, L"český-東京.bmp", false); WatchDirectory(fixtures, unicode, false, 3);
        LoadBitmap(fixtures, fixtures.Root, L"český-東京.bmp", false);
        const auto utf8Long = fixtures.Directory(fixtures.Root, std::wstring(90, L'東'));
        Check(Utf8(utf8Long).size() > MAX_PATH && utf8Long.size() < MAX_PATH, "UTF8-byte-long fixture stays below UTF16 MAX_PATH");
        LoadBitmap(fixtures, utf8Long, L"long-bytes.bmp", false); WatchDirectory(fixtures, utf8Long, false, 4);
        auto deep = fixtures.Root;
        for (int i = 0; i < 18; ++i) deep = fixtures.Directory(deep, L"component-東京-" + std::to_wstring(i));
        Check(deep.size() > MAX_PATH, "real Win32 UTF16-long fixture");
        LoadBitmap(fixtures, deep, L"long-utf16.bmp", false); WatchDirectory(fixtures, deep, false, 5);
        ImageKinds(); FailureAndLegacy(fixtures);
        HANDLE untouched = CreateFileW(sentinel.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        char bytes[3] = {}; DWORD done = 0;
        Check(untouched != INVALID_HANDLE_VALUE && ReadFile(untouched, bytes, 3, &done, nullptr) && done == 3 && memcmp(bytes, "xxx", 3) == 0,
            "unrelated owned sentinel remains intact");
        if (untouched != INVALID_HANDLE_VALUE) CloseHandle(untouched);
    }
    std::printf("Core wide handles: %d checks, %d failures, %d backend subcases skipped\n", Checks, Failures, Skipped);
    return Failures ? 1 : 0;
}
