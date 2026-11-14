// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Format comparison errors without interpreting input. | Preserve localized messages while avoiding format-string injection. | #731 | GPT-6 |
// | 2026-11-10 | Hand off comparison activation before closing progress. | Keep a foreground comparison with its owner after UAC. | #731 | GPT-6.1 Sol |
// | 2026-11-10 | Retry denied nested comparisons in one rooted session. | Keep the grant across both passes and compare accessible siblings. | #731 | GPT-6.1 Sol |
// | 2026-11-10 | Offer protected content comparison from ordinary panels. | Reuse each verified side and reject changed sources. | #731 | GPT-6.1 Sol |
// | 2026-11-10 | Use each panel's protected scope for directory comparison. | Compare protected subtrees without misreporting unreadable data. | #731 | GPT-6.1 Sol |

#include "precomp.h"
#include "format_two_strings.h"

#include "plugins.h"
#include "fileswnd.h"
#include "mainwnd.h"
#include "cfgdlg.h"
#include "dialogs.h"
#include "common/widepath.h"
#include "uacbroker/f7pilot.h"

void GetFileDateAndTimeFromPanel(DWORD validFileData, CPluginDataInterfaceEncapsulation* pluginData,
                                 const CFileData* f, BOOL isDir, SYSTEMTIME* st, BOOL* validDate,
                                 BOOL* validTime)
{
    *validDate = FALSE;
    *validTime = FALSE;
    FILETIME ft;
    if (validFileData & (VALID_DATA_DATE | VALID_DATA_TIME)) // at least something is in LastWrite
    {
        if (FileTimeToLocalFileTime(&f->LastWrite, &ft) &&
            FileTimeToSystemTime(&ft, st))
        {
            if (validFileData & VALID_DATA_DATE)
                *validDate = TRUE;
            if (validFileData & VALID_DATA_TIME)
                *validTime = TRUE;
        }
    }
    if ((validFileData & VALID_DATA_PL_DATE) &&
        pluginData->NotEmpty() &&
        pluginData->GetLastWriteDate(f, isDir, st))
    {
        *validDate = TRUE;
    }
    if ((validFileData & VALID_DATA_PL_TIME) &&
        pluginData->NotEmpty() &&
        pluginData->GetLastWriteTime(f, isDir, st))
    {
        *validTime = TRUE;
    }
    if (!*validDate) // no date set, reset it...
    {
        st->wYear = 1602;
        st->wMonth = 1;
        st->wDay = 1;
        st->wDayOfWeek = 2;
    }
    if (!*validTime) // no time set, reset it...
    {
        st->wHour = 0;
        st->wMinute = 0;
        st->wSecond = 0;
        st->wMilliseconds = 0;
    }
}

int MyCompareFileTime(FILETIME* lf, FILETIME* rf, int* foundDSTShifts, int* compResNoDSTShiftIgn)
{
    int res = *compResNoDSTShiftIgn = CompareFileTime(lf, rf);
    if (res != 0)
    {
        CQuadWord left(lf->dwLowDateTime, lf->dwHighDateTime);
        CQuadWord right(rf->dwLowDateTime, rf->dwHighDateTime);
        // compute the difference between the times and its rounding
        unsigned __int64 diff = (right.Value > left.Value) ? right.Value - left.Value : left.Value - right.Value;
        if (diff == (unsigned __int64)3600 * 10000000 ||
            diff == (unsigned __int64)2 * 3600 * 10000000)
        {
            (*foundDSTShifts)++;
            if (Configuration.IgnoreDSTShifts)
                res = 0;
        }
    }
    return res;
}

// compare two files 'l' and 'r' by date/time
int CompareFilesByTime(CFilesWindow* leftPanel, const CFileData* l, BOOL lFAT,
                       CFilesWindow* rightPanel, CFileData* r, BOOL rFAT,
                       int* foundDSTShifts, int* compResNoDSTShiftIgn)
{
    SLOW_CALL_STACK_MESSAGE3("CompareFilesByTime(, , %d, , , %d, ,)", lFAT, rFAT);
    int res = *compResNoDSTShiftIgn = 0;

    SYSTEMTIME stLeft;
    BOOL validDateLeft, validTimeLeft;
    GetFileDateAndTimeFromPanel(leftPanel->ValidFileData, &leftPanel->PluginData, l, FALSE, &stLeft,
                                &validDateLeft, &validTimeLeft);
    SYSTEMTIME stRight;
    BOOL validDateRight, validTimeRight;
    GetFileDateAndTimeFromPanel(rightPanel->ValidFileData, &rightPanel->PluginData, r, FALSE, &stRight,
                                &validDateRight, &validTimeRight);

    if (validDateLeft && validDateRight ||                                    // if the time is unknown it is initialized to 0:00:00.000, so we ignore time...
        !validDateLeft && !validDateRight && validTimeLeft && validTimeRight) // date is unknown (zeroed), times are both known, so nothing prevents comparison
    {
        FILETIME lf, rf;
        if (Configuration.UseTimeResolution)
        {
            // trim time with second precision
            stLeft.wMilliseconds = 0;
            stRight.wMilliseconds = 0;
            // convert time to numbers
            if (!SystemTimeToFileTime(&stLeft, &lf))
            {
                TRACE_E("CompareFilesByTime(): date&time of left file is invalid!");
                *compResNoDSTShiftIgn = res = -1;
            }
            else
            {
                if (!SystemTimeToFileTime(&stRight, &rf))
                {
                    TRACE_E("CompareFilesByTime(): date&time of right file is invalid!");
                    *compResNoDSTShiftIgn = res = 1;
                }
                else
                {
                    CQuadWord left(lf.dwLowDateTime, lf.dwHighDateTime);
                    CQuadWord right(rf.dwLowDateTime, rf.dwHighDateTime);
                    unsigned __int64 ld = left.Value / 10000000;
                    unsigned __int64 rd = right.Value / 10000000;
                    // compute the difference between the times and its rounding
                    unsigned __int64 diff = (rd > ld) ? rd - ld : ld - rd;
                    *compResNoDSTShiftIgn = res = (left < right) ? -1 : 1;
                    if (diff < 100000)
                    {
                        int d = (int)diff;
                        if (d <= Configuration.TimeResolution)
                            *compResNoDSTShiftIgn = res = 0;
                        else
                        {
                            int dIgnOneHour = d < 3600 ? 3600 - d : d - 3600;
                            int dIgnTwoHours = d < 2 * 3600 ? 2 * 3600 - d : d - 2 * 3600;
                            if (dIgnOneHour <= Configuration.TimeResolution || dIgnTwoHours <= Configuration.TimeResolution)
                            {
                                (*foundDSTShifts)++;
                                if (Configuration.IgnoreDSTShifts)
                                    res = 0;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (!SystemTimeToFileTime(&stLeft, &lf))
            {
                TRACE_E("CompareFilesByTime(): date&time of left file is invalid!");
                *compResNoDSTShiftIgn = res = -1;
            }
            else
            {
                if (!SystemTimeToFileTime(&stRight, &rf))
                {
                    TRACE_E("CompareFilesByTime(): date&time of right file is invalid!");
                    *compResNoDSTShiftIgn = res = 1;
                }
                else
                {
                    if (lFAT == rFAT)
                        res = MyCompareFileTime(&lf, &rf, foundDSTShifts, compResNoDSTShiftIgn); // same filesystems -> OK
                    else                                                                         // FAT plus a different filesystem -> adjust the non-FAT time to FAT time (by 2 seconds)
                    {
                        WORD date, time; // FAT values
                        FILETIME t;
                        if (lFAT)
                        {
                            FileTimeToDosDateTime(&rf, &date, &time);
                            DosDateTimeToFileTime(date, time, &t);
                            res = MyCompareFileTime(&lf, &t, foundDSTShifts, compResNoDSTShiftIgn);
                        }
                        else
                        {
                            FileTimeToDosDateTime(&lf, &date, &time);
                            DosDateTimeToFileTime(date, time, &t);
                            res = MyCompareFileTime(&t, &rf, foundDSTShifts, compResNoDSTShiftIgn);
                        }
                    }
                }
            }
        }
    }
    else
    {
        if (validDateLeft)
            res = 1; // any date is newer than an unknown date
        else
        {
            if (validDateRight)
                res = -1; // an unknown date is older than any date
            else          // neither date is known
            {
                if (validTimeLeft)
                    res = 1; // any time is newer than an unknown time
                else
                {
                    if (validTimeRight)
                        res = -1; // an unknown time is older than any time
                }
            }
        }
        *compResNoDSTShiftIgn = res;
    }
    return res;
}

void GetFileSizeFromPanel(DWORD validFileData, CPluginDataInterfaceEncapsulation* pluginData,
                          const CFileData* f, BOOL isDir, CQuadWord* size, BOOL* validSize)
{
    *validSize = FALSE;
    if (validFileData & VALID_DATA_SIZE)
    {
        *validSize = TRUE;
        *size = f->Size;
    }
    else
    {
        if ((validFileData & VALID_DATA_PL_SIZE) &&
            pluginData->NotEmpty() &&
            pluginData->GetByteSize(f, isDir, size))
        {
            *validSize = TRUE;
        }
    }
}

// it compares two files 'l' and 'r' by size
int CompareFilesBySize(CFilesWindow* leftPanel, CFileData* l, CFilesWindow* rightPanel, CFileData* r)
{
    CALL_STACK_MESSAGE_NONE

    BOOL leftValidSize;
    CQuadWord leftSize;
    GetFileSizeFromPanel(leftPanel->ValidFileData, &leftPanel->PluginData, l, FALSE, &leftSize, &leftValidSize);
    BOOL rightValidSize;
    CQuadWord rightSize;
    GetFileSizeFromPanel(rightPanel->ValidFileData, &rightPanel->PluginData, r, FALSE, &rightSize, &rightValidSize);

    if (leftValidSize && rightValidSize)
    {
        if (leftSize != rightSize)
            return (leftSize > rightSize) ? 1 : -1;
    }
    else
    {
        if (leftValidSize)
            return 1; // anything is larger than an unknown size
        else
        {
            if (rightValidSize)
                return -1; // an unknown size is smaller than anything
        }
    }
    return 0;
}

// compares files 'file1' and 'file2' specified by full path by their content
// on successful comparison, it returns TRUE and sets 'different'
// variable (TRUE if a difference was found, otherwise FALSE)
// on error or user abort, returns FALSE and sets 'canceled' variable
// (TRUE if the user canceled the operation, otherwise FALSE)

#define COMPARE_BUFFER_SIZE (2 * 1024 * 1024) // buffer size for comparison in bytes (does not necessarily need to be fully used)
#define COMPARE_BLOCK_SIZE (32 * 1024)        // size of a block read continuously from the file; NOTE: COMPARE_BUFFER_SIZE must be divisible by COMPARE_BLOCK_SIZE
#define COMPARE_BLOCK_GROUP 8                 // how many blocks can be read at once when reading is fast enough (more than 1 MB/s, see COMPARE_BUF_TIME_LIMIT); NOTE: the number of blocks in the buffer (COMPARE_BUFFER_SIZE / COMPARE_BLOCK_SIZE) must be divisible by COMPARE_BLOCK_GROUP
#define COMPARE_BUF_TIME_LIMIT 2000           // time limit in milliseconds for reading the entire buffer (COMPARE_BUFFER_SIZE) - if met, blocks are read in groups COMPARE_BLOCK_GROUP which speeds up network reading on Vista+ 2-3x; otherwise reading is done block by block (COMPARE_BLOCK_SIZE)

void AddProgressSizeWithLimit(CCmpDirProgressDialog* progressDlg, DWORD read, CQuadWord* fileProgressTotal, const CQuadWord& sizeLimit)
{
    if (*fileProgressTotal + CQuadWord(read, 0) > sizeLimit)
        read = (DWORD)(sizeLimit - *fileProgressTotal).Value;
    if (read > 0)
    {
        progressDlg->AddSize(CQuadWord(read, 0));
        *fileProgressTotal += CQuadWord(read, 0);
    }
}

// Keep the panel grant separate from the read grant of this comparison side.
class CCompareDirectorySide
{
public:
    CCompareDirectorySide(CFilesWindow* panel, bool compareContents) :
        Panel(panel),
        CompareContents(compareContents),
        Root(SalPathRemoveExtendedPrefixW(panel->GetPathW())),
        Generation(panel->GetListingIdentityGeneration()),
        Listing(panel->Is(ptDisk) && panel->ProtectedListing &&
                        panel->ProtectedListing->Contains(Root) ?
                    panel->ProtectedListing.get() : nullptr)
    {
    }

    bool IsProtected() const { return Listing != nullptr; }

    DWORD List(const std::wstring& path, std::vector<WIN32_FIND_DATAW>& entries) const
    {
        return Listing && IsCurrent() ? Listing->List(path, entries) : ERROR_INVALID_HANDLE;
    }

    DWORD RetryList(HWND owner, const std::wstring& path, std::vector<WIN32_FIND_DATAW>& entries)
    {
        UacF7::MoveFileIdentity identity{};
        const bool verified = UacF7::ListingSession::CaptureDirectoryIdentity(path, identity);
        if (!verified && !ContentRequested)
            return ERROR_ACCESS_DENIED;
        const DWORD authorized = AuthorizeJob(owner, path);
        return authorized ? authorized : Content.List(path, entries, verified ? &identity : nullptr);
    }

    DWORD AuthorizeContent(HWND owner, const std::wstring& path, UacF7::PreparedLeaf& file)
    {
        if (!IsCurrent())
            return ERROR_INVALID_HANDLE;
        if (ContentRequested && !Content.IsOpen())
            return ContentStartError ? ContentStartError : ERROR_BROKEN_PIPE;

        if (Listing)
        {
            const DWORD captured = Listing->PrepareViewer(path, file);
            if (captured)
                return captured;
        }
        else if (!file.PrepareView(path))
        {
            if (!Content.IsOpen())
                return ERROR_ACCESS_DENIED;
            const DWORD captured = Content.PrepareViewer(path, file);
            if (captured)
                return captured;
        }
        return AuthorizeJob(owner, path);
    }

    DWORD OpenContent(const std::wstring& path, ULONGLONG& size, UacF7::ArchiveRevision& revision)
    {
        if (!IsCurrent())
            return ERROR_INVALID_HANDLE;
        FILETIME modified{};
        return Content.OpenContent(path, size, modified, revision);
    }

    bool HasProtectedWork() const { return Listing || ContentRequested; }
    UacF7::ListingSession& ContentSession() { return Content; }

private:
    DWORD AuthorizeJob(HWND owner, const std::wstring& path)
    {
        if (!IsCurrent())
            return ERROR_INVALID_HANDLE;
        if (Content.IsOpen())
            return Content.Contains(path) ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
        if (ContentRequested)
            return ContentStartError ? ContentStartError : ERROR_BROKEN_PIPE;

        if (Listing)
        {
            UacF7::FindRootIdentity identity;
            const DWORD captured = Listing->CaptureFindRoot(Root, identity);
            if (captured)
                return captured;
            if (!Content.PrepareFindRoot(identity, true))
                return ERROR_INVALID_HANDLE;
        }
        else if (!Content.PrepareComparison(Root, CompareContents))
            return ERROR_ACCESS_DENIED;

        const int prompt = CompareContents ? IDS_UAC_COMPARE_CONTENT_CONFIRM : IDS_UAC_LIST_CONFIRM;
        ContentRequested = true;
        if (!ConfirmProtectedRead(owner, IDS_COMPAREDIRSTITLE, prompt, Root))
            return ERROR_CANCELLED;
        if (!IsCurrent())
            return ERROR_INVALID_HANDLE;
        ContentStartError = Content.Start(owner);
        return ContentStartError;
    }

    bool IsCurrent() const
    {
        return Panel->GetListingIdentityGeneration() == Generation &&
               (!Listing || (Panel->ProtectedListing.get() == Listing && Listing->Contains(Root)));
    }

    CFilesWindow* const Panel;
    const bool CompareContents;
    const std::wstring Root;
    const ULONGLONG Generation;
    UacF7::ListingSession* const Listing;
    UacF7::ListingSession Content;
    bool ContentRequested = false;
    DWORD ContentStartError = ERROR_SUCCESS;
};

class CCompareFileInput
{
public:
    ~CCompareFileInput()
    {
        if (File != INVALID_HANDLE_VALUE)
            HANDLES(CloseHandle(File));
        CloseContent();
    }

    CCompareFileInput() = default;
    CCompareFileInput(const CCompareFileInput&) = delete;
    CCompareFileInput& operator=(const CCompareFileInput&) = delete;

    DWORD Prepare(HWND owner, CCompareDirectorySide& side, const std::wstring& path, const CFileData& expected)
    {
        if (path.empty())
            return ERROR_INVALID_NAME;
        const std::wstring nativePath = path.size() >= MAX_PATH ? SalPathAddExtendedPrefixW(path.c_str()) : path;
        File = HANDLES_Q(CreateFileW(nativePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
        if (File != INVALID_HANDLE_VALUE)
            return ERROR_SUCCESS;
        const DWORD error = GetLastError();
        constexpr DWORD unsupported = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_ENCRYPTED |
                                      FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_RECALL_ON_OPEN |
                                      FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS;
        if (error != ERROR_ACCESS_DENIED || (expected.Attr & unsupported))
            return error;

        ProtectedPath = SalPathRemoveExtendedPrefixW(path.c_str());
        const DWORD authorized = side.AuthorizeContent(owner, ProtectedPath, Identity);
        if (!authorized)
        {
            ProtectedSide = &side;
            ExpectedSize = expected.Size.Value;
            ExpectedWriteTime = expected.LastWrite;
        }
        return authorized;
    }

    DWORD OpenContent()
    {
        if (!ProtectedSide)
            return ERROR_SUCCESS;
        UacF7::ArchiveRevision revision;
        const DWORD opened = ProtectedSide->OpenContent(ProtectedPath, Size, revision);
        if (opened)
            return opened;
        Content = &ProtectedSide->ContentSession();
        ULARGE_INTEGER writeTime{};
        writeTime.LowPart = ExpectedWriteTime.dwLowDateTime;
        writeTime.HighPart = ExpectedWriteTime.dwHighDateTime;
        if (!Identity.MatchesReadSource(revision) || Size != ExpectedSize ||
            static_cast<ULONGLONG>(revision.modified) != writeTime.QuadPart)
        {
            CloseContent();
            return ERROR_FILE_INVALID;
        }
        return ERROR_SUCCESS;
    }

    DWORD Read(char* buffer, DWORD bytes, DWORD& received)
    {
        if (!Content)
            return ReadFile(File, buffer, bytes, &received, nullptr) ? ERROR_SUCCESS : GetLastError();
        received = 0;
        const DWORD count = static_cast<DWORD>(min(static_cast<ULONGLONG>(bytes), Size - Offset));
        while (received < count)
        {
            const DWORD block = min(count - received, UacF7::archiveReadBlock);
            const DWORD error = Content->ReadContent(Offset, buffer + received, block);
            if (error)
                return error;
            received += block;
            Offset += block;
        }
        return ERROR_SUCCESS;
    }

    DWORD CloseContent()
    {
        if (!Content)
            return ERROR_SUCCESS;
        auto* session = Content;
        Content = nullptr;
        return session->CloseContent();
    }

private:
    HANDLE File = INVALID_HANDLE_VALUE;
    CCompareDirectorySide* ProtectedSide = nullptr;
    std::wstring ProtectedPath;
    UacF7::PreparedLeaf Identity;
    ULONGLONG ExpectedSize = 0;
    FILETIME ExpectedWriteTime{};
    UacF7::ListingSession* Content = nullptr;
    ULONGLONG Size = 0;
    ULONGLONG Offset = 0;
};

static void CompareError(HWND owner, CCmpDirProgressDialog* progress, int messageId, const char* path,
                         DWORD error, BOOL* canceled)
{
    const char* details = GetErrorText(error);
    const std::string message = FormatTwoStrings(LoadStr(messageId), path, details);
    progress->FlushDataToControls();
    *canceled = SalMessageBox(owner, message.c_str(), LoadStr(IDS_ERRORTITLE),
                              MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL;
}

BOOL CompareFilesByContent(HWND hWindow, CCmpDirProgressDialog* progressDlg,
                           CCompareDirectorySide& left, const std::wstring& file1, const CFileData& expected1,
                           CCompareDirectorySide& right, const std::wstring& file2, const CFileData& expected2,
                           BOOL* different, BOOL* canceled)
{
    const CQuadWord bothFileSize = expected1.Size + expected2.Size;
    const std::string display1 = SalWideToMultiBytePath(file1.c_str(), CP_UTF8);
    const std::string display2 = SalWideToMultiBytePath(file2.c_str(), CP_UTF8);
    *canceled = !progressDlg->Continue(); // repaint the completed previous file
    if (*canceled)
        return FALSE;

    progressDlg->SetSource(display1.c_str());
    progressDlg->SetTarget(display2.c_str());
    progressDlg->SetFileSize(bothFileSize);
    progressDlg->SetActualFileSize(CQuadWord(0, 0));

    CCompareFileInput input1;
    CCompareFileInput input2;
    const char* errorPath = display1.c_str();
    DWORD err = input1.Prepare(hWindow, left, file1, expected1);
    if (!err)
    {
        errorPath = display2.c_str();
        err = input2.Prepare(hWindow, right, file2, expected2);
    }
    // Do not hold an open broker stream while waiting for the other side's consent.
    if (!err)
    {
        errorPath = display1.c_str();
        err = input1.OpenContent();
    }
    if (!err)
    {
        errorPath = display2.c_str();
        err = input2.OpenContent();
    }
    if (err)
    {
        input1.CloseContent();
        if (err == ERROR_CANCELLED)
            *canceled = TRUE;
        else
            CompareError(hWindow, progressDlg, IDS_ERROR_OPENING_FILE, errorPath, err, canceled);
        if (!*canceled)
            progressDlg->AddSize(bothFileSize);
        return FALSE;
    }

    const std::unique_ptr<char, decltype(&free)> buffer1(static_cast<char*>(malloc(COMPARE_BUFFER_SIZE)), free);
    if (!buffer1)
    {
        TRACE_E(LOW_MEMORY);
        *canceled = TRUE;
        return FALSE;
    }
    char buffer2[COMPARE_BLOCK_GROUP * COMPARE_BLOCK_SIZE];
    CQuadWord fileProgressTotal(0, 0);
    BOOL ret = FALSE;
    DWORD read1;
    DWORD read2;

    BOOL readingIsFast1 = FALSE; // FALSE = wait until speed reaches the limit and we can read in groups (COMPARE_BLOCK_GROUP)
    BOOL readingIsFast2 = FALSE;
    while (TRUE)
    {
        int blockCount = COMPARE_BUFFER_SIZE / COMPARE_BLOCK_SIZE;

        // load the next part from 'file1' into 'buffer1'
        // read the entire buffer in 32KB blocks from one file (on W2K and XP this is fastest when both files are on one physical disk; on Vista it's only slightly slower than sequential reading)
        read1 = 0;
        DWORD readBegTime = GetTickCount();
        for (int block = 0; block < blockCount;)
        {
            DWORD locRead;
            BOOL canReadGroup = readingIsFast1 && (block % COMPARE_BLOCK_GROUP) == 0 && block + COMPARE_BLOCK_GROUP <= blockCount;
            DWORD readBlockSize = canReadGroup ? COMPARE_BLOCK_GROUP * COMPARE_BLOCK_SIZE : COMPARE_BLOCK_SIZE;
            err = input1.Read(buffer1.get() + block * COMPARE_BLOCK_SIZE, readBlockSize, locRead);
            if (err)
            {
                errorPath = display1.c_str();
                break;
            }

            AddProgressSizeWithLimit(progressDlg, locRead, &fileProgressTotal, bothFileSize);
            if (!progressDlg->Continue()) // give the dialog a chance to repaint
            {
                *canceled = TRUE;
                break;
            }

            read1 += locRead;
            if (locRead != readBlockSize)
                break; // EOF
            if (GetTickCount() - readBegTime > 200)
            { // slow reading (probably a slow network disk (VPN) or floppy) - reduce the buffer, at worst we'll compare block by block
                blockCount = block + (canReadGroup ? COMPARE_BLOCK_GROUP : 1);
                break;
            }
            block += canReadGroup ? COMPARE_BLOCK_GROUP : 1;
        }
        if (err || *canceled)
            break;
        readingIsFast1 = WindowsVistaAndLater &&
                         GetTickCount() - readBegTime < (DWORD)(((unsigned __int64)read1 * COMPARE_BUF_TIME_LIMIT) / COMPARE_BUFFER_SIZE); // measure the speed so it is over 1 MB/s

        // load the next part from 'file2' into 'buffer2'
        // read the whole buffer in blocks from one file (on W2K and XP this is fastest when both files are on one physical disk; on Vista it's only slightly slower than sequential reading)
        read2 = 0;
        readBegTime = GetTickCount();
        for (int block = 0; block < blockCount;)
        {
            DWORD locRead;
            BOOL canReadGroup = readingIsFast2 && (block % COMPARE_BLOCK_GROUP) == 0 && block + COMPARE_BLOCK_GROUP <= blockCount;
            DWORD readBlockSize = canReadGroup ? COMPARE_BLOCK_GROUP * COMPARE_BLOCK_SIZE : COMPARE_BLOCK_SIZE;
            err = input2.Read(buffer2, readBlockSize, locRead);
            if (err)
            {
                errorPath = display2.c_str();
                break;
            }

            AddProgressSizeWithLimit(progressDlg, locRead, &fileProgressTotal, bothFileSize);
            if (!progressDlg->Continue()) // give the dialog a chance to repaint
            {
                *canceled = TRUE;
                break;
            }

            read2 += locRead;
            if (read1 < read2 || // files are now of different length => content differs
                locRead > 0 && memcmp(buffer1.get() + block * COMPARE_BLOCK_SIZE, buffer2, locRead) != 0)
            { // file contents differ, no point in continuing reading
                *different = TRUE;
                ret = TRUE;
                break;
            }
            if (locRead != readBlockSize)
            { // The second file reached EOF. Compare the lengths of the final blocks.
                *different = read1 != read2;
                ret = TRUE;
                break;
            }
            block += canReadGroup ? COMPARE_BLOCK_GROUP : 1;
            if (readingIsFast2 &&
                GetTickCount() - readBegTime > (DWORD)(((unsigned __int64)read2 * COMPARE_BUF_TIME_LIMIT) / COMPARE_BUFFER_SIZE))
            {
                // detecting slowdown, continue block by block to keep progress smooth and allow canceling
                readingIsFast2 = FALSE;
            }
        }
        if (err || ret || *canceled)
            break;
        readingIsFast2 = WindowsVistaAndLater &&
                         GetTickCount() - readBegTime < (DWORD)(((unsigned __int64)read2 * COMPARE_BUF_TIME_LIMIT) / COMPARE_BUFFER_SIZE); // measure the speed so it is over 1 MB/s



    }
    const DWORD close1 = input1.CloseContent();
    const DWORD close2 = input2.CloseContent();
    if (!*canceled && (err || close1 || close2))
    {
        if (!err)
        {
            err = close1 ? close1 : close2;
            errorPath = close1 ? display1.c_str() : display2.c_str();
        }
        CompareError(hWindow, progressDlg, IDS_ERROR_READING_FILE, errorPath, err, canceled);
        ret = FALSE;
    }
    if (!*canceled)
        progressDlg->AddSize(bothFileSize - fileProgressTotal);
    return ret;
}

static std::wstring CompareDiskPath(const CFilesWindow* panel, const std::string& subPath)
{
    std::wstring path = SalPathRemoveExtendedPrefixW(panel->GetPathW());
    if (!subPath.empty())
        SalPathAppendW(path, SalMultiByteToWidePath(subPath.c_str(), CP_UTF8).c_str());
    return path;
}

// Disk subpaths are UTF-8. Archive subpaths retain the plugin's existing encoding.
static std::string CompareChildPath(CFilesWindow* panel, const std::string& parent, const CFileData& child)
{
    std::string path = parent;
    if (!path.empty() && path.back() != '\\')
        path += '\\';
    if (panel->Is(ptDisk))
    {
        const auto name = child.UseWideName() ? std::wstring(child.NameW) :
                          SalMultiByteToWidePath(child.Name, IsValidPathUtf8Text(child.Name) ? CP_UTF8 : CP_ACP);
        path += SalWideToMultiBytePath(name.c_str(), CP_UTF8);
    }
    else
        path += child.Name;
    return path;
}

// loads directories and files into the arrays 'dirs' and 'files'
// the source path is determined by combining the path in the 'panel' with 'subPath'
// 'hWindow' is the window for displaying message boxes
// 'progressDlg' is used to update progress after loading X files/directories
// returns TRUE if both arrays were successfully filled
// returns FALSE on error or if the user canceled the operation
// (the variable 'canceled' will then be set)
//
// supports ptDisk and ptZIPArchive

BOOL ReadDirsAndFilesAux(HWND hWindow, DWORD flags, CCmpDirProgressDialog* progressDlg,
                         CFilesWindow* panel, CCompareDirectorySide& side, const std::string& subPath,
                         CFilesArray* dirs, CFilesArray* files, BOOL* canceled, BOOL getTotal)
{
    BOOL ignFileNames = (flags & COMPARE_DIRECTORIES_IGNFILENAMES) != 0;
    BOOL ignDirNames = (flags & COMPARE_DIRECTORIES_IGNDIRNAMES) != 0;

    if (panel->Is(ptDisk))
    {
        const std::wstring path = CompareDiskPath(panel, subPath);
        DWORD counter = 0;
        const auto add = [&](const WIN32_FIND_DATAW& data) -> bool
        {
            const wchar_t* name = data.cFileName;
            if (!*name || wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
                return true;
            if (counter++ > 200)
            {
                counter = 0;
                if (!progressDlg->Continue())
                    return false;
            }

            const bool isDir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (isDir ? ignDirNames && Configuration.CompareIgnoreDirsMasks.AgreeMasksW(name, nullptr) :
                        ignFileNames && Configuration.CompareIgnoreFilesMasks.AgreeMasksW(name, nullptr))
                return true;
            CFileData file{};
            const std::string nameUtf8 = SalWideToMultiBytePath(name, CP_UTF8);
            file.Name = _strdup(nameUtf8.c_str());
            file.NameW = _wcsdup(name);
            if (file.Name && file.NameW)
            {
                file.NameLen = static_cast<int>(min(nameUtf8.size(), size_t{511}));
                const char* dot = isDir && !Configuration.SortDirsByExt ? nullptr : strrchr(file.Name, '.');
                file.Ext = dot ? file.Name + (dot - file.Name + 1) : file.Name + nameUtf8.size();
                file.PluginData = -1;
                file.IconOverlayIndex = ICONOVERLAYINDEX_NOTUSED;
                file.Size = CQuadWord(data.nFileSizeLow, data.nFileSizeHigh);
                file.Attr = data.dwFileAttributes;
                file.LastWrite = data.ftLastWriteTime;

                auto* array = isDir ? dirs : files;
                array->Add(file);
                if (array->IsGood())
                    return true;
            }
            free(file.NameW);
            free(file.Name);
            TRACE_E(LOW_MEMORY);
            return false;
        };

        DWORD error = ERROR_SUCCESS;
        bool completed = true;
        std::vector<WIN32_FIND_DATAW> entries;
        if (side.IsProtected())
            error = side.List(path, entries);
        else
        {
            std::wstring pattern = path;
            SalPathAppendW(pattern, L"*");
            if (pattern.size() >= MAX_PATH)
                pattern = SalPathAddExtendedPrefixW(pattern.c_str());
            WIN32_FIND_DATAW data{};
            HANDLE find = HANDLES_Q(FindFirstFileW(pattern.c_str(), &data));
            if (find == INVALID_HANDLE_VALUE)
            {
                error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND || error == ERROR_NO_MORE_FILES)
                    error = ERROR_SUCCESS;
            }
            else
            {
                do
                {
                    completed = add(data);
                } while (completed && FindNextFileW(find, &data));
                error = GetLastError();
                HANDLES(FindClose(find));
                if (error == ERROR_NO_MORE_FILES)
                    error = ERROR_SUCCESS;
            }
            if (completed && error == ERROR_ACCESS_DENIED)
            {
                error = side.RetryList(hWindow, path, entries);
                if (!error)
                {
                    dirs->DestroyMembers();
                    files->DestroyMembers();
                }
            }
        }

        if (!error)
            for (const auto& entry : entries)
                if (!add(entry))
                {
                    completed = false;
                    break;
                }

        if (!completed || error == ERROR_CANCELLED)
        {
            *canceled = TRUE;
            return FALSE;
        }
        if (error)
        {
            *canceled = FALSE;
            if (!getTotal)
                CompareError(hWindow, progressDlg, IDS_CANNOTREADDIR,
                             SalWideToMultiBytePath(path.c_str(), CP_UTF8).c_str(), error, canceled);
            return FALSE;
        }
    }
    else if (panel->Is(ptZIPArchive))
    {
        CFilesArray* zipDirs = panel->GetArchiveDirDirs(subPath.c_str());
        CFilesArray* zipFiles = panel->GetArchiveDirFiles(subPath.c_str());

        if (zipFiles == NULL || zipDirs == NULL)
        {
            *canceled = TRUE;
            return FALSE; // low memory, bail out
        }

        files->SetDeleteData(FALSE); // shallow data copies only
        dirs->SetDeleteData(FALSE);  // shallow data copies only

        int i;
        for (i = 0; i < zipDirs->Count; i++)
        {
            CFileData* f = &zipDirs->At(i);
            if (!ignDirNames || !(f->UseWideName() ? Configuration.CompareIgnoreDirsMasks.AgreeMasksW(f->NameW, NULL) : Configuration.CompareIgnoreDirsMasks.AgreeMasks(f->Name, NULL)))
            {
                dirs->Add(*f);
                if (!dirs->IsGood())
                {
                    TRACE_E(LOW_MEMORY);
                    *canceled = TRUE;
                    return FALSE; // low memory, bail out
                }
            }
        }

        for (i = 0; i < zipFiles->Count; i++)
        {
            CFileData* f = &zipFiles->At(i);
            if (!ignFileNames || !(f->UseWideName() ? Configuration.CompareIgnoreFilesMasks.AgreeMasksW(f->NameW, NULL) : Configuration.CompareIgnoreFilesMasks.AgreeMasks(f->Name, f->Ext)))
            {
                files->Add(*f);
                if (!files->IsGood())
                {
                    TRACE_E(LOW_MEMORY);
                    *canceled = TRUE;
                    return FALSE; // low memory, bail out
                }
            }
        }
    }
    else
    {
        TRACE_E("not implemented");
        *canceled = TRUE;
        return FALSE;
    }

    return TRUE;
}

// recursive function searching for differences between directories
// directories are determined by the paths in the left and right panel
// and the variables 'leftSubDir' and 'rightSubDir'
// leftFAT and rightFAT indicate whether the respective panel is on a FAT system; if an archive is opened,
// the corresponding xxxFAT will be set to FALSE
// 'flags' specifies comparison criteria and comes from the COMPARE_DIRECTORIES_xxx family
// the function returns TRUE on successful comparison and sets 'different' (TRUE
// if the directories differ, otherwise FALSE).
// on error or user abort it returns FALSE
// and sets the variable 'canceled' (TRUE if aborted by the user, otherwise FALSE)

// supports ptDisk and ptZIPArchive

BOOL CompareDirsAux(HWND hWindow, CCmpDirProgressDialog* progressDlg,
                    CFilesWindow* leftPanel, CCompareDirectorySide& leftSide, const std::string& leftSubDir, BOOL leftFAT,
                    CFilesWindow* rightPanel, CCompareDirectorySide& rightSide, const std::string& rightSubDir, BOOL rightFAT,
                    DWORD flags, BOOL* different, BOOL* canceled,
                    BOOL getTotal, CQuadWord* total, int* foundDSTShifts)
{
    // left/rightPanel and left/rightSubDir specify the path
    // whose directories and files are stored in the arrays below
    CFilesArray leftDirs;
    CFilesArray rightDirs;
    int foundDSTShiftsInThisDir = 0;

    { // local block to limit the scope of leftFiles & rightFiles (saves memory before recursing into subdirectories)
        CFilesArray leftFiles;
        CFilesArray rightFiles;

        // from the previous file comparison, a value remained set
        if ((flags & COMPARE_DIRECTORIES_BYCONTENT) && (!getTotal))
            progressDlg->SetActualFileSize(CQuadWord(0, 0)); // set to 0%

        // set texts in the progress dialog
        const auto displayPath = [](CFilesWindow* panel, const std::string& subDir)
        {
            if (panel->Is(ptDisk))
                return SalWideToMultiBytePath(CompareDiskPath(panel, subDir).c_str(), CP_UTF8);
            return std::string(panel->GetPath()) + "\\" + subDir;
        };
        progressDlg->SetSource(displayPath(leftPanel, leftSubDir).c_str());
        progressDlg->SetTarget(displayPath(rightPanel, rightSubDir).c_str());

        if (!progressDlg->Continue())
        {
            *canceled = TRUE;
            return FALSE;
        }

        // load directories and files for the left side
        if (!ReadDirsAndFilesAux(hWindow, flags, progressDlg, leftPanel, leftSide, leftSubDir, &leftDirs, &leftFiles, canceled, getTotal))
            return FALSE;

        // and the right panel
        if (!ReadDirsAndFilesAux(hWindow, flags, progressDlg, rightPanel, rightSide, rightSubDir, &rightDirs, &rightFiles, canceled, getTotal))
            return FALSE;

        // if the counts differ in the left and right panel, the directories differ
        if (leftDirs.Count != rightDirs.Count || leftFiles.Count != rightFiles.Count)
        {
            *different = TRUE;
            return TRUE;
        }

        // at this point, the number of directories and files is the same in both panels

        // sort the arrays by name so we can compare them
        if (leftDirs.Count > 1)
            SortNameExt(leftDirs, 0, leftDirs.Count - 1, FALSE);
        if (leftFiles.Count > 1)
            SortNameExt(leftFiles, 0, leftFiles.Count - 1, FALSE);
        if (rightDirs.Count > 1)
            SortNameExt(rightDirs, 0, rightDirs.Count - 1, FALSE);
        if (rightFiles.Count > 1)
            SortNameExt(rightFiles, 0, rightFiles.Count - 1, FALSE);

        // first compare files by name, time and attributes
        // postpone comparing by content because it's slow and if
        // we find a difference at this level, we save time
        BOOL timeDiffWithDSTShiftExists = FALSE;
        CFileData *leftFile, *rightFile;
        int i;
        for (i = 0; i < leftFiles.Count; i++)
        {
            leftFile = &leftFiles[i];
            rightFile = &rightFiles[i];

            // By Name
            if (CmpNameExtIgnCase(*leftFile, *rightFile) != 0)
            {
                *different = TRUE;
                return TRUE;
            }

            // By Size
            if (flags & COMPARE_DIRECTORIES_BYSIZE)
            {
                if (CompareFilesBySize(leftPanel, leftFile, rightPanel, rightFile) != 0)
                {
                    *different = TRUE;
                    return TRUE;
                }
            }

            // By Attributes
            if (flags & COMPARE_DIRECTORIES_BYATTR)
            {
                if ((leftPanel->ValidFileData & VALID_DATA_ATTRIBUTES) &&
                    (rightPanel->ValidFileData & VALID_DATA_ATTRIBUTES))
                {
                    if ((leftFile->Attr & DISPLAYED_ATTRIBUTES) != (rightFile->Attr & DISPLAYED_ATTRIBUTES))
                    {
                        *different = TRUE;
                        return TRUE;
                    }
                }
            }

            // compare files by content -- here we only compare file sizes as a trivial test for mismatched content
            if (flags & COMPARE_DIRECTORIES_BYCONTENT)
            {
                if (leftFile->Size != rightFile->Size)
                {
                    if (!leftPanel->Is(ptDisk) || !rightPanel->Is(ptDisk))
                    {
                        TRACE_E("not implemented");
                        *canceled = FALSE;
                        return FALSE;
                    }
                    *different = TRUE;
                    return TRUE;
                }
            }

            // By Time -- because of DST time shifts we test timestamps last (we don't report a DST problem when files also differ in size or attributes)
            if (flags & COMPARE_DIRECTORIES_BYTIME)
            {
                int isDSTShift = 0; // 1 = the timestamps of the currently compared pair of files differ by exactly one or two hours, 0 = they don’t
                int compResNoDSTShiftIgn;
                if (CompareFilesByTime(leftPanel, leftFile, leftFAT, rightPanel, rightFile, rightFAT,
                                       &isDSTShift, &compResNoDSTShiftIgn) != 0)
                {
                    if (isDSTShift != 0)
                        timeDiffWithDSTShiftExists = TRUE; // try to find another difference (ideally without a DST warning)
                    else
                    {
                        *different = TRUE;
                        return TRUE;
                    }
                }
                else
                    foundDSTShiftsInThisDir += isDSTShift;
            }
        }

        // compare directories
        CFileData *leftDir, *rightDir;
        for (i = 0; i < leftDirs.Count; i++)
        {
            leftDir = &leftDirs[i];
            rightDir = &rightDirs[i];

            // By Name
            if (CmpNameExtIgnCase(*leftDir, *rightDir) != 0)
            {
                *different = TRUE;
                return TRUE;
            }

            // By Attributes
            if (flags & COMPARE_DIRECTORIES_SUBDIRS_ATTR)
            {
                if ((leftPanel->ValidFileData & VALID_DATA_ATTRIBUTES) &&
                    (rightPanel->ValidFileData & VALID_DATA_ATTRIBUTES))
                {
                    if ((leftDir->Attr & DISPLAYED_ATTRIBUTES) != (rightDir->Attr & DISPLAYED_ATTRIBUTES))
                    {
                        *different = TRUE;
                        return TRUE;
                    }
                }
            }

            // directories are not compared by time
        }

        if (timeDiffWithDSTShiftExists) // we found no other difference, so we report an unignored DST time shift including a warning
        {
            (*foundDSTShifts)++;
            *different = TRUE;
            return TRUE;
        }

        // compare files by content
        if (flags & COMPARE_DIRECTORIES_BYCONTENT)
        {
            if (!leftPanel->Is(ptDisk) || !rightPanel->Is(ptDisk))
            {
                TRACE_E("not implemented");
                *canceled = FALSE;
                return FALSE;
            }

            for (i = 0; i < leftFiles.Count; i++)
            {
                leftFile = &leftFiles[i];
                rightFile = &rightFiles[i];

                if (leftFile->Size == rightFile->Size)
                {
                    if (leftFile->Size != CQuadWord(0, 0))
                    {
                        if (!getTotal)
                        {
                            const auto leftPath = CompareDiskPath(leftPanel, CompareChildPath(leftPanel, leftSubDir, *leftFile));
                            const auto rightPath = CompareDiskPath(rightPanel, CompareChildPath(rightPanel, rightSubDir, *rightFile));
                            if (!CompareFilesByContent(hWindow, progressDlg, leftSide, leftPath, *leftFile,
                                                       rightSide, rightPath, *rightFile, different, canceled))
                            {
                                return FALSE;
                            }

                            // found two different files, stop
                            if (*different)
                                return TRUE;
                        }
                        else
                            *total += leftFile->Size + rightFile->Size;
                    }
                }
                else
                {
                    // files have different length, they differ in content
                    *different = TRUE;
                    return TRUE;
                }
            }
        }

        // no difference found
        // we no longer need the files, so we can free the memory
        // leftFiles.DestroyMembers();  -- Petr: directly destroy the local object, frees more
        // rightFiles.DestroyMembers(); -- Petr: directly destroy the local object, frees more
    }

    // Keep comparing accessible siblings if a nested comparison could not finish.
    bool incomplete = false;
    int i;
    for (i = 0; i < leftDirs.Count; i++)
    {
        const auto newLeftSubDir = CompareChildPath(leftPanel, leftSubDir, leftDirs[i]);
        const auto newRightSubDir = CompareChildPath(rightPanel, rightSubDir, rightDirs[i]);
        int foundDSTShiftsInSubDir = 0;
        if (!CompareDirsAux(hWindow, progressDlg,
                            leftPanel, leftSide, newLeftSubDir, leftFAT,
                            rightPanel, rightSide, newRightSubDir, rightFAT,
                            flags, different, canceled, getTotal,
                            total, &foundDSTShiftsInSubDir))
        {
            if (*canceled)
                return FALSE;
            incomplete = true;
        }
        else
        {
            if (*different)
            {
                *foundDSTShifts += foundDSTShiftsInSubDir;
                return TRUE; // found a difference in a subdirectory, stop
            }
            foundDSTShiftsInThisDir += foundDSTShiftsInSubDir;
        }
    }

    // no difference found
    *different = FALSE;
    *foundDSTShifts += foundDSTShiftsInThisDir;
    return !incomplete;
}

// create a shallow copy and set Selected to FALSE for all items
CFilesArray* GetShallowCopy(CFilesArray* items)
{
    CFilesArray* ret = new CFilesArray();
    if (ret != NULL)
    {
        ret->Add(items->GetData(), items->Count);
        if (ret->IsGood())
        {
            int i;
            for (i = 0; i < items->Count; i++)
                ret->At(i).Selected = 0;
            ret->SetDeleteData(FALSE); // shallow copy of data only, prevent destruction
        }
        else
        {
            TRACE_E(LOW_MEMORY);
            delete ret;
            ret = NULL;
        }
    }
    else
        TRACE_E(LOW_MEMORY);
    return ret;
}

void SkipIgnoredNames(BOOL ignoreNames, CMaskGroup* ignoreNamesMasks, BOOL dirs, int* l, CFileData** leftFile,
                      CFilesArray* left, int* r, CFileData** rightFile, CFilesArray* right)
{
    if (ignoreNames)
    {
        if (*l < left->Count)
        {
            while (((*leftFile)->UseWideName() ? ignoreNamesMasks->AgreeMasksW((*leftFile)->NameW, NULL) : ignoreNamesMasks->AgreeMasks((*leftFile)->Name, dirs ? NULL : (*leftFile)->Ext)))
            { // skip ignored names in the left panel
                if (++(*l) < left->Count)
                    *leftFile = &left->At(*l);
                else
                    break;
            }
        }
        if (*r < right->Count)
        {
            while (((*rightFile)->UseWideName() ? ignoreNamesMasks->AgreeMasksW((*rightFile)->NameW, NULL) : ignoreNamesMasks->AgreeMasks((*rightFile)->Name, dirs ? NULL : (*rightFile)->Ext)))
            { // skip ignored names in the right panel
                if (++(*r) < right->Count)
                    *rightFile = &right->At(*r);
                else
                    break;
            }
        }
    }
}

void CMainWindow::CompareDirectories(DWORD flags)
{
    CALL_STACK_MESSAGE2("CMainWindow::CompareDirectories(%u)", flags);
    if (flags & COMPARE_DIRECTORIES_BYCONTENT)
    {
        if (!LeftPanel->Is(ptDisk) || !RightPanel->Is(ptDisk))
        {
            TRACE_E("CMainWindow::CompareDirectories: Comparing by content is supported on ptDisk only.");
            return;
        }
    }
    if (flags & COMPARE_DIRECTORIES_SUBDIRS)
    {
        if (LeftPanel->Is(ptPluginFS) || RightPanel->Is(ptPluginFS))
        {
            TRACE_E("CMainWindow::CompareDirectories: Comparing including subdirectories is not supported on ptPluginFS.");
            return;
        }
    }
    BOOL ignFileNames = (flags & COMPARE_DIRECTORIES_IGNFILENAMES) != 0;
    if (ignFileNames)
    {
        int errPos;
        if (!Configuration.CompareIgnoreFilesMasks.PrepareMasks(errPos))
        {
            TRACE_E("CMainWindow::CompareDirectories: CompareIgnoreFilesMasks is invalid."); // likely only happens when configuration is manually edited in the registry
            return;
        }
    }
    BOOL ignDirNames = (flags & COMPARE_DIRECTORIES_IGNDIRNAMES) != 0;
    if (ignDirNames)
    {
        int errPos;
        if (!Configuration.CompareIgnoreDirsMasks.PrepareMasks(errPos))
        {
            TRACE_E("CMainWindow::CompareDirectories: CompareIgnoreDirsMasks is invalid."); // likely only happens when configuration is manually edited in the registry
            return;
        }
    }

    BeginStopRefresh(); // the snooper takes a break

    // lower the thread priority to "normal" (so the operation doesn't overburden the machine)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    //--- open the progress dialog
    BOOL displayDialogBox = (flags & COMPARE_DIRECTORIES_BYCONTENT) != 0 || // for quick actions there's no point in showing the dialog
                            (flags & COMPARE_DIRECTORIES_SUBDIRS) != 0;
    BOOL displayProgressBar = (flags & COMPARE_DIRECTORIES_BYCONTENT) != 0;
    CCmpDirProgressDialog progressDlg(HWindow, displayProgressBar, &TaskBarList3);

    HWND hFocusedWnd = NULL;
    if (displayDialogBox)
    {
        // set texts in the progress dialog
        char message[2 * MAX_PATH];
        LeftPanel->GetGeneralPath(message, 2 * MAX_PATH);
        progressDlg.SetSource(message);
        RightPanel->GetGeneralPath(message, 2 * MAX_PATH);
        progressDlg.SetTarget(message);

        hFocusedWnd = GetFocus();
        EnableWindow(HWindow, FALSE);
        progressDlg.Create();
    }

    BOOL identical = TRUE;
    BOOL canceled = FALSE;  // user canceled the operation
    int foundDSTShifts = 0; // how many times file timestamps differed by exactly one or two hours, with that being the only difference in the user-selected criteria

    //--- shallow copy of Files and Dirs arrays; (we must work on a copy of data because while the progress dialog is shown,
    //    a panel redraw would reveal that we changed panel sorting or moved the selection)

    CFilesArray* leftFiles = GetShallowCopy(LeftPanel->Files);
    CFilesArray* leftDirs = GetShallowCopy(LeftPanel->Dirs);
    CFilesArray* rightFiles = GetShallowCopy(RightPanel->Files);
    CFilesArray* rightDirs = GetShallowCopy(RightPanel->Dirs);

    if (leftFiles != NULL && leftDirs != NULL && rightFiles != NULL && rightDirs != NULL)
    {
        // shallow copies were created successfully
        CCompareDirectorySide leftSide(LeftPanel, displayProgressBar != FALSE);
        CCompareDirectorySide rightSide(RightPanel, displayProgressBar != FALSE);

        //--- determine FAT file system because of lastWrite accuracy
        BOOL leftFAT = FALSE;
        BOOL rightFAT = FALSE;
        if (LeftPanel->Is(ptDisk))
        {
            char fileSystem[20];
            MyGetVolumeInformation(LeftPanel->GetPath(), NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, fileSystem, 20);
            leftFAT = StrNICmp(fileSystem, "FAT", 3) == 0; // FAT and FAT32 use DOS time (precision only 2 seconds)
        }
        if (RightPanel->Is(ptDisk))
        {
            char fileSystem[20];
            MyGetVolumeInformation(RightPanel->GetPath(), NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, fileSystem, 20);
            rightFAT = StrNICmp(fileSystem, "FAT", 3) == 0; // FAT and FAT32 use DOS time (precision only 2 seconds)
        }
        // stop loading icons to avoid competition with the comparison
        if (LeftPanel->UseSystemIcons || LeftPanel->UseThumbnails)
            LeftPanel->SleepIconCacheThread();
        if (RightPanel->UseSystemIcons || RightPanel->UseThumbnails)
            RightPanel->SleepIconCacheThread();
        //--- sort by name
        if (LeftPanel->SortType != stName || LeftPanel->ReverseSort)
            SortFilesAndDirectories(leftFiles, leftDirs, stName, FALSE, Configuration.SortDirsByName);
        if (RightPanel->SortType != stName || RightPanel->ReverseSort)
            SortFilesAndDirectories(rightFiles, rightDirs, stName, FALSE, Configuration.SortDirsByName);
        //--- comparison + marking
        BOOL getTotal;
        if (flags & COMPARE_DIRECTORIES_BYCONTENT)
            getTotal = TRUE; // TRUE: we don't mark differences, instead accumulate the size of files to compare into the 'total' variable
        else
            getTotal = FALSE;  // FALSE: we mark differences, compare files by content
        CQuadWord total(0, 0); // total number of bytes that may need to be compared in the worst case

        // the 'dirSubTotal' array contains sizes of individual subdirectories (files contained in them)
        // the array is filled in the first pass (getTotal == TRUE)
        // it is used in the second pass (getTotal == FALSE): if a subdirectory differs we know how much to skip on the total progress
        TDirectArray<CQuadWord> dirSubTotal(max(1, min(leftDirs->Count, rightDirs->Count)), 1);
        int subTotalIndex; // index into the dirSubTotal array

    ONCE_MORE:
        // first the files from the left and right directory
        CFilesArray *left = leftFiles, *right = rightFiles;

        CFileData *leftFile, *rightFile;
        int l = 0, r = 0, compRes;

        if (l < left->Count)
            leftFile = &left->At(l);
        if (r < right->Count)
            rightFile = &right->At(r);
        while (l < left->Count || r < right->Count)
        {
            SkipIgnoredNames(ignFileNames, &Configuration.CompareIgnoreFilesMasks, FALSE,
                             &l, &leftFile, left, &r, &rightFile, right);
            if (l < left->Count)
            {
                if (r < right->Count) // both panels contain files
                {
                    compRes = CmpNameExtIgnCase(*leftFile, *rightFile);
                    if (compRes == -1) // left < right
                    {
                        if (!getTotal)
                        {
                            leftFile->Selected = 1;
                            identical = FALSE;
                        }
                        if (++l < left->Count)
                            leftFile = &left->At(l);
                    }
                    else
                    {
                        if (compRes == 1) // left > right
                        {
                            if (!getTotal)
                            {
                                rightFile->Selected = 1;
                                identical = FALSE;
                            }
                            if (++r < right->Count)
                                rightFile = &right->At(r);
                        }
                        else // left == right
                        {
                            BOOL selectLeft = FALSE;
                            BOOL selectRight = FALSE;
                            BOOL leftIsNewer = FALSE;
                            BOOL rightIsNewer = FALSE;
                            BOOL leftIsNewerNoDSTShiftIgn = FALSE;
                            BOOL rightIsNewerNoDSTShiftIgn = FALSE;
                            int isDSTShift = 0; // 1 = times of the currently compared file pair differ by exactly one or two hours, 0 = they don't (or times are not compared at all)

                            // By Size
                            if (flags & COMPARE_DIRECTORIES_BYSIZE)
                            {
                                compRes = CompareFilesBySize(LeftPanel, leftFile, RightPanel, rightFile);
                                if (compRes == 1)
                                    selectLeft = TRUE;
                                if (compRes == -1)
                                    selectRight = TRUE;
                            }

                            // By Time
                            if (flags & COMPARE_DIRECTORIES_BYTIME)
                            {
                                int compResNoDSTShiftIgn;
                                compRes = CompareFilesByTime(LeftPanel, leftFile, leftFAT, RightPanel,
                                                             rightFile, rightFAT, &isDSTShift, &compResNoDSTShiftIgn);
                                if (compRes == 1)
                                    leftIsNewer = TRUE;
                                if (compRes == -1)
                                    rightIsNewer = TRUE;
                                if (compResNoDSTShiftIgn == 1)
                                    leftIsNewerNoDSTShiftIgn = TRUE;
                                if (compResNoDSTShiftIgn == -1)
                                    rightIsNewerNoDSTShiftIgn = TRUE;
                            }

                            // By Attributes
                            if (flags & COMPARE_DIRECTORIES_BYATTR)
                            {
                                if ((LeftPanel->ValidFileData & VALID_DATA_ATTRIBUTES) &&
                                    (RightPanel->ValidFileData & VALID_DATA_ATTRIBUTES))
                                {
                                    if ((leftFile->Attr & DISPLAYED_ATTRIBUTES) != (rightFile->Attr & DISPLAYED_ATTRIBUTES))
                                    {
                                        selectLeft = TRUE;
                                        selectRight = TRUE;
                                    }
                                }
                            }

                            // By Content
                            if (flags & COMPARE_DIRECTORIES_BYCONTENT)
                            {
                                if (leftFile->Size == rightFile->Size) // size test is intentionally not covered by the following optimization so both files are marked and unnecessary DST warnings don't pop up
                                {
                                    if (!selectLeft && !leftIsNewer || !selectRight && !rightIsNewer) // if both files are already flagged, there is no point in comparing them by content
                                    {
                                        // if both files are zero length, they must be identical in content
                                        if (leftFile->Size != CQuadWord(0, 0))
                                        {
                                            if (!getTotal)
                                            {
                                                BOOL different;
                                                BOOL ret = CompareFilesByContent(progressDlg.HWindow, &progressDlg,
                                                                                 leftSide, LeftPanel->GetItemFullPathW(*leftFile), *leftFile,
                                                                                 rightSide, RightPanel->GetItemFullPathW(*rightFile), *rightFile,
                                                                                 &different, &canceled);
                                                if (ret)
                                                {
                                                    if (different)
                                                    {
                                                        selectLeft = TRUE;
                                                        selectRight = TRUE;
                                                    }
                                                }
                                                else
                                                {
                                                    if (canceled)
                                                        goto ABORT_COMPARE;
                                                    selectLeft = TRUE; // on read error mark the pair as if it had different content (it might)
                                                    selectRight = TRUE;
                                                }
                                            }
                                            else
                                                total += leftFile->Size + rightFile->Size;
                                        }
                                    }
                                }
                                else
                                {
                                    // files have different length, so they must differ in content
                                    selectLeft = TRUE;
                                    selectRight = TRUE;
                                }
                            }

                            if (!getTotal)
                            {
                                if (leftIsNewerNoDSTShiftIgn && !selectLeft || rightIsNewerNoDSTShiftIgn && !selectRight)
                                    foundDSTShifts += isDSTShift; // count only time differences of files that are not already marked for another reason (e.g., due to a difference by another criterion) -- motivation: if we don't need to show a complex DST warning, don't show it

                                if (selectLeft || leftIsNewer)
                                {
                                    leftFile->Selected = 1;
                                    identical = FALSE;
                                }
                                if (selectRight || rightIsNewer)
                                {
                                    rightFile->Selected = 1;
                                    identical = FALSE;
                                }
                            }
                            if (++l < left->Count)
                                leftFile = &left->At(l);
                            if (++r < right->Count)
                                rightFile = &right->At(r);
                        }
                    }
                }
                else // left only
                {
                    if (!getTotal)
                    {
                        leftFile->Selected = 1;
                        identical = FALSE;
                    }
                    if (++l < left->Count)
                        leftFile = &left->At(l);
                }
            }
            else // right only
            {
                if (r < right->Count)
                {
                    if (!getTotal)
                    {
                        rightFile->Selected = 1;
                        identical = FALSE;
                    }
                    if (++r < right->Count)
                        rightFile = &right->At(r);
                }
            }
        }

        // Sal2.0 and TC compare without directories in such a way that they even ignore their names
        // people kept pointing this out to us, so we'll behave the same (with
        // COMPARE_DIRECTORIES_ONEPANELDIRS disabled)
        if ((flags & COMPARE_DIRECTORIES_SUBDIRS) || (flags & COMPARE_DIRECTORIES_ONEPANELDIRS))
        {
            // now compare subdirectories
            subTotalIndex = 0;

            left = leftDirs;
            right = rightDirs;

            CFileData *leftDir, *rightDir;
            l = 0;
            r = 0;

            if (left->Count > 0 && strcmp(left->At(0).Name, "..") == 0)
                l++;
            if (right->Count > 0 && strcmp(right->At(0).Name, "..") == 0)
                r++;

            if (l < left->Count)
                leftDir = &left->At(l);
            if (r < right->Count)
                rightDir = &right->At(r);
            while (l < left->Count || r < right->Count)
            {
                SkipIgnoredNames(ignDirNames, &Configuration.CompareIgnoreDirsMasks, TRUE,
                                 &l, &leftDir, left, &r, &rightDir, right);
                if (l < left->Count)
                {
                    if (r < right->Count) // both panels contain directories
                    {
                        compRes = CmpNameExtIgnCase(*leftDir, *rightDir);
                        if (compRes == -1) // left < right
                        {
                            leftDir->Selected = 1;
                            identical = FALSE;
                            if (++l < left->Count)
                                leftDir = &left->At(l);
                        }
                        else
                        {
                            if (compRes == 1) // left > right
                            {
                                rightDir->Selected = 1;
                                identical = FALSE;
                                if (++r < right->Count)
                                    rightDir = &right->At(r);
                            }
                            else // left == right
                            {
                                BOOL select = FALSE;

                                // By Attributes
                                if (flags & COMPARE_DIRECTORIES_SUBDIRS_ATTR)
                                {
                                    if ((LeftPanel->ValidFileData & VALID_DATA_ATTRIBUTES) &&
                                        (RightPanel->ValidFileData & VALID_DATA_ATTRIBUTES))
                                    {
                                        if ((leftDir->Attr & DISPLAYED_ATTRIBUTES) != (rightDir->Attr & DISPLAYED_ATTRIBUTES))
                                        {
                                            select = TRUE;
                                        }
                                    }
                                }

                                // same names and attributes -> we'll examine the subdirectory content
                                // if the directory was selected in the previous round we won't enumerate it again (getTotal)
                                if ((flags & COMPARE_DIRECTORIES_SUBDIRS) && (!select) && leftDir->Selected == 0)
                                {
                                    // we insert the subpath into the left/rightSubDir variables, which is
                                    // in the case of ptDisk relative to the panel path and in the case
                                    // of ptZIPArchive relative to the path to the archive
                                    const auto leftSubDir = CompareChildPath(LeftPanel, LeftPanel->Is(ptZIPArchive) ? LeftPanel->GetZIPPath() : "", *leftDir);
                                    const auto rightSubDir = CompareChildPath(RightPanel, RightPanel->Is(ptZIPArchive) ? RightPanel->GetZIPPath() : "", *rightDir);

                                    BOOL different;
                                    CQuadWord lastTotal; // if the subdirectory differs, skip its size using 'lastTotal' + 'dirSubTotal'
                                    if (!getTotal)
                                        progressDlg.GetActualTotalSize(lastTotal);
                                    CQuadWord subTotal(0, 0);
                                    int foundDSTShiftsInSubDir = 0;
                                    BOOL ret = CompareDirsAux(progressDlg.HWindow, &progressDlg,
                                                              LeftPanel, leftSide, leftSubDir, leftFAT,
                                                              RightPanel, rightSide, rightSubDir, rightFAT,
                                                              flags, &different, &canceled,
                                                              getTotal, &subTotal, &foundDSTShiftsInSubDir);
                                    if (ret)
                                    {
                                        if (different)
                                        {
                                            foundDSTShifts += foundDSTShiftsInSubDir;
                                            select = TRUE; // set select even when getTotal is TRUE; optimization, skips the entire tree in the second pass
                                            if (!getTotal && (flags & COMPARE_DIRECTORIES_BYCONTENT))
                                            {
                                                if (dirSubTotal.IsGood() && dirSubTotal.Count > subTotalIndex)
                                                    progressDlg.SetActualTotalSize(lastTotal + dirSubTotal[subTotalIndex]);
                                                else
                                                    TRACE_E("Error total size adjusting, please report it to Honza.");
                                            }
                                        }
                                        else
                                        {
                                            // add to total only if we compare the entire tree by content
                                            if (getTotal)
                                            {
                                                if (dirSubTotal.IsGood())
                                                    dirSubTotal.Add(subTotal);
                                                total += subTotal;
                                            }
                                            else
                                                foundDSTShifts += foundDSTShiftsInSubDir;
                                        }
                                        subTotalIndex++;
                                    }
                                    else
                                    {
                                        if (canceled)
                                            goto ABORT_COMPARE;
                                        else // if it is not 'canceled', an error message was shown and the user wants to continue
                                        {
                                            if (!getTotal)
                                                select = TRUE;
                                        }
                                    }
                                }

                                if (select) // note: used even when getTotal == TRUE (optimization)
                                {
                                    leftDir->Selected = 1;
                                    rightDir->Selected = 1;
                                    identical = FALSE;
                                }

                                if (++l < left->Count)
                                    leftDir = &left->At(l);
                                if (++r < right->Count)
                                    rightDir = &right->At(r);
                            }
                        }
                    }
                    else // left only
                    {
                        leftDir->Selected = 1;
                        identical = FALSE;
                        if (++l < left->Count)
                            leftDir = &left->At(l);
                    }
                }
                else // right only
                {
                    if (r < right->Count)
                    {
                        rightDir->Selected = 1;
                        identical = FALSE;
                        if (++r < right->Count)
                            rightDir = &right->At(r);
                    }
                }
            }
        }

        if (getTotal)
        {
            progressDlg.SetTotalSize(total);
            getTotal = FALSE;
            goto ONCE_MORE;
        }

    ABORT_COMPARE:

        //--- sort according to settings
        if (LeftPanel->SortType != stName || LeftPanel->ReverseSort)
            SortFilesAndDirectories(leftFiles, leftDirs, LeftPanel->SortType, LeftPanel->ReverseSort, Configuration.SortDirsByName);
        if (RightPanel->SortType != stName || RightPanel->ReverseSort)
            SortFilesAndDirectories(rightFiles, rightDirs, RightPanel->SortType, RightPanel->ReverseSort, Configuration.SortDirsByName);

        //--- close the progress dialog
        if (displayDialogBox)
        {
            EnableWindow(HWindow, TRUE);
            HWND actWnd = GetForegroundWindow();
            BOOL activate = actWnd == progressDlg.HWindow || actWnd == HWindow;
            // Hand activation back before destroying the foreground window.
            if (actWnd == progressDlg.HWindow)
                SetForegroundWindow(HWindow);
            DestroyWindow(progressDlg.HWindow);
            if (activate && hFocusedWnd != NULL)
                SetFocus(hFocusedWnd);
        }

        // previously the selection was cleared at the start of the compare and continuously set during it
        // if an icon redraw request arrived from the icon reader during that time,
        // the dirty bit for the given file/directory was cleared and as a result, it was not redrawn at the end
        // (it remained unselected even though it should have been marked)

        // now we clear the selection at the end
        // and between changing the selection state and calling
        // RepaintListBox no messages are distributed, so a request to redraw the icon cannot be delivered

        // Keep the original selections if the user cancels a protected comparison.
        if (!canceled || (!leftSide.HasProtectedWork() && !rightSide.HasProtectedWork()))
        {
            LeftPanel->SetSel(FALSE, -1);
            RightPanel->SetSel(FALSE, -1);
        }

        // if not canceled, transfer the selection
        if (!canceled)
        {
            int i;
            for (i = 0; i < leftDirs->Count; i++)
                if (leftDirs->At(i).Selected != 0)
                    LeftPanel->SetSel(TRUE, i);
            for (i = 0; i < leftFiles->Count; i++)
                if (leftFiles->At(i).Selected != 0)
                    LeftPanel->SetSel(TRUE, leftDirs->Count + i);
            for (i = 0; i < rightDirs->Count; i++)
                if (rightDirs->At(i).Selected != 0)
                    RightPanel->SetSel(TRUE, i);
            for (i = 0; i < rightFiles->Count; i++)
                if (rightFiles->At(i).Selected != 0)
                    RightPanel->SetSel(TRUE, rightDirs->Count + i);
        }

    } // if (leftFiles != NULL && leftDirs != NULL && rightFiles != NULL && rightDirs != NULL)
    else
    {
        // out of memory
        canceled = TRUE;
    }

    //--- destroy helper arrays
    if (leftFiles != NULL)
        delete leftFiles;
    if (leftDirs != NULL)
        delete leftDirs;
    if (rightFiles != NULL)
        delete rightFiles;
    if (rightDirs != NULL)
        delete rightDirs;

    //--- mark the appropriate files
    LeftPanel->RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
    RightPanel->RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
    PostMessage(LeftPanel->HWindow, WM_USER_SELCHANGED, 0, 0);  // sel-change notify
    PostMessage(RightPanel->HWindow, WM_USER_SELCHANGED, 0, 0); // sel-change notify

    //--- allow icon loading again
    if (LeftPanel->UseSystemIcons || LeftPanel->UseThumbnails)
        LeftPanel->WakeupIconCacheThread();
    if (RightPanel->UseSystemIcons || RightPanel->UseThumbnails)
        RightPanel->WakeupIconCacheThread();

    //--- display a confirmation if files differing by exactly one or two hours appear
    //    either warn that not all files have the same time or notify that the detected difference can be ignored
    BOOL resultAlreadyShown = FALSE;
    if (!canceled && foundDSTShifts > 0 &&
        (Configuration.IgnoreDSTShifts ? Configuration.CnfrmDSTShiftsIgnored : Configuration.CnfrmDSTShiftsOccured))
    {
        char buf[550];
        char buf2[400];
        CQuadWord qwShifts[2];
        qwShifts[0].Set(foundDSTShifts, 0);
        qwShifts[1].Set(foundDSTShifts, 0);
        ExpandPluralString(buf2, 400,
                           LoadStr(Configuration.IgnoreDSTShifts ? (!canceled && identical ? IDS_CMPDIRS_IGNDSTDIFFSEXACT : IDS_CMPDIRS_IGNDSTDIFFS) : IDS_CMPDIRS_FOUNDDSTDIFFS),
                           2, qwShifts);
        buf[0] = 0;
        if (!canceled && identical)
        {
            resultAlreadyShown = TRUE;
            int messageID = (flags & COMPARE_DIRECTORIES_BYCONTENT) ? IDS_COMPAREDIR_ARE_IDENTICAL : IDS_COMPAREDIR_SEEMS_IDENTICAL;
            _snprintf_s(buf, _TRUNCATE, "%s\n\n%s ", LoadStr(messageID), LoadStr(IDS_CMPDIRS_NOTE));
        }
        _snprintf_s(buf + strlen(buf), _countof(buf) - strlen(buf), _TRUNCATE, buf2, foundDSTShifts);

        MSGBOXEX_PARAMS params;
        memset(&params, 0, sizeof(params));
        params.HParent = HWindow;
        params.Flags = MSGBOXEX_OK | MSGBOXEX_ICONINFORMATION | MSGBOXEX_HINT;
        params.Caption = LoadStr(IDS_COMPAREDIRSTITLE);
        params.Text = buf;
        params.CheckBoxText = LoadStr(!canceled && identical ? IDS_CMPDIRS_DONTSHOWNOTEAG : IDS_DONTSHOWAGAIN);
        int dontShow = Configuration.IgnoreDSTShifts ? !Configuration.CnfrmDSTShiftsIgnored : !Configuration.CnfrmDSTShiftsOccured;
        params.CheckBoxValue = &dontShow;
        SalMessageBoxEx(&params);

        if (Configuration.IgnoreDSTShifts)
            Configuration.CnfrmDSTShiftsIgnored = !dontShow;
        else
            Configuration.CnfrmDSTShiftsOccured = !dontShow;
    }

    //--- display the verdict
    if (!resultAlreadyShown && !canceled && identical)
    {
        int messageID = (flags & COMPARE_DIRECTORIES_BYCONTENT) ? IDS_COMPAREDIR_ARE_IDENTICAL : IDS_COMPAREDIR_SEEMS_IDENTICAL;
        SalMessageBox(HWindow, LoadStr(messageID), LoadStr(IDS_COMPAREDIRSTITLE), MB_OK | MB_ICONINFORMATION);
    }

    // raise the thread priority again, the operation is finished
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    EndStopRefresh(); // now the snooper starts again
}

BOOL CMainWindow::GetViewersAssoc(int wantedViewerType, CDynString* strViewerMasks)
{
    BOOL ok = TRUE;
    EnterViewerMasksCS();
    BOOL first = TRUE;
    CViewerMasks* masks = ViewerMasks;
    int i;
    for (i = 0; i < masks->Count; i++)
    {
        CViewerMasksItem* item = masks->At(i);
        if (!item->OldType && item->ViewerType == wantedViewerType)
        {
            const char* masksStr = item->Masks->GetMasksString();
            int len = (int)strlen(masksStr);
            if (len > 0 && masksStr[len - 1] == ';')
                len--;
            if (strchr(masksStr, '|') != NULL)
            {
                TRACE_E("CMainWindow::GetViewersAssoc(): unexpected situation: masks contains forbidden char '|'!");
                len = 0;
            }
            if (len > 0)
            {
                if (!first)
                    ok &= strViewerMasks->Append(";", 1);
                else
                    first = FALSE;
                ok &= strViewerMasks->Append(masksStr, len);
            }
        }
    }
    LeaveViewerMasksCS();
    return ok;
}

//
// ****************************************************************************
// CDynString
//

BOOL CDynString::Append(const char* str, int len)
{
    if (len == -1)
        len = (int)strlen(str);
    if (Length + len >= Allocated)
    {
        int size = Length + len + 1 + 256; // +256 characters as reserve so we don't allocate so often
        char* newBuf = (char*)realloc(Buffer, size);
        if (newBuf != NULL)
        {
            Buffer = newBuf;
            Allocated = size;
        }
        else // out of memory, tough luck...
        {
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    memmove(Buffer + Length, str, len);
    Length += len;
    Buffer[Length] = 0;
    return TRUE;
}
