// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Cancel the protected Find broker handshake on Stop. | Release the read-only pipe while preserving the transfer cancellation contract. | | Codex |
// | 2026-11-09 | Bind queued Find notifications to the originating search. | Ignore old refresh and completion messages after a restart. | | Codex |
// | 2026-11-09 | Substitute Find error details as plain text. | Preserve localization without interpreting resource text as a printf format. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Stop Find work for disconnected protected scopes. | Keep scanning and comparing the other scopes without reporting artificial access errors. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Match protected duplicate predicates during hashing. | Evaluate text, tags and digest on one broker-opened file. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Bind duplicate hashing to the revision used by protected filters. | Reject changed files between the two passes. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Match protected text and tags through one file open. | Avoid combining results from separate file revisions. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Verify every content duplicate at comparison time. | Keep stale or unreadable entries out of duplicate groups, including zero-length files. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Allow protected text and tag checks when refining Find results. | Reuse exact-file consent and preserve the original wide paths and refine mode. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Queue unreadable files discovered by the ordinary Find traversal. | Require exact-file consent for denied text or keyword reads. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Request exact-file access for protected Find candidates. | Share the verified content reader across text, keywords and duplicate comparison. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Queue multiple independently authorized Find roots. | Serialize prompts and route duplicate reads to the original scope. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Match tags in the authorized Find subtree. | Read keywords through the host and require a verified broker close. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Include approved protected files in duplicate groups. | Use the same ordering and digest progress for both sources. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Search protected file contents with the existing matcher. | Consume bounded blocks after the ordinary traversal while retaining search options. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Resume metadata Find in one verified protected subtree. | Keep ordinary search running during consent and use one bounded read-only session. | | GPT-6.1 Sol (Codex) |
// | 2026-11-07 | Post coalesced Find result refreshes asynchronously. | Keep the search worker from blocking the GUI thread during large searches. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | OpenAI GPT-5.6 Luna Pro |
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "cfgdlg.h"
#include "find.h"
#include "findtagstream.h"
#include "md5.h"
#include "common/widepath.h"
#include "uacbroker/f7pilot.h"
#include <array>
#include <mutex>
#include <optional>

char* FindNamedHistory[FIND_NAMED_HISTORY_SIZE];
char* FindLookInHistory[FIND_LOOKIN_HISTORY_SIZE];
char* FindGrepHistory[FIND_GREP_HISTORY_SIZE];

CFindOptions FindOptions;
CFindIgnore FindIgnore;
CFindDialogQueue FindDialogQueue("Find Dialogs");

HANDLE FindDialogContinue = NULL;

HACCEL FindDialogAccelTable = NULL;

const char* FINDOPTIONSITEM_ITEMNAME_REG = "ItemName";
const char* FINDOPTIONSITEM_SUBDIRS_REG = "SubDirectories";
const char* FINDOPTIONSITEM_WHOLEWORDS_REG = "WholeWords";
const char* FINDOPTIONSITEM_CASESENSITIVE_REG = "CaseSensitive";
const char* FINDOPTIONSITEM_HEXMODE_REG = "HexMode";
const char* FINDOPTIONSITEM_REGULAR_REG = "RegularExpresions";
const char* FINDOPTIONSITEM_AUTOLOAD_REG = "AutoLoad";
const char* FINDOPTIONSITEM_NAMED_REG = "Named";
const char* FINDOPTIONSITEM_LOOKIN_REG = "LookIn";
const char* FINDOPTIONSITEM_GREP_REG = "Grep";

const char* FINDIGNOREITEM_PATH_REG = "Path";
const char* FINDIGNOREITEM_ENABLED_REG = "Enabled";

// following variable was used up to Altap Salamander 2.5,
// where we switched to CFilterCriteria with its Save/Load
const char* OLD_FINDOPTIONSITEM_EXCLUDEMASK_REG = "ExcludeMask";

static std::string FormatFindError(int messageId, DWORD error)
{
    std::string message = LoadStr(messageId);
    const size_t placeholder = message.find("%s");
    if (placeholder != std::string::npos)
        message.replace(placeholder, 2, GetErrorText(error));
    return message;
}

//*********************************************************************************
//
// InitializeFind, ReleaseFind
//

BOOL InitializeFind()
{
    int i;
    for (i = 0; i < FIND_NAMED_HISTORY_SIZE; i++)
        FindNamedHistory[i] = NULL;
    for (i = 0; i < FIND_LOOKIN_HISTORY_SIZE; i++)
        FindLookInHistory[i] = NULL;
    for (i = 0; i < FIND_GREP_HISTORY_SIZE; i++)
        FindGrepHistory[i] = NULL;

    FindDialogContinue = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (FindDialogContinue == NULL)
    {
        TRACE_E("Unable to create FindDialogContinue event.");
        return FALSE;
    }

    FindDialogAccelTable = HANDLES(LoadAccelerators(HInstance, MAKEINTRESOURCE(IDA_FINDDIALOGACCELS)));
    if (FindDialogAccelTable == NULL)
    {
        TRACE_E("Unable to load accelerators for Find dialog.");
        return FALSE;
    }
    return TRUE;
}

void ClearFindHistory(BOOL dataOnly)
{
    int i;
    for (i = 0; i < FIND_NAMED_HISTORY_SIZE; i++)
    {
        if (FindNamedHistory[i] != NULL)
        {
            free(FindNamedHistory[i]);
            FindNamedHistory[i] = NULL;
        }
    }

    for (i = 0; i < FIND_LOOKIN_HISTORY_SIZE; i++)
    {
        if (FindLookInHistory[i] != NULL)
        {
            free(FindLookInHistory[i]);
            FindLookInHistory[i] = NULL;
        }
    }

    for (i = 0; i < FIND_GREP_HISTORY_SIZE; i++)
    {
        if (FindGrepHistory[i] != NULL)
        {
            free(FindGrepHistory[i]);
            FindGrepHistory[i] = NULL;
        }
    }

    // we should also clear the comboboxes of open windows
    if (!dataOnly)
    {
        FindDialogQueue.BroadcastMessage(WM_USER_CLEARHISTORY, 0, 0);
    }
}

void ReleaseFind()
{
    ClearFindHistory(TRUE); // we only release data
    if (FindDialogContinue != NULL)
        HANDLES(CloseHandle(FindDialogContinue));
}

//*********************************************************************************
//
// CFindOptionsItem
//

CFindOptionsItem::CFindOptionsItem()
{
    // Internal
    ItemName[0] = 0;

    // Find dialog
    SubDirectories = TRUE;
    WholeWords = FALSE;
    CaseSensitive = FALSE;
    HexMode = FALSE;
    RegularExpresions = FALSE;

    AutoLoad = FALSE;

    NamedText[0] = 0;
    LookInText[0] = 0;
    GrepText[0] = 0;
}

CFindOptionsItem&
CFindOptionsItem::operator=(const CFindOptionsItem& s)
{
    // Internal
    lstrcpy(ItemName, s.ItemName);

    memmove(&Criteria, &s.Criteria, sizeof(Criteria));

    // Find dialog
    SubDirectories = s.SubDirectories;
    WholeWords = s.WholeWords;
    CaseSensitive = s.CaseSensitive;
    HexMode = s.HexMode;
    RegularExpresions = s.RegularExpresions;

    AutoLoad = s.AutoLoad;

    lstrcpy(NamedText, s.NamedText);
    lstrcpy(LookInText, s.LookInText);
    lstrcpy(GrepText, s.GrepText);

    return *this;
}

void CFindOptionsItem::BuildItemName()
{
    sprintf(ItemName, "\"%s\" %s \"%s\"",
            NamedText, LoadStr(IDS_FF_IN), LookInText);
}

BOOL CFindOptionsItem::Save(HKEY hKey)
{
    // optimize registry size by storing only non-default values;
    // before saving, we need to clear the key we’re going to save into
    CFindOptionsItem def;

    if (strcmp(ItemName, def.ItemName) != 0)
        SetValue(hKey, FINDOPTIONSITEM_ITEMNAME_REG, REG_SZ, ItemName, -1);
    if (SubDirectories != def.SubDirectories)
        SetValue(hKey, FINDOPTIONSITEM_SUBDIRS_REG, REG_DWORD, &SubDirectories, sizeof(DWORD));
    if (WholeWords != def.WholeWords)
        SetValue(hKey, FINDOPTIONSITEM_WHOLEWORDS_REG, REG_DWORD, &WholeWords, sizeof(DWORD));
    if (CaseSensitive != def.CaseSensitive)
        SetValue(hKey, FINDOPTIONSITEM_CASESENSITIVE_REG, REG_DWORD, &CaseSensitive, sizeof(DWORD));
    if (HexMode != def.HexMode)
        SetValue(hKey, FINDOPTIONSITEM_HEXMODE_REG, REG_DWORD, &HexMode, sizeof(DWORD));
    if (RegularExpresions != def.RegularExpresions)
        SetValue(hKey, FINDOPTIONSITEM_REGULAR_REG, REG_DWORD, &RegularExpresions, sizeof(DWORD));
    if (AutoLoad != def.AutoLoad)
        SetValue(hKey, FINDOPTIONSITEM_AUTOLOAD_REG, REG_DWORD, &AutoLoad, sizeof(DWORD));
    if (strcmp(NamedText, def.NamedText) != 0)
        SetValue(hKey, FINDOPTIONSITEM_NAMED_REG, REG_SZ, NamedText, -1);
    if (strcmp(LookInText, def.LookInText) != 0)
        SetValue(hKey, FINDOPTIONSITEM_LOOKIN_REG, REG_SZ, LookInText, -1);
    if (strcmp(GrepText, def.GrepText) != 0)
        SetValue(hKey, FINDOPTIONSITEM_GREP_REG, REG_SZ, GrepText, -1);

    // advanced options
    Criteria.Save(hKey);
    return TRUE;
}

BOOL CFindOptionsItem::Load(HKEY hKey, DWORD cfgVersion)
{
    GetValue(hKey, FINDOPTIONSITEM_ITEMNAME_REG, REG_SZ, ItemName, ITEMNAME_TEXT_LEN);
    GetValue(hKey, FINDOPTIONSITEM_SUBDIRS_REG, REG_DWORD, &SubDirectories, sizeof(DWORD));
    GetValue(hKey, FINDOPTIONSITEM_WHOLEWORDS_REG, REG_DWORD, &WholeWords, sizeof(DWORD));
    GetValue(hKey, FINDOPTIONSITEM_CASESENSITIVE_REG, REG_DWORD, &CaseSensitive, sizeof(DWORD));
    GetValue(hKey, FINDOPTIONSITEM_HEXMODE_REG, REG_DWORD, &HexMode, sizeof(DWORD));
    GetValue(hKey, FINDOPTIONSITEM_REGULAR_REG, REG_DWORD, &RegularExpresions, sizeof(DWORD));
    GetValue(hKey, FINDOPTIONSITEM_AUTOLOAD_REG, REG_DWORD, &AutoLoad, sizeof(DWORD));
    GetValue(hKey, FINDOPTIONSITEM_NAMED_REG, REG_SZ, NamedText, NAMED_TEXT_LEN);
    GetValue(hKey, FINDOPTIONSITEM_LOOKIN_REG, REG_SZ, LookInText, LOOKIN_TEXT_LEN);
    GetValue(hKey, FINDOPTIONSITEM_GREP_REG, REG_SZ, GrepText, GREP_TEXT_LEN);

    if (cfgVersion <= 13)
    {
        // conversion of old values

        // exclude mask
        BOOL excludeMask = FALSE;
        GetValue(hKey, OLD_FINDOPTIONSITEM_EXCLUDEMASK_REG, REG_DWORD, &excludeMask, sizeof(DWORD));
        if (excludeMask)
        {
            memmove(NamedText + 1, NamedText, NAMED_TEXT_LEN - 1);
            NamedText[0] = '|';
        }

        Criteria.LoadOld(hKey);
    }
    else
        Criteria.Load(hKey);

    return TRUE;
}

//*********************************************************************************
//
// CFindOptions
//

CFindOptions::CFindOptions()
    : Items(20, 10)
{
}

BOOL CFindOptions::Save(HKEY hKey)
{
    ClearKey(hKey);

    HKEY subKey;
    char buf[30];
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        itoa(i + 1, buf, 10);
        if (CreateKey(hKey, buf, subKey))
        {
            Items[i]->Save(subKey);
            CloseKey(subKey);
        }
        else
            break;
    }
    return TRUE;
}

BOOL CFindOptions::Load(HKEY hKey, DWORD cfgVersion)
{
    HKEY subKey;
    char buf[30];
    int i = 1;
    strcpy(buf, "1");
    Items.DestroyMembers();
    while (OpenKey(hKey, buf, subKey))
    {
        CFindOptionsItem* item = new CFindOptionsItem();
        if (item == NULL)
        {
            TRACE_E(LOW_MEMORY);
            break;
        }
        item->Load(subKey, cfgVersion);
        Items.Add(item);
        if (!Items.IsGood())
        {
            Items.ResetState();
            delete item;
            break;
        }
        itoa(++i, buf, 10);
        CloseKey(subKey);
    }

    return TRUE;
}

BOOL CFindOptions::Load(CFindOptions& source)
{
    CFindOptionsItem* item;
    Items.DestroyMembers();
    int i;
    for (i = 0; i < source.Items.Count; i++)
    {
        item = new CFindOptionsItem();
        if (item == NULL)
        {
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
        *item = *source.Items[i];
        Items.Add(item);
        if (!Items.IsGood())
        {
            delete item;
            Items.ResetState();
            return FALSE;
        }
    }
    return TRUE;
}

BOOL CFindOptions::Add(CFindOptionsItem* item)
{
    Items.Add(item);
    if (!Items.IsGood())
    {
        Items.ResetState();
        return FALSE;
    }
    return TRUE;
}

//*********************************************************************************
//
// CFindIgnoreItem
//

CFindIgnoreItem::CFindIgnoreItem()
{
    Enabled = TRUE;
    Path = NULL;
    Type = fiitUnknow;
}

CFindIgnoreItem::~CFindIgnoreItem()
{
    if (Path != NULL)
        free(Path);
}

//*********************************************************************************
//
// CFindIgnore
//

CFindIgnore::CFindIgnore()
    : Items(5, 5)
{
    Reset();
}

void CFindIgnore::Reset()
{
    Items.DestroyMembers();

    Add(TRUE, "\\System Volume Information");
    Add(FALSE, "Local Settings\\Temporary Internet Files");
}

BOOL CFindIgnore::Save(HKEY hKey)
{
    ClearKey(hKey);

    HKEY subKey;
    char buf[30];
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        itoa(i + 1, buf, 10);
        if (CreateKey(hKey, buf, subKey))
        {
            SetValue(subKey, FINDIGNOREITEM_PATH_REG, REG_SZ, Items[i]->Path, -1);
            if (!Items[i]->Enabled) // save only if it is FALSE
                SetValue(subKey, FINDIGNOREITEM_ENABLED_REG, REG_DWORD, &Items[i]->Enabled, sizeof(DWORD));
            CloseKey(subKey);
        }
        else
            break;
    }
    return TRUE;
}

BOOL CFindIgnore::Load(HKEY hKey, DWORD cfgVersion)
{
    HKEY subKey;
    char buf[30];
    int i = 1;
    strcpy(buf, "1");
    Items.DestroyMembers();
    while (OpenKey(hKey, buf, subKey))
    {
        CFindIgnoreItem* item = new CFindIgnoreItem;
        if (item == NULL)
        {
            TRACE_E(LOW_MEMORY);
            break;
        }
        char path[MAX_PATH];
        if (!GetValue(subKey, FINDIGNOREITEM_PATH_REG, REG_SZ, path, MAX_PATH))
            path[0] = 0;
        item->Path = DupStr(path);
        if (!GetValue(subKey, FINDIGNOREITEM_ENABLED_REG, REG_DWORD, &item->Enabled, sizeof(DWORD)))
            item->Enabled = TRUE; // saved only if it is FALSE
        if (Configuration.ConfigVersion < 32)
        {
            // users were confused that this folder was not searched
            // so we keep it listed but uncheck the checkbox
            // anyone interested can manually enable it
            if (strcmp(item->Path, "Local Settings\\Temporary Internet Files") == 0)
                item->Enabled = FALSE;
        }
        Items.Add(item);
        if (!Items.IsGood())
        {
            Items.ResetState();
            delete item;
            break;
        }
        itoa(++i, buf, 10);
        CloseKey(subKey);
    }

    return TRUE;
}

BOOL CFindIgnore::Load(CFindIgnore* source)
{
    Items.DestroyMembers();
    int i;
    for (i = 0; i < source->Items.Count; i++)
    {
        CFindIgnoreItem* item = source->At(i);
        if (!Add(item->Enabled, item->Path))
            return FALSE;
    }
    return TRUE;
}

BOOL CFindIgnore::Prepare(CFindIgnore* source)
{
    Items.DestroyMembers();
    int i;
    for (i = 0; i < source->Items.Count; i++)
    {
        CFindIgnoreItem* item = source->At(i);
        if (item->Enabled) // we are only interested in enabled items
        {
            const char* path = item->Path;
            while (*path == ' ')
                path++;
            CFindIgnoreItemType type = fiitRelative;
            if (path[0] == '\\' && path[1] != '\\')
                type = fiitRooted;
            else if ((path[0] == '\\' && path[1] == '\\') ||
                     LowerCase[path[0]] >= 'a' && LowerCase[path[0]] <= 'z' && path[1] == ':')
                type = fiitFull;

            char buff[3 * MAX_PATH];
            if (strlen(path) >= 2 * MAX_PATH)
            {
                TRACE_E("CFindIgnore::Prepare() Path too long!");
                return FALSE;
            }
            if (type == fiitFull)
            {
                strcpy(buff, path);
            }
            else
            {
                if (path[0] == '\\')
                    strcpy(buff, path);
                else
                {
                    buff[0] = '\\';
                    strcpy(buff + 1, path);
                }
            }
            if (buff[strlen(buff) - 1] != '\\')
                strcat(buff, "\\");
            if (!Add(TRUE, buff))
                return FALSE;
            item = Items[Items.Count - 1];
            item->Type = type;
            item->Len = (int)strlen(buff);
        }
    }
    return TRUE;
}

const char* SkipRoot(const char* path)
{
    if (path[0] == '\\' && path[1] == '\\') // UNC
    {
        const char* s = path + 2;
        while (*s != 0 && *s != '\\')
            s++;
        if (*s != 0)
            s++; // '\\'
        while (*s != 0 && *s != '\\')
            s++;
        return s;
    }
    else
    {
        return path + 2;
    }
}

BOOL CFindIgnore::Contains(const char* path, int startPathLen)
{
    // full path
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        // startPathLen is the path length entered in the Find dialog (search root);
        // only its subpaths are ignored, see https://forum.altap.cz/viewtopic.php?f=7&t=7434
        CFindIgnoreItem* item = Items[i];
        switch (item->Type)
        {
        case fiitFull:
        {
            if (item->Len > startPathLen && StrNICmp(path, item->Path, item->Len) == 0)
                return TRUE;
            break;
        }

        case fiitRooted:
        {
            const char* noRoot = SkipRoot(path);
            if ((noRoot - path) + item->Len > startPathLen && StrNICmp(noRoot, item->Path, item->Len) == 0)
                return TRUE;
            break;
        }

        case fiitRelative:
        {
            const char* m = path;
            while (m != NULL)
            {
                m = StrIStr(m, item->Path);
                if (m != NULL) // found
                {
                    if ((m - path) + item->Len > startPathLen) // is it a subpath? then ignore it
                        return TRUE;
                    m++; // look for another occurrence, maybe it will be in a subpath
                }
            }
            break;
        }
        }
    }
    return FALSE;
}

BOOL CFindIgnore::Move(int srcIndex, int dstIndex)
{
    CFindIgnoreItem* tmp = Items[srcIndex];
    if (srcIndex < dstIndex)
    {
        int i;
        for (i = srcIndex; i < dstIndex; i++)
            Items[i] = Items[i + 1];
    }
    else
    {
        int i;
        for (i = srcIndex; i > dstIndex; i--)
            Items[i] = Items[i - 1];
    }
    Items[dstIndex] = tmp;
    return TRUE;
}

BOOL CFindIgnore::Add(BOOL enabled, const char* path)
{
    CFindIgnoreItem* item = new CFindIgnoreItem;
    if (item == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }
    item->Enabled = enabled;
    item->Path = DupStr(path);
    if (item->Path == NULL)
    {
        TRACE_E(LOW_MEMORY);
        delete item;
        return FALSE;
    }
    Items.Add(item);
    if (!Items.IsGood())
    {
        Items.ResetState();
        delete item;
        return FALSE;
    }
    return TRUE;
}

BOOL CFindIgnore::AddUnique(BOOL enabled, const char* path)
{
    int len = (int)strlen(path);
    if (len < 1)
        return FALSE;
    if (path[len - 1] == '\\') // compare without trailing backslashes
        len--;
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        CFindIgnoreItem* item = Items[i];
        int itemLen = (int)strlen(item->Path);
        if (itemLen < 1)
            continue;
        if (item->Path[itemLen - 1] == '\\') // compare without a trailing backslash
            itemLen--;
        if (len != itemLen)
            continue;
        if (StrNICmp(path, item->Path, len) == 0)
        {
            item->Enabled = TRUE; // always enable this item
            return TRUE;
        }
    }
    // not found -- add it
    return Add(enabled, path);
}

BOOL CFindIgnore::Set(int index, BOOL enabled, const char* path)
{
    if (index < 0 || index >= Items.Count)
    {
        TRACE_E("Index is out of range");
        return FALSE;
    }
    char* p = DupStr(path);
    if (p == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }
    CFindIgnoreItem* item = Items[index];
    if (item->Path != NULL)
        free(item->Path);
    item->Path = p;
    item->Enabled = enabled;
    return TRUE;
}

//*********************************************************************************
//
// CDuplicateCandidates
//
// Container for CFoundFilesData when searching for duplicate files.
// 1) In the first phase, all files matching the Find criteria are added
//    to the CDuplicateCandidates object using the Add method.
// 2) Then the Examine() method is called which sorts the array using data->FindDupFlags criteria. If file contents
//    are compared, MD5 digests are calculated for potentially identical files.
//    Then, the array is sorted again and single files are removed so
//    Only files that appear at least twice remain in the array.
//    These get a Group variable so that sets can be distinguished in the result window.
//

constexpr size_t maxFindProtectedScopes = 32;
static_assert(maxFindProtectedScopes < (1u << 6)); // CFoundFilesData::ProtectedScope
using FindProtectedSessions = std::array<UacF7::ListingSession*, maxFindProtectedScopes>;

namespace
{
struct FindConsent
{
    CGrepData* data;
    const std::wstring& path;
    UacF7::ListingSession& session;
    FindConsentScope scope;
    DWORD result = ERROR_CANCELLED;

    static DWORD WINAPI Launch(void* context)
    {
#ifndef CALLSTK_DISABLE
        CCallStack stack;
#endif
        auto& request = *static_cast<FindConsent*>(context);
        if (request.data->StopSearch)
            return 0;
        static std::mutex consentMutex;
        const std::unique_lock<std::mutex> consent(consentMutex, std::try_to_lock);
        if (!consent.owns_lock())
            request.result = ERROR_BUSY;
        else if (SendMessage(request.data->HWindow, WM_USER_FIND_PROTECTED_CONSENT,
                             reinterpret_cast<WPARAM>(&request.path), static_cast<LPARAM>(request.scope)) &&
                 !request.data->StopSearch)
            request.result = request.session.Start(request.data->HWindow, request.data->StopEvent);
        return 0;
    }
};

DWORD AuthorizeFindFile(CGrepData* data, const std::wstring& path, UacF7::ListingSession& session)
{
    if (data->StopSearch)
        return ERROR_CANCELLED;
    if (!session.PrepareFile(path))
        return ERROR_ACCESS_DENIED;
    FindConsent consent{data, path, session, FindConsentScope::File};
    HANDLE thread = HANDLES(CreateThread(nullptr, 0, FindConsent::Launch, &consent, 0, nullptr));
    if (!thread)
        return ERROR_NOT_ENOUGH_MEMORY;
    WaitForSingleObject(thread, INFINITE);
    HANDLES(CloseHandle(thread));
    return consent.result;
}
}

static bool FindDuplicateContent(const CGrepData& data)
{
    constexpr DWORD flags = FIND_DUPLICATES_SIZE | FIND_DUPLICATES_CONTENT;
    return data.FindDuplicates && (data.FindDupFlags & flags) == flags;
}

BOOL TestFileContentAux(BOOL& ok, CQuadWord& fileOffset, const CQuadWord& totalSize,
                        DWORD viewSize, const char* path, char* txt, CGrepData* data);

class ProtectedFindPredicates
{
public:
    void Prepare(const CGrepData* data)
    {
        if (data->Criteria.HasTags())
        {
            const auto text = SalMultiByteToWidePath(data->Criteria.GetTagsText(), CP_UTF8);
            ParseFileTagsW(text.c_str(), requestedTags);
        }
    }

    bool Match(CGrepData* data, UacF7::ListingSession& session, const std::wstring& path,
               const std::string& display, const CQuadWord& size, DWORD& error)
    {
        return (requestedTags.empty() || MatchTags(data, session, path, size.Value, error)) &&
               (!data->Grep || MatchContent(data, session, display, size, error));
    }

private:
    struct TagRead
    {
        CGrepData* data;
        UacF7::ListingSession& session;
    };

    bool MatchContent(CGrepData* data, UacF7::ListingSession& session, const std::string& pathText, const CQuadWord& listedSize, DWORD& error)
    {
        if (content.empty())
        {
            try
            {
                content.resize(4 * UacF7::archiveReadBlock);
            }
            catch (const std::bad_alloc&)
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                return false;
            }
        }

        CQuadWord offset(0, 0);
        BOOL found = FALSE;
        while (!error && !data->StopSearch && offset < listedSize)
        {
            const DWORD viewSize = static_cast<DWORD>(std::min<ULONGLONG>(content.size(), listedSize.Value - offset.Value));
            for (DWORD position = 0; position < viewSize && !error && !data->StopSearch;)
            {
                const DWORD count = std::min<DWORD>(UacF7::archiveReadBlock, viewSize - position);
                error = session.ReadContent(offset.Value + position, content.data() + position, count);
                position += count;
            }
            if (error || data->StopSearch)
                break;
            const CQuadWord before(offset);
            if (!TestFileContentAux(found, offset, listedSize, viewSize, pathText.c_str(), content.data(), data) || found)
                break;
            if (!data->StopSearch && offset == before)
                error = ERROR_INVALID_DATA;
        }
        return found;
    }

    static DWORD ReadTagBlock(void* context, ULONGLONG offset, void* output, DWORD bytes)
    {
        auto& request = *static_cast<TagRead*>(context);
        return request.data->StopSearch ? ERROR_CANCELLED : request.session.ReadContent(offset, output, bytes);
    }

    bool MatchTags(CGrepData* data, UacF7::ListingSession& session, const std::wstring& filePath, ULONGLONG size, DWORD& error)
    {
        TagRead request{data, session};
        auto* stream = CFindTagStream::Create(size, UacF7::archiveReadBlock, ReadTagBlock, &request);
        if (!stream)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            return false;
        }
        std::vector<std::wstring> tags;
        const HRESULT hr = ReadFileTagsFromStreamW(filePath.c_str(), stream, tags);
        error = stream->Detach();
        stream->Release();
        if (!error && FAILED(hr))
            error = static_cast<DWORD>(hr);
        return !error && FileTagValuesMatchW(tags, requestedTags, data->Criteria.GetTagsMode()) != FALSE;
    }

    std::vector<char> content;
    std::vector<std::wstring> requestedTags;
};

class CDuplicateCandidates : public TIndirectArray<CFoundFilesData>
{
public:
    CDuplicateCandidates() : TIndirectArray<CFoundFilesData>(2000, 4000) {}

    // - loading/calculating MD5 digests
    // - removing single files
    // - setting the Group variable
    // - setting the Different flag
    void Examine(CGrepData* data, const FindProtectedSessions& protectedSessions, size_t remainingFileConsents);
protected:
    // compares two records using criteria byName, bySize and byMD5
    // byPath is a criterium with the lowest priority and is used only for clearer output
    int CompareFunc(CFoundFilesData* f1, CFoundFilesData* f2, BOOL byName, BOOL bySize, BOOL byMD5, BOOL byPath);

    // sort stored files by byName, bySize and byMD5 criteria
    void QuickSort(int left, int right, BOOL byName, BOOL bySize, BOOL byMD5);

    // goes through all stored items and uses CompareFunc to identify those that
    // appear only once; those are then removed from the array
    // before calling this method, the array must be sorted with QuickSort
    void RemoveSingleFiles(BOOL byName, BOOL bySize, BOOL byMD5);

    // goes through all stored items and uses CompareFunc assign them
    // to groups; Alternates the Different bit for the groups  (0, 1, 0, 1, 0, 1, ...)
    // before calling this method, the array must be sorted with QuickSort
    void SetDifferentFlag(BOOL byName, BOOL bySize, BOOL byMD5);

    // goes through all stored items and uses the Different flag to assign
    // Group values; groups are numbered increasingly (0, 1, 2, 3, 4, 5, ...)
    void SetGroupByDifferentFlag();

    // compute MD5 from the file 'file'
    // 'progress' is the numeric value shown in the status bar (optimized so we do not
    // update the same porgress repeatedly)
    // 'readSize' holds the number of bytes read so far across all files
    // 'totalSize' is the total number of bytes of all files for which the MD5 digest
    // will be determined
    // the method returns TRUE, if the MD5 value was successfully read; the digest is stored at
    // (BYTE*)data->Group
    // the method returns FALSE on read errors or when the user aborts the operation
    // (then, the variable data->StopSearch is set to TRUE)
    BOOL GetMD5Digest(CGrepData* data, CFoundFilesData* file,
                      int* progress, CQuadWord* readSize, const CQuadWord* totalSize,
                      UacF7::ListingSession* protectedSession, size_t& remainingFileConsents,
                      ProtectedFindPredicates& predicates);
};

int CDuplicateCandidates::CompareFunc(CFoundFilesData* f1, CFoundFilesData* f2,
                                      BOOL byName, BOOL bySize, BOOL byMD5, BOOL byPath)
{
    int res;
    if (bySize)
    {
        if (byName)
            res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, f1->NameW.c_str(), -1, f2->NameW.c_str(), -1) - CSTR_EQUAL;
        else
            res = 0;
        if (res == 0)
        {
            if (f1->Size < f2->Size)
                res = -1;
            else
            {
                if (f1->Size == f2->Size)
                {
                    if (!byMD5 || f1->Size == CQuadWord(0, 0))
                        res = 0;
                    else
                        res = memcmp((void*)f1->Group, (void*)f2->Group, MD5_DIGEST_SIZE);
                }
                else
                    res = 1;
            }
        }
    }
    else
    {
        // byName && !bySize
        res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, f1->NameW.c_str(), -1, f2->NameW.c_str(), -1) - CSTR_EQUAL;
    }
    if (byPath && res == 0)
        res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, f1->PathW.c_str(), -1, f2->PathW.c_str(), -1) - CSTR_EQUAL;
    return res;
}

void CDuplicateCandidates::QuickSort(int left, int right, BOOL byName, BOOL bySize, BOOL byMD5)
{

LABEL_QuickSort:

    int i = left, j = right;
    CFoundFilesData* pivot = At((i + j) / 2);

    do
    {
        while (CompareFunc(At(i), pivot, byName, bySize, byMD5, TRUE) < 0 && i < right)
            i++;
        while (CompareFunc(pivot, At(j), byName, bySize, byMD5, TRUE) < 0 && j > left)
            j--;

        if (i <= j)
        {
            CFoundFilesData* swap = At(i);
            At(i) = At(j);
            At(j) = swap;
            i++;
            j--;
        }
    } while (i <= j);

    // the following "nice" code was replaced by a version that saves stack space (max. log(N) recursion depth)
    //  if (left < j) QuickSort(left, j, byName, bySize, byMD5);
    //  if (i < right) QuickSort(i, right, byName, bySize, byMD5);

    if (left < j)
    {
        if (i < right)
        {
            if (j - left < right - i) // both halves must be sorted: send the smaller half to recursion and handle the other via "goto"
            {
                QuickSort(left, j, byName, bySize, byMD5);
                left = i;
                goto LABEL_QuickSort;
            }
            else
            {
                QuickSort(i, right, byName, bySize, byMD5);
                right = j;
                goto LABEL_QuickSort;
            }
        }
        else
        {
            right = j;
            goto LABEL_QuickSort;
        }
    }
    else
    {
        if (i < right)
        {
            left = i;
            goto LABEL_QuickSort;
        }
    }
}

#define DUPLICATES_BUFFER_SIZE 16384 // buffer size for MD5 calculation

BOOL CDuplicateCandidates::GetMD5Digest(CGrepData* data, CFoundFilesData* file,
                                        int* progress, CQuadWord* readSize, const CQuadWord* totalSize,
                                        UacF7::ListingSession* protectedSession, size_t& remainingFileConsents,
                                        ProtectedFindPredicates& predicates)
{
    bool scoped = file->ProtectedScope != 0;
    const std::wstring fullPath = file->GetFullNameW();
    const std::string display = SalWideToMultiBytePath(fullPath.c_str(), CP_UTF8);
    data->SearchingText->Set(display.c_str());
    HANDLE local = INVALID_HANDLE_VALUE;
    DWORD error = ERROR_SUCCESS;
    ULONGLONG size = 0;
    FILE_BASIC_INFO initial{};
    bool opened = false;
    UacF7::ListingSession singleFile;
    if (!scoped)
    {
        const std::wstring apiPath = SalPathAddExtendedPrefixW(fullPath.c_str());
        local = HANDLES_Q(CreateFileW(apiPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL));
        opened = local != INVALID_HANDLE_VALUE;
        if (opened)
        {
            LARGE_INTEGER length{};
            ULARGE_INTEGER listedWrite{};
            listedWrite.LowPart = file->LastWrite.dwLowDateTime;
            listedWrite.HighPart = file->LastWrite.dwHighDateTime;
            if (!GetFileSizeEx(local, &length) || !GetFileInformationByHandleEx(local, FileBasicInfo, &initial, sizeof(initial)))
                error = GetLastError();
            else if (length.QuadPart < 0 || static_cast<ULONGLONG>(length.QuadPart) != file->Size.Value ||
                     static_cast<ULONGLONG>(initial.LastWriteTime.QuadPart) != listedWrite.QuadPart)
                error = ERROR_FILE_INVALID;
            else
                size = static_cast<ULONGLONG>(length.QuadPart);
        }
        else
        {
            error = GetLastError();
            if (error == ERROR_ACCESS_DENIED && remainingFileConsents &&
                !(file->Attr & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_ENCRYPTED)))
            {
                --remainingFileConsents;
                error = AuthorizeFindFile(data, fullPath, singleFile);
                if (!error)
                {
                    protectedSession = &singleFile;
                    scoped = true;
                }
            }
        }
    }
    bool match = true;
    if (scoped)
    {
        FILETIME modified{};
        UacF7::ArchiveRevision revision{};
        error = protectedSession ? protectedSession->OpenContent(fullPath, size, modified, revision) : ERROR_ACCESS_DENIED;
        opened = error == ERROR_SUCCESS;
        if (opened)
        {
            if (size != file->Size.Value || CompareFileTime(&modified, &file->LastWrite) != 0)
                error = ERROR_FILE_INVALID;
            else
                match = predicates.Match(data, *protectedSession, fullPath, display, file->Size, error);
        }
    }
    BYTE buffer[DUPLICATES_BUFFER_SIZE];
    MD5 context;
    ULONGLONG offset = 0;
    while (match && !error && !data->StopSearch && offset < size)
    {
        const DWORD requested = static_cast<DWORD>(std::min<ULONGLONG>(sizeof(buffer), size - offset));
        DWORD read = requested;
        if (scoped)
            error = protectedSession->ReadContent(offset, buffer, read);
        else if (!ReadFile(local, buffer, requested, &read, nullptr))
            error = GetLastError();
        else if (!read)
            error = ERROR_HANDLE_EOF;

        if (error || data->StopSearch)
            break;
        offset += read;
        context.update(buffer, read);
        *readSize += CQuadWord(read, 0);
        const int newProgress = *readSize >= *totalSize ? (totalSize->Value == 0 ? 0 : 100)
            : static_cast<int>(((*readSize * CQuadWord(100, 0)) / *totalSize).Value);
        if (newProgress != *progress)
        {
            *progress = newProgress;
            char value[] = {static_cast<char>(newProgress), 0};
            data->SearchingText2->Set(value);
        }
    }
    if (scoped && opened)
    {
        const DWORD closeError = protectedSession->CloseContent();
        if (!error)
            error = closeError;
    }
    if (local != INVALID_HANDLE_VALUE)
    {
        if (!error && !data->StopSearch)
        {
            FILE_BASIC_INFO current{};
            LARGE_INTEGER length{};
            if (!GetFileInformationByHandleEx(local, FileBasicInfo, &current, sizeof(current)) || !GetFileSizeEx(local, &length))
                error = GetLastError();
            else if (static_cast<ULONGLONG>(length.QuadPart) != size ||
                     current.LastWriteTime.QuadPart != initial.LastWriteTime.QuadPart ||
                     current.ChangeTime.QuadPart != initial.ChangeTime.QuadPart)
                error = ERROR_FILE_INVALID;
        }
        HANDLES(CloseHandle(local));
    }
    if (data->StopSearch)
        return FALSE;
    if (error)
    {
        const auto message = error == ERROR_CANCELLED ? std::string(LoadStr(IDS_FINDLOG_SKIP)) :
            FormatFindError(opened ? IDS_ERROR_READING_FILE2 : IDS_ERROR_OPENING_FILE2, error);
        const DWORD flags = error == ERROR_CANCELLED ? FLI_INFO : FLI_ERROR;
        FIND_LOG_ITEM log{flags, message.c_str(), display.c_str()};
        SendMessage(data->HWindow, WM_USER_ADDLOG, reinterpret_cast<WPARAM>(&log), 0);
        return FALSE;
    }
    if (!match)
        return FALSE;
    if (file->Group)
    {
        context.finalize();
        memcpy(reinterpret_cast<BYTE*>(file->Group), context.digest, MD5_DIGEST_SIZE);
    }
    return TRUE;
}

void CDuplicateCandidates::RemoveSingleFiles(BOOL byName, BOOL bySize, BOOL byMD5)
{
    if (Count == 0)
        return;

    CFoundFilesData* lastData = Count > 0 ? At(Count - 1) : NULL;
    int lastDataIndex = Count - 1;
    BOOL lastIsSingle = TRUE;
    int i;
    for (i = Count - 2; i >= 0; i--)
    {
        if (CompareFunc(At(i), lastData, byName, bySize, byMD5, FALSE) == 0)
        {
            lastIsSingle = FALSE;
        }
        else
        {
            if (lastIsSingle)
            {
                // lastData points to an item that occurs only once; remove it
                Delete(lastDataIndex);
            }
            lastDataIndex = i;
            lastData = At(i);
            lastIsSingle = TRUE;
        }
    }
    if (lastIsSingle)
    {
        // lastData points to an item that occurs only once; remove it
        Delete(lastDataIndex);
    }
}

void CDuplicateCandidates::SetDifferentFlag(BOOL byName, BOOL bySize, BOOL byMD5)
{
    if (Count == 0)
        return;

    CFoundFilesData* lastData = NULL;
    int different = 0;
    if (Count > 0)
    {
        lastData = At(0);
        lastData->Different = different;
    }
    int i;
    for (i = 1; i < Count; i++)
    {
        CFoundFilesData* data = At(i);
        if (CompareFunc(data, lastData, byName, bySize, byMD5, FALSE) == 0)
        {
            data->Different = different;
        }
        else
        {
            different++;
            if (different > 1)
                different = 0;
            lastData = data;
            lastData->Different = different;
        }
    }
}

void CDuplicateCandidates::SetGroupByDifferentFlag()
{
    if (Count == 0)
        return;

    CFoundFilesData* lastData = NULL;
    DWORD custom = 0;
    if (Count > 0)
    {
        lastData = At(0);
        lastData->Group = custom;
    }
    int i;
    for (i = 1; i < Count; i++)
    {
        CFoundFilesData* data = At(i);
        if (data->Different == lastData->Different)
        {
            data->Group = custom;
        }
        else
        {
            custom++;
            lastData = data;
            lastData->Group = custom;
        }
    }
}

void CDuplicateCandidates::Examine(CGrepData* data, const FindProtectedSessions& protectedSessions,
                                   size_t remainingFileConsents)
{
    if (Count == 0)
        return;

    // we received a list of found files matching the criteria

    // extract criteria for duplicate search
    BOOL byName = (data->FindDupFlags & FIND_DUPLICATES_NAME) != 0;
    BOOL bySize = (data->FindDupFlags & FIND_DUPLICATES_SIZE) != 0;
    const bool byContent = FindDuplicateContent(*data);

    // search completed, preparing results (MD5 computation may still follow)
    data->SearchingText->Set(LoadStr(IDS_FIND_DUPS_RESULTS));

    // sort them according to selected criteria
    QuickSort(0, Count - 1, byName, bySize, FALSE);

    // remove items that occur only once
    RemoveSingleFiles(byName, bySize, FALSE);

    CMD5Digest* digest = NULL;
    if (byContent)
    {
        // for files larger than 0 bytes we'll compute MD5
        // allocate memory for MD5 digests at once

        // determine the number of files with size greater than 0 bytes
        DWORD count = 0;
        int i;
        for (i = 0; i < Count; i++)
        {
            CFoundFilesData* file = At(i);
            if (file->Size > CQuadWord(0, 0))
                count++;
        }

        if (count > 0)
        {
            // allocate memory for MD5 digests in one array
            digest = (CMD5Digest*)malloc(count * sizeof(CMD5Digest));
            if (digest == NULL)
            {
                TRACE_E(LOW_MEMORY);
                return;
            }

            // set up the pointers
            CMD5Digest* iterator = digest;
            for (i = 0; i < Count; i++)
            {
                CFoundFilesData* file = At(i);
                if (file->Size > CQuadWord(0, 0))
                {
                    file->Group = (DWORD_PTR)iterator;
                    iterator++;
                }
                else
                    file->Group = 0;
            }
        }

        // Zero-length candidates also need their size and revision verified.
        CQuadWord totalSize(0, 0);
        for (i = 0; i < Count; i++)
            totalSize += At(i)->Size;

        // retrieve the MD5 digest of files
        CQuadWord readSize(0, 0);
        int progress = -1;
        ProtectedFindPredicates predicates;
        predicates.Prepare(data);
        for (i = Count - 1; i >= 0; i--)
        {
            CFoundFilesData* file = At(i);
            const unsigned scope = file->ProtectedScope;
            auto* session = scope && scope <= protectedSessions.size() ? protectedSessions[scope - 1] : nullptr;
            if (scope && (!session || !session->IsOpen()))
            {
                // The operation that lost the connection already logged the error.
                Delete(i);
                continue;
            }
            if (!GetMD5Digest(data, file, &progress, &readSize, &totalSize, session, remainingFileConsents, predicates))
            {
                if (data->StopSearch)
                {
                    // the user wants to stop searching
                    // trim the unprocessed items
                    int j;
                    for (j = 0; j <= i; j++)
                        Delete(0);
                    break; // show at least the duplicates that have been already found
                }
                // an error occurred during reading the file but the user wants to continue
                // exclude the file from candidates
                Delete(i);
            }
        }

        // search finished, preparing results
        data->SearchingText->Set(LoadStr(IDS_FIND_DUPS_RESULTS));

        // sort the files again
        if (Count > 0)
            QuickSort(0, Count - 1, byName, bySize, TRUE);

        // remove items that occur only once
        RemoveSingleFiles(byName, bySize, TRUE);
    }

    // set the Different bit
    SetDifferentFlag(byName, bySize, byContent);

    if (digest != NULL)
        free(digest);

    // assign numbers starting from 0 to the Group variable
    // files with the same Different bit will share the same value
    SetGroupByDifferentFlag();

    // add them to the listview
    int i;
    for (i = 0; i < Count; i++)
    {
        data->FoundFilesListView->Add(At(i));
        if (!data->FoundFilesListView->IsGood())
        {
            TRACE_E(LOW_MEMORY);
            data->FoundFilesListView->ResetState();
            // cut off items that were not added
            int j;
            for (j = Count - 1; j >= i; j--)
                Delete(j);
            // detach the already added ones
            DetachMembers();
            return;
        }
    }
    DetachMembers();
    return;
}

//*********************************************************************************
//
// CSearchForData
//

void CSearchForData::Set(const char* dir, const char* masksGroup, BOOL includeSubDirs)
{
    strcpy(Dir, dir);
    MasksGroup.SetMasksString(masksGroup);
    IncludeSubDirs = includeSubDirs;
}

//*********************************************************************************
//
// Search engine
//

#define SEARCH_SIZE 10000 // must be greater than the maximum string length

int SearchForward(CGrepData* data, char* txt, int size, int off)
{
    if (size < 0)
        return -1;
    int curOff = off, curSize = min(SEARCH_SIZE, size - curOff);
    int found;
    while (!data->StopSearch)
    {
        if (curSize >= data->SearchData.GetLength())
            found = data->SearchData.SearchForward(txt + curOff, curSize, 0); // find
        else
            break; // not found
        if (found == -1)
        {
            curOff += curSize - data->SearchData.GetLength() + 1;
            curSize = min(SEARCH_SIZE, size - curOff);
        }
        else
            return curOff + found;
    }
    return -1;
}

//
// ****************************************************************************

BOOL TestFileContentAux(BOOL& ok, CQuadWord& fileOffset, const CQuadWord& totalSize,
                        DWORD viewSize, const char* path, char* txt, CGrepData* data)
{
    __try
    {
        if (data->Regular)
        {
            char *beg, *end, *nextBeg, *totalEnd, *endLimit;
            //      BOOL EOL_NULL = TRUE;
            BOOL EOL_CR = data->EOL_CR;
            BOOL EOL_LF = data->EOL_LF;
            BOOL EOL_CRLF = data->EOL_CRLF;
            beg = txt;
            totalEnd = txt + viewSize;

            while (!data->StopSearch && beg < totalEnd)
            {
                end = beg;
                endLimit = beg + GREP_LINE_LEN;
                if (endLimit > totalEnd)
                    endLimit = totalEnd;
                nextBeg = NULL;
                do
                {
                    if (*end > '\r')
                        end++;
                    else
                    {
                        if (*end == '\r')
                        {
                            if (end + 1 < totalEnd && *(end + 1) == '\n' && EOL_CRLF)
                            {
                                nextBeg = end + 2;
                                break;
                            }
                            else
                            {
                                if (EOL_CR &&
                                    (end + 1 < totalEnd ||                              // it was able to test that there is no LF there
                                     !EOL_CRLF ||                                       // LF should not be considered an EOL
                                     fileOffset + CQuadWord(viewSize, 0) >= totalSize)) // it is the end of the file
                                {
                                    nextBeg = end + 1;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (*end == '\n' && EOL_LF || *end == 0 /*&& EOL_NULL*/)
                            {
                                nextBeg = end + 1;
                                break;
                            }
                        }
                        end++;
                    }
                } while (end < endLimit);
                if (nextBeg == NULL)
                    nextBeg = end;

                if (end == endLimit && endLimit == totalEnd &&       // the line may continue beyond this view
                    fileOffset + CQuadWord(viewSize, 0) < totalSize) // the end of the file is not in the file view
                {                                                    // the line can continue beyond the boundary of the current view of the file
                    fileOffset += CQuadWord(DWORD(beg - txt), 0);
                    return TRUE; // continue with the next view segment
                }

                // line beg->end
                if (data->RegExp.SetLine(beg, end))
                {
                    int foundLen, start = 0;

                GREP_REGEXP_NEXT:

                    int found = data->RegExp.SearchForward(start, foundLen);
                    if (found != -1)
                    {
                        if (data->WholeWords)
                        {
                            if ((found == 0 || *(beg + found - 1) != '_' && IsNotAlphaNorNum[*(beg + found - 1)]) &&
                                (found + foundLen == (end - beg) ||
                                 *(beg + found + foundLen) != '_' && IsNotAlphaNorNum[*(beg + found + foundLen)]))
                            {
                                ok = TRUE; // found
                                break;
                            }
                            start = found + 1;
                            if (start < end - beg)
                                goto GREP_REGEXP_NEXT;
                        }
                        else
                        {
                            ok = TRUE; // found
                            break;
                        }
                    }
                }
                else
                {
                    FIND_LOG_ITEM log;
                    log.Flags = FLI_ERROR;
                    log.Text = data->RegExp.GetLastErrorText();
                    log.Path = NULL;
                    SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
                    return FALSE; // do not search this file further
                }

                beg = nextBeg;
            }
            // line ends exactly at the end of the view segment (may also be the end of the file)
            if (beg >= totalEnd)
                fileOffset += CQuadWord(viewSize, 0); // advance the offset to continue searching
        }
        else
        {
            int off = 0;
            while (1)
            {
                off = SearchForward(data, txt, viewSize, off);
                if (off != -1)
                {
                    if (data->WholeWords)
                    {
                        if ((fileOffset + CQuadWord(off, 0) == CQuadWord(0, 0) ||                                        // beginning of the file
                             off > 0 && txt[off - 1] != '_' && IsNotAlphaNorNum[txt[off - 1]]) &&                        // not at the start of the buffer and no letter or digit before the pattern
                            (fileOffset + CQuadWord(off, 0) + CQuadWord(data->SearchData.GetLength(), 0) >= totalSize || // end of the file
                             (DWORD)(off + data->SearchData.GetLength()) < viewSize &&                                   // not at the end of the buffer
                                 txt[off + data->SearchData.GetLength()] != '_' &&
                                 IsNotAlphaNorNum[txt[off + data->SearchData.GetLength()]])) // no letter or digit after the pattern
                        {
                            ok = TRUE; // found
                            break;
                        }
                        off++;
                    }
                    else
                    {
                        ok = TRUE; // found
                        break;
                    }
                }
                else
                    break; // not found or terminated
            }
            if (!ok && !data->StopSearch) // not found and not interrupted
            {
                if (fileOffset + CQuadWord(viewSize, 0) < totalSize &&
                    CQuadWord(data->SearchData.GetLength() + 1, 0) < CQuadWord(viewSize, 0))
                {
                    fileOffset = fileOffset + CQuadWord(viewSize, 0) - CQuadWord(data->SearchData.GetLength() + 1, 0);
                }
                else
                    fileOffset = totalSize; // the pattern cannot be in the file anymore
            }
        }
        return TRUE; // continue searching (unless we are at the end of the file)
    }
    __except (HandleFileException(GetExceptionInformation(), txt, viewSize))
    {
        // file error
        FIND_LOG_ITEM log;
        log.Flags = FLI_ERROR;
        log.Text = LoadStr(IDS_FILEREADERROR2);
        log.Path = path;
        SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
        ok = FALSE;   // not found
        return FALSE; // do not continue searching
    }
}

BOOL TestFileContent(DWORD sizeLow, DWORD sizeHigh, const char* path, CGrepData* data, BOOL isLink,
                     bool* openDenied = nullptr, const wchar_t* pathW = nullptr)
{
    if (openDenied)
        *openDenied = false;
    CQuadWord totalSize(sizeLow, sizeHigh);
    CQuadWord fileOffset(0, 0);
    DWORD viewSize = 0;

    BOOL ok = FALSE;
    if (totalSize > CQuadWord(0, 0) || isLink)
    {
        DWORD err = ERROR_SUCCESS;
        data->SearchingText->Set(path); // set the current file
        HANDLE hFile = pathW
            ? HANDLES_Q(CreateFileW(pathW, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr))
            : HANDLES_Q(CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
        BOOL getLinkFileSizeErr = FALSE;
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // links have zero file size; size of the target file must be obtained separately
            if (!isLink || SalGetFileSize(hFile, totalSize, err))
            {
                HANDLE mFile = HANDLES(CreateFileMapping(hFile, NULL, PAGE_READONLY,
                                                         0, 0, NULL));
                if (mFile != NULL)
                {
                    CQuadWord allocGran(AllocationGranularity, 0);
                    while (!data->StopSearch && fileOffset < totalSize)
                    {
                        // ensure the offset matches the granularity
                        CQuadWord mapFileOffset(fileOffset);
                        mapFileOffset = (mapFileOffset / allocGran) * allocGran;

                        // calculate the size of the view segment
                        if (CQuadWord(VOF_VIEW_SIZE, 0) <= totalSize - mapFileOffset)
                            viewSize = VOF_VIEW_SIZE;
                        else
                            viewSize = (DWORD)(totalSize - mapFileOffset).Value;

                        // map the file view
                        char* txt = (char*)HANDLES(MapViewOfFile(mFile, FILE_MAP_READ,
                                                                 mapFileOffset.HiDWord, mapFileOffset.LoDWord,
                                                                 viewSize));
                        if (txt != NULL)
                        {
                            // let the file view be examined
                            DWORD diff = (DWORD)(fileOffset - mapFileOffset).Value;
                            BOOL err2 = !TestFileContentAux(ok, fileOffset, totalSize, viewSize - diff,
                                                            path, txt + diff, data);
                            HANDLES(UnmapViewOfFile(txt));
                            if (err2 || ok)
                                break;
                        }
                        else
                            err = GetLastError();
                    }

                    HANDLES(CloseHandle(mFile));
                }
                else
                    err = GetLastError();
            }
            else
                getLinkFileSizeErr = TRUE;
            HANDLES(CloseHandle(hFile));
        }
        else
        {
            err = GetLastError();
            if (openDenied && !isLink && err == ERROR_ACCESS_DENIED)
            {
                *openDenied = true;
                return FALSE;
            }
        }

        if (err != ERROR_SUCCESS || getLinkFileSizeErr)
        {
            const auto errorText = FormatFindError(getLinkFileSizeErr ? IDS_GETLINKTGTFILESIZEERROR : IDS_ERROR_OPENING_FILE2, err);
            FIND_LOG_ITEM log{FLI_ERROR, errorText.c_str(), path};
            SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
        }
    }

    return ok;
}

static void RequestFindResultsRefresh(CGrepData* data)
{
    if (InterlockedExchange(&data->RefreshPosted, TRUE) == FALSE &&
        !PostMessage(data->HWindow, WM_USER_ADDFILE, data->SearchGeneration, 0))
    {
        InterlockedExchange(&data->RefreshPosted, FALSE);
    }
}

BOOL AddFoundItem(const char* path, const char* name, DWORD sizeLow, DWORD sizeHigh,
                  DWORD attr, const FILETIME* lastWrite, BOOL isDir, CGrepData* data,
                  CDuplicateCandidates* duplicateCandidates,
                  const wchar_t* pathW = nullptr, const wchar_t* nameW = nullptr,
                  unsigned protectedScope = 0)
{
    if (duplicateCandidates != NULL && isDir) // directories are irrelevant to us when searching for duplicates
        return TRUE;

    CFoundFilesData* foundData = new CFoundFilesData;
    if (foundData != NULL)
    {
        const CQuadWord size(sizeLow, sizeHigh);
        const BOOL good = pathW ? foundData->SetW(pathW, nameW, size, attr, lastWrite, isDir)
                               : foundData->Set(path, name, size, attr, lastWrite, isDir);
        if (good)
        {
            if (duplicateCandidates == NULL)
            {
                // duplicateCandidates == NULL, adding the item to data->FoundFilesListView
                data->FoundFilesListView->Add(foundData);
                if (!data->FoundFilesListView->IsGood())
                {
                    data->FoundFilesListView->ResetState();
                    delete foundData;
                    foundData = NULL;
                }
                else
                {
                    // request a listview redraw after every 100 added items
                    // also after 0.5 seconds has passed since the last redraw
                    // we also call update for each item when grepping
                    if (data->FoundFilesListView->GetCount() >= data->FoundVisibleCount + 100 ||
                        GetTickCount() - data->FoundVisibleTick >= 500
                        /* || data->Grep*/) // a half-second update interval is enough even for grepping
                    {
                        RequestFindResultsRefresh(data);
                    }
                    else
                        data->NeedRefresh = TRUE; // we will redraw at latest after 0.5 second
                }
            }
            else
            {
                // duplicateCandidates != NULL, adding the item to duplicateCandidates
                foundData->ProtectedScope = protectedScope;
                duplicateCandidates->Add(foundData);
                if (!duplicateCandidates->IsGood())
                {
                    duplicateCandidates->ResetState();
                    delete foundData;
                    foundData = NULL;
                }
            }
        }
        else
        {
            delete foundData;
            foundData = NULL;
        }
    }
    if (foundData == NULL)
    {
        FIND_LOG_ITEM log;
        log.Flags = FLI_ERROR;
        log.Text = LoadStr(IDS_CANTSHOWRESULTS);
        log.Path = NULL;
        SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);

        data->StopSearch = TRUE;
        return FALSE;
    }
    return TRUE;
}

namespace
{
bool IncludeRefinedFindResult(int mode, bool match)
{
    return (mode == 1 && match) || (mode == 2 && !match);
}

class FindProtectedRequest
{
public:
    ~FindProtectedRequest()
    {
        if (launchThread)
        {
            WaitForSingleObject(launchThread, INFINITE);
            HANDLES(CloseHandle(launchThread));
        }
    }

    bool Prepare(CGrepData* search, std::wstring deniedPath, CMaskGroup* masksGroup,
                 BOOL recursive, int searchRootLength, const std::shared_ptr<CFindIgnore>& ignore,
                 unsigned scope, const WIN32_FIND_DATAA* file)
    {
        path = std::move(deniedPath);
        singleFile = file != nullptr;
        readContent = singleFile || search->Grep || search->Criteria.HasTags() || FindDuplicateContent(*search);
        if (singleFile ? !session.PrepareFile(path) : !session.Prepare(path, readContent))
            return false;

        data = search;
        masks = masksGroup;
        includeSubDirs = recursive != FALSE;
        rootLength = searchRootLength;
        ignoreSnapshot = ignore;
        scopeId = scope;
        if (file)
        {
            const auto* name = SalPathFindFileNameW(path.c_str());
            if (wcscpy_s(fileEntry.cFileName, name) != 0)
                return false;
            fileEntry.dwFileAttributes = file->dwFileAttributes;
            fileEntry.nFileSizeLow = file->nFileSizeLow;
            fileEntry.nFileSizeHigh = file->nFileSizeHigh;
            fileEntry.ftLastWriteTime = file->ftLastWriteTime;
        }
        return true;
    }

    bool Matches(const std::wstring& deniedPath, const CMaskGroup* masksGroup, bool file) const
    {
        return singleFile == file && masks == masksGroup && path == deniedPath;
    }

    bool MergeRefinedRow(const WIN32_FIND_DATAA& file)
    {
        if (fileEntry.dwFileAttributes != file.dwFileAttributes || fileEntry.nFileSizeLow != file.nFileSizeLow ||
            fileEntry.nFileSizeHigh != file.nFileSizeHigh || CompareFileTime(&fileEntry.ftLastWriteTime, &file.ftLastWriteTime) != 0)
            return false;
        ++rows;
        return true;
    }

    bool Start()
    {
        consent.emplace(FindConsent{data, path, session, singleFile ? FindConsentScope::File :
                                    readContent ? FindConsentScope::Content : FindConsentScope::Metadata});
        launchThread = HANDLES(CreateThread(NULL, 0, FindConsent::Launch, &*consent, 0, nullptr));
        return launchThread != nullptr;
    }

    UacF7::ListingSession* Finish(CDuplicateCandidates* candidates)
    {
        if (!launchThread && (data->StopSearch || !Start()))
        {
            if (!data->StopSearch)
            {
                Log(path, ERROR_NOT_ENOUGH_MEMORY);
                if (singleFile && data->Refine == 2)
                    PublishSingleFile(nullptr, false);
            }
            return nullptr;
        }
        if (!data->StopSearch && WaitForSingleObject(launchThread, 0) == WAIT_TIMEOUT)
            data->SearchingText->Set(LoadStr(IDS_UAC_FIND_WAIT));
        WaitForSingleObject(launchThread, INFINITE);
        HANDLES(CloseHandle(launchThread));
        launchThread = nullptr;
        if (data->StopSearch)
            return nullptr;
        if (consent->result != ERROR_SUCCESS)
        {
            Log(path, consent->result);
            if (singleFile && data->Refine == 2)
                PublishSingleFile(nullptr, false);
            return nullptr;
        }

        data->SearchingText->Set(LoadStr(IDS_UAC_FIND_SCOPE));
        if (!candidates || !FindDuplicateContent(*data))
            predicates.Prepare(data);
        if (singleFile)
            PublishSingleFile(candidates, true);
        else
        {
            size_t remaining = 4096;
            Scan(path, 0, remaining, candidates);
        }
        return session.IsOpen() && !data->Refine && FindDuplicateContent(*data) ? &session : nullptr;
    }

private:
    void Log(const std::wstring& directory, DWORD error)
    {
        const std::string pathText = SalWideToMultiBytePath(directory.c_str(), CP_UTF8);
        const std::string text = error == ERROR_CANCELLED ? LoadStr(IDS_FINDLOG_SKIP) : GetErrorText(error);
        const DWORD flags = error == ERROR_CANCELLED ? FLI_INFO : FLI_ERROR;
        FIND_LOG_ITEM log{flags, text.c_str(), pathText.c_str()};
        SendMessage(data->HWindow, WM_USER_ADDLOG, reinterpret_cast<WPARAM>(&log), 0);
    }

    bool MatchFile(const std::wstring& directory, const WIN32_FIND_DATAW& file, const CQuadWord& listedSize)
    {
        std::wstring filePath = directory;
        SalPathAppendW(filePath, file.cFileName);
        const auto pathText = SalWideToMultiBytePath(filePath.c_str(), CP_UTF8);
        data->SearchingText->Set(pathText.c_str());

        ULONGLONG size = 0;
        FILETIME modified{};
        UacF7::ArchiveRevision revision{};
        DWORD error = session.OpenContent(filePath, size, modified, revision);
        if (error)
        {
            Log(filePath, error);
            return false;
        }

        if (size != listedSize.Value || CompareFileTime(&modified, &file.ftLastWriteTime) != 0)
            error = ERROR_FILE_INVALID;
        const bool match = !error && predicates.Match(data, session, filePath, pathText, listedSize, error);
        const DWORD closeError = session.CloseContent();
        if (!data->StopSearch && (error || closeError))
            Log(filePath, error ? error : closeError);
        return match && !error && !closeError && !data->StopSearch;
    }

    void Scan(const std::wstring& directory, unsigned depth, size_t& remaining, CDuplicateCandidates* candidates)
    {
        if (data->StopSearch || !session.IsOpen())
            return;
        if (!remaining || depth > 32)
        {
            Log(directory, ERROR_BUFFER_OVERFLOW);
            return;
        }
        if (ignoreSnapshot)
        {
            const std::string legacyPath = SalWideToMultiBytePath(directory.c_str(), GetACP()) + "\\";
            if (ignoreSnapshot->Contains(legacyPath.c_str(), rootLength))
            {
                Log(directory, ERROR_CANCELLED);
                return;
            }
        }

        std::vector<WIN32_FIND_DATAW> entries;
        DWORD error = session.List(directory, entries);
        if (!error && entries.size() > remaining)
            error = ERROR_BUFFER_OVERFLOW;
        if (error)
        {
            Log(directory, error);
            return;
        }
        remaining -= entries.size();
        for (auto& file : entries)
        {
            if (data->StopSearch || !session.IsOpen())
                return;
            AddCandidate(directory, file, candidates);
            const bool isDir = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (includeSubDirs && isDir && !(file.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                std::wstring child = directory;
                SalPathAppendW(child, file.cFileName);
                Scan(child, depth + 1, remaining, candidates);
            }
        }
    }

    bool MatchesCandidate(const std::wstring& directory, const WIN32_FIND_DATAW& file, CDuplicateCandidates* candidates)
    {
        CQuadWord size(file.nFileSizeLow, file.nFileSizeHigh);
        const bool isDir = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (!masks->AgreeMasksW(file.cFileName, nullptr) ||
            !data->Criteria.Test(file.dwFileAttributes, &size, &file.ftLastWriteTime))
            return false;
        if (isDir)
            return !data->Criteria.HasTags() && !data->Grep && !candidates;
        if (data->Grep && !size.Value)
            return false;
        // Content-duplicate candidates are grouped by metadata first. Match
        // their predicates later while the same file remains open for hashing.
        if ((candidates && FindDuplicateContent(*data)) || (!data->Grep && !data->Criteria.HasTags()))
            return true;
        return MatchFile(directory, file, size);
    }

    void PublishCandidate(const std::wstring& directory, const WIN32_FIND_DATAW& file, CDuplicateCandidates* candidates)
    {
        AddFoundItem(nullptr, nullptr, file.nFileSizeLow, file.nFileSizeHigh, file.dwFileAttributes,
                     &file.ftLastWriteTime, (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0,
                     data, candidates, directory.c_str(), file.cFileName, scopeId);
    }

    void AddCandidate(const std::wstring& directory, const WIN32_FIND_DATAW& file, CDuplicateCandidates* candidates)
    {
        if (MatchesCandidate(directory, file, candidates))
            PublishCandidate(directory, file, candidates);
    }

    void PublishSingleFile(CDuplicateCandidates* candidates, bool authorized)
    {
        const auto slash = path.find_last_of(L'\\');
        const auto directory = path.substr(0, slash == 2 ? 3 : slash);
        const bool match = authorized && MatchesCandidate(directory, fileEntry, candidates);
        if (data->Refine ? IncludeRefinedFindResult(data->Refine, match) : match)
            for (size_t i = 0; i < rows && !data->StopSearch; ++i)
                PublishCandidate(directory, fileEntry, candidates);
    }

    UacF7::ListingSession session;
    ProtectedFindPredicates predicates;
    std::wstring path;
    std::shared_ptr<CFindIgnore> ignoreSnapshot;
    CGrepData* data = nullptr;
    CMaskGroup* masks = nullptr;
    HANDLE launchThread = nullptr;
    std::optional<FindConsent> consent;
    WIN32_FIND_DATAW fileEntry{};
    size_t rows = 1;
    int rootLength = 0;
    unsigned scopeId = 0;
    bool includeSubDirs = false;
    bool readContent = false;
    bool singleFile = false;
};

class FindProtectedQueue
{
public:
    bool Add(CGrepData* search, const char* deniedPath, CMaskGroup* masks,
             BOOL recursive, int rootLength, const std::shared_ptr<CFindIgnore>& ignore,
             const WIN32_FIND_DATAA* file = nullptr)
    {
        if (search->StopSearch)
            return false;
        try
        {
            auto path = SalPathRemoveExtendedPrefixW(SalMultiByteToWidePath(deniedPath, GetACP()).c_str());
            return AddPath(search, std::move(path), masks, recursive, rootLength, ignore, file);
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
    }

    bool AddRefined(CGrepData* search, const std::wstring& path, CMaskGroup* masks, const CFoundFilesData& item)
    {
        WIN32_FIND_DATAA file{};
        file.dwFileAttributes = item.Attr;
        file.nFileSizeLow = item.Size.LoDWord;
        file.nFileSizeHigh = item.Size.HiDWord;
        file.ftLastWriteTime = item.LastWrite;
        try
        {
            return AddPath(search, SalPathRemoveExtendedPrefixW(path.c_str()), masks, FALSE, 0, {}, &file);
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
    }

    size_t Remaining() const { return requests.size() - count; }

    FindProtectedSessions Finish(CDuplicateCandidates* candidates)
    {
        FindProtectedSessions sessions{};
        for (size_t i = 0; i < count; ++i)
        {
            sessions[i] = requests[i]->Finish(candidates);
            if (!sessions[i])
                requests[i].reset();
        }
        return sessions;
    }

private:
    bool AddPath(CGrepData* search, std::wstring path, CMaskGroup* masks, BOOL recursive, int rootLength,
                 const std::shared_ptr<CFindIgnore>& ignore, const WIN32_FIND_DATAA* file)
    {
        if (search->StopSearch)
            return false;
        for (size_t i = 0; i < count; ++i)
            if (requests[i]->Matches(path, masks, file != nullptr) &&
                (!search->Refine || (file && requests[i]->MergeRefinedRow(*file))))
                return true;
        if (count == requests.size())
            return false;
        auto request = std::make_unique<FindProtectedRequest>();
        if (!request->Prepare(search, std::move(path), masks, recursive, rootLength, ignore,
                              static_cast<unsigned>(count + 1), file))
            return false;
        // Let ordinary traversal continue during the first consent.
        if (count == 0 && !request->Start())
            return false;
        requests[count++] = std::move(request);
        return true;
    }

    std::array<std::unique_ptr<FindProtectedRequest>, maxFindProtectedScopes> requests;
    size_t count = 0;
};
}

// 'dirStack' stores directories for late grepping. Otherwise,
// during searching in the current directory, recursive searching in subdirectories would occur. With this
// trick all files and directories matching the criteria are found first and
// then this function is called for all discovered directories.
// 'dirStack' only grows. When items are removed from it, they are just destroyed but
// not removed from the array, therefore the variable 'dirStackCount' holds the
// actual number of items in the array (always less than or equal to dirStack->Count).
// If memory is low or subdirectories are not searched,
// 'dirStack' is NULL.
// If 'duplicateCandidates' != NULL, found items will be added to this array
// instead of data->FoundFilesListView
void SearchDirectory(char (&path)[MAX_PATH], char* end, int startPathLen,
                     CMaskGroup* masksGroup, BOOL includeSubDirs, CGrepData* data,
                     TDirectArray<char*>* dirStack, int dirStackCount,
                     CDuplicateCandidates* duplicateCandidates,
                     const std::shared_ptr<CFindIgnore>& ignoreList, char (&message)[2 * MAX_PATH],
                     FindProtectedQueue& protectedQueue)
{
    SLOW_CALL_STACK_MESSAGE6("SearchDirectory(%s, , %d, %s, %d, , , %d, , )", path, startPathLen,
                             masksGroup->GetMasksString(), includeSubDirs, dirStackCount);

    if (ignoreList != NULL && ignoreList->Contains(path, startPathLen))
    {
        FIND_LOG_ITEM log;
        log.Flags = FLI_INFO;
        log.Text = LoadStr(IDS_FINDLOG_SKIP);
        log.Path = path;
        SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
        return;
    }

    if ((end - path) + 1 < _countof(path))
        strcpy_s(end, _countof(path) - (end - path), "*");
    else
    {
        FIND_LOG_ITEM log;
        log.Flags = FLI_ERROR;
        log.Text = LoadStr(IDS_TOOLONGNAME);
        log.Path = path;
        SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
        return;
    }

    WIN32_FIND_DATA file;
    HANDLE find = HANDLES_Q(FindFirstFile(path, &file));
    if (find != INVALID_HANDLE_VALUE)
    {
        if (end - path > 3)
            *(end - 1) = 0;
        else
            *end = 0;
        data->SearchingText->Set(path); // set the current path
        if (end - path > 3)
            *(end - 1) = '\\';
        else
            *end = 0;

        int dirStackEnterCount = 0; // number of items before starting the search at this level
        if (dirStack != NULL)
            dirStackEnterCount = dirStackCount;
        BOOL testFindNextErr = TRUE;

        do
        {
            BOOL isDir = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            BOOL ignoreDir = isDir && (lstrcmp(file.cFileName, ".") == 0 || lstrcmp(file.cFileName, "..") == 0);
            if (ignoreDir || (end - path) + lstrlen(file.cFileName) < _countof(path))
            {
                // after finding an item without displaying it and once 0.5 s have passed since the last redraw,
                // we request the listview to redraw
                if (data->NeedRefresh && GetTickCount() - data->FoundVisibleTick >= 500)
                {
                    RequestFindResultsRefresh(data);
                    data->NeedRefresh = FALSE;
                }

                if (file.cFileName[0] != 0 && !ignoreDir)
                {
                    // add all files and directories except "." and ".."

                    // test the criteria attributes, size, date and time
                    CQuadWord size(file.nFileSizeLow, file.nFileSizeHigh);
                    if (data->Criteria.Test(file.dwFileAttributes, &size, &file.ftLastWriteTime))
                    {
                        // file name
                        // let the extension be resolved if ext==NULL
                        if (masksGroup->AgreeMasks(file.cFileName, NULL)) // mask is OK
                        {
                            BOOL ok = TRUE;
                            bool openDenied = false;
                            const bool canQueueFile = !isDir &&
                                !(file.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_ENCRYPTED));
                            if (data->Criteria.HasTags())
                            {
                                if (isDir)
                                    ok = FALSE;
                                else
                                {
                                    strcpy_s(end, _countof(path) - (end - path), file.cFileName);
                                    const auto pathW = SalMultiByteToWidePath(path, GetACP());
                                    if (canQueueFile)
                                    {
                                        // The Shell property store can return empty keywords for an unreadable file.
                                        const auto apiPath = SalPathAddExtendedPrefixW(pathW.c_str());
                                        HANDLE readable = HANDLES_Q(CreateFileW(apiPath.c_str(), FILE_READ_DATA,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                            FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
                                        if (readable != INVALID_HANDLE_VALUE)
                                            HANDLES(CloseHandle(readable));
                                        else
                                            openDenied = GetLastError() == ERROR_ACCESS_DENIED;
                                    }
                                    ok = !openDenied && data->Criteria.TestTags(pathW.c_str());
                                }
                            }
                            if (ok && data->Grep)
                            {
                                // content
                                if (isDir)
                                    ok = FALSE; // a directory cannot be grepped
                                else
                                {
                                    strcpy_s(end, _countof(path) - (end - path), file.cFileName);
                                    // links: file.nFileSizeLow == 0 && file.nFileSizeHigh == 0, the file size
                                    // must be additionally obtained via SalGetFileSize()
                                    BOOL isLink = (file.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
                                    ok = TestFileContent(file.nFileSizeLow, file.nFileSizeHigh, path, data, isLink,
                                                         canQueueFile ? &openDenied : nullptr);
                                }
                            }
                            if (openDenied && !protectedQueue.Add(data, path, masksGroup, FALSE, startPathLen, ignoreList, &file))
                            {
                                FIND_LOG_ITEM log{FLI_ERROR, GetErrorText(ERROR_ACCESS_DENIED), path};
                                SendMessage(data->HWindow, WM_USER_ADDLOG, reinterpret_cast<WPARAM>(&log), 0);
                            }

                            // if the item matches all criteria,
                            // add it to the list of found items
                            if (ok)
                            {
                                if (end - path > 3)
                                    *(end - 1) = 0;
                                else
                                    *end = 0;

                                AddFoundItem(path, file.cFileName, file.nFileSizeLow, file.nFileSizeHigh,
                                             file.dwFileAttributes, &file.ftLastWriteTime, isDir, data,
                                             duplicateCandidates);

                                if (end - path > 3)
                                    *(end - 1) = '\\';
                                else
                                    *end = 0;
                            }
                        }
                    }
                }
                if (isDir && includeSubDirs && !ignoreDir) // directory + not "." or ".."
                {
                    int l = (int)strlen(file.cFileName);

                    if ((end - path) + l + 1 /* 1 za backslash */ < _countof(path))
                    {
                        BOOL searchNow = TRUE;

                        if (dirStack != NULL)
                        {
                            // just store for later search
                            char* newFileName = new char[l + 1];
                            if (newFileName != NULL)
                            {
                                memmove(newFileName, file.cFileName, l + 1);
                                if (dirStackCount < dirStack->Count)
                                {
                                    // no need to assign an item - we have space
                                    dirStack->At(dirStackCount) = newFileName;
                                    dirStackCount++;
                                    searchNow = FALSE;
                                }
                                else
                                {
                                    // we must allocate a new item in the array
                                    dirStack->Add(newFileName);
                                    if (dirStack->IsGood())
                                    {
                                        dirStackCount++;
                                        searchNow = FALSE;
                                    }
                                    else
                                    {
                                        dirStack->ResetState();
                                        delete[] newFileName;
                                    }
                                }
                            }
                        }

                        if (searchNow)
                        {
                            // out of memory - we will not use dirStack
                            strcpy_s(end, _countof(path) - (end - path), file.cFileName);
                            strcat_s(end, _countof(path) - (end - path), "\\");
                            l++;
                            SearchDirectory(path, end + l, startPathLen, masksGroup, includeSubDirs, data, NULL,
                                            0, duplicateCandidates, ignoreList, message, protectedQueue);
                        }
                    }
                    else
                    {
                        FIND_LOG_ITEM log;
                        log.Flags = FLI_ERROR;
                        log.Text = LoadStr(IDS_TOOLONGNAME);
                        strcpy_s(end, _countof(path) - (end - path), file.cFileName);
                        log.Path = path;
                        SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
                    }
                }
            }
            else // too long file-name
            {
                FIND_LOG_ITEM log;
                log.Flags = FLI_ERROR;
                log.Text = LoadStr(IDS_TOOLONGNAME);
                *end = 0;
                strcpy_s(message, path);
                strcat_s(message, file.cFileName);
                log.Path = message;
                SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
            }
            if (data->StopSearch)
            {
                testFindNextErr = FALSE;
                break;
            }
        } while (FindNextFile(find, &file));
        DWORD err = GetLastError();
        HANDLES(FindClose(find));

        if (testFindNextErr && err != ERROR_NO_MORE_FILES)
        {
            if (end - path > 3)
                *(end - 1) = 0;
            else
                *end = 0;

            const auto errorText = FormatFindError(IDS_DIRERRORFORMAT, err);
            FIND_LOG_ITEM log{FLI_ERROR, errorText.c_str(), path};
            SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);

            if (end - path > 3)
                *(end - 1) = '\\';
            else
                *end = 0;
        }

        // search through directories
        if (dirStack != NULL)
        {
            int i;
            for (i = dirStackEnterCount; i < dirStackCount; i++)
            {
                char* newFileName = (char*)dirStack->At(i);
                if (!data->StopSearch) // may be set during SearchDirectory
                {
                    strcpy_s(end, _countof(path) - (end - path), newFileName);
                    strcat_s(end, _countof(path) - (end - path), "\\");
                    SearchDirectory(path, end + strlen(end), startPathLen, masksGroup, includeSubDirs, data,
                                    dirStack, dirStackCount, duplicateCandidates, ignoreList, message, protectedQueue);
                }
            }
            // and release data from this level
            for (i = dirStackEnterCount; i < dirStackCount; i++)
                delete[] dirStack->At(i);
        }
    }
    else
    {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES)
        {
            if (end - path > 3)
                *(end - 1) = 0;
            else
                *end = 0;

            if (err != ERROR_ACCESS_DENIED ||
                !protectedQueue.Add(data, path, masksGroup, includeSubDirs, startPathLen, ignoreList))
            {
                const auto errorText = FormatFindError(IDS_DIRERRORFORMAT, err);
                FIND_LOG_ITEM log{FLI_ERROR | FLI_IGNORE, errorText.c_str(), path};
                SendMessage(data->HWindow, WM_USER_ADDLOG, (WPARAM)&log, 0);
            }

            if (end - path > 3)
                *(end - 1) = '\\';
            else
                *end = 0;
        }
    }
    *end = 0;
}

void RefineData(CMaskGroup* masksGroup, CGrepData* data)
{
    int refineCount = data->FoundFilesListView->GetDataForRefineCount();
    int oldProgress = -1;
    FindProtectedQueue protectedQueue;

    int i;
    for (i = 0; i < refineCount && !data->StopSearch; i++)
    {
        if (!data->Grep)
        {
            // if grepping is disabled, show progress in percent
            int progress = (int)((double)i / (double)refineCount * 100.0);
            if (progress != oldProgress)
            {
                char buf[20];
                sprintf(buf, "%d%%", progress);
                data->SearchingText->Set(buf); // set the current path
                oldProgress = progress;
            }
        }

        CFoundFilesData* refineData = data->FoundFilesListView->GetDataForRefine(i);

        // test the criteria
        BOOL ok = TRUE;

        // attributes, size, date, time
        if (ok && !data->Criteria.Test(refineData->Attr, &refineData->Size, &refineData->LastWrite))
            ok = FALSE;

        // file name (let the extension be resolved if ext==NULL)
        if (ok && !( !refineData->NameW.empty() ? masksGroup->AgreeMasksW(refineData->NameW.c_str(), NULL) : masksGroup->AgreeMasks(refineData->Name, NULL)))
            ok = FALSE;

        const bool canRequest = !refineData->IsDir && !(refineData->Attr & FILE_ATTRIBUTE_REPARSE_POINT);
        bool openDenied = false;
        std::wstring fullPath;
        if (ok && (data->Criteria.HasTags() || data->Grep) && !refineData->IsDir)
            fullPath = refineData->GetFullNameW();
        const auto apiPath = fullPath.empty() ? std::wstring{} : SalPathAddExtendedPrefixW(fullPath.c_str());
        if (ok && data->Criteria.HasTags())
        {
            if (refineData->IsDir)
                ok = FALSE;
            else
            {
                if (canRequest)
                {
                    HANDLE readable = HANDLES_Q(CreateFileW(apiPath.c_str(), FILE_READ_DATA,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
                    if (readable != INVALID_HANDLE_VALUE)
                        HANDLES(CloseHandle(readable));
                    else
                        openDenied = GetLastError() == ERROR_ACCESS_DENIED;
                }
                ok = !openDenied && data->Criteria.TestTags(fullPath.c_str());
            }
        }

        // content
        if (ok && data->Grep)
        {
            if (refineData->IsDir)
                ok = FALSE; // a directory cannot be grepped
            else
            {
                const auto display = SalWideToMultiBytePath(fullPath.c_str(), CP_UTF8);
                const BOOL isLink = (refineData->Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
                ok = TestFileContent(refineData->Size.LoDWord, refineData->Size.HiDWord,
                    display.c_str(), data, isLink, canRequest ? &openDenied : nullptr, apiPath.c_str());
            }
        }

        if (openDenied)
        {
            if (protectedQueue.AddRefined(data, fullPath, masksGroup, *refineData))
                continue;
            if (!data->StopSearch)
            {
                const auto display = SalWideToMultiBytePath(fullPath.c_str(), CP_UTF8);
                FIND_LOG_ITEM log{FLI_ERROR, GetErrorText(ERROR_ACCESS_DENIED), display.c_str()};
                SendMessage(data->HWindow, WM_USER_ADDLOG, reinterpret_cast<WPARAM>(&log), 0);
            }
        }
        if (!data->StopSearch && IncludeRefinedFindResult(data->Refine, ok != FALSE))
        {
            const bool wide = !refineData->PathW.empty() && !refineData->NameW.empty();
            AddFoundItem(refineData->Path, refineData->Name,
                         refineData->Size.LoDWord, refineData->Size.HiDWord,
                         refineData->Attr, &refineData->LastWrite, refineData->IsDir, data, nullptr,
                         wide ? refineData->PathW.c_str() : nullptr, wide ? refineData->NameW.c_str() : nullptr);
        }
    }
    RequestFindResultsRefresh(data);
    protectedQueue.Finish(nullptr);
}

unsigned GrepThreadFBody(void* ptr)
{
    CALL_STACK_MESSAGE1("GrepThreadFBody()");

    SetThreadNameInVCAndTrace("Grep");
    TRACE_I("Begin");
    //  Sleep(200);  // give the dialog a moment to redraw...
    CGrepData* data = (CGrepData*)ptr;
    HRESULT tagsCOM = data->Criteria.HasTags() ? CoInitializeEx(NULL, COINIT_MULTITHREADED) : S_FALSE;
    data->NeedRefresh = FALSE;
    data->Criteria.PrepareForTest();
    char path[MAX_PATH];
    char* end;
    if (data->Refine != 0)
    {
        if (data->Data->Count > 0)
        {
            CMaskGroup* mg = &data->Data->At(0)->MasksGroup;
            int errorPos;
            if (mg->PrepareMasks(errorPos))
            {
                RefineData(mg, data);
            }
            else
            {
                TRACE_E("PrepareMasks failed errorPos=" << errorPos);
                data->StopSearch = TRUE;
            }
        }
    }
    else
    {
        FindProtectedQueue protectedQueue;
        // if we search for duplicates, data are primarily placed into this array
        // after scanning all directories, the array is sorted (by name or by size)
        // if content is checked, MD5 is calculated for ambiguous cases
        // afterwards the data are passed to FoundFilesListView
        CDuplicateCandidates* duplicateCandidates = NULL;
        if (data->FindDuplicates)
        {
            duplicateCandidates = new CDuplicateCandidates;
            if (duplicateCandidates == NULL)
            {
                TRACE_E(LOW_MEMORY); // the algorithm will run even without the stack
                data->StopSearch = TRUE;
            }
        }

        if (!data->StopSearch)
        {
            int i;
            for (i = 0; i < data->Data->Count; i++)
            {
                strcpy_s(path, data->Data->At(i)->Dir);
                int len = (int)strlen(path);
                if (path[len - 1] != '\\')
                {
                    strcat_s(path, "\\");
                    len++;
                }
                end = path + len;
                CMaskGroup* mg = &data->Data->At(i)->MasksGroup;
                int errorPos;
                if (!mg->PrepareMasks(errorPos))
                {
                    TRACE_E("PrepareMasks failed errorPos=" << errorPos);
                    data->StopSearch = TRUE;
                    break;
                }

                BOOL includeSubDirs = data->Data->At(i)->IncludeSubDirs;
                TDirectArray<char*>* dirStack = NULL; // see description at SearchDirectory
                if (includeSubDirs)
                {
                    dirStack = new TDirectArray<char*>(1000, 1000);
                    if (dirStack == NULL)
                        TRACE_E(LOW_MEMORY); // the algorithm will run even without the stack
                }

                // create a local copy of the ignore list since it has to be processed anyway
                // and as a bonus the user can edit the ignore list while searching
                std::shared_ptr<CFindIgnore> ignoreList;
                try
                {
                    ignoreList = std::make_shared<CFindIgnore>();
                    if (!ignoreList->Prepare(&FindIgnore))
                        ignoreList.reset();
                }
                catch (const std::bad_alloc&)
                {
                    ignoreList.reset();
                    TRACE_E(LOW_MEMORY); // the algorithm will run even without the ignore list
                }

                char message[2 * MAX_PATH];
                SearchDirectory(path, end, (int)(end - path), mg, includeSubDirs, data, dirStack, 0,
                                duplicateCandidates, ignoreList, message, protectedQueue);

                if (dirStack != NULL)
                    delete dirStack;
                if (data->StopSearch)
                    break;
            }
        }
        RequestFindResultsRefresh(data); // publish ordinary results before waiting for consent
        const auto protectedSessions = protectedQueue.Finish(duplicateCandidates);
        if (duplicateCandidates != NULL)
        {
            if (!data->StopSearch)
                duplicateCandidates->Examine(data, protectedSessions, protectedQueue.Remaining());
            delete duplicateCandidates;
        }
    }

    data->SearchStopped = data->StopSearch;
    RequestFindResultsRefresh(data); // update the listview
    PostMessage(data->HWindow, WM_USER_FIND_COMPLETE, data->SearchGeneration, 0);
    if (data->Criteria.HasTags() && SUCCEEDED(tagsCOM))
        CoUninitialize();
    TRACE_I("End");
    return 0;
}

unsigned GrepThreadFEH(void* param)
{
#ifndef CALLSTK_DISABLE
    __try
    {
#endif // CALLSTK_DISABLE
        return GrepThreadFBody(param);
#ifndef CALLSTK_DISABLE
    }
    __except (CCallStack::HandleException(GetExceptionInformation()))
    {
        TRACE_I("Thread Grep: calling ExitProcess(1).");
        //    ExitProcess(1);
        TerminateProcess(GetCurrentProcess(), 1); // harder exit (this call still performs some operations)
        return 1;
    }
#endif // CALLSTK_DISABLE
}

DWORD WINAPI GrepThreadF(void* param)
{
#ifndef CALLSTK_DISABLE
    CCallStack stack;
#endif // CALLSTK_DISABLE
    return GrepThreadFEH(param);
}

//*********************************************************************************
//
// CSearchingString
//

CSearchingString::CSearchingString()
{
    HANDLES(InitializeCriticalSection(&Section));
    Buffer[0] = 0;
    BaseLen = 0;
    Dirty = FALSE;
}

CSearchingString::~CSearchingString()
{
    HANDLES(DeleteCriticalSection(&Section));
}

void CSearchingString::SetBase(const char* buf)
{
    HANDLES(EnterCriticalSection(&Section));
    lstrcpyn(Buffer, buf, MAX_PATH + 50);
    BaseLen = (int)strlen(Buffer);
    HANDLES(LeaveCriticalSection(&Section));
}

void CSearchingString::Set(const char* buf)
{
    HANDLES(EnterCriticalSection(&Section));
    lstrcpyn(Buffer + BaseLen, buf, MAX_PATH + 50 - BaseLen);
    Dirty = TRUE;
    HANDLES(LeaveCriticalSection(&Section));
}

void CSearchingString::Get(char* buf, int bufSize)
{
    HANDLES(EnterCriticalSection(&Section));
    lstrcpyn(buf, Buffer, bufSize);
    HANDLES(LeaveCriticalSection(&Section));
}

void CSearchingString::SetDirty(BOOL dirty)
{
    HANDLES(EnterCriticalSection(&Section));
    Dirty = dirty;
    HANDLES(LeaveCriticalSection(&Section));
}

BOOL CSearchingString::GetDirty()
{
    BOOL r;
    HANDLES(EnterCriticalSection(&Section));
    r = Dirty;
    HANDLES(LeaveCriticalSection(&Section));
    return r;
}

//*********************************************************************************
//
// Find Dialog Thread functions
//

struct CTFDData
{
    CFindDialog* FindDialog;
    BOOL Success;
};

unsigned ThreadFindDialogMessageLoopBody(void* parameter)
{
    CALL_STACK_MESSAGE1("ThreadFindDialogMessageLoopBody()");
    BOOL ok;

    { // this block ensures destructors are called properly before calling _end_thread() (see below)
        SetThreadNameInVCAndTrace("FindDialog");
        TRACE_I("Begin");
        CTFDData* data = (CTFDData*)parameter;
        CFindDialog* findDialog = data->FindDialog;
        findDialog->SetZeroOnDestroy(&findDialog); // on WM_DESTROY the pointer is zeroed
                                                   // protection against accessing an invalid pointer
                                                   // from the message loop after the window is destroyed

        data->Success = TRUE;

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

        if (findDialog->Create() != NULL)
        {
            SetForegroundWindow(findDialog->HWindow);
            UpdateWindow(findDialog->HWindow);
        }
        else
            data->Success = FALSE;

        ok = data->Success;
        data = NULL;                  // no longer valid afterwards
        SetEvent(FindDialogContinue); // let the main thread continue

        if (ok) // if the window was created, run the message loop
        {
            CALL_STACK_MESSAGE1("ThreadFindDialogMessageLoopBody::message_loop");

            MSG msg;
            HWND findDialogHWindow = findDialog->HWindow; // because of WM_QUIT, when the window will no longer be allocated
            BOOL haveMSG = FALSE;                         // FALSE means GetMessage() should be called in the loop condition
            while (haveMSG || GetMessage(&msg, NULL, 0, 0))
            {
                haveMSG = FALSE;
                if ((msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYDOWN) &&
                    msg.wParam != VK_MENU && msg.wParam != VK_CONTROL && msg.wParam != VK_SHIFT)
                    SetCurrentToolTip(NULL, 0); // turn off the tooltip
                // ensure messages reach our menu (avoids the need for a keyboard hook)
                if (findDialog == NULL || !findDialog->IsMenuBarMessage(&msg))
                {
                    if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE && findDialog != NULL)
                        findDialog->SetProcessingEscape(TRUE);
                    if (findDialog == NULL ||
                        (!TranslateAccelerator(findDialogHWindow, FindDialogAccelTable, &msg)) &&
                            (!findDialog->ManageHiddenShortcuts(&msg)) &&
                            (!IsDialogMessage(findDialogHWindow, &msg)))
                    {
                        TranslateMessage(&msg); // prevent generating WM_CHAR -> would cause a beep on Cancel
                        DispatchMessage(&msg);
                    }
                    if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE && findDialog != NULL)
                        findDialog->SetProcessingEscape(FALSE);
                }

                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        break;      // equivalent to the situation when GetMessage() is returning FALSE
                    haveMSG = TRUE; // a message is pending; process it without calling GetMessage()
                }
                else // if there is no message in the queue, perform Idle processing
                {
                    if (findDialog != NULL)
                        findDialog->OnEnterIdle();
                }
            }
        }

        TRACE_I("End");
    }

#ifndef CALLSTK_DISABLE
    CCallStack::ReleaseBeforeExitThread(); // before exiting the thread, we must release call-stack data (still in protected section - generating our bug report)
#endif                                     // CALLSTK_DISABLE
    _endthreadex(ok ? 0 : 1);
    return ok ? 0 : 1; // dead code to keep the compiler happy
}

unsigned ThreadFindDialogMessageLoopEH(void* param)
{
#ifndef CALLSTK_DISABLE
    __try
    {
#endif // CALLSTK_DISABLE
        return ThreadFindDialogMessageLoopBody(param);
#ifndef CALLSTK_DISABLE
    }
    __except (CCallStack::HandleException(GetExceptionInformation()))
    {
        TRACE_I("Thread FindDialogMessageLoop: calling ExitProcess(1).");
        //    ExitProcess(1);
        TerminateProcess(GetCurrentProcess(), 1); // harder exit (this call still performs some operations)
        return 1;
    }
#endif // CALLSTK_DISABLE
}

DWORD WINAPI ThreadFindDialogMessageLoop(void* param)
{
    CCallStack stack;
    return ThreadFindDialogMessageLoopEH(param);
}

BOOL OpenFindDialog(HWND hCenterAgainst, const char* initPath)
{
    CALL_STACK_MESSAGE3("OpenFindDialog(0x%p, %s)", hCenterAgainst, initPath);

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    CFindDialog* findDlg = new CFindDialog(hCenterAgainst, initPath);
    if (findDlg != NULL && findDlg->IsGood())
    {
        CTFDData data;
        data.FindDialog = findDlg;

        DWORD ThreadID;
        HANDLE loop = HANDLES(CreateThread(NULL, 0, ThreadFindDialogMessageLoop, &data, 0, &ThreadID));
        if (loop == NULL)
        {
            TRACE_E("Unable to start ThreadFindDialogMessageLoop thread.");
            goto ERROR_TFD_CREATE;
        }

        WaitForSingleObject(FindDialogContinue, INFINITE); // wait until the thread starts
        if (!data.Success)
        {
            HANDLES(CloseHandle(loop));
            goto ERROR_TFD_CREATE;
        }
        AddAuxThread(loop); // add the thread among existing viewers (killed on exit)
        SetCursor(hOldCur);
        return TRUE;
    }
    else
    {
        TRACE_E(LOW_MEMORY);

    ERROR_TFD_CREATE:

        if (findDlg != NULL)
            delete findDlg;

        SetCursor(hOldCur);
        return FALSE;
    }
    SetCursor(hOldCur);
    return TRUE;
}
