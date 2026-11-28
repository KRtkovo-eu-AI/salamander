// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Resolve the bundled 7-Zip library relative to the plugin module. | The post-exit helper runs under rundll32, not the Salamander executable. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Extract official packages through the bundled 7-Zip SDK with path and link validation. | Remove the managed SharpCompress dependency and reject unsafe archive entries. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_archive.h"
#include "native_utils.h"

#include "../7zip/7za/cpp/7zip/Archive/IArchive.h"
#include "../7zip/7za/cpp/7zip/PropID.h"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace SamandarinNative
{
namespace
{
constexpr ULONGLONG kMaximumExpandedBytes = 1024ull * 1024ull * 1024ull;

const GUID kSevenZipFormat = {0x23170F69, 0x40C1, 0x278A, {0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00}};
const GUID kIidInArchive = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x00, 0x00}};
const GUID kIidInStream = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00}};
const GUID kIidSequentialInStream = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00}};
const GUID kIidSequentialOutStream = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00}};
const GUID kIidArchiveOpenCallback = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00}};
const GUID kIidArchiveExtractCallback = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x06, 0x00, 0x20, 0x00, 0x00}};
const GUID kIidProgress = {0x23170F69, 0x40C1, 0x278A, {0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00}};

typedef HRESULT(WINAPI* FCreateObject)(const GUID* classId, const GUID* interfaceId, void** object);

template <typename T, typename... Args>
T* NewNoThrow(Args&&... args)
{
#if defined(_DEBUG) && defined(_MSC_VER)
#undef new
#define SAMANDARIN_RESTORE_DEBUG_NEW
#endif
    T* result = nullptr;
    try
    {
        result = new (std::nothrow) T(std::forward<Args>(args)...);
    }
    catch (...)
    {
        result = nullptr;
    }
#ifdef SAMANDARIN_RESTORE_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SAMANDARIN_RESTORE_DEBUG_NEW
#endif
    return result;
}

struct ArchiveItem
{
    std::wstring RelativePath;
    bool IsDirectory = false;
    ULONGLONG Size = 0;
};

HRESULT FileError(DWORD error)
{
    return HRESULT_FROM_WIN32(error == ERROR_SUCCESS ? ERROR_GEN_FAILURE : error);
}

bool EqualsIgnoreCase(const std::wstring& left, const std::wstring& right)
{
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

class CArchiveInStream final : public IInStream
{
public:
    explicit CArchiveInStream(const std::wstring& path)
    {
        File = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    ~CArchiveInStream()
    {
        if (File != INVALID_HANDLE_VALUE) CloseHandle(File);
    }

    bool IsOpen() const { return File != INVALID_HANDLE_VALUE; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (object == nullptr) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, kIidInStream))
        {
            *object = static_cast<IInStream*>(this);
        }
        else if (IsEqualIID(iid, kIidSequentialInStream))
        {
            *object = static_cast<ISequentialInStream*>(this);
        }
        else
        {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&References)); }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG remaining = InterlockedDecrement(&References);
        if (remaining == 0) delete this;
        return static_cast<ULONG>(remaining);
    }

    HRESULT STDMETHODCALLTYPE Read(void* data, UInt32 size, UInt32* processedSize) noexcept override
    {
        if (processedSize == nullptr) return E_POINTER;
        *processedSize = 0;
        if (size == 0) return S_OK;
        DWORD read = 0;
        if (!ReadFile(File, data, size, &read, nullptr)) return FileError(GetLastError());
        *processedSize = read;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) noexcept override
    {
        DWORD method = FILE_BEGIN;
        if (seekOrigin == STREAM_SEEK_CUR) method = FILE_CURRENT;
        else if (seekOrigin == STREAM_SEEK_END) method = FILE_END;
        else if (seekOrigin != STREAM_SEEK_SET) return STG_E_INVALIDFUNCTION;
        LARGE_INTEGER distance = {};
        distance.QuadPart = offset;
        LARGE_INTEGER position = {};
        if (!SetFilePointerEx(File, distance, &position, method)) return FileError(GetLastError());
        if (position.QuadPart < 0) return STG_E_INVALIDFUNCTION;
        if (newPosition != nullptr) *newPosition = static_cast<UInt64>(position.QuadPart);
        return S_OK;
    }

private:
    volatile LONG References = 1;
    HANDLE File = INVALID_HANDLE_VALUE;
};

class CArchiveOpenCallback final : public IArchiveOpenCallback
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (object == nullptr) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, kIidArchiveOpenCallback))
            *object = static_cast<IArchiveOpenCallback*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&References)); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG remaining = InterlockedDecrement(&References);
        if (remaining == 0) delete this;
        return static_cast<ULONG>(remaining);
    }
    HRESULT STDMETHODCALLTYPE SetTotal(const UInt64*, const UInt64*) noexcept override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetCompleted(const UInt64*, const UInt64*) noexcept override { return S_OK; }

private:
    volatile LONG References = 1;
};

class CArchiveOutStream final : public ISequentialOutStream
{
public:
    CArchiveOutStream(HANDLE file, ULONGLONG* totalWritten) : File(file), TotalWritten(totalWritten) {}
    ~CArchiveOutStream()
    {
        if (File != INVALID_HANDLE_VALUE) CloseHandle(File);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (object == nullptr) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, kIidSequentialOutStream))
            *object = static_cast<ISequentialOutStream*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&References)); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG remaining = InterlockedDecrement(&References);
        if (remaining == 0) delete this;
        return static_cast<ULONG>(remaining);
    }
    HRESULT STDMETHODCALLTYPE Write(const void* data, UInt32 size, UInt32* processedSize) noexcept override
    {
        if (processedSize == nullptr) return E_POINTER;
        *processedSize = 0;
        if (TotalWritten == nullptr || size > kMaximumExpandedBytes ||
            *TotalWritten > kMaximumExpandedBytes - size)
        {
            return E_FAIL;
        }
        DWORD written = 0;
        if (!WriteFile(File, data, size, &written, nullptr)) return FileError(GetLastError());
        *processedSize = written;
        *TotalWritten += written;
        return written == size ? S_OK : STG_E_WRITEFAULT;
    }

private:
    volatile LONG References = 1;
    HANDLE File = INVALID_HANDLE_VALUE;
    ULONGLONG* TotalWritten;
};

class CArchiveExtractCallback final : public IArchiveExtractCallback
{
public:
    CArchiveExtractCallback(IInArchive* archive, const std::wstring& outputRoot,
                            const std::wstring& packageRoot, const std::vector<ArchiveItem>& items)
        : Archive(archive), OutputRoot(outputRoot), PackageRoot(packageRoot), Items(items)
    {
        if (Archive != nullptr) Archive->AddRef();
    }

    ~CArchiveExtractCallback()
    {
        if (ActiveStream != nullptr) ActiveStream->Release();
        if (Archive != nullptr) Archive->Release();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (object == nullptr) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, kIidArchiveExtractCallback))
            *object = static_cast<IArchiveExtractCallback*>(this);
        else if (IsEqualIID(iid, kIidProgress))
            *object = static_cast<IProgress*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&References)); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG remaining = InterlockedDecrement(&References);
        if (remaining == 0) delete this;
        return static_cast<ULONG>(remaining);
    }
    HRESULT STDMETHODCALLTYPE SetTotal(UInt64) noexcept override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetCompleted(const UInt64*) noexcept override { return S_OK; }

    HRESULT STDMETHODCALLTYPE GetStream(UInt32 index, ISequentialOutStream** output, Int32 askMode) noexcept override
    {
        try
        {
        if (output == nullptr) return E_POINTER;
        *output = nullptr;
        if (ActiveStream != nullptr)
        {
            ActiveStream->Release();
            ActiveStream = nullptr;
        }
        if (askMode != NArchive::NExtract::NAskMode::kExtract) return S_OK;
        if (index >= Items.size()) return E_INVALIDARG;
        const ArchiveItem& item = Items[index];
        const std::wstring path = CombinePath(PackageRoot, item.RelativePath);
        if (!IsPathInside(path, OutputRoot)) return E_ACCESSDENIED;
        if (item.IsDirectory)
        {
            return CreateDirectories(path) ? S_OK : FileError(GetLastError());
        }
        const std::wstring parent = GetParentDirectory(path);
        if (!CreateDirectories(parent)) return FileError(GetLastError());
        HANDLE file = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_WRITE, 0, nullptr,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return FileError(GetLastError());
        CArchiveOutStream* stream = NewNoThrow<CArchiveOutStream>(file, &TotalWritten);
        if (stream == nullptr)
        {
            CloseHandle(file);
            DeleteFileW(AddLongPathPrefix(path).c_str());
            return E_OUTOFMEMORY;
        }
        stream->AddRef();
        ActiveStream = stream;
        *output = static_cast<ISequentialOutStream*>(stream);
        return S_OK;
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT STDMETHODCALLTYPE PrepareOperation(Int32) noexcept override { return S_OK; }

    HRESULT STDMETHODCALLTYPE SetOperationResult(Int32 operationResult) noexcept override
    {
        if (ActiveStream != nullptr)
        {
            ActiveStream->Release();
            ActiveStream = nullptr;
        }
        return operationResult == NArchive::NExtract::NOperationResult::kOK ? S_OK : E_FAIL;
    }

private:
    volatile LONG References = 1;
    IInArchive* Archive = nullptr;
    std::wstring OutputRoot;
    std::wstring PackageRoot;
    const std::vector<ArchiveItem>& Items;
    CArchiveOutStream* ActiveStream = nullptr;
    ULONGLONG TotalWritten = 0;
};

void ClearPropVariant(PROPVARIANT* value)
{
    if (value != nullptr) VariantClear(reinterpret_cast<VARIANT*>(value));
}

bool ReadStringProperty(IInArchive* archive, UInt32 index, PROPID property, std::wstring* output)
{
    PROPVARIANT value = {};
    if (FAILED(archive->GetProperty(index, property, &value))) return false;
    bool success = true;
    if (value.vt == VT_BSTR && value.bstrVal != nullptr)
        output->assign(value.bstrVal, SysStringLen(value.bstrVal));
    else if (value.vt != VT_EMPTY && value.vt != VT_NULL)
        success = false;
    ClearPropVariant(&value);
    return success;
}

bool ReadBoolProperty(IInArchive* archive, UInt32 index, PROPID property, bool* output)
{
    PROPVARIANT value = {};
    if (FAILED(archive->GetProperty(index, property, &value))) return false;
    bool success = true;
    if (value.vt == VT_BOOL) *output = value.boolVal != VARIANT_FALSE;
    else if (value.vt != VT_EMPTY && value.vt != VT_NULL) success = false;
    ClearPropVariant(&value);
    return success;
}

bool ReadSizeProperty(IInArchive* archive, UInt32 index, ULONGLONG* output)
{
    PROPVARIANT value = {};
    if (FAILED(archive->GetProperty(index, kpidSize, &value))) return false;
    bool success = true;
    if (value.vt == VT_UI8) *output = value.uhVal.QuadPart;
    else if (value.vt == VT_UI4) *output = value.ulVal;
    else if (value.vt != VT_EMPTY && value.vt != VT_NULL) success = false;
    ClearPropVariant(&value);
    return success;
}

bool HasLinkProperty(IInArchive* archive, UInt32 index, PROPID property)
{
    PROPVARIANT value = {};
    if (FAILED(archive->GetProperty(index, property, &value))) return true;
    bool link = false;
    if (value.vt == VT_BSTR) link = value.bstrVal != nullptr && SysStringLen(value.bstrVal) != 0;
    else if (value.vt == VT_BOOL) link = value.boolVal != VARIANT_FALSE;
    else if (value.vt == VT_UI4) link = value.ulVal != 0;
    else if (value.vt != VT_EMPTY && value.vt != VT_NULL) link = true;
    ClearPropVariant(&value);
    return link;
}

bool ReadAttributes(IInArchive* archive, UInt32 index, ULONG* attributes)
{
    PROPVARIANT value = {};
    if (FAILED(archive->GetProperty(index, kpidAttrib, &value))) return false;
    bool success = true;
    if (value.vt == VT_UI4) *attributes = value.ulVal;
    else if (value.vt == VT_UI8) *attributes = static_cast<ULONG>(value.uhVal.QuadPart);
    else if (value.vt != VT_EMPTY && value.vt != VT_NULL) success = false;
    ClearPropVariant(&value);
    return success;
}

bool ValidateArchivePath(std::wstring path, bool isDirectory, std::wstring* normalized,
                         std::wstring* rootName)
{
    std::replace(path.begin(), path.end(), L'/', L'\\');
    if (path.empty() || path.front() == L'\\' || path.find(L':') != std::wstring::npos) return false;
    if (path.back() == L'\\')
    {
        if (!isDirectory) return false;
        path.pop_back();
    }
    if (path.empty()) return false;
    for (wchar_t c : path)
    {
        if (c < 0x20 || c == L'<' || c == L'>' || c == L'"' || c == L'|' || c == L'?' || c == L'*') return false;
    }
    size_t start = 0;
    size_t segmentCount = 0;
    std::wstring first;
    for (size_t index = 0; index <= path.size(); ++index)
    {
        if (index != path.size() && path[index] != L'\\') continue;
        const std::wstring segment = path.substr(start, index - start);
        if (segment.empty() || segment == L"." || segment == L"..") return false;
        if (segmentCount == 0) first = segment;
        ++segmentCount;
        start = index + 1;
    }
    if (segmentCount == 1 && !isDirectory) return false;
    if (rootName->empty()) *rootName = first;
    else if (!EqualsIgnoreCase(*rootName, first)) return false;
    *normalized = path;
    return true;
}

bool EnumerateArchive(IInArchive* archive, std::vector<ArchiveItem>* items,
                      std::wstring* rootName, std::wstring* error)
{
    UInt32 count = 0;
    if (FAILED(archive->GetNumberOfItems(&count)) || count == 0)
    {
        if (error != nullptr) *error = L"The package is empty.";
        return false;
    }
    items->clear();
    items->resize(count);
    ULONGLONG totalSize = 0;
    for (UInt32 index = 0; index < count; ++index)
    {
        std::wstring path;
        bool isDirectory = false;
        ULONGLONG size = 0;
        ULONG attributes = 0;
        if (!ReadStringProperty(archive, index, kpidPath, &path) ||
            !ReadBoolProperty(archive, index, kpidIsDir, &isDirectory) ||
            !ReadSizeProperty(archive, index, &size) ||
            !ReadAttributes(archive, index, &attributes))
        {
            if (error != nullptr) *error = L"A package entry could not be read.";
            return false;
        }
        if (HasLinkProperty(archive, index, kpidSymLink) || HasLinkProperty(archive, index, kpidHardLink) ||
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ||
            (attributes & 0170000) == 0120000)
        {
            if (error != nullptr) *error = L"Symbolic and hard links are not allowed in update packages.";
            return false;
        }
        std::wstring normalized;
        if (!ValidateArchivePath(path, isDirectory, &normalized, rootName))
        {
            if (error != nullptr) *error = L"The package contains an unsafe path or more than one top-level directory.";
            return false;
        }
        if (!isDirectory)
        {
            if (size > kMaximumExpandedBytes || totalSize > kMaximumExpandedBytes - size)
            {
                if (error != nullptr) *error = L"The expanded package is larger than the allowed limit.";
                return false;
            }
            totalSize += size;
        }
        (*items)[index] = {normalized, isDirectory, size};
    }
    return !rootName->empty();
}

bool EnumerateFiles(const std::wstring& root, std::vector<std::wstring>* files,
                    std::wstring* error)
{
    std::vector<std::wstring> directories{root};
    files->clear();
    while (!directories.empty())
    {
        const std::wstring directory = std::move(directories.back());
        directories.pop_back();
        WIN32_FIND_DATAW data = {};
        const std::wstring pattern = AddLongPathPrefix(CombinePath(directory, L"*"));
        HANDLE find = FindFirstFileW(pattern.c_str(), &data);
        if (find == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) continue;
            if (error != nullptr) *error = L"The extracted package could not be scanned.";
            return false;
        }
        do
        {
            if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
            const std::wstring path = CombinePath(directory, data.cFileName);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            {
                FindClose(find);
                if (error != nullptr) *error = L"Reparse points are not allowed in update packages.";
                return false;
            }
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                directories.push_back(path);
            else
                files->push_back(path);
        } while (FindNextFileW(find, &data));
        const DWORD findError = GetLastError();
        FindClose(find);
        if (findError != ERROR_NO_MORE_FILES)
        {
            if (error != nullptr) *error = L"The extracted package could not be scanned.";
            return false;
        }
    }
    return true;
}

bool LoadSevenZipLibrary(HMODULE* module, FCreateObject* createObject, std::wstring* error)
{
    const std::wstring modulePath = GetModulePath(DLLInstance);
    const std::wstring pluginDirectory = GetParentDirectory(modulePath);
    const std::wstring pluginsDirectory = GetParentDirectory(pluginDirectory);
    std::wstring exeDirectory = GetParentDirectory(pluginsDirectory);
    if (exeDirectory.empty()) exeDirectory = GetExecutableDirectory();
    std::wstring library = CombinePath(CombinePath(CombinePath(exeDirectory, L"plugins"), L"7zip"), L"7za.dll");
    *module = LoadLibraryExW(AddLongPathPrefix(library).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (*module == nullptr)
    {
        library = CombinePath(CombinePath(exeDirectory, L"plugins"), L"7za.dll");
        *module = LoadLibraryExW(AddLongPathPrefix(library).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    if (*module == nullptr)
    {
        if (error != nullptr) *error = L"The bundled 7-Zip archive library could not be loaded.";
        return false;
    }
    *createObject = reinterpret_cast<FCreateObject>(GetProcAddress(*module, "CreateObject"));
    if (*createObject == nullptr)
    {
        FreeLibrary(*module);
        *module = nullptr;
        if (error != nullptr) *error = L"The bundled 7-Zip archive library is missing its archive interface.";
        return false;
    }
    return true;
}
} // namespace

bool ExtractAndValidatePackage(const std::wstring& archivePath, const std::wstring& payloadDirectory,
                               PackageKind kind, std::wstring* extractedRoot,
                               std::wstring* error)
{
    if (kind != PackageKind::Plugin && kind != PackageKind::Extension) return false;
    if (!CreateDirectories(payloadDirectory))
    {
        if (error != nullptr) *error = L"The package staging directory could not be created.";
        return false;
    }
    HMODULE module = nullptr;
    FCreateObject createObject = nullptr;
    if (!LoadSevenZipLibrary(&module, &createObject, error)) return false;

    IInArchive* archive = nullptr;
    CArchiveInStream* input = NewNoThrow<CArchiveInStream>(archivePath);
    CArchiveOpenCallback* openCallback = NewNoThrow<CArchiveOpenCallback>();
    bool success = false;
    if (input == nullptr || openCallback == nullptr || !input->IsOpen())
    {
        if (error != nullptr) *error = L"The package archive could not be opened.";
        if (input != nullptr) input->Release();
        if (openCallback != nullptr) openCallback->Release();
        FreeLibrary(module);
        return false;
    }
    HRESULT hr = createObject(&kSevenZipFormat, &kIidInArchive, reinterpret_cast<void**>(&archive));
    if (FAILED(hr) || archive == nullptr)
    {
        if (error != nullptr) *error = L"The 7-Zip library could not create an archive reader.";
    }
    else
    {
        hr = archive->Open(input, nullptr, openCallback);
        std::vector<ArchiveItem> items;
        std::wstring rootName;
        if (SUCCEEDED(hr) && EnumerateArchive(archive, &items, &rootName, error))
        {
            const std::wstring outputRoot = FullPath(payloadDirectory);
            const std::wstring packageRoot = CombinePath(outputRoot, rootName);
            if (!IsPathInside(packageRoot, outputRoot) || !CreateDirectories(packageRoot))
            {
                if (error != nullptr) *error = L"The extracted package path is invalid.";
            }
            else
            {
                CArchiveExtractCallback* extractCallback =
                    NewNoThrow<CArchiveExtractCallback>(archive, outputRoot, packageRoot, items);
                if (extractCallback == nullptr)
                {
                    if (error != nullptr) *error = L"The package extraction callback could not be created.";
                }
                else
                {
                    hr = archive->Extract(nullptr, static_cast<UInt32>(-1), 0, extractCallback);
                    extractCallback->Release();
                    if (SUCCEEDED(hr))
                    {
                        std::vector<std::wstring> files;
                        std::wstring scanError;
                        if (EnumerateFiles(packageRoot, &files, &scanError))
                        {
                            bool hasPluginModule = false;
                            bool hasExtensionManifest = false;
                            for (const std::wstring& file : files)
                            {
                                const std::wstring extension = GetFileExtension(file);
                                if (EqualsIgnoreCase(extension, L".spl")) hasPluginModule = true;
                                if (EqualsIgnoreCase(GetFileName(file), L"extension.json")) hasExtensionManifest = true;
                            }
                            success = kind == PackageKind::Plugin ? hasPluginModule : hasExtensionManifest;
                            if (!success && error != nullptr)
                            {
                                *error = kind == PackageKind::Plugin
                                             ? L"The plugin package does not contain an .spl module."
                                             : L"The extension package does not contain extension.json.";
                            }
                            if (success && extractedRoot != nullptr) *extractedRoot = packageRoot;
                        }
                        else if (error != nullptr)
                        {
                            *error = scanError;
                        }
                    }
                    else if (error != nullptr)
                    {
                        *error = L"The 7-Zip library could not extract the package.";
                    }
                }
            }
        }
        else if (SUCCEEDED(hr) && error != nullptr && error->empty())
        {
            *error = L"The package archive is invalid.";
        }
        if (archive != nullptr)
        {
            archive->Close();
            archive->Release();
        }
    }
    input->Release();
    openCallback->Release();
    FreeLibrary(module);
    return success;
}
} // namespace SamandarinNative
