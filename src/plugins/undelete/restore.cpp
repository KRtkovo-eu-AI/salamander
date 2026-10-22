// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "undelete.rh"
#include "undelete.rh2"
#include "lang\lang.rh"

#include "miscstr.h"
#include "os.h"

#include "miscstr.h"
#include "volume.h"
#include "snapshot.h"
#include "dialogs.h"
#include "undelete.h"
#include "restore.h"

#define COPY_BUFFER_SIZE (256 * 1024)

// ****************************************************************************
//
// RestoreEncryptedFiles
//

char* GetSCFData(FILETIME* lastWrite, CQuadWord& size);

static CRestoreProgressDlg* Progress;
static HWND hProgressWnd;
static QWORD FileProgress, FileTotal;
static QWORD TotalProgress, GrandTotal;
static DWORD SrcSilentMask, DstSilentMask, DirSilentMask;

static void UpdateRestoreProgress()
{
    CALL_STACK_MESSAGE1("UpdateRestoreProgress()");
    if (FileTotal)
        Progress->SetFileProgress((DWORD)(FileProgress * 1000 / FileTotal));
    if (GrandTotal)
        Progress->SetTotalProgress((DWORD)(TotalProgress * 1000 / GrandTotal));
}

struct IMPORT_CONTEXT
{
    SAFE_FILE* file;
    HWND parent;
};

static DWORD WINAPI RestoreCallback(PBYTE data, PVOID _ctx, PULONG plen)
{
    CALL_STACK_MESSAGE1("RestoreCallback()");
    IMPORT_CONTEXT* ctx = (IMPORT_CONTEXT*)_ctx;
    DWORD numread;
    if (!SalamanderSafeFile->SafeFileRead(ctx->file, data, *plen, &numread, ctx->parent, BUTTONS_RETRYCANCEL, NULL, NULL) ||
        Progress->GetWantCancel())
    {
        return ERROR_CANCELLED;
    }
    FileProgress += numread;
    TotalProgress += numread;
    UpdateRestoreProgress();
    *plen = numread;
    return ERROR_SUCCESS;
}

// SafeFile owns retry/overwrite/skip policy. Keep each successfully opened
// handle scoped so cancellation and allocation failure cannot leak it.
struct CRestoreSafeFile
{
    SAFE_FILE File;
    CRestoreSafeFile() { memset(&File, 0, sizeof(File)); File.HFile = INVALID_HANDLE_VALUE; }
    void Close() { if (File.HFile != INVALID_HANDLE_VALUE) { SalamanderSafeFile->SafeFileClose(&File); File.HFile = INVALID_HANDLE_VALUE; } }
    ~CRestoreSafeFile() { Close(); }
};
struct CRestoreFind
{
    HANDLE Handle;
    explicit CRestoreFind(HANDLE handle) : Handle(handle) {}
    ~CRestoreFind() { if (Handle != INVALID_HANDLE_VALUE) HANDLES(FindClose(Handle)); }
};
struct CRestoreRawContext
{
    PVOID Context;
    CRestoreRawContext() : Context(NULL) {}
    ~CRestoreRawContext() { if (Context != NULL) CloseEncryptedFileRaw(Context); }
};

static BOOL RestoreFile(const std::wstring& source, const std::wstring& destination)
{
    const std::wstring sourceIO = UndeletePaths::IoPath(source);
    const std::string sourceUtf8 = UndeletePaths::Utf8(source.c_str());
    CRestoreSafeFile srcfile;
    DWORD button = DIALOG_CANCEL;
    if (!SalamanderSafeFile->SafeFileOpen(&srcfile.File, sourceUtf8.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
                                        FILE_FLAG_SEQUENTIAL_SCAN, hProgressWnd, BUTTONS_RETRYSKIPCANCEL, &button, &SrcSilentMask))
        return button != DIALOG_CANCEL;

    BY_HANDLE_FILE_INFORMATION info = {};
    if (!GetFileInformationByHandle(srcfile.File.HFile, &info))
        return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED);
    CQuadWord size(info.nFileSizeLow, info.nFileSizeHigh);
    FILETIME times[3] = {info.ftCreationTime, info.ftLastAccessTime, info.ftLastWriteTime};
    DWORD sig[3] = {}, numread = 0;
    BOOL real = source.size() >= 4 && _wcsicmp(source.c_str() + source.size() - 4, L".bak") == 0;
    if (real)
        real = ReadFile(srcfile.File.HFile, sig, sizeof(sig), &numread, NULL) && numread == sizeof(sig) &&
               sig[1] == 0x004f0052 && sig[2] == 0x00530042;
    LARGE_INTEGER zero = {};
    if (!SetFilePointerEx(srcfile.File.HFile, zero, NULL, FILE_BEGIN))
        return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED);

    std::wstring target = destination;
    if (real) target.resize(target.size() - 4);
    const std::wstring targetIO = UndeletePaths::IoPath(target);
    const std::string targetUtf8 = UndeletePaths::Utf8(target.c_str());
    if (sourceIO.empty() || targetIO.empty())
        return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED);
    if (CompareStringOrdinal(sourceIO.c_str(), -1, targetIO.c_str(), -1, TRUE) == CSTR_EQUAL)
    { SetLastError(ERROR_SHARING_VIOLATION); return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED); }
    // Allocate before creating/truncating any destination file.
    std::vector<BYTE> buffer(real ? 0 : COPY_BUFFER_SIZE);
    FileProgress = 1;
    FileTotal = size.Value + 1;
    TotalProgress++;
    UpdateRestoreProgress();
    Progress->SetSourceFileName(sourceUtf8.c_str());
    Progress->SetDestFileName(targetUtf8.c_str());

    CRestoreSafeFile dstfile;
    BOOL skipped = FALSE;
    HANDLE hdst = SalamanderSafeFile->SafeFileCreate(targetUtf8.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, FALSE, hProgressWnd, sourceUtf8.c_str(),
                                                    GetSCFData(times, size), &DstSilentMask, TRUE, &skipped, NULL, 0, NULL, &dstfile.File);
    if (skipped)
    {
        FileProgress = 0;
        TotalProgress += size.Value;
        UpdateRestoreProgress();
        return TRUE;
    }
    if (hdst == INVALID_HANDLE_VALUE) return FALSE;
    BOOL ret = TRUE;
    if (real)
    {
        dstfile.Close(); // EFS owns its output handle, after SafeFile's overwrite decision.
        IMPORT_CONTEXT ctx = {&srcfile.File, hProgressWnd};
        CRestoreRawContext raw;
        DWORD result = OpenEncryptedFileRawW(targetIO.c_str(), CREATE_FOR_IMPORT, &raw.Context);
        if (result == ERROR_SUCCESS)
            result = WriteEncryptedFileRaw(RestoreCallback, &ctx, raw.Context);
        if (result != ERROR_SUCCESS)
        {
            if (result != ERROR_CANCELLED)
            { SetLastError(result); String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED); }
            ret = FALSE;
        }
    }
    else
    {
        do
        {
            DWORD numwritten = 0;
            if (!SalamanderSafeFile->SafeFileRead(&srcfile.File, buffer.data(), COPY_BUFFER_SIZE, &numread, hProgressWnd, BUTTONS_RETRYCANCEL, NULL, NULL) ||
                !SalamanderSafeFile->SafeFileWrite(&dstfile.File, buffer.data(), numread, &numwritten, hProgressWnd, BUTTONS_RETRYCANCEL, NULL, NULL) ||
                numwritten != numread || Progress->GetWantCancel())
            { ret = FALSE; break; }
            FileProgress += numread;
            TotalProgress += numread;
            UpdateRestoreProgress();
        } while (numread == COPY_BUFFER_SIZE);
    }
    dstfile.Close();
    srcfile.Close();
    if (ret)
    {
        HANDLE file = CreateFileW(targetIO.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (file != INVALID_HANDLE_VALUE)
        { SetFileTime(file, times, times + 1, times + 2); CloseHandle(file); }
        SetFileAttributesW(targetIO.c_str(), info.dwFileAttributes | FILE_ATTRIBUTE_ENCRYPTED);
    }
    else if (Progress->GetWantCancel()) DeleteFileW(targetIO.c_str());
    return ret;
}

static BOOL RestoreDir(const std::wstring& source, const std::wstring& target)
{
    const std::string sourceUtf8 = UndeletePaths::Utf8(source.c_str());
    const std::string targetUtf8 = UndeletePaths::Utf8(target.c_str());
    FileProgress = 0;
    FileTotal = 1;
    TotalProgress++;
    UpdateRestoreProgress();
    Progress->SetSourceFileName(sourceUtf8.c_str());
    Progress->SetDestFileName(targetUtf8.c_str());
    BOOL skipped = FALSE;
    if (SalamanderSafeFile->SafeFileCreate(targetUtf8.c_str(), 0, 0, 0, TRUE, hProgressWnd, NULL, NULL,
                                         &DirSilentMask, TRUE, &skipped, NULL, 0, NULL, NULL) == INVALID_HANDLE_VALUE)
        return skipped;
    WIN32_FIND_DATAW data;
    const std::wstring pattern = UndeletePaths::IoPath(UndeletePaths::Join(source, L"*"));
    CRestoreFind find(HANDLES_Q(FindFirstFileW(pattern.c_str(), &data)));
    if (find.Handle == INVALID_HANDLE_VALUE)
        return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED);
    BOOL ret = TRUE;
    do
    {
        if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
        {
            const std::wstring childSource = UndeletePaths::Join(source, data.cFileName);
            const std::wstring childTarget = UndeletePaths::Join(target, data.cFileName);
            ret = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? RestoreDir(childSource, childTarget) : RestoreFile(childSource, childTarget);
        }
        if (!ret || Progress->GetWantCancel()) return FALSE;
    } while (FindNextFileW(find.Handle, &data));
    if (GetLastError() != ERROR_NO_MORE_FILES)
        return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED);
    DWORD attr = GetFileAttributesW(UndeletePaths::IoPath(source).c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) SetFileAttributesW(UndeletePaths::IoPath(target).c_str(), attr);
    return TRUE;
}

static QWORD GetDirSize(const std::wstring& source, BOOL* cancel)
{
    WIN32_FIND_DATAW data;
    const std::wstring pattern = UndeletePaths::IoPath(UndeletePaths::Join(source, L"*"));
    CRestoreFind find(HANDLES_Q(FindFirstFileW(pattern.c_str(), &data)));
    if (find.Handle == INVALID_HANDLE_VALUE)
    { *cancel = TRUE; String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED); return 0; }
    QWORD total = 0;
    do
    {
        if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
            total += (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? GetDirSize(UndeletePaths::Join(source, data.cFileName), cancel) + 1 : MAKEQWORD(data.nFileSizeLow, data.nFileSizeHigh) + 1;
        if (*cancel || (GetAsyncKeyState(VK_ESCAPE) & 0x8001) || SalamanderGeneral->GetSafeWaitWindowClosePressed())
        { *cancel = TRUE; return 0; }
    } while (FindNextFileW(find.Handle, &data));
    if (GetLastError() != ERROR_NO_MORE_FILES)
    { *cancel = TRUE; String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED); }
    return total;
}

struct CRestoreWaitScope
{
    ~CRestoreWaitScope() { SalamanderGeneral->DestroySafeWaitWindow(); }
};
struct CRestoreProgressScope
{
    HWND Parent, Window;
    CRestoreProgressScope(HWND parent, HWND window) : Parent(parent), Window(window) { EnableWindow(Parent, FALSE); }
    ~CRestoreProgressScope() { EnableWindow(Parent, TRUE); DestroyWindow(Window); Progress = NULL; hProgressWnd = NULL; }
};

BOOL RestoreEncryptedFiles(const CSalamanderDiskSelection& selection, const char* targetPath, HWND parent)
try
{
    const std::wstring target = UndeletePaths::Absolute(UndeletePaths::Wide(targetPath));
    if (target.empty() || selection.GetCount() == 0)
    { SetLastError(ERROR_INVALID_PARAMETER); return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED); }
    // Reject restoring a directory into itself or one of its descendants,
    // before creating any output or beginning recursive enumeration.
    for (int i = 0; i < selection.GetCount(); ++i)
    {
        const CSalamanderDiskSelectionItem& item = *selection.GetItem(i);
        if (!item.IsDir) continue;
        const std::wstring source = UndeletePaths::Comparable(UndeletePaths::Absolute(item.FullPathW));
        const std::wstring destination = UndeletePaths::Comparable(UndeletePaths::Join(target, item.NameW));
        if (destination.size() >= source.size() && !source.empty() &&
            CompareStringOrdinal(source.c_str(), static_cast<int>(source.size()), destination.c_str(), static_cast<int>(source.size()), TRUE) == CSTR_EQUAL &&
            (destination.size() == source.size() || destination[source.size()] == L'\\'))
        { SetLastError(ERROR_INVALID_PARAMETER); return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED); }
    }
    std::vector<wchar_t> root(SAL_MAX_PATH);
    DWORD flags = 0;
    if (!GetVolumePathNameW(UndeletePaths::IoPath(target).c_str(), root.data(), static_cast<DWORD>(root.size())) ||
        !GetVolumeInformationW(root.data(), NULL, 0, NULL, NULL, &flags, NULL, 0) || !(flags & FILE_SUPPORTS_ENCRYPTION))
        return String<char>::Error(IDS_RESTORE, IDS_NOEFS);

    FileProgress = FileTotal = TotalProgress = GrandTotal = 0;
    BOOL cancel = FALSE;
    {
        SalamanderGeneral->CreateSafeWaitWindow(String<char>::LoadStr(IDS_READINGTREE), NULL, 1500, TRUE, parent);
        CRestoreWaitScope wait;
        for (int i = 0; i < selection.GetCount() && !cancel; ++i)
        {
            const CSalamanderDiskSelectionItem& item = *selection.GetItem(i);
            GrandTotal += (item.IsDir ? GetDirSize(item.FullPathW, &cancel) : item.Size.Value) + 1;
        }
    }
    if (cancel || !SalamanderGeneral->TestFreeSpace(parent, targetPath, CQuadWord().SetUI64(GrandTotal), String<char>::LoadStr(IDS_RESTORE))) return FALSE;
    CRestoreProgressDlg dlg(parent, ooStatic);
    if (dlg.Create() == NULL) return String<char>::SysError(IDS_UNDELETE, IDS_ERROROPENINGPROGRESS);
    CRestoreProgressScope progress(parent, dlg.HWindow);
    Progress = &dlg;
    hProgressWnd = dlg.HWindow;
    SetForegroundWindow(hProgressWnd);
    SrcSilentMask = DstSilentMask = DirSilentMask = 0;
    for (int i = 0; i < selection.GetCount(); ++i)
    {
        const CSalamanderDiskSelectionItem& item = *selection.GetItem(i);
        const std::wstring destination = UndeletePaths::Join(target, item.NameW);
        if (!(item.IsDir ? RestoreDir(item.FullPathW, destination) : RestoreFile(item.FullPathW, destination))) return FALSE;
    }
    return TRUE;
}
catch (const std::bad_alloc&)
{
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return String<char>::SysError(IDS_UNDELETE, IDS_ERRORENCRYPTED);
}
