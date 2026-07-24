// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define WIN32_LEAN_AND_MEAN // exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <CommDlg.h>
#include <ShellAPI.h>
#include <oleidl.h> // IDropTarget used by shared mhandles.h
#include <crtdbg.h>
#include <tchar.h>
#include <sys/stat.h>
#include <ostream>
#include <math.h>
#include <limits>
#include <commctrl.h>
#include <limits.h>
#include <vector>
#include <string>

#ifndef SAL_MAX_PATH
#define SAL_MAX_PATH 32768
#endif
#include <algorithm>
#include <map>
#include <zmouse.h>

#if defined(_DEBUG) && defined(_MSC_VER) // without passing file+line to 'new' operator, list of memory leaks shows only 'crtdbg.h(552)'
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// workaround for runtime check failures in the debug build: the original macro casts rgb to WORD,
// so it reports data loss (the RED component)
#undef GetGValue
#define GetGValue(rgb) ((BYTE)(((rgb) >> 8) & 0xFF))

#include "spl_com.h"
#include "spl_base.h"
#include "spl_gen.h"
#include "spl_menu.h"
#include "spl_gui.h"

#include "versinfo.rh2"

#include "dbg.h"
#include "arraylt.h"
#include "winliblt.h"
#include "../../common/winlibdpi.h"
#include "auxtools.h"
#include "mhandles.h"
#include "../../darkmode.h"

// get rid of some anoying warnings
#pragma warning(disable : 4661)
#pragma warning(disable : 4786)

// ****************************************************************************
//
// Cleanup after including the MS headers
//
#undef min
#undef max
// ****************************************************************************

#include "lcutils.h"
#include "str.h"
#include "diff.h"

//#include "counter.h"
//#include "profile.h"

#include "filecomp.rh"
#include "filecomp.rh2"
#include "lang\lang.rh"

#include "xunicode.h"
#include "mtxtout.h"
#include "filemap.h"
#include "filecache.h"
#include "textio.h"
#include "worker.h"
#include "cwbase.h"
#include "cwstrict.h"
#include "cwoptim.h"
#include "filecomp.h"
#include "dlg_com.h"
#include "controls.h"
#include "dialogs.h"
#include "viewwnd.h"
#include "viewtext.h"
#include "mainwnd.h"
#include "messages.h"
#include "remotmsg.h"
#include "remote.h"

#define SizeOf(x) (sizeof(x) / sizeof(x[0]))

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
