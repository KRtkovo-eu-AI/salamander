# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Track the source revalidation result signature. | Keep cleanup contracts attached to the actual worker methods. | #731 | GPT-6 |
# | 2026-11-14 | Distinguish ordinary overwrites from scoped target cleanup. | Preserve upstream behavior without using the broker for local mutations. | #731 | GPT-6 |
# | 2026-11-13 | Cover unstarted prepared scope during Retry. | Prevent a missing target from being created before identity validation. | #731 | GPT-6 |
# | 2026-11-13 | Check scoped preallocation reset and failure exit. | Keep the cleanup regression check aligned with handle-based recovery. | #731 | GPT-6 |
# | 2026-11-12 | Cover deferred source cancellation and safe local retry. | Preserve live targets and reject side-effecting reopen. | #731 | GPT-6 |
# | 2026-11-12 | Guard scoped-copy cleanup contracts. | Catch reintroduced pathname deletion and unsafe restarts. | #731 | GPT-6 |
"""Structural regression checks; these do not replace actual worker fault tests."""
from pathlib import Path
import unittest

TEXT = (Path(__file__).resolve().parents[1] / "worker.cpp").read_text(encoding="utf-8-sig")


def section(start, end):
    return TEXT.split(start, 1)[1].split(end, 1)[0]


class ScopedCleanupContracts(unittest.TestCase):
    def test_sync_retry_checks_connection_before_each_write(self):
        code = section("void DoCopyFileLoopOrig(", "enum CCopy_BlkState")
        write = code.index("THIN_COPY_WRITE(out, buffer, read, &written, NULL)")
        loop = code.rfind("while (1)", 0, write)
        guard = code[loop:write]
        self.assertIn("#ifdef SAM_UAC_THIN_COPY_TEST", guard)
        self.assertIn("thin->CheckTransferConnection()", guard)
        self.assertIn("thin->ReportTransferLoss(error, IDS_ERRORWRITINGFILE)", guard)
        self.assertIn("copyError = TRUE", guard)
        self.assertIn("return;", guard)
        self.assertGreater(code.index("case IDRETRY:", write), write)

    def test_sync_and_async_allocation_restart_are_guarded(self):
        for name, end in (("DoCopyFileLoopOrig(", "enum CCopy_BlkState"),
                          ("void DoCopyFileLoopAsync(", "static UacF7::Result ExecuteWithNativeTransferProgress")):
            with self.subTest(loop=name):
                code = section(name, end)
                clear = code.index("WorkerClearReadOnlyW(")
                guard = code.rfind("if (thin && thin->ScopedTarget())", 0, clear)
                self.assertGreater(guard, -1)
                scoped = code[guard:clear]
                self.assertIn("CloseScopedTarget(out)", scoped)
                self.assertIn("THIN_DELETE_TARGET(thin, FALSE)", scoped)
                self.assertLess(scoped.index("THIN_DELETE_TARGET"), scoped.index("copyAgain = TRUE"))
                self.assertIn("copyError = TRUE", scoped)
                self.assertIn("return;", scoped)

    def test_initial_allocation_cleanup_stops_if_unverified(self):
        worker = section("BOOL DoCopyFile(COperation* op", "BOOL DoMoveFile(")
        code = worker.split("// if possible, allocate the required space for the file", 1)[1].split("script->SetFileStartParams();", 1)[0]
        guard = code.index("if (thin.ScopedTarget())")
        scoped = code[guard:code.index("\n                            else\n#endif", guard)]
        self.assertIn("!thin.dataTransferStarted && !cancelled", scoped)
        self.assertIn("ThinCopyResetFailedPreallocation(out)", scoped)
        self.assertIn("thin.CloseScopedTarget(out)", scoped)
        self.assertIn("closed && THIN_DELETE_TARGET(&thin, FALSE)", scoped)
        self.assertIn("WORKER_DIALOG_UAC_THIN_TERMINAL", scoped)
        self.assertIn("CThinTerminalFileError details{resetError == ERROR_SUCCESS || resetError == ERROR_CANCELLED", scoped)
        self.assertIn("? ERROR_GEN_FAILURE : resetError", scoped)
        self.assertIn("return FALSE", scoped)
        self.assertNotIn("goto OPEN_TGT_FILE", scoped)
        self.assertNotIn("DeleteFileW", scoped)

    def test_reset_failure_uses_terminal_native_error_dialog(self):
        dialogs = (Path(__file__).resolve().parents[1] / "dialogs.cpp").read_text(encoding="utf-8-sig")
        dispatch = dialogs.split("case WORKER_DIALOG_UAC_THIN_TERMINAL:", 1)[1].split("#endif", 1)[0]
        self.assertIn("GetErrorText(error.error)", dispatch)
        self.assertIn("MB_OK | MB_ICONEXCLAMATION", dispatch)
        self.assertNotIn("CFileErrorDlg", dispatch)
        self.assertNotIn("IDRETRY", dispatch)
        self.assertNotIn("IDB_SKIP", dispatch)
        self.assertIn("error.cleanupUncertain", dispatch)
        self.assertIn("IDS_UAC_COPY_UNKNOWN", dispatch)
        self.assertIn("CancelWorkerEvent", dispatch)
        guard = dialogs.split("case WM_USER_DIALOG:", 1)[1].split("if (DelayParallelProgressShow)", 1)[0]
        self.assertIn("wParam != WORKER_DIALOG_UAC_THIN_TERMINAL", guard)

    def test_failed_close_cannot_delete_or_restart(self):
        worker = section("BOOL DoCopyFile(COperation* op", "BOOL DoMoveFile(")
        code = worker.split("if (!HANDLES(CloseHandle(out)))", 1)[1].split("SetFileAttributesW(WorkerOperationPathW(op->TargetName", 1)[0]
        guarded = code.split("WaitForSingleObject(dlgData.WorkerNotSuspended", 1)[0]
        self.assertIn("if (thinScopedAttributes)", guarded)
        self.assertIn("thin.ReportTargetFailure(err)", guarded)
        self.assertIn("return FALSE", guarded)
        self.assertIn("if (!thinScopedAttributes)", code)

    def test_unknown_close_does_not_send_discard(self):
        code = section("HANDLE CThinCopyWorker::CancelReceivedHandle(", "bool CThinCopyWorker::RememberSource(")
        failure = code.split("if (!CloseHandle(received))", 1)[1].split("const bool receivedWasTarget", 1)[0]
        self.assertIn("ReportTargetFailure(closeError)", failure)
        self.assertIn("return INVALID_HANDLE_VALUE", failure)
        self.assertNotIn("DiscardTarget()", failure)
        self.assertIn("kind == UacF7::ThinOpenKind::TargetCreate", code)
        self.assertIn("kind == UacF7::ThinOpenKind::TargetRetry", code)
        self.assertIn("!receivedWasTarget || !targetCreatedByBroker || DiscardTarget()", code)

    def test_scoped_attributes_use_live_handle(self):
        code = section("const bool thinScopedAttributes = thin.ScopedTarget()", "if (!HANDLES(CloseHandle(out)))")
        self.assertIn("SetFileInformationByHandle(out, FileBasicInfo", code)
        self.assertIn("thin.CloseScopedTarget(out)", code)
        self.assertIn("IDS_ERRORCHANGINGATTRS", code)
        self.assertNotIn("SetFileAttributesW", code)

    def test_existing_target_uses_only_native_overwrite_if_not_uncertain(self):
        code = section("if (thin.Scoped())\n                            {\n                                // Continue only", "BOOL targetCannotOpenForWrite")
        self.assertIn("thin.nativeTargetOverwrite = true", code)
        self.assertIn("if (!thin.NativeOverwrite())", code)
        self.assertIn("ERROR_NOT_SUPPORTED", code)
        self.assertIn("return FALSE", code)
        self.assertNotIn("DeleteFileW", code)

    def test_scoped_local_retry_does_not_create_before_identity_check(self):
        code = section("static HANDLE ThinCopyOpenLocalRetry(", "#define THIN_REMEMBER_SOURCE")
        self.assertIn("GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING", code)
        self.assertIn("FILE_FLAG_SEQUENTIAL_SCAN | (overlapped ? FILE_FLAG_OVERLAPPED : 0)", code)
        self.assertNotIn("OPEN_ALWAYS", code.split("return ThinCopyOpened", 1)[1])
        self.assertIn("UacF7::ThinOpenKind::TargetRetry", code)
        macro = section("#define THIN_OPEN_TARGET_RETRY(ctx, local, overlapped)", "#define THIN_OPEN_TARGET_PROBE")
        self.assertIn("((ctx)->request || (ctx)->session) ? ThinCopyOpenLocalRetry", macro)
        self.assertIn("ThinCopyOpenRetry", macro)

    def test_async_retry_uses_the_target_scope_not_only_the_source_session(self):
        code = section("BOOL CCopy_Context::RetryCopyReadErr(", "BOOL CCopy_Context::HandleWritingErr(")
        self.assertEqual(code.count("if (Thin && Thin->ScopedTarget())"), 3)
        self.assertNotIn("if (Thin && Thin->Scoped())", code)
        self.assertEqual(code.count("Thin->CloseScopedTarget(*Out)"), 3)

    def test_uncertain_cleanup_is_not_retried(self):
        code = section("bool CThinCopyWorker::DiscardTarget()", "#endif")
        self.assertIn("if (cleanupUncertain) return false", code)
        self.assertIn("session->DiscardTarget(*request, target)", code)
        self.assertIn("return ReportTargetFailure(error)", code)


if __name__ == "__main__":
    unittest.main()
