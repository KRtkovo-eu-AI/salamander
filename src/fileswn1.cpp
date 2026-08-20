// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"
#include "common/winlibdpi.h"

#include "cfgdlg.h"
#include "dialogs.h"
#include "mainwnd.h"
#include "usermenu.h"
#include "plugins.h"
#include "fileswnd.h"
#include "stswnd.h"
#include "snooper.h"
#include "zip.h"
#include "shellib.h"
#include "pack.h"
#include "thumbnl.h"
#include "geticon.h"
#include "shiconov.h"
#include "common/widepath.h"

namespace
{
std::wstring TreeViewTextToWide(const char* text)
{
    if (text == NULL || text[0] == 0)
        return std::wstring();

    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (required == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        required = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    }
    if (required <= 1)
        return std::wstring();

    std::wstring result(required, L'\0');
    int converted = MultiByteToWideChar(codePage, flags, text, -1, &result[0], required);
    if (converted == 0)
        return std::wstring();
    if (result[converted - 1] == L'\0')
        --converted;
    result.resize(converted);
    return result;
}

std::wstring PathToWideMirror(const char* path)
{
    return TreeViewTextToWide(path);
}

std::string TreeViewWideToText(const wchar_t* text)
{
    return SalWideToMultiBytePath(text, CP_UTF8);
}


BOOL IsIcoFileName(const char* fileName)
{
    if (fileName == NULL)
        return FALSE;

    const char* slash = strrchr(fileName, '\\');
    const char* slash2 = strrchr(fileName, '/');
    if (slash2 != NULL && (slash == NULL || slash2 > slash))
        slash = slash2;

    const char* dot = strrchr(fileName, '.');
    return dot != NULL && (slash == NULL || dot > slash) && stricmp(dot + 1, "ico") == 0;
}

WORD ReadWordLE(const BYTE* data)
{
    return (WORD)(data[0] | (data[1] << 8));
}

DWORD ReadDWordLE(const BYTE* data)
{
    return (DWORD)data[0] | ((DWORD)data[1] << 8) | ((DWORD)data[2] << 16) | ((DWORD)data[3] << 24);
}

int GetBestIcoImageSize(const char* path, int maxSize)
{
    HANDLE file = HANDLES_Q(CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return 0;

    BYTE header[6];
    DWORD read = 0;
    BOOL ok = ReadFile(file, header, sizeof(header), &read, NULL) && read == sizeof(header) &&
              ReadWordLE(header) == 0 && ReadWordLE(header + 2) == 1;
    WORD count = ok ? ReadWordLE(header + 4) : 0;
    if (count > 512) // sanity limit for malformed files
        ok = FALSE;

    int bestFitSize = 0;
    int bestFitBpp = -1;
    DWORD bestFitBytes = 0;
    int smallestOversize = 0;
    int smallestOversizeBpp = -1;
    DWORD smallestOversizeBytes = 0;

    for (WORD i = 0; ok && i < count; i++)
    {
        BYTE entry[16];
        ok = ReadFile(file, entry, sizeof(entry), &read, NULL) && read == sizeof(entry);
        if (!ok)
            break;

        int width = entry[0] == 0 ? 256 : entry[0];
        int height = entry[1] == 0 ? 256 : entry[1];
        int size = max(width, height);
        int bpp = ReadWordLE(entry + 6);
        if (bpp == 0)
            bpp = entry[2] == 0 ? 256 : entry[2];
        DWORD bytes = ReadDWordLE(entry + 8);

        if (size <= maxSize)
        {
            if (size > bestFitSize ||
                size == bestFitSize && (bpp > bestFitBpp || bpp == bestFitBpp && bytes > bestFitBytes))
            {
                bestFitSize = size;
                bestFitBpp = bpp;
                bestFitBytes = bytes;
            }
        }
        else if (smallestOversize == 0 || size < smallestOversize ||
                 size == smallestOversize && (bpp > smallestOversizeBpp || bpp == smallestOversizeBpp && bytes > smallestOversizeBytes))
        {
            smallestOversize = size;
            smallestOversizeBpp = bpp;
            smallestOversizeBytes = bytes;
        }
    }

    HANDLES(CloseHandle(file));
    return bestFitSize != 0 ? bestFitSize : smallestOversize;
}

BOOL LoadIcoThumbnail(const char* path, int thumbnailSize, COLORREF bkgndColor, CSalamanderThumbnailMaker* thumbMaker)
{
    if (!IsIcoFileName(path) || thumbnailSize <= 0 || thumbMaker == NULL)
        return FALSE;

    int iconSize = GetBestIcoImageSize(path, thumbnailSize);
    if (iconSize <= 0)
        iconSize = thumbnailSize;

    HICON hIcon = (HICON)HANDLES(LoadImage(NULL, path, IMAGE_ICON, iconSize, iconSize,
                                           LR_LOADFROMFILE | IconLRFlags));
    if (hIcon == NULL)
        return FALSE;

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = iconSize;
    bi.bmiHeader.biHeight = -iconSize;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HDC screenDC = HANDLES(GetDC(NULL));
    HDC memDC = screenDC != NULL ? HANDLES(CreateCompatibleDC(screenDC)) : NULL;
    HBITMAP bitmap = memDC != NULL ? HANDLES(CreateDIBSection(memDC, &bi, DIB_RGB_COLORS, &bits, NULL, 0)) : NULL;
    HBITMAP oldBitmap = NULL;
    BOOL ret = FALSE;
    if (bitmap != NULL && bits != NULL)
    {
        oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);
        DWORD* pixel = (DWORD*)bits;
        DWORD bkgndPixel = bkgndColor & 0x00ffffff;
        for (int i = 0; i < iconSize * iconSize; i++)
            pixel[i] = bkgndPixel;
        if (DrawIconEx(memDC, 0, 0, hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL))
        {
            thumbMaker->Clear(thumbnailSize);
            if (thumbMaker->SetParameters(iconSize, iconSize, 0))
            {
                thumbMaker->ProcessBuffer(bits, iconSize);
                ret = thumbMaker->ThumbnailReady();
            }
        }
        if (oldBitmap != NULL)
            SelectObject(memDC, oldBitmap);
    }

    if (!ret)
        thumbMaker->Clear();
    if (bitmap != NULL)
        HANDLES(DeleteObject(bitmap));
    if (memDC != NULL)
        HANDLES(DeleteDC(memDC));
    if (screenDC != NULL)
        HANDLES(ReleaseDC(NULL, screenDC));
    HANDLES(DestroyIcon(hIcon));
    return ret;
}

std::string BuildDiskThumbnailPathUtf8(CFilesWindow* window, const char* fileName)
{
    if (window == NULL || fileName == NULL || fileName[0] == 0 || !window->Is(ptDisk))
        return std::string();

    std::wstring fullPath;
    if (window->GetPathW() != NULL && window->GetPathW()[0] != 0)
    {
        fullPath = window->GetPathW();
    }
    else
    {
        fullPath = SalMultiByteToWidePath(window->GetPath(), CP_UTF8);
        if (fullPath.empty())
            fullPath = SalMultiByteToWidePath(window->GetPath(), CP_ACP);
    }
    if (fullPath.empty())
        return std::string();

    const CFileData* file = NULL;
    for (int i = 0; i < window->Files->Count; ++i)
    {
        if (strcmp(window->Files->At(i).Name, fileName) == 0)
        {
            file = &window->Files->At(i);
            break;
        }
    }

    std::wstring nameW;
    if (file != NULL && file->UseWideName())
    {
        nameW = file->NameW;
    }
    else
    {
        nameW = SalMultiByteToWidePath(fileName, CP_UTF8);
        if (nameW.empty())
            nameW = SalMultiByteToWidePath(fileName, CP_ACP);
    }
    if (nameW.empty() || !SalPathAppendW(fullPath, nameW.c_str()))
        return std::string();

    return SalWideToMultiBytePath(fullPath.c_str(), CP_UTF8);
}
} // namespace

struct CTreeViewExpandedPaths
{
    char** Paths;
    int Count;
};

static char* DuplicateTreeViewString(const char* text)
{
    int len = (int)strlen(text) + 1;
    char* copy = (char*)malloc(len);
    if (copy != NULL)
        memcpy(copy, text, len);
    return copy;
}

static void FreeTreeViewNodeData(CTreeViewNodeData* itemData)
{
    if (itemData == NULL)
        return;

    free(itemData->FullPath);
    free(itemData->FocusPath);
    free(itemData->FocusName);
    free(itemData);
}

static BOOL GetTreeViewItemData(HWND hTreeView, HTREEITEM hItem, CTreeViewNodeData* itemData)
{
    if (hTreeView == NULL || hItem == NULL)
        return FALSE;

    TVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(hTreeView, &item))
        return FALSE;
    if (item.lParam == 0)
        return FALSE;

    *itemData = *(CTreeViewNodeData*)item.lParam;
    return TRUE;
}

CTreeViewNodeData* GetTreeViewItemDataPtr(HWND hTreeView, HTREEITEM hItem)
{
    if (hTreeView == NULL || hItem == NULL)
        return NULL;

    TVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(hTreeView, &item) || item.lParam == 0)
        return NULL;

    return (CTreeViewNodeData*)item.lParam;
}

static const char* GetTreeViewItemPath(HWND hTreeView, HTREEITEM hItem)
{
    CTreeViewNodeData itemData;
    if (!GetTreeViewItemData(hTreeView, hItem, &itemData))
        return NULL;

    return itemData.FullPath;
}

static BOOL IsTreeViewDirectoryItem(HWND hTreeView, HTREEITEM hItem)
{
    CTreeViewNodeData itemData;
    if (!GetTreeViewItemData(hTreeView, hItem, &itemData))
        return FALSE;

    return itemData.Type == tvntDirectory;
}

static HTREEITEM FindTreeViewChildByPath(HWND hTreeView, HTREEITEM hParent, const char* path)
{
    HTREEITEM hChild = TreeView_GetChild(hTreeView, hParent);
    while (hChild != NULL)
    {
        const char* childPath = GetTreeViewItemPath(hTreeView, hChild);
        if (childPath != NULL && IsTheSamePath(childPath, path))
            return hChild;
        hChild = TreeView_GetNextSibling(hTreeView, hChild);
    }
    return NULL;
}

static HTREEITEM FindTreeViewItemByPath(HWND hTreeView, HTREEITEM hItem, const char* path)
{
    while (hItem != NULL)
    {
        const char* itemPath = GetTreeViewItemPath(hTreeView, hItem);
        if (itemPath != NULL && IsTheSamePath(itemPath, path))
            return hItem;

        HTREEITEM hFound = FindTreeViewItemByPath(hTreeView, TreeView_GetChild(hTreeView, hItem), path);
        if (hFound != NULL)
            return hFound;
        hItem = TreeView_GetNextSibling(hTreeView, hItem);
    }
    return NULL;
}

void SetTreeViewItemChildren(HWND hTreeView, HTREEITEM hItem, int children)
{
    TVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = TVIF_CHILDREN;
    item.hItem = hItem;
    item.cChildren = children;
    TreeView_SetItem(hTreeView, &item);
}

static BOOL GetTreeViewShellIconIndexes(const char* path, BOOL isDirectory,
                                        int* imageIndex, int* selectedImageIndex,
                                        HIMAGELIST* systemImageList)
{
    SHFILEINFO sfi;
    memset(&sfi, 0, sizeof(sfi));

    DWORD attributes = isDirectory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    UINT flags = SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES;

    HIMAGELIST images = (HIMAGELIST)SHGetFileInfo(path ? path : "", attributes,
                                                  &sfi, sizeof(sfi), flags);
    if (images == NULL)
        return FALSE;

    if (systemImageList != NULL)
        *systemImageList = images;
    *imageIndex = sfi.iIcon;

    if (isDirectory)
    {
        SHFILEINFO selectedSfi;
        memset(&selectedSfi, 0, sizeof(selectedSfi));
        if (SHGetFileInfo(path ? path : "", attributes, &selectedSfi, sizeof(selectedSfi),
                          flags | SHGFI_OPENICON) != 0)
            *selectedImageIndex = selectedSfi.iIcon;
        else
            *selectedImageIndex = *imageIndex;
    }
    else
        *selectedImageIndex = *imageIndex;

    return TRUE;
}

static CTreeViewNodeData* CreateTreeViewNodeData(CTreeViewNodeTypeEnum type, const char* fullPath,
                                                 const char* focusPath, const char* focusName,
                                                 HIMAGELIST* systemImageList)
{
    CTreeViewNodeData* itemData = (CTreeViewNodeData*)malloc(sizeof(CTreeViewNodeData));
    if (itemData == NULL)
        return NULL;
    memset(itemData, 0, sizeof(CTreeViewNodeData));

    itemData->Type = type;
    itemData->FullPath = DuplicateTreeViewString(fullPath);
    itemData->FocusPath = DuplicateTreeViewString(focusPath != NULL ? focusPath : fullPath);
    if (focusName != NULL)
        itemData->FocusName = DuplicateTreeViewString(focusName);

    if (itemData->FullPath == NULL || itemData->FocusPath == NULL || (focusName != NULL && itemData->FocusName == NULL))
    {
        FreeTreeViewNodeData(itemData);
        return NULL;
    }

    if (!GetTreeViewShellIconIndexes(fullPath, type == tvntDirectory,
                                     &itemData->ImageIndex, &itemData->SelectedImageIndex,
                                     systemImageList))
    {
        itemData->ImageIndex = I_IMAGECALLBACK;
        itemData->SelectedImageIndex = I_IMAGECALLBACK;
    }

    return itemData;
}

static void CopyTreeViewImage(HWND hTreeView, HIMAGELIST systemImageList, int imageIndex)
{
    if (hTreeView == NULL || systemImageList == NULL || imageIndex < 0)
        return;

    HIMAGELIST targetImageList = TreeView_GetImageList(hTreeView, TVSIL_NORMAL);
    if (targetImageList == NULL || targetImageList == systemImageList)
        return;

    // Tree View currently contains directories only, so the generic closed
    // and open folder indices repeat for every inserted node.  A present
    // index was already copied into this newly created sparse list.
    if (ImageList_GetImageCount(targetImageList) > imageIndex)
        return;
    if (!ImageList_SetImageCount(targetImageList, imageIndex + 1))
        return;

    HICON icon = ImageList_GetIcon(systemImageList, imageIndex, ILD_TRANSPARENT);
    if (icon != NULL)
    {
        ImageList_ReplaceIcon(targetImageList, imageIndex, icon);
        DestroyIcon(icon);
    }
}

static void RefreshTreeViewImageListAux(HWND hTreeView, HTREEITEM hItem,
                                        HIMAGELIST systemImageList)
{
    while (hItem != NULL)
    {
        CTreeViewNodeData* itemData = GetTreeViewItemDataPtr(hTreeView, hItem);
        if (itemData != NULL)
        {
            CopyTreeViewImage(hTreeView, systemImageList, itemData->ImageIndex);
            if (itemData->SelectedImageIndex != itemData->ImageIndex)
                CopyTreeViewImage(hTreeView, systemImageList, itemData->SelectedImageIndex);
        }
        RefreshTreeViewImageListAux(hTreeView, TreeView_GetChild(hTreeView, hItem), systemImageList);
        hItem = TreeView_GetNextSibling(hTreeView, hItem);
    }
}

void RefreshTreeViewImageList(HWND hTreeView, HIMAGELIST systemImageList)
{
    if (hTreeView != NULL && systemImageList != NULL)
        RefreshTreeViewImageListAux(hTreeView, TreeView_GetRoot(hTreeView), systemImageList);
}

HTREEITEM InsertTreeViewItem(HWND hTreeView, HTREEITEM hParent, const char* text,
                                    CTreeViewNodeTypeEnum type, const char* fullPath,
                                    const char* focusPath, const char* focusName, BOOL hasChildren)
{
    HIMAGELIST systemImageList = NULL;
    CTreeViewNodeData* itemData = CreateTreeViewNodeData(type, fullPath, focusPath, focusName,
                                                        &systemImageList);
    if (itemData == NULL)
        return NULL;

    // The panel owns a DPI-sized sparse image list.  Copy only the shell
    // indices referenced by real tree items instead of cloning the complete
    // process-wide shell list during the first auto-hide expansion.
    CopyTreeViewImage(hTreeView, systemImageList, itemData->ImageIndex);
    if (itemData->SelectedImageIndex != itemData->ImageIndex)
        CopyTreeViewImage(hTreeView, systemImageList, itemData->SelectedImageIndex);

    std::wstring textW = TreeViewTextToWide(text);

    TVINSERTSTRUCTW tvis;
    memset(&tvis, 0, sizeof(tvis));
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = (LPWSTR)textW.c_str();
    tvis.item.lParam = (LPARAM)itemData;
    tvis.item.cChildren = hasChildren ? 1 : 0;
    tvis.item.iImage = itemData->ImageIndex;
    tvis.item.iSelectedImage = itemData->SelectedImageIndex;

    HTREEITEM hItem = (HTREEITEM)SendMessageW(hTreeView, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
    if (hItem == NULL)
        FreeTreeViewNodeData(itemData);
    return hItem;
}

enum
{
    TREEVIEW_MIN_WIDTH = 120,
    TREEVIEW_MIN_LIST_WIDTH = 50,
    TREEVIEW_SPLITTER_WIDTH = 4
};

static int __cdecl CompareTreeViewPopulateEntries(const void* p1, const void* p2)
{
    const CTreeViewPopulateEntry* e1 = (const CTreeViewPopulateEntry*)p1;
    const CTreeViewPopulateEntry* e2 = (const CTreeViewPopulateEntry*)p2;
    return lstrcmpi(e1->Name, e2->Name);
}

static BOOL AddTreeViewPopulateEntry(CTreeViewPopulateEntry** entries, int* count,
                                     const char* name, const char* fullPath, BOOL isDirectory)
{
    char* fullPathCopy = DuplicateTreeViewString(fullPath);
    if (fullPathCopy == NULL)
        return FALSE;

    CTreeViewPopulateEntry* newEntries = (CTreeViewPopulateEntry*)realloc(*entries,
                                                                          (*count + 1) * sizeof(CTreeViewPopulateEntry));
    if (newEntries == NULL)
    {
        free(fullPathCopy);
        return FALSE;
    }

    *entries = newEntries;
    lstrcpyn(newEntries[*count].Name, name, MAX_PATH);
    newEntries[*count].FullPath = fullPathCopy;
    newEntries[*count].IsDirectory = isDirectory;
    (*count)++;
    return TRUE;
}

static void FreeTreeViewPopulateEntries(CTreeViewPopulateEntry* entries, int count)
{
    if (entries != NULL)
    {
        for (int i = 0; i < count; i++)
            free(entries[i].FullPath);
        free(entries);
    }
}

static void FreeTreeViewAsyncLoadData(CTreeViewAsyncLoadData* loadData)
{
    if (loadData == NULL)
        return;

    FreeTreeViewPopulateEntries(loadData->DirEntries, loadData->DirCount);
    free(loadData);
}

static BOOL CopyTreeViewFindDataWToA(const WIN32_FIND_DATAW& src, WIN32_FIND_DATA& dst)
{
    memset(&dst, 0, sizeof(dst));
    dst.dwFileAttributes = src.dwFileAttributes;
    dst.ftCreationTime = src.ftCreationTime;
    dst.ftLastAccessTime = src.ftLastAccessTime;
    dst.ftLastWriteTime = src.ftLastWriteTime;
    dst.nFileSizeHigh = src.nFileSizeHigh;
    dst.nFileSizeLow = src.nFileSizeLow;
    dst.dwReserved0 = src.dwReserved0;
    dst.dwReserved1 = src.dwReserved1;

    std::string fileName = TreeViewWideToText(src.cFileName);
    if (fileName.empty() && src.cFileName[0] != L'\0')
        return FALSE;
    lstrcpyn(dst.cFileName, fileName.c_str(), _countof(dst.cFileName));

    std::string alternateName = TreeViewWideToText(src.cAlternateFileName);
    lstrcpyn(dst.cAlternateFileName, alternateName.c_str(), _countof(dst.cAlternateFileName));
    return TRUE;
}

static BOOL IsTreeViewItemExpanded(HWND hTreeView, HTREEITEM hItem)
{
    TVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = TVIF_STATE;
    item.stateMask = TVIS_EXPANDED;
    item.hItem = hItem;
    return TreeView_GetItem(hTreeView, &item) && (item.state & TVIS_EXPANDED) != 0;
}

static BOOL AddTreeViewExpandedPath(CTreeViewExpandedPaths* expanded, const char* path)
{
    char** paths = (char**)realloc(expanded->Paths, (expanded->Count + 1) * sizeof(char*));
    if (paths == NULL)
        return FALSE;
    expanded->Paths = paths;
    paths[expanded->Count] = DuplicateTreeViewString(path);
    if (paths[expanded->Count] == NULL)
        return FALSE;
    expanded->Count++;
    return TRUE;
}

static void CollectExpandedTreeViewPaths(HWND hTreeView, HTREEITEM hParent, CTreeViewExpandedPaths* expanded)
{
    HTREEITEM hChild = TreeView_GetChild(hTreeView, hParent);
    while (hChild != NULL)
    {
        if (IsTreeViewDirectoryItem(hTreeView, hChild) && IsTreeViewItemExpanded(hTreeView, hChild))
        {
            const char* path = GetTreeViewItemPath(hTreeView, hChild);
            if (path != NULL && AddTreeViewExpandedPath(expanded, path))
                CollectExpandedTreeViewPaths(hTreeView, hChild, expanded);
        }
        hChild = TreeView_GetNextSibling(hTreeView, hChild);
    }
}

static BOOL ContainsExpandedTreeViewPath(const CTreeViewExpandedPaths* expanded, const char* path)
{
    for (int i = 0; i < expanded->Count; i++)
    {
        if (IsTheSamePath(expanded->Paths[i], path))
            return TRUE;
    }
    return FALSE;
}

static void FreeExpandedTreeViewPaths(CTreeViewExpandedPaths* expanded)
{
    for (int i = 0; i < expanded->Count; i++)
        free(expanded->Paths[i]);
    free(expanded->Paths);
    expanded->Paths = NULL;
    expanded->Count = 0;
}

static void RestoreExpandedTreeViewPaths(CFilesWindow* panel, HTREEITEM hParent,
                                         const CTreeViewExpandedPaths* expanded)
{
    HTREEITEM hChild = TreeView_GetChild(panel->HTreeView, hParent);
    while (hChild != NULL)
    {
        const char* path = GetTreeViewItemPath(panel->HTreeView, hChild);
        if (path != NULL && IsTreeViewDirectoryItem(panel->HTreeView, hChild) &&
            ContainsExpandedTreeViewPath(expanded, path))
        {
            panel->PopulateTreeViewItem(hChild);
            TreeView_Expand(panel->HTreeView, hChild, TVE_EXPAND);
            RestoreExpandedTreeViewPaths(panel, hChild, expanded);
        }
        hChild = TreeView_GetNextSibling(panel->HTreeView, hChild);
    }
}

static BOOL ShouldSkipTreeViewEntry(const WIN32_FIND_DATA* findData)
{
    if (strcmp(findData->cFileName, ".") == 0 || strcmp(findData->cFileName, "..") == 0)
        return TRUE;

    int len = (int)strlen(findData->cFileName);
    const char* st = findData->cFileName + len - 1;
    if (Configuration.NotHiddenSystemFiles &&
        !IsFilePlaceholder(findData) &&
        (findData->dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) &&
        (len != 2 || *st != '.' || *(st + 1) != '.'))
        return TRUE;

    return FALSE;
}

static DWORD WINAPI TreeViewAsyncLoadThreadBody(void* param)
{
    CALL_STACK_MESSAGE1("TreeViewAsyncLoadThreadBody()");
    TRACE_I("TreeViewAsyncLoadThread: begin");

    CTreeViewAsyncLoadData* data = (CTreeViewAsyncLoadData*)param;

    data->DirEntries = NULL;
    data->DirCount = 0;
    data->HasChildren = FALSE;

    char searchPath[32768];
    lstrcpyn(searchPath, data->Path, _countof(searchPath));
    if (!SalPathAppend(searchPath, "*", _countof(searchPath)))
    {
        PostMessage(data->HHostWindow, WM_USER_TREEVIEW_ASYNC_DONE, 0, (LPARAM)data);
        TRACE_I("TreeViewAsyncLoadThread: end (path append failed)");
        return 0;
    }

    WIN32_FIND_DATA findData;
    WIN32_FIND_DATAW findDataW;
    std::wstring searchPathW = TreeViewTextToWide(searchPath);
    if (searchPathW.length() >= MAX_PATH)
        searchPathW = SalPathAddExtendedPrefixW(searchPathW.c_str());
    HANDLE find = FindFirstFileExW(searchPathW.c_str(), FindExInfoBasic, &findDataW,
                                   FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (find != INVALID_HANDLE_VALUE && !CopyTreeViewFindDataWToA(findDataW, findData))
    {
        FindClose(find);
        find = INVALID_HANDLE_VALUE;
    }
    if (find == INVALID_HANDLE_VALUE)
    {
        PostMessage(data->HHostWindow, WM_USER_TREEVIEW_ASYNC_DONE, 0, (LPARAM)data);
        TRACE_I("TreeViewAsyncLoadThread: end (find failed)");
        return 0;
    }

    do
    {
        if (data->Cancelled)
            break;

        if (!ShouldSkipTreeViewEntry(&findData))
        {
            BOOL isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            // Only collect directories for the tree view (files are not shown)
            if (isDir)
            {
                char childPath[32768];
                lstrcpyn(childPath, data->Path, _countof(childPath));
                if (SalPathAppend(childPath, findData.cFileName, _countof(childPath)))
                {
                    if (!AddTreeViewPopulateEntry(&data->DirEntries, &data->DirCount,
                                                   findData.cFileName, childPath, TRUE))
                    {
                        data->Cancelled = TRUE;
                        break;
                    }
                }
            }
        }
        if (!FindNextFileW(find, &findDataW) || !CopyTreeViewFindDataWToA(findDataW, findData))
            break;
    } while (TRUE);

    FindClose(find);

    if (!data->Cancelled)
    {
        if (data->DirCount > 1)
            qsort(data->DirEntries, data->DirCount, sizeof(CTreeViewPopulateEntry), CompareTreeViewPopulateEntries);

        data->HasChildren = data->DirCount > 0;
    }

    PostMessage(data->HHostWindow, WM_USER_TREEVIEW_ASYNC_DONE, 0, (LPARAM)data);
    TRACE_I("TreeViewAsyncLoadThread: end");
    return 0;
}

//
// ****************************************************************************
// CFilesWindowAncestor
//

CFilesWindowAncestor::CFilesWindowAncestor()
{
    CALL_STACK_MESSAGE_NONE
    Files = new CFilesArray;
    Dirs = new CFilesArray;
    SelectedCount = 0;

    Path[0] = 0;
    SuppressAutoRefresh = FALSE;
    PanelType = ptDisk;
    MonitorChanges = TRUE;
    DriveType = DRIVE_UNKNOWN;

    ArchiveDir = NULL;
    ZIPArchive[0] = 0;
    ZIPPath[0] = 0;

    PluginFS.Init(NULL, NULL, NULL, NULL, NULL, NULL, -1, 0, 0, 0);
    PluginFSDir = NULL;
    PluginIconsType = pitSimple;
    SimplePluginIcons = NULL;

    char buf[MAX_PATH];
    GetSystemDirectory(buf, MAX_PATH);
    GetRootPath(Path, buf);

    OnlyDetachFSListing = FALSE;
    NewFSFiles = NULL;
    NewFSDirs = NULL;
    NewFSPluginFSDir = NULL;
    NewFSIconCache = NULL;

    PluginIface = NULL;
    PluginIfaceLastIndex = -1;
}

CFilesWindowAncestor::~CFilesWindowAncestor()
{
    CALL_STACK_MESSAGE1("CFilesWindowAncestor::~CFilesWindowAncestor()");
    if (Files != NULL)
        delete Files;
    if (Dirs != NULL)
        delete Dirs;
    if (PluginFS.NotEmpty() || PluginData.NotEmpty() ||
        ArchiveDir != NULL || PluginFSDir != NULL)
    {
        TRACE_E("Unexpected situation in CFilesWindowAncestor::~CFilesWindowAncestor()");
    }
}

DWORD
CFilesWindowAncestor::CheckPath(BOOL echo, const char* path, DWORD err, BOOL postRefresh, HWND parent)
{
    CALL_STACK_MESSAGE5("CFilesWindowAncestor::CheckPath(%d, %s, 0x%X, %d, )", echo, path, err, postRefresh);

    parent = (parent == NULL) ? HWindow : parent;
    if (path == NULL)
        path = GetPath();

    return SalCheckPath(echo, path, err, postRefresh, parent);
}

void CFilesWindowAncestor::ReleaseListing()
{
    CALL_STACK_MESSAGE_NONE

        ((CFilesWindow*)this)
            ->VisibleItemsArray.InvalidateArr();
    ((CFilesWindow*)this)->VisibleItemsArraySurround.InvalidateArr();
    if (OnlyDetachFSListing)
    {
        // disconnect the listing from the panel including icons
        Files = NewFSFiles;
        Dirs = NewFSDirs;
        SetPluginFSDir(NewFSPluginFSDir);
        PluginData.Init(NULL, NULL, NULL, NULL, 0);
        if (NewFSIconCache != NULL)
            ((CFilesWindow*)this)->IconCache = NewFSIconCache;
        ((CFilesWindow*)this)->SetValidFileData(GetPluginFSDir()->GetValidData());

        OnlyDetachFSListing = FALSE;
        NewFSFiles = NULL;
        NewFSDirs = NULL;
        NewFSPluginFSDir = NULL;
        NewFSIconCache = NULL;
    }
    else
    {
        ReleaseListingBody(PanelType, ArchiveDir, PluginFSDir, PluginData, Files, Dirs, FALSE);
    }
    SelectedCount = 0;
}

BOOL CFilesWindowAncestor::IsPathFromActiveFS(const char* fsName, char* fsUserPart, int& fsNameIndex,
                                              BOOL& convertPathToInternal)
{
    CALL_STACK_MESSAGE_NONE
    fsNameIndex = -1;
    if (Is(ptPluginFS) && PluginFS.NotEmpty())
    {
        if (Plugins.AreFSNamesFromSamePlugin(PluginFS.GetPluginFSName(), fsName, fsNameIndex)) // we compare whether the file systems are from the same plug-in
        {
            if (convertPathToInternal)
            {
                PluginFS.GetPluginInterfaceForFS()->ConvertPathToInternal(fsName, fsNameIndex, fsUserPart);
                convertPathToInternal = FALSE;
            }
            return PluginFS.IsOurPath(PluginFS.GetPluginFSNameIndex(), fsNameIndex, fsUserPart);
        }
    }
    return FALSE;
}

BOOL CFilesWindowAncestor::GetGeneralPath(char* buf, int bufSize, BOOL convertFSPathToExternal)
{
    CALL_STACK_MESSAGE_NONE
    if (bufSize == 0)
        return FALSE;
    BOOL ret = TRUE;
    char buf2[2 * MAX_PATH];
    if (Is(ptDisk))
    {
        int l = (int)strlen(GetPath());
        if (l >= bufSize)
        {
            l = bufSize - 1;
            ret = FALSE;
        }
        memcpy(buf, GetPath(), l);
        buf[l] = 0;
    }
    else
    {
        if (Is(ptZIPArchive))
        {
            strcpy(buf2, GetZIPArchive());
            if (GetZIPPath()[0] != 0)
            {
                if (GetZIPPath()[0] != '\\')
                    strcat(buf2, "\\");
                strcat(buf2, GetZIPPath());
            }
            int l = (int)strlen(buf2);
            if (l >= bufSize)
            {
                l = bufSize - 1;
                ret = FALSE;
            }
            memcpy(buf, buf2, l);
            buf[l] = 0;
        }
        else
        {
            if (Is(ptPluginFS))
            {
                strcpy(buf2, PluginFS.GetPluginFSName());
                strcat(buf2, ":");
                char* userPart = buf2 + strlen(buf2);
                if (PluginFS.NotEmpty() && PluginFS.GetCurrentPath(userPart))
                {
                    if (convertFSPathToExternal)
                    {
                        PluginFS.GetPluginInterfaceForFS()->ConvertPathToExternal(PluginFS.GetPluginFSName(),
                                                                                  PluginFS.GetPluginFSNameIndex(),
                                                                                  userPart);
                    }

                    int l = (int)strlen(buf2);
                    if (l >= bufSize)
                    {
                        l = bufSize - 1;
                        ret = FALSE;
                    }
                    memcpy(buf, buf2, l);
                    buf[l] = 0;
                }
                else
                {
                    buf[0] = 0;
                    ret = FALSE;
                }
            }
            else
            {
                buf[0] = 0;
                ret = FALSE;
            }
        }
    }
    return ret;
}

void CFilesWindowAncestor::SetPath(const char* path)
{
    CALL_STACK_MESSAGE2("CFilesWindowAncestor::SetPath(%s)", path);
    BOOL pathChanged = Path[0] == '\0' || !IsTheSamePath(path, Path);
    if (SuppressAutoRefresh && (!Is(ptDisk) || !IsTheSamePath(path, Path)))
        SuppressAutoRefresh = FALSE;
    DetachDirectory((CFilesWindow*)this);
    strcpy(Path, path);
    PathW = PathToWideMirror(path);

    if (MainWindow != NULL)
    {
        MainWindow->UpdatePanelTabTitle((CFilesWindow*)this);
        if (pathChanged)
            Plugins.Event(
                PLUGINEVENT_PATHCHANGED,
                ((CFilesWindow*)this)->GetPanelSide() == cpsRight
                    ? PANEL_RIGHT
                    : PANEL_LEFT);
    }

    //--- zjisteni file-based komprese/sifrovani a FAT32
    DWORD dummy1, flags;
    if ((Is(ptDisk) || Is(ptZIPArchive)) &&
        MyGetVolumeInformation(path, NULL, NULL, NULL, NULL, 0, NULL, &dummy1, &flags, NULL, 0))
    {
        ((CFilesWindow*)this)->FileBasedCompression = (flags & FS_FILE_COMPRESSION) != 0 && Is(ptDisk);
        ((CFilesWindow*)this)->FileBasedEncryption = (flags & FILE_SUPPORTS_ENCRYPTION) != 0 && Is(ptDisk);
        ((CFilesWindow*)this)->SupportACLS = (flags & FS_PERSISTENT_ACLS) != 0 && Is(ptDisk);
    }
    else
    {
        ((CFilesWindow*)this)->FileBasedCompression = FALSE;
        ((CFilesWindow*)this)->FileBasedEncryption = FALSE;
        ((CFilesWindow*)this)->SupportACLS = FALSE;
    }

    MonitorChanges = FALSE;
    DriveType = DRIVE_UNKNOWN;
    if (!Is(ptPluginFS)) // pluginFS handles changes differently...
    {
        DriveType = MyGetDriveType(Path);
        switch (DriveType)
        {
        case DRIVE_REMOVABLE:
        {
            BOOL isDriveFloppy = FALSE; // floppies have their own configuration beside other removable drives
            int drv = UpperCase[Path[0]] - 'A' + 1;
            if (drv >= 1 && drv <= 26) // we perform a range check just to be sure
            {
                DWORD medium = GetDriveFormFactor(drv);
                if (medium == 350 || medium == 525 || medium == 800 || medium == 1)
                    isDriveFloppy = TRUE;
            }
            MonitorChanges = isDriveFloppy ? Configuration.DrvSpecFloppyMon : Configuration.DrvSpecRemovableMon;
            break;
        }

        case DRIVE_REMOTE:
        {
            MonitorChanges = Configuration.DrvSpecRemoteMon;
            break;
        }

        case DRIVE_CDROM:
        {
            MonitorChanges = Configuration.DrvSpecCDROMMon;
            break;
        }

        default: // case DRIVE_FIXED:   // not only fixed drives but also the others (RAM DISK, etc.)
        {
            MonitorChanges = Configuration.DrvSpecFixedMon;
            break;
        }
        }

        // we handle suppression of auto refresh
        if (SuppressAutoRefresh)
            MonitorChanges = FALSE;

        if (MonitorChanges)
            AddDirectory((CFilesWindow*)this, Path, DriveType == DRIVE_REMOVABLE || DriveType == DRIVE_FIXED);
        else // if changes are not monitored, Snooper does not call SetAutomaticRefresh -> we do it here
        {
            ((CFilesWindow*)this)->SetAutomaticRefresh(FALSE, TRUE);
        }
    }
    else // ptPluginFS - do not perform any refreshes; the plug-in manages them itself
    {
        ((CFilesWindow*)this)->SetAutomaticRefresh(TRUE, TRUE);
    }

    // Note: RefreshTreeView() is NOT called here. Callers that need the tree view
    // updated (e.g. ChangePathToDisk) call RefreshTreeView() explicitly after
    // CommonRefresh(), so the file list loads first (responsive UI).
}

void CFilesWindow::RefreshTreeView(BOOL forceRefresh)
{
    CALL_STACK_MESSAGE1("CFilesWindow::RefreshTreeView()");

    if (!IsTreeViewHost() || HTreeView == NULL)
        return;

    if (!TreeViewActive)
    {
        EnableWindow(HTreeView, FALSE);
        return;
    }

    HTREEITEM hRestoreSelected = NULL;

    BOOL hadSelectedFile = FALSE;
    char selectedFileFullPath[32768];
    char selectedFileFocusPath[32768];
    selectedFileFullPath[0] = 0;
    selectedFileFocusPath[0] = 0;
    HTREEITEM hSelected = TreeView_GetSelection(HTreeView);
    if (hSelected != NULL)
    {
        CTreeViewNodeData selectedItemData;
        if (GetTreeViewItemData(HTreeView, hSelected, &selectedItemData) &&
            selectedItemData.Type == tvntFile &&
            selectedItemData.FullPath != NULL && selectedItemData.FocusPath != NULL)
        {
            hadSelectedFile = TRUE;
            lstrcpyn(selectedFileFullPath, selectedItemData.FullPath, _countof(selectedFileFullPath));
            lstrcpyn(selectedFileFocusPath, selectedItemData.FocusPath, _countof(selectedFileFocusPath));
        }
    }

    TreeViewDisableNotify = TRUE;
    SendMessage(HTreeView, WM_SETREDRAW, FALSE, 0);

    do
    {
        CFilesWindow* sourcePanel = GetTreeViewSourcePanel();
        if (sourcePanel == NULL || !sourcePanel->Is(ptDisk))
        {
            EnableWindow(HTreeView, FALSE);
            TreeView_DeleteAllItems(HTreeView);
            break;
        }

        EnableWindow(HTreeView, TRUE);
        UpdateTreeViewColors();

        const char* sourcePath = sourcePanel->GetPath();
        char root[MAX_PATH];
        GetRootPath(root, sourcePath);
        if (root[0] == 0)
        {
            TreeView_DeleteAllItems(HTreeView);
            break;
        }

        HTREEITEM hCurrent = TreeView_GetRoot(HTreeView);
        if (hCurrent == NULL)
            hCurrent = InsertTreeViewItem(HTreeView, TVI_ROOT, root, tvntDirectory, root, root, NULL, TRUE);
        else
        {
            const char* rootPath = GetTreeViewItemPath(HTreeView, hCurrent);
            if (rootPath == NULL || !IsTheSamePath(rootPath, root))
            {
                TreeView_DeleteAllItems(HTreeView);
                hCurrent = InsertTreeViewItem(HTreeView, TVI_ROOT, root, tvntDirectory, root, root, NULL, TRUE);
            }
        }
        if (hCurrent == NULL)
            break;

        CTreeViewNodeData* rootData = GetTreeViewItemDataPtr(HTreeView, hCurrent);
        if (!forceRefresh && rootData != NULL && !rootData->Populated)
        {
            // The first reveal used to enumerate every directory from the drive
            // root to the current path on the UI thread.  Load one level at a
            // time and let WM_USER_TREEVIEW_ASYNC_DONE continue towards the
            // current directory.
            PopulateTreeViewItem(hCurrent, FALSE, TRUE, sourcePath);
            TreeView_SelectItem(HTreeView, hCurrent);
            hRestoreSelected = hCurrent;
            break;
        }

        PopulateTreeViewItem(hCurrent, forceRefresh);
        TreeView_Expand(HTreeView, hCurrent, TVE_EXPAND);

        if (!IsTheSamePath(root, sourcePath))
        {
            char currentPath[32768];
            lstrcpyn(currentPath, root, _countof(currentPath));

            const char* segment = sourcePath + strlen(root);
            while (*segment == '\\' || *segment == '/')
                segment++;

            while (*segment != 0)
            {
                char nextPath[32768];
                lstrcpyn(nextPath, currentPath, _countof(nextPath));

                char name[32768];
                int len = 0;
                while (segment[len] != 0 && segment[len] != '\\' && segment[len] != '/')
                    len++;
                if (len >= _countof(name))
                    break;
                memcpy(name, segment, len);
                name[len] = 0;

                if (!SalPathAppend(nextPath, name, _countof(nextPath)))
                    break;

                HTREEITEM hChild = FindTreeViewChildByPath(HTreeView, hCurrent, nextPath);

                if (hChild == NULL)
                    break;

                hCurrent = hChild;
                lstrcpyn(currentPath, nextPath, _countof(currentPath));
                PopulateTreeViewItem(hCurrent, forceRefresh);
                TreeView_Expand(HTreeView, hCurrent, TVE_EXPAND);

                segment += len;
                while (*segment == '\\' || *segment == '/')
                    segment++;
            }
        }

        PopulateTreeViewItem(hCurrent, forceRefresh);

        HTREEITEM hSelect = hCurrent;
        if (hadSelectedFile && IsTheSamePath(selectedFileFocusPath, sourcePath))
        {
            HTREEITEM hSelectedFile = FindTreeViewChildByPath(HTreeView, hCurrent, selectedFileFullPath);
            if (hSelectedFile != NULL)
                hSelect = hSelectedFile;
        }

        TreeView_SelectItem(HTreeView, hSelect);
        hRestoreSelected = hSelect;
    } while (0);

    SendMessage(HTreeView, WM_SETREDRAW, TRUE, 0);

    // Always scroll so the current directory (selected item) is visible.  This ensures the
    // tree view follows the active directory when the user switches tabs or navigates to a
    // different path — even if both tabs show the same disk.
    if (hRestoreSelected != NULL)
        TreeView_EnsureVisible(HTreeView, hRestoreSelected);
    TreeViewDisableNotify = FALSE;
    RedrawWindow(HTreeView, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
}

BOOL CFilesWindow::PopulateTreeViewItem(HTREEITEM hItem, BOOL forceRefresh, BOOL async,
                                        const char* asyncTargetPath)
{
    CALL_STACK_MESSAGE1("CFilesWindow::PopulateTreeViewItem()");

    CFilesWindow* sourcePanel = GetTreeViewSourcePanel();
    if (HTreeView == NULL || hItem == NULL || sourcePanel == NULL || !sourcePanel->Is(ptDisk))
        return FALSE;

    CTreeViewNodeData* itemData = GetTreeViewItemDataPtr(HTreeView, hItem);
    if (itemData == NULL)
        return FALSE;

    const char* itemPath = GetTreeViewItemPath(HTreeView, hItem);
    if (itemPath == NULL || itemPath[0] == 0)
        return FALSE;
    if (!IsTreeViewDirectoryItem(HTreeView, hItem))
    {
        SetTreeViewItemChildren(HTreeView, hItem, 0);
        return FALSE;
    }

    if (!forceRefresh && itemData->Populated)
        return TreeView_GetChild(HTreeView, hItem) != NULL;

    // Async mode: start background thread instead of blocking UI
    if (async)
    {
        // If an async load is already in progress, don't start another one
        if (TreeViewAsyncLoadData != NULL)
            return FALSE;

        CTreeViewAsyncLoadData* loadData = (CTreeViewAsyncLoadData*)malloc(sizeof(CTreeViewAsyncLoadData));
        if (loadData == NULL)
            return FALSE;

        memset(loadData, 0, sizeof(CTreeViewAsyncLoadData));
        loadData->HHostWindow = HWindow;
        loadData->Panel = this;
        loadData->hParentItem = hItem;
        lstrcpyn(loadData->Path, itemPath, _countof(loadData->Path));
        if (asyncTargetPath != NULL)
            lstrcpyn(loadData->TargetPath, asyncTargetPath, _countof(loadData->TargetPath));
        loadData->Cancelled = FALSE;

        TreeViewAsyncLoadData = loadData;
        ResetEvent(TreeViewAsyncTerminateEvent);

        DWORD threadID;
        TreeViewAsyncLoadThread = HANDLES(CreateThread(NULL, 0, TreeViewAsyncLoadThreadBody,
                                                        loadData, 0, &threadID));
        if (TreeViewAsyncLoadThread == NULL)
        {
            free(loadData);
            TreeViewAsyncLoadData = NULL;
            return FALSE;
        }

        return FALSE; // no children yet, they will be inserted when async load completes
    }

    char searchPath[32768];
    lstrcpyn(searchPath, itemPath, _countof(searchPath));
    if (!SalPathAppend(searchPath, "*", _countof(searchPath)))
        return TreeView_GetChild(HTreeView, hItem) != NULL;

    WIN32_FIND_DATA data;
    WIN32_FIND_DATAW dataW;
    std::wstring searchPathW = TreeViewTextToWide(searchPath);
    if (searchPathW.length() >= MAX_PATH)
        searchPathW = SalPathAddExtendedPrefixW(searchPathW.c_str());
    HANDLE find = FindFirstFileExW(searchPathW.c_str(), FindExInfoBasic, &dataW,
                                   FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (find != INVALID_HANDLE_VALUE && !CopyTreeViewFindDataWToA(dataW, data))
    {
        FindClose(find);
        find = INVALID_HANDLE_VALUE;
    }
    if (find == INVALID_HANDLE_VALUE)
        return TreeView_GetChild(HTreeView, hItem) != NULL;

    CTreeViewPopulateEntry* dirEntries = NULL;
    int dirCount = 0;
    BOOL hasChildren = FALSE;
    do
    {
        if (!ShouldSkipTreeViewEntry(&data))
        {
            BOOL isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (isDirectory) // skip files - tree view only shows directories
            {
                char childPath[32768];
                lstrcpyn(childPath, itemPath, _countof(childPath));
                if (SalPathAppend(childPath, data.cFileName, _countof(childPath)))
                {
                    if (!AddTreeViewPopulateEntry(&dirEntries, &dirCount,
                                                   data.cFileName, childPath, TRUE))
                    {
                        FindClose(find);
                        FreeTreeViewPopulateEntries(dirEntries, dirCount);
                        return TreeView_GetChild(HTreeView, hItem) != NULL;
                    }
                }
            }
        }
        if (!FindNextFileW(find, &dataW) || !CopyTreeViewFindDataWToA(dataW, data))
            break;
    } while (TRUE);

    FindClose(find);

    if (dirCount > 1)
        qsort(dirEntries, dirCount, sizeof(CTreeViewPopulateEntry), CompareTreeViewPopulateEntries);

    CTreeViewExpandedPaths expanded = {NULL, 0};
    if (forceRefresh)
        CollectExpandedTreeViewPaths(HTreeView, hItem, &expanded);

    HTREEITEM hChild = TreeView_GetChild(HTreeView, hItem);
    while (hChild != NULL)
    {
        HTREEITEM hNext = TreeView_GetNextSibling(HTreeView, hChild);
        TreeView_DeleteItem(HTreeView, hChild);
        hChild = hNext;
    }

    int i;
    for (i = 0; i < dirCount; i++)
    {
        if (InsertTreeViewItem(HTreeView, hItem, dirEntries[i].Name, tvntDirectory,
                               dirEntries[i].FullPath, dirEntries[i].FullPath, NULL, TRUE) != NULL)
            hasChildren = TRUE;
    }
    // Note: file entries are not inserted into the tree view. The tree view only shows
    // directories for performance (avoids SHGetFileInfo + InsertTreeViewItem per file).

    if (expanded.Count > 0)
        RestoreExpandedTreeViewPaths(this, hItem, &expanded);
    FreeExpandedTreeViewPaths(&expanded);

    FreeTreeViewPopulateEntries(dirEntries, dirCount);

    itemData->Populated = TRUE;
    SetTreeViewItemChildren(HTreeView, hItem, hasChildren ? 1 : 0);
    return hasChildren;
}

CFilesArray*
CFilesWindowAncestor::GetArchiveDirFiles(const char* zipPath)
{
    CALL_STACK_MESSAGE_NONE
    if (zipPath == NULL)
        zipPath = ZIPPath;
    return ArchiveDir->GetFiles(zipPath);
}

CFilesArray*
CFilesWindowAncestor::GetArchiveDirDirs(const char* zipPath)
{
    CALL_STACK_MESSAGE_NONE
    if (zipPath == NULL)
        zipPath = ZIPPath;
    return ArchiveDir->GetDirs(zipPath);
}

CFilesArray*
CFilesWindowAncestor::GetFSFiles()
{
    CALL_STACK_MESSAGE_NONE
    return PluginFSDir->GetFiles("");
}

CFilesArray*
CFilesWindowAncestor::GetFSDirs()
{
    CALL_STACK_MESSAGE_NONE
    return PluginFSDir->GetDirs("");
}

CPluginData*
CFilesWindowAncestor::GetPluginDataForPluginIface()
{
    return Plugins.GetPluginData(PluginIface, &PluginIfaceLastIndex);
}

void CFilesWindowAncestor::SetZIPPath(const char* path)
{
    CALL_STACK_MESSAGE_NONE
    if (*path == '\\')
        path++; // ZIPPath will not start with '\\'
    int l = (int)strlen(path);
    if (l > 0 && path[l - 1] == '\\')
        l--; // ZIPPath will not end with '\\'
    if (l >= SAL_MAX_PATH)
        l = SAL_MAX_PATH - 1;
    memcpy(ZIPPath, path, l);
    ZIPPath[l] = 0;
}

void CFilesWindowAncestor::SetZIPArchive(const char* archive)
{
    CALL_STACK_MESSAGE_NONE
    lstrcpyn(ZIPArchive, archive, SAL_MAX_PATH);
}

BOOL CFilesWindowAncestor::SamePath(CFilesWindowAncestor* other)
{
    CALL_STACK_MESSAGE_NONE
    int l1 = (int)strlen(Path);
    if (l1 > 0 && Path[l1 - 1] == '\\')
        l1--;
    int l2 = (int)strlen(other->Path);
    if (l2 > 0 && other->Path[l2 - 1] == '\\')
        l2--;
    return (PanelType == ptDisk || PanelType == ptZIPArchive) &&
           (other->PanelType == ptDisk || other->PanelType == ptZIPArchive) &&
           l1 == l2 && StrNICmp(Path, other->Path, l1) == 0;
}

//
// ****************************************************************************
// CFilesWindow
//

void IconThreadThreadFBodyAux(const char* path, SHFILEINFO& shi, CIconSizeEnum iconSize)
{
    CALL_STACK_MESSAGE_NONE
    __try
    {
        // do not let a default icon be returned; if it fails, simple icons are used
        if (!GetFileIcon(path, FALSE, &shi.hIcon, iconSize, FALSE, FALSE))
            shi.hIcon = NULL;

        // We switched to our own implementation (lower memory usage, working XOR icons)
        // Additionally it does not support obtaining EXTRALARGE and JUMBO icons; accessing the system image list is required
        //SHGetFileInfo(path, 0, &shi, sizeof(shi),
        //              SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SHELLICONSIZE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Shell icon handlers run in-process and some third-party handlers are
        // known to throw SEH exceptions while extracting icons.  This icon
        // reader already has a safe fallback path (simple/default icons), so do
        // not route these exceptions through CCallStack::HandleException(): that
        // handler intentionally terminates the process after writing a bug
        // report.  Keep Salamander alive and mark this icon as unavailable.
        TRACE_E("Shell icon handler failed while getting an icon for: " << path);
        FGIExceptionHasOccured++;
        shi.hIcon = NULL;
    }
}

unsigned IconThreadThreadFBody(void* parameter)
{
    CALL_STACK_MESSAGE1("IconThreadThreadFBody()");

    SetThreadNameInVCAndTrace("IconsReader");
    TRACE_I("Begin");
    CFilesWindow* window = (CFilesWindow*)parameter;

    // let shell extensions that retrieve icons via IconHandler and other COM/OLE stuff work correctly
    if (OleInitialize(NULL) != S_OK)
        TRACE_E("Error in OleInitialize.");

    IShellIconOverlayIdentifier** iconReadersIconOverlayIds = ShellIconOverlays.CreateIconReadersIconOverlayIds();

    HANDLE handles[2];
    handles[0] = window->ICEventTerminate;
    handles[1] = window->ICEventWork;
    DWORD wait = WAIT_TIMEOUT;
    BOOL run = TRUE;
    BOOL firstRound = TRUE; // on error a REFRESH is sent, but only the first time

    CSalamanderThumbnailMaker thumbMaker(window);

    while (run)
    {
        if (wait == WAIT_TIMEOUT) // otherwise wait is already set from work mode
            wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

        switch (wait)
        {
        case WAIT_OBJECT_0 + 1: // work
        {
            CALL_STACK_MESSAGE1("IconThreadThreadFBody::work");
            window->IconCacheValid = FALSE; // required for refreshes when the icon reader sleeps or wakes up; otherwise the main thread sets it

            // j.r. the original 200 ms delay was probably too long, reduced to 20 ms
            // j.r. 20 ms was still short; the thread could start when Enter was held
            // Petr: the main thread repaints with higher priority; with this sleep here
            //       icon overlays (e.g., Tortoise SVN) flickered even more than they do now
            // give the main thread some time to draw and to quickly interrupt when changing directories
            // (now used only as a "pause" during which RefreshDirectory() can push new icons into the cache, see 'WaitBeforeReadingIcons')
            if (window->WaitBeforeReadingIcons > 0)
                Sleep(window->WaitBeforeReadingIcons);
            if (window->WaitOneTimeBeforeReadingIcons > 0)
            {
                DWORD time = window->WaitOneTimeBeforeReadingIcons;
                window->WaitOneTimeBeforeReadingIcons = 0;
                Sleep(time); // wait before starting to read icon overlays; during this wait all notifications about changes from Tortoise SVN should arrive (see IconOverlaysChangedOnPath())
            }

            HANDLES(EnterCriticalSection(&window->ICSleepSection));

            // should we start new work (wake-up -> sleep -> wake-up) or terminate?
            wait = WaitForMultipleObjects(2, handles, FALSE, 0);

            //        BOOL postRefresh = FALSE;
            if (wait == WAIT_TIMEOUT && !window->ICStopWork)
            {
                //          TRACE_I("Start reading.");
                window->ICWorking = TRUE;

                CIconSizeEnum iconSize = window->GetIconSizeForCurrentViewMode();
                int iconPixelSize = window->GetIconSize(iconSize);
                int iconDPI = window->GetWindowDPI();

                CIconList* iconList;
                int iconListIndex;
                SHFILEINFO shi; // for historical reasons (SHGetFileInfo) shi.hIcon is used for all icon types

                // prepare the full path for files/directories being loaded (only when window->Is(ptDisk))
                char path[MAX_PATH + 10];
                path[0] = 0;
                WCHAR wPath[MAX_PATH + 10];
                wPath[0] = 0;
                char* name = path;
                WCHAR* wName = wPath;
                BOOL pathIsInvalid = FALSE;
                BOOL isGoogleDrivePath = FALSE;
                if (window->Is(ptDisk))
                {
                    int l = (int)strlen(window->GetPath());
                    memmove(path, window->GetPath(), l);
                    if (path[l - 1] != '\\')
                        path[l++] = '\\';
                    name = path + l; // pointer to the location of the name in the full path
                    *name = 0;
                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path, l, wPath, MAX_PATH + 10);
                    wName = wPath + l;
                    *wName = 0;
                    pathIsInvalid = !PathContainsValidComponents(path, FALSE);
                    if (pathIsInvalid)
                        TRACE_I("Path contains invalid components, shell cannot read icons from such paths! Path: " << path);
                    isGoogleDrivePath = ShellIconOverlays.IsGoogleDrivePath(path);
                }

                BOOL readOnlyVisibleItems = window->InactWinOptimizedReading; // refreshes from the snooper in an inactive window: read only visible icons/thumbnails/overlays to save CPU time (we are in the background)
                                                                              //          if (readOnlyVisibleItems) TRACE_I("Refresh in inactive window, reading only visible icons...");
                BOOL readOnlyVisibleItemsDueToUMI = FALSE;                    // description below
                if (!readOnlyVisibleItems && UserMenuIconBkgndReader.IsReadingIcons())
                {
                    //            TRACE_I("Reading of usermenu icons is in progress, reading only visible icons...");
                    readOnlyVisibleItems = TRUE;
                    readOnlyVisibleItemsDueToUMI = TRUE;
                }

                BOOL readThumbnails = window->UseThumbnails; // should we try to load thumbnails?

                if (window->StopThumbnailLoading)
                    readThumbnails = FALSE; // unwanted wake-up - at least suppress thumbnail loading

                BOOL pluginFSIconsFromPlugin = window->Is(ptPluginFS) &&
                                               window->GetPluginIconsType() == pitFromPlugin;
                BOOL pluginFSIconsFromRegistry = window->Is(ptPluginFS) &&
                                                 window->GetPluginIconsType() == pitFromRegistry;

                BOOL waitBeforeFirstReadIcon = FALSE; // TRUE only when jumping to SECOND_ROUND:
                BOOL repeatedRound = FALSE;           // TRUE when icons/thumbnails are reloaded because user-menu icon reading is still in progress

            SECOND_ROUND: // if some icon cannot be read from disk, a second attempt is made at the end

                DWORD wanted = -1;                                 // invalid -> does nothing and then sleeps
                if (window->Is(ptDisk) || pluginFSIconsFromPlugin) // disk + FS/icons-from-plugin
                {
                    wanted = 0; // first load new icons and only then the old ones
                }
                else
                {
                    if (window->Is(ptZIPArchive) || pluginFSIconsFromRegistry) // archive + FS/icons-from-registry
                    {
                        wanted = 3; // our icons are determined by their icon location
                    }
                    else
                        TRACE_E("Unexpected situation.");
                }
                // before starting set "ReadingDone" of all icon-cache items and "IconOverlayDone" of all panel items to FALSE
                int x;
                if (!repeatedRound)
                    for (x = 0; x < window->IconCache->Count; x++)
                        window->IconCache->At(x).SetReadingDone(0);
                if (firstRound && !repeatedRound)
                {
                    for (x = 0; x < window->Files->Count; x++)
                        window->Files->At(x).IconOverlayDone = 0;
                    for (x = 0; x < window->Dirs->Count; x++)
                        window->Dirs->At(x).IconOverlayDone = 0;
                }

                BOOL failed = FALSE;
                BOOL destroyPluginIcon = TRUE;

                int selectMode = 1;
                // 1 = sequential traversal (VisibleItemsArray.IsArrValid() == FALSE),
                // 2 = traversal according to VisibleItemsArray,
                // 3 = traversal according to VisibleItemsArraySurround,
                // 4 = sequential traversal (VisibleItemsArray.IsArrValid() == TRUE)

                BOOL canReadIconOverlays = firstRound && window->Is(ptDisk) && iconReadersIconOverlayIds != NULL;
                BOOL readIconOverlaysNow = FALSE; // TRUE = reading overlays now, FALSE = reading icons + thumbnails

                //          TRACE_I("wanted=" << wanted << ", selectMode=" << selectMode);

                int lastVisArrVersion = -1;
                BOOL someNameSkipped = FALSE;
                int thumbnailFlag = 0;
                int i = 0;
                while (1)
                {
                    BOOL callWaitForObjects = TRUE;                                                                        // optimization only - while searching for an item (takes almost no time) WaitForMultipleObjects is not called
                    if (i < (readIconOverlaysNow ? window->Files->Count + window->Dirs->Count : window->IconCache->Count)) // loading an icon from a file/directory or retrieving icon overlay for a file/directory
                    {
                        CIconData* iconData = readIconOverlaysNow ? NULL : &window->IconCache->At(i);

                        BOOL skipName = FALSE;
                        if (selectMode == 1)
                        {
                            int visArrVer;
                            if (window->VisibleItemsArray.IsArrValid(&visArrVer))
                            {
                                i = 0;
                                lastVisArrVersion = visArrVer;
                                selectMode = 2;
                                //                  TRACE_I("selectMode=" << selectMode);
                                readIconOverlaysNow = FALSE;
                                //                  TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                                continue;
                            }
                        }
                        else
                        {
                            if (selectMode == 2 || selectMode == 3)
                            {
                                int visArrVer;
                                BOOL visArrValid;
                                BOOL cont;
                                if (selectMode == 2)
                                {
                                    if (readIconOverlaysNow)
                                        cont = window->VisibleItemsArray.ArrContainsIndex(i, &visArrValid, &visArrVer);
                                    else
                                        cont = window->VisibleItemsArray.ArrContains(iconData->NameAndData,
                                                                                     &visArrValid, &visArrVer);
                                }
                                else
                                {
                                    if (readIconOverlaysNow)
                                        cont = window->VisibleItemsArraySurround.ArrContainsIndex(i, &visArrValid, &visArrVer);
                                    else
                                        cont = window->VisibleItemsArraySurround.ArrContains(iconData->NameAndData,
                                                                                             &visArrValid, &visArrVer);
                                }
                                if (!cont && visArrValid && visArrVer == lastVisArrVersion)
                                    skipName = TRUE;
                                else
                                {
                                    if (!visArrValid)
                                    {
                                        i = 0;
                                        selectMode = 1;
                                        //                      TRACE_I("selectMode=" << selectMode);
                                        readIconOverlaysNow = FALSE;
                                        //                      TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                                        continue;
                                    }
                                    else
                                    {
                                        if (visArrVer != lastVisArrVersion)
                                        {
                                            i = 0;
                                            lastVisArrVersion = visArrVer;
                                            selectMode = 2;
                                            //                        TRACE_I("selectMode=" << selectMode);
                                            readIconOverlaysNow = FALSE;
                                            //                        TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                                            continue;
                                        }
                                    }
                                }
                            }
                            else // selectMode == 4
                            {
                                int visArrVer;
                                if (window->VisibleItemsArray.IsArrValid(&visArrVer) && visArrVer != lastVisArrVersion)
                                {
                                    i = 0;
                                    lastVisArrVersion = visArrVer;
                                    selectMode = 2;
                                    //                    TRACE_I("selectMode=" << selectMode);
                                    readIconOverlaysNow = FALSE;
                                    //                    TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                                    continue;
                                }
                            }
                        }

                        if (!skipName)
                        {
                            if (readIconOverlaysNow) // new icons/thumbnails for the selected area (see 'selectMode') are loaded, now we read icon overlays
                            {
                                CFileData* fileData = i < window->Dirs->Count ? &window->Dirs->At(i) : &window->Files->At(i - window->Dirs->Count);
                                if (fileData->IconOverlayDone == 0 && (i > 0 || strcmp(fileData->Name, "..") != 0))
                                {
                                    fileData->IconOverlayDone = 1; // mark that this overlay was already retrieved so we don't repeat it in this cycle

                                    char fileName[MAX_PATH];
                                    DWORD fileAttrs = fileData->Attr;
                                    memcpy(fileName, fileData->Name, fileData->NameLen + 1);
                                    int minPriority = 100;
                                    if (i >= window->Dirs->Count && fileData->IsLink || // file is a link
                                        fileData->IsOffline ||                          // file or directory is offline (slow)
                                        i < window->Dirs->Count && fileData->Shared)    // directory is shared
                                    {
                                        minPriority = 9; // overlays for links, shares and slow files (offline) have priority 10, so we take only overlays with a higher priority (numerically lower than 10)
                                    }

                                    if (window->ICSleep)
                                        goto GO_SLEEP_MODE;
                                    HANDLES(LeaveCriticalSection(&window->ICSleepSection));

                                    // let the icon be loaded from the file; the icon reader may enter sleep mode during loading
                                    *name = 0;
                                    //                    TRACE_I("Getting icon overlay index for: " << fileName << "...");
                                    SLOW_CALL_STACK_MESSAGE5("IconThreadThreadFBody::GetIconOverlayIndex(%s%s, 0x%08X, %d)",
                                                             path, fileName, fileAttrs, isGoogleDrivePath);
                                    DWORD iconOverlayIndex = ShellIconOverlays.GetIconOverlayIndex(wPath, wName, path, name,
                                                                                                   fileName, fileAttrs,
                                                                                                   minPriority, iconReadersIconOverlayIds,
                                                                                                   isGoogleDrivePath);
                                    //                    TRACE_I("Getting icon overlay index is done.");

                                    HANDLES(EnterCriticalSection(&window->ICSleepSection));
                                    if (window->ICSleep)
                                        goto GO_SLEEP_MODE; // panel already wants to switch to sleep mode

                                    CFileData* fileDataCheck = i < window->Dirs->Count ? &window->Dirs->At(i) : i < window->Files->Count + window->Dirs->Count ? &window->Files->At(i - window->Dirs->Count)
                                                                                                                                                               : NULL;
                                    if (fileData != fileDataCheck || strcmp(fileName, fileData->Name) != 0)
                                    {
                                        if (fileData != fileDataCheck)
                                            TRACE_E("IconThreadThreadFBody::GetIconOverlayIndex: PRUSER!!! (fileData != fileDataCheck)");
                                        else
                                            TRACE_E("IconThreadThreadFBody::GetIconOverlayIndex: PRUSER!!! (file name changed)");
                                    }
                                    else
                                    {
                                        BOOL redraw = fileData->IconOverlayIndex != iconOverlayIndex;
                                        fileData->IconOverlayIndex = iconOverlayIndex;

                                        int visArrVer;
                                        BOOL visArrValid;
                                        if (redraw && // the index needs to be redrawn (icon overlay changed)
                                            (window->VisibleItemsArray.ArrContainsIndex(i, &visArrValid, &visArrVer) || !visArrValid))
                                        { // if we know the item is visible or if visibility is unknown, let the index be redrawn
                                            PostMessage(window->HWindow, WM_USER_REFRESHINDEX2, i, 0);
                                        }
                                    }
                                }
                                else
                                    callWaitForObjects = FALSE; // no work -> no waiting
                            }
                            else
                            {
                                if (iconData->GetReadingDone() == 0 &&
                                    iconData->GetFlag() == wanted)
                                {
                                    iconData->SetReadingDone(1);    // mark that we have already worked with this icon so we do not try again during this cycle
                                    if (wanted == 0 || wanted == 2) // loading icons directly from a file or from a plug-in
                                    {
                                        if (!pluginFSIconsFromPlugin) // icon on disk
                                        {
                                            if (strlen(iconData->NameAndData) + (name - path) < MAX_PATH)
                                            {
                                                strcpy(name, iconData->NameAndData);

                                                if (window->ICSleep)
                                                    goto GO_SLEEP_MODE;
                                                HANDLES(LeaveCriticalSection(&window->ICSleepSection));

                                                if (waitBeforeFirstReadIcon)
                                                {
                                                    waitBeforeFirstReadIcon = FALSE;
                                                    //                            TRACE_I("Waiting 500ms before reading first icon in second round to have bigger chance to succeed.");
                                                    Sleep(500); // let's pause for a moment (before the second attempt to load the icon)
                                                }

                                                // let the icon be loaded from the file; the icon reader may enter sleep mode during loading
                                                CALL_STACK_MESSAGE3("IconThreadThreadFBody::GetFileIcon(%s, %d)", path, iconSize);

                                                if (!pathIsInvalid)
                                                {
                                                    //                            TRACE_I("Getting icon for: " << name << "...");
                                                    IconThreadThreadFBodyAux(path, shi, iconSize);
                                                    if (shi.hIcon == NULL)
                                                        TRACE_I("Unable to get icon from: " << path);
                                                    //                            else
                                                    //                              TRACE_I("Getting icon is done.");
                                                }
                                                else
                                                {
                                                    shi.hIcon = NULL;
                                                }

                                                HANDLES(EnterCriticalSection(&window->ICSleepSection));
                                            }
                                            else
                                            {
                                                shi.hIcon = NULL;
                                                *name = 0;
                                                TRACE_I("Too long filename to get icon from: " << path << iconData->NameAndData);
                                            }
                                        }
                                        else // icon in a plug-in FS - reading cannot be interrupted (risk of PluginData being destroyed)
                                        {
                                            const CFileData* f = iconData->GetFSFileData();
                                            if (f != NULL)
                                            {
                                                shi.hIcon = window->PluginData.GetPluginIcon(f, iconSize, destroyPluginIcon);
                                                if (shi.hIcon == NULL)
                                                {
                                                    TRACE_I("Unable to get icon from FS item: " << iconData->NameAndData);
                                                }
                                            }
                                            else
                                            {
                                                shi.hIcon = NULL;
                                                TRACE_E("Unexpected error: Icon Cache doesn't contain FSFileData for item from FS with "
                                                        "pitFromPlugin icon type! Item: "
                                                        << iconData->NameAndData);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (wanted == 3) // loading icons from the icon-location
                                        {
                                            shi.hIcon = NULL;
                                            char* nameAndData = iconData->NameAndData;
                                            int size = (int)strlen(nameAndData) + 4;
                                            size -= (size & 0x3);         // size % 4 (alignment to four bytes)
                                            char* s = nameAndData + size; // skip the alignment zeros
                                            BOOL doExtractIcons = FALSE;
                                            BOOL doLoadImage = FALSE;
                                            int index = -1;
                                            char* num = strrchr(s, ','); // icon index follows the last comma
                                            if (num != NULL)
                                            {
                                                *num = 0;
                                                index = atoi(num + 1);
                                                if (strlen(s) < MAX_PATH)
                                                {
                                                    strcpy(path, s);
                                                    doExtractIcons = TRUE;
                                                    //                            TRACE_I("ExtractIcons for: " << nameAndData << "...");
                                                }
                                                else
                                                    TRACE_I("Too long filename to get icon from: " << s << ", " << index);
                                                *num = ',';
                                            }
                                            else
                                            {
                                                if (strlen(s) < MAX_PATH)
                                                {
                                                    strcpy(path, s);
                                                    doLoadImage = TRUE;
                                                    //                            TRACE_I("LoadImage for: " << nameAndData << "...");
                                                }
                                                else
                                                    TRACE_I("Too long filename to get icon from: " << s);
                                            }

                                            if (window->ICSleep)
                                                goto GO_SLEEP_MODE;
                                            HANDLES(LeaveCriticalSection(&window->ICSleepSection));

                                            if (waitBeforeFirstReadIcon)
                                            {
                                                waitBeforeFirstReadIcon = FALSE;
                                                //                          TRACE_I("Waiting 500ms before reading first icon in second round to have bigger chance to succeed.");
                                                Sleep(500); // take a short break before the second attempt to load the icon
                                            }

                                            if (doExtractIcons)
                                            {
                                                // load the icon from the file (ExtractIcons retrieves it by index);
                                                // the icon reader may go to sleep mode while loading
                                                CALL_STACK_MESSAGE4("IconThreadThreadFBody::ExtractIcons(%s, %d, %d, ...)", path, index, window->GetIconSize(iconSize));
                                                if (ExtractIcons(path, index, window->GetIconSize(iconSize), window->GetIconSize(iconSize), &shi.hIcon, NULL, 1, IconLRFlags) != 1)
                                                {
                                                    TRACE_I("Unable to get icon from: " << path << ", " << index);
                                                    shi.hIcon = NULL;
                                                }
                                                //                          else
                                                //                            TRACE_I("ExtractIcons is done.");
                                            }

                                            if (doLoadImage)
                                            {
                                                {
                                                    // load the icon from a file (likely .ico); the icon reader can switch to sleep mode during loading
                                                    CALL_STACK_MESSAGE2("IconThreadThreadFBody::LoadImage(%s)", path);
                                                    shi.hIcon = (HICON)NOHANDLES(LoadImage(NULL, path, IMAGE_ICON, window->GetIconSize(iconSize), window->GetIconSize(iconSize),
                                                                                           LR_LOADFROMFILE | IconLRFlags));
                                                    //                            TRACE_I("LoadImage " << (shi.hIcon == NULL ? "has failed, now trying ExtractIcons..." : "is done."));
                                                }
                                                if (shi.hIcon == NULL) // LoadImage failed; trying ExtractIcons as well (e.g., an icon without index from zipfldr.dll on XP: a .zip archive packed in a .7z archive)
                                                {
                                                    // let the first icon load from the file; the icon reader may enter sleep mode while loading
                                                    CALL_STACK_MESSAGE3("IconThreadThreadFBody::ExtractIcons(%s, (0), %d, ...)", path, window->GetIconSize(iconSize));
                                                    if (ExtractIcons(path, 0, window->GetIconSize(iconSize), window->GetIconSize(iconSize), &shi.hIcon, NULL, 1, IconLRFlags) != 1)
                                                    {
                                                        TRACE_I("Unable to get first icon from: " << path);
                                                        shi.hIcon = NULL;
                                                    }
                                                    //                            else
                                                    //                              TRACE_I("ExtractIcons is done.");
                                                }
                                            }

                                            HANDLES(EnterCriticalSection(&window->ICSleepSection));
                                        }
                                        else // wanted == 4 or 6; loading thumbnails from a plug-in ("thumbnail loader")
                                        {
                                            shi.hIcon = NULL; // precaution against incorrect icon deallocation (none is created here)

                                            char* s = iconData->NameAndData;
                                            int len = (int)strlen(s);
                                            int size = len + 4;
                                            size -= (size & 0x3); // size % 4 (alignment to four bytes)
                                            int thumbnailSize = window->GetThumbnailSize();
                                            BOOL thumbnailLoaded = FALSE;

                                            if (strlen(s) + (name - path) < MAX_PATH)
                                            {
                                                strcpy(name, s);
                                                if (LoadIcoThumbnail(path, thumbnailSize, GetCOLORREF(CurrentColors[ITEM_BK_NORMAL]), &thumbMaker))
                                                {
                                                    thumbnailFlag = 5;
                                                    thumbnailLoaded = TRUE;
                                                }
                                            }

                                            if (!thumbnailLoaded)
                                            {
                                                std::string thumbnailPathUtf8 = BuildDiskThumbnailPathUtf8(window, s);
                                                const char* thumbnailPath = NULL;
                                                if (!thumbnailPathUtf8.empty())
                                                {
                                                    thumbnailPath = thumbnailPathUtf8.c_str();
                                                }
                                                else if (strlen(s) + (name - path) < MAX_PATH)
                                                {
                                                    strcpy(name, s);
                                                    thumbnailPath = path;
                                                }

                                                if (thumbnailPath != NULL)
                                                {
                                                    //                          TRACE_I("Load thumbnail for: " << name << "...");
                                                    CPluginInterfaceForThumbLoaderEncapsulation** loader;
                                                    loader = (CPluginInterfaceForThumbLoaderEncapsulation**)(s + size + sizeof(CQuadWord) + sizeof(FILETIME));
                                                    while (*loader != NULL)
                                                    {
                                                        thumbMaker.Clear(thumbnailSize);
                                                        CALL_STACK_MESSAGE3("IconThreadThreadFBody::LoadThumbnail(%s, %d)", thumbnailPath, wanted == 4);
                                                        if ((*loader)->LoadThumbnail(thumbnailPath, thumbnailSize, thumbnailSize, &thumbMaker, wanted == 4))
                                                        {
                                                            thumbnailFlag = wanted == 4 /* first thumbnail loading round */ ? (thumbMaker.IsOnlyPreview() ? 6 /* low-quality/smaller */ : 5 /* quality */) : 5 /* in the second round all obtained thumbnails are quality */;
                                                            thumbMaker.HandleIncompleteImages();
                                                            break; // the thumbnail may be loaded; do not try another plug-in
                                                        }
                                                        loader++; // try the next plug-in in line, it might load the thumbnail
                                                    }
                                                    if (*loader == NULL)
                                                        thumbMaker.Clear(); // failed thumbnail -> clean it up
                                                                            //                          TRACE_I("Load thumbnail is done.");
                                                }
                                            }
                                        }
                                    }

                                    if (window->ICSleep) // the panel wants to switch to sleep mode
                                    {
                                        thumbMaker.Clear(); // the thumbnail will no longer be needed

                                        // if this is not an icon from a plug-in that forbids icon destruction, destroy it
                                        if (shi.hIcon != NULL && (!pluginFSIconsFromPlugin || destroyPluginIcon))
                                        {
                                            ::NOHANDLES(DestroyIcon(shi.hIcon));
                                        }
                                        goto GO_SLEEP_MODE;
                                    }

                                    if (wanted <= 3) // we were obtaining an icon
                                    {
                                        if (shi.hIcon == NULL)
                                            failed = TRUE;
                                        else if (iconPixelSize != window->GetIconSize(iconSize) || iconDPI != window->GetWindowDPI())
                                        {
                                            // The icon was extracted while the panel was still using an old DPI.
                                            // Do not let a delayed 24px read populate a freshly rebuilt 16px cache
                                            // after moving from a high-DPI monitor to a 100% monitor.
                                            HANDLES(DestroyIcon(shi.hIcon));
                                            shi.hIcon = NULL;
                                            failed = TRUE;
                                        }
                                        else
                                        {
                                            if (window->IconCache->GetIcon(iconData->GetIndex(),
                                                                           &iconList, &iconListIndex))
                                            {
                                                HANDLES(EnterCriticalSection(&window->ICSectionUsingIcon));

                                                iconList->ReplaceIcon(iconListIndex, shi.hIcon);
                                                iconData->SetFlag(1); // already loaded

                                                HANDLES(LeaveCriticalSection(&window->ICSectionUsingIcon));

                                                // find the index of the item for which we loaded the icon

                                                if (pluginFSIconsFromPlugin) // pitFromPlugin: let the plug-in compare items itself (must compare with no duplicates)
                                                {
                                                    const CFileData* file = iconData->GetFSFileData();
                                                    if (file != NULL)
                                                    {
                                                        CPluginDataInterfaceEncapsulation* dataIface = &window->PluginData;
                                                        CFilesArray* arr = window->Dirs;
                                                        int z;
                                                        for (z = 0; z < arr->Count; z++)
                                                        {
                                                            if (dataIface->CompareFilesFromFS(file, &arr->At(z)) == 0)
                                                            {
                                                                PostMessage(window->HWindow, WM_USER_REFRESHINDEX, z, 0);
                                                                break;
                                                            }
                                                        }
                                                        if (z == window->Dirs->Count) // it was not a directory
                                                        {
                                                            arr = window->Files;
                                                            int j;
                                                            for (j = 0; j < arr->Count; j++)
                                                            {
                                                                if (dataIface->CompareFilesFromFS(file, &arr->At(j)) == 0)
                                                                {
                                                                    PostMessage(window->HWindow, WM_USER_REFRESHINDEX,
                                                                                window->Dirs->Count + j, 0);
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                else // duplicate names are not a problem (e.g., archives where identical names cannot have different icons)
                                                {
                                                    char* name2 = iconData->NameAndData;
                                                    CFilesArray* arr = window->Dirs;
                                                    int z;
                                                    for (z = 0; z < arr->Count; z++)
                                                    {
                                                        if (strcmp(name2, arr->At(z).Name) == 0)
                                                        {
                                                            PostMessage(window->HWindow, WM_USER_REFRESHINDEX, z, 0);
                                                            break;
                                                        }
                                                    }
                                                    if (z == window->Dirs->Count) // it was not a directory
                                                    {
                                                        arr = window->Files;
                                                        int j;
                                                        for (j = 0; j < arr->Count; j++)
                                                        {
                                                            if (strcmp(name2, arr->At(j).Name) == 0)
                                                            {
                                                                PostMessage(window->HWindow, WM_USER_REFRESHINDEX,
                                                                            window->Dirs->Count + j, 0);
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            // if this is not an icon from a plug-in that forbids icon destruction, destroy it
                                            if (!pluginFSIconsFromPlugin || destroyPluginIcon)
                                            {
                                                ::NOHANDLES(DestroyIcon(shi.hIcon));
                                            }
                                        }
                                    }
                                    else // we were obtaining a thumbnail
                                    {
                                        if (thumbMaker.ThumbnailReady())
                                        {
                                            CThumbnailData* thumbnailData;
                                            if (window->IconCache->GetThumbnail(iconData->GetIndex(),
                                                                                &thumbnailData))
                                            {
                                                BOOL thumbnailCreated = FALSE;

                                                HANDLES(EnterCriticalSection(&window->ICSectionUsingThumb));
                                                thumbMaker.TransformThumbnail();
                                                if (thumbMaker.RenderToThumbnailData(thumbnailData))
                                                {
                                                    iconData->SetFlag(thumbnailFlag); // already loaded
                                                    if (thumbnailFlag == 6 /* low-quality/smaller thumbnail in the first loading round */)
                                                        iconData->SetReadingDone(0); // another round will follow, so mark as not "done"
                                                    thumbnailCreated = TRUE;
                                                }
                                                HANDLES(LeaveCriticalSection(&window->ICSectionUsingThumb));

                                                if (thumbnailCreated)
                                                {
                                                    // find the index of the file (directories have no thumbnails) for which we loaded the thumbnail
                                                    char* name2 = iconData->NameAndData;
                                                    int z;
                                                    for (z = 0; z < window->Files->Count; z++)
                                                    {
                                                        if (strcmp(name2, window->Files->At(z).Name) == 0)
                                                        {
                                                            PostMessage(window->HWindow, WM_USER_REFRESHINDEX,
                                                                        window->Dirs->Count + z, 0);
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        thumbMaker.Clear(); // the thumbnail will not be needed anymore
                                    }
                                }
                                else
                                    callWaitForObjects = FALSE; // no work -> no waiting
                            }
                        }
                        else
                        {
                            someNameSkipped = TRUE;     // at least one name was skipped
                            callWaitForObjects = FALSE; // no work -> no waiting
                        }
                    }
                    else
                    {
                        if (canReadIconOverlays && !readIconOverlaysNow)
                        { // now we are going to read icon overlays
                            i = 0;
                            readIconOverlaysNow = TRUE;
                            //                TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                            continue;
                        }
                        else
                        {
                            readIconOverlaysNow = FALSE;
                            //                TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                        }

                        if (!readOnlyVisibleItems && (selectMode == 2 || selectMode == 3))
                        {
                            i = 0;
                            selectMode++;
                            //                TRACE_I("selectMode=" << selectMode);
                            continue;
                        }

                        // the first icon-reading round is over, so all icon overlays are loaded -> prevent needless attempts to read them again
                        canReadIconOverlays = FALSE;

                        // loading order: new icons, new thumbnails, old icons, old thumbnails
                        BOOL done = FALSE; // TRUE == break, everything is loaded
                        switch (wanted)
                        {
                        case 0: // new icons have already been loaded
                        {
                            // if thumbnails should be read and this is the first round (plug-ins do not work
                            // randomly like the system, so if they fail the first time they will never load), read
                            // new thumbnails (wanted == 4)
                            if (readThumbnails && firstRound)
                                wanted = 4;
                            else
                                wanted = 2; // otherwise reload old (inherited) icons
                            break;
                        }

                        case 4: // new thumbnails have already been loaded
                        {
                            wanted = 2; // reload old (inherited) icons
                            break;
                        }

                        case 2: // old icons have already been loaded
                        {
                            if (readThumbnails && firstRound)
                                wanted = 6; // reload old (inherited + low-quality/smaller) thumbnails
                            else
                                done = TRUE;
                            break;
                        }

                        default:
                            done = TRUE;
                            break;
                        }
                        if (done)
                            break; // finished - wanted 0 and 2 or 0, 4, 2 and 6 or just 3 or -1 (error)

                        //              TRACE_I("wanted=" << wanted);

                        if (selectMode == 4)
                        {
                            i = 0;
                            selectMode = 2;
                            //                TRACE_I("selectMode=" << selectMode);
                            readIconOverlaysNow = FALSE;
                            //                TRACE_I("readIconOverlaysNow=" << readIconOverlaysNow);
                            continue;
                        }

                        i = -1;                     // ensure 'i' becomes zero
                        callWaitForObjects = FALSE; // no work -> no waiting
                    }

                    i++;
                    if (callWaitForObjects)
                    {
                        wait = WaitForMultipleObjects(2, handles, FALSE, 0);
                        // we will not ignore the "work" signal because each "sleep->wake-up" means starting work from the beginning
                        if (wait != WAIT_TIMEOUT)
                            break; // process the wait event
                    }
                    // else wait = WAIT_TIMEOUT;  // needless, wait is already WAIT_TIMEOUT
                }
                repeatedRound = FALSE;

                if (wait == WAIT_TIMEOUT && readOnlyVisibleItemsDueToUMI)
                { // not all icons may be loaded due to priority given to usermenu icons (read before icons outside the visible area)
                    if (UserMenuIconBkgndReader.IsReadingIcons())
                    {
                        //              TRACE_I("Visible icons done, giving priority to usermenu icons...");
                        while (1)
                        {
                            if (window->ICSleep)
                                goto GO_SLEEP_MODE;
                            HANDLES(LeaveCriticalSection(&window->ICSleepSection));

                            wait = WaitForMultipleObjects(2, handles, FALSE, 100); // give some time for usermenu icon loading

                            HANDLES(EnterCriticalSection(&window->ICSleepSection));
                            if (window->ICSleep)
                                goto GO_SLEEP_MODE; // the panel already wants to switch to sleep mode

                            if (wait != WAIT_TIMEOUT)
                            {
                                //                  TRACE_I("Handling event...");
                                break; // process the wait event
                            }
                            int visArrVer; // check if the visible area changed; if so we must start reading icons again
                            if (someNameSkipped && window->VisibleItemsArray.IsArrValid(&visArrVer) && visArrVer != lastVisArrVersion)
                            {
                                //                  TRACE_I("Change of visible items array...");
                                break;
                            }
                            if (!UserMenuIconBkgndReader.IsReadingIcons())
                            {
                                //                  TRACE_I("Usermenu icons done...");
                                break; // if usermenu icons are already done, read the remaining icons in the panel
                            }
                        }
                    }
                    if (wait == WAIT_TIMEOUT) // reason to retry reading icons (visible area change or usermenu icons finished)
                    {
                        if (!UserMenuIconBkgndReader.IsReadingIcons()) // if usermenu icons are done, read icons outside the visible area
                        {
                            //                if (someNameSkipped) TRACE_I("Usermenu icons done, going to read the rest of icons in panel...");
                            readOnlyVisibleItems = FALSE;
                            readOnlyVisibleItemsDueToUMI = FALSE;
                        }
                        //              else
                        //                if (someNameSkipped) TRACE_I("Going to reread visible icons in panel...");
                        if (someNameSkipped)
                        {
                            repeatedRound = TRUE; // an extra round (we do not want to read icon overlays again)
                            goto SECOND_ROUND;
                        }
                        //              else
                        //                TRACE_I("All items in panel are visible, so no reason to reread icons...");
                    }
                }

                if (wait == WAIT_TIMEOUT) // work is done -> notify the main thread
                {
                    if (window->Is(ptDisk) && failed && firstRound)
                    {                                   // try again (not all icons were loaded)
                        firstRound = FALSE;             // only one extra round
                        waitBeforeFirstReadIcon = TRUE; // prevent immediate rereading (low chance of success)
                                                        //              TRACE_I("Going to second round of reading (some icons have not been read in the first round).");
                        goto SECOND_ROUND;
                        // postRefresh = TRUE;
                    }
                    else
                        firstRound = TRUE;

                    //            TRACE_I("Stop reading.");
                    // send a notification that icon reading in the panel has finished
                    if (window->HWindow == NULL ||
                        !PostMessage(window->HWindow, WM_USER_ICONREADING_END, 0, 0))
                    { // something failed ("always false"), set IconCacheValid = TRUE here
                        window->IconCacheValid = TRUE;
                    }

                    //            if (window->HWindow != NULL)  // continuous repainting is enough
                    //              InvalidateRect(window->HWindow, NULL, TRUE);
                }
                else
                {

                GO_SLEEP_MODE:

                    // interruption (sleep icon cache thread, new work, or terminate)
                    firstRound = TRUE;
                    //            TRACE_I("Reading terminated.");
                }

                window->ICWorking = FALSE;
            }

            window->ICSleep = FALSE;
            HANDLES(LeaveCriticalSection(&window->ICSleepSection));

            /*    // replaced with goto SECOND_ROUND (reading the entire directory again freezes on network drives)
        if (postRefresh)  // moved Sleep(500) out of the critical section—it was freezing unnecessarily...
        {
          HANDLES(EnterCriticalSection(&TimeCounterSection));  // take the time when a refresh is needed
          int t1 = MyTimeCounter++;
          HANDLES(LeaveCriticalSection(&TimeCounterSection));
          Sleep(500);  // a short breather
          PostMessage(window->HWindow, WM_USER_REFRESH_DIR, 0, t1);
        }
*/

            break;
        }

        default: // terminate
        {
            run = FALSE;
            break;
        }
        }
    }

    ShellIconOverlays.ReleaseIconReadersIconOverlayIds(iconReadersIconOverlayIds);

    OleUninitialize();

    TRACE_I("End");
    return 0;
}

unsigned IconThreadThreadFEH(void* param)
{
    CALL_STACK_MESSAGE_NONE
#ifndef CALLSTK_DISABLE
    __try
    {
#endif // CALLSTK_DISABLE
        return IconThreadThreadFBody(param);
#ifndef CALLSTK_DISABLE
    }
    __except (CCallStack::HandleException(GetExceptionInformation()))
    {
        TRACE_I("Thread IconReader: calling ExitProcess(1).");
        //    ExitProcess(1);
        TerminateProcess(GetCurrentProcess(), 1); // harder exit (this call still performs some operations)
        return 1;
    }
#endif // CALLSTK_DISABLE
}

DWORD WINAPI IconThreadThreadF(void* param)
{
    CALL_STACK_MESSAGE_NONE
#ifndef CALLSTK_DISABLE
    CCallStack stack;
#endif // CALLSTK_DISABLE
    return IconThreadThreadFEH(param);
}

CFilesWindow::CFilesWindow(CMainWindow* parent, CPanelSide side)
    : Columns(20, 10), ColumnsTemplate(20, 10), VisibleItemsArray(FALSE), VisibleItemsArraySurround(TRUE)
{
    static ULONGLONG nextPanelTabId = 0;

    CALL_STACK_MESSAGE1("CFilesWindow::CFilesWindow()");
    NarrowedNameColumn = FALSE;
    FullWidthOfNameCol = 0;
    WidthOfMostOfNames = 0;
    ColumnsTemplateIsForDisk = FALSE; // just initialization; set later in BuildColumnsTemplate()
    StopThumbnailLoading = FALSE;
    UserWorkedOnThisPath = FALSE;

    UnpackedAssocFiles.SetPanel(this);
    QuickRenameWindow.SetPanel(this);

    FilesMap.SetPanel(this);
    ScrollObject.SetPanel(this);
    HiddenDirsFilesReason = 0;
    HiddenDirsCount = HiddenFilesCount = 0;
    IconCacheValid = FALSE;
    InactWinOptimizedReading = FALSE;
    WaitBeforeReadingIcons = 0;
    WaitOneTimeBeforeReadingIcons = 0;
    EndOfIconReadingTime = GetTickCount() - 10000;
    ICEventTerminate = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    ICEventWork = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    ICSleep = FALSE;
    ICWorking = FALSE;
    ICStopWork = FALSE;
    HANDLES(InitializeCriticalSection(&ICSleepSection));
    HANDLES(InitializeCriticalSection(&ICSectionUsingIcon));
    HANDLES(InitializeCriticalSection(&ICSectionUsingThumb));
    DWORD ThreadID;
    IconCacheThread = NULL;
    if (ICEventTerminate != NULL && ICEventWork != NULL)
        IconCacheThread = HANDLES(CreateThread(NULL, 0, IconThreadThreadF, this, 0, &ThreadID));
    if (ICEventTerminate == NULL ||
        ICEventWork == NULL ||
        IconCacheThread == NULL)
    {
        TRACE_E("Unable to start icon-reader thread.");
        IconCache = NULL;
    }
    else
    {
        //    SetThreadPriority(IconCacheThread, THREAD_PRIORITY_IDLE); // loading then fails
        IconCache = new CIconCache();
    }

    OpenedDrivesList = NULL;

    Parent = parent;
    PanelSide = side;
    PanelTabId = ++nextPanelTabId;
    if (PanelTabId == 0)
        PanelTabId = ++nextPanelTabId;
    CustomTabColorValid = false;
    CustomTabColor = RGB(0, 0, 0);
    CustomTabPrefixValid = false;
    CustomTabPrefix.clear();
    TabLocked = false;
    ViewTemplate = parent->ViewTemplates.Get(2); // detailed view
    BuildColumnsTemplate();
    CopyColumnsTemplateToColumns();
    ListBox = NULL;
    StatusLine = NULL;
    DirectoryLine = NULL;
    HTreeView = NULL;
    HTreeHeader = NULL;
    HTreeHeaderToolTip = NULL;
    HTreeSplit = NULL;
    HTreeDPIImageList = NULL;
    WindowDPI = 0;
    WindowPanelFont = NULL;
    WindowPanelFontUL = NULL;
    WindowEnvFont = NULL;
    WindowEnvFontBold = NULL;
    WindowEnvFontUL = NULL;
    WindowPanelFontHeight = 0;
    WindowEnvFontHeight = 0;
    WindowTextEllipsisWidth = 0;
    WindowTextEllipsisWidthEnv = 0;
    for (int dpiIconIndex = 0; dpiIconIndex < ICONSIZE_COUNT; ++dpiIconIndex)
        WindowIconSizes[dpiIconIndex] = 0;
    TreeViewAutoHideExpanded = FALSE;
    TreeViewAutoHideCollapseStart = 0;
    TreeViewWidth = Configuration.TreeViewWidth;
    TreeViewAutoHide = Configuration.TreeViewAutoHide;
    TreeViewAsyncLoadThread = NULL;
    TreeViewAsyncTerminateEvent = HANDLES(CreateEvent(NULL, TRUE, FALSE, NULL)); // manual reset
    TreeViewAsyncLoadData = NULL;
    ExplorerSortThread = NULL;
    ExplorerSortData = NULL;
    ExplorerPropertyCache = NULL;
    ExplorerSortThrobberID = -1;
    StatusLineVisible = TRUE;
    DirectoryLineVisible = TRUE;
    HeaderLineVisible = TRUE;
    TreeViewActive = FALSE;
    TreeViewDisableNotify = FALSE;
    TreeViewSplitDragging = FALSE;
    TreeViewSplitOffset = 0;

    SortType = stName;
    SortCustomData = 0;
    ReverseSort = FALSE;
    SortedWithRegSet = FALSE;    // initial state doesn't matter; set in SortDirectory()
    SortedWithDetectNum = FALSE; // initial state doesn't matter; set in SortDirectory()
    LastFocus = INT_MAX;
    SetValidFileData(VALID_DATA_ALL);
    AutomaticRefresh = TRUE;
    NeedsRefreshOnActivation = FALSE;
    NextFocusName[0] = 0;
    DontClearNextFocusName = FALSE;
    LastRefreshTime = 0;
    FilesActionInProgress = FALSE;
    CanDrawItems = TRUE;
    FileBasedCompression = FALSE;
    FileBasedEncryption = FALSE;
    SupportACLS = FALSE;
    DeviceNotification = NULL;
    ContextMenu = NULL;
    ContextSubmenuNew = new CMenuNew;
    UseSystemIcons = FALSE;
    UseThumbnails = FALSE;
    NeedRefreshAfterEndOfSM = FALSE;
    RefreshAfterEndOfSMTime = 0;
    PluginFSNeedRefreshAfterEndOfSM = FALSE;
    SmEndNotifyTimerSet = FALSE;
    RefreshDirExTimerSet = FALSE;
    RefreshDirExLParam = 0;
    InactiveRefreshTimerSet = FALSE;
    InactRefreshLParam = 0;
    LastInactiveRefreshStart = LastInactiveRefreshEnd = 0;

    NeedRefreshAfterIconsReading = FALSE;
    RefreshAfterIconsReadingTime = 0;

    WorkDirHistory = NULL;
    PathHistory = new CPathHistory();

    DontDrawIndex = -1;
    DrawOnlyIndex = -1;

    FocusFirstNewItem = FALSE;

    ExecuteAssocEvent = HANDLES(CreateEvent(NULL, TRUE, FALSE, NULL));
    AssocUsed = FALSE;

    FilterEnabled = FALSE;
    Filter.SetMasksString("*.*");
    int errPos;
    Filter.PrepareMasks(errPos);

    QuickSearchMode = FALSE;
    CaretHeight = 1; // dummy
    QuickSearch[0] = 0;
    QuickSearchMask[0] = 0;
    SearchIndex = INT_MAX;
    FocusedIndex = 0;
    FocusVisible = FALSE;

    DropTargetIndex = -1;
    SingleClickIndex = -1;
    SingleClickAnchorIndex = -1;
    GetCursorPos(&OldSingleClickMousePos);

    TrackingSingleClick = FALSE;
    DragBox = FALSE;
    DragBoxVisible = FALSE;
    ScrollingWindow = FALSE;

    SkipCharacter = FALSE;
    SkipSysCharacter = FALSE;

    //  ShiftSelect = FALSE;
    DragSelect = FALSE;
    BeginDragDrop = FALSE;
    DragDropLeftMouseBtn = FALSE;
    BeginBoxSelect = FALSE;
    PersistentTracking = FALSE;

    TrackingSingleClick = 0;

    CutToClipChanged = FALSE;

    PerformingDragDrop = FALSE;

    GetPluginIconIndex = InternalGetPluginIconIndex;

    EnumFileNamesSourceUID = -1;

    TemporarilySimpleIcons = FALSE;
    NumberOfItemsInCurDir = 0;

    NeedIconOvrRefreshAfterIconsReading = FALSE;
    LastIconOvrRefreshTime = GetTickCount() - ICONOVR_REFRESH_PERIOD;
    IconOvrRefreshTimerSet = FALSE;
}

CFilesWindow::~CFilesWindow()
{
    CALL_STACK_MESSAGE1("CFilesWindow::~CFilesWindow()");

    ClearIndependentIconLists();
    StopExplorerSortAsync();
    ClearExplorerPropertyCache();

    if (DeviceNotification != NULL)
        TRACE_E("CFilesWindow::~CFilesWindow(): unexpected situation: DeviceNotification != NULL");

    if (WindowPanelFont != NULL)
        HANDLES(DeleteObject(WindowPanelFont));
    if (WindowPanelFontUL != NULL)
        HANDLES(DeleteObject(WindowPanelFontUL));
    if (WindowEnvFont != NULL)
        HANDLES(DeleteObject(WindowEnvFont));
    if (WindowEnvFontBold != NULL)
        HANDLES(DeleteObject(WindowEnvFontBold));
    if (WindowEnvFontUL != NULL)
        HANDLES(DeleteObject(WindowEnvFontUL));

    ClearHistory();

    if (WorkDirHistory != NULL)
        delete WorkDirHistory;
    if (PathHistory != NULL)
        delete PathHistory;

    if (TreeViewAsyncLoadThread != NULL)
    {
        if (TreeViewAsyncLoadData != NULL)
            ((CTreeViewAsyncLoadData*)TreeViewAsyncLoadData)->Cancelled = TRUE;
        SetEvent(TreeViewAsyncTerminateEvent);
        if (WaitForSingleObject(TreeViewAsyncLoadThread, 2000) == WAIT_TIMEOUT)
        {
            TRACE_E("Terminating TreeView Async Thread");
            TerminateThread(TreeViewAsyncLoadThread, 666);
            WaitForSingleObject(TreeViewAsyncLoadThread, INFINITE);
        }
        HANDLES(CloseHandle(TreeViewAsyncLoadThread));
        TreeViewAsyncLoadThread = NULL;
    }
    if (TreeViewAsyncLoadData != NULL)
    {
        MSG msg;
        while (PeekMessage(&msg, HWindow, WM_USER_TREEVIEW_ASYNC_DONE, WM_USER_TREEVIEW_ASYNC_DONE, PM_REMOVE))
        {
            if ((CTreeViewAsyncLoadData*)msg.lParam != TreeViewAsyncLoadData)
                FreeTreeViewAsyncLoadData((CTreeViewAsyncLoadData*)msg.lParam);
        }
        FreeTreeViewAsyncLoadData((CTreeViewAsyncLoadData*)TreeViewAsyncLoadData);
        TreeViewAsyncLoadData = NULL;
    }
    if (TreeViewAsyncTerminateEvent != NULL)
        HANDLES(CloseHandle(TreeViewAsyncTerminateEvent));

    if (IconCacheThread != NULL)
    {
        SetEvent(ICEventTerminate); // icon reader, terminate yourself!
        if (WaitForSingleObject(IconCacheThread, 1000) == WAIT_TIMEOUT)
        { // it has one second to exit gracefully, then a kill is necessary (the window is being deallocated)
            TRACE_E("Terminating Icon Thread");
            TerminateThread(IconCacheThread, 666);
            WaitForSingleObject(IconCacheThread, INFINITE); // wait until the thread really ends; sometimes it takes quite a while
        }
        HANDLES(CloseHandle(IconCacheThread));
    }

    HANDLES(DeleteCriticalSection(&ICSectionUsingThumb));
    HANDLES(DeleteCriticalSection(&ICSectionUsingIcon));
    HANDLES(DeleteCriticalSection(&ICSleepSection));
    if (ICEventTerminate != NULL)
        HANDLES(CloseHandle(ICEventTerminate));
    if (ICEventWork != NULL)
        HANDLES(CloseHandle(ICEventWork));

    if (IconCache != NULL)
        delete IconCache;
    if (ContextSubmenuNew != NULL)
        delete ContextSubmenuNew;
    if (ExecuteAssocEvent != NULL)
        HANDLES(CloseHandle(ExecuteAssocEvent));
}

BOOL CFilesWindow::RefreshDPIResources(BOOL force)
{
    HWND dpiWindow = HWindow != NULL ? HWindow : (Parent != NULL ? Parent->HWindow : NULL);
    int dpi = (int)WinLibDPIGetWindowDPI(dpiWindow);
    if (dpi <= 0)
        dpi = USER_DEFAULT_SCREEN_DPI;
    if (!force && WindowDPI == dpi && WindowPanelFont != NULL && WindowEnvFont != NULL)
        return TRUE;

    ClearIndependentIconLists();

    LOGFONT envLF;
    GetEffectiveDefaultUILogFont(&envLF, dpiWindow);

    LOGFONT panelLF;
    if (UseCustomPanelFont)
    {
        panelLF = LogFont;
        int sourceDPI = GetSystemDPI();
        if (sourceDPI <= 0)
            sourceDPI = USER_DEFAULT_SCREEN_DPI;
        panelLF.lfHeight = MulDiv(panelLF.lfHeight, dpi, sourceDPI);
        panelLF.lfWidth = MulDiv(panelLF.lfWidth, dpi, sourceDPI);
    }
    else
    {
        if (!WinLibDPIGetIconTitleLogFont(dpiWindow, &panelLF))
            GetSystemGUIFont(&panelLF);
    }

    LOGFONT panelULLF = panelLF;
    panelULLF.lfUnderline = TRUE;
    envLF.lfWeight = FW_NORMAL;
    LOGFONT envBoldLF = envLF;
    envBoldLF.lfWeight = FW_BOLD;
    LOGFONT envULLF = envLF;
    envULLF.lfUnderline = TRUE;

    HFONT panelFont = HANDLES(CreateFontIndirect(&panelLF));
    HFONT panelFontUL = HANDLES(CreateFontIndirect(&panelULLF));
    HFONT envFont = HANDLES(CreateFontIndirect(&envLF));
    HFONT envFontBold = HANDLES(CreateFontIndirect(&envBoldLF));
    HFONT envFontUL = HANDLES(CreateFontIndirect(&envULLF));
    if (panelFont == NULL || panelFontUL == NULL || envFont == NULL ||
        envFontBold == NULL || envFontUL == NULL)
    {
        if (panelFont != NULL)
            HANDLES(DeleteObject(panelFont));
        if (panelFontUL != NULL)
            HANDLES(DeleteObject(panelFontUL));
        if (envFont != NULL)
            HANDLES(DeleteObject(envFont));
        if (envFontBold != NULL)
            HANDLES(DeleteObject(envFontBold));
        if (envFontUL != NULL)
            HANDLES(DeleteObject(envFontUL));
        return FALSE;
    }

    HWND dcWindow = dpiWindow;
    HDC dc = HANDLES(GetDC(dcWindow));
    if (dc == NULL)
    {
        dcWindow = NULL;
        dc = HANDLES(GetDC(NULL));
    }
    if (dc == NULL)
    {
        HANDLES(DeleteObject(panelFont));
        HANDLES(DeleteObject(panelFontUL));
        HANDLES(DeleteObject(envFont));
        HANDLES(DeleteObject(envFontBold));
        HANDLES(DeleteObject(envFontUL));
        return FALSE;
    }
    TEXTMETRIC tm;
    SIZE ellipsis;
    HFONT oldFont = (HFONT)SelectObject(dc, panelFont);
    GetTextMetrics(dc, &tm);
    int panelHeight = tm.tmHeight;
    GetTextExtentPoint32(dc, "...", 3, &ellipsis);
    int panelEllipsis = ellipsis.cx;
    SelectObject(dc, envFont);
    GetTextMetrics(dc, &tm);
    int envHeight = tm.tmHeight;
    GetTextExtentPoint32(dc, "...", 3, &ellipsis);
    int envEllipsis = ellipsis.cx;
    SelectObject(dc, oldFont);
    HANDLES(ReleaseDC(dcWindow, dc));

    if (WindowPanelFont != NULL)
        HANDLES(DeleteObject(WindowPanelFont));
    if (WindowPanelFontUL != NULL)
        HANDLES(DeleteObject(WindowPanelFontUL));
    if (WindowEnvFont != NULL)
        HANDLES(DeleteObject(WindowEnvFont));
    if (WindowEnvFontBold != NULL)
        HANDLES(DeleteObject(WindowEnvFontBold));
    if (WindowEnvFontUL != NULL)
        HANDLES(DeleteObject(WindowEnvFontUL));
    WindowPanelFont = panelFont;
    WindowPanelFontUL = panelFontUL;
    WindowEnvFont = envFont;
    WindowEnvFontBold = envFontBold;
    WindowEnvFontUL = envFontUL;
    WindowPanelFontHeight = panelHeight;
    WindowEnvFontHeight = envHeight;
    WindowTextEllipsisWidth = panelEllipsis;
    WindowTextEllipsisWidthEnv = envEllipsis;
    WindowDPI = dpi;
    WindowIconSizes[ICONSIZE_16] = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
    WindowIconSizes[ICONSIZE_32] = MulDiv(32, dpi, USER_DEFAULT_SCREEN_DPI);
    WindowIconSizes[ICONSIZE_48] = MulDiv(48, dpi, USER_DEFAULT_SCREEN_DPI);
    return TRUE;
}

CIconList* CFilesWindow::GetIndependentIconList(CIconList* source, int sourceIndex,
                                                 CIconSizeEnum iconSize, int* copyIndex)
{
    if (copyIndex != NULL)
        *copyIndex = sourceIndex;
    if (source == NULL || sourceIndex < 0)
        return source;

    int pixelSize = GetIconSize(iconSize);
    if (source->GetImageWidth() == pixelSize &&
        source->GetImageHeight() == pixelSize)
    {
        // Preserve the original pixels and mask whenever no scaling is needed.
        // Apart from being cheaper, this avoids degrading legacy mask-based
        // 16px icons by round-tripping them through an HICON.
        return source;
    }

    LONG sourceVersion = source->GetContentVersion();
    for (size_t i = 0; i < WindowDPIIconLists.size(); ++i)
    {
        CDPIIconListEntry& entry = WindowDPIIconLists[i];
        if (entry.Source == source && entry.SourceIndex == sourceIndex &&
            entry.PixelSize == pixelSize)
        {
            if (entry.SourceVersion != sourceVersion)
            {
                // Association icons are populated asynchronously. Refresh a
                // scaled copy after the source placeholder has been replaced.
                HICON icon = source->GetIcon(sourceIndex);
                if (icon != NULL)
                {
                    if (entry.Copy->ReplaceIcon(0, icon))
                        entry.SourceVersion = sourceVersion;
                    HANDLES(DestroyIcon(icon));
                }
            }
            if (copyIndex != NULL)
                *copyIndex = 0;
            return entry.Copy;
        }
    }

    HICON icon = source->GetIcon(sourceIndex);
    if (icon == NULL)
        return source;

    CIconList* copy = new CIconList();
    if (copy == NULL || !copy->Create(pixelSize, pixelSize, 1) ||
        !copy->ReplaceIcon(0, icon))
    {
        if (copy != NULL)
            delete copy;
        HANDLES(DestroyIcon(icon));
        return source;
    }
    HANDLES(DestroyIcon(icon));
    copy->SetBkColor(GetCOLORREF(CurrentColors[ITEM_BK_NORMAL]));

    CDPIIconListEntry entry;
    entry.Source = source;
    entry.SourceIndex = sourceIndex;
    entry.PixelSize = pixelSize;
    entry.SourceVersion = sourceVersion;
    entry.Copy = copy;
    WindowDPIIconLists.push_back(entry);
    if (copyIndex != NULL)
        *copyIndex = 0;
    return copy;
}

void CFilesWindow::ClearIndependentIconLists()
{
    for (size_t i = 0; i < WindowDPIIconLists.size(); ++i)
        delete WindowDPIIconLists[i].Copy;
    WindowDPIIconLists.clear();
}

CPathHistory* CFilesWindow::EnsureWorkDirHistory()
{
    if (WorkDirHistory == NULL)
    {
        WorkDirHistory = new CPathHistory(TRUE);
        if (WorkDirHistory == NULL)
        {
            TRACE_E(LOW_MEMORY);
            return NULL;
        }
    }
    return WorkDirHistory;
}

void CFilesWindow::CapturePathForShutdown()
{
    std::vector<char> path(2 * SAL_MAX_PATH);
    if (GetGeneralPath(path.data(), (int)path.size(), TRUE))
        ShutdownGeneralPath.assign(path.data());
    else
        ShutdownGeneralPath.clear();
}

void CFilesWindow::ClearWorkDirHistory()
{
    if (WorkDirHistory != NULL)
        WorkDirHistory->ClearHistory();
}

void CFilesWindow::ClearHistory()
{
    if (PathHistory != NULL)
        PathHistory->ClearHistory();

    ClearWorkDirHistory();

    OldSelection.Clear();
}

void CFilesWindow::SleepIconCacheThread()
{
    CALL_STACK_MESSAGE1("CFilesWindow::SleepIconCacheThread()");
    ICSleep = TRUE;          // to interrupt the icon-reading loop (ICSleepSection may not be left at all)
    ICStopWork = TRUE;       // to interrupt the icon-reading loop if ICStopWork has already been processed
    ResetEvent(ICEventWork); // to interrupt the icon-reading loop if ICStopWork has not been processed yet
    // wait until the icon reader enters a part where sleep mode is possible
    HANDLES(EnterCriticalSection(&ICSleepSection));
    ICSleep = ICWorking; // TRUE only if the icon reader is stuck in SHGetFileInfo
    HANDLES(LeaveCriticalSection(&ICSleepSection));
}

void CFilesWindow::WakeupIconCacheThread()
{
    CALL_STACK_MESSAGE_NONE
    ICStopWork = FALSE;    // so that the work is not interrupted right from the start
    SetEvent(ICEventWork); // switch to work mode without waiting for a response
    MSG msg;               // remove any WM_USER_ICONREADING_END that would set IconCacheValid = TRUE
    while (PeekMessage(&msg, HWindow, WM_USER_ICONREADING_END, WM_USER_ICONREADING_END, PM_REMOVE))
        ;
}

BOOL CFilesWindow::CheckAndRestorePath(const char* path)
{
    CALL_STACK_MESSAGE2("CFilesWindow::CheckAndRestorePath(%s)", path);

    // we will not test network paths if we have just accessed them
    BOOL tryNet = (!Is(ptDisk) && !Is(ptZIPArchive)) || !HasTheSameRootPath(path, GetPath());

    return SalCheckAndRestorePath(HWindow, path, tryNet);
}

BOOL CFilesWindow::CanUnloadPlugin(HWND parent, CPluginInterfaceAbstract* plugin)
{
    CALL_STACK_MESSAGE1("CFilesWindow::CanUnloadPlugin()");

    if (Is(ptDisk))
    {
        if (UseThumbnails && // thumbnails are being loaded
            !IconCacheValid) // the icon reader has not finished loading yet
        {
            CPluginData* p = Plugins.GetPluginData(plugin);
            if (p != NULL) // "always true"
            {
                if (p->ThumbnailMasks.GetMasksString()[0] != 0)
                { // this plugin provides thumbnails—we aren't sure whether
                    // it also serves this panel, so we must stop reading icons
                    SleepIconCacheThread();
                    p->ThumbnailMasksDisabled = TRUE; // during plugin unload/remove this plugin cannot be used to load thumbnails
                    StopThumbnailLoading = TRUE;      // in case WakeupIconCacheThread is called; icon-cache data about "thumbnail loaders" can't be used
                    UseThumbnails = FALSE;            // prevent an unwanted icon-reader wake-up (WakeupIconCacheThread())
                    if (!CriticalShutdown)
                    {
                        HANDLES(EnterCriticalSection(&TimeCounterSection));
                        int t1 = MyTimeCounter++;
                        HANDLES(LeaveCriticalSection(&TimeCounterSection));
                        PostMessage(HWindow, WM_USER_REFRESH_DIR, 0, t1); // ensure the icon cache is refilled (ideally after the plug-in unload/remove)
                    }
                }
            }
            else
                TRACE_E("CFilesWindow::CanUnloadPlugin(): Unexpected situation!");
        }
    }
    else
    {
        BOOL used = FALSE;
        if ((Is(ptZIPArchive) || Is(ptPluginFS)) &&
            PluginData.NotEmpty() && PluginData.GetPluginInterface() == plugin)
            used = TRUE;
        else
        { // a filesystem may not use PluginData, so we must also check PluginFS
            if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
                GetPluginFS()->GetPluginInterface() == plugin)
                used = TRUE;
            else
            {
                if (Is(ptZIPArchive))
                { // an archive may not use PluginData, therefore we must also test archive associations
                    // this part matters only when shutting Salamander down—otherwise the plug-in
                    // could unload while the archiver is still in use (each archiver function loads the plug-in)
                    // NOTE: icon overlays from the plug-in are an exception; after unload they would stop drawing
                    //       (the plug-in's overlay table is released during unload)
                    int format = PackerFormatConfig.PackIsArchive(GetZIPArchive());
                    if (format != 0) // found a supported archive
                    {
                        format--;
                        CPluginData* data;
                        int index = PackerFormatConfig.GetUnpackerIndex(format);
                        if (index < 0) // view: is this internal processing (plug-in)?
                        {
                            data = Plugins.Get(-index - 1);
                            if (data != NULL && data->GetPluginInterface()->GetInterface() == plugin)
                                used = TRUE;
                        }
                        if (PackerFormatConfig.GetUsePacker(format)) // has an editor?
                        {
                            index = PackerFormatConfig.GetPackerIndex(format);
                            if (index < 0) // is this internal processing (plug-in)?
                            {
                                data = Plugins.Get(-index - 1);
                                if (data != NULL && data->GetPluginInterface()->GetInterface() == plugin)
                                    used = TRUE;
                            }
                        }
                    }
                }
            }
        }
        if (used)
        {
            if (Is(ptZIPArchive) || Is(ptPluginFS)) // archive -> just leave it; plug-in FS -> return to the last disk path
            {
                char path[MAX_PATH];
                strcpy(path, GetPath());

                DWORD err, lastErr;
                BOOL pathInvalid, cut;
                BOOL tryNet = FALSE; // no more network delays, unnecessary...
                if (SalCheckAndRestorePathWithCut(HWindow, path, tryNet, err, lastErr, pathInvalid, cut, TRUE))
                { // switch to a path that should load without issues
                    ChangePathToDisk(parent, path, -1, NULL, NULL, TRUE, TRUE, FALSE, NULL, FALSE, FSTRYCLOSE_UNLOADCLOSEFS);
                }
                else // the original path (or its subpath) is inaccessible -> switching to a fixed drive (cannot call
                     // ChangePathToDisk directly, because it would display an error like "X: not ready")
                {
                    ChangeToRescuePathOrFixedDrive(parent, NULL, TRUE, TRUE, FSTRYCLOSE_UNLOADCLOSEFS);
                }
                if (!Is(ptDisk))
                {
                    return FALSE; // switching to a disk path failed; unload is not possible
                }
            }
        }
    }
    return TRUE;
}

void CFilesWindow::RedrawFocusedIndex()
{
    CALL_STACK_MESSAGE1("CFilesWindow::RedrawFocusedIndex()");
    RedrawIndex(FocusedIndex);
}

void CFilesWindow::DirectoryLineSetText()
{
    CALL_STACK_MESSAGE1("CFilesWindow::DirectoryLineSetText()");
    char ZIPbuf[2 * MAX_PATH];
    std::string diskPathText;
    const char* path = NULL;
    if (Is(ptZIPArchive))
    {
        strcpy(ZIPbuf, GetZIPArchive());
        if (GetZIPPath()[0] != 0)
        {
            if (GetZIPPath()[0] != '\\')
                strcat(ZIPbuf, "\\");
            strcat(ZIPbuf, GetZIPPath());
        }
        path = ZIPbuf;
        PathHistory->AddPath(1, GetZIPArchive(), GetZIPPath(), NULL, NULL);
    }
    else
    {
        if (Is(ptDisk))
        {
            PathHistory->AddPath(0, GetPath(), NULL, NULL, NULL);
            if (GetPathW() != NULL && GetPathW()[0] != 0)
            {
                diskPathText = SalWideToMultiBytePath(GetPathW(), CP_UTF8);
                path = diskPathText.c_str();
            }
            else
                path = GetPath();
        }
        else
        {
            if (Is(ptPluginFS))
            {
                int l = (int)strlen(GetPluginFS()->GetPluginFSName());
                memcpy(ZIPbuf, GetPluginFS()->GetPluginFSName(), l);
                ZIPbuf[l++] = ':';
                if (!GetPluginFS()->NotEmpty() || !GetPluginFS()->GetCurrentPath(ZIPbuf + l))
                    ZIPbuf[l] = 0;
                else
                {
                    PathHistory->AddPath(2, GetPluginFS()->GetPluginFSName(), ZIPbuf + l,
                                         GetPluginFS()->GetInterface(), GetPluginFS());
                }
                path = ZIPbuf;
            }
        }
    }

    if (path == NULL)
        return;

    if (FilterEnabled)
    {
        std::string filterText;
        char buf[3 * MAX_PATH]; // zip path (2x) + filter (1x) = 3x MAX_PATH
        int pathLen = (int)strlen(path);
        if (Is(ptDisk) || Is(ptZIPArchive))
        {
            filterText.assign(path, pathLen);
            if (!filterText.empty() && filterText[filterText.length() - 1] != '\\')
                filterText += '\\';
            filterText += Filter.GetMasksString();
        }
        else
        {
            if (Is(ptPluginFS))
            {
                int l = pathLen;
                memcpy(buf, path, l);
                buf[l++] = ':';
                //        if (FilterInverse) buf[l++] = '-';
                lstrcpyn(buf + l, Filter.GetMasksString(), MAX_PATH);
            }
        }
        DirectoryLine->SetText(filterText.empty() ? buf : filterText.c_str(), pathLen);
    }
    else
    {
        DirectoryLine->SetText(path);
    }

    if (Parent != NULL)
        Parent->UpdatePanelTabTitle(this);
}

void CFilesWindow::SelectUnselect(BOOL forceIncludeDirs, BOOL select, BOOL showMaskDlg)
{
    CALL_STACK_MESSAGE4("CFilesWindow::SelectUnselect(%d, %d, %d)", forceIncludeDirs, select, showMaskDlg);
    if (showMaskDlg)
    {
        BeginStopRefresh(); // snooper takes a break
    }
    if (!showMaskDlg || CSelectDialog(HLanguage, select ? IDD_SELECTMASK : IDD_DESELECTMASK,
                                      select ? IDD_SELECTMASK : IDD_DESELECTMASK /* helpID */,
                                      HWindow, MainWindow->SelectionMask)
                                .Execute() == IDOK)
    {
        BOOL includeDirs = Configuration.IncludeDirs | forceIncludeDirs;
        const char* maskStr = showMaskDlg ? MainWindow->SelectionMask : "*.*";
        CMaskGroup mask(maskStr);
        int err;
        if (mask.PrepareMasks(err))
        {
            int dirsCount = Dirs->Count;
            int count = dirsCount + Files->Count;
            int start;
            if (Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0)
                start = 1;
            else
                start = 0;
            int i = includeDirs ? start : Dirs->Count;
            BOOL changed = FALSE;
            for (; i < count; i++)
            {
                CFileData* d = (i < dirsCount) ? &Dirs->At(i) : &Files->At(i - dirsCount);
                if (!showMaskDlg || mask.AgreeMasks(d->Name, i < dirsCount ? NULL : d->Ext)) // in the case of *.* we will not call agree mask
                {
                    SetSel(select, d);
                    changed = TRUE;
                }
            }
            if (changed)
            {
                PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
                RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
            }
            else
                SalMessageBox(HWindow, LoadStr(IDS_NOMATCHESFOUND), LoadStr(IDS_INFOTITLE),
                              MB_OK | MB_ICONINFORMATION);
        }
    }
    if (showMaskDlg)
    {
        UpdateWindow(MainWindow->HWindow);
        EndStopRefresh(); // the snooper starts again now
    }
}

void CFilesWindow::InvertSelection(BOOL forceIncludeDirs)
{
    CALL_STACK_MESSAGE2("CFilesWindow::InvertSelection(%d)", forceIncludeDirs);
    BOOL includeDirs = Configuration.IncludeDirs | forceIncludeDirs;
    int count = GetSelCount();
    int firstIndex = 0;
    if (includeDirs)
    {
        if (Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0)
            firstIndex = 1;
    }
    else
    {
        firstIndex = Dirs->Count;
    }

    int lastIndex = Dirs->Count + Files->Count - 1;
    if (firstIndex <= lastIndex)
    {
        int i;
        for (i = firstIndex; i <= lastIndex; i++)
        {
            CFileData* item = (i < Dirs->Count) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
            SetSel(item->Selected != 1, item);
        }
        RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
        PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
    }
}

void CFilesWindow::SelectUnselectByFocusedItem(BOOL select, BOOL byName)
{
    CALL_STACK_MESSAGE3("CFilesWindow::SelectUnselectByFocusedItem(%d, %d)", select, byName);
    if (FocusedIndex >= 0 && FocusedIndex < Dirs->Count + Files->Count)
    {

        //    if (!byName && FocusedIndex < Dirs->Count)
        //    {
        //
        //    }

        BOOL isDir = FocusedIndex < Dirs->Count;
        const CFileData* focusedItem = isDir ? &Dirs->At(FocusedIndex) : &Files->At(FocusedIndex - Dirs->Count);

        int firstIndex = 0;
        if (Configuration.IncludeDirs)
        {
            if (Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0)
                firstIndex = 1;
        }
        else
        {
            firstIndex = Dirs->Count;
        }
        int lastIndex = Dirs->Count + Files->Count - 1;
        int lastSelectdCount = SelectedCount;
        const char* focusedStr = byName ? focusedItem->Name : (isDir ? "" : focusedItem->Ext);
        int focusedLen = byName ? (isDir ? focusedItem->NameLen : (int)(focusedItem->Ext - focusedItem->Name)) : (isDir ? 0 : (int)lstrlen(focusedItem->Ext));
        if (!isDir && byName && *focusedItem->Ext != 0)
            focusedLen--; // skip '.'
        int i;
        for (i = firstIndex; i <= lastIndex; i++)
        {
            BOOL itemIsDir = i < Dirs->Count;
            CFileData* item = itemIsDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
            const char* str = byName ? item->Name : (itemIsDir ? "" : item->Ext);
            int len = byName ? (itemIsDir ? item->NameLen : (int)(item->Ext - item->Name)) : (itemIsDir ? 0 : (int)lstrlen(item->Ext));
            if (!itemIsDir && byName && *item->Ext != 0)
                len--; // skip '.'
            if (len == focusedLen && StrNICmp(str, focusedStr, len) == 0)
                SetSel(select, item);
        }
        if (SelectedCount != lastSelectdCount)
        {
            RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
            PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
        }
    }
}

void CFilesWindow::StoreGlobalSelection()
{
    CALL_STACK_MESSAGE1("CFilesWindow::StoreGlobalSelection()");
    int count = GetSelCount();
    if (count != 0)
    {
        BeginStopRefresh(); // snooper takes a break

        BOOL clipboard = FALSE;
        CSaveSelectionDialog dlg(HWindow, &clipboard);
        if (dlg.Execute() == IDOK)
        {
            int totalCount = Dirs->Count + Files->Count;
            if (clipboard)
            {
                // we should put the list on the clipboard

                // compute the required buffer size (name1CRLFname2CRLF...nameNCRLF)
                DWORD size = 0;
                int i;
                for (i = 0; i < totalCount; i++)
                {
                    CFileData* f = (i < Dirs->Count) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                    if (f->Selected)
                        size += f->NameLen + 2; // nameCRLF
                }
                if (size > 0)
                {
                    char* buff = (char*)malloc(size);
                    if (buff != NULL)
                    {
                        char* p = buff;
                        for (i = 0; i < totalCount; i++)
                        {
                            CFileData* f = (i < Dirs->Count) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                            if (f->Selected)
                            {
                                memcpy(p, f->Name, f->NameLen);
                                p += f->NameLen;
                                memcpy(p, "\r\n", 2);
                                p += 2;
                            }
                        }
                        CopyTextToClipboard(buff, size);
                        free(buff);
                    }
                    else
                        TRACE_E(LOW_MEMORY);
                }
            }
            else
            {
                // store the list in GlobalSelection
                GlobalSelection.Clear();
                int i;
                for (i = 0; i < totalCount; i++)
                {
                    CFileData* f = (i < Dirs->Count) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                    if (f->Selected)
                    {
                        if (!GlobalSelection.Add(i < Dirs->Count, f->Name))
                            break; // low memory
                    }
                }
                GlobalSelection.Sort();
            }
            IdleRefreshStates = TRUE; // force state variables check on next Idle
        }
        UpdateWindow(MainWindow->HWindow);

        EndStopRefresh(); // the snooper starts again now
    }
}

void CFilesWindow::RestoreGlobalSelection()
{
    CALL_STACK_MESSAGE1("CFilesWindow::RestoreGlobalSelection()");

    BOOL clipboardValid = IsTextOnClipboard();
    BOOL globalValid = GlobalSelection.GetCount() > 0;
    if (clipboardValid || globalValid)
    {
        BeginStopRefresh(); // snooper takes a break

        CLoadSelectionOperation operation = lsoCOPY;
        BOOL clipboard = !globalValid;
        CLoadSelectionDialog dlg(HWindow, &operation, &clipboard, clipboardValid, globalValid);
        if (dlg.Execute() == IDOK)
        {
            CNames* selection = &GlobalSelection;
            CNames clipboardSelection;
            if (clipboard)
            {
                clipboardSelection.LoadFromClipboard(HWindow);
                clipboardSelection.Sort();
                selection = &clipboardSelection;
            }

            int count = Files->Count + Dirs->Count;
            int i;
            for (i = 0; i < count; i++)
            {
                BOOL isDir = i < Dirs->Count;
                CFileData* file = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                if (clipboard)
                    isDir = FALSE; // when using the clipboard everything is in Files
                switch (operation)
                {
                case lsoCOPY:
                {
                    SetSel(selection->Contains(isDir, file->Name), file);
                    break;
                }

                case lsoOR:
                {
                    if (selection->Contains(isDir, file->Name))
                        SetSel(TRUE, file);
                    break;
                }

                case lsoDIFF:
                {
                    if (file->Selected)
                        SetSel(!selection->Contains(isDir, file->Name), file);
                    break;
                }

                case lsoAND:
                {
                    SetSel(file->Selected && selection->Contains(isDir, file->Name), file);
                    break;
                }

                default:
                {
                    TRACE_E("Unknown operation: " << operation);
                    break;
                }
                }
            }
            RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
            PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
        }
        UpdateWindow(MainWindow->HWindow);
        EndStopRefresh(); // the snooper starts again now
    }
}

void CFilesWindow::StoreSelection()
{
    CALL_STACK_MESSAGE1("CFilesWindow::StoreSelection()");
    OldSelection.Clear();
    int count = GetSelCount();
    if (count != 0)
    {
        OldSelection.SetCaseSensitive(IsCaseSensitive());
        int totalCount = Files->Count + Dirs->Count;
        int i;
        for (i = 0; i < totalCount; i++)
        {
            BOOL isDir = i < Dirs->Count;
            CFileData* f = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
            if (f->Selected)
            {
                if (!OldSelection.Add(isDir, f->Name))
                    break; // low memory
            }
        }
        OldSelection.Sort();
        IdleRefreshStates = TRUE; // force state variables check on next Idle
    }
}

void CFilesWindow::Reselect()
{
    CALL_STACK_MESSAGE1("CFilesWindow::Reselect()");
    int count = Files->Count + Dirs->Count;
    int i;
    for (i = 0; i < count; i++)
    {
        BOOL isDir = i < Dirs->Count;
        CFileData* file = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
        if (OldSelection.Contains(isDir, file->Name))
            SetSel(TRUE, file);
        else
            SetSel(FALSE, file);
    }
    RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
    PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
}

void CFilesWindow::ShowHideNames(int mode)
{
    BOOL refreshPanel = FALSE;
    switch (mode)
    {
    case 0: // show all
    {
        if (HiddenNames.GetCount() > 0)
        {
            HiddenNames.Clear();
            refreshPanel = TRUE;
        }
        break;
    }

    case 1: // hide selected names
    {
        int count = GetSelCount();
        if (count > 0)
        {
            int totalCount = Files->Count + Dirs->Count;
            int startIndex = 0;
            if (Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0) // ".." should not appear in the array
                startIndex = 1;
            int i;
            for (i = 0; i < totalCount; i++)
            {
                BOOL isDir = i < Dirs->Count;
                CFileData* f = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                if (f->Selected)
                {
                    if (!HiddenNames.Add(isDir, f->Name))
                        break; // low memory, we will not continue
                    refreshPanel = TRUE;
                }
            }
        }
        break;
    }

    case 2: // hide unselected name
    {
        int totalCount = Files->Count + Dirs->Count;
        int startIndex = 0;
        if (Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0) // ".." should not appear in the array
            startIndex = 1;
        int i;
        for (i = startIndex; i < totalCount; i++)
        {
            BOOL isDir = i < Dirs->Count;
            CFileData* f = (isDir) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
            if (!f->Selected)
            {
                if (!HiddenNames.Add(isDir, f->Name))
                    break; // low memory, we will not continue
                refreshPanel = TRUE;
            }
        }
        break;
    }

    default:
    {
        TRACE_E("ShowHideNames: unknown mode=" << mode);
    }
    }

    if (refreshPanel)
    {
        if (mode == 1 || mode == 2)
            HiddenNames.SetCaseSensitive(IsCaseSensitive());
        HiddenNames.Sort();
        HANDLES(EnterCriticalSection(&TimeCounterSection));
        int t1 = MyTimeCounter++;
        HANDLES(LeaveCriticalSection(&TimeCounterSection));
        PostMessage(HWindow, WM_USER_REFRESH_DIR, 0, t1);
    }
}

void CFilesWindow::SetAutomaticRefresh(BOOL value, BOOL force)
{
    CALL_STACK_MESSAGE_NONE
    if (force || AutomaticRefresh != value)
    {
        AutomaticRefresh = value;
        /* // "throwing away" the refresh mark from the directory line
    // it crashed here; a destroyed object was called
    if (DirectoryLine != NULL)                       
      DirectoryLine->SetAutomatic(AutomaticRefresh);
*/
    }
}

BOOL CFilesWindow::ConfirmUnlockTabForPathChange()
{
    CALL_STACK_MESSAGE1("CFilesWindow::ConfirmUnlockTabForPathChange()");

    if (!IsTabLocked())
        return TRUE;

    if (MainWindow == NULL || MainWindow->GetPanelTabIndex(GetPanelSide(), this) <= 0)
        return TRUE;

    if (MainWindow->HWindow == NULL || !IsWindowVisible(MainWindow->HWindow) || GetPath()[0] == 0)
        return TRUE;

    if (SalMessageBox(HWindow, LoadStr(IDS_LOCKEDTAB_CHANGEPATH), LoadStr(IDS_LOCKEDTAB_TITLE),
                      MB_YESNO | MB_ICONQUESTION) != IDYES)
        return FALSE;

    MainWindow->CommandUnlockTab(this);
    return TRUE;
}

void CFilesWindow::GotoRoot()
{
    CALL_STACK_MESSAGE1("CFilesWindow::GotoRoot()");
    TopIndexMem.Clear(); // long jump

    char root[MAX_PATH];
    if (Is(ptDisk) || Is(ptZIPArchive))
    {
        if (Is(ptZIPArchive) && GetZIPPath()[0] != 0) // we are not in the root of the archive -> go there
        {
            ChangePathToArchive(GetZIPArchive(), "");
        }
        else // go to the root of the Windows path
        {
            if (IsUNCRootPath(GetPath()) && Plugins.GetFirstNethoodPluginFSName(root))
            {
                ChangePathToPluginFS(root, "");
            }
            else
            {
                GetRootPath(root, GetPath());
                if (root[0] == '\\')
                    root[strlen(root) - 1] = 0; // UNC paths should not end with '\\'
                ChangePathToDisk(HWindow, root);
            }
        }
    }
    else
    {
        if (Is(ptPluginFS))
        {
            if (GetPluginFS()->GetRootPath(root))
            {
                char fsname[MAX_PATH];
                strcpy(fsname, GetPluginFS()->GetPluginFSName()); // in case of changes, a local copy of the name
                ChangePathToPluginFS(fsname, root);
            }
        }
    }
}

void CFilesWindow::GotoHotPath(int index)
{
    CALL_STACK_MESSAGE2("CFilesWindow::GotoHotPath(%d)", index);
    if (index < 0 || index >= HOT_PATHS_COUNT)
        return;
    //---  switch to a hot path
    char path[SAL_MAX_PATH];
    if (MainWindow->GetExpandedHotPath(HWindow, index, path, SAL_MAX_PATH))
        ChangeDir(path);
}

void CFilesWindow::SetUnescapedHotPath(int index)
{
    CALL_STACK_MESSAGE2("CFilesWindow::SetUnescapedHotPath(%d)", index);
    if (index < 0 || index >= HOT_PATHS_COUNT)
        return;
    char path[2 * MAX_PATH];
    GetGeneralPath(path, 2 * MAX_PATH, TRUE);
    MainWindow->SetUnescapedHotPath(index, path);
}

BOOL CFilesWindow::SetUnescapedHotPathToEmptyPos()
{
    CALL_STACK_MESSAGE1("CFilesWindow::SetUnescapedHotPathToEmptyPos()");
    int index = MainWindow->GetUnassignedHotPathIndex();
    if (index != -1)
    {
        char path[2 * MAX_PATH];
        GetGeneralPath(path, 2 * MAX_PATH, TRUE);
        MainWindow->SetUnescapedHotPath(index, path);
        return TRUE;
    }
    return FALSE;
}

#ifndef _WIN64

BOOL AreNextPathComponents(const char* relPath, const char* nextComp)
{
    int len = (int)strlen(nextComp);
    return StrNICmp(relPath, nextComp, len) == 0 && (relPath[len] == '\\' || relPath[len] == 0);
}

#endif // _WIN64

void CFilesWindow::OpenActiveFolder()
{
    CALL_STACK_MESSAGE1("CFilesWindow::OpenActiveFolder()");
    if (Is(ptDisk) && CheckPath(TRUE) != ERROR_USER_TERMINATED)
    {
        UserWorkedOnThisPath = TRUE;
        const char* path = GetPath();

#ifndef _WIN64
        // replace "C:\\Windows\\sysnative\\*" with "C:\\Windows\\system32\\*" on 64-bit systems
        // the Explorer process knows nothing about "sysnative", so let's not bother users with it,
        // also replace "C:\\Windows\\system32\\*" with "C:\\Windows\\SysWOW64\\*"
        //  (except for a group of directories excluded from the redirector that thus point back to System32)
        char dirName[MAX_PATH];
        dirName[0] = 0;
        if (Windows64Bit && WindowsDirectory[0] != 0)
        {
            BOOL done = FALSE;
            lstrcpyn(dirName, WindowsDirectory, MAX_PATH);
            if (SalPathAppend(dirName, "Sysnative", MAX_PATH))
            {
                int len = (int)strlen(dirName);
                if (StrNICmp(path, dirName, len) == 0 && (path[len] == '\\' || path[len] == 0))
                {
                    lstrcpyn(dirName, WindowsDirectory, MAX_PATH);
                    SalPathAppend(dirName, "System32", MAX_PATH); // if Sysnative fit, System32 will fit as well
                    memmove(dirName + strlen(dirName), path + len, strlen(path + len) + 1);
                    path = dirName;
                    done = TRUE;
                }
            }
            if (!done)
            {
                lstrcpyn(dirName, WindowsDirectory, MAX_PATH);
                if (SalPathAppend(dirName, "System32", MAX_PATH))
                {
                    int len = (int)strlen(dirName);
                    if (StrNICmp(path, dirName, len) == 0 && (path[len] == '\\' || path[len] == 0))
                    {
                        // check whether it is a directory excluded from the redirector
                        if (path[len] == '\\' &&
                            (AreNextPathComponents(path + len + 1, "catroot") ||
                             AreNextPathComponents(path + len + 1, "catroot2") ||
                             Windows7AndLater && AreNextPathComponents(path + len + 1, "DriverStore") ||
                             AreNextPathComponents(path + len + 1, "drivers\\etc") ||
                             AreNextPathComponents(path + len + 1, "LogFiles") ||
                             AreNextPathComponents(path + len + 1, "spool")))
                        {
                            done = TRUE;
                        }
                        if (!done)
                        {
                            lstrcpyn(dirName, WindowsDirectory, MAX_PATH);
                            SalPathAppend(dirName, "SysWOW64", MAX_PATH); // if System32 fit, SysWOW64 will fit as well
                            memmove(dirName + strlen(dirName), path + len, strlen(path + len) + 1);
                            path = dirName;
                        }
                    }
                }
            }
        }
#endif // _WIN64

        char itemName[MAX_PATH];
        itemName[0] = 0;
        if (FocusedIndex < Dirs->Count + Files->Count)
        {
            CFileData* item = (FocusedIndex < Dirs->Count) ? &Dirs->At(FocusedIndex) : &Files->At(FocusedIndex - Dirs->Count);
            // hack for people who need to focus a Unicode name in Explorer; we try it via the short name
            AlterFileName(itemName, item->DosName != NULL ? item->DosName : item->Name, -1, Configuration.FileNameFormat, 0, FocusedIndex < Dirs->Count);
            if (FocusedIndex < Dirs->Count && FocusedIndex == 0 && strcmp(itemName, "..") == 0)
                itemName[0] = 0;
        }

        OpenFolderAndFocusItem(HWindow, path, itemName);
    }
    else if (Is(ptPluginFS) &&
             GetPluginFS()->NotEmpty() &&
             GetPluginFS()->IsServiceSupported(FS_SERVICE_OPENACTIVEFOLDER))
    {
        UserWorkedOnThisPath = TRUE;
        GetPluginFS()->OpenActiveFolder(GetPluginFS()->GetPluginFSName(), HWindow);
    }
}

BOOL CFilesWindow::CommonRefresh(HWND parent, int suggestedTopIndex, const char* suggestedFocusName,
                                 BOOL refreshListBox, BOOL readDirectory, BOOL isRefresh)
{
    CALL_STACK_MESSAGE6("CFilesWindow::CommonRefresh(, %d, %s, %d, %d, %d)", suggestedTopIndex,
                        suggestedFocusName, refreshListBox, readDirectory, isRefresh);

    //TRACE_I("common refresh: begin");
    if (readDirectory) // if only the top index and focus name should be reflected, this is not needed (could even be harmful, so we do not call it)
    {
        DirectoryLineSetText();
        if (Parent->GetActivePanel() == this)
        {
            Parent->EditWindowSetDirectory();
        }
        if (Parent->DetachedPanels && IsRightPanel())
            Parent->UpdateDetachedCommandLine();
    }

    //TRACE_I("read directory: begin");
    BOOL ret = FALSE;
    if (!readDirectory || ReadDirectory(parent, isRefresh))
        ret = TRUE;
    else
    {
        if (Is(ptDisk) || Is(ptZIPArchive))
            DetachDirectory(this); // something went wrong
    }
    //TRACE_I("read directory: begin");

    if (refreshListBox)
    {
        // find the item that should be selected
        int suggestedFocusIndex = -1;
        int suggestedFocusIndexIgnCase = -1;
        if (suggestedFocusName != NULL)
        {
            int i;
            for (i = 0; i < Dirs->Count; i++)
            {
                if (StrICmp(Dirs->At(i).Name, suggestedFocusName) == 0)
                {
                    if (suggestedFocusIndexIgnCase == -1)
                        suggestedFocusIndexIgnCase = i;
                    if (strcmp(Dirs->At(i).Name, suggestedFocusName) == 0)
                    {
                        suggestedFocusIndex = i;
                        break; // found the exact requested name
                    }
                }
            }
            if (suggestedFocusIndex == -1) // search among files as well (e.g., when returning from a ZIP archive)
            {
                for (i = 0; i < Files->Count; i++)
                {
                    if (StrICmp(Files->At(i).Name, suggestedFocusName) == 0)
                    {
                        if (suggestedFocusIndexIgnCase == -1)
                            suggestedFocusIndexIgnCase = i + Dirs->Count;
                        if (strcmp(Files->At(i).Name, suggestedFocusName) == 0)
                        {
                            suggestedFocusIndex = i + Dirs->Count;
                            break; // found the exact requested name
                        }
                    }
                }
            }
            // if the exact requested name was not found, use the name matching aside from case (if any)
            if (suggestedFocusIndex == -1)
                suggestedFocusIndex = suggestedFocusIndexIgnCase;
        }

        //TRACE_I("refresh listbox: begin");
        RefreshListBox(0, suggestedTopIndex, suggestedFocusIndex, TRUE, !isRefresh);
        //TRACE_I("refresh listbox: end");
    }

    if (Parent != NULL)
        Parent->UpdatePanelTabTitle(this);

    DirectoryLine->InvalidateIfNeeded();
    //TRACE_I("common refresh: end");
    return ret;
}
