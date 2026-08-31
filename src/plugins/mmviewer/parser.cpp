// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "parser.h"
#include "output.h"
#include "renderer.h"
#include "mmviewer.h"
#include "mmviewer.rh"
#include "mmviewer.rh2"
#include "lang\lang.rh"
#include "taglibparser.h"

#ifdef _MP4_SUPPORT_
#include "mp4\\mp4parser.h"
#endif

#ifdef _MPG_SUPPORT_
#include "mp3\\mpgparser.h"
#endif

#ifdef _WAV_SUPPORT_
#include "wav\\wavparser.h"
#endif

#ifdef _WMA_SUPPORT_
#include "wma\\wmaparser.h"
#endif

#ifdef _VQF_SUPPORT_
#include "vqf\\vqfparser.h"
#endif

#ifdef _OGG_SUPPORT_
#include "ogg\\oggparser.h"
#endif

#ifdef _MOD_SUPPORT_
#include "mod\\modparser.h"
#endif

void ShowParserError(HWND hParent, CParserResultEnum result)
{
    int strID = -1;
    switch (result)
    {
    case preOK:
        return;
    case preOutOfMemory:
        strID = IDS_MMV_OOM;
        break;
    case preUnknownFile:
        strID = IDS_MMV_UNKNOWN_FILE;
        break;
    case preOpenError:
        strID = IDS_MMV_OPEN_ERROR;
        break;
    case preReadError:
        strID = IDS_MMV_READ_ERROR;
        break;
    case preWriteError:
        strID = IDS_MMV_WRITE_ERROR;
        break;
    case preSeekError:
        strID = IDS_MMV_SEEK_ERROR;
        break;
    case preCorruptedFile:
        strID = IDS_MMV_CORRUPTED_FILE;
        break;
    case preExtensionError:
        strID = IDS_MMV_EXTENSION_ERROR;
        break;
    }
    const char* text;
    if (strID == -1)
        text = "Unknown error";
    else
        text = LoadStr(strID);
    SalGeneral->SalMessageBox(hParent, text, LoadStr(IDS_PLUGIN_NAME), MB_OK | MB_ICONEXCLAMATION);
}

CParserResultEnum
CreateAppropriateParser(const char* fileName, CParserInterface** parser)
{
    CParserInterface* iface = NULL;
    BOOL extensionSupported = FALSE;

    const char* ext = strrchr(fileName, '.'); // ".cvspass" is an extension in Windows

    if (!ext)
        return preUnknownFile;

#ifdef _MP4_SUPPORT_
    if ((SalGeneral->StrICmp(ext, ".mp4") == 0) || (SalGeneral->StrICmp(ext, ".m4a") == 0) || (SalGeneral->StrICmp(ext, ".aac") == 0))
    {
        extensionSupported = TRUE;
        iface = new CParserMP4();
    }
#endif

#ifdef _MPG_SUPPORT_
    if ((SalGeneral->StrICmp(ext, ".mp3") == 0) || (SalGeneral->StrICmp(ext, ".mp2") == 0))
    {
        extensionSupported = TRUE;
        iface = new CParserMPG();
    }
#endif

#ifdef _WAV_SUPPORT_
    if ((SalGeneral->StrICmp(ext, ".wav") == 0) || (SalGeneral->StrICmp(ext, ".wave") == 0))
    {
        extensionSupported = TRUE;
        iface = new CParserWAV();
    }
#endif

#ifdef _WMA_SUPPORT_
    if (SalGeneral->StrICmp(ext, ".wma") == 0)
    {
        extensionSupported = TRUE;
        iface = new CParserWMA();
    }
#endif

#ifdef _VQF_SUPPORT_
    if (SalGeneral->StrICmp(ext, ".vqf") == 0)
    {
        extensionSupported = TRUE;
        iface = new CParserVQF();
    }
#endif

    static const char* tagLibExtensions[] = {
        ".ogg", ".oga", ".opus", ".flac", ".m4a", ".mp4", ".m4b", ".aac",
        ".aif", ".aiff", ".ape", ".mpc", ".wv", ".tta", ".dsf", ".dff",
        ".mka", ".webm", ".spx"};
    for (size_t i = 0; iface == NULL && i < _countof(tagLibExtensions); i++)
    {
        if (SalGeneral->StrICmp(ext, tagLibExtensions[i]) == 0)
        {
            extensionSupported = TRUE;
#ifdef new
#undef new
#define RESTORE_MMV_TAGLIB_PARSER_DEBUG_NEW_MACRO
#endif
            iface = new (std::nothrow) CParserTagLib();
#ifdef RESTORE_MMV_TAGLIB_PARSER_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_MMV_TAGLIB_PARSER_DEBUG_NEW_MACRO
#endif
            break;
        }
    }

#ifdef _MOD_SUPPORT_
    if ((SalGeneral->StrICmp(ext, ".it") == 0) || (SalGeneral->StrICmp(ext, ".s3m") == 0) || (SalGeneral->StrICmp(ext, ".stm") == 0) ||
        (SalGeneral->StrICmp(ext, ".xm") == 0) || (SalGeneral->StrICmp(ext, ".mod") == 0) || (SalGeneral->StrICmp(ext, ".mtm") == 0) ||
        (SalGeneral->StrICmp(ext, ".669") == 0))
    {
        extensionSupported = TRUE;
        iface = new CParserMOD();
    }
#endif

    if (iface == NULL)
        return extensionSupported ? preOutOfMemory : preUnknownFile;

    CParserResultEnum result;
    result = iface->OpenFile(fileName);
    if (result != preOK)
        delete iface;

    if (result == preOK)
    {
        *parser = iface;
        return preOK;
    }
    else
        return result;
}
