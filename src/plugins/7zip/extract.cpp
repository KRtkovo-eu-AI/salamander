// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "7zip.h"
#include "extract.h"
#include "7zip.rh"
#include "7zip.rh2"
#include "lang\lang.rh"
#include "math.h"
#include "dialogs.h"
#include "7zthreads.h"
#include "7zclient.h"

#include "Common/StringConvert.h"
#include "Windows/WinDefs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "7zip/IPassword.h"

using namespace NWindows;
using namespace NFile;

// Adapted from the qualified ZIP output guard; no I/O for rejected paths.
static DWORD SevenZipExtractTargetError(const char* targetDir, const char* entry)
{
    if (targetDir == NULL || entry == NULL) return ERROR_INVALID_NAME;
    const size_t targetLength = strlen(targetDir), entryLength = strlen(entry);
    if (targetLength >= SAL_MAX_PATH || entryLength >= SAL_MAX_PATH)
        return ERROR_FILENAME_EXCED_RANGE;
    try
    {
        const std::string relative(entry);
        if (relative.size() >= SAL_MAX_PATH - targetLength - 1)
            return ERROR_FILENAME_EXCED_RANGE; // reserve separator and NUL, for both legacy callers
        if (relative.empty()) return ERROR_INVALID_NAME;
        size_t start = 0;
        while (start < relative.size())
        {
            const size_t slash = relative.find('\\', start);
            const std::string component = relative.substr(start, slash == std::string::npos ? slash : slash - start);
            if (component.empty() || component == "." || component == ".." ||
                component.back() == '.' || component.back() == ' ' ||
                component.find_first_of("<>:\"/|?*") != std::string::npos)
                return ERROR_INVALID_NAME;
            for (const unsigned char character : component)
                if (character < 32) return ERROR_INVALID_NAME;
            std::wstring device = std::wstring(GetSalamanderUnicodeString(component.c_str()).Ptr());
            device.resize(device.find('.') == std::wstring::npos ? device.size() : device.find('.'));
            while (!device.empty() && device.back() == L' ') device.pop_back();
            if (_wcsicmp(device.c_str(), L"CON") == 0 || _wcsicmp(device.c_str(), L"PRN") == 0 ||
                _wcsicmp(device.c_str(), L"AUX") == 0 || _wcsicmp(device.c_str(), L"NUL") == 0 ||
                _wcsicmp(device.c_str(), L"CONIN$") == 0 || _wcsicmp(device.c_str(), L"CONOUT$") == 0 ||
                device.size() == 4 && (_wcsnicmp(device.c_str(), L"COM", 3) == 0 || _wcsnicmp(device.c_str(), L"LPT", 3) == 0) &&
                (device[3] >= L'1' && device[3] <= L'9' || device[3] == L'\u00b9' || device[3] == L'\u00b2' || device[3] == L'\u00b3'))
                return ERROR_INVALID_NAME;
            if (slash == std::string::npos) break;
            start = slash + 1;
            if (start == relative.size()) return ERROR_INVALID_NAME;
        }
        const std::string destination = std::string(targetDir) + "\\" + relative;
        const std::wstring wide = std::wstring(GetSalamanderUnicodeString(destination.c_str()).Ptr());
        if (wide.empty() || !(wide.size() > 2 && wide[1] == L':' && wide[2] == L'\\' ||
                              wide.compare(0, 2, L"\\\\") == 0)) return ERROR_INVALID_NAME;
        return wide.size() >= 32767 ? ERROR_FILENAME_EXCED_RANGE : ERROR_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

CExtractCallbackImp::CExtractCallbackImp(HWND _hProgWnd, UString& password,
                                         ItemsToExtractMap& itemsToExtract) : Password(password), ItemsToExtract(itemsToExtract)
{
    //  Silent = false; // preparation for skip and skip all

    InitializeCriticalSection(&CSExtract);

    OutFileStreamSpec = NULL;
    TargetDir = NULL;
    WholeFile = TRUE;
    OutputCreated = FALSE;
    hProgWnd = _hProgWnd;
    NumErrors = 0;
    ExpectedItems = CompletedItems = 0;
    PendingItem = false;
    ExtractionIncomplete = true;
}

CExtractCallbackImp::~CExtractCallbackImp()
{
    DeleteCriticalSection(&CSExtract);
}

// silent - if TRUE and a DataError occurs during extraction (usually an incorrect password), the file is deleted
// automatically and the user is not asked anything (used when extracting a single file for preview - F3)
BOOL CExtractCallbackImp::Init(IInArchive* archive, const char* outDir,
                               const FILETIME& utcLastWriteTimeDefault, DWORD attributesDefault, BOOL silentDelete /* = FALSE*/)
{
    NumErrors = 0;
    ExpectedItems = ItemsToExtract.size();
    CompletedItems = 0;
    PendingItem = false;
    ExtractionIncomplete = false;

    UTCLastWriteTimeDefault = utcLastWriteTimeDefault;
    AttributesDefault = attributesDefault;
    ArchiveHandler = archive;
    TargetDir = outDir;

    //  ItemsToExtract = itemsToExtract;

    PasswordIsDefined = !Password.IsEmpty();

    TargetFileName[0] = '\0';
    WholeFile = TRUE;
    OutputCreated = FALSE;

    // initialize variables for interacting with the user
    SilentDelete = silentDelete;
    if (silentDelete)
    {
        DataErrorMode = Delete;
        DataErrorSilent = TRUE;
        DataErrorDeleteSilent = TRUE;
    }
    else
    {
        DataErrorSilent = FALSE;
        DataErrorMode = Ask;
        DataErrorDeleteSilent = FALSE;
    }

    //
    //  OverwriteMode = Ask;
    //  OverwriteSilent = FALSE;
    OverwriteSilent = 0;
    OverwriteSkip = FALSE;
    OverwriteCancel = FALSE;

    return TRUE;
}

BOOL CExtractCallbackImp::InitTest()
{
    NumErrors = 0;
    TargetFileName[0] = '\0';
    WholeFile = TRUE;
    OutputCreated = FALSE;

    OverwriteCancel = FALSE;
    PasswordIsDefined = !Password.IsEmpty();

    return TRUE;
}

void CExtractCallbackImp::Cleanup()
{
    if (!WholeFile && OutputCreated && !ProcessedFileInfo.IsDirectory)
    {
        //    TRACE_I("Cleanup File: " << GetAnsiString(ProcessedFileInfo.FileName));

        OutFileStream.Release();

        BOOL silent = FALSE;
        SafeDeleteFileW(ProcessedFileInfo.FileName.Ptr(), TargetFileName, silent);
        OutputCreated = FALSE;
    }
}

STDMETHODIMP CExtractCallbackImp::SetTotal(UINT64 size)
{
    //  TRACE_I("CExtractCallbackImp::SetTotal: size=" << (DWORD)size);

    Total.Value = size;
    SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_SETTOTAL, (LPARAM)&Total);

    return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UINT64* completeValue)
{
    //  TRACE_I("CExtractCallbackImp::SetCompleted: completeValue=" << (DWORD)(*completeValue));

    if (completeValue != NULL)
    {
        Completed.Value = *completeValue;

        return (HRESULT)SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_PROGRESS, (LPARAM)&Completed);
    }

    return S_OK;
}

STDMETHODIMP CExtractCallbackImp::GetStream(UINT32 index, ISequentialOutStream** outStream, INT32 askExtractMode)
{
    HRESULT ret = S_OK;

    // we already pressed cancel and yet the callback was invoked again (caused by the 7za.dll implementation)
    if (OverwriteCancel)
        return E_ABORT;

    EnterCriticalSection(&CSExtract);

    try
    {
        /*    char u[1024];
    sprintf(u, "CExtractCallbackImp::GetStream: index: %d, askExtractMode: %d", index, askExtractMode);
    TRACE_I(u);
*/
        //    TRACE_I("CExtractCallbackImp::GetStream: index=" << index << ", askExtractMode=" << askExtractMode);

        *outStream = NULL;
        OutFileStream.Release();
        OutFileStreamSpec = NULL;
        OutputCreated = FALSE;
        if (PendingItem) ExtractionIncomplete = true; // a requested result never arrived
        PendingItem = false;
        ExtractMode = false;
        OverwriteSkip = FALSE;

        // if we are extracting a file
        if (askExtractMode == NArchive::NExtract::NAskMode::kExtract)
        {

            const auto requested = ItemsToExtract.find(index);
            const CArchiveItemInfo* aii = requested == ItemsToExtract.end() ? NULL : requested->second;
            if (!aii)
            {
                // Do not insert an unrequested/duplicate index into the pending map.
                ExtractionIncomplete = true;
                // Already extracted??? Not to be extracted?
                throw S_OK;
            }
            PendingItem = true;
            ItemsToExtract.erase(requested);
            const CFileData* fd = aii->FileData;
            // fd is certainly not NULL

            // because we handed over a list of CArchiveItem with all information we need, we can extract directly.
            // however, we need reverse mapping (a hash function) that tells us the index of the item to use
            // because this function receives the index in the archive
            //
            // we could save memory and retrieve the properties we need via ArchiveHandler->GetProperty,
            // but the problem is with names: GetProperty returns path+filename relative to the archive root and if we extract
            // from a different root, we would have to strip the path

            ProcessedFileInfo.Attributes = fd->Attr;
            ProcessedFileInfo.AttributesAreDefined = true;

            ProcessedFileInfo.IsDirectory = aii->IsDir;
            ProcessedFileInfo.LastWrite = fd->LastWrite;
            ProcessedFileInfo.Size = fd->Size.Value;
            ProcessedFileInfo.Name = aii->NameInArchive;

            // TODO: check for free space

            const DWORD targetError = SevenZipExtractTargetError(TargetDir, aii->NameInArchive);
            if (targetError != ERROR_SUCCESS)
            {
                NumErrors++;
                SysError(IDS_ERROR, targetError);
                throw S_OK;
            }
            _tcscpy(TargetFileName, TargetDir); // guard above validates capacity before the first copy
            if (SalamanderGeneral->SalPathAppend(TargetFileName, aii->NameInArchive, SAL_MAX_PATH))
            {
                ProcessedFileInfo.FileName = GetSalamanderUnicodeString(TargetFileName);

                // Show file name in the progress dialog
                SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_ADDTEXT, (LPARAM)GetName());

                if (ProcessedFileInfo.IsDirectory)
                {
                    // create the directory if it does not already exist
                    if (!SalamanderGeneral->CheckAndCreateDirectory(TargetFileName))
                    {
                        NumErrors++;
                        throw E_ABORT;
                    }
                    OutputCreated = TRUE;
                    throw S_OK;
                }
                else
                {
                    // WARNING! This performs the overwrite test
                    CCreateFileParams cfp;
                    char fileInfo[100];

                    FILETIME ftLW(GetLastWrite());
                    GetInfo(fileInfo, &ftLW, GetSize());
                    cfp.FileInfo = fileInfo;
                    cfp.FileName = GetFileName();
                    cfp.Name = GetName();
                    cfp.pSilent = GetOverwriteSilent();
                    cfp.pSkip = GetOverwriteSkip();

                    if (SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_CREATEFILE, (LPARAM)&cfp))
                    {
                        if (!*GetOverwriteSkip())
                            SetOverwriteCancel();
                    }

                    *outStream = NULL;
                    if (OverwriteSkip)
                    {
                        ExtractionIncomplete = true;
                        throw S_OK;
                    }
                    // the overall operation cannot continue (cancel)
                    if (OverwriteCancel)
                        throw E_ABORT;

                    OutputCreated = TRUE; // SafeFileCreate succeeded and neither Skip nor Cancel was chosen
                    WholeFile = FALSE;

                    // open the stream for extraction
                    // at this point there is an empty file with FILE_ATTRIBUTES_NORMAL
                    // it is the result of the overwrite test - see above

                    // if the path we are extracting to does not exist -> create it
                    std::string parentPath(TargetFileName);
                    const size_t lastComp = parentPath.find_last_of('\\');
                    if (lastComp != std::string::npos)
                    {
                        parentPath.resize(lastComp);
                        if (!SalamanderGeneral->CheckAndCreateDirectory(parentPath.c_str()))
                        {
                            NumErrors++;
                            throw E_ABORT;
                        }
                    } // keep the full byte path intact for host UI and cleanup display

                    OutFileStreamSpec = new CRetryableOutFileStream(hProgWnd);
                    CMyComPtr<ISequentialOutStream> outStreamLoc(OutFileStreamSpec);
                    if (!OutFileStreamSpec->Open(us2fs(GetSalamanderLongPath(ProcessedFileInfo.FileName)), OPEN_ALWAYS))
                    {
                        SysError(IDS_ERROR, ::GetLastError());
                        NumErrors++;
                        throw S_OK;
                    }
                    OutFileStream = outStreamLoc;
                    *outStream = outStreamLoc.Detach();
                }
            }
            else
            {
                *outStream = NULL;

                NumErrors++;
                char errText[1000];
                _snprintf_s(errText, _TRUNCATE, LoadStr(IDS_NAMEISTOOLONG), (const char*)(aii->NameInArchive), TargetFileName);
                SalamanderGeneral->ShowMessageBox(errText, LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
            }
        }
        else
        {
            *outStream = NULL;
        }
    }
    catch (HRESULT e)
    {
        ret = e;
    }
    catch (const std::bad_alloc&)
    {
        NumErrors++;
        ret = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&CSExtract);

    return ret;
}

STDMETHODIMP CExtractCallbackImp::PrepareOperation(INT32 askExtractMode)
{
    /*  char u[1024];
  sprintf(u, "CExtractCallbackImp::PrepareOperation: askExtractMode: %d", askExtractMode);
  TRACE_I(u);
*/
    WholeFile = FALSE;
    ExtractMode = false;
    switch (askExtractMode)
    {
    case NArchive::NExtract::NAskMode::kExtract:
        ExtractMode = true;
        //      TRACE_I("CExtractCallbackImp::PrepareOperation: Extract!");
        break;

    case NArchive::NExtract::NAskMode::kTest:
        //      TRACE_I("CExtractCallbackImp::PrepareOperation: Test!");
        break;

    case NArchive::NExtract::NAskMode::kSkip:
        //      TRACE_I("CExtractCallbackImp::PrepareOperation: Skip!");
        break;
    }

    return S_OK;
}

//
// Let the user choose whether to keep or delete the damaged file
//
// TRUE if we continue
// FALSE if canceled
BOOL CExtractCallbackImp::OnDataError()
{
    if (!OutputCreated || ProcessedFileInfo.IsDirectory) return TRUE;
    EOperationMode mode = DataErrorMode;

    if (DataErrorMode == Ask)
    {
        TCHAR btnBuffer[1024];
        /* used by the export_mnu.py script that generates salmenu.mnu for Translator
           we let the message box buttons resolve hotkey collisions by simulating a menu
MENU_TEMPLATE_ITEM MsgBoxButtons[] =
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_BTN_DELETE
  {MNTT_IT, IDS_BTN_KEEP
  {MNTT_PE, 0
};
*/
        _stprintf(btnBuffer, _T("%d\t%s\t%d\t%s"),
                  DIALOG_YES, LoadStr(IDS_BTN_DELETE),
                  //      DIALOG_ALL, LoadStr(IDS_BTN_DELETE_ALL),
                  DIALOG_NO, LoadStr(IDS_BTN_KEEP)
                  //      DIALOG_SKIPALL, LoadStr(IDS_BTN_KEEP_ALL)
                  //  DIALOG_CANCEL, LoadStr(IDS_BTN_)
        );
        //buffer: "1\t&Start\t2\tE&xit"

        TCHAR msg[1024];
        _stprintf(msg, LoadStr(PasswordIsDefined ? IDS_ERROR_PROCESSING_FILE_PWD : IDS_ERROR_PROCESSING_FILE),
                  (LPCTSTR)GetAnsiString(ProcessedFileInfo.Name));

        MSGBOXEX_PARAMS mbep;
        ZeroMemory(&mbep, sizeof(mbep));
        mbep.HParent = hProgWnd;
        mbep.Caption = LoadStr(IDS_PLUGINNAME);
        mbep.Text = msg;
        mbep.Flags = MSGBOXEX_ICONEXCLAMATION | MSGBOXEX_YESNOCANCEL;
        mbep.AliasBtnNames = btnBuffer;

        int mbRet = (int)SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_SHOWMBOXEX, (LPARAM)&mbep);

        switch (mbRet)
        {
        case DIALOG_YES:
            mode = Delete;
            break;
        case DIALOG_NO:
            mode = Keep;
            break;
            /*
      case DIALOG_YES: mode = Delete; break;
      case DIALOG_ALL: mode = Delete; DataErrorSilent = TRUE; break;
      case DIALOG_SKIP: mode = Keep; break;
      case DIALOG_SKIPALL: mode = Keep; DataErrorSilent = TRUE; break;
*/
        case DIALOG_CANCEL:
            mode = Cancel;
            break;
        }
    }

    // release the OutStream so it can be deleted if needed
    if (OutFileStream != NULL)
        OutFileStreamSpec->SetMTime(&ProcessedFileInfo.LastWrite);
    OutFileStream.Release();

    switch (mode)
    {
    case Cancel:
        SafeDeleteFileW(ProcessedFileInfo.FileName.Ptr(), TargetFileName, DataErrorDeleteSilent);
        OutputCreated = FALSE;
        return FALSE;

    case Delete:
        // if cancel, bail out
        if (!SafeDeleteFileW(ProcessedFileInfo.FileName.Ptr(), TargetFileName, DataErrorDeleteSilent))
            return FALSE;
        break;

    case Keep:
        // keep the file, so set its attributes
        if (ExtractMode && OutputCreated && ProcessedFileInfo.AttributesAreDefined)
            SetFileAttributesW(GetSalamanderLongPath(ProcessedFileInfo.FileName).Ptr(), ProcessedFileInfo.Attributes);
        break;
    }

    OutputCreated = FALSE;

    // we will not ask again, so set the mode to the chosen operation
    if (DataErrorSilent)
        DataErrorMode = mode;

    return TRUE;
}

LRESULT CExtractCallbackImp::Error(int resID, ...)
{
    MSGBOXEX_PARAMS mbep;
    char msg[1024];
    va_list arglist;

    va_start(arglist, resID);
    vsprintf(msg, LoadStr(resID), arglist);
    va_end(arglist);

    ZeroMemory(&mbep, sizeof(mbep));
    mbep.HParent = hProgWnd;
    mbep.Caption = LoadStr(IDS_PLUGINNAME);
    mbep.Text = msg;
    mbep.Flags = MSGBOXEX_ICONHAND | MSGBOXEX_OK;

    return SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_SHOWMBOXEX, (LPARAM)&mbep);
}

STDMETHODIMP CExtractCallbackImp::SetOperationResult(INT32 resultEOperationResult)
{
    //  TRACE_I("CExtractCallbackImp::SetOperationResult: result = " << resultEOperationResult);

    WholeFile = TRUE;

    if (PendingItem)
    {
        if (resultEOperationResult == NArchive::NExtract::NOperationResult::kOK &&
            ExtractMode && OutputCreated)
            ++CompletedItems;
        else
            ExtractionIncomplete = true;
        PendingItem = false;
    }

    switch (resultEOperationResult)
    {
    case NArchive::NExtract::NOperationResult::kOK:
        break;

    default:
        NumErrors++;

        switch (resultEOperationResult)
        {
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
            Error(IDS_UNSUPPORTED_METHOD);
            break;

        case NArchive::NExtract::NOperationResult::kCRCError:
            Error(IDS_CRC_FAILED);
            break;

        case NArchive::NExtract::NOperationResult::kDataError:
            //          // use this code if a patch for the keep/delete functionality cannot be applied
            //          if (ExtractMode)
            //            Error(IDS_DATA_ERROR, GetAnsiString(ProcessedFileInfo.FileName));

            // this code works only with the keep/delete functionality patch
            if (ExtractMode)
            {
                // give the user the option to keep/delete the damaged file
                if (!OnDataError())
                    return E_ABORT;
                // an error occurred during extraction; delete the file (handled by the function above) and finish
                if (SilentDelete)
                {
                    Error(PasswordIsDefined ? IDS_DATA_ERROR_PWD : IDS_DATA_ERROR, (LPCTSTR)ProcessedFileInfo.Name);
                    if (PasswordIsDefined)
                        Password = L""; // Allow entering another password
                    return E_ABORT;
                }
                return S_OK;
            }

            break;

        default:
            Error(IDS_UNKNOWN_ERROR);
            break;
        }

        break;
    }

    if (OutFileStream != NULL)
        OutFileStreamSpec->SetMTime(&ProcessedFileInfo.LastWrite);
    OutFileStream.Release();

    if (ExtractMode && OutputCreated && ProcessedFileInfo.AttributesAreDefined)
        SetFileAttributesW(GetSalamanderLongPath(ProcessedFileInfo.FileName).Ptr(), ProcessedFileInfo.Attributes);

    if (TargetDir && !ItemsToExtract.size())
    {
        // NULL TargetDir means testing the archive
        // Doing partial extract & everything has been extracted. Looks like a solid archive
        return E_STOPEXTRACTION;
    }

    return S_OK;
}

STDMETHODIMP CExtractCallbackImp::CryptoGetTextPassword(BSTR* password)
{
    if (!PasswordIsDefined /*&& !Silent*/) // Silent is for skip, which is not implemented in 7za.dll
    {
        char pwd[PASSWORD_LEN];

        pwd[0] = 0;
        switch (SendMessage(hProgWnd, WM_7ZIP, WM_7ZIP_PASSWORD, (LPARAM)pwd))
        {
        case IDOK:
            PasswordIsDefined = true;
            Password = GetUnicodeString(pwd);
            break;

        case IDCANCEL:
            return E_ABORT;
            /*
      // prepared for skip and skip all (hopefully it will be completed in the next version)
      case DIALOG_SKIP:
        PasswordIsDefined = false;
        Silent = false;
        return S_OK;

      case DIALOG_SKIPALL:
        PasswordIsDefined = false;
        Silent = true;
        return S_FALSE;
*/
        }
    }
    StringToBstr(Password, password);

    return S_OK;
}
