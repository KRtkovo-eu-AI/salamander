// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Include after spl_com.h and spl_gen.h. Requires a Samandarin host whose general
// interface contains QueryService (older Open Salamander ABI is not supported).
// The fallback below supports older Samandarin hosts without DiskSelection;
// it never guesses root + name after an advertised resolver fails.
#ifdef new
#pragma push_macro("new")
#undef new
#define SAL_DISK_SELECTION_RESTORE_INCLUDE_NEW
#endif
#include <string>
#include <vector>
#include <memory>
#include <new>
#include <climits>
#include <cstring>
#include <cwchar>
#include <utility>
#ifdef SAL_DISK_SELECTION_RESTORE_INCLUDE_NEW
#pragma pop_macro("new")
#undef SAL_DISK_SELECTION_RESTORE_INCLUDE_NEW
#endif

namespace SalamanderDiskSelection
{
// Conversion helpers allocate and may throw bad_alloc; Capture handles it.
inline std::wstring WideFromPath(const char* text)
{
    if (text == NULL || *text == 0) return std::wstring();
    const UINT cp = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0) > 0 ? CP_UTF8 : CP_ACP;
    const int length = MultiByteToWideChar(cp, 0, text, -1, NULL, 0);
    if (length <= 0) return std::wstring();
    std::wstring result(length, L'\0');
    if (MultiByteToWideChar(cp, 0, text, -1, &result[0], length) != length) return std::wstring();
    result.resize(length - 1);
    return result;
}
inline std::string Utf8FromWide(const wchar_t* text)
{
    if (text == NULL || *text == 0) return std::string();
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, NULL, 0, NULL, NULL);
    if (length <= 0) return std::string();
    std::string result(length, '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, &result[0], length, NULL, NULL) != length) return std::string();
    result.resize(length - 1);
    return result;
}
namespace Detail
{
inline BOOL Fail(DWORD error) { SetLastError(error); return FALSE; }
inline bool UpDirectory(const CFileData& item, BOOL isDir)
{
    return isDir && (item.UseWideName() ? wcscmp(item.NameW, L"..") == 0 :
        item.Name != NULL && strcmp(item.Name, "..") == 0);
}
inline std::wstring ComparablePath(const std::wstring& path)
{
    if (path.compare(0, 8, L"\\\\?\\UNC\\") == 0) return L"\\\\" + path.substr(8);
    if (path.compare(0, 4, L"\\\\?\\") == 0) return path.substr(4);
    return path;
}
inline std::wstring RelativePath(const std::wstring& root, const std::wstring& full)
{
    std::wstring base = ComparablePath(root), path = ComparablePath(full);
    while (!base.empty() && (base.back() == L'\\' || base.back() == L'/')) base.pop_back();
    if (base.empty() || base.size() >= path.size() || base.size() > INT_MAX ||
        (path[base.size()] != L'\\' && path[base.size()] != L'/') ||
        CompareStringOrdinal(base.c_str(), static_cast<int>(base.size()), path.c_str(), static_cast<int>(base.size()), TRUE) != CSTR_EQUAL)
        return std::wstring();
    return path.substr(base.size() + 1);
}
inline std::wstring ParentPath(const std::wstring& full)
{
    const size_t slash = full.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return std::wstring();
    return full.substr(0, slash > 0 && full[slash - 1] == L':' ? slash + 1 : slash);
}
struct OwnedItem
{
    CSalamanderDiskSelectionItem Item = {};
    std::wstring Name, Full, Directory, Relative, Extension;
    void Bind()
    {
        Item.NameW = Name.c_str(); Item.FullPathW = Full.c_str();
        Item.DirectoryW = Directory.c_str(); Item.RelativePathW = Relative.c_str();
        Item.ExtensionW = Extension.c_str();
    }
};
// This implementation is compiled in the allocating module. Release therefore
// frees memory in that same module, including for SDK-created legacy snapshots.
class OwnedSnapshot : public CSalamanderDiskSelectionSnapshotAbstract
{
public:
    std::wstring Root;
    std::vector<OwnedItem> Items;
    void Seal() { for (size_t i = 0; i < Items.size(); ++i) Items[i].Bind(); }
    void Append(const CFileData& source, BOOL isDir, BOOL focused, int panelIndex,
                const std::wstring& full, const std::wstring& directory, const std::wstring& relative)
    {
        OwnedItem entry;
        entry.Full = full; entry.Directory = directory; entry.Relative = relative;
        const size_t slash = full.find_last_of(L"\\/");
        entry.Name = full.substr(slash == std::wstring::npos ? 0 : slash + 1);
        if (!isDir || (source.Ext != NULL && source.Ext[0] != 0))
        {
            const size_t dot = entry.Name.find_last_of(L'.');
            if (dot != std::wstring::npos) entry.Extension = entry.Name.substr(dot + 1);
        }
        entry.Item.IsDir = isDir; entry.Item.Selected = source.Selected;
        entry.Item.Focused = focused; entry.Item.PanelIndex = panelIndex;
        entry.Item.Attr = source.Attr; entry.Item.Size = source.Size;
        entry.Item.LastWrite = source.LastWrite; entry.Item.SizeValid = source.SizeValid;
        entry.Item.Hidden = source.Hidden; entry.Item.IsLink = source.IsLink; entry.Item.IsOffline = source.IsOffline;
        Items.push_back(std::move(entry));
    }
    virtual int WINAPI GetCount() const { return static_cast<int>(Items.size()); }
    virtual const CSalamanderDiskSelectionItem* WINAPI GetItem(int index) const
    {
        return index >= 0 && static_cast<size_t>(index) < Items.size() ? &Items[index].Item : NULL;
    }
    virtual const wchar_t* WINAPI GetRootPathW() const { return Root.c_str(); }
    virtual void WINAPI Release() { delete this; }
};
inline OwnedSnapshot* Allocate()
{
#ifdef new
#undef new
#define SAL_DISK_SELECTION_RESTORE_NEW
#endif
    OwnedSnapshot* result = new (std::nothrow) OwnedSnapshot;
#ifdef SAL_DISK_SELECTION_RESTORE_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SAL_DISK_SELECTION_RESTORE_NEW
#endif
    return result;
}
inline bool Include(DWORD mode, bool anySelected, const CFileData& item, bool focused)
{
    return mode == SALDISKSELECTION_ALL_ITEMS ||
        (mode == SALDISKSELECTION_FOCUSED_ONLY ? focused :
         mode == SALDISKSELECTION_SELECTED_ONLY || anySelected ? item.Selected != 0 : focused);
}
inline BOOL CaptureLegacy(CSalamanderGeneralAbstract* general, int panel, DWORD mode,
                          CSalamanderDiskSelectionSnapshotAbstract** result)
{
    // The old SDK exposes only the UI thread's HWND. Check before touching any
    // borrowed panel row; do not turn wrong-thread failures into an empty result.
    const HWND mainWindow = general->GetMainWindowHWND();
    if (mainWindow == NULL || GetWindowThreadProcessId(mainWindow, NULL) != GetCurrentThreadId())
        return Fail(ERROR_INVALID_THREAD_ID);
    std::vector<char> rootText(4 * SAL_MAX_PATH, '\0');
    int type = 0;
    if (!general->GetPanelPath(panel, rootText.data(), static_cast<int>(rootText.size()), &type, NULL))
        return Fail(ERROR_INVALID_PARAMETER);
    if (type != PATH_TYPE_WINDOWS) return Fail(ERROR_NOT_SUPPORTED);
    std::unique_ptr<OwnedSnapshot> snapshot(Allocate());
    if (!snapshot) return Fail(ERROR_NOT_ENOUGH_MEMORY);
    snapshot->Root = WideFromPath(rootText.data());
    if (snapshot->Root.empty()) return Fail(ERROR_INVALID_DATA);
    CSalamanderServiceQuery query = {SALAMANDER_SERVICE_PANEL_ITEM_PATHS, SALAMANDER_PANEL_ITEM_PATHS_VERSION_1_0, 0};
    CSalamanderServiceResult service = {};
    CSalamanderPanelItemPathsAbstract* resolver = NULL;
    if (general->QueryService(&query, &service))
    {
        if (service.Interface == NULL || service.Version < query.MinimumVersion) return Fail(ERROR_NOT_SUPPORTED);
        resolver = static_cast<CSalamanderPanelItemPathsAbstract*>(service.Interface);
    }
    int selectedFiles = 0, selectedDirs = 0;
    general->GetPanelSelection(panel, &selectedFiles, &selectedDirs);
    if (selectedFiles < 0 || selectedDirs < 0 || selectedFiles > INT_MAX - selectedDirs)
        return Fail(ERROR_INVALID_DATA);
    const int expectedSelected = selectedFiles + selectedDirs;
    const CFileData* focused = general->GetPanelFocusedItem(panel, NULL);
    int cursor = 0, ordinal = 0, selectedSeen = 0;
    std::vector<wchar_t> path(512, L'\0');
    while (true)
    {
        BOOL isDir = FALSE;
        const CFileData* item = general->GetPanelItem(panel, &cursor, &isDir);
        if (item == NULL) break;
        const int index = ordinal++;
        if (UpDirectory(*item, isDir)) continue;
        if (item->Selected) ++selectedSeen;
        if (!Include(mode, expectedSelected != 0, *item, item == focused)) continue;
        std::wstring full;
        if (resolver != NULL)
        {
            for (;;)
            {
                if (resolver->GetItemFullPath(panel, item, path.data(), static_cast<int>(path.size())))
                {
                    full = path.data(); break;
                }
                const DWORD error = GetLastError();
                if (error != ERROR_INSUFFICIENT_BUFFER || path.size() >= SAL_MAX_PATH)
                    return Fail(error != ERROR_SUCCESS ? error : ERROR_INVALID_DATA);
                path.resize(path.size() * 2);
            }
        }
        else
        {
            const std::wstring name = item->UseWideName() ? std::wstring(item->NameW) : WideFromPath(item->Name);
            if (name.empty()) return Fail(ERROR_INVALID_DATA);
            full = snapshot->Root;
            if (full.back() != L'\\' && full.back() != L'/') full += L'\\';
            full += name;
        }
        const std::wstring relative = RelativePath(snapshot->Root, full), directory = ParentPath(full);
        if (full.empty() || relative.empty() || directory.empty() || full.back() == L'\\' || full.back() == L'/')
            return Fail(ERROR_INVALID_DATA);
        snapshot->Append(*item, isDir, item == focused, index, full, directory, relative);
    }
    if (selectedSeen != expectedSelected) return Fail(ERROR_INVALID_DATA);
    snapshot->Seal();
    *result = snapshot.release();
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}
} // namespace Detail
} // namespace SalamanderDiskSelection

class CSalamanderDiskSelection
{
    CSalamanderDiskSelectionSnapshotAbstract* Snapshot;
public:
    CSalamanderDiskSelection() : Snapshot(NULL) {}
    ~CSalamanderDiskSelection() { Reset(); }
    CSalamanderDiskSelection(const CSalamanderDiskSelection&) = delete;
    CSalamanderDiskSelection& operator=(const CSalamanderDiskSelection&) = delete;
    CSalamanderDiskSelection(CSalamanderDiskSelection&& other) noexcept : Snapshot(other.Snapshot) { other.Snapshot = NULL; }
    CSalamanderDiskSelection& operator=(CSalamanderDiskSelection&& other) noexcept
    {
        if (this != &other) { Reset(); Snapshot = other.Snapshot; other.Snapshot = NULL; }
        return *this;
    }
    void Reset() { if (Snapshot != NULL) Snapshot->Release(); Snapshot = NULL; }
    BOOL Capture(CSalamanderGeneralAbstract* general, int panel, DWORD mode = SALDISKSELECTION_SELECTED_OR_FOCUSED)
    {
        Reset();
        if (general == NULL || mode > SALDISKSELECTION_ALL_ITEMS ||
            (panel != PANEL_SOURCE && panel != PANEL_TARGET && panel != PANEL_LEFT && panel != PANEL_RIGHT))
            return SalamanderDiskSelection::Detail::Fail(ERROR_INVALID_PARAMETER);
        try
        {
            CSalamanderServiceQuery query = {SALAMANDER_SERVICE_DISK_SELECTION, SALAMANDER_DISK_SELECTION_VERSION_1_0, 0};
            CSalamanderServiceResult result = {};
            if (general->QueryService(&query, &result))
            {
                if (result.Interface == NULL || result.Version < query.MinimumVersion)
                    return SalamanderDiskSelection::Detail::Fail(ERROR_NOT_SUPPORTED);
                CSalamanderDiskSelectionSnapshotAbstract* captured = NULL;
                if (!static_cast<CSalamanderDiskSelectionAbstract*>(result.Interface)->Capture(panel, mode, &captured))
                {
                    const DWORD error = GetLastError();
                    if (captured != NULL) captured->Release();
                    return SalamanderDiskSelection::Detail::Fail(error != ERROR_SUCCESS ? error : ERROR_INVALID_DATA);
                }
                if (captured == NULL) return SalamanderDiskSelection::Detail::Fail(ERROR_INVALID_DATA);
                Snapshot = captured;
                SetLastError(ERROR_SUCCESS);
                return TRUE;
            }
            return SalamanderDiskSelection::Detail::CaptureLegacy(general, panel, mode, &Snapshot);
        }
        catch (const std::bad_alloc&)
        {
            Reset();
            return SalamanderDiskSelection::Detail::Fail(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    int GetCount() const { return Snapshot != NULL ? Snapshot->GetCount() : 0; }
    const CSalamanderDiskSelectionItem* GetItem(int index) const { return Snapshot != NULL ? Snapshot->GetItem(index) : NULL; }
    const wchar_t* GetRootPathW() const { return Snapshot != NULL ? Snapshot->GetRootPathW() : L""; }
};
