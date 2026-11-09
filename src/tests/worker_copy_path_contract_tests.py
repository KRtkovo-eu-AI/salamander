# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-09 | Check overwrite when reading the source is denied. | Require the reviewed target and keep failure from switching to a free-target request. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check exact Scan Ahead overwrite admission. | Require one scanned revision and serial conflict requeue on a mismatch. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Cover Scan Ahead protected worker admission and cancellation. | Exercise the guarded move and the boundary between a free target and a scanned conflict. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Guard protected F6 retries at the worker boundary. | Require verified completion instead of a second pathname deletion. | | GPT-6.1 Sol (Codex) |
from pathlib import Path
import unittest
SRC = Path(__file__).resolve().parents[1]
def text(name):
    return (SRC / name).read_text(encoding="utf-8-sig")
def section(source, first, last):
    return source.split(first, 1)[1].split(last, 1)[0]
class WorkerCopyPathContracts(unittest.TestCase):
    def test_owned_target_registers_handle_once(self):
        # Preserve the target's earlier handle-accounting fix when porting the
        # donor copy opens. Both paths register once after the conditional.
        code = section(text("worker.cpp"), "if (op->TargetNameWValid)", "if (out == INVALID_HANDLE_VALUE && encryptionNotSupported")
        self.assertEqual(code.count("NOHANDLES(CreateFileW(op->TargetNameW.c_str()"), 2)
        self.assertNotIn("HANDLES_Q(CreateFileW(op->TargetNameW.c_str()", code)
    def test_ads_preflight_opens_wide(self):
        code = section(text("worker.cpp"), "BOOL CheckFileOrDirADS(", "BOOL DeleteAllADS(")
        self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(fileName, NULL).c_str(),", code)
        self.assertNotIn("CreateFile(", code)
        self.assertNotIn("fileNameCrFileCopy", code)
    def test_copy_planning_error_probe_retains_owned_path(self):
        code = section(text("fileswn6.cpp"), "BOOL CFilesWindow::BuildScriptFile(", "case atDelete:")
        self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(op.SourceName, op.SourceNameWValid ? op.SourceNameW.c_str() : NULL).c_str(),", code)
        self.assertNotIn("CreateFile(op.SourceName", code)
    def test_every_copy_retry_uses_wide_boundary(self):
        code = text("worker.cpp")
        self.assertNotIn("CreateFile(op->", code)
        self.assertNotIn("CreateFile(Op->", code)
        for item in ("op->SourceName", "op->TargetName", "Op->SourceName", "Op->TargetName"):
            self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(" + item + ", " + item + "WValid ? " + item + "W.c_str() : NULL).c_str(),", code)
    def test_stream_copy_and_delete_keep_dynamic_wide_names(self):
        code = text("worker.cpp")
        copy = section(code, "BOOL DoCopyADS(", "HANDLE SalCreateFileEx(")
        self.assertIn("std::wstring sourceStream = sourceBase + streamNames[i]", copy)
        self.assertIn("std::wstring targetStream = targetBase + streamNames[i]", copy)
        self.assertNotIn("wchar_t srcName[", copy)
        self.assertNotIn("MultiByteToWideChar(CP_ACP", copy)
        delete = section(code, "BOOL DeleteAllADS(", "void MyStrCpyNW(")
        self.assertIn("std::wstring adsFullName", delete)
        self.assertNotIn("adsFullName[2 * MAX_PATH]", delete)
    def test_attribute_reopen_stays_wide(self):
        code = section(text("worker.cpp"), "void SetCompressAndEncryptedAttrs(", "void CorrectCaseOfTgtName(")
        self.assertIn("GetFileAttributesW(nameW.c_str())", code)
        self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(name, nameW.c_str()).c_str(),", code)
        self.assertIn("EncryptFileW(nameW.c_str())", code)
        self.assertIn("DecryptFileW(nameW.c_str(), 0)", code)
        self.assertNotIn("CreateFile(", code)
    def test_denied_move_completion_does_not_retry_path_deletion(self):
        code = section(text("worker.cpp"), "MoveCopyReceipt receipt;", "BOOL DoDeleteFile(")
        ordinary_delete = section(code, "if (!protectedCompletion)", "WaitForSingleObject")
        self.assertIn("DeleteFileW(", ordinary_delete)
        self.assertEqual(code.count("DeleteFileW("), 1)
        prepared = section(code, "if (request.PrepareMoveCompletion(", "else if (protectedCompletion")
        self.assertLess(prepared.index("protectedCompletion = true;"), prepared.index("ExecuteWorkerTransfer("))
        stale = section(code, "if (err == ERROR_FILE_INVALID)", "int ret = IDCANCEL;")
        self.assertIn("WORKER_DIALOG_UAC_TRANSFER_UNKNOWN", stale)
        self.assertIn("return FALSE;", stale)
    def test_scan_ahead_uses_serial_free_target_elevation(self):
        panel = section(text("fileswn8.cpp"), "script->AllowTransferPilot =", "script->IgnoreTransferAds")
        self.assertIn("conflictMode == CMCM_SCAN_AHEAD", panel)
        code = text("worker.cpp")
        worker = section(code, "unsigned ThreadWorkerBody(void* parameter)", "case ocMoveDir:")
        self.assertLess(worker.index("GetNextConflictReadyItem("), worker.index("Error = !DoCopyFile("))
        self.assertLess(worker.index("RunParallelCopyBatch("), worker.index("Error = !DoCopyFile("))
        transfer = section(code, "static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite(")
        self.assertIn("dlgData.ConflictDecisionInjected", transfer)
        self.assertIn("GetFileAttributesW(targetPath.c_str()) != INVALID_FILE_ATTRIBUTES", transfer)
        revalidate = section(code, "BOOL COperations::RevalidateConflictItem(", "BOOL COperations::RevalidatePreparedConflictAncestors(")
        vanished = section(revalidate, "else if (GetLastError() == ERROR_FILE_NOT_FOUND", "if (!valid)")
        self.assertIn("conflictType = cmctNone;", vanished)
        self.assertIn("executionConflictType != cmctNone", worker)
    def test_compressed_move_with_ads_reaches_the_cross_volume_broker(self):
        transfer = section(text("worker.cpp"), "static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite(")
        eligible = transfer.split("request.Prepare(", 1)[0]
        self.assertIn("IsEligibleMoveFile(op.Attr, script.CopyAttrs, script.CopySecurity)", eligible)
        self.assertIn("Operation::MoveCompressed", eligible)
        self.assertNotIn("OPFL_COPY_ADS", eligible)
        self.assertIn("moving && !request.IsCrossVolumeMove()", transfer)
        self.assertIn("OPFL_UAC_TRANSFER_SOURCE | OPFL_COPY_ADS", transfer)
    def test_cancelled_elevation_stops_the_batch(self):
        transfer = section(text("worker.cpp"), "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertIn("success = FALSE;", transfer)
        declined = section(transfer, "if (!prompt.approved)", "const HANDLE cancel")
        self.assertIn("jobs.clear();", declined)
        self.assertIn("return true;", declined)
        self.assertIn("result.unknown || result.error == ERROR_CANCELLED", transfer)
        self.assertIn("return error == ERROR_CANCELLED || *dlgData.CancelWorker != FALSE;", transfer)
    def test_scan_ahead_overwrite_requires_the_reviewed_revision(self):
        code = text("worker.cpp")
        capture = section(code, "static void CaptureScannedOverwriteRevision(", "BOOL COperations::RevalidateConflictItem(")
        self.assertIn("FILE_FLAG_OPEN_REPARSE_POINT", capture)
        self.assertIn("current.nFileIndexLow == observed.nFileIndexLow", capture)
        scanner = section(code, "static DWORD WINAPI ConflictScannerBody(", "unsigned ThreadWorkerBody")
        self.assertIn("CaptureScannedOverwriteRevision(target, info, revision)", scanner)
        requeue = section(code, "void COperations::RequeueChangedConflictItem(", "void COperations::MarkConflictDirectoryPrepared(")
        self.assertIn("CaptureScannedOverwriteRevision(target, info, revision)", requeue)
        self.assertIn("op.ScannedTargetRevision = revision;", requeue)
        overwrite = section(code, "static bool PrepareWorkerOverwrite(", "struct MoveCopyReceipt")
        self.assertIn("SameFileRevision(op.ScannedTargetRevision, request.Approval().revision)", overwrite)
        self.assertIn("dlgData.InjectedConflictType != cmctFileExists", overwrite)
        lease = section(code, "COperation& op = At(*item);", "CConflictLeaseOutcome COperations::FinishParallelCopyLeaseLocked")
        self.assertIn("op.ScannedTargetRevision.attributes", lease)
        transfer = section(code, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        stale = section(transfer, "if (error == ERROR_FILE_INVALID", "return error == ERROR_CANCELLED")
        self.assertIn("request.IsOverwrite() && dlgData.ConflictDecisionInjected", stale)
        self.assertIn("dlgData.ConflictTargetChanged = TRUE;", stale)
        self.assertNotIn("totalDone", stale)
    def test_source_denial_can_overwrite_only_the_reviewed_target(self):
        code = text("worker.cpp")
        helper = section(code, "static SourceOverwriteResult TryOverwriteDeniedSource(", "struct MoveCopyReceipt")
        self.assertLess(helper.index("PrepareWorkerOverwrite("), helper.index("MatchesOverwriteTarget(target)"))
        self.assertLess(helper.index("MatchesOverwriteTarget(target)"), helper.index("SendMessage(progress, WM_USER_DIALOG, 1,"))
        self.assertLess(helper.index("answer != IDYES && answer != IDB_ALL"), helper.index("ExecuteWorkerTransfer("))
        self.assertIn("approval.captured && !overwrite.MatchesApproval(approval)", helper)
        self.assertIn("return Outcome::Skipped;", helper)
        self.assertIn("return Outcome::Failed;", helper)
        worker = section(code, "BOOL DoCopyFile(COperation* op", "BOOL DoMoveFile(")
        deferred = section(worker, "if ((op->OpFlags & OPFL_UAC_TRANSFER_SOURCE)", "BOOL tgtNameCaseCorrected")
        denied = section(worker, "OPEN_IN_ERROR:", "SKIP_OPEN_IN:")
        for path in (deferred, denied):
            self.assertIn("TryOverwriteDeniedSource(", path)
            self.assertIn("outcome == SourceOverwriteResult::Unavailable &&", path)
            self.assertLess(path.index("TryOverwriteDeniedSource("), path.index("TryTransferWithBroker("))
            self.assertIn("SourceOverwriteResult::Skipped", path)
if __name__ == "__main__":
    unittest.main()
