// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Check the destination bound before reading src in AsciiToUpperCopy. | CodeQL flagged the offset use before the range check. | | Composer |
// | 2026-11-04 | Implement the official language-pack loader. | English ships in host binaries; non-English languages ship in one signed pack. | | Composer |

#include "precomp.h"

#include "langpack.h"

#include <ctype.h>

namespace
{

void AsciiToUpperCopy(const char* src, char* dst, int dstSize)
{
    if (dst == NULL || dstSize <= 0)
        return;
    if (src == NULL)
    {
        dst[0] = 0;
        return;
    }
    int i = 0;
    for (; i < dstSize - 1 && src[i] != 0; i++)
    {
        unsigned char c = (unsigned char)src[i];
        dst[i] = (char)toupper(c);
    }
    dst[i] = 0;
}

BOOL LooksLikePeImage(const void* data, DWORD size)
{
    if (data == NULL || size < sizeof(IMAGE_DOS_HEADER))
        return FALSE;
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)data;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;
    if (dos->e_lfanew <= 0 || (DWORD)dos->e_lfanew + sizeof(DWORD) > size)
        return FALSE;
    const DWORD* sig = (const DWORD*)((const BYTE*)data + dos->e_lfanew);
    return *sig == IMAGE_NT_SIGNATURE;
}

struct CEnumLangPackCtx
{
    const char* ModuleSuffix; // "_SALAMAND" etc. (uppercase, with leading underscore)
    SalLangPackEnumCallback Callback;
    void* UserData;
    int Count;
};

BOOL CALLBACK EnumLangPackResNameProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
    CEnumLangPackCtx* ctx = (CEnumLangPackCtx*)lParam;
    if (IS_INTRESOURCE(lpszName))
        return TRUE;

    const char* name = lpszName;
    const size_t prefixLen = strlen(SAL_LANGPACK_RC_PREFIX);
    if (_strnicmp(name, SAL_LANGPACK_RC_PREFIX, prefixLen) != 0)
        return TRUE;

    const char* afterPrefix = name + prefixLen;
    const char* moduleSuffix = ctx->ModuleSuffix;
    const size_t suffixLen = strlen(moduleSuffix);
    const size_t nameLen = strlen(afterPrefix);
    if (nameLen <= suffixLen)
        return TRUE;
    if (_stricmp(afterPrefix + (nameLen - suffixLen), moduleSuffix) != 0)
        return TRUE;
    if (afterPrefix[nameLen - suffixLen - 1] != '_' && suffixLen > 0)
    {
        // require LANG_MODULE with underscore separator already part of ModuleSuffix
    }

    // afterPrefix is LANG_MODULE; ModuleSuffix is _MODULE
    char languageBase[64];
    size_t langLen = nameLen - suffixLen;
    if (langLen == 0 || langLen >= sizeof(languageBase))
        return TRUE;
    memcpy(languageBase, afterPrefix, langLen);
    languageBase[langLen] = 0;
    // trim trailing underscore if ModuleSuffix includes the separator
    if (languageBase[langLen - 1] == '_')
        languageBase[langLen - 1] = 0;
    if (languageBase[0] == 0)
        return TRUE;

    // normalize to lowercase for stable language ids matching .slg base names
    for (char* p = languageBase; *p; p++)
        *p = (char)tolower((unsigned char)*p);

    ctx->Count++;
    if (ctx->Callback != NULL && !ctx->Callback(languageBase, ctx->UserData))
        return FALSE;
    return TRUE;
}

} // namespace

BOOL SalIsEnglishLanguageName(const char* slgName)
{
    if (slgName == NULL || slgName[0] == 0)
        return FALSE;
    char base[64];
    SalLanguageBaseName(slgName, base, _countof(base));
    return _stricmp(base, "english") == 0;
}

void SalLanguageBaseName(const char* slgName, char* out, int outSize)
{
    if (out == NULL || outSize <= 0)
        return;
    out[0] = 0;
    if (slgName == NULL)
        return;
    lstrcpyn(out, slgName, outSize);
    char* dot = strrchr(out, '.');
    if (dot != NULL && _stricmp(dot, ".slg") == 0)
        *dot = 0;
}

BOOL SalMakeLangPackResourceName(const char* languageBase, const char* moduleKey,
                                 char* out, int outSize)
{
    if (out == NULL || outSize <= 0 || languageBase == NULL || moduleKey == NULL)
        return FALSE;
    char langU[64];
    char modU[64];
    AsciiToUpperCopy(languageBase, langU, _countof(langU));
    AsciiToUpperCopy(moduleKey, modU, _countof(modU));
    if (langU[0] == 0 || modU[0] == 0)
        return FALSE;
    return _snprintf_s(out, outSize, _TRUNCATE, "%s%s_%s", SAL_LANGPACK_RC_PREFIX, langU, modU) > 0;
}

BOOL SalGetLangPackPath(char* path, int pathSize)
{
    if (path == NULL || pathSize <= 0)
        return FALSE;
    if (GetModuleFileName(NULL, path, pathSize) == 0)
        return FALSE;
    char* slash = strrchr(path, '\\');
    if (slash == NULL)
        return FALSE;
    return _snprintf_s(slash + 1, pathSize - (int)(slash + 1 - path), _TRUNCATE,
                       "lang\\%s", SAL_LANGPACK_FILENAME) > 0;
}

BOOL SalLanguageModuleIsHost(HINSTANCE hostModule, HINSTANCE hLanguage)
{
    return hostModule != NULL && hLanguage != NULL && hostModule == hLanguage;
}

HINSTANCE SalLoadLibraryFromMemory(const void* data, DWORD size)
{
    if (!LooksLikePeImage(data, size))
        return NULL;

    char tempDir[MAX_PATH];
    char tempFile[MAX_PATH];
    DWORD dirLen = GetTempPath(MAX_PATH, tempDir);
    if (dirLen == 0 || dirLen >= MAX_PATH)
        return NULL;
    if (GetTempFileName(tempDir, "slg", 0, tempFile) == 0)
        return NULL;

    // Replace the .tmp extension with .slg so LoadLibrary treats it as a library.
    char* ext = strrchr(tempFile, '.');
    if (ext != NULL)
        lstrcpy(ext, ".slg");
    else
        lstrcat(tempFile, ".slg");

    HANDLE hFile = CreateFile(tempFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DeleteFile(tempFile);
        return NULL;
    }

    DWORD written = 0;
    BOOL writeOk = WriteFile(hFile, data, size, &written, NULL) && written == size;
    if (writeOk)
        FlushFileBuffers(hFile);

    HINSTANCE hLib = NULL;
    if (writeOk)
        hLib = LoadLibrary(tempFile);

    // Close the handle so DELETE_ON_CLOSE can remove the file once LoadLibrary's
    // mapping is released (or immediately if LoadLibrary failed).
    CloseHandle(hFile);
    if (hLib == NULL)
        DeleteFile(tempFile);
    return hLib;
}

HINSTANCE SalOpenLangPackDataFile()
{
    char path[MAX_PATH];
    if (!SalGetLangPackPath(path, _countof(path)))
        return NULL;
    return LoadLibraryEx(path, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
}

BOOL SalExtractLangPackBlob(HINSTANCE hPackData, const char* languageBase, const char* moduleKey,
                            void** outData, DWORD* outSize)
{
    if (outData != NULL)
        *outData = NULL;
    if (outSize != NULL)
        *outSize = 0;
    if (hPackData == NULL || languageBase == NULL || moduleKey == NULL || outData == NULL || outSize == NULL)
        return FALSE;

    char resName[128];
    if (!SalMakeLangPackResourceName(languageBase, moduleKey, resName, _countof(resName)))
        return FALSE;

    HRSRC hrsrc = FindResourceA(hPackData, resName, RT_RCDATA);
    if (hrsrc == NULL)
        return FALSE;
    DWORD size = SizeofResource(hPackData, hrsrc);
    if (size == 0)
        return FALSE;
    HGLOBAL hglob = LoadResource(hPackData, hrsrc);
    if (hglob == NULL)
        return FALSE;
    const void* data = LockResource(hglob);
    if (data == NULL)
        return FALSE;

    void* copy = malloc(size);
    if (copy == NULL)
        return FALSE;
    memcpy(copy, data, size);
    *outData = copy;
    *outSize = size;
    return TRUE;
}

int SalEnumLangPackLanguages(const char* moduleKey, SalLangPackEnumCallback callback, void* userData)
{
    if (moduleKey == NULL)
        return 0;
    HINSTANCE hPack = SalOpenLangPackDataFile();
    if (hPack == NULL)
        return 0;

    char modU[64];
    AsciiToUpperCopy(moduleKey, modU, _countof(modU));
    char suffix[80];
    _snprintf_s(suffix, _TRUNCATE, "_%s", modU);

    CEnumLangPackCtx ctx;
    ctx.ModuleSuffix = suffix;
    ctx.Callback = callback;
    ctx.UserData = userData;
    ctx.Count = 0;
    EnumResourceNamesA(hPack, RT_RCDATA, EnumLangPackResNameProc, (LONG_PTR)&ctx);
    FreeLibrary(hPack);
    return ctx.Count;
}

HINSTANCE SalLoadLanguageForModule(HINSTANCE hostModule, const char* moduleKey,
                                   const char* slgName, BOOL* owned)
{
    if (owned != NULL)
        *owned = FALSE;
    if (hostModule == NULL || moduleKey == NULL || slgName == NULL || slgName[0] == 0)
        return NULL;

    // 1) English is compiled into the host binary.
    if (SalIsEnglishLanguageName(slgName))
    {
        // Host resources do not carry SLGIncomplete; treat English as always complete.
        if (owned != NULL)
            *owned = FALSE;
        return hostModule;
    }

    char languageBase[64];
    SalLanguageBaseName(slgName, languageBase, _countof(languageBase));

    // 2) Official language pack.
    HINSTANCE hPack = SalOpenLangPackDataFile();
    if (hPack != NULL)
    {
        void* blob = NULL;
        DWORD blobSize = 0;
        if (SalExtractLangPackBlob(hPack, languageBase, moduleKey, &blob, &blobSize))
        {
            HINSTANCE hLib = SalLoadLibraryFromMemory(blob, blobSize);
            free(blob);
            FreeLibrary(hPack);
            if (hLib != NULL)
            {
                WORD langId = 0;
                if (IsSLGFileValid(hostModule, hLib, langId, NULL))
                {
                    if (owned != NULL)
                        *owned = TRUE;
                    return hLib;
                }
                FreeLibrary(hLib);
            }
            else
            {
                // fall through to .slg
            }
        }
        else
        {
            FreeLibrary(hPack);
        }
    }

    // 3) Legacy / 3rd-party .slg next to the host module.
    char path[MAX_PATH];
    if (GetModuleFileName(hostModule, path, MAX_PATH) == 0)
        return NULL;
    char* slash = strrchr(path, '\\');
    if (slash == NULL)
        return NULL;
    if (_snprintf_s(slash + 1, MAX_PATH - (int)(slash + 1 - path), _TRUNCATE, "lang\\%s", slgName) <= 0)
        return NULL;

    HINSTANCE hSlg = LoadLibrary(path);
    if (hSlg == NULL)
        return NULL;
    WORD langId = 0;
    if (!IsSLGFileValid(hostModule, hSlg, langId, NULL))
    {
        FreeLibrary(hSlg);
        return NULL;
    }
    if (owned != NULL)
        *owned = TRUE;
    return hSlg;
}
