# SPDX-License-Identifier: GPL-2.0-or-later
"""Integration guards for Branch View lifetime, identity and navigation.

These checks protect boundaries that the standalone scanner cannot execute.
They complement branch_view_tests.cpp and manual panel tests; they are not a
substitute for running the Win32 application.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


def source(name):
    return (ROOT / name).read_text(encoding="utf-8-sig")


def body(name, signature):
    text = source(name)
    start = text.index(signature)
    end = re.search(r"^}\s*$", text[start:], re.MULTILINE)
    if end is None:
        raise AssertionError("Missing function end: " + signature)
    return text[start:start + end.end()]


class BranchViewContracts(unittest.TestCase):
    def assertIn(self, member, container, msg=None):
        super().assertTrue(member in container, msg or "Missing contract: " + str(member))

    def assertNotIn(self, member, container, msg=None):
        super().assertFalse(member in container, msg or "Forbidden contract: " + str(member))

    def ordered(self, text, *fragments):
        cursor = 0
        for fragment in fragments:
            position = text.find(fragment, cursor)
            self.assertGreaterEqual(position, 0, "Missing or out of order: " + fragment)
            cursor = position + len(fragment)

    def test_cancelled_worker_never_owns_panel_or_window(self):
        worker = source("branch_view.cpp").split("#ifndef BRANCH_VIEW_TEST\n#include \"plugins.h\"")[0]
        self.assertIn("std::shared_ptr<State> state", worker)
        for forbidden in ("CFilesWindow*", "HWND", "PostMessage", "SendMessage", "TerminateThread", "WaitForSingleObject"):
            self.assertNotIn(forbidden, worker)
        cancel = body("branch_view.cpp", "void Scan::Cancel()")
        self.ordered(cancel, "InterlockedExchange(&Shared->Cancelled, 1)", "CancelSynchronousIo(Thread)", "CloseHandle(Thread)")
        start = body("branch_view.cpp", "bool Scan::Start(")
        self.ordered(start, "Cancel();", "Shared = std::make_shared<State>()", "CreateThread(")

    def test_metadata_outlives_icon_reader_and_old_listing(self):
        destroy = body("fileswn1.cpp", "CFilesWindow::~CFilesWindow()")
        self.ordered(destroy, "BranchView->Worker.Cancel();", "CloseHandle(IconCacheThread)", "ShutdownBranchView();")
        self.assertNotIn("CancelBranchViewScan();", destroy)
        self.assertNotIn("DirectoryLineSetText();", destroy)
        read = body("fileswn3.cpp", "BOOL CFilesWindow::ReadDirectory(")
        self.ordered(read, "SleepIconCacheThread();", "Files->DestroyMembers();", "BranchView->Items.clear();", "VisibleItemsArray.InvalidateArr();")
        refresh = body("fileswn0.cpp", "void CFilesWindow::RefreshDirectory(")
        self.ordered(refresh, "ReleaseListingBody(oldPanelType", "PruneBranchViewMetadata();")
        header = source("branch_view.h")
        self.assertIn("std::unordered_map<const char*, Salamander::BranchView::Entry> Items", header)
        self.assertIn("std::mutex ItemsMutex", header)

    def test_poll_respects_operation_suspend_and_stops_idle_timer(self):
        poll = body("branch_view.cpp", "void CFilesWindow::PollBranchView()")
        self.ordered(poll, "BranchView->Applying || StopRefresh || SnooperSuspended", "BranchView->Worker.Drain", "!Salamander::BranchView::SameSnapshot", "++BranchView->Generation", "BranchView->Applying = true", "RefreshDirectory();", "BranchView->Applying = false")
        self.assertIn("GetTickCount() - BranchView->LastPublish >= 1000", poll)
        self.assertIn("!BranchView->Status.Running && !BranchView->Preparing", poll)
        self.assertIn("!BranchView->Refresh.HasPending() && !BranchView->Dirty", poll)
        self.assertIn("KillTimer(HWindow, IDT_BRANCHVIEW_POLL)", poll)
        self.assertIn("PollBranchView();", source("fileswnb.cpp"))

    def test_refresh_does_not_synchronously_reopen_root(self):
        refresh = body("fileswn0.cpp", "void CFilesWindow::RefreshDirectory(")
        self.ordered(refresh, "if (IsBranchView() && !BranchView->Applying)", "RefreshBranchView();", "return;")
        self.assertRegex(refresh, r"if \(IsBranchView\(\)\)\s*\{\s*noChange = FALSE;\s*result = ReadDirectory\(HWindow, TRUE\);")
        schedule = body("branch_view.cpp", "void CFilesWindow::RefreshBranchView(")
        self.ordered(schedule, "BranchView->Refresh.Request(now, automatic != FALSE)", "BranchView->Refresh.Due(now)", "StartBranchViewScan(this)")
        dispatch = source("fileswnb.cpp")
        self.assertIn("const BOOL automatic = uMsg != WM_USER_REFRESH_DIR || wParam != 0", dispatch)
        self.assertIn("RefreshBranchView(automatic)", dispatch)

    def test_refresh_restores_duplicate_names_by_exact_wide_identity(self):
        refresh = body("fileswn0.cpp", "void CFilesWindow::RefreshDirectory(")
        self.assertIn("std::unordered_map<std::string, CBranchSavedState> branchSelection", refresh)
        self.assertIn("branchSelection[key == file.Name ? SalWideToMultiBytePath(GetItemIdentityW(file).c_str(), CP_UTF8) : std::string(key)]", refresh)
        self.assertIn("const char* identity = GetItemCacheKeyPtr(file)", refresh)
        self.assertIn("branchSelection.find(identity)", refresh)
        self.assertIn("branchRestoreSelected.find(identity)", refresh)
        self.assertIn("if (identity == branchFocus)", refresh)
        self.assertIn("if (!BranchView->Status.Running)", refresh)
        self.assertIn("BranchView->PendingFocus.clear()", refresh)

    def test_branch_refresh_avoids_repeated_large_ui_work(self):
        refresh = body("fileswn0.cpp", "void CFilesWindow::RefreshDirectory(")
        self.assertIn("if (!IsBranchView() && (SortType != stName", refresh)
        self.assertIn("BranchView->RestoreState != NULL || file.Selected || file.CutToClip", refresh)
        self.assertIn("branchSelection.empty() ? branchSelection.end()", refresh)
        self.ordered(refresh, "if (!IsBranchView())", "ordinary listings use the basename merge", "return to the user-selected sorting")
        sort = body("fileswn3.cpp", "void CFilesWindow::SortDirectory(")
        self.assertIn("CompareEntryPaths(first->second, second->second)", sort)
        self.assertIn("first->second.RelativePath < second->second.RelativePath", sort)
        self.assertNotIn("GetItemDirectoryW(a)", sort)
        self.assertNotIn("GetItemIdentityW(a)", sort)
        self.assertIn("Files->Reserve(", source("fileswn3.cpp"))
        self.assertIn("if (!IsBranchView())", sort)
        poll = body("branch_view.cpp", "void CFilesWindow::PollBranchView()")
        self.assertIn("InitialPreviewCount(", poll)
        self.assertIn("previewCount > BranchView->Entries.size()", poll)
        self.assertNotIn("Entries = BranchView->IncomingEntries", poll)
        self.assertNotIn("PruneBranchViewMetadata();", poll)
        prune = body("branch_view.cpp", "void CFilesWindow::PruneBranchViewMetadata()")
        self.ordered(prune, "ItemsMutex", "Items.extract", "live.insert(std::move(item))", "Items.swap(live)")
        names = body("sort.cpp", "int CompareWideFileNames(")
        self.assertIn("f1.UseWideName() ? f1.NameW", names)
        self.assertIn("f2.UseWideName() ? f2.NameW", names)
        self.assertIn("storage1 = FileSortNameW(f1)", names)
        self.assertIn("StrCmpLogicalWEx(n1, length1, n2, length2, ignoreCase)", names)

    def test_background_refresh_preserves_visible_snapshot_until_complete(self):
        start = body("branch_view.cpp", "static void StartBranchViewScan(")
        self.assertIn("state.PreserveListing = !state.Entries.empty()", start)
        self.ordered(start, "state.ForcePublish = state.Refresh.IsManual()", "state.Refresh.Started(")
        self.assertNotIn("Generation", start)
        self.assertNotIn("state.Entries.clear()", start)
        poll = body("branch_view.cpp", "void CFilesWindow::PollBranchView()")
        self.assertIn("finished || (!BranchView->PreserveListing", poll)
        self.assertIn("if (!finished || BranchView->ForcePublish ||", poll)
        self.ordered(poll, "!Salamander::BranchView::SameSnapshot", "BranchView->Entries.swap", "++BranchView->Generation", "RefreshDirectory();")
        cancel = body("branch_view.cpp", "void CFilesWindow::CancelBranchViewScan()")
        self.assertIn("BranchView->Refresh.Cancel()", cancel)
        self.assertIn("BranchView->IncomingEntries.clear()", cancel)
        self.assertNotIn("BranchView->Entries.clear()", cancel)
        self.ordered(poll, "if (BranchView->Status.Cancelled)", "return;", "BranchView->Worker.Drain")
        self.assertNotIn("FILE_NOTIFY_CHANGE_LAST_ACCESS", source("snooper.cpp"))

    def test_background_status_describes_retained_snapshot(self):
        status = body("branch_view.cpp", "std::wstring CFilesWindow::GetBranchViewStatusW() const")
        self.assertIn("BranchView->PreserveListing &&", status)
        self.assertIn("BranchView->Status.Running || BranchView->Status.Cancelled", status)
        self.assertIn("IDS_BRANCH_REFRESHING : IDS_BRANCH_REFRESH_STOPPED", status)
        self.assertIn("(retainingSnapshot || BranchView->Status.Cancelled) ? (ULONGLONG)BranchView->Entries.size() : BranchView->Status.Files", status)
        for name in ("IDS_BRANCH_REFRESHING", "IDS_BRANCH_REFRESH_STOPPED"):
            self.assertIn(name, source("texts.rh2"))
            self.assertIn(name, source("lang/texts.rc2"))
        self.assertIn("refresh stopped; previous results retained (%I64u files)", source("lang/texts.rc2"))

    def test_association_preparation_does_not_change_live_icon_bookkeeping(self):
        prepare = body("icncache.cpp", "void CAssociations::PrepareShellAssociation(")
        self.assertIn("extension.size() + sizeof(DWORD)", prepare)
        self.assertIn("QueryShellAssociationCached(padded.data(), canOpen, TRUE)", prepare)
        for mutation in ("InsertData", "SetPixelIconIndex", "SetIndex", "IsAssociated("):
            self.assertNotIn(mutation, prepare)
        release = body("icncache.cpp", "void CAssociations::Release()")
        self.ordered(release, "++ShellAssociationEpoch", "PreparedShellAssociations.clear()")
        cached = body("icncache.cpp", "BOOL CAssociations::QueryShellAssociationCached(")
        self.ordered(cached, "const ULONGLONG epoch", "QueryShellAssociation(ext", "epoch != ShellAssociationEpoch", "if (prepare) PreparedShellAssociations.emplace")
        associated = body("icncache.cpp", "BOOL CAssociations::IsAssociated(char* ext, BOOL&")
        self.ordered(associated, "QueryShellAssociationCached", "InsertData", "semanticIndex == -2", "SetPixelIconIndex(index, pixelSize, -3)")

    def test_preparation_finishes_before_next_scan_and_survives_shell_callbacks(self):
        poll = body("branch_view.cpp", "void CFilesWindow::PollBranchView()")
        self.ordered(poll, "lifetime->Busy", "preparation->Queue.RunSlice", "if (!preparation->Alive) return", "preparation->Queue.Restart()", "const bool finished = BranchView->CompletionPending", "RefreshDirectory();", "BranchView->Refresh.Finished", "BranchView->Refresh.Due")
        self.assertIn("preparation->Queue.HasPending() ? 15 : 200", poll)
        self.assertNotIn("DispatchMessage", poll)
        self.assertNotIn("PeekMessage", poll)
        self.assertIn("BranchView->CompletionPending = true", poll)
        self.assertIn("BranchView->Dirty && !preparation->Queue.HasPending()", poll)
        cancel = body("branch_view.cpp", "void CFilesWindow::CancelBranchViewScan()")
        self.ordered(cancel, "Preparation->Alive = false", "Preparation->Queue.Clear()", "Preparing = false", "CompletionPending = false")
        shutdown = body("branch_view.cpp", "void CFilesWindow::ShutdownBranchView()")
        self.ordered(shutdown, "PreparationLifetime->Alive = false", "Preparation->Alive = false", "delete BranchView")
        self.ordered(poll, "RefreshDirectory();", "if (!lifetime->Alive) return", "BranchView->Applying = false", "if (!preparation->Alive) return")

    def test_subtree_watcher_has_distinct_registration(self):
        text = source("snooper.cpp")
        self.assertIn('recursive ? "|recursive" : "|directory"', text)
        self.assertIn("FindFirstChangeNotificationW(prepared.WidePath.c_str(), prepared.Recursive", text)
        self.assertIn("PrepareWatchPath(path, win->IsBranchView())", text)
        self.assertIn("PrepareWatchPath(newPath, win->IsBranchView())", text)

    def test_failed_navigation_does_not_leave_mode(self):
        close = body("fileswn2.cpp", "void CFilesWindow::CloseCurrentPath(")
        self.assertIn("if (!cancel && (!newPathIsTheSame || !isRefresh)) LeaveBranchView();", close)
        leave = body("branch_view.cpp", "void CFilesWindow::LeaveBranchView()")
        self.ordered(leave, "CancelBranchViewScan();", "BranchView->Enabled = false", "BranchView->OriginalSort", "BuildColumnsTemplate();", "KillTimer(HWindow, IDT_BRANCHVIEW_POLL)")
        self.assertNotIn("Items.clear()", leave)
        self.assertNotIn("SelectViewTemplate(", leave)
        toggle = body("branch_view.cpp", "void CFilesWindow::ToggleBranchView()")
        self.assertNotIn("SelectViewTemplate(", toggle) # S28 preserves the user-selected layout.
        self.ordered(toggle, "if (!Is(ptDisk)) return;", "BranchView->OriginalViewTemplate = GetViewTemplateIndex()", "BranchView->Enabled = true", "BuildColumnsTemplate();", "EnsureWatching(this, FALSE);", "RefreshBranchView();")

    def test_tab_state_captured_before_creation_restored_before_activation(self):
        duplicate = body("mainwnd3.cpp", "CFilesWindow* CMainWindow::CreateDuplicatePanelTab(")
        self.ordered(duplicate, "sourcePanel->CaptureBranchViewState()", "AddPanelTab(", "ChangePathToDiskW(", "sourceDiskPath == newPanel->GetPathW()", "RestoreBranchViewState(sourceBranchView)", "SwitchPanelTab(newPanel)")
        closed = body("mainwnd3.cpp", "void CMainWindow::RememberClosedTab(")
        self.assertIn("info.BranchState = panel->CaptureBranchViewState()", closed)
        self.assertIn("info.DiskPathW", closed)
        reopen = body("mainwnd3.cpp", "bool CMainWindow::CommandReopenClosedTab(")
        self.ordered(reopen, "ChangePathToDiskW(", "entry.DiskPathW == panel->GetPathW()", "RestoreBranchViewState(entry.BranchState)", "SwitchPanelTab(panel)")

    def test_history_owns_snapshot_and_preserves_unicode_focus(self):
        history = body("salamdr3.cpp", "void CPathHistory::ChangeActualPathData(")
        self.assertIn("std::make_shared<CBranchViewRestoreState>(*branchState)", history)
        self.assertIn("n2->BranchState.reset()", history)
        append = body("salamdr3.cpp", "void CPathHistory::AppendFrom(")
        self.assertIn("copy->BranchState = item->BranchState", append)
        execute = body("salamdr3.cpp", "BOOL CPathHistoryItem::Execute(")
        self.ordered(execute, "restore = BranchState", "panel->ChangePathToDisk(", "::IsTheSamePath(panel->GetPath(), PathOrArchiveOrFSName)", "panel->RestoreBranchViewState(*restore)")
        registry = source("salamdr3.cpp")
        self.assertIn("REG_BINARY, focus.c_str()", registry)
        self.assertIn("REG_BINARY, focus.data()", registry)
        capture = body("branch_view.cpp", "CBranchViewRestoreState CFilesWindow::CaptureBranchViewState()")
        self.assertIn("GetItemIdentityW(Files->At(i))", capture)
        self.assertIn("state.Selected.push_back(identity)", capture)

    def test_path_column_does_not_displace_extension_or_persist_globally(self):
        columns = body("fileswn9.cpp", "BOOL CFilesWindow::BuildColumnsTemplate()")
        branch = columns[columns.index("if (IsBranchView())"):]
        self.assertIn("BRANCH_VIEW_PATH_COLUMN", branch)
        self.assertIn("GetBranchViewPathColumnWidth()", branch)
        self.assertIn("COLUMN_ID_EXTENSION", branch)
        self.assertNotIn("Insert(1, pathColumn)", branch)
        width = body("fileswn9.cpp", "void CFilesWindow::OnHeaderLineColWidthChanged()")
        self.assertRegex(width, r"BRANCH_VIEW_PATH_COLUMN\)\s*\{\s*SetBranchViewPathColumnWidth\(column->Width\);\s*\}\s*else")
        self.assertIn("DrawTextW", source("fileswn4.cpp"))

    def test_temporary_dialog_selection_uses_exact_branch_identity(self):
        capture = body("fileswn0.cpp", "void CFilesWindow::SelectFocusedItemAndGetName(")
        self.ordered(capture, "selection.Clear()", "if (GetSelCount() == 0)", "selection.Name = f->Name", "selection.BranchIdentity = GetItemIdentityW(*f)", "SetSel(TRUE")
        cleanup = body("fileswn0.cpp", "void CFilesWindow::UnselectItemWithName(")
        self.ordered(cleanup, "if (!selection.BranchIdentity.empty())", "FindBranchTemporarySelection", "GetItemIdentityW(file)", "SetSel(FALSE, index)", "return;", "selection.Name.c_str()", "RegSetStrICmpEx")
        self.assertIn("file.Selected ? GetItemIdentityW(file) : std::wstring()", cleanup)
        for file in ("fileswn5.cpp", "fileswn7.cpp", "mainwnd3.cpp"):
            text = source(file)
            self.assertIn("CPanelTemporarySelection temporarySelected", text)
            self.assertNotIn("char temporarySelected[", text)
            self.assertNotIn("SelectFocusedItemAndGetName(temporarySelected,", text)
        self.assertNotIn("branchTemporarySelection", source("fileswn5.cpp"))

    def test_global_saved_selection_distinguishes_duplicate_names(self):
        save = body("fileswn1.cpp", "void CFilesWindow::StoreGlobalSelection()")
        self.ordered(save, "GlobalSelection.Clear()", "GlobalSelectionUsesFullPaths = IsBranchView()", "GlobalSelection.SetCaseSensitive(GlobalSelectionUsesFullPaths)", "GetItemIdentityW(*f)", "GlobalSelection.Add(i < Dirs->Count, identity.c_str())")
        restore = body("fileswn1.cpp", "void CFilesWindow::RestoreGlobalSelection()")
        self.assertIn("!clipboard && GlobalSelectionUsesFullPaths", restore)
        self.assertIn("GetItemIdentityW(*file)", restore)
        self.assertIn("selection->Contains(isDir, identity.c_str())", restore)
        self.assertNotIn("selection->Contains(isDir, file->Name)", restore)
        for operation in ("lsoCOPY", "lsoOR", "lsoDIFF", "lsoAND"):
            self.assertIn(operation, restore)

    def test_path_sort_keeps_focus_and_suspends_icon_reader(self):
        sort = body("fileswn2.cpp", "void CFilesWindow::ChangeCustomSortType(")
        self.ordered(sort, "SleepIconCacheThread();", "SortDirectory();", "file.Name == d1.Name", "focusIndex = i", "WakeupIconCacheThread();", "branchListing ? focusIndex : FocusedIndex")
        listing = body("fileswn3.cpp", "void CFilesWindow::SortDirectory(")
        self.ordered(listing, "BRANCH_VIEW_PATH_COLUMN", "std::stable_sort", "ReindexBranchViewFiles();", "VisibleItemsArray.InvalidateArr();")

    def test_property_results_are_rejected_after_snapshot_changes(self):
        text = source("fileswn3.cpp")
        self.assertIn("data->BranchGeneration = GetBranchViewGeneration()", text)
        self.assertIn("data->BranchGeneration == GetBranchViewGeneration()", text)
        self.assertIn("ExplorerPropertyRequestNeedsReplacement(data->BranchGeneration, data->ColumnIndex", text)
        self.assertIn("item.FullPath = GetItemFullPathW(files->At(i))", text)
        self.assertIn("GetItemCacheKey(file)", text)
        self.assertIn("ReindexBranchViewFiles();", text)


# These donor contracts require later stages; keep their original assertions intact.
DEFERRED = {"test_subtree_watcher_has_distinct_registration", "test_tab_state_captured_before_creation_restored_before_activation", "test_history_owns_snapshot_and_preserves_unicode_focus", "test_temporary_dialog_selection_uses_exact_branch_identity"}
def load_tests(loader, tests, pattern):
    return unittest.TestSuite(BranchViewContracts(name) for name in loader.getTestCaseNames(BranchViewContracts) if name not in DEFERRED)

if __name__ == "__main__":
    unittest.main()
