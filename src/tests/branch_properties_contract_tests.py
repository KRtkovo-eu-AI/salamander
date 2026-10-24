# SPDX-License-Identifier: GPL-2.0-or-later
"""Keep Branch properties tied to exact items independently of active sort."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
TEXT = (ROOT / "fileswn3.cpp").read_text(encoding="utf-8-sig")
class BranchPropertiesContracts(unittest.TestCase):
    def test_async_snapshot_owns_full_path_and_unique_identity(self):
        start = TEXT.index("BOOL CFilesWindow::StartExplorerSortAsync(")
        part = TEXT[start:TEXT.index("void CFilesWindow::FinishExplorerSortAsync(", start)]
        for kind in ("files", "dirs"):
            self.assertIn("item.Name = GetItemCacheKey(" + kind + "->At(i))", part)
            self.assertIn("item.FullPath = GetItemFullPathW(" + kind + "->At(i))", part)
        self.assertIn("GetExplorerColumnTextForPathW(item.FullPath.c_str()", TEXT)
    def test_visible_property_load_is_not_an_implicit_sort(self):
        self.assertIn("data->ColumnIndex = -1;", TEXT)
        self.assertIn("panelColumn->GetText == InternalGetExplorerColumn", TEXT)
        self.assertIn("if (SortType == stCustom && panelColumn->CustomData == SortCustomData)", TEXT)
        self.assertIn("data->ColumnIndex >= 0 && SortType == stCustom", TEXT)
    def test_path_sort_still_starts_visible_property_loader(self):
        start = TEXT.index("void CFilesWindow::SortDirectory(")
        part = TEXT[start:TEXT.index("else if (SortType == stCustom && Is(ptDisk))", start)]
        self.assertIn("SortCustomData == BRANCH_VIEW_PATH_COLUMN", part)
        self.assertIn("StartExplorerSortAsync(files, dirs,", part)
    def test_stale_generation_restarts_even_when_sorted_by_path(self):
        start = TEXT.index("const bool restartBranchProperties")
        part = TEXT[start:TEXT.index("BOOL CFilesWindow::GetCachedExplorerColumnText", start)]
        self.assertIn("ExplorerPropertyRequestNeedsReplacement(data->BranchGeneration, data->ColumnIndex", part)
        self.assertIn("GetBranchViewGeneration(), CurrentExplorerSortColumn(this)", part)
        self.assertIn("if (restartBranchProperties)", part)
        self.assertNotIn("BRANCH_VIEW_PATH_COLUMN", part)
    def test_refresh_cannot_snapshot_old_rows_as_new_generation(self):
        start = TEXT.index("BOOL CFilesWindow::StartExplorerSortAsync(")
        part = TEXT[start:TEXT.index("std::wstring panelPath", start)]
        self.assertIn("IsBranchView() && BranchView->Applying", part)
        branch = (ROOT / "branch_view.cpp").read_text(encoding="utf-8-sig")
        start = branch.index("void CFilesWindow::PollBranchView()")
        part = branch[start:branch.index("\n}", start) + 2]
        positions = [part.index(item) for item in ("++BranchView->Generation;", "RefreshDirectory();", "BranchView->Applying = false;", "StartExplorerSortAsync(Files, Dirs,")]
        self.assertEqual(positions, sorted(positions))
    def test_deferred_publication_does_not_fall_back_to_sync_property_reads(self):
        self.assertIn("BOOL asyncSortPending = IsBranchView();", TEXT)
        start = TEXT.index("BOOL asyncSortPending = IsBranchView();")
        part = TEXT[start:TEXT.index("if (asyncSortPending)\n        {\n            // Keep the UI responsive", start)]
        self.assertIn("if (asyncSortPending)", part)
        self.assertIn("StartExplorerSortAsync(files, dirs, firstDirIndex);", part)
        self.assertNotIn("FillExplorerSortCache", part)
    def test_display_load_is_viewport_bounded_but_explicit_sort_is_complete(self):
        self.assertIn("data->VisibleOnly = IsBranchView() && data->ColumnIndex < 0;", TEXT)
        self.assertIn("ListBox->GetVisibleItems(&first, &count)", TEXT)
        self.assertIn("ExplorerPropertyDisplayRows(files->Count + dirs->Count, first, count,", TEXT)
        self.assertIn("data->Items.reserve(files->Count + dirs->Count - firstDirIndex)", TEXT)
        self.assertIn("for (int i = 0; i < files->Count; ++i)", TEXT)
    def test_branch_metadata_neither_shows_scan_spinner_nor_measures_all_rows(self):
        self.assertIn("if (DirectoryLine != NULL && !IsBranchView())", TEXT)
        start = TEXT.index("else if (ExplorerPropertyCache != NULL)")
        part = TEXT[start:TEXT.index("const bool restartBranchProperties", start)]
        self.assertIn("if (IsBranchView() && ListBox != NULL)", part)
        self.assertIn("InvalidateRect(GetListBoxHWND(), NULL, FALSE)", part)
    def test_viewport_refill_merges_values_and_never_reads_properties_in_paint(self):
        self.assertIn("ExplorerPropertyCache->Values[value.first] = std::move(value.second)", TEXT)
        start = TEXT.index("BOOL CFilesWindow::GetCachedExplorerColumnText")
        part = TEXT[start:TEXT.index("if (ExplorerPropertyCache == NULL)", start)]
        self.assertIn("ExplorerPropertyCache->BranchGeneration == GetBranchViewGeneration()", part)
        self.assertIn("StartExplorerSortAsync(Files, Dirs,", part)
        self.assertNotIn("GetExplorerColumnTextForFile", part)
        self.assertIn("return !value->second.empty();", part)
    def test_shutdown_is_cooperative_and_disconnects_before_hwnd_destruction(self):
        start = TEXT.index("void CFilesWindow::StopExplorerSortAsync()")
        part = TEXT[start:TEXT.index("int DeltaForTotalCount", start)]
        self.assertIn("data->CancelCompletion(WM_USER_EXPLORER_SORT_DONE)", part)
        self.assertIn("data->Release()", part)
        self.assertNotIn("WaitForSingleObject", part)
        self.assertNotIn("TerminateThread", part)
        events = (ROOT / "fileswnb.cpp").read_text(encoding="utf-8-sig")
        part = events[events.index("case WM_DESTROY:"):]
        self.assertLess(part.index("StopExplorerSortAsync();"), part.index("delete DirectoryLine;"))
        self.assertIn("data->AddRef(); // worker ownership", TEXT)
        self.assertIn("data->Release(); // worker ownership", TEXT)
    def test_pending_sort_intent_is_cancelled_and_restarted_without_sync_fallback(self):
        start = TEXT.index("BOOL CFilesWindow::StartExplorerSortAsync(")
        part = TEXT[start:TEXT.index("std::wstring panelPath", start)]
        self.assertIn("ExplorerPropertyRequestNeedsReplacement(", part)
        self.assertIn("CurrentExplorerSortColumn(this)", part)
        self.assertIn("InterlockedExchange(&pending->Cancelled, 1)", part)
        self.assertIn("ExplorerSortData == NULL || IsBranchView()", TEXT)
        self.assertIn("if (sortingPanelListing)\n            StartExplorerSortAsync(files, dirs, dirs->Count", TEXT)
    def test_property_sort_retains_focused_item_identity(self):
        self.assertIn("const char* branchFocusName = NULL", TEXT)
        self.assertIn("Files->At(i).Name == branchFocusName", TEXT)
        self.assertIn("RefreshListBox(-1, -1, focusIndex, FALSE, FALSE)", TEXT)
    def test_new_work_helper_and_native_test_are_registered(self):
        project = (ROOT / "vcxproj" / "salamand.vcxproj").read_text(encoding="utf-8-sig")
        self.assertIn("explorerpropertywork.h", project)
        runner = (ROOT.parent / "tools" / "run_native_tests.ps1").read_text(encoding="utf-8-sig")
        self.assertIn("-Filter '*_tests.vcxproj' -File -Recurse", runner)
        self.assertIn("branch_properties_contract_tests.py", runner)
        self.assertTrue((ROOT / "tests" / "explorer_property_work_tests.vcxproj").is_file())
    def test_cache_lookup_is_full_identity_not_duplicate_basename(self):
        start = TEXT.index("BOOL CFilesWindow::GetCachedExplorerColumnText")
        part = TEXT[start:TEXT.index("void CFilesWindow::ClearExplorerPropertyCache", start)]
        self.assertIn("std::make_pair(GetItemCacheKey(*file), columnIndex)", part)
        self.assertNotIn("std::make_pair(file->Name", part)
if __name__ == "__main__":
    unittest.main()
