// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "diskdir_format.h"

HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;

static CDiskDirPlugin PluginInterface;
static CDiskDirArchiver ArchiverInterface;
static BOOL PackPathNames = TRUE;
static BOOL PackSubdirectories = TRUE;

const char* LoadStr(int resourceID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resourceID);
}

namespace
{
static void ShowError(HWND parent, const std::string& text)
{
    SalamanderGeneral->SalMessageBox(parent, text.c_str(), LoadStr(IDS_DISKDIR_TITLE),
                                     MB_OK | MB_ICONERROR);
}

struct CDiskDirPackDialogData
{
    const char* FileName;
    BOOL PackPaths;
    BOOL Recurse;
};

static INT_PTR CALLBACK DiskDirPackDialogProc(HWND window, UINT message,
                                              WPARAM wParam, LPARAM lParam)
{
    CDiskDirPackDialogData* data =
        reinterpret_cast<CDiskDirPackDialogData*>(GetWindowLongPtr(window, DWLP_USER));
    switch (message)
    {
    case WM_INITDIALOG:
    {
        data = reinterpret_cast<CDiskDirPackDialogData*>(lParam);
        SetWindowLongPtr(window, DWLP_USER, reinterpret_cast<LONG_PTR>(data));
        std::wstring wideFileName = DiskDirUtf8ToWide(DiskDirTextToUtf8(data->FileName));
        SetDlgItemTextW(window, IDC_DD_ARCHIVE, wideFileName.c_str());
        CheckDlgButton(window, IDC_DD_PACK_PATHS,
                       data->PackPaths ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, IDC_DD_RECURSE,
                       data->Recurse ? BST_CHECKED : BST_UNCHECKED);
        SalamanderGeneral->MultiMonCenterWindow(window, GetParent(window), TRUE);
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            data->PackPaths =
                IsDlgButtonChecked(window, IDC_DD_PACK_PATHS) == BST_CHECKED;
            data->Recurse =
                IsDlgButtonChecked(window, IDC_DD_RECURSE) == BST_CHECKED;
            EndDialog(window, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(window, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static bool ConfirmOverwrite(HWND parent, const char* fileName)
{
    std::wstring wideFileName = DiskDirPathToExtendedWide(fileName);
    if (wideFileName.empty())
    {
        ShowError(parent, LoadStr(IDS_ERR_INVALID_PATH));
        return false;
    }
    DWORD attributes = GetFileAttributesW(wideFileName.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return true;
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        ShowError(parent, LoadStr(IDS_ERR_NAME_IS_DIRECTORY));
        return false;
    }

    std::string question = LoadStr(IDS_OVERWRITE_PREFIX);
    question += fileName;
    question += LoadStr(IDS_OVERWRITE_SUFFIX);
    return SalamanderGeneral->SalMessageBox(parent, question.c_str(),
                                             LoadStr(IDS_DISKDIR_TITLE),
                                             MB_YESNO | MB_ICONQUESTION |
                                                 MB_DEFBUTTON2) == IDYES;
}

static const char* LeafName(const char* path)
{
    const char* slash = strrchr(path, '\\');
    const char* slash2 = strrchr(path, '/');
    if (slash2 != NULL && (slash == NULL || slash2 > slash))
        slash = slash2;
    return slash == NULL ? path : slash + 1;
}

struct CDiskDirPendingEntry
{
    std::string Name;
    bool IsDirectory;
    uint64_t Size;
    FILETIME LastWrite;
};

static bool SplitArchivePath(const std::string& fullPath, std::string& directory,
                             std::string& name)
{
    size_t slash = fullPath.find_last_of("\\/");
    if (slash == std::string::npos)
    {
        directory.clear();
        name = fullPath;
    }
    else
    {
        directory = fullPath.substr(0, slash);
        name = fullPath.substr(slash + 1);
    }
    return !name.empty();
}

static bool AddCatalogEntry(CSalamanderDirectoryAbstract* directory,
                            const CDiskDirEntry& entry)
{
    std::string path;
    std::string name;
    if (!SplitArchivePath(entry.Path, path, name))
        return true;

    std::wstring wideName = DiskDirUtf8ToWide(name);
    if (wideName.empty() || wideName.size() >= 32767)
        return false;

    std::string narrowName = name;
    if (narrowName.size() > 511)
    {
        UINT codePage = GetACP() == CP_UTF8 ? 1252 : CP_ACP;
        int length = WideCharToMultiByte(codePage, 0, wideName.c_str(), -1, NULL, 0,
                                         NULL, NULL);
        if (length <= 1 || length - 1 > 511)
            return false;
        narrowName.resize(static_cast<size_t>(length));
        WideCharToMultiByte(codePage, 0, wideName.c_str(), -1, narrowName.data(),
                            length, NULL, NULL);
        narrowName.resize(static_cast<size_t>(length - 1));
    }

    CFileData file = {};
    file.Name = SalamanderGeneral->DupStr(narrowName.c_str());
    if (file.Name == NULL)
        return false;
    file.NameLen = static_cast<int>(narrowName.size());
    int wideNameBytes =
        static_cast<int>((wideName.size() + 1) * sizeof(wchar_t));
    file.NameW =
        static_cast<wchar_t*>(SalamanderGeneral->Alloc(wideNameBytes));
    if (file.NameW == NULL)
    {
        SalamanderGeneral->Free(file.Name);
        return false;
    }
    memcpy(file.NameW, wideName.c_str(), (wideName.size() + 1) * sizeof(wchar_t));
    char* extension = strrchr(file.Name, '.');
    file.Ext = !entry.IsDirectory && extension != NULL ? extension + 1 : file.Name + file.NameLen;
    file.Size = CQuadWord(static_cast<DWORD>(entry.Size),
                          static_cast<DWORD>(entry.Size >> 32));
    file.Attr = entry.IsDirectory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
    file.LastWrite = entry.HasLastWrite ? entry.LastWrite : FILETIME{};
    file.IsLink = !entry.IsDirectory && SalamanderGeneral->IsFileLink(file.Ext);
    file.IconOverlayIndex = ICONOVERLAYINDEX_NOTUSED;

    BOOL added = entry.IsDirectory
                     ? directory->AddDir(path.c_str(), file, NULL)
                     : directory->AddFile(path.c_str(), file, NULL);
    if (!added)
    {
        SalamanderGeneral->Free(file.Name);
        SalamanderGeneral->Free(file.NameW);
    }
    return added != FALSE;
}

static bool CopyCatalogFile(const CDiskDirCatalog& catalog, const std::string& archivePath,
                            const std::string& destination)
{
    std::string source;
    if (!DiskDirResolveSourcePath(catalog, archivePath, source))
        return false;

    std::string parent;
    std::string ignored;
    SplitArchivePath(destination, parent, ignored);
    if (!parent.empty() && !DiskDirEnsureDirectory(parent))
        return false;

    std::wstring sourceW = DiskDirPathToExtendedWide(source.c_str());
    std::wstring destinationW = DiskDirPathToExtendedWide(destination.c_str());
    return !sourceW.empty() && !destinationW.empty() &&
           CopyFileW(sourceW.c_str(), destinationW.c_str(), FALSE) != FALSE;
}

static bool CopyAllEntries(const CDiskDirCatalog& catalog, const char* destinationRoot,
                           const std::vector<std::string>& selected)
{
    for (const CDiskDirEntry& entry : catalog.Entries)
    {
        bool wanted = selected.empty();
        for (const std::string& item : selected)
        {
            if (_stricmp(entry.Path.c_str(), item.c_str()) == 0 ||
                (entry.Path.size() > item.size() &&
                 _strnicmp(entry.Path.c_str(), item.c_str(), item.size()) == 0 &&
                 entry.Path[item.size()] == '\\'))
            {
                wanted = true;
                break;
            }
        }
        if (!wanted)
            continue;

        std::string destination = destinationRoot;
        if (!destination.empty() && destination.back() != '\\')
            destination.push_back('\\');
        destination += entry.Path;
        if (entry.IsDirectory)
        {
            if (!DiskDirEnsureDirectory(destination))
                return false;
        }
        else if (!CopyCatalogFile(catalog, entry.Path, destination))
            return false;
    }
    return true;
}
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        DLLInstance = instance;
    return TRUE;
}

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

CPluginInterfaceAbstract* WINAPI
SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBoxA(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER,
                    "DiskDir", MB_OK | MB_ICONERROR);
        return NULL;
    }

    HLanguage = salamander->LoadLanguageModule(
        salamander->GetParentWindow(), "DISKDIR" /* neprekladat! */);
    if (HLanguage == NULL)
        return NULL;

    SalamanderGeneral = salamander->GetSalamanderGeneral();
    salamander->SetBasicPluginData(
        LoadStr(IDS_PLUGINNAME),
        FUNCTION_PANELARCHIVERVIEW | FUNCTION_CUSTOMARCHIVERPACK |
            FUNCTION_CUSTOMARCHIVERUNPACK | FUNCTION_LOADSAVECONFIGURATION,
        VERSINFO_VERSION_NO_PLATFORM,
        VERSINFO_COPYRIGHT,
        LoadStr(IDS_PLUGIN_DESCRIPTION),
        "DISKDIR", "lst");
    salamander->SetPluginHomePageURL("www.altap.cz");
    return &PluginInterface;
}

void WINAPI CDiskDirPlugin::About(HWND parent)
{
    SalamanderGeneral->SalMessageBox(
        parent,
        LoadStr(IDS_ABOUT_TEXT), LoadStr(IDS_ABOUT_TITLE),
        MB_OK | MB_ICONINFORMATION);
}

void WINAPI CDiskDirPlugin::LoadConfiguration(
    HWND, HKEY key, CSalamanderRegistryAbstract* registry)
{
    PackPathNames = TRUE;
    PackSubdirectories = TRUE;
    if (key != NULL)
    {
        registry->GetValue(key, "Pack Path Names", REG_DWORD, &PackPathNames,
                           sizeof(PackPathNames));
        registry->GetValue(key, "Pack Subdirectories", REG_DWORD,
                           &PackSubdirectories, sizeof(PackSubdirectories));
    }
}

void WINAPI CDiskDirPlugin::SaveConfiguration(
    HWND, HKEY key, CSalamanderRegistryAbstract* registry)
{
    if (key != NULL)
    {
        registry->SetValue(key, "Pack Path Names", REG_DWORD, &PackPathNames,
                           sizeof(PackPathNames));
        registry->SetValue(key, "Pack Subdirectories", REG_DWORD,
                           &PackSubdirectories, sizeof(PackSubdirectories));
    }
}

void WINAPI CDiskDirPlugin::Connect(HWND, CSalamanderConnectAbstract* salamander)
{
    salamander->AddCustomPacker(LoadStr(IDS_PACKER_NAME), "lst", FALSE);
    salamander->AddCustomUnpacker(LoadStr(IDS_PACKER_NAME), "*.lst", FALSE);
    salamander->AddPanelArchiver("lst", FALSE, FALSE);
}

CPluginInterfaceForArchiverAbstract* WINAPI CDiskDirPlugin::GetInterfaceForArchiver()
{
    return &ArchiverInterface;
}

BOOL WINAPI CDiskDirArchiver::ListArchive(CSalamanderForOperationsAbstract*,
                                          const char* fileName,
                                          CSalamanderDirectoryAbstract* dir,
                                          CPluginDataInterfaceAbstract*& pluginData)
{
    pluginData = NULL;
    dir->SetValidData(VALID_DATA_EXTENSION | VALID_DATA_SIZE | VALID_DATA_DATE |
                      VALID_DATA_TIME | VALID_DATA_ATTRIBUTES | VALID_DATA_ISLINK);

    CDiskDirCatalog catalog;
    std::string error;
    if (!DiskDirReadCatalog(fileName, catalog, error))
    {
        ShowError(SalamanderGeneral->GetMsgBoxParent(), error);
        return FALSE;
    }
    for (const CDiskDirEntry& entry : catalog.Entries)
    {
        if (!AddCatalogEntry(dir, entry))
        {
            dir->Clear(NULL);
            return FALSE;
        }
    }
    return TRUE;
}

BOOL WINAPI CDiskDirArchiver::PackToArchive(CSalamanderForOperationsAbstract*,
                                            const char* fileName, const char* archiveRoot,
                                            BOOL move, const char* sourcePath,
                                            SalEnumSelection2 next, void* nextParam)
{
    if (move)
    {
        ShowError(SalamanderGeneral->GetMsgBoxParent(),
                  LoadStr(IDS_ERR_MOVE_NOT_SUPPORTED));
        return FALSE;
    }
    if (archiveRoot != NULL && archiveRoot[0] != 0)
    {
        ShowError(SalamanderGeneral->GetMsgBoxParent(),
                  LoadStr(IDS_ERR_ADD_NOT_SUPPORTED));
        return FALSE;
    }

    CDiskDirPackDialogData options = {fileName, PackPathNames,
                                      PackSubdirectories};
    HWND parent = SalamanderGeneral->GetMsgBoxParent();
    if (DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_DISKDIR_PACK), parent,
                       DiskDirPackDialogProc,
                       reinterpret_cast<LPARAM>(&options)) != IDOK)
        return FALSE;
    PackPathNames = options.PackPaths;
    PackSubdirectories = options.Recurse;

    std::vector<CDiskDirPendingEntry> entries;
    int enumerationResult = SALENUM_SUCCESS;
    for (;;)
    {
        const char* dosName = NULL;
        BOOL isDirectory = FALSE;
        CQuadWord size;
        DWORD attributes = 0;
        FILETIME lastWrite = {};
        const char* name = next(SalamanderGeneral->GetMsgBoxParent(),
                                options.Recurse ? 1 : 0, &dosName,
                                &isDirectory, &size, &attributes, &lastWrite,
                                nextParam, &enumerationResult);
        if (name == NULL)
            break;
        const char* storedName = options.PackPaths ? name : LeafName(name);
        CDiskDirPendingEntry entry = {};
        entry.Name = storedName;
        entry.IsDirectory = isDirectory != FALSE;
        entry.Size = size.Value;
        entry.LastWrite = lastWrite;
        entries.push_back(entry);
    }

    if (enumerationResult != SALENUM_SUCCESS)
        return FALSE;

    if (!ConfirmOverwrite(parent, fileName))
        return FALSE;

    std::wstring wideFileName = DiskDirPathToExtendedWide(fileName);
    HANDLE output = wideFileName.empty()
                        ? INVALID_HANDLE_VALUE
                        : CreateFileW(wideFileName.c_str(), GENERIC_WRITE,
                                      FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (output == INVALID_HANDLE_VALUE)
    {
        ShowError(SalamanderGeneral->GetMsgBoxParent(),
                  LoadStr(IDS_ERR_CREATE_CATALOG));
        return FALSE;
    }

    std::string error;
    static const unsigned char utf8Bom[] = {0xef, 0xbb, 0xbf};
    bool ok = DiskDirWriteAll(output, utf8Bom, sizeof(utf8Bom), error);
    std::string root = DiskDirTextToUtf8(sourcePath);
    if (!root.empty() && root.back() != '\\')
        root.push_back('\\');
    root += "\r\n";
    if (ok)
        ok = DiskDirWriteAll(output, root.data(), root.size(), error);
    for (const CDiskDirPendingEntry& entry : entries)
    {
        if (!ok)
            break;
        std::string line = DiskDirFormatEntry(
            entry.Name.c_str(), entry.IsDirectory, entry.Size, entry.LastWrite);
        ok = DiskDirWriteAll(output, line.data(), line.size(), error);
    }
    CloseHandle(output);

    if (!ok)
    {
        DeleteFileW(wideFileName.c_str());
        ShowError(parent, error);
    }
    return ok ? TRUE : FALSE;
}

BOOL WINAPI CDiskDirArchiver::UnpackOneFile(CSalamanderForOperationsAbstract*,
                                            const char* fileName,
                                            CPluginDataInterfaceAbstract*,
                                            const char* nameInArchive,
                                            const CFileData*, const char* targetDir,
                                            const char* newFileName,
                                            BOOL* renamingNotSupported)
{
    CDiskDirCatalog catalog;
    std::string error;
    if (!DiskDirReadCatalog(fileName, catalog, error))
        return FALSE;

    const char* leaf = strrchr(nameInArchive, '\\');
    leaf = leaf == NULL ? nameInArchive : leaf + 1;
    std::string destination = targetDir;
    if (!destination.empty() && destination.back() != '\\')
        destination.push_back('\\');
    destination += newFileName != NULL ? newFileName : leaf;
    if (newFileName != NULL && renamingNotSupported != NULL)
        *renamingNotSupported = FALSE;
    if (!CopyCatalogFile(catalog, nameInArchive, destination))
    {
        ShowError(SalamanderGeneral->GetMsgBoxParent(),
                  LoadStr(IDS_ERR_SOURCE_NOT_AVAILABLE));
        return FALSE;
    }
    return TRUE;
}

BOOL WINAPI CDiskDirArchiver::UnpackArchive(CSalamanderForOperationsAbstract* salamander,
                                            const char* fileName,
                                            CPluginDataInterfaceAbstract*,
                                            const char* targetDir,
                                            const char* archiveRoot,
                                            SalEnumSelection next, void* nextParam)
{
    CDiskDirCatalog catalog;
    std::string error;
    if (!DiskDirReadCatalog(fileName, catalog, error))
        return FALSE;

    std::vector<std::string> selected;
    BOOL isDirectory = FALSE;
    CQuadWord size;
    const CFileData* data = NULL;
    const char* name = NULL;
    while ((name = next(NULL, 0, &isDirectory, &size, &data, nextParam, NULL)) != NULL)
    {
        std::string full = archiveRoot != NULL ? archiveRoot : "";
        if (!full.empty() && full.back() != '\\')
            full.push_back('\\');
        full += name;
        selected.push_back(full);
    }

    char temporary[SAL_MAX_PATH];
    DWORD tempError = ERROR_SUCCESS;
    if (!SalamanderGeneral->SalGetTempFileName(targetDir, "SDD", temporary,
                                               FALSE, &tempError))
        return FALSE;
    bool removeTemporary = true;
    bool copied = CopyAllEntries(catalog, temporary, selected);
    if (copied)
    {
        copied = salamander->MoveFiles(temporary, targetDir, temporary, fileName) != FALSE;
        removeTemporary = copied;
    }
    if (removeTemporary)
        SalamanderGeneral->RemoveTemporaryDir(temporary);
    if (!copied)
        ShowError(SalamanderGeneral->GetMsgBoxParent(),
                  LoadStr(IDS_ERR_SOURCES_NOT_AVAILABLE));
    return copied ? TRUE : FALSE;
}

BOOL WINAPI CDiskDirArchiver::UnpackWholeArchive(CSalamanderForOperationsAbstract*,
                                                 const char* fileName, const char*,
                                                 const char* targetDir,
                                                 BOOL delArchiveWhenDone,
                                                 CDynamicString* archiveVolumes)
{
    CDiskDirCatalog catalog;
    std::string error;
    if (!DiskDirReadCatalog(fileName, catalog, error))
        return FALSE;
    if (!CopyAllEntries(catalog, targetDir, {}))
        return FALSE;
    if (delArchiveWhenDone)
        archiveVolumes->Add(fileName, -2);
    return TRUE;
}
