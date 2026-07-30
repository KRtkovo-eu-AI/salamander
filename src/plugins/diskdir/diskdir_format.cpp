// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "diskdir.h"
#include "diskdir_format.h"

#include <algorithm>
#include <charconv>
#include <climits>
#include <sstream>

namespace
{
const size_t MAX_CATALOG_SIZE = 128 * 1024 * 1024;

static std::string TrimCarriageReturn(std::string value)
{
    if (!value.empty() && value.back() == '\r')
        value.pop_back();
    return value;
}

static bool StartsWithNoCase(const std::string& value, const std::string& prefix)
{
    if (value.size() < prefix.size())
        return false;
    return _strnicmp(value.c_str(), prefix.c_str(), prefix.size()) == 0;
}

static std::string WideToUtf8(const wchar_t* text, size_t length)
{
    if (length == 0)
        return std::string();
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(length),
                                         NULL, 0, NULL, NULL);
    if (utf8Length <= 0)
        return std::string();
    std::string utf8(static_cast<size_t>(utf8Length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(length), utf8.data(),
                        utf8Length, NULL, NULL);
    return utf8;
}

static UINT LegacyAnsiCodePage()
{
    wchar_t value[16] = {};
    if (GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE,
                       value, _countof(value)) > 1)
    {
        unsigned long codePage = wcstoul(value, NULL, 10);
        if (codePage > 0 && codePage <= UINT_MAX)
            return static_cast<UINT>(codePage);
    }
    return 1252;
}

static std::string AnsiToUtf8(const char* text, size_t length)
{
    if (length == 0)
        return std::string();
    UINT codePage = LegacyAnsiCodePage();
    int wideLength = MultiByteToWideChar(codePage, 0, text, static_cast<int>(length),
                                         NULL, 0);
    if (wideLength <= 0)
        return std::string();
    std::wstring wide(static_cast<size_t>(wideLength), L'\0');
    if (MultiByteToWideChar(codePage, 0, text, static_cast<int>(length), wide.data(),
                            wideLength) != wideLength)
        return std::string();
    return WideToUtf8(wide.data(), wide.size());
}

static bool IsValidUtf8(const char* text, size_t length)
{
    return length == 0 ||
           MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text,
                               static_cast<int>(length), NULL, 0) > 0;
}

static std::wstring AddExtendedPrefix(const std::wstring& path)
{
    if (path.empty() || path.rfind(L"\\\\?\\", 0) == 0)
        return path;
    if (path.rfind(L"\\\\", 0) == 0)
        return std::wstring(L"\\\\?\\UNC\\") + path.substr(2);
    if (path.size() >= 3 && path[1] == L':' &&
        (path[2] == L'\\' || path[2] == L'/'))
        return std::wstring(L"\\\\?\\") + path;
    return path;
}

static std::vector<std::string> SplitTabs(const std::string& line)
{
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;)
    {
        size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab == std::string::npos ? tab : tab - start));
        if (tab == std::string::npos)
            return fields;
        start = tab + 1;
    }
}

static bool ParseUInt64(const std::string& text, uint64_t& value)
{
    value = 0;
    if (text.empty())
        return false;
    const char* first = text.data();
    const char* last = first + text.size();
    auto result = std::from_chars(first, last, value);
    return result.ec == std::errc() && result.ptr == last;
}

static bool ParseDateTime(const std::string& date, const std::string& time, FILETIME& value)
{
    SYSTEMTIME local = {};
    int fields = sscanf_s(date.c_str(), "%hu.%hu.%hu", &local.wYear, &local.wMonth, &local.wDay);
    if (fields != 3)
        return false;
    if (local.wYear < 100)
        local.wYear = static_cast<WORD>(local.wYear < 80 ? local.wYear + 2000 : local.wYear + 1900);

    unsigned short hour = 0;
    unsigned short minute = 0;
    unsigned short second = 0;
    fields = sscanf_s(time.c_str(), "%hu:%hu%*[:.]%hu", &hour, &minute, &second);
    if (fields < 2)
        return false;
    local.wHour = hour;
    local.wMinute = minute;
    local.wSecond = fields >= 3 ? second : 0;

    FILETIME localFileTime = {};
    return SystemTimeToFileTime(&local, &localFileTime) &&
           LocalFileTimeToFileTime(&localFileTime, &value);
}

static std::string StripSourcePrefix(const std::string& path, const std::string& sourceRoot)
{
    std::string result = path;
    std::replace(result.begin(), result.end(), '/', '\\');
    std::string root = sourceRoot;
    std::replace(root.begin(), root.end(), '/', '\\');
    if (!root.empty() && root.back() != '\\')
        root.push_back('\\');
    if (!root.empty() && StartsWithNoCase(result, root))
        result.erase(0, root.size());
    else if (result.size() >= 3 && result[1] == ':' && result[2] == '\\')
        result.erase(0, 3);
    while (!result.empty() && result.front() == '\\')
        result.erase(result.begin());
    return result;
}
}

bool DiskDirNormalizeRelativePath(const std::string& input, std::string& output)
{
    output.clear();
    std::string value = input;
    std::replace(value.begin(), value.end(), '/', '\\');
    while (!value.empty() && value.back() == '\\')
        value.pop_back();
    if (value.empty() || (value.size() >= 2 && value[1] == ':') ||
        (!value.empty() && value.front() == '\\'))
        return false;

    size_t start = 0;
    while (start <= value.size())
    {
        size_t slash = value.find('\\', start);
        std::string component = value.substr(start, slash == std::string::npos ? slash : slash - start);
        if (component.empty() || component == "." || component == "..")
            return false;
        if (!output.empty())
            output.push_back('\\');
        output += component;
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return !output.empty();
}

bool DiskDirReadCatalog(const char* fileName, CDiskDirCatalog& catalog, std::string& error)
{
    catalog = {};
    error.clear();

    std::wstring wideFileName = DiskDirPathToExtendedWide(fileName);
    HANDLE file = CreateFileW(wideFileName.c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        error = LoadStr(IDS_ERR_OPEN_CATALOG);
        return false;
    }

    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 ||
        static_cast<uint64_t>(size.QuadPart) > MAX_CATALOG_SIZE)
    {
        CloseHandle(file);
        error = LoadStr(IDS_ERR_CATALOG_TOO_LARGE);
        return false;
    }

    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    bool readOk = bytes.empty() ||
                  (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, NULL) &&
                   read == bytes.size());
    CloseHandle(file);
    if (!readOk)
    {
        error = LoadStr(IDS_ERR_READ_CATALOG);
        return false;
    }

    bool utf8Catalog = bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xef &&
        static_cast<unsigned char>(bytes[1]) == 0xbb &&
        static_cast<unsigned char>(bytes[2]) == 0xbf;
    if (utf8Catalog)
    {
        bytes.erase(0, 3);
        if (!IsValidUtf8(bytes.data(), bytes.size()))
        {
            error = LoadStr(IDS_ERR_INVALID_UTF8);
            return false;
        }
    }
    else
        bytes = AnsiToUtf8(bytes.data(), bytes.size());

    std::istringstream stream(bytes);
    std::string line;
    std::string lastDirectory;
    bool firstDataLine = true;
    while (std::getline(stream, line))
    {
        line = TrimCarriageReturn(line);
        if (line.empty())
            continue;

        if (firstDataLine && line.find('\t') == std::string::npos)
        {
            catalog.SourceRoot = line;
            std::replace(catalog.SourceRoot.begin(), catalog.SourceRoot.end(), '/', '\\');
            if (!catalog.SourceRoot.empty() && catalog.SourceRoot.back() != '\\')
                catalog.SourceRoot.push_back('\\');
            firstDataLine = false;
            continue;
        }
        firstDataLine = false;

        std::vector<std::string> fields = SplitTabs(line);
        if (fields.empty() || fields[0].empty())
            continue;

        bool isDirectory = fields[0].back() == '\\' || fields[0].back() == '/';
        std::string candidate = StripSourcePrefix(fields[0], catalog.SourceRoot);
        // DiskDir 1.3 writes files below a directory as leaf names following
        // that directory's full entry. Our BOM-marked UTF-8 variant always
        // stores complete relative paths and does not need this state.
        if (!utf8Catalog && candidate.find('\\') == std::string::npos &&
            !lastDirectory.empty())
            candidate = lastDirectory + candidate;

        std::string relative;
        if (!DiskDirNormalizeRelativePath(candidate, relative))
            continue;

        CDiskDirEntry entry = {};
        entry.Path = relative;
        entry.IsDirectory = isDirectory;
        entry.Size = 0;
        if (!isDirectory && fields.size() > 1)
            ParseUInt64(fields[1], entry.Size);
        entry.HasLastWrite = fields.size() > 3 &&
                             ParseDateTime(fields[2], fields[3], entry.LastWrite);
        catalog.Entries.push_back(entry);
        if (!utf8Catalog && isDirectory)
            lastDirectory = relative + "\\";
    }

    if (catalog.Entries.empty() && !bytes.empty())
    {
        error = LoadStr(IDS_ERR_INVALID_CATALOG);
        return false;
    }
    return true;
}

bool DiskDirWriteAll(HANDLE file, const void* data, size_t size, std::string& error)
{
    const unsigned char* position = static_cast<const unsigned char*>(data);
    while (size != 0)
    {
        DWORD chunk = static_cast<DWORD>(std::min<size_t>(size, 1024 * 1024));
        DWORD written = 0;
        if (!WriteFile(file, position, chunk, &written, NULL) || written != chunk)
        {
            error = LoadStr(IDS_ERR_WRITE_CATALOG);
            return false;
        }
        position += written;
        size -= written;
    }
    return true;
}

std::string DiskDirFormatEntry(const char* name, bool isDirectory, uint64_t size,
                               const FILETIME& lastWrite)
{
    std::string path = DiskDirTextToUtf8(name);
    std::replace(path.begin(), path.end(), '/', '\\');
    while (!path.empty() && path.front() == '\\')
        path.erase(path.begin());
    if (isDirectory && (path.empty() || path.back() != '\\'))
        path.push_back('\\');

    FILETIME localTime = {};
    SYSTEMTIME systemTime = {};
    FileTimeToLocalFileTime(&lastWrite, &localTime);
    FileTimeToSystemTime(&localTime, &systemTime);

    char metadata[128];
    _snprintf_s(metadata, _TRUNCATE, "\t%llu\t%u.%u.%u\t%u:%u.%u\r\n",
                static_cast<unsigned long long>(isDirectory ? 0 : size),
                systemTime.wYear, systemTime.wMonth, systemTime.wDay,
                systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
    return path + metadata;
}

bool DiskDirResolveSourcePath(const CDiskDirCatalog& catalog, const std::string& relativePath,
                              std::string& sourcePath)
{
    std::string normalized;
    if (catalog.SourceRoot.empty() ||
        !DiskDirNormalizeRelativePath(relativePath, normalized))
        return false;
    sourcePath = catalog.SourceRoot;
    if (!sourcePath.empty() && sourcePath.back() != '\\')
        sourcePath.push_back('\\');
    sourcePath += normalized;
    return true;
}

std::string DiskDirTextToUtf8(const char* text)
{
    if (text == NULL || text[0] == 0)
        return std::string();
    size_t length = strlen(text);
    if (IsValidUtf8(text, length))
        return std::string(text, length);
    return AnsiToUtf8(text, length);
}

std::wstring DiskDirUtf8ToWide(const std::string& text)
{
    if (text.empty())
        return std::wstring();
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                     static_cast<int>(text.size()), NULL, 0);
    if (length <= 0)
        return std::wstring();
    std::wstring wide(static_cast<size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), wide.data(), length) != length)
        return std::wstring();
    return wide;
}

std::wstring DiskDirPathToExtendedWide(const char* path)
{
    std::string utf8 = DiskDirTextToUtf8(path);
    std::wstring wide = DiskDirUtf8ToWide(utf8);
    if (wide.size() >= 32767)
        return std::wstring();
    return AddExtendedPrefix(wide);
}

bool DiskDirEnsureDirectory(const std::string& path)
{
    std::wstring directory = AddExtendedPrefix(DiskDirUtf8ToWide(path));
    if (directory.empty() || directory.size() >= 32767)
        return false;

    DWORD attributes = GetFileAttributesW(directory.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    size_t rootLength = 0;
    if (directory.rfind(L"\\\\?\\UNC\\", 0) == 0)
    {
        size_t serverEnd = directory.find(L'\\', 8);
        size_t shareEnd = serverEnd == std::wstring::npos
                              ? std::wstring::npos
                              : directory.find(L'\\', serverEnd + 1);
        rootLength = shareEnd == std::wstring::npos ? directory.size() : shareEnd + 1;
    }
    else if (directory.rfind(L"\\\\?\\", 0) == 0 && directory.size() >= 7 &&
             directory[5] == L':' && directory[6] == L'\\')
        rootLength = 7;
    else if (directory.size() >= 3 && directory[1] == L':' && directory[2] == L'\\')
        rootLength = 3;
    else
        return false;

    for (size_t pos = rootLength; pos <= directory.size();)
    {
        pos = directory.find(L'\\', pos);
        std::wstring componentPath =
            pos == std::wstring::npos ? directory : directory.substr(0, pos);
        if (!componentPath.empty() && !CreateDirectoryW(componentPath.c_str(), NULL))
        {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS)
                return false;
        }
        if (pos == std::wstring::npos)
            break;
        ++pos;
    }
    return true;
}
