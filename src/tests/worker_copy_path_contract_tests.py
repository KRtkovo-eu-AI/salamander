# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Update scoped worker copy contracts. | Check current cancellation, identity and admission boundaries without global token counts. | #731 | GPT-6 |
# | 2026-11-11 | Check typed ignored-stream overwrite routing. | Keep F5 policy distinct from ordinary copy and F6. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Cover shared copy and move overwrite routing. | Preserve listed source identity and move completion in the common retry. | #731 | GPT-6.1 Sol |
# | 2026-11-11 | Follow the shared operation selector for compressed moves. | Preserve the exact MoveCompressed route after removing duplicate host selection. | #731 | GPT-6.1 Sol |
# | 2026-11-11 | Check opened source identities and exact tree-copy consent. | Cover the protected tree regression and avoid a second application prompt. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Guard the error reported when the copy broker is unavailable. | Retain the original file-system error in the ordinary fallback. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check that a planned root never falls back to local mutation. | Cover collision and broker disconnection in the host routing. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Match the target creation error guard. | Check ordinary target creation before the privileged retry. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Cover source refusal followed by a target denial. | Preserve ordinary copy and suppress a second elevation in the same job. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check that prepared tree sessions do not prompt again. | Keep the planned broker scoped to the approved copy. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify existing-root identity capture and first-denial consent. | Keep normal confirmations ahead of the privileged merge. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify plain recursive copy startup and reuse. | Preserve direct selection rules and prevent descendant prompts. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check routing for deferred same-volume directory moves. | Prevent local fallback from bypassing the verified target. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Guard the atomic directory move worker and its separate consent. | Keep recursive file operations and cross-volume moves on their existing paths. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Cover deferred destinations through the existing file worker. | Verify single-file sessions and retain source overwrite and Branch View protection. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Cover the verified transfer destination preview. | Require matching target identities before using a shielded approval. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Guard the VM regression for missing relative destination parents. | Keep ADS discovery from blocking a verified retry before directory creation. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check the installed host gate and protected overwrite source preparation. | Pair the native fallback evidence with the actual host dispatch. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check protected source identity per Branch View transfer. | Preserve mixed access, exact overwrite and ordinary worker behavior. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify automatic F5 and F6 scope reuse. | Keep one shielded confirmation for the verified batch. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check that shielded OK is scoped to a matching target denial. | Preserve separate source elevation and consume the target choice once. | #731 | GPT-6 (Codex) |
# | 2026-11-09 | Check Scan Ahead shutdown after a cancelled protected transfer. | Prevent queued conflict work from presenting another dialog. | | Codex |
# | 2026-11-09 | Check identity-bound retirement after an ordinary F6 copy. | Require the verified path before any broker retry. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check cleanup of disconnected scoped broker entries. | Keep live consent and discard closed scopes before the next item. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check direct file admission in mixed protected transfer selections. | Keep recursively discovered items out of the direct file consent scope. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check named-stream overwrite admission. | Keep alternate data on the guarded disk-copy route. | | GPT-6.1 Sol (Codex) |
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
        self.assertIn("UacF7::DeleteCopiedMoveSource(", ordinary_delete)
        self.assertIn("else\n                        err = DeleteFileW(", ordinary_delete)
        self.assertEqual(code.count("DeleteFileW("), 1)
        prepared = section(code, "if (request.PrepareMoveCompletion(", "else if (protectedCompletion")
        self.assertLess(prepared.index("protectedCompletion = true;"), prepared.index("ExecuteWorkerTransfer("))
        stale = section(code, "if (err == ERROR_FILE_INVALID && verified)", "int ret = IDCANCEL;")
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
    def test_mixed_selection_limits_protected_retries_to_direct_jobs(self):
        panel = section(text("fileswn8.cpp"), "script->AllowTransferPilot =", "script->AllowTreeTransfer =")
        self.assertNotIn("dirs == 0", panel)
        self.assertIn("!target->IsBranchView()", panel)
        self.assertNotIn("!IsBranchView()", panel)
        builder = text("fileswn6.cpp")
        direct = section(builder, "BOOL CFilesWindow::BuildScriptMain(", "BOOL CFilesWindow::BuildScriptDir(")
        recursive = section(builder, "BOOL CFilesWindow::BuildScriptDir(", "BOOL CFilesWindow::BuildScriptFile(")
        self.assertIn("script->AllowTransferPilot ? OPFL_UAC_TRANSFER_ELIGIBLE : 0", direct)
        self.assertIn("itemFlags, fileWideName", direct)
        atomic = section(recursive, "op.Opcode = ocMoveDir;", "int createDirIndex = -1")
        self.assertIn("if (firstLevelDir && script->AllowTransferPilot", atomic)
        self.assertIn("op.SetSourceNameW", atomic)
        self.assertIn("op.SetTargetNameW", atomic)
        self.assertIn("OPFL_UAC_TRANSFER_ELIGIBLE", atomic)
        self.assertIn("OPFL_UAC_TREE_ENTRY | OPFL_UAC_TRANSFER_ELIGIBLE", recursive)
        self.assertIn("(protectedCopy || protectedMove) && (op.OpFlags & OPFL_UAC_TRANSFER_ELIGIBLE)", builder)

        worker = text("worker.cpp")
        for start, end in (("static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite("),
                           ("static bool PrepareWorkerOverwrite(", "enum class SourceOverwriteResult")):
            self.assertIn("!(op.OpFlags & OPFL_UAC_TRANSFER_ELIGIBLE)", section(worker, start, end))
        probe = section(worker, "static SourceOverwriteResult TrySingleProbedOverwrite(",
                        "static SourceOverwriteResult TryOverwriteDeniedSource(")
        self.assertIn("!(op.OpFlags & OPFL_UAC_TRANSFER_ELIGIBLE)", probe)
        execution = section(worker, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        single = section(execution, "const bool singleProbeJob =", "const bool shareJob =")
        self.assertIn("script.Count == 1 && op.Opcode == ocCopyFile", single)
        self.assertIn("(op.OpFlags & OPFL_UAC_TRANSFER_ELIGIBLE)", single)

    def test_recursive_copy_creation_uses_only_the_selected_root(self):
        controller = text("fileswn8.cpp")
        self.assertIn("script->AllowTreeTransfer = type == atCopy", controller)
        self.assertIn("f->Attr == FILE_ATTRIBUTE_DIRECTORY && count <= 1", controller)
        builder = text("fileswn6.cpp")
        self.assertIn("if (firstLevelDir)\n                        op.OpFlags |= OPFL_UAC_TREE_ROOT", builder)
        worker = text("worker.cpp")
        create = section(worker, "BOOL DoCreateDir(", "BOOL DoCopyDirTime(")
        self.assertLess(create.index("WorkerCreateDirectory("), create.index("if (err == ERROR_ACCESS_DENIED"))
        self.assertIn("TryTransferWithBroker(", create)
        execute = section(worker, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertLess(execute.index("!(op.OpFlags & OPFL_UAC_TREE_ROOT)"), execute.index("TransferPrompt prompt"))
        self.assertIn("request.IsMoveCompletion() || request.IsDirectoryCopyEntry()", execute)

    def test_recursive_merge_freezes_the_root_before_the_confirmation(self):
        worker = text("worker.cpp")
        create = section(worker, 'if (attr2 != INVALID_FILE_ATTRIBUTES && (attr2 & FILE_ATTRIBUTE_DIRECTORY))',
                         'alreadyExisted = TRUE;')
        self.assertLess(create.index("PrepareWorkerTreeMerge(op, dlgData)"), create.index("WM_USER_DIALOG, 7"))
        execute = section(worker, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertLess(execute.index("pending->Request.AllowsTreeEntry(request)"), execute.index("TransferPrompt prompt"))
        self.assertLess(execute.index("pending->Request.Execute("), execute.index("ExecuteWithNativeTransferProgress("))
        self.assertIn("opened.unknown || !request.IsInJob(*existing)", execute)
        self.assertIn("pending.reset()", execute)

    def test_branch_transfer_retries_use_only_the_prepared_row(self):
        builder = text("fileswn6.cpp")
        self.assertIn("ProtectedListing->Contains(listedPath)", builder)
        self.assertIn("ProbeLocalElevationAccess(listedPath, readAccess, false) == ERROR_ACCESS_DENIED", builder)
        self.assertIn("ProtectedFileSources->emplace(op.SourceNameW", builder)
        self.assertIn("if (!listedSource && sourcePathSupADS", builder)
        worker = text("worker.cpp")
        listed = section(worker, "static const COperations::ProtectedFileSource* ListedTransferSource(",
                         "static bool TryTransferWithBroker(")
        self.assertIn("OPFL_UAC_TRANSFER_LISTED", listed)
        self.assertIn("sources.find(op.SourceNameW)", listed)
        overwrite = section(worker, "static bool PrepareWorkerOverwrite(", "enum class SourceOverwriteResult")
        self.assertIn("(!listed || !listed->Request)", overwrite)
        self.assertIn("listed ? listed->Request.get() : nullptr", overwrite)
        broker = text("uacbroker/uacbroker.cpp")
        target = section(broker, "bool PreparedLeaf::PrepareTarget(", "bool PreparedLeaf::PrepareZipSelection(")
        self.assertLess(target.index("install.MatchesProcess"), target.index("CaptureChildId("))
        self.assertIn("identityFromListing = transferring", target)
        prepared = section(broker, "bool PreparedTransfer::PrepareSource(", "bool CaptureMoveFileIdentity(")
        self.assertIn("const Operation listedOperation = TransferJobFamily(operation)", prepared)
        replacement = section(broker, "bool PreparedTransfer::PrepareOverwrite(", "OverwriteApproval PreparedTransfer::Approval()")
        self.assertIn("moving ? Operation::MoveOverwrite :", replacement)
        self.assertIn("ignoreAds ? Operation::CopyOverwriteIgnoreAds : Operation::CopyOverwrite", replacement)
        self.assertIn("(moving && ignoreAds)", replacement)
        self.assertIn("(script.IgnoreTransferAds && op.Opcode != ocCopyFile)", overwrite)
        self.assertIn("request.NeedsDiskRecoveryWarning(result)", worker)
        self.assertIn("WORKER_DIALOG_UAC_DISK_RECOVERY_UNKNOWN", worker)
        self.assertIn("LoadStr(IDS_UAC_DISK_RECOVERY_UNKNOWN)", text("dialogs.cpp"))
        self.assertIn("Check the source, destination, and any backup before retrying.", text("lang/texts.rc2"))
        self.assertIn("prompt.request.IgnoresAds() ? IDS_UAC_COPY_OVERWRITE_IGNORE_ADS_CONFIRM", text("dialogs.cpp"))
        self.assertIn("listedSource", replacement)
        self.assertIn("CaptureOverwriteRevision(target.source, overwriteTarget)", replacement)
        move = section(worker, "BOOL DoMoveFile(", "if (!copyAsEncrypted")
        self.assertIn("(!dir && (op->OpFlags & OPFL_UAC_TRANSFER_LISTED))", move)

    def test_directory_move_uses_the_verified_missing_target(self):
        dialog = text("fileswn8.cpp")
        callback = section(dialog, "static bool DeferProtectedTransferTarget(", "static bool StartProtectedTreeCopy(")
        self.assertLess(callback.index("CanMoveDirectoryToParent("), callback.index("state.result ="))
        self.assertIn("deferDirectoryMove ? sourceDirectory.c_str() : nullptr", dialog)
        self.assertIn("HasTheSameRootPathAndVolume(GetPath(), transferChangePath)", dialog)
        move = section(text("worker.cpp"), "BOOL DoMoveFile(", "if (!copyAsEncrypted")
        self.assertIn("if (script->DeferredTransferTarget ||", move)
        self.assertIn("return DoCopyFile(", move)

    def test_branch_relative_target_defers_source_denial_until_parent_creation(self):
        builder = text("fileswn6.cpp")
        direct = section(builder, "CTargetPathState itemTargetState = targetPathState;",
                         "// Every created ancestor")
        self.assertLess(direct.index("branchCreatedDirectories.push_back(createIndex)"),
                        direct.index("BuildScriptFile("))
        self.assertIn("? tpsEncryptedNotExisting : tpsNotEncryptedNotExisting", direct)
        self.assertIn("itemTargetState,", direct)
        discovery = section(builder, "const bool ordinaryTarget =", "int res;")
        self.assertIn("((IsBranchView() || script->DeferredTransferTarget) && targetPathState == tpsNotEncryptedNotExisting)", discovery)
        self.assertIn("!script->OverwriteOlder && ordinaryTarget", discovery)
        self.assertIn("OPFL_UAC_TRANSFER_SOURCE", discovery)

    def test_compressed_move_with_ads_reaches_the_cross_volume_broker(self):
        transfer = section(text("worker.cpp"), "static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite(")
        eligible = transfer.split("request.Prepare(", 1)[0]
        self.assertIn("IsEligibleMoveFile(op.Attr, script.CopyAttrs, script.CopySecurity)", eligible)
        self.assertIn("UacF7::FileTransferOperation(op.Attr, moving, script.CopyAttrs,", eligible)
        self.assertIn("script.IgnoreTransferAds, script.CopySecurity)", eligible)
        selector = section(text("uacbroker/f7pilot.h"), "inline Operation FileTransferOperation(", "inline bool IsPlainDeleteLink(")
        self.assertIn("if (moving)", selector)
        self.assertIn("return copyAttributes ? Operation::MoveCompressed : Operation::Move;", selector)
        self.assertNotIn("OPFL_COPY_ADS", eligible.replace("(op.OpFlags & OPFL_COPY_ADS)", "", 1))
        self.assertIn("moving && !directory && !request.IsCrossVolumeMove()", transfer)
        self.assertIn("!UacF7::IsNativePlainCopyFile(op.Attr)", transfer)
        self.assertNotIn("OPFL_UAC_TRANSFER_SOURCE | OPFL_COPY_ADS", transfer)
    def test_atomic_directory_move_is_a_separate_same_volume_retry(self):
        worker = text("worker.cpp")
        transfer = section(worker, "static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite(")
        self.assertIn("op.Opcode == ocMoveDir", transfer)
        self.assertIn("Operation::MoveDirectory", transfer)
        self.assertIn("!(op.Attr & FILE_ATTRIBUTE_REPARSE_POINT)", transfer)
        move = section(worker, "BOOL DoMoveFile(", "// Novell patch")
        self.assertLess(move.index("MoveFileW("), move.index("if (err == ERROR_ACCESS_DENIED"))
        self.assertIn("ProbeLocalElevationAccess(op->SourceNameW, DELETE, dir != FALSE)", move)
        broker = text("uacbroker/uacbroker.cpp")
        prepared = section(broker, "bool PreparedTransfer::Prepare(", "bool CaptureMoveFileIdentity(")
        self.assertIn("directoryMove && crossMove", prepared)
        rename = text("uacbroker/renameleaf.h")
        self.assertIn("kind != MoveKind::OrdinaryDirectory", rename)
        self.assertIn("return ERROR_NOT_SAME_DEVICE;", rename)

    def test_closed_sessions_do_not_accumulate_during_the_job(self):
        transfer = section(text("worker.cpp"), "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertIn("else if (existing != jobs.end() && !existing->IsOpen())", transfer)
        self.assertIn("jobs.erase(existing);", transfer)
        leaf = section(text("dialogs.cpp"), "case WORKER_DIALOG_UAC_METADATA:", "StatusPaused = FALSE;")
        self.assertIn("(CancelWorker || !existing->IsOpen())", leaf)
        self.assertIn("TransferJobs.erase(existing);", leaf)

    def test_shielded_ok_is_consumed_only_for_the_matching_target_denial(self):
        dialog = section(text("dialogs.cpp"), "case WORKER_DIALOG_UAC_TRANSFER:", "case WORKER_DIALOG_UAC_TRANSFER_UNKNOWN:")
        self.assertIn("prompt.targetDenied && Script->ShieldedTransferTarget", dialog)
        self.assertIn("MatchesTargetDirectory(*Script->ShieldedTransferTarget)", dialog)
        self.assertLess(dialog.index("ShieldedTransferTarget.reset()"), dialog.index("SalMessageBoxEx(&confirm)"))
        self.assertIn("prompt.approved = SalMessageBoxEx(&confirm) == IDOK", dialog)
        self.assertNotIn("IDCONTINUE", dialog)
        worker = text("worker.cpp")
        transfer = section(worker, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertIn("if (!planned && shareJob && existing == jobs.end())", transfer)
        self.assertIn("if (!planned && existing == jobs.end())", transfer)
        self.assertLess(transfer.index("!request.IsInJob(*planned)"), transfer.index("WORKER_DIALOG_UAC_TRANSFER"))
        self.assertIn("return request.IsInJob(job)", transfer)
        transfer_entry = section(worker, "static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite(")
        self.assertIn("HANDLE* input = nullptr, bool targetDenied = false, CreateOnlySourceReceipt* receipt = nullptr)", transfer_entry)
        self.assertIn("request, error, targetDenied, receipt)", transfer_entry)
        self.assertIn("merge ? pending->Target.c_str() : targetText.c_str(), !merge && targetDenied", transfer)
        target_open = section(worker, "!dlgData.SkipAllFileOpenOut && GetLastError() == ERROR_ACCESS_DENIED", "if (in != INVALID_HANDLE_VALUE)")
        self.assertIn("success, transferError, &in, true, &createOnlyReceipt)", target_open)
        self.assertLess(dialog.index("prompt.targetDenied && Script->ShieldedTransferTarget"),
                        dialog.index("ShieldedTransferTarget.reset()"))
        self.assertLess(dialog.index("MatchesTargetDirectory(*Script->ShieldedTransferTarget)"),
                        dialog.index("ShieldedTransferTarget.reset()"))

    def test_shield_and_transfer_share_the_verified_destination_capture(self):
        preview = section(text("fileswn8.cpp"), "struct TransferElevationPreview", "static bool StartProtectedTreeCopy")
        self.assertLess(preview.index("ProbeLocalElevationAccess"), preview.index("CaptureCreateParentIdentity"))
        broker = text("uacbroker/uacbroker.cpp")
        capture = section(broker, "bool PreparedLeaf::CaptureCreateParentIdentity(", "bool PreparedLeaf::PrepareRenameDirectory(")
        self.assertIn("target.Prepare(", capture)
        transfer = section(broker, "bool PreparedTransfer::Prepare(", "bool CaptureMoveFileIdentity(")
        self.assertIn("target.Prepare(targetPath)", transfer)
        target = section(broker, "bool PreparedLeaf::PrepareTarget(", "bool PreparedLeaf::PrepareZipSelection(")
        self.assertIn("requested == Operation::Mkdir", target)
        self.assertIn("openError == ERROR_ACCESS_DENIED && !parentPath.empty()", target)
        self.assertLess(target.index("SameParent(snapshot, visible)"), target.index("CaptureChildDirectoryId"))

    def test_refusing_source_elevation_preserves_ordinary_copy_and_suppresses_target_consent(self):
        planning = section(text("fileswn6.cpp"), "static int ReadProtectedCopyDirectory(", "BOOL CFilesWindow::BuildScriptDir(")
        self.assertIn("if (!error)\n        error = ERROR_NOT_SUPPORTED;", planning)
        self.assertIn("if (plan.ElevationDeclined)\n        return IDYES;", planning)
        self.assertIn("if (choice != IDOK)\n            return choice;", planning)
        self.assertIn("plan.ElevationDeclined = error == ERROR_CANCELLED;", planning)
        worker = text("worker.cpp")
        transfer = section(worker, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        refusal = section(transfer, "if (script.CopySource && script.CopySource->ElevationDeclined)", "const auto sourceDisplay")
        self.assertIn("return false;", refusal)
        self.assertNotIn("error =", refusal)
        copy = section(worker, "BOOL DoCopyFile(", "BOOL DoMoveFile(")
        self.assertLess(copy.index("!dlgData.SkipAllFileOpenOut && GetLastError() == ERROR_ACCESS_DENIED"),
                        copy.index("success, transferError, &in, true, &createOnlyReceipt)"))

    def test_protected_tree_copies_directory_with_an_opened_identity(self):
        broker = text("uacbroker/uacbroker.cpp")
        prepare = section(broker, "bool PreparedTransfer::PrepareSource(", "bool PreparedTransfer::CaptureDeferredTarget(")
        self.assertIn("operation == Operation::CopyDirectoryEntry &&\n                                     listedSource->parent && listedSource->source", prepare)
        self.assertIn("listedSource->operation != listedOperation || (!listedSource->identityFromListing && !openedCopyDirectory)", prepare)
        self.assertIn("std::memcpy(source.sourceId, listedSource->sourceId, sizeof(source.sourceId));", prepare)

    def test_protected_tree_planning_honors_only_a_matching_shielded_target(self):
        planning = section(text("fileswn6.cpp"), "static int ReadProtectedCopyDirectory(", "BOOL CFilesWindow::BuildScriptDir(")
        direct = section(planning, "if (script.ShieldedTransferTarget && opening.MatchesTargetDirectory(*script.ShieldedTransferTarget))", "else\n        {")
        self.assertIn("script.ShieldedTransferTarget.reset();", direct)
        self.assertIn("opening.Execute(owner, nullptr, job.get())", direct)
        self.assertNotIn("consent.Execute()", direct)
        self.assertLess(planning.index("opening.Execute("), planning.index("choice = consent.Execute()"))
        self.assertLess(planning.index("choice = consent.Execute()"), planning.index("plan.Session = std::move(job)"))

    def test_cancelled_elevation_stops_the_batch(self):
        transfer = section(text("worker.cpp"), "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertIn("success = FALSE;", transfer)
        declined = section(transfer, "if (!prompt.approved)", "const HANDLE cancel")
        self.assertIn("jobs.clear();", declined)
        self.assertIn("return true;", declined)
        cancellation = section(transfer, "const bool cancelled =", "else if (existing != jobs.end()")
        self.assertIn("WaitForSingleObject(cancel, 0) == WAIT_OBJECT_0", cancellation)
        self.assertIn("*dlgData.CancelWorker || result.error == ERROR_CANCELLED", cancellation)
        self.assertIn("if (cancelled || result.unknown)", cancellation)
        self.assertIn("jobs.clear();", cancellation)
        self.assertIn("planned->Close();", cancellation)
        unknown = section(transfer, "if (result.unknown)", "if (result.error != ERROR_SUCCESS)")
        self.assertIn("WORKER_DIALOG_UAC_TRANSFER_UNKNOWN", unknown)
        self.assertIn("return true;", unknown)
        self.assertIn("error = cancelled ? ERROR_CANCELLED : result.error;", transfer)
        self.assertIn("return error == ERROR_CANCELLED || *dlgData.CancelWorker != FALSE;", transfer)
    def test_terminal_worker_error_stops_scan_ahead_and_suppresses_queued_prompts(self):
        worker = section(text("worker.cpp"), "unsigned ThreadWorkerBody(void* parameter)", "FreeScript(script);")
        shutdown = worker.rsplit("if (conflictScanner != NULL)", 1)[1]
        self.assertLess(shutdown.index("if (Error)"), shutdown.index("SetEvent(cancelWorkerEvent);"))
        self.assertLess(shutdown.index("SetEvent(cancelWorkerEvent);"), shutdown.index("WaitForSingleObject(conflictScanner"))
        prompt = section(text("dialogs.cpp"), "void CProgressDialog::PromptNextConflict()", "int index = -1;")
        self.assertIn("CancelWorker ||", prompt)
        self.assertIn("WaitForSingleObject(CancelWorkerEvent, 0) == WAIT_OBJECT_0", prompt)

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
        overwrite = section(code, "static bool PrepareWorkerOverwrite(", "enum class SourceOverwriteResult")
        self.assertIn("SameFileRevision(op.ScannedTargetRevision, request.Approval().revision)", overwrite)
        self.assertIn("dlgData.InjectedConflictType != cmctFileExists", overwrite)
        self.assertNotIn("OPFL_COPY_ADS", overwrite)
        broker = section(text("uacbroker/uacbroker.cpp"), "DWORD CopyOverExistingFile(", "DWORD WriteZipUpdate(")
        self.assertIn("StreamPolicy::PreserveNamed", broker)
        self.assertLess(broker.index("CopyOverwriteStreams("), broker.index("UacZipUpdate::Publish("))
        lease = section(code, "BOOL COperations::LeaseNextParallelCopy(", "CConflictLeaseOutcome COperations::FinishParallelCopyLeaseLocked")
        self.assertIn("COperation& op = At(item);", lease)
        self.assertIn("op.ScannedTargetRevision.attributes", lease)
        transfer = section(code, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        stale = section(transfer, "if (error == ERROR_FILE_INVALID", "return error == ERROR_CANCELLED")
        self.assertIn("request.IsOverwrite() && dlgData.ConflictDecisionInjected", stale)
        self.assertIn("dlgData.ConflictTargetChanged = TRUE;", stale)
        self.assertNotIn("totalDone", stale)
    def test_planned_root_cannot_fall_back_to_local_creation_or_merge(self):
        worker = text("worker.cpp")
        create = section(worker, "BOOL DoCreateDir(", "BOOL DoDelete")
        self.assertIn("Session->NeedsTreeRoot()", create)
        self.assertIn("!reviewedSource && !plannedRoot && WorkerCreateDirectory(", create)
        self.assertIn("if (!plannedRoot && (err == ERROR_ALREADY_EXISTS", create)
        transfer = section(worker, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertIn("planned && (!planned->IsOpen() || !request.IsInJob(*planned))", transfer)

    def test_deferred_destination_never_reverts_to_ordinary_mutation(self):
        code = text("worker.cpp")
        transfer = section(code, "static bool TryTransferWithBroker(", "static bool PrepareWorkerOverwrite(")
        self.assertIn("if (!deferred && !plannedRoot)", transfer)
        self.assertIn("operation, listedSource, deferred)", transfer)
        worker = section(code, "BOOL DoCopyFile(COperation* op", "BOOL DoMoveFile(")
        pending = section(worker, "if (deferred ||", "BOOL tgtNameCaseCorrected")
        self.assertIn("!deferred && !copyAsEncrypted", pending)
        self.assertIn("outcome == SourceOverwriteResult::Finished && isMove && skip", pending)
        self.assertIn("TryTransferWithBroker(", pending)
        self.assertIn("return TRUE;", pending)
        execute = section(code, "static bool ExecuteWorkerTransfer(", "static bool TryTransferWithBroker(")
        self.assertIn("script.Count > 1 || request.HasDeferredTarget()", execute)
        broker = text("uacbroker/uacbroker.cpp")
        prepare = section(broker, "bool PreparedTransfer::Prepare(", "bool CaptureMoveFileIdentity(")
        self.assertIn("MatchesTargetDirectory(deferred->identity)", prepare)
        execute = section(broker, "Result PreparedTransfer::Execute(", "ArchiveReadSession::~ArchiveReadSession(")
        self.assertIn("(HasDeferredTarget() || IsScopedTree()) && !job", execute)
        self.assertIn("header.targetSuffixLength", execute)
        self.assertIn("target.leaf + targetSuffix", execute)

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
        deferred = section(worker, "if (deferred || (op->OpFlags & OPFL_UAC_TRANSFER_SOURCE)", "BOOL tgtNameCaseCorrected")
        denied = section(worker, "OPEN_IN_ERROR:", "SKIP_OPEN_IN:")
        for path in (deferred, denied):
            self.assertIn("TryOverwriteDeniedSource(", path)
            self.assertIn("outcome == SourceOverwriteResult::Unavailable &&", path)
            self.assertLess(path.index("TryOverwriteDeniedSource("), path.index("TryTransferWithBroker("))
            self.assertIn("SourceOverwriteResult::Skipped", path)
if __name__ == "__main__":
    unittest.main()
