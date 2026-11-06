// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Internal owned paths. Input identities come from the SDK snapshot, never from
// a displayed panel root plus a basename. No public interface layout changes.
namespace SplitCBNPaths
{
    const int Capacity = 4 * SAL_MAX_PATH;
    typedef std::vector<char> Buffer;

    inline std::string Utf8(const wchar_t* value)
    {
        return SalamanderDiskSelection::Utf8FromWide(value);
    }
    inline std::wstring Wide(const char* value)
    {
        return SalamanderDiskSelection::WideFromPath(value);
    }
    inline std::string Join(const std::string& parent, const std::string& name)
    {
        return parent + (parent.empty() || parent.back() == '\\' ? "" : "\\") + name;
    }
    inline std::string Name(const std::string& path)
    {
        const size_t slash = path.find_last_of("\\/");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }
    inline std::string Parent(const std::string& path)
    {
        const size_t slash = path.find_last_of("\\/");
        if (slash == std::string::npos)
            return std::string();
        return path.substr(0, slash == 2 && path[1] == ':' ? slash + 1 : slash);
    }
    inline std::string Stem(const std::string& name)
    {
        const size_t dot = name.find_last_of('.');
        return dot == std::string::npos ? name : name.substr(0, dot);
    }
    inline std::wstring ApiPath(const char* value)
    {
        std::wstring path = Wide(value);
        if (path.compare(0, 4, L"\\\\?\\") == 0 || path.compare(0, 4, L"\\\\.\\") == 0)
            return path;
        if (path.compare(0, 2, L"\\\\") == 0)
            return L"\\\\?\\UNC\\" + path.substr(2);
        if (path.size() >= 3 && path[1] == L':' && path[2] == L'\\')
            return L"\\\\?\\" + path;
        return path;
    }
    inline std::wstring Root(const std::wstring& path)
    {
        if (path.size() >= 3 && path[1] == L':' && path[2] == L'\\')
            return path.substr(0, 3);
        if (path.compare(0, 2, L"\\\\") == 0)
        {
            size_t server = path.find(L'\\', 2);
            if (server != std::wstring::npos)
            {
                size_t share = path.find(L'\\', server + 1);
                return share == std::wstring::npos ? path + L"\\" : path.substr(0, share + 1);
            }
        }
        return std::wstring();
    }
    inline bool Absolute(const std::string& value, const char* base, std::string& result)
    {
        std::wstring path = Wide(value.c_str());
        const std::wstring parent = Wide(base);
        for (wchar_t& ch : path)
            if (ch == L'/') ch = L'\\';
        if (path.empty()) path = parent;
        if (path.compare(0, 8, L"\\\\?\\UNC\\") == 0)
            path = L"\\\\" + path.substr(8);
        else if (path.compare(0, 4, L"\\\\?\\") == 0)
            path.erase(0, 4);
        bool absolute = path.compare(0, 2, L"\\\\") == 0 ||
                        (path.size() >= 3 && path[1] == L':' && path[2] == L'\\');
        if (!absolute)
        {
            if (path[0] == L'\\')
                path = Root(parent) + path.substr(1);
            else if (path.size() >= 2 && path[1] == L':')
            {
                if (parent.size() >= 2 && towupper(path[0]) == towupper(parent[0]))
                    path = parent + L"\\" + path.substr(2);
            }
            else
                path = parent + L"\\" + path;
        }
        // Normalize explicit dot components; otherwise preserve literal trailing
        // dots/spaces so the later extended-path API targets the displayed item.
        bool dots = false;
        for (size_t begin = 0; begin < path.size();)
        {
            size_t end = path.find(L'\\', begin);
            if (end == std::wstring::npos) end = path.size();
            const size_t count = end - begin;
            dots = dots || (count == 1 && path[begin] == L'.') ||
                   (count == 2 && path[begin] == L'.' && path[begin + 1] == L'.');
            begin = end + 1;
        }
        if (dots || Root(path).empty())
        {
            DWORD required = GetFullPathNameW(path.c_str(), 0, NULL, NULL);
            if (required == 0) return false;
            std::vector<wchar_t> buffer(required);
            const DWORD written = GetFullPathNameW(path.c_str(), required, buffer.data(), NULL);
            if (written == 0 || written >= required) return false;
            path = buffer.data();
        }
        if (path.size() + 8 >= SAL_MAX_PATH || Root(path).empty())
        {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return false;
        }
        for (size_t i = 0; i < path.size(); ++i)
            if (path[i] < L' ' || wcschr(L"*?<>|\"", path[i]) != NULL || (path[i] == L':' && i != 1))
            {
                SetLastError(ERROR_INVALID_NAME);
                return false;
            }
        result = Utf8(path.c_str());
        return true;
    }

    struct FileIdentity
    {
        DWORD Volume, IndexHigh, IndexLow;
    };
    inline bool GetIdentity(HANDLE file, FileIdentity& identity)
    {
        BY_HANDLE_FILE_INFORMATION info;
        if (!GetFileInformationByHandle(file, &info))
            return false;
        // Some providers cannot supply a stable file ID. Do not infer identity
        // from a displayed name or silently allow an unchecked overwrite.
        if (info.nFileIndexHigh == 0 && info.nFileIndexLow == 0)
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            return false;
        }
        identity.Volume = info.dwVolumeSerialNumber;
        identity.IndexHigh = info.nFileIndexHigh;
        identity.IndexLow = info.nFileIndexLow;
        return true;
    }
    inline bool CanCreateCombineTarget(const char* target, const std::vector<FileIdentity>& sources)
    {
        HANDLE file = CreateFileW(ApiPath(target).c_str(), FILE_READ_ATTRIBUTES,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            const DWORD error = GetLastError();
            return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
        }
        FileIdentity targetId;
        const bool identified = GetIdentity(file, targetId);
        const DWORD error = GetLastError();
        CloseHandle(file);
        if (!identified)
        {
            SetLastError(error);
            return false;
        }
        for (size_t i = 0; i < sources.size(); ++i)
            if (sources[i].Volume == targetId.Volume && sources[i].IndexHigh == targetId.IndexHigh &&
                sources[i].IndexLow == targetId.IndexLow)
            {
                SetLastError(ERROR_SHARING_VIOLATION);
                return false;
            }
        return true;
    }

    inline bool Equal(const std::wstring& a, const std::wstring& b)
    {
        return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL;
    }
    inline bool Copy(Buffer& target, const std::string& path)
    {
        if (path.size() >= target.size())
        {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return false;
        }
        memcpy(target.data(), path.c_str(), path.size() + 1);
        return true;
    }
    inline std::string WindowText(HWND window)
    {
        const int length = GetWindowTextLengthW(window);
        std::vector<wchar_t> value(static_cast<size_t>(length) + 1);
        GetWindowTextW(window, value.data(), static_cast<int>(value.size()));
        return Utf8(value.data());
    }
    inline void SetWindowPath(HWND window, const char* path)
    {
        const std::wstring wide = Wide(path);
        SetWindowTextW(window, wide.c_str());
    }
    inline void DiskSpace(CQuadWord* result, const char* path)
    {
        ULARGE_INTEGER freeBytes;
        if (GetDiskFreeSpaceExW(ApiPath(path).c_str(), &freeBytes, NULL, NULL))
            result->Value = freeBytes.QuadPart;
        else
            *result = CQuadWord(-1, -1);
    }
    inline std::string PartName(const std::string& prefix, int index, bool padded)
    {
        char number[32];
        sprintf_s(number, padded ? "%03d" : "%d", index);
        return prefix + number;
    }

    inline std::string EscapeBatchName(const std::string& name, bool unquoted)
    {
        std::string result;
        for (size_t i = 0; i < name.size(); ++i)
        {
            const char ch = name[i];
            if (ch == '%')
                result += '%'; // literal percent, including inside double quotes
            else if (unquoted && (ch == '&' || ch == '^' || ch == '|' || ch == '<' || ch == '>'))
                result += '^';
            result += ch;
        }
        return result;
    }
    inline std::string DecodeBatchMetadata(const std::string& name)
    {
        std::string result;
        for (size_t i = 0; i < name.size(); ++i)
        {
            if (i + 1 < name.size() && ((name[i] == '%' && name[i + 1] == '%') ||
                (name[i] == '^' && strchr("&^|<>", name[i + 1]) != NULL)))
                ++i;
            result += name[i];
        }
        return result;
    }
    inline std::string BatchCommands(const std::string& base, const std::string& original, int parts)
    {
        std::string result;
        const std::string destination = EscapeBatchName(original, false);
        int part = 1;
        while (part <= parts)
        {
            std::string line = "copy /b \"";
            if (part > 1)
                line += destination + "\"+\"";
            line += EscapeBatchName(PartName(base + ".", part++, true), false) + "\"";
            while (part <= parts && line.size() + destination.size() + base.size() + 12 <= 127)
                line += "+\"" + EscapeBatchName(PartName(base + ".", part++, true), false) + "\"";
            result += line + " \"" + destination + "\"\r\n";
            if (result.size() >= 100000)
                break; // Existing BAT limit: do not materialize an unbounded rejected script.
        }
        return result;
    }

    struct CombinePlan
    {
        std::vector<std::string> Files;
        std::string SourceDirectory;
        std::string CompanionPrefix; // includes the final period
        bool TestCompanion;
        CombinePlan() : TestCompanion(false) {}
    };

    inline const CSalamanderDiskSelectionItem* FindSibling(
        const CSalamanderDiskSelection& all, const wchar_t* directory, const std::string& name)
    {
        const std::wstring wanted = Wide(name.c_str());
        const CSalamanderDiskSelectionItem* match = NULL;
        bool ambiguous = false;
        for (int i = 0; i < all.GetCount(); ++i)
        {
            const CSalamanderDiskSelectionItem* item = all.GetItem(i);
            // Exact parent identity keeps case-sensitive Windows directories distinct.
            if (item->IsDir || wcscmp(item->DirectoryW, directory) != 0)
                continue;
            if (wanted == item->NameW)
                return item;
            if (Equal(wanted, item->NameW))
            {
                ambiguous = match != NULL;
                match = item;
            }
        }
        return ambiguous ? NULL : match;
    }

    inline bool BuildCombinePlan(const CSalamanderDiskSelection& selection,
                                 const CSalamanderDiskSelection* siblings, CombinePlan& result)
    {
        CombinePlan plan;
        bool selected = false;
        for (int i = 0; i < selection.GetCount(); ++i)
            selected = selected || selection.GetItem(i)->Selected != FALSE;
        if (selected)
        {
            std::string base;
            std::wstring parent;
            bool same = true;
            for (int i = 0; i < selection.GetCount(); ++i)
            {
                const CSalamanderDiskSelectionItem& item = *selection.GetItem(i);
                if (item.IsDir)
                    continue;
                const std::string name = Stem(Utf8(item.NameW));
                if (plan.Files.empty())
                {
                    base = name;
                    parent = item.DirectoryW;
                    plan.SourceDirectory = Utf8(item.DirectoryW);
                }
                else if (parent != item.DirectoryW || !Equal(Wide(base.c_str()), Wide(name.c_str())))
                    same = false;
                plan.Files.push_back(Utf8(item.FullPathW));
            }
            if (plan.Files.empty())
                return false;
            for (int i = 0; i < selection.GetCount(); ++i)
                if (!selection.GetItem(i)->IsDir && selection.GetItem(i)->Focused)
                    plan.SourceDirectory = Utf8(selection.GetItem(i)->DirectoryW);
            plan.TestCompanion = same;
            plan.CompanionPrefix = Join(plan.SourceDirectory, same ? base : "combinedfile") + ".";
        }
        else
        {
            if (selection.GetCount() != 1 || selection.GetItem(0)->IsDir)
                return false;
            const CSalamanderDiskSelectionItem& focus = *selection.GetItem(0);
            const std::string name = Utf8(focus.NameW);
            plan.SourceDirectory = Utf8(focus.DirectoryW);
            const size_t dot = name.find_last_of('.');
            bool expand = false, addFocus = true, padded = false;
            int next = 1;
            if (dot != std::string::npos)
            {
                const std::string ext = name.substr(dot + 1);
                if (_stricmp(ext.c_str(), "tns") == 0)
                {
                    expand = true;
                    next = 2;
                }
                else if (_stricmp(ext.c_str(), "bat") == 0 || _stricmp(ext.c_str(), "crc") == 0)
                {
                    expand = padded = true;
                    addFocus = false;
                }
                else if (!ext.empty() && ext.size() <= 3 && ext.find_first_not_of("0123456789") == std::string::npos)
                {
                    expand = true;
                    padded = ext[0] == '0';
                    next = atoi(ext.c_str()) + 1;
                }
            }
            const std::string prefix = dot == std::string::npos ? "combinedfile." : name.substr(0, dot + 1);
            plan.CompanionPrefix = Join(plan.SourceDirectory, prefix);
            if (expand && siblings != NULL)
            {
                for (int previous = next - 2; previous >= 0; --previous)
                {
                    const CSalamanderDiskSelectionItem* item = FindSibling(*siblings, focus.DirectoryW, PartName(prefix, previous, padded));
                    if (item == NULL)
                        break;
                    plan.Files.insert(plan.Files.begin(), Utf8(item->FullPathW));
                }
                if (addFocus)
                    plan.Files.push_back(Utf8(focus.FullPathW));
                for (;; ++next)
                {
                    const CSalamanderDiskSelectionItem* item = FindSibling(*siblings, focus.DirectoryW, PartName(prefix, next, padded));
                    if (item == NULL)
                        break;
                    plan.Files.push_back(Utf8(item->FullPathW));
                }
                plan.TestCompanion = true;
            }
            else
                plan.Files.push_back(Utf8(focus.FullPathW));
        }
        result = std::move(plan);
        return true;
    }
}
