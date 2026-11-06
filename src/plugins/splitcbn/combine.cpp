// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include <tchar.h>
#include "splitcbn.h"
#include "splitcbn.rh"
#include "splitcbn.rh2"
#include "lang\lang.rh"
#include "combine.h"
#include "dialogs.h"

// *****************************************************************************
//
//  Combine Files
//

#define BUFSIZE (512 * 1024)

BOOL CombineFiles(TIndirectArray<char>& files, const char* targetName,
                  BOOL bOnlyCrc, BOOL bTestCrc, UINT32& Crc,
                  BOOL bTime, FILETIME* origTime, HWND parent,
                  CSalamanderForOperationsAbstract* salamander)
{
    CALL_STACK_MESSAGE4("CombineFiles( , %s, %ld, %X, , )", targetName, bTestCrc, Crc);

    if (!bOnlyCrc && !files.Count)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_ZEROFILES), LoadStr(IDS_COMBINE), MSGBOX_ERROR);
        return FALSE;
    }

    int idTitle = bOnlyCrc ? IDS_CRCTITLE : IDS_COMBINE;

    // sum sizes of all partial files (while simultaneously checking their accessibility)
    CQuadWord totalSize = CQuadWord(0, 0);
    SplitCBNPaths::Buffer textBuffer(2 * SplitCBNPaths::Capacity);
    char* text = textBuffer.data();
    CSplitCBNInputFiles inputFiles(files.Count);
    std::vector<SplitCBNPaths::FileIdentity> identities(bOnlyCrc ? 0 : static_cast<size_t>(files.Count));
    int i;
    for (i = 0; i < files.Count; i++)
    {
        SAFE_FILE& file = inputFiles.Get(i);
        if (!SalamanderSafeFile->SafeFileOpen(&file, files[i], GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
                                              0, parent, BUTTONS_RETRYCANCEL, NULL, NULL))
        {
            return FALSE;
        }
        CQuadWord size;
        size.LoDWord = GetFileSize(file.HFile, &size.HiDWord);
        totalSize += size;
        if (!bOnlyCrc && !SplitCBNPaths::GetIdentity(file.HFile, identities[static_cast<size_t>(i)]))
            return Error2(parent, IDS_COMBINE, IDS_OPENERROR);
    }

    // check available free space
    if (!bOnlyCrc)
    {
        const std::string dir = SplitCBNPaths::Parent(targetName);
        if (!TestTargetSpace(parent, dir.c_str(), totalSize, IDS_COMBINE))
            return FALSE;
    }

    // Compare actual volume/file IDs, including hardlinks and reparse aliases.
    // Every source remains open without write/delete sharing until completion.
    if (!bOnlyCrc && !SplitCBNPaths::CanCreateCombineTarget(targetName, identities))
        return Error2(parent, IDS_COMBINE, IDS_WRITEERROR);

    // create the output file
    SAFE_FILE outfile;
    CSplitCBNSafeFileScope outfileScope(outfile);
    if (!bOnlyCrc)
    {
        if (SalamanderSafeFile->SafeFileCreate(targetName, GENERIC_WRITE, FILE_SHARE_READ, FILE_ATTRIBUTE_NORMAL,
                                               FALSE, parent, NULL, NULL, NULL, FALSE, NULL, NULL, 0, NULL, &outfile) == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
    }

    // merge the files
#ifdef new
#define SPLITCBN_RESTORE_NEW
#undef new
#endif
    char* pBuffer = new (std::nothrow) char[BUFSIZE];
#ifdef SPLITCBN_RESTORE_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SPLITCBN_RESTORE_NEW
#endif
    if (pBuffer == NULL)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_OUTOFMEM), LoadStr(idTitle), MSGBOX_ERROR);
        if (!bOnlyCrc)
            SalamanderSafeFile->SafeFileClose(&outfile);
        return FALSE;
    }

    std::unique_ptr<char[]> bufferOwner(pBuffer);

    UINT32 CrcVal = 0;

    // open the progress dialog
    CSplitCBNProgressScope progressScope(salamander);
    salamander->OpenProgressDialog(LoadStr(idTitle), TRUE, parent, FALSE);
    progressScope.Started();
    salamander->ProgressSetTotalSize(CQuadWord(-1, -1), totalSize);
    salamander->ProgressSetSize(CQuadWord(-1, -1), CQuadWord(0, 0), FALSE);
    CQuadWord totalProgress = CQuadWord(0, 0);

    int ret = TRUE;
    int j;
    for (j = 0; j < files.Count; j++)
    {
        sprintf(text, "%s %s...", LoadStr(IDS_PROCESSING), files[j]);
        salamander->ProgressDialogAddText(text, TRUE);

        SAFE_FILE& file = inputFiles.Get(j);

        DWORD numread, numwr;
        CQuadWord currentProgress = CQuadWord(0, 0), size;
        size.LoDWord = GetFileSize(file.HFile, &size.HiDWord);
        salamander->ProgressSetTotalSize(size, CQuadWord(-1, -1));
        salamander->ProgressSetSize(CQuadWord(0, 0), CQuadWord(-1, -1), TRUE);
        do
        {
            if (!SalamanderSafeFile->SafeFileRead(&file, pBuffer, BUFSIZE, &numread, parent, BUTTONS_RETRYCANCEL, NULL, NULL))
            {
                ret = FALSE;
                break;
            }
            if (!bOnlyCrc && numread)
            {
                if (!SalamanderSafeFile->SafeFileWrite(&outfile, pBuffer, numread, &numwr, parent, BUTTONS_RETRYCANCEL, NULL, NULL))
                {
                    ret = FALSE;
                    break;
                }
            }
            CrcVal = SalamanderGeneral->UpdateCrc32(pBuffer, numread, CrcVal);
            currentProgress += CQuadWord(numread, 0);
            if (!salamander->ProgressSetSize(currentProgress, totalProgress + currentProgress, TRUE))
            {
                ret = FALSE;
                break;
            }
        } while (numread == BUFSIZE);

        totalProgress += currentProgress;
        if (ret == FALSE)
            break;
    }

    progressScope.Close();
    if (!bOnlyCrc)
    {
        if (ret)
        {
            if (bTime)
                SetFileTime(outfile.HFile, NULL, NULL, origTime);
            SalamanderSafeFile->SafeFileClose(&outfile);

            const std::string directory = SplitCBNPaths::Parent(targetName);
            SalamanderGeneral->PostChangeOnPathNotification(directory.c_str(), FALSE);
        }
        else
        {
            SalamanderSafeFile->SafeFileClose(&outfile);
            DeleteFileW(SplitCBNPaths::ApiPath(targetName).c_str());
        }
    }

    if (!bOnlyCrc)
    {
        if (ret && bTestCrc && Crc != CrcVal)
        {
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_CRCERROR), LoadStr(idTitle), MSGBOX_ERROR);
            ret = FALSE;
        }
    }
    else
        Crc = CrcVal;

    return ret;
}

static BOOL FindValue(const char*& p)
{
    CALL_STACK_MESSAGE1("FindValue()");
    while (*p && *p != '\r' && *p != '\n' && (*p == ' ' || *p == '\t'))
        p++;
    if (*p != '=' && *p != ':')
        return FALSE;
    p++;
    while (*p && *p != '\r' && *p != '\n' && (*p == ' ' || *p == '\t'))
        p++;
    return *p && *p != '\r' && *p != '\n';
}

static BOOL FindCrc(const char* text, LPCTSTR searchstring, UINT32& crc)
{
    CALL_STACK_MESSAGE2("FindCrc( , %s, )", searchstring);
    const char* p = strstr(text, searchstring);
    if (p != NULL)
    {
        p += strlen(searchstring);
        if (FindValue(p))
            return sscanf(p, "%x", &crc) == 1;
        else
            return FALSE;
    }
    else
        return FALSE;
}

static BOOL FindName(const char* text, const char* text_locase, LPCTSTR searchstring, std::string& name)
{
    CALL_STACK_MESSAGE2("FindName( , %s, )", searchstring);
    const char* p = strstr(text_locase, searchstring);
    if (p != NULL)
    {
        p += strlen(searchstring);
        if (FindValue(p))
        {
            p += text - text_locase;
            if (*p == '\"')
                p++;
            const char* begin = p;
            while (*p && *p != '\r' && *p != '\n' && *p != '\"')
                ++p;
            name.assign(begin, p);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

static BOOL FindTime(const char* text, LPCTSTR searchstring, FILETIME* ft)
{
    CALL_STACK_MESSAGE2("FindTime( , %s, )", searchstring);
    const char* p = strstr(text, searchstring);
    if (p != NULL)
    {
        p += strlen(searchstring);
        if (FindValue(p))
        {
            SYSTEMTIME st;
            st.wMilliseconds = 0;
            if (sscanf(p, "%hu-%hu-%hu %hu:%hu:%hu", &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond) != 6)
                return FALSE;
            return SystemTimeToFileTime(&st, ft);
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

static void AnalyzeFile(const char* fileName, std::string& origName, UINT32& origCrc, FILETIME* origTime,
                        BOOL& bNameAcquired, BOOL& bCrcAcquired, BOOL& bTimeAcquired)
{
    CALL_STACK_MESSAGE2("AnalyzeFile(%s, , , , )", fileName);
    // load the file into a buffer
    HANDLE hFile;
    if ((hFile = CreateFileW(SplitCBNPaths::ApiPath(fileName).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
        return;
    DWORD size = GetFileSize(hFile, NULL), numread;
    // j.r. 21.1.2003: when splitting to 999 parts I received a batch of 30KB
    //  if (size > 10000) { CloseHandle(hFile); return; } // skip such large files
    if (size > 200000)
    {
        CloseHandle(hFile);
        return;
    } // skip such large files
#ifdef new
#define SPLITCBN_RESTORE_NEW
#undef new
#endif
    char* text = new (std::nothrow) char[size + 1];
    char* text_locase = new (std::nothrow) char[size + 1];
#ifdef SPLITCBN_RESTORE_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SPLITCBN_RESTORE_NEW
#endif
    if (text == NULL || text_locase == NULL)
    {
        delete[] text;
        delete[] text_locase;
        CloseHandle(hFile);
        return;
    }
    BOOL ok = ReadFile(hFile, text, size, &numread, NULL);
    CloseHandle(hFile);
    if (!ok || size != numread)
    {
        delete[] text;
        delete[] text_locase;
        return;
    }
    text[size] = 0;

    strcpy(text_locase, text);
    for (char* ch = text_locase; *ch != 0; ++ch)
        if (*ch >= 'A' && *ch <= 'Z')
            *ch += 'a' - 'A';
    // try to find "crc32" or "crc"
    if (!bCrcAcquired)
    {
        bCrcAcquired = FindCrc(text_locase, "crc32", origCrc);
        if (!bCrcAcquired)
            bCrcAcquired = FindCrc(text_locase, "crc", origCrc);
    }
    // "filename" or "name"
    const BOOL needName = !bNameAcquired;
    if (!bNameAcquired)
    {
        bNameAcquired = FindName(text, text_locase, "filename", origName);
        if (!bNameAcquired)
            bNameAcquired = FindName(text, text_locase, "name", origName);
    }
    if (needName && bNameAcquired && strstr(text_locase, "rem encoding=utf8-cmd") != NULL)
        origName = SplitCBNPaths::DecodeBatchMetadata(origName);
    // "time"
    if (!bTimeAcquired)
    {
        bTimeAcquired = FindTime(text_locase, "time", origTime);
    }

    delete[] text;
    delete[] text_locase;
}

static BOOL CombineCommandImpl(HWND parent, CSalamanderForOperationsAbstract* salamander)
{
    CSalamanderDiskSelection selection;
    if (!selection.Capture(SalamanderGeneral, PANEL_SOURCE))
        return Error(IDS_COMBINE, IDS_OPENERROR);
    bool selected = false;
    for (int i = 0; i < selection.GetCount(); ++i)
        selected = selected || selection.GetItem(i)->Selected != FALSE;
    CSalamanderDiskSelection siblings;
    if (!selected && !siblings.Capture(SalamanderGeneral, PANEL_SOURCE, SALDISKSELECTION_ALL_ITEMS))
        return Error(IDS_COMBINE, IDS_OPENERROR);
    SplitCBNPaths::CombinePlan plan;
    if (!SplitCBNPaths::BuildCombinePlan(selection, selected ? NULL : &siblings, plan))
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_ZEROFILES), LoadStr(IDS_COMBINE), MSGBOX_ERROR);
        return FALSE;
    }
    TIndirectArray<char> files(100, 100, dtDelete);
    for (size_t i = 0; i < plan.Files.size(); ++i)
    {
        char* name = _strdup(plan.Files[i].c_str());
        if (name == NULL)
            throw std::bad_alloc();
        files.Add(name);
        if (!files.IsGood())
        {
            free(name);
            throw std::bad_alloc();
        }
    }

    BOOL bName = FALSE, bCrc = FALSE, bTime = FALSE;
    UINT32 origCrc = 0;
    FILETIME origTime = {};
    std::string originalName;
    if (plan.TestCompanion)
    {
        AnalyzeFile((plan.CompanionPrefix + "bat").c_str(), originalName, origCrc, &origTime, bName, bCrc, bTime);
        if (!bName || !bCrc)
            AnalyzeFile((plan.CompanionPrefix + "crc").c_str(), originalName, origCrc, &origTime, bName, bCrc, bTime);
    }
    std::string target;
    if (bName)
        target = SplitCBNPaths::Join(plan.SourceDirectory, SplitCBNPaths::Utf8(SplitCBNPaths::Wide(originalName.c_str()).c_str()));
    else
    {
        target = plan.CompanionPrefix.substr(0, plan.CompanionPrefix.size() - 1);
        if (SplitCBNPaths::Name(target).find('.') == std::string::npos)
            target += ".EXT";
    }
    const std::string targetDirectory = GetTargetDir(plan.SourceDirectory.c_str(), NULL, FALSE);
    if (configCombineToOther)
        target = SplitCBNPaths::Join(targetDirectory, SplitCBNPaths::Name(target));
    if (!CombineDialog(files, target, bCrc, origCrc, parent, salamander))
        return FALSE;
    if (!MakePathAbsolute(target, FALSE, targetDirectory.c_str(), !configCombineToOther, IDS_COMBINE))
        return FALSE;
    return CombineFiles(files, target.c_str(), FALSE, bCrc, origCrc, bTime, &origTime, parent, salamander);
}

BOOL CombineCommand(DWORD eventMask, HWND parent, CSalamanderForOperationsAbstract* salamander)
{
    CALL_STACK_MESSAGE2("CombineCommand(%X, , )", eventMask);
    try
    {
        return CombineCommandImpl(parent, salamander);
    }
    catch (const std::bad_alloc&)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_OUTOFMEM), LoadStr(IDS_COMBINE), MSGBOX_ERROR);
        return FALSE;
    }
}
