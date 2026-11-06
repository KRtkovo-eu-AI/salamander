// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <string>

namespace
{
bool GetManualEditLineUtf8(HWND edit, int lineIndex, char* buffer, int bufferSize)
{
    if (buffer == NULL || bufferSize <= 0)
        return false;
    buffer[0] = 0;

    if (edit == NULL)
        return false;

    if (!IsWindowUnicode(edit))
    {
        int charIndex = (int)SendMessage(edit, EM_LINEINDEX, lineIndex, 0);
        if (charIndex < 0)
            return false;
        int lineLen = (int)SendMessage(edit, EM_LINELENGTH, charIndex, 0);
        if (bufferSize < (int)sizeof(WORD) || lineLen >= bufferSize || lineLen > USHRT_MAX)
            return false;
        *LPWORD(buffer) = (WORD)min(bufferSize - 1, USHRT_MAX);
        int copied = (int)SendMessage(edit, EM_GETLINE, lineIndex, (LPARAM)buffer);
        buffer[copied] = 0;
        return true;
    }

    int textLen = GetWindowTextLengthW(edit);
    if (textLen <= 0)
        return lineIndex == 0;

    std::wstring text(textLen + 1, L'\0');
    GetWindowTextW(edit, &text[0], textLen + 1);
    text.resize(textLen);

    size_t start = 0;
    for (int line = 0; line < lineIndex; line++)
    {
        start = text.find(L'\n', start);
        if (start == std::wstring::npos)
            return false;
        start++;
    }
    size_t end = text.find(L'\n', start);
    if (end == std::wstring::npos)
        end = text.length();
    if (end > start && text[end - 1] == L'\r')
        end--;

    std::wstring line = text.substr(start, end - start);
    int required = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.length(), NULL, 0, NULL, NULL);
    if (required >= bufferSize)
        return false;
    int written = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.length(), buffer, bufferSize - 1, NULL, NULL);
    if (written < 0)
        written = 0;
    buffer[written] = 0;
    return true;
}
} // namespace

// ****************************************************************************
//
// CRenameScriptEntry
//

struct CRenameScriptEntry
{
    CSourceFile* Source;
    char* NewName;
    char* NewPart;
    int Blocks : 28;            // script construction: index of an item depending on this one
                                // script processing: the following item depends on this entry
    unsigned int Overwrite : 1; // the user confirmed overwriting the existing file

    // helper variables for building the script
    unsigned int Skip : 1;    // will not be renamed
    unsigned int Done : 1;    // the item has already been added to the script
    unsigned int Blocked : 1; // the item depends on another item

    CRenameScriptEntry()
    {
        Source = NULL;
        NewName = NULL;
        Blocks = -1;
        Overwrite = 0;
        Skip = 0;
        Done = 0;
        Blocked = 0;
    }
    ~CRenameScriptEntry()
    {
        if (NewName)
            free(NewName);
    }
    static int __cdecl CompareOldNames(const void* elem1, const void* elem2)
    {
        return RenamerPaths::Compare(
            ((CRenameScriptEntry*)elem1)->Source->FullName,
            ((CRenameScriptEntry*)elem2)->Source->FullName);
    }
    static int __cdecl CompareOldName(const void* key, const void* elem2)
    {
        return RenamerPaths::Compare((const char*)key, ((CRenameScriptEntry*)elem2)->Source->FullName);
    }
    static int __cdecl CompareNewNames(const void* elem1, const void* elem2)
    {
        if (((CRenameScriptEntry*)elem1)->NewName && ((CRenameScriptEntry*)elem2)->NewName)
        {
            return RenamerPaths::Compare(
                ((CRenameScriptEntry*)elem1)->NewName,
                ((CRenameScriptEntry*)elem2)->NewName);
        }
        else
        {
            if (((CRenameScriptEntry*)elem1)->NewName)
                return 1;
            if (((CRenameScriptEntry*)elem2)->NewName)
                return -1;
            return 0;
        }
    }
};

// ****************************************************************************
//
// CUndoStackEntry
//

CUndoStackEntry::CUndoStackEntry(char* source, char* target, CSourceFile* renamedFile,
                                 BOOL isDir, BOOL blocks)
{
    CALL_STACK_MESSAGE_NONE
    Source = SG->DupStr(source);
    if (target)
        Target = SG->DupStr(target);
    else
        Target = NULL;
    RenamedFile = renamedFile;
    IsDir = isDir ? 1 : 0;
    Blocks = blocks ? 1 : 0;
    Independent = 0;
}

CUndoStackEntry::~CUndoStackEntry()
{
    CALL_STACK_MESSAGE_NONE
    if (Source)
        free(Source);
    if (Target)
        free(Target);
}

// ****************************************************************************
//
// CRenamerDialog
//

void CRenamerDialog::Rename(BOOL validate)
{
    CALL_STACK_MESSAGE2("CRenamerDialog::Rename(%d)", validate);
    Progress = NULL;

    CRenameScriptEntry* script;
    int count;
    BOOL somethingToDo;
    if (BuildScript(script, count, validate, somethingToDo))
    {
        if (validate || !somethingToDo)
        {
            if (Progress)
            {
                EnableWindow(HWindow, TRUE);
                DestroyWindow(Progress->HWindow);
            }
            if (validate)
            {
                SG->SalMessageBox(HWindow, LoadStr(IDS_VALIDATEOK),
                                  LoadStr(IDS_PLUGINNAME), MB_ICONINFORMATION);
            }
            else
            {
                delete[] script;
                SG->SalMessageBox(HWindow, LoadStr(IDS_NOTTODO),
                                  LoadStr(IDS_PLUGINNAME), MB_ICONINFORMATION);
            }
            return;
        }

        Errors = FALSE;
        // always clear the undo stack
        // if (ProcessRenamed)
        // {
        RenamedFiles.DestroyMembers();
        UndoStack.DestroyMembers();
        // }

        ExecuteScript(script, count);

        delete[] script;

        NotRenamedFiles.DestroyMembers();
        if (Errors || count < SourceFiles.Count)
        {
            SG->SalMessageBox(HWindow, LoadStr(IDS_SOMEERRORS),
                              LoadStr(IDS_PLUGINNAME), MB_ICONINFORMATION);
            int i;
            for (i = 0; i < SourceFiles.Count; i++)
                if (SourceFiles[i]->State == 0)
                {
                    CSourceFile* copy = NewSourceFile(SourceFiles[i]);
                    if (copy == NULL)
                    {
                        Error(IDS_LOWMEM);
                        break;
                    }
                    NotRenamedFiles.Add(copy);
                    if (!NotRenamedFiles.IsGood())
                    {
                        delete copy;
                        NotRenamedFiles.ResetState();
                        Error(IDS_LOWMEM);
                        break;
                    }
                }

            ProcessRenamed = FALSE;
            ProcessNotRenamed = TRUE;
        }
        else
        {
            ProcessRenamed = TRUE;
            ProcessNotRenamed = FALSE;
            strcpy(DefMask, "*.*");
            ResetOptions(TRUE);
        }

        if (RenamerOptions.Spec == rsFullPath)
        {
            Root[0] = 0;
            RootLen = 0;
        }

        ReloadSourceFiles();
    }

    if (Progress)
    {
        EnableWindow(HWindow, TRUE);
        DestroyWindow(Progress->HWindow);
    }

    if (MinBeepWhenDone && IsIconic(HWindow))
        MessageBeep(0);
}

BOOL CRenamerDialog::BuildScript(CRenameScriptEntry*& script, int& count,
                                 BOOL validate, BOOL& somethingToDo)
{
    CALL_STACK_MESSAGE4("CRenamerDialog::BuildScript(, %d, %d, %d)", count,
                        validate, somethingToDo);
    BOOL ret = FALSE;
    CRenameScriptEntry* tmpScript = NULL;
    script = NULL;
    TBuffer<char> nameStorage;
    if (!nameStorage.Reserve(RenamerPaths::Capacity))
    {
        Error(IDS_LOWMEM);
        return FALSE;
    }
    char* newName = nameStorage.Get();
    char* newPart;
    BOOL skip;
    BOOL skipAllLongNames = FALSE,
         skipAllBadNames = FALSE,
         skipAllDuplicateNames = FALSE,
         skipDifferentDirRoots = FALSE;
    SkipAllFileDir = FALSE;
    SkipAllDependingNames = FALSE;
    Silent = ::Silent;
    somethingToDo = FALSE;
    int i = 0;
    BOOL usrBreak = FALSE;

    // set the options
    CRenamer renamer(Root, RootLen);
    if (!ManualMode && !renamer.SetOptions(&RenamerOptions))
    {
        int error, errorPos1, errorPos2;
        CRenamerErrorType errorType;
        renamer.GetError(error, errorPos1, errorPos2, errorType);
        Error(error);
        int id;
        switch (errorType)
        {
        case retNewName:
            id = IDC_NEWNAME;
            break;
        case retBMSearch:
            id = IDC_SEARCH;
            break;
        case retRegExp:
            id = IDC_SEARCH;
            break;
        case retReplacePattern:
            id = IDC_REPLACE;
            break;
        default:
            id = -1;
            break;
        }
        if (id != -1)
        {
            HWND ctrl = GetDlgItem(HWindow, id);
            HWND wnd = GetFocus();
            while (wnd != NULL && wnd != ctrl)
                wnd = GetParent(wnd);
            if (wnd == NULL) // focus only if the control is not an ancestor of GetFocus
            {                // such as the edit line in a combo box
                SendMessage(HWindow, WM_NEXTDLGCTL, (WPARAM)ctrl, TRUE);
            }
            SendMessage(ctrl, CB_SETEDITSEL, 0, MAKELONG(errorPos1, errorPos2));
        }
        goto LBUILD_SCRIPT_ERROR;
    }

    // create the progress dialog
    Progress = new CProgressDialog(HWindow);
    Progress->Create();
    EnableWindow(HWindow, FALSE);
    Progress->SetText(LoadStr(IDS_PREPARING));
    Progress->EmptyMessageLoop();

    // create the helper script
    tmpScript = new CRenameScriptEntry[SourceFiles.Count];
    for (i = 0; i < SourceFiles.Count; i++)
    {
        tmpScript[i].Source = SourceFiles[i];
        // create a new name
        int l = ManualMode ? GetManualModeNewName(SourceFiles[i], i, newName, RenamerPaths::Capacity, newPart) : renamer.Rename(SourceFiles[i], i, newName, RenamerPaths::Capacity, &newPart);
        if (l < 0)
        {
            FileError(HWindow, SourceFiles[i]->FullName, IDS_EXP_SMALLBUFFER,
                      FALSE, &skip, &skipAllLongNames, IDS_ERROR);
            if (!skip)
                goto LBUILD_SCRIPT_ERROR;
            tmpScript[i].Skip = 1;
            continue;
        }
        // verify the correctness of the name
        if (!ValidateFileName(newPart, l, RenamerOptions.Spec, &skip, &skipAllBadNames))
        {
            if (!skip)
                goto LBUILD_SCRIPT_ERROR;
            tmpScript[i].Skip = 1;
            continue;
        }
        CutTrailingDots(newPart, l, RenamerOptions.Spec);
        tmpScript[i].NewName = SG->DupStr(newName);
        tmpScript[i].NewPart = tmpScript[i].NewName + (newPart - newName);
        somethingToDo = somethingToDo || strcmp(SourceFiles[i]->FullName, newName);
    }

    // remove duplicate names
    qsort(tmpScript, SourceFiles.Count, sizeof(*tmpScript), CRenameScriptEntry::CompareNewNames);
    for (i = 1; i < SourceFiles.Count; i++)
        if (!tmpScript[i - 1].Skip && !tmpScript[i].Skip &&
            RenamerPaths::Compare(tmpScript[i - 1].NewName, tmpScript[i].NewName) == 0)
        {
            FileError(HWindow, tmpScript[i].NewName, IDS_DUPLICATENAME,
                      FALSE, &skip, &skipAllDuplicateNames, IDS_ERROR);
            if (!skip)
                goto LBUILD_SCRIPT_ERROR;
            tmpScript[i - 1].Skip = 1;
            do
                tmpScript[i].Skip = 1;
            while (++i < SourceFiles.Count &&
                   RenamerPaths::Compare(tmpScript[i - 1].NewName, tmpScript[i].NewName) == 0);
            continue;
        }

    // sort the script by the original name (for binary search)
    qsort(tmpScript, SourceFiles.Count, sizeof(*tmpScript), CRenameScriptEntry::CompareOldNames);

    // test the helper script for correctness
    for (i = 0; i < SourceFiles.Count; i++)
    {
        if (!tmpScript[i].Skip)
        {
            // verify that directories share the same root (we cannot handle recursive directory copies)
            if (tmpScript[i].Source->IsDir &&
                !RenamerIO::SameRoot(tmpScript[i].Source->FullName, tmpScript[i].NewName))
            {
                FileError(HWindow, tmpScript[i].Source->FullName, IDS_DIRNOTSAMEROOT,
                          FALSE, &skip, &skipDifferentDirRoots, IDS_ERROR);
                if (!skip)
                    goto LBUILD_SCRIPT_ERROR;
                tmpScript[i].Skip = 1;
                continue;
            }
            // verify that target names do not exist and request overwrite confirmation
            DWORD attr = RenamerIO::Attributes(tmpScript[i].NewName);
            if (attr != 0xFFFFFFFF)
            {
                if ((attr & FILE_ATTRIBUTE_DIRECTORY) ||
                    !(Silent & (SILENT_OVERWRITE_FILE_EXIST | SILENT_SKIP_FILE_EXIST)) ||
                    (attr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) &&
                        !(Silent & (SILENT_OVERWRITE_FILE_SYSHID | SILENT_SKIP_FILE_SYSHID)))
                {
                    // the new name is not among the files that will be renamed
                    if (!bsearch(tmpScript[i].NewName, tmpScript, SourceFiles.Count,
                                 sizeof(*tmpScript), CRenameScriptEntry::CompareOldName))
                    {
                        if (attr & FILE_ATTRIBUTE_DIRECTORY) // cannot overwrite a directory
                        {
                            FileError(HWindow, tmpScript[i].Source->FullName,
                                      tmpScript[i].Source->IsDir ? IDS_DIRDIR : IDS_FILEDIR,
                                      FALSE, &skip, &SkipAllFileDir, IDS_ERROR);
                            if (!skip)
                                goto LBUILD_SCRIPT_ERROR;
                            tmpScript[i].Skip = 1;
                            continue;
                        }
                        if (!FileOverwrite(HWindow, tmpScript[i].NewName, NULL,
                                           tmpScript[i].Source->FullName, NULL, attr,
                                           IDS_CNFRM_SHOVERWRITE, IDS_OVEWWRITETITLE, &skip, &Silent))
                        {
                            if (!skip)
                                goto LBUILD_SCRIPT_ERROR;
                            tmpScript[i].Skip = 1;
                            continue;
                        }
                        tmpScript[i].Overwrite = 1;
                    }
                }
                else
                {
                    if (Silent & SILENT_SKIP_FILE_EXIST)
                    {
                        tmpScript[i].Skip = 1;
                        continue;
                    }
                    if ((attr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) &&
                        (Silent & SILENT_SKIP_FILE_SYSHID))
                    {
                        tmpScript[i].Skip = 1;
                        continue;
                    }
                    tmpScript[i].Overwrite = 1;
                }
            }
        }
        if ((usrBreak = Progress->Update(min((i + 1) * 1000 / SourceFiles.Count, 990))) != 0)
            goto LBUILD_SCRIPT_ERROR;
    }

    // find the optimal order for performing the rename operation
    // and create the script according to which we will rename
    if (!validate)
        script = new CRenameScriptEntry[SourceFiles.Count];
    count = 0;
    int skipped;
    skipped = 0;
    for (i = 0; i < SourceFiles.Count; i++) // detect dependency chains
    {
        if (tmpScript[i].Skip)
            skipped++;
        else
        {
            CRenameScriptEntry* blocker = (CRenameScriptEntry*)bsearch(
                tmpScript[i].NewName, tmpScript, SourceFiles.Count, sizeof(*tmpScript),
                CRenameScriptEntry::CompareOldName);
            if (blocker && blocker != tmpScript + i)
            {
                blocker->Blocks = i;
                tmpScript[i].Blocked = 1;
            }
        }
    }
    for (i = 0; i < SourceFiles.Count; i++) // build the script
    {
        // find the start of the dependency chain and add its members
        // to the script
        if (!tmpScript[i].Skip && !tmpScript[i].Done && !tmpScript[i].Blocked)
        {
            int prev = i;
            do
            {
                if (!validate)
                {
                    script[count].Source = tmpScript[prev].Source;
                    script[count].NewName = tmpScript[prev].NewName;
                    script[count].NewPart = tmpScript[prev].NewPart;
                    script[count].Blocks = tmpScript[prev].Blocks != -1;
                    script[count].Overwrite = tmpScript[prev].Overwrite;
                    tmpScript[prev].NewName = NULL;
                }
                tmpScript[prev].Done = 1;
                count++;
                prev = tmpScript[prev].Blocks;
            } while (prev != -1);
        }
    }

    // if there are items that cannot be renamed, display them
    if (count < SourceFiles.Count - skipped)
    {
        for (i = 0; i < SourceFiles.Count; i++)
        {
            if (!tmpScript[i].Skip && !tmpScript[i].Done)
            {
                FileError(HWindow, tmpScript[i].Source->FullName, IDS_DEPENDENCE,
                          FALSE, &skip, &SkipAllDependingNames, IDS_ERROR);
                if (!skip)
                    goto LBUILD_SCRIPT_ERROR;
            }
        }
    }

    ret = TRUE; // if we got here, everything is OK
    Progress->Update(1000);

LBUILD_SCRIPT_ERROR:

    if (!ret && !usrBreak)
    {
        // focus the item with the error
        ListView_SetItemState(Preview->HWindow, i,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible(Preview->HWindow, i, FALSE);

        // position the cursor on the line with the error
        if (ManualMode)
        {
            int charIndex = (int)SendMessage(ManualEdit->HWindow, EM_LINEINDEX, i, 0);
            if (charIndex >= 0)
            {
                HWND wnd = GetFocus();
                if (wnd != ManualEdit->HWindow)
                {
                    if (Progress) // so the focus can be set to the edit control
                    {
                        EnableWindow(HWindow, TRUE);
                        DestroyWindow(Progress->HWindow);
                        Progress = NULL;
                    }

                    SendMessage(HWindow, WM_NEXTDLGCTL, (WPARAM)ManualEdit->HWindow, TRUE);
                }
                SendMessage(ManualEdit->HWindow, EM_SETSEL, charIndex, charIndex);
                SendMessage(ManualEdit->HWindow, EM_SCROLLCARET, 0, 0);
            }
        }
    }

    if (tmpScript)
        delete[] tmpScript;
    if (!ret && script)
        delete[] script;

    return ret;
}

int CRenamerDialog::GetManualModeNewName(CSourceFile* file, int index, char* newName, int capacity, char*& newPart)
{
    CALL_STACK_MESSAGE_NONE
    if (newName == NULL || capacity <= 0)
        return -1;
    int pathLen = 0;
    switch (RenamerOptions.Spec)
    {
    case rsFileName:
    {
        pathLen = (int)(file->Name - file->FullName);
        if (pathLen >= capacity)
            return -1;
        memcpy(newName, file->FullName, pathLen);
        break;
    }
    case rsRelativePath:
    {
        pathLen = RootLen;
        if (pathLen <= 0 || pathLen >= capacity - 1)
            return -1;
        memcpy(newName, Root, pathLen);
        if (newName[pathLen - 1] != '\\')
            newName[pathLen++] = '\\';
        break;
    }
    case rsFullPath:
        break;
    }
    newName += pathLen;
    newPart = newName;

    int charIndex = (int)SendMessage(ManualEdit->HWindow, EM_LINEINDEX, index, 0);
    if (charIndex < 0 && !IsWindowUnicode(ManualEdit->HWindow)) // this should not happen -- CRenamerDialog::Validate would fail
    {
        *newName = 0;
        return 0;
    }
    else
    {
        if (!GetManualEditLineUtf8(ManualEdit->HWindow, index, newName, capacity - pathLen))
            return -1;
        return (int)strlen(newName);
    }
}

void CRenamerDialog::ExecuteScript(CRenameScriptEntry* script, int count)
{
    CALL_STACK_MESSAGE2("CRenamerDialog::ExecuteScript(, %d)", count);
    BOOL skip;
    SkipAllOverwrite = SkipAllMove = SkipAllDeleteErr = SkipAllBadWrite =
        SkipAllBadRead = SkipAllOpenOut = SkipAllOpenIn =
            SkipAllDirChangeCase = SkipAllCreateDir = FALSE;

    Progress->SetText(LoadStr(IDS_RENAMING));

    // perform the rename
    BOOL blocked = FALSE;
    BOOL success = TRUE;
    int i;
    for (i = 0; i < count; i++)
    {
        // Reserve the model and cleanup scratch before changing the filesystem.
        CSourceFile* f = NewSourceFile(script[i].Source, script[i].NewName);
        TBuffer<char> directory;
        if (f == NULL || (RemoveSourcePath &&
                          !directory.Reserve(strlen(script[i].Source->FullName) + 1)))
        {
            delete f;
            Error(IDS_LOWMEM);
            Errors = TRUE;
            return;
        }
        if (success || !blocked)
        {
            success = MoveFile(script[i].Source->FullName, script[i].NewName, script[i].NewPart,
                               script[i].Overwrite, script[i].Source->IsDir, skip);
        }
        else
        {
            FileError(HWindow, script[i].Source->FullName, IDS_DEPENDENCE,
                      FALSE, &skip, &SkipAllDependingNames, IDS_ERROR);
        }
        if (success)
        {
            if (RemoveSourcePath)
            {
                char* dir = directory.Get();
                strcpy(dir, script[i].Source->FullName);
                do
                {
                    SG->CutDirectory(dir);
                    RenamerIO::ClearReadOnly(dir); // so it can be deleted
                } while (RenamerIO::RemoveDir(dir));
            }
            script[i].Source->State = 1;
            RenamedFiles.Add(f);
            UndoStack.Add(new CUndoStackEntry(script[i].NewName, script[i].Source->FullName,
                                              f, script[i].Source->IsDir, blocked));
        }
        else
        {
            delete f;
            Errors = TRUE;
            if (!skip)
                return;
        }
        blocked = script[i].Blocks;
        if (Progress->Update((i + 1) * 1000 / count))
        {
            Errors = TRUE;
            break;
        }
    }
}

void CRenamerDialog::Undo()
{
    CALL_STACK_MESSAGE1("CRenamerDialog::Undo()");
    Undoing = TRUE;
    SkipAllOverwrite = SkipAllMove = SkipAllDeleteErr = SkipAllBadWrite =
        SkipAllBadRead = SkipAllOpenOut = SkipAllOpenIn =
            SkipAllDirChangeCase = SkipAllCreateDir = SkipAllFileDir =
                SkipAllDependingNames = SkipAllRemoveDir = FALSE;

    // create the progress dialog
    Progress = new CProgressDialog(HWindow);
    Progress->Create();
    EnableWindow(HWindow, FALSE);
    Progress->SetText(LoadStr(IDS_UNDOING));
    Progress->EmptyMessageLoop();

    int total = UndoStack.Count;
    int done = 0;
    BOOL skip;
    BOOL blocked = FALSE;
    BOOL success = TRUE;
    BOOL pathSuccess = TRUE;
    int i;
    for (i = UndoStack.Count - 1; i >= 0; i--, done++)
    {
        CUndoStackEntry* entry = UndoStack[i];
        const BOOL entryBlocks = entry->Blocks;
        if (entry->RenamedFile)
        {
            if (Progress->Update(done * 1000 / total))
                break;

            // undo move file
            if (success || !blocked)
            {
                CSourceFile* restored = NewSourceFile(entry->RenamedFile, entry->Target);
                if (restored == NULL)
                {
                    Error(IDS_LOWMEM);
                    goto LUNDONE;
                }
                success = MoveFile(entry->Source, entry->Target, entry->Target,
                                   FALSE, entry->IsDir, skip);
                if (success)
                    entry->RenamedFile->SwapName(*restored);
                delete restored;
            }
            else
            {
                FileError(HWindow, entry->Source, IDS_DEPENDENCE,
                          FALSE, &skip, &SkipAllDependingNames, IDS_ERROR);
            }
            if (success)
            {
                int j;
                for (j = RenamedFiles.Count - 1; j >= 0; j--)
                {
                    if (RenamedFiles[j] == entry->RenamedFile)
                    {
                        RenamedFiles.Detach(j);
                        NotRenamedFiles.Add(entry->RenamedFile);
                        break;
                    }
                }
                UndoStack.Delete(i);
            }
            else
            {
                if (!skip)
                    goto LUNDONE;
            }
            blocked = entryBlocks;
            pathSuccess = success;
        }
        else
        {
            if (Progress->Update(done * 1000 / total))
            {
                if (pathSuccess)
                    entry->Independent = 1; // to allow continuing later
                break;
            }

            // clean up the path only when the file was successfully removed
            if (pathSuccess || entry->Independent)
            {
                if (entry->Target)
                {
                    // undo change directory case
                    while (1)
                    {
                        pathSuccess = RenamerIO::Move(entry->Source, entry->Target, NULL);
                        if (pathSuccess)
                        {
                            UndoStack.Delete(i);
                            break;
                        }

                        if (!FileError(HWindow, entry->Source, IDS_DIRCASEERROR,
                                       TRUE, &skip, &SkipAllDirChangeCase, IDS_ERROR))
                        {
                            entry->Independent = 1; // to allow continuing later
                            if (!skip)
                                goto LUNDONE;
                            break;
                        }
                    }
                }
                else
                {
                    // undo create directory
                    while (1)
                    {
                        pathSuccess = RenamerIO::RemoveDir(entry->Source);
                        if (pathSuccess)
                        {
                            UndoStack.Delete(i);
                            break;
                        }

                        if (!FileError(HWindow, entry->Source, IDS_REMOVEDIR,
                                       TRUE, &skip, &SkipAllRemoveDir, IDS_ERROR))
                        {
                            entry->Independent = 1; // to allow continuing later
                            if (!skip)
                                goto LUNDONE;
                            break;
                        }
                    }
                }
            }
        }
    }

    Progress->Update(done * 1000 / total);

LUNDONE:

    EnableWindow(HWindow, TRUE);
    DestroyWindow(Progress->HWindow);
    Undoing = FALSE;

    if (NotRenamedFiles.Count)
    {
        ProcessRenamed = FALSE;
        ProcessNotRenamed = TRUE;
    }
    ReloadSourceFiles();
}
