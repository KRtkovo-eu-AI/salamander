# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Cover deferred source cancellation and safe local retry. | Preserve live targets and reject side-effecting reopen. | #731 | GPT-6 |
# | 2026-11-12 | Guard scoped-copy cleanup contracts. | Catch reintroduced pathname deletion and unsafe restarts. | #731 | GPT-6 |
"""Structural regression checks; these do not replace actual worker fault tests."""
from pathlib import Path
import unittest

TEXT = (Path(__file__).resolve().parents[1] / "worker.cpp").read_text(encoding="utf-8-sig")


def section(start, end):
    return TEXT.split(start, 1)[1].split(end, 1)[0]


class ScopedCleanupContracts(unittest.TestCase):
    def test_sync_and_async_allocation_restart_are_guarded(self):
        for name, end in (("DoCopyFileLoopOrig(", "enum CCopy_BlkState"),
                          ("void DoCopyFileLoopAsync(", "static UacF7::Result ExecuteWithNativeTransferProgress")):
            with self.subTest(loop=name):
                code = section(name, end)
                clear = code.index("WorkerClearReadOnlyW(")
                guard = code.rfind("if (thin && thin->Scoped())", 0, clear)
                self.assertGreater(guard, -1)
                scoped = code[guard:clear]
                self.assertIn("CloseScopedTarget(out)", scoped)
                self.assertIn("THIN_DELETE_TARGET(thin, FALSE)", scoped)
                self.assertLess(scoped.index("THIN_DELETE_TARGET"), scoped.index("copyAgain = TRUE"))
                self.assertIn("copyError = TRUE", scoped)
                self.assertIn("return;", scoped)

    def test_initial_allocation_cleanup_stops_if_unverified(self):
        worker = section("BOOL DoCopyFile(COperation* op", "BOOL DoMoveFile(")
        code = worker.split("// try truncating the file to zero", 1)[1].split("script->SetFileStartParams();", 1)[0]
        guard = code.index("if (thin.Scoped())")
        legacy = code.index("WorkerClearReadOnlyW(")
        scoped = code[guard:legacy]
        self.assertIn("!thin.CloseScopedTarget(out) || !THIN_DELETE_TARGET(&thin, FALSE)", scoped)
        self.assertLess(scoped.index("return FALSE"), scoped.index("goto OPEN_TGT_FILE"))

    def test_failed_close_cannot_delete_or_restart(self):
        worker = section("BOOL DoCopyFile(COperation* op", "BOOL DoMoveFile(")
        code = worker.split("if (!HANDLES(CloseHandle(out)))", 1)[1].split("SetFileAttributesW(WorkerOperationPathW(op->TargetName", 1)[0]
        guarded = code.split("WaitForSingleObject(dlgData.WorkerNotSuspended", 1)[0]
        self.assertIn("if (thinScopedAttributes)", guarded)
        self.assertIn("thin.ReportTargetFailure(err)", guarded)
        self.assertIn("return FALSE", guarded)
        self.assertIn("if (!thinScopedAttributes)", code)

    def test_unknown_close_does_not_send_discard(self):
        code = section("HANDLE CThinCopyWorker::CancelReceivedHandle(", "void CThinCopyWorker::RememberSource(")
        failure = code.split("if (!CloseHandle(received))", 1)[1].split("const bool receivedWasTarget", 1)[0]
        self.assertIn("ReportTargetFailure(closeError)", failure)
        self.assertIn("return INVALID_HANDLE_VALUE", failure)
        self.assertNotIn("DiscardTarget()", failure)
        self.assertIn("kind == UacF7::ThinOpenKind::TargetCreate", code)
        self.assertIn("kind == UacF7::ThinOpenKind::TargetRetry", code)
        self.assertIn("!receivedWasTarget || !targetCreatedByBroker || DiscardTarget()", code)

    def test_scoped_attributes_use_live_handle(self):
        code = section("const bool thinScopedAttributes = thin.Scoped()", "if (!HANDLES(CloseHandle(out)))")
        self.assertIn("SetFileInformationByHandle(out, FileBasicInfo", code)
        self.assertIn("thin.CloseScopedTarget(out)", code)
        self.assertIn("IDS_ERRORCHANGINGATTRS", code)
        self.assertNotIn("SetFileAttributesW", code)

    def test_existing_target_not_deleted_by_scoped_experiment(self):
        code = section("if (thin.Scoped())\n                            {\n                                // Existing conflict targets", "BOOL targetCannotOpenForWrite")
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
        self.assertIn("(ctx)->Scoped() ? ThinCopyOpenLocalRetry", macro)
        self.assertIn("ThinCopyOpenRetry", macro)

    def test_uncertain_cleanup_is_not_retried(self):
        code = section("bool CThinCopyWorker::DiscardTarget()", "#endif")
        self.assertIn("if (cleanupUncertain) return false", code)
        self.assertIn("session->DiscardTarget(*request, target)", code)
        self.assertIn("return ReportTargetFailure(error)", code)


if __name__ == "__main__":
    unittest.main()
