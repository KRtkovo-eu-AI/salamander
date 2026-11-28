// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Add Unicode and long-path helpers for native updater files. | Keep catalog and installer paths wide end to end. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_utils.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace SamandarinNative
{
namespace
{
bool IsSeparator(wchar_t value)
{
    return value == L'\\' || value == L'/';
}

bool EqualPath(const std::wstring& left, const std::wstring& right)
{
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

std::wstring NormalizeSeparators(std::wstring path)
{
    std::replace(path.begin(), path.end(), L'/', L'\\');
    while (path.size() > 3 && path.back() == L'\\') path.pop_back();
    return path;
}
} // namespace

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty() || text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) return std::wstring();
    const int size = static_cast<int>(text.size());
    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), size, nullptr, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (required == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        required = MultiByteToWideChar(codePage, flags, text.data(), size, nullptr, 0);
    }
    if (required <= 0) return std::wstring();
    std::wstring result(static_cast<size_t>(required), L'\0');
    if (MultiByteToWideChar(codePage, flags, text.data(), size, result.data(), required) != required)
    {
        return std::wstring();
    }
    return result;
}

std::wstring AnsiOrUtf8ToWide(const std::string& text)
{
    return Utf8ToWide(text);
}

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty() || text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) return std::string();
    const int length = static_cast<int>(text.size());
    const int required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length,
                                             nullptr, 0, nullptr, nullptr);
    if (required <= 0) return std::string();
    std::string result(static_cast<size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length,
                            &result[0], required, nullptr, nullptr) != required)
    {
        return std::string();
    }
    return result;
}

std::string WideToAnsi(const std::wstring& text)
{
    if (text.empty() || text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) return std::string();
    const int length = static_cast<int>(text.size());
    const int required = WideCharToMultiByte(CP_ACP, 0, text.data(), length, nullptr, 0, nullptr, nullptr);
    if (required <= 0) return std::string();
    std::string result(static_cast<size_t>(required), '\0');
    if (WideCharToMultiByte(CP_ACP, 0, text.data(), length, &result[0], required, nullptr, nullptr) != required)
    {
        return std::string();
    }
    return result;
}

std::wstring GetModulePath(HINSTANCE module)
{
    size_t capacity = 256;
    for (;;)
    {
        std::vector<wchar_t> path(capacity, L'\0');
        const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0) return std::wstring();
        if (length < path.size()) return std::wstring(path.data(), length);
        if (capacity >= 32768) return std::wstring();
        capacity = (std::min)(capacity * 2, static_cast<size_t>(32768));
    }
}

std::wstring GetModuleDirectory(HINSTANCE module)
{
    std::wstring path = GetModulePath(module);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return std::wstring();
    path.resize(slash + 1);
    return path;
}

std::wstring GetExecutableDirectory()
{
    return GetModuleDirectory(nullptr);
}

std::wstring AddLongPathPrefix(const std::wstring& path)
{
    if (path.empty() || path.rfind(L"\\\\?\\", 0) == 0) return path;
    if (path.size() < MAX_PATH) return path;
    if (path.rfind(L"\\\\", 0) == 0) return L"\\\\?\\UNC\\" + path.substr(2);
    return L"\\\\?\\" + path;
}

std::wstring FullPath(const std::wstring& path)
{
    if (path.empty()) return std::wstring();
    DWORD capacity = 256;
    for (;;)
    {
        std::vector<wchar_t> buffer(capacity, L'\0');
        const DWORD length = GetFullPathNameW(path.c_str(), capacity, buffer.data(), nullptr);
        if (length == 0) return std::wstring();
        if (length < capacity) return std::wstring(buffer.data(), length);
        if (length >= 32768) return std::wstring();
        capacity = length + 1;
    }
}

std::wstring CombinePath(const std::wstring& directory, const std::wstring& child)
{
    if (directory.empty()) return child;
    if (child.empty()) return directory;
    if (IsSeparator(directory.back())) return directory + child;
    return directory + L"\\" + child;
}

std::wstring GetParentDirectory(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return std::wstring();
    if (slash == 2 && path.size() >= 3 && path[1] == L':') return path.substr(0, 3);
    return path.substr(0, slash);
}

std::wstring GetFileName(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring GetFileNameWithoutExtension(const std::wstring& path)
{
    std::wstring name = GetFileName(path);
    const size_t dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos && dot != 0) name.resize(dot);
    return name;
}

std::wstring GetFileExtension(const std::wstring& path)
{
    const std::wstring name = GetFileName(path);
    const size_t dot = name.find_last_of(L'.');
    return dot == std::wstring::npos ? std::wstring() : name.substr(dot);
}

bool IsPathInside(const std::wstring& path, const std::wstring& root)
{
    const std::wstring fullPath = NormalizeSeparators(FullPath(path));
    std::wstring fullRoot = NormalizeSeparators(FullPath(root));
    if (fullPath.empty() || fullRoot.empty()) return false;
    if (!fullRoot.empty() && fullRoot.back() != L'\\') fullRoot.push_back(L'\\');
    if (fullPath.size() < fullRoot.size()) return false;
    return CompareStringOrdinal(fullPath.data(), static_cast<int>(fullRoot.size()),
                                fullRoot.data(), static_cast<int>(fullRoot.size()), TRUE) == CSTR_EQUAL;
}

bool PathExists(const std::wstring& path, bool* isDirectory)
{
    const DWORD attributes = GetFileAttributesW(AddLongPathPrefix(path).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    if (isDirectory != nullptr) *isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return true;
}

bool CreateDirectories(const std::wstring& path)
{
    if (path.empty()) return false;
    const std::wstring full = AddLongPathPrefix(FullPath(path));
    if (full.empty()) return false;
    if (CreateDirectoryW(full.c_str(), nullptr)) return true;
    DWORD error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS)
    {
        const DWORD attributes = GetFileAttributesW(full.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    if (error != ERROR_PATH_NOT_FOUND) return false;

    const std::wstring parent = GetParentDirectory(path);
    if (parent.empty() || EqualPath(parent, path) || !CreateDirectories(parent)) return false;
    if (CreateDirectoryW(full.c_str(), nullptr)) return true;
    error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS)
    {
        const DWORD attributes = GetFileAttributesW(full.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    return false;
}

bool ReadUtf8File(const std::wstring& path, std::string* contents, size_t maximumBytes)
{
    if (contents == nullptr) return false;
    contents->clear();
    HANDLE file = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER fileSize = {};
    bool success = GetFileSizeEx(file, &fileSize) != FALSE && fileSize.QuadPart >= 0 &&
                   static_cast<ULONGLONG>(fileSize.QuadPart) <= maximumBytes;
    if (success)
    {
        contents->reserve(static_cast<size_t>(fileSize.QuadPart));
        char buffer[32768];
        for (;;)
        {
            DWORD read = 0;
            if (!ReadFile(file, buffer, sizeof(buffer), &read, nullptr))
            {
                success = false;
                break;
            }
            if (read == 0) break;
            if (read > maximumBytes || contents->size() > maximumBytes - read)
            {
                success = false;
                break;
            }
            contents->append(buffer, read);
        }
    }
    CloseHandle(file);
    if (!success) contents->clear();
    return success;
}

bool WriteUtf8File(const std::wstring& path, const std::string& contents, bool createNew)
{
    HANDLE file = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_WRITE, 0, nullptr,
                              createNew ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    size_t offset = 0;
    bool success = true;
    while (offset < contents.size())
    {
        const DWORD chunk = static_cast<DWORD>((std::min)(contents.size() - offset,
                                                           static_cast<size_t>(0x7ffff000)));
        DWORD written = 0;
        if (!WriteFile(file, contents.data() + offset, chunk, &written, nullptr) || written != chunk)
        {
            success = false;
            break;
        }
        offset += written;
    }
    if (!FlushFileBuffers(file)) success = false;
    CloseHandle(file);
    if (!success) DeleteFileW(AddLongPathPrefix(path).c_str());
    return success;
}

bool WriteUtf8FileAtomic(const std::wstring& path, const std::string& contents)
{
    const std::wstring temporary = path + L"." + std::to_wstring(GetCurrentProcessId()) + L"." +
                                   std::to_wstring(GetTickCount64()) + L".tmp";
    if (!WriteUtf8File(temporary, contents, true)) return false;
    if (MoveFileExW(AddLongPathPrefix(temporary).c_str(), AddLongPathPrefix(path).c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        return true;
    }
    DeleteFileW(AddLongPathPrefix(temporary).c_str());
    return false;
}
} // namespace SamandarinNative
