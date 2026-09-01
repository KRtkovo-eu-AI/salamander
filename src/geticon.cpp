// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"
#include "commoncontrols.h"
#include <shlwapi.h>

#include <string>
#include <vector>

static UINT PrivateExtractSingleIcon(LPCTSTR fileName, int iconIndex, int iconSize, HICON* hIcon, UINT* iconID, UINT flags)
{
    typedef UINT(WINAPI * FPrivateExtractIcons)(LPCTSTR, int, int, int, HICON*, UINT*, UINT, UINT);
    static FPrivateExtractIcons privateExtractIcons = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandle("user32.dll");
        if (user32 != NULL)
            privateExtractIcons = (FPrivateExtractIcons)GetProcAddress(user32, "PrivateExtractIconsA");
        loaded = TRUE;
    }

    if (privateExtractIcons == NULL || hIcon == NULL || iconSize <= 0)
        return 0;

    *hIcon = NULL;
    UINT count = privateExtractIcons(fileName, iconIndex, iconSize, iconSize, hIcon, iconID, 1, flags);
    return count == (UINT)-1 ? 0 : count;
}

UINT WINAPI ExtractIcons(LPCTSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON* phicon, UINT* piconid, UINT nIcons, UINT flags)
{
    int largeIconSize = LOWORD(cxIcon);
    int smallIconSize = HIWORD(cxIcon);
    if (largeIconSize == 0)
        largeIconSize = cxIcon;

    if (phicon != NULL)
    {
        phicon[0] = NULL;
        if (nIcons > 1)
            phicon[1] = NULL;
    }

    // Prefer exact-size extraction. SHDefExtractIcon may return an already-scaled
    // shell bitmap from a high-DPI image list, while PrivateExtractIcons asks for
    // the concrete size we need (for example the native 16x16 group image at 100%).
    UINT extracted = 0;
    if (phicon != NULL)
    {
        extracted += PrivateExtractSingleIcon(szFileName, nIconIndex, largeIconSize, &phicon[0], piconid, flags) > 0 ? 1 : 0;
        if (nIcons > 1 && smallIconSize > 0)
        {
            UINT* smallIconID = piconid != NULL ? piconid + 1 : NULL;
            extracted += PrivateExtractSingleIcon(szFileName, nIconIndex, smallIconSize, &phicon[1], smallIconID, flags) > 0 ? 1 : 0;
        }
        BOOL exactExtractionComplete = nIcons > 1 ? phicon[0] != NULL && phicon[1] != NULL : phicon[0] != NULL;
        if (exactExtractionComplete)
            return extracted;
        if (phicon[0] != NULL)
        {
            HANDLES(DestroyIcon(phicon[0]));
            phicon[0] = NULL;
        }
        if (nIcons > 1 && phicon[1] != NULL)
        {
            HANDLES(DestroyIcon(phicon[1]));
            phicon[1] = NULL;
        }
    }

    UINT nIconSize = cxIcon;
    HICON hLarge{};
    HICON hSmall{};
    auto shRet = SHDefExtractIcon(szFileName, nIconIndex, 0, &hLarge, &hSmall, nIconSize);
    if (shRet == S_OK)
    {
        if (phicon)
        {
            phicon[0] = hLarge;
            if (nIcons == 2)
            {
                phicon[1] = hSmall;
            }
            else
            {
                if (hSmall != 0)
                {
                    DestroyIcon(hSmall);
                }
            }
        }
    }
    return shRet == S_OK ? 1 : 0;
}

STDAPI SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void** ppv, LPCITEMIDLIST* ppidlLast)
{
    return SHBindToFolderIDListParent(NULL, pidl, riid, ppv, ppidlLast);
}

BOOL OnExtList(LPCTSTR pszExtList, LPCTSTR pszExt)
{
    for (; *pszExtList; pszExtList += lstrlen(pszExtList) + 1)
    {
        if (!lstrcmpi(pszExt, pszExtList))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ExtIsExe(LPCTSTR szExt)
{
    return OnExtList("cmd\0bat\0pif\0scf\0exe\0com\0scr\0", szExt);
}

#define SHIL_LARGE 0      // The image size is normally 32x32 pixels. However, if the Use large icons option is selected from the Effects section of the Appearance tab in Display Properties, the image is 48x48 pixels.
#define SHIL_SMALL 1      // These images are the Shell standard small icon size of 16x16, but the size can be customized by the user.
#define SHIL_EXTRALARGE 2 // These images are the Shell standard extra-large icon size. This is typically 48x48, but the size can be customized by the user.
#define SHIL_SYSSMALL 3   // These images are the size specified by GetSystemMetrics called with SM_CXSMICON and GetSystemMetrics called with SM_CYSMICON.
#define SHIL_JUMBO 4      // Windows Vista and later. The image is normally 256x256 pixels.
// regarding icon sizes on Windows Vista: see "Creating a DPI-Aware Application" (http://msdn.microsoft.com/en-us/library/ms701681(VS.85).aspx)

static int GetIconPixelWidth(HICON hIcon)
{
    if (hIcon == NULL)
        return 0;

    ICONINFO iconInfo;
    memset(&iconInfo, 0, sizeof(iconInfo));
    if (!GetIconInfo(hIcon, &iconInfo))
        return 0;

    BITMAP bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    int width = 0;
    HBITMAP hBitmap = iconInfo.hbmColor != NULL ? iconInfo.hbmColor : iconInfo.hbmMask;
    if (hBitmap != NULL && GetObject(hBitmap, sizeof(bitmap), &bitmap) == sizeof(bitmap))
        width = bitmap.bmWidth;

    if (iconInfo.hbmColor != NULL)
        HANDLES(DeleteObject(iconInfo.hbmColor));
    if (iconInfo.hbmMask != NULL)
        HANDLES(DeleteObject(iconInfo.hbmMask));

    return width;
}

static HICON GetShellImageListIcon(int imageListSize, int iconIndex)
{
    IImageList* imageList = NULL;
    HICON hIcon = NULL;
    HRESULT hres = SHGetImageList(imageListSize, IID_IImageList, (void**)&imageList);
    if (SUCCEEDED(hres) && imageList != NULL)
    {
        if (imageList->GetIcon(iconIndex, ILD_NORMAL, &hIcon) != S_OK)
            hIcon = NULL;
        imageList->Release();
    }
    return hIcon;
}

static BOOL IsSolidBlackIcon(HICON icon, int pixelSize)
{
    if (icon == NULL || pixelSize <= 0)
        return FALSE;

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = pixelSize;
    bi.bmiHeader.biHeight = -pixelSize;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    DWORD* bits = NULL;
    HDC screenDC = GetDC(NULL);
    HBITMAP bitmap = CreateDIBSection(screenDC, &bi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    HDC dc = bitmap != NULL ? CreateCompatibleDC(screenDC) : NULL;
    ReleaseDC(NULL, screenDC);
    if (bitmap == NULL || dc == NULL)
    {
        if (dc != NULL)
            DeleteDC(dc);
        if (bitmap != NULL)
            DeleteObject(bitmap);
        return FALSE;
    }

    HBITMAP oldBitmap = (HBITMAP)SelectObject(dc, bitmap);
    // Draw on an opaque background.  A transparent white background can make a
    // valid alpha icon render as all-zero RGB and trigger the black-icon fallback.
    for (int i = 0; i < pixelSize * pixelSize; ++i)
        bits[i] = 0xffffffff;
    BOOL solidBlack = FALSE;
    if (DrawIconEx(dc, 0, 0, icon, pixelSize, pixelSize, 0, NULL, DI_NORMAL))
    {
        GdiFlush();
        solidBlack = TRUE;
        for (int i = 0; i < pixelSize * pixelSize; ++i)
        {
            if ((bits[i] & 0x00ffffff) != 0)
            {
                solidBlack = FALSE;
                break;
            }
        }
    }

    SelectObject(dc, oldBitmap);
    DeleteDC(dc);
    DeleteObject(bitmap);
    return solidBlack;
}

static void DiscardSolidBlackIcon(HICON* icon, int pixelSize)
{
    if (icon != NULL && *icon != NULL && IsSolidBlackIcon(*icon, pixelSize))
    {
        HANDLES(DestroyIcon(*icon));
        *icon = NULL;
    }
}

static std::wstring PanelPathToWide(const char* path)
{
    if (path == NULL || path[0] == 0)
        return std::wstring();

    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int length = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    if (length == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        length = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    }
    if (length <= 1)
        return std::wstring();

    std::vector<wchar_t> wide(length);
    if (MultiByteToWideChar(codePage, flags, path, -1, wide.data(), length) == 0)
        return std::wstring();
    return std::wstring(wide.data());
}

static HICON GetExplorerFileIcon(const char* path, int pixelSize, BOOL smallIcon)
{
    if (path == NULL || pixelSize <= 0)
        return NULL;

    std::wstring widePath = PanelPathToWide(path);
    if (widePath.empty())
        return NULL;

    SHFILEINFOW fileInfo;
    ZeroMemory(&fileInfo, sizeof(fileInfo));
    UINT iconFlags = SHGFI_ICON | (smallIcon ? SHGFI_SMALLICON : SHGFI_LARGEICON);
    if (SHGetFileInfoW(widePath.c_str(), 0, &fileInfo, sizeof(fileInfo), iconFlags) == 0 ||
        fileInfo.hIcon == NULL)
        return NULL;

    HICON icon = fileInfo.hIcon;
    if (GetIconPixelWidth(icon) != pixelSize)
    {
        HICON resized = (HICON)CopyImage(icon, IMAGE_ICON, pixelSize, pixelSize, 0);
        if (resized == NULL)
        {
            HANDLES(DestroyIcon(icon));
            return NULL;
        }
        HANDLES(DestroyIcon(icon));
        icon = resized;
    }

    DiscardSolidBlackIcon(&icon, pixelSize);
    return icon;
}

static HICON GetDefaultAssociationIcon(const char* path, int pixelSize)
{
    if (path == NULL || pixelSize <= 0)
        return NULL;

    const char* name = path;
    for (const char* p = path; *p != 0; ++p)
    {
        if (*p == '\\' || *p == '/')
            name = p + 1;
    }
    const char* extension = strrchr(name, '.');
    if (extension == NULL || extension[1] == 0)
        return NULL;

    std::wstring wideExtension = PanelPathToWide(extension);
    if (wideExtension.empty())
        return NULL;

    DWORD length = 0;
    AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_DEFAULTICON, wideExtension.c_str(), NULL, NULL, &length);
    if (length <= 1)
        return NULL;

    std::vector<wchar_t> location(length);
    if (FAILED(AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_DEFAULTICON, wideExtension.c_str(),
                                 NULL, location.data(), &length)))
        return NULL;

    int iconIndex = PathParseIconLocationW(location.data());
    std::wstring iconPath;
    if (_wcsicmp(location.data(), L"%1") == 0)
        iconPath = PanelPathToWide(path);
    else
    {
        DWORD expandedLength = ExpandEnvironmentStringsW(location.data(), NULL, 0);
        if (expandedLength == 0)
            return NULL;
        std::vector<wchar_t> expanded(expandedLength);
        if (ExpandEnvironmentStringsW(location.data(), expanded.data(), expandedLength) == 0)
            return NULL;
        iconPath.assign(expanded.data());
    }
    if (iconPath.empty())
        return NULL;

    HICON largeIcon = NULL;
    HICON smallIcon = NULL;
    HRESULT result = SHDefExtractIconW(iconPath.c_str(), iconIndex, 0, &largeIcon, &smallIcon,
                                       MAKELONG(pixelSize, pixelSize));
    if (FAILED(result))
        return NULL;

    HICON icon = smallIcon != NULL ? smallIcon : largeIcon;
    if (largeIcon != NULL && largeIcon != icon)
        HANDLES(DestroyIcon(largeIcon));
    if (smallIcon != NULL && smallIcon != icon)
        HANDLES(DestroyIcon(smallIcon));
    if (icon != NULL && GetIconPixelWidth(icon) != pixelSize)
    {
        HICON resized = (HICON)CopyImage(icon, IMAGE_ICON, pixelSize, pixelSize, 0);
        if (resized == NULL)
        {
            HANDLES(DestroyIcon(icon));
            return NULL;
        }
        HANDLES(DestroyIcon(icon));
        icon = resized;
    }
    DiscardSolidBlackIcon(&icon, pixelSize);
    return icon;
}

BOOL SalGetIconFromPIDL(IShellFolder* psf, const char* path, LPCITEMIDLIST pidl, HICON* hIcon,
                        CIconSizeEnum iconSize, BOOL fallbackToDefIcon, BOOL defIconIsDir)
{
    BOOL ret = FALSE;

    IExtractIconA* pxi = NULL; // if 'isIExtractIconW' is TRUE, this pointer is actually IExtractIconW
    BOOL isIExtractIconW = FALSE;
    HICON hIconSmall = NULL;
    HICON hIconLarge = NULL;

    char iconFile[MAX_PATH];
    WCHAR iconFileW[MAX_PATH];
    int iconIndex;
    UINT wFlags = 0; // clear because the DWGIcon.dll shell extension just ORs these bits

    CIconSizeEnum largeIconSize = ICONSIZE_32;
    if (iconSize == ICONSIZE_48)
        largeIconSize = ICONSIZE_48;
    BOOL preferExtractorSmallIcon = iconSize == ICONSIZE_16 && IconSizes[ICONSIZE_16] <= 16;

    HRESULT hres = psf->GetUIObjectOf(NULL, 1, &pidl, IID_IExtractIconA, NULL, (void**)&pxi);
    if (SUCCEEDED(hres))
    {
        hres = pxi->GetIconLocation(GIL_FORSHELL, iconFile, MAX_PATH, &iconIndex, &wFlags);
        //TRACE_I("  SalGetIconFromPIDL() IID_IExtractIconA iconFile="<<iconFile<<" iconIndex="<<iconIndex<<" wFlags="<<wFlags);
    }
    else
    {
        // The ANSI version failed, so we try the UNICODE IID_IExtractIcon variant
        hres = psf->GetUIObjectOf(NULL, 1, &pidl, IID_IExtractIconW, NULL, (void**)&pxi);
        if (SUCCEEDED(hres))
        {
            isIExtractIconW = TRUE;
            hres = ((IExtractIconW*)pxi)->GetIconLocation(GIL_FORSHELL, iconFileW, MAX_PATH, &iconIndex, &wFlags);
            if (SUCCEEDED(hres))
            {
                // Convert the UNICODE string to ANSI
                WideCharToMultiByte(CP_ACP, 0, iconFileW, -1, iconFile, MAX_PATH, NULL, NULL);
                iconFile[MAX_PATH - 1] = 0;
                //TRACE_I("  SalGetIconFromPIDL() IID_IExtractIconW iconFile="<<iconFile<<" iconIndex="<<iconIndex<<" wFlags="<<wFlags);
            }
        }
    }
    //  TRACE_I("iconFile="<<iconFile<<" iconIndex="<<iconIndex);
    if (SUCCEEDED(hres))
    {
        // on XP we can obtain the 48x48 system image list (Extract() incorrectly returns 32x32)
        // another way to get 48x48 icons is LoadImage, but we would need the file path and icon number
        // a "*" in the file name means iconIndex already refers to a system icon index
        //TRACE_I("  SalGetIconFromPIDL() wFlags="<<wFlags<<" iconFile='"<<iconFile<<"' TryObtainGetImageList="<<TryObtainGetImageList);
        if ((wFlags & GIL_NOTFILENAME) && iconFile[0] == '*' && iconFile[1] == 0)
        {
            // multiple attempts helped JIS, but if icon extraction keeps failing
            // we would waste 50 ms on each icon retrieval for no reason
            // Petr moved the retry logic to IconReader, so we fail here and return FALSE
            // icons will be retried after 500 ms
            //      int attempt = 1;
            //AGAIN:
            // ***** hIconSmall ******
            //TRACE_I("  SalGetIconFromPIDL() Asking system image list '*' for iconIndex="<<iconIndex);
            IImageList* imageListSmall = NULL;
            // Use the shell image list that matches the requested pixel size.
            // At 100% DPI we want the real 16x16 artwork from SHIL_SMALL, not a
            // 24x24 SYSSMALL icon from a primary high-DPI monitor scaled down.
            // At higher DPI, SYSSMALL can provide the DPI-sized small icon and
            // avoids upscaling the 16x16 variant.
            int smallImageListSize = IconSizes[ICONSIZE_16] <= 16 ? SHIL_SMALL : SHIL_SYSSMALL;
            hres = SHGetImageList(smallImageListSize, IID_IImageList, (void**)&imageListSmall);
            if (SUCCEEDED(hres) && (imageListSmall != NULL))
            {
                if (imageListSmall->GetIcon(iconIndex, ILD_NORMAL, &hIconSmall) != S_OK)
                    hIconSmall = NULL;
                // A successful image-list call can still return an all-black
                // icon.  Treat that as extraction failure here so the direct
                // SHGFI_ICON fallback below gets a chance to supply the same
                // usable HICON that Explorer draws.
                DiscardSolidBlackIcon(&hIconSmall, IconSizes[ICONSIZE_16]);
                //TRACE_I("  SalGetIconFromPIDL() SHIL_SMALL IID_IImageList, hIconSmall="<<hIconSmall);
                imageListSmall->Release();
            }
            if (hIconSmall == NULL)
            {
                // With old man Bill, GetImageList fails and pxi->Extract() then returns NULL icon handles
                //TRACE_I("  SalGetIconFromPIDL() SHIL_SMALL IID_IImageList was not obtained!");
                if (path != NULL)
                {
                    // Try asking the system for the system image list index and use that handle to extract the icon
                    SHFILEINFO sfi;
                    ZeroMemory(&sfi, sizeof(sfi));
                    HIMAGELIST hSysImageList = (HIMAGELIST)SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON); // returns a persistent handle, no need to release
                    if (hSysImageList != NULL)
                    {
                        hIconSmall = ImageList_GetIcon(hSysImageList, sfi.iIcon, ILD_NORMAL);
                        DiscardSolidBlackIcon(&hIconSmall, IconSizes[ICONSIZE_16]);
                        //TRACE_I("  SalGetIconFromPIDL() ImageList_GetIcon for SHGFI_SMALLICON hIconSmall="<<hIconSmall<<" sfi.iIcon="<<sfi.iIcon<<" hSysImageList="<<hSysImageList);
                    }
                    if (hIconSmall == NULL)
                    {
                        // Try asking directly for the icon
                        ZeroMemory(&sfi, sizeof(sfi));
                        if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON) != 0)
                            hIconSmall = sfi.hIcon;
                        //TRACE_I("  SalGetIconFromPIDL() SHGetFileInfo for SHGFI_ICON | SHGFI_SMALLICON hIconSmall="<<hIconSmall);
                    }
                    if (hIconSmall == NULL || IsSolidBlackIcon(hIconSmall, IconSizes[ICONSIZE_16]))
                    {
                        HICON associationIcon = GetDefaultAssociationIcon(path, IconSizes[ICONSIZE_16]);
                        if (associationIcon != NULL &&
                            !IsSolidBlackIcon(associationIcon, IconSizes[ICONSIZE_16]))
                        {
                            if (hIconSmall != NULL)
                                HANDLES(DestroyIcon(hIconSmall));
                            hIconSmall = associationIcon;
                        }
                        else if (associationIcon != NULL)
                            HANDLES(DestroyIcon(associationIcon));
                    }
                }
                //else TRACE_I("  SalGetIconFromPIDL() path == NULL");
            }
            // ***** hIconLarge ******
            if (iconSize == ICONSIZE_48)
            {
                IImageList* imageListExtraLarge = NULL;
                if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, (void**)&imageListExtraLarge)) && (imageListExtraLarge != NULL))
                {
                    if (imageListExtraLarge->GetIcon(iconIndex, ILD_NORMAL, &hIconLarge) != S_OK)
                        hIconLarge = NULL;
                    //TRACE_I("  SalGetIconFromPIDL() SHIL_EXTRALARGE IID_IImageList, hIconLarge="<<hIconLarge);
                    imageListExtraLarge->Release();
                }
            }
            else // ICONSIZE_16 || ICONSIZE_32
            {
                IImageList* imageListLarge = NULL;
                hres = SHGetImageList(SHIL_LARGE, IID_IImageList, (void**)&imageListLarge);
                if (SUCCEEDED(hres) && (imageListLarge != NULL))
                {
                    if (imageListLarge->GetIcon(iconIndex, ILD_NORMAL, &hIconLarge) != S_OK)
                        hIconLarge = NULL;
                    //TRACE_I("  SalGetIconFromPIDL() SHIL_LARGE IID_IImageList, hIconLarge="<<hIconLarge);
                    imageListLarge->Release();
                }
                if (hIconLarge == NULL)
                {
                    // With old man Bill, GetImageList fails and pxi->Extract() then returns NULL icon handles
                    //TRACE_I("  SalGetIconFromPIDL() SHIL_LARGE IID_IImageList was not obtained!");
                    if (path != NULL)
                    {
                        // Try asking the system for the system image list index and use that handle to extract the icon
                        SHFILEINFO sfi;
                        ZeroMemory(&sfi, sizeof(sfi));
                        HIMAGELIST hSysImageList = (HIMAGELIST)SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_ICON);
                        if (hSysImageList != NULL)
                        {
                            hIconLarge = ImageList_GetIcon(hSysImageList, sfi.iIcon, ILD_NORMAL);
                            //TRACE_I("  SalGetIconFromPIDL() ImageList_GetIcon for SHGFI_ICON hIconLarge="<<hIconLarge<<" sfi.iIcon="<<sfi.iIcon<<" hSysImageList="<<hSysImageList);
                        }
                        if (hIconLarge == NULL)
                        {
                            // Try asking directly for the icon
                            ZeroMemory(&sfi, sizeof(sfi));
                            if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON) != 0)
                                hIconLarge = sfi.hIcon;
                            //TRACE_I("  SalGetIconFromPIDL() SHGetFileInfo for SHGFI_ICON | SHGFI_LARGEICON hIconLarge="<<hIconLarge);
                        }
                    }
                    //else TRACE_I("  SalGetIconFromPIDL() path == NULL");
                }
            }
            //      // if we failed to extract any icon
            //      if (hIconSmall == NULL && hIconLarge == NULL && attempt <= 3)
            //      {
            //        // try again up to three times with a 50 ms delay
            //        //TRACE_I("  SalGetIconFromPIDL() Sleeping and trying again. Attempt="<<attempt);
            //        Sleep(50);
            //        attempt++;
            //        goto AGAIN;
            //      }
        }

        // For 100% DPI prefer extracting from the icon resource file first.
        // Shell-provided small image lists may already be initialized for a
        // high-DPI monitor and can contain a downscaled 24px design at 16px.
        if (preferExtractorSmallIcon && hIconSmall == NULL && hIconLarge == NULL && !(wFlags & GIL_NOTFILENAME))
        {
            HICON hIcons[2] = {0, 0};
            UINT u = ExtractIcons(iconFile, iconIndex, MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]),
                                  MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]), hIcons, NULL, 2, IconLRFlags);
            if (u != -1)
            {
                hIconLarge = hIcons[0];
                hIconSmall = hIcons[1];
            }
            //TRACE_I("  SalGetIconFromPIDL() ExtractIcons hIconLarge="<<hIconLarge<<" hIconSmall="<<hIconSmall);
        }

        // if the icon was not taken from the system image list, ask the pxi interface for it
        if (hIconSmall == NULL && hIconLarge == NULL)
        {
            // try IExtractIcon::Extract()
            // Note: if iconFile == '*', Extract sometimes returns valid icons but in some implementations it doesn't,
            // leaving users with default icons; see below
            if (isIExtractIconW)
                hres = ((IExtractIconW*)pxi)->Extract(iconFileW, iconIndex, &hIconLarge, &hIconSmall, MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]));
            else
                hres = pxi->Extract(iconFile, iconIndex, &hIconLarge, &hIconSmall, MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]));
            //TRACE_I("  SalGetIconFromPIDL() pxi->Extract() hIconLarge="<<hIconLarge<<" hIconSmall="<<hIconSmall<<" isIExtractIconW="<<isIExtractIconW);
            // WARNING: for *.ai files iconFile==0 and iconIndex==0 yet Extract() still returns an icon (Adobe Illustrator shell extension)
            // WARNING: D:\Store\Salamand\ICO_SONY\SonyF707_Day_Flash.icc returns hIconLarge==hIconSmall, both 32x32
        }

        // if the icon is stored in a file, we can attempt to retrieve it ourselves via ExtractIcons()
        if (hIconSmall == NULL && hIconLarge == NULL && !(wFlags & GIL_NOTFILENAME))
        {
            HICON hIcons[2] = {0, 0};
            UINT u = ExtractIcons(iconFile, iconIndex, MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]), MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]), hIcons, NULL, 2, IconLRFlags);
            if (u != -1)
            {
                hIconLarge = hIcons[0];
                hIconSmall = hIcons[1];
            }
            //TRACE_I("  SalGetIconFromPIDL() ExtractIcons hIconLarge="<<hIconLarge<<" hIconSmall="<<hIconSmall);
        }

    }
    HICON* requestedIcon = iconSize == ICONSIZE_16 ? &hIconSmall : &hIconLarge;
    HICON* otherIcon = iconSize == ICONSIZE_16 ? &hIconLarge : &hIconSmall;
    int requestedIconSize = IconSizes[iconSize];
    if (*requestedIcon != NULL && GetIconPixelWidth(*requestedIcon) != requestedIconSize && pxi != NULL)
    {
        // If the process first touched a shell image list on another DPI, the
        // process-global bitmap can have the wrong size. Ask the item's
        // extractor directly for the requested icon before falling back to a
        // resize. This applies to small, large, and extra-large panel icons.
        HICON hExtractedLarge = NULL;
        HICON hExtractedSmall = NULL;
        HRESULT extractResult;
        if (isIExtractIconW)
            extractResult = ((IExtractIconW*)pxi)->Extract(iconFileW, iconIndex, &hExtractedLarge, &hExtractedSmall,
                                                           MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]));
        else
            extractResult = pxi->Extract(iconFile, iconIndex, &hExtractedLarge, &hExtractedSmall,
                                         MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]));

        HICON hExtracted = iconSize == ICONSIZE_16 ? hExtractedSmall : hExtractedLarge;
        if (SUCCEEDED(extractResult) && hExtracted != NULL &&
            GetIconPixelWidth(hExtracted) == requestedIconSize)
        {
            if (*requestedIcon != NULL && *requestedIcon != *otherIcon)
                HANDLES(DestroyIcon(*requestedIcon));
            *requestedIcon = hExtracted;
            if (iconSize == ICONSIZE_16)
                hExtractedSmall = NULL;
            else
                hExtractedLarge = NULL;
        }

        if (hExtractedSmall != NULL && hExtractedSmall != hIconSmall && hExtractedSmall != hIconLarge)
            HANDLES(DestroyIcon(hExtractedSmall));
        if (hExtractedLarge != NULL && hExtractedLarge != hIconSmall && hExtractedLarge != hIconLarge)
            HANDLES(DestroyIcon(hExtractedLarge));
    }

    if (pxi != NULL)
    {
        if (isIExtractIconW)
            ((IExtractIconW*)pxi)->Release();
        else
            pxi->Release();
    }

    // none of the methods worked, so return the default icon
    if (fallbackToDefIcon && hIconSmall == NULL && hIconLarge == NULL)
    {
        BOOL fileIsExecutable = FALSE;
        if (!defIconIsDir && path != NULL)
        {
            const char* name = strrchr(path, '\\');
            const char* ext = name != NULL ? strrchr(name + 1, '.') : NULL;
            //      if (ext > path && *(ext - 1) != '\\')    // ".cvspass" is an extension in Windows ...
            if (ext != NULL)
                fileIsExecutable = ExtIsExe(ext + 1);
        }

        int resID;
        if (WindowsVistaAndLater)
            resID = defIconIsDir ? 4 : (fileIsExecutable ? 15 : 2); // symbolsDirectory : symbolsExecutable : symbolsNonAssociated
        else
            resID = defIconIsDir ? 4 : (fileIsExecutable ? 3 : 1); // symbolsDirectory : symbolsExecutable : symbolsNonAssociated
        HICON hIcons[2] = {0, 0};
        UINT u = ExtractIcons(WindowsVistaAndLater ? "imageres.dll" : "shell32.dll", -resID,
                              MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]), MAKELONG(IconSizes[largeIconSize], IconSizes[ICONSIZE_16]),
                              hIcons, NULL, 2, IconLRFlags);
        if (u != -1)
        {
            hIconLarge = hIcons[0];
            hIconSmall = hIcons[1];
        }
        //TRACE_I("  SalGetIconFromPIDL() DEFAULT ICON ExtractIcons hIconLarge="<<hIconLarge<<" hIconSmall="<<hIconSmall);
    }

    if (hIconLarge != NULL || hIconSmall != NULL)
    {
        ret = TRUE;
        // Use a real icon for the current DPI. IExtractIcon::Extract() and
        // shell image lists can return a bitmap for a different DPI than the
        // panel currently uses. Normalize every supported icon size here
        // instead of allowing a stale size to enter the icon cache.
        if (iconSize == ICONSIZE_16)
        {
            int targetIconSize = IconSizes[ICONSIZE_16];
            int smallIconSize = GetIconPixelWidth(hIconSmall);
            if (hIconSmall == NULL || hIconSmall == hIconLarge || smallIconSize != targetIconSize)
            {
                HICON hIconSource = hIconLarge != NULL ? hIconLarge : hIconSmall;
                // Do not use LR_COPYFROMRESOURCE here: handles taken from the
                // shell image list are already concrete bitmaps, and that flag
                // may keep their original size when we need to shrink 24 -> 16.
                HICON hIconDPI = hIconSource != NULL ?
                                     (HICON)CopyImage(hIconSource, IMAGE_ICON, targetIconSize, targetIconSize, 0) :
                                     NULL;
                if (hIconDPI != NULL)
                {
                    if (hIconSmall != NULL && hIconSmall != hIconLarge)
                        HANDLES(DestroyIcon(hIconSmall));
                    hIconSmall = hIconDPI;
                }
                //TRACE_I("  SalGetIconFromPIDL() CopyImage 1 hIconSmall="<<hIconSmall<<" hIconLarge="<<hIconLarge);
            }
            *hIcon = hIconSmall;
            if (hIconLarge != NULL)
            {
                DestroyIcon(hIconLarge);
                hIconLarge = NULL;
            }
        }
        else // ICONSIZE_32 || ICONSIZE_48
        {
            // if the large icon is missing or we were given the handle of the small one, create it
            if (hIconLarge == NULL || hIconSmall == hIconLarge ||
                GetIconPixelWidth(hIconLarge) != requestedIconSize)
            {
                HICON hIconSource = hIconLarge != NULL ? hIconLarge : hIconSmall;
                HICON hIconDPI = hIconSource != NULL ?
                                     (HICON)CopyImage(hIconSource, IMAGE_ICON, requestedIconSize, requestedIconSize, 0) :
                                     NULL;
                if (hIconDPI != NULL)
                {
                    if (hIconLarge != NULL && hIconLarge != hIconSmall)
                        HANDLES(DestroyIcon(hIconLarge));
                    hIconLarge = hIconDPI;
                }
                //TRACE_I("  SalGetIconFromPIDL() CopyImage 2 hIconSmall="<<hIconSmall<<" hIconLarge="<<hIconLarge);
            }
            *hIcon = hIconLarge;
            if (hIconSmall != NULL)
            {
                DestroyIcon(hIconSmall);
                hIconSmall = NULL;
            }
        }
    }

    // Do not let a stale/corrupt shell image-list entry poison Salamander's
    // icon caches. Try the system list once more, otherwise keep the caller's
    // existing association/default icon by reporting extraction failure.
    if (ret && path != NULL && *hIcon != NULL &&
        IsSolidBlackIcon(*hIcon, requestedIconSize))
    {
        HICON fallbackIcon = GetExplorerFileIcon(path, requestedIconSize, iconSize == ICONSIZE_16);
        if (fallbackIcon == NULL)
            fallbackIcon = GetDefaultAssociationIcon(path, requestedIconSize);
        if (fallbackIcon != NULL &&
            !IsSolidBlackIcon(fallbackIcon, requestedIconSize))
        {
            HANDLES(DestroyIcon(*hIcon));
            *hIcon = fallbackIcon;
        }
        else
        {
            if (fallbackIcon != NULL)
                HANDLES(DestroyIcon(fallbackIcon));
            HANDLES(DestroyIcon(*hIcon));
            *hIcon = NULL;
            ret = FALSE;
        }
    }

    return ret;
}

LPITEMIDLIST SHILCreateFromPath(LPCSTR pszPath)
{
    LPITEMIDLIST pidl = NULL;
    IShellFolder* psfDesktop;
    if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
    {
        ULONG cchEaten;
        WCHAR wszPath[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, pszPath, -1, wszPath, MAX_PATH);
        wszPath[MAX_PATH - 1] = 0;

        psfDesktop->ParseDisplayName(NULL, NULL, wszPath, &cchEaten, &pidl, NULL);

        psfDesktop->Release();
    }
    return pidl;
}

// comment see spl_gen.h/GetFileIcon

static BOOL IsIconFilePath(LPCTSTR path)
{
    if (path == NULL)
        return FALSE;

    LPCTSTR slash = strrchr(path, '\\');
    LPCTSTR slash2 = strrchr(path, '/');
    if (slash2 != NULL && (slash == NULL || slash2 > slash))
        slash = slash2;

    LPCTSTR dot = strrchr(path, '.');
    return dot != NULL && (slash == NULL || dot > slash) && stricmp(dot + 1, "ico") == 0;
}

static BOOL LoadIcoFileSmallIcon(LPCTSTR path, HICON* hIcon)
{
    if (hIcon == NULL)
        return FALSE;

    *hIcon = NULL;
    if (!IsIconFilePath(path))
        return FALSE;

    int iconSize = IconSizes[ICONSIZE_16];
    *hIcon = (HICON)HANDLES(LoadImage(NULL, path, IMAGE_ICON, iconSize, iconSize,
                                       LR_LOADFROMFILE | IconLRFlags));
    if (*hIcon != NULL)
        return TRUE;

    if (ExtractIcons(path, 0, iconSize, iconSize, hIcon, NULL, 1, IconLRFlags) == 1 && *hIcon != NULL)
        return TRUE;

    *hIcon = NULL;
    return FALSE;
}

BOOL GetFileIcon(const char* path, BOOL pathIsPIDL, HICON* hIcon, CIconSizeEnum iconSize,
                 BOOL fallbackToDefIcon, BOOL defIconIsDir)
{
    BOOL ret = FALSE;
    LPITEMIDLIST pidlFull;

    if (hIcon == NULL)
    {
        TRACE_E("hIcon == NULL");
        return FALSE;
    }
    /*
  if (!pathIsPIDL)
    TRACE_I("GetFileIcon() path="<<path<<" iconSize="<<iconSize);
  else
    TRACE_I("GetFileIcon() pathIsPIDL"); // not used by Salamander itself, only by the Folders plugin
*/
    if (!pathIsPIDL && iconSize == ICONSIZE_16 && LoadIcoFileSmallIcon(path, hIcon))
        return TRUE;

    // For ordinary small file icons use the same path-based shell result that
    // Explorer displays. IExtractIcon resource locations and registered
    // DefaultIcon values can both be valid yet select different artwork.
    if (!pathIsPIDL && iconSize == ICONSIZE_16)
    {
        *hIcon = GetExplorerFileIcon(path, IconSizes[ICONSIZE_16], TRUE);
        if (*hIcon != NULL)
            return TRUE;
    }

    if (!pathIsPIDL)
        pidlFull = SHILCreateFromPath(path);
    else
        pidlFull = (LPITEMIDLIST)path;

    if (pidlFull != NULL)
    {
        IShellFolder* psf;
        LPITEMIDLIST pidlLast;
        HRESULT hres = SHBindToIDListParent(pidlFull, IID_IShellFolder, (void**)&psf, (LPCITEMIDLIST*)&pidlLast);
        if (SUCCEEDED(hres))
        {
            // if we know the path, pass it to SalGetIconFromPIDL
            ret = SalGetIconFromPIDL(psf, pathIsPIDL ? NULL : path, pidlLast, hIcon, iconSize,
                                     fallbackToDefIcon, defIconIsDir);

            psf->Release();
        }

        if (!pathIsPIDL)
            ILFree(pidlFull);
    }

    return ret;
}
