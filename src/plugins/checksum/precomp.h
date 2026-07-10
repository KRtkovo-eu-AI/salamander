// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define WIN32_LEAN_AND_MEAN // exclude rarely-used stuff from Windows headers

#include <tchar.h>
#include <windows.h>
#include <CommDlg.h>
#include <crtdbg.h>
#include <ostream>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <limits.h>
#include <string>

#ifndef SAL_MAX_PATH
#define SAL_MAX_PATH 32768
#endif

#if defined(_DEBUG) && defined(_MSC_VER) // without passing file+line to 'new' operator, list of memory leaks shows only 'crtdbg.h(552)'
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "versinfo.rh2"

#include "spl_com.h"
#include "spl_crypt.h"
#include "spl_base.h"
#include "spl_file.h"
#include "spl_gen.h"
#include "spl_gui.h"
#include "spl_menu.h"
#include "spl_file.h"
#include "spl_vers.h"
#include "dbg.h"
#include "arraylt.h"
#include "mhandles.h"
#include "winliblt.h"
#include "auxtools.h"

static std::wstring PluginMultiByteToWidePath(const char* path, UINT codePage = CP_ACP)
{
    if (path == NULL || *path == 0)
        return std::wstring();
    if (codePage == CP_ACP && GetACP() == CP_UTF8)
        codePage = CP_UTF8;
    DWORD flags = codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    int len = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    if (len <= 0 && codePage != CP_ACP)
    {
        codePage = CP_ACP;
        flags = 0;
        len = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    }
    if (len <= 0)
        return std::wstring();
    std::wstring ret(len - 1, L'\0');
    MultiByteToWideChar(codePage, flags, path, -1, &ret[0], len);
    return ret;
}

static std::string PluginWideToMultiBytePath(const wchar_t* path, UINT codePage = CP_ACP)
{
    if (path == NULL || *path == 0)
        return std::string();
    if (codePage == CP_ACP && GetACP() == CP_UTF8)
        codePage = CP_UTF8;
    int len = WideCharToMultiByte(codePage, 0, path, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        return std::string();
    std::string ret(len - 1, '\0');
    WideCharToMultiByte(codePage, 0, path, -1, &ret[0], len, NULL, NULL);
    return ret;
}

static std::wstring PluginPathAddExtendedPrefixW(const wchar_t* path)
{
    if (path == NULL || *path == 0 || wcsncmp(path, L"\\\\?\\", 4) == 0)
        return path != NULL ? std::wstring(path) : std::wstring();
    if (wcsncmp(path, L"\\\\", 2) == 0)
        return std::wstring(L"\\\\?\\UNC\\") + (path + 2);
    return std::wstring(L"\\\\?\\") + path;
}
