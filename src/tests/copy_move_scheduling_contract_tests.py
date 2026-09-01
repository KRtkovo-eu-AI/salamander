# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def forbid(text: str, needle: str, description: str) -> None:
    if needle in text:
        raise AssertionError(f"Unexpected {description}: {needle}")


def function_slice(text: str, start: str, end: str) -> str:
    first = text.find(start)
    last = text.find(end, first + len(start))
    if first < 0 or last < 0:
        raise AssertionError(f"Cannot locate function section: {start}")
    return text[first:last]


def main() -> None:
    src = ROOT / "src"
    worker_h = (src / "worker.h").read_text(encoding="utf-8")
    worker_cpp = (src / "worker.cpp").read_text(encoding="utf-8")
    sched_h = (src / "storagesched.h").read_text(encoding="utf-8")
    sched_cpp = (src / "storagesched.cpp").read_text(encoding="utf-8")
    salamdr7 = (src / "salamdr7.cpp").read_text(encoding="utf-8")
    cfgdlg = (src / "cfgdlg.h").read_text(encoding="utf-8")
    dialogs4 = (src / "dialogs4.cpp").read_text(encoding="utf-8")
    dialogs3 = (src / "dialogs3.cpp").read_text(encoding="utf-8")
    dialogs = (src / "dialogs.cpp").read_text(encoding="utf-8")
    dialogs_h = (src / "dialogs.h").read_text(encoding="utf-8")
    fileswn6 = (src / "fileswn6.cpp").read_text(encoding="utf-8")
    fileswn8 = (src / "fileswn8.cpp").read_text(encoding="utf-8")
    mainwnd2 = (src / "mainwnd2.cpp").read_text(encoding="utf-8")
    lang_rc = (src / "lang/lang.rc").read_text(encoding="utf-8")
    lang_rh = (src / "lang/lang.rh").read_text(encoding="utf-8")
    texts_rc2 = (src / "lang/texts.rc2").read_text(encoding="utf-8")

    require(sched_h, "CMS_STORAGE_AWARE", "parallel file-transfer mode")
    require(sched_h, "CMS_SEQUENTIAL", "single-file transfer mode")
    require(sched_h, "COSP_STORAGE_AWARE", "storage-aware operation policy")
    require(sched_h, "COSP_GLOBAL_SEQUENTIAL", "global sequential operation policy")
    require(sched_h, "COSP_ASK", "ask-per-operation policy")
    require(sched_h, "COSO_START_NOW", "per-operation start-now override")
    require(sched_h, "COSO_WAIT_ALL", "per-operation wait-all override")
    require(sched_h, "CSWR_SSD_NVME_STREAM_LIMIT", "structured queue wait reasons")
    require(sched_h, "COPYMOVE_SSD_MAX_WRITES 2", "SSD write concurrency limit")
    require(sched_cpp, "StorageOperationConflictsWithRunning", "pure conflict function")
    require(sched_cpp, "StorageOperationConflictsWithRunningWithLimits", "configurable storage write limits")
    require(sched_cpp, "CopyMoveShouldStartPaused", "mode decision function")
    require(sched_cpp, "StorageUse_GetParallelFileLimit", "endpoint-aware file stream limit")
    require(sched_cpp, "claim->Access == SACCESS_READWRITE",
            "same-device copy remains sequential")
    require(sched_cpp, "network path or unresolved volume", "conservative network/unknown fallback")

    require(worker_h, "COperationStorageUse StorageUse", "per-operation storage claims")
    require(sched_h, "int StreamDemand", "per-operation file stream reservation")
    require(worker_h, "TryResumeCompatible", "resume of compatible waiters")
    require(worker_h, "const COperationStorageUse* storageUse", "AddOperation receives resources")
    add_op = function_slice(
        worker_cpp,
        "BOOL COperationsQueue::AddOperation(",
        "void COperationsQueue::TryResumeCompatible(")
    require(add_op, "StorageOperationGetWaitReason", "AddOperation uses operation scheduling policy")
    require(add_op, "StorageOperationIsFifoBarrier", "AddOperation respects FIFO barriers")
    require(add_op, "runningViewsOverflow", "operations beyond the view limit are safely queued")
    resume = function_slice(
        worker_cpp,
        "void COperationsQueue::TryResumeCompatible(",
        "void COperationsQueue::OperationEnded(")
    require(worker_cpp, "OperPaused[j] == 0", "wait-reason calculation considers running operations")
    require(resume, "GetWaitReasonForIndex", "resume recomputes structured wait reasons")
    require(worker_cpp, "runningViewsOverflow", "scheduler does not ignore operations beyond the view limit")
    require(resume, "PostMessage(OperDlgs[i], WM_COMMAND, CM_RESUMEOPER, 0)",
            "compatible waiters are resumed while others may still run")

    require(salamdr7, "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS", "physical disk extents")
    require(salamdr7, "IOCTL_STORAGE_GET_DEVICE_NUMBER", "device-number fallback")
    require(salamdr7, "StoragePathUtf8OrAcpToWide", "UTF-8 first storage path conversion")
    require(salamdr7, "GetVolumePathNameW", "wide volume root resolution")
    require(salamdr7, "CreateFileW", "wide storage device open")
    require(salamdr7, "AddNetworkShareClaim", "UNC/mapped-drive network identity")
    require(salamdr7, "void AddPathToStorageUse(", "path-to-resource helper")

    require(cfgdlg, "CopyMoveOperationPolicy", "separate operation scheduling field")
    require(cfgdlg, "CopyMoveScheduling", "separate file-transfer preference field")
    require(dialogs4, "CopyMoveOperationPolicy = COSP_STORAGE_AWARE", "storage-aware operation default")
    require(dialogs4, "CopyMoveScheduling = CMTP_STORAGE_AWARE", "parallel transfer default")
    require(mainwnd2, "CONFIG_COPYMOVEOPERATIONPOLICY_REG", "operation policy persistence")
    require(mainwnd2, "CONFIG_COPYMOVESCHEDULING_REG", "transfer preference compatibility persistence")
    require(mainwnd2,
            "BOOL hasCopyMoveConflictPreference = GetValue(actKey, CONFIG_COPYMOVECONFLICTPREFERENCE_REG",
            "conflict preference presence marker for page migration")
    migration_order = function_slice(
        mainwnd2,
        "GetValue(actKey, CONFIG_LASTFOCUSEDPAGE",
        "GetValue(actKey, CONFIG_CONFIGURATION_HEIGHT")
    require(migration_order,
            "if (!hasTabCaptionMode && Configuration.LastFocusedPage >= 2)",
            "Tabs page insertion migration")
    require(migration_order,
            "if (!hasCopyMoveConflictPreference && Configuration.LastFocusedPage >= 4)",
            "File Operations page insertion migration")
    if migration_order.find("!hasTabCaptionMode") > migration_order.find("!hasCopyMoveConflictPreference"):
        raise AssertionError("File Operations migration must follow Tabs insertion migration")
    general_resource = function_slice(
        lang_rc,
        "IDD_CFGPAGE_GENERAL DIALOGEX",
        "IDD_CFGPAGE_FILEOPERATIONS DIALOGEX")
    file_operations_resource = function_slice(
        lang_rc,
        "IDD_CFGPAGE_FILEOPERATIONS DIALOGEX",
        "IDD_CFGPAGE_REGIONAL DIALOGEX")
    forbid(general_resource, "IDC_COPYMOVE_", "copy/move controls on General page")
    require(file_operations_resource, 'CAPTION "File Operations"',
            "localized File Operations page caption")
    require(file_operations_resource, "IDC_COPYMOVE_OPERATION_POLICY",
            "File Operations policy combo")
    require(file_operations_resource, "IDC_COPYMOVE_TRANSFER_PREFERENCE",
            "File Operations transfer preference combo")
    require(file_operations_resource, "IDC_COPYMOVE_CONFLICT_PREFERENCE",
            "File Operations conflict handling combo")
    require(file_operations_resource, "IDC_COPYMOVE_SSD_PARALLEL",
            "File Operations SSD parallel limit")
    require(file_operations_resource, "IDC_COPYMOVE_NVME_PARALLEL",
            "File Operations NVMe parallel limit")
    require(lang_rh, "#define IDD_CFGPAGE_FILEOPERATIONS",
            "File Operations resource dialog ID")
    history_add = dialogs4.find("Add(&PageHistory)")
    file_operations_add = dialogs4.find("Add(&PageFileOperations)")
    recycle_bin_add = dialogs4.find("Add(&PageSystem)")
    if not (0 <= history_add < file_operations_add < recycle_bin_add):
        raise AssertionError("File Operations page is not between History and Recycle Bin")
    forbid(lang_rc, "IDC_COPYMOVE_PARALLEL_WARNING", "inline parallel-files warning label")
    require(texts_rc2, "IDS_COPYMOVE_POLICY_STORAGEAWARE", "localized operation policy names")
    require(dialogs3, "IDC_CM_TRANSFERMODE",
            "per-operation transfer-mode selector")
    require(dialogs3, "OperationSchedulingOverrideInOut",
            "per-operation Start now or Wait choice")
    require(fileswn8, "CopyMoveLastTransferMode",
            "Keep-last preference is applied to the Copy/Move dialog")
    require(worker_h, "CopyMoveTransferMode",
            "transfer mode is captured in the operation script")
    require(worker_h, "OperationSchedulingPolicy", "operation policy is captured separately")
    require(worker_h, "OperationSchedulingOverride", "operation override is captured separately")
    require(worker_h, "CParallelProgressData", "parallel file progress payload")
    require(worker_h, "int DisplayCount", "operation-stable parallel row count")
    require(worker_h, "BOOL Active", "stable parallel slot active state")
    require(worker_cpp, "progress.ActiveCount = batch->SlotCount",
            "parallel snapshots preserve physical slot positions")
    require(worker_cpp, "CParallelProgressStreamData& stream = progress.Streams[slot]",
            "parallel task progress remains bound to its slot")
    require(dialogs, "SetParallelProgressSlotActive(HWindow, index, stream.Active)",
            "inactive slots clear without compacting later rows")
    require(dialogs, "ShowWindow(GetDlgItem(dialog, progressId), SW_SHOWNA)",
            "inactive slots preserve visible empty progress bars")
    require(worker_cpp, "progress.ResetSlots = TRUE;",
            "completed non-refilled slots are cleared promptly")
    require(worker_h, "BOOL ResetSlots", "atomic parallel batch row replacement")
    require(worker_cpp, "RunParallelCopyBatch", "parallel copy dispatcher")
    require(worker_cpp, "batch.ActiveTasks[slot] = next",
            "rolling parallel progress slot reuse")
    forbid(worker_cpp, "ShrinkFinishedSlots", "per-file progress dialog resizing")
    require(worker_cpp, "CreateFileW", "wide API for parallel copy streams")
    forbid(worker_cpp, "FileAllocationInfo",
           "serialized eager extent allocation in parallel copy workers")
    require(worker_cpp, "GetParallelCopyPathW", "UTF-8-first/fallback-ACP stream paths")
    require(worker_cpp, "CanReplaceInParallel", "authorized replacement parallelization")
    require(worker_cpp, "task->ReplaceTarget ? OPEN_EXISTING : CREATE_NEW",
            "authorized in-place replacement")
    require(worker_cpp, "task->DeleteOnFailure", "parallel output ownership tracking")
    require(worker_cpp, "SetFileTime(output", "parallel source timestamp preservation")
    require(worker_cpp, "basicInfo.FileAttributes = task->FinalAttributes",
            "handle-safe parallel source attribute preservation")
    require(worker_cpp, "GetParallelFileIdentity",
            "replacement and source file identity preflight")
    forbid(worker_cpp, "taskThreadCount", "fixed wait-all parallel batches")
    require(worker_cpp, "RunRollingParallelCopy", "rolling parallel copy window")
    require(worker_cpp, "LeaseNextParallelCopy", "coordinator-backed exact copy leases")
    require(worker_cpp, "CompleteParallelCopyLease", "lease completion by index and generation")
    require(worker_cpp, "script->CopyMoveConflictMode == CMCM_SCAN_AHEAD", "scan-ahead rolling lease adapter")
    require(worker_cpp, "if (task.Thread != NULL && !task.Finished)",
            "parallel progress publishes only genuinely active workers")
    require(dialogs, "if (activeCount < 0)",
            "parallel progress accepts a fully drained zero-active snapshot")
    forbid(worker_cpp, "HasUsefulParallelRefillWindow",
           "payload threshold that prematurely drains rolling copy")
    forbid(worker_cpp, "COPYMOVE_MIN_PARALLEL_BATCH_SIZE",
           "size-based fallback from storage-aware to sequential copy")
    require(worker_cpp, "if (initialCount < 2)",
            "parallel copy requires only two eligible files")
    require(worker_cpp, "ProgressCompensated",
            "parallel completion records synthetic progress compensation")
    require(worker_cpp, "completed.Operation->Size - completed.Operation->FileSize",
            "parallel ETA receives minimum-file work units")
    require(worker_cpp, "TRUE, 0, NULL, MAX_OP_FILESIZE",
            "synthetic parallel work updates only the progress meter")
    require(dialogs, "CacheIsDirty = FALSE;",
            "parallel snapshots invalidate stale regular operation text")
    require(dialogs, "OperationProgressCacheIsDirty = FALSE;",
            "parallel snapshots invalidate stale regular file progress")
    require(worker_cpp, "task.ReplaceTarget = replaceTarget",
            "authorized replacements participate in the rolling window")
    require(worker_cpp, "task->ReplaceTarget ? OPEN_EXISTING : CREATE_NEW",
            "replacement target is revalidated before in-place truncation")
    require(worker_cpp, "task->Progress = progress >= 1000 ? 999 : progress",
            "100 percent is published only after file finalization")
    require(worker_cpp, "ParallelHandleHasNamedStreams(output",
            "replacement ADS state is revalidated on the output handle")
    require(worker_cpp, "inputInfo.dwVolumeSerialNumber != task->SourceVolumeSerial",
            "source identity is revalidated after opening")
    require(worker_cpp, "OpenParallelCopyInput(task)",
            "only the next rolling source is pinned before output truncation")
    require(worker_cpp, "task.ReplaceTarget && task.Success",
            "successful replacements survive a later task failure")
    require(worker_cpp, "WaitForMultipleObjects(handleCount, handles, FALSE, 100)",
            "wait-any rolling slot refill")
    require(worker_cpp, "batch.CompletedDone += completed.Operation->Size",
            "rolling committed progress accounting")
    require(worker_cpp, "std::deque<CParallelCopyTask>", "stable rolling task storage")
    require(worker_cpp, "PlanNextParallelCopy", "continuous single-item rolling planner")
    require(worker_cpp, "Allocate the rollback record before CreateDirectoryW",
            "prepared-directory allocation-before-side-effect ordering")
    forbid(worker_cpp, "const int maxItems = 256", "fixed rolling item barrier")
    forbid(worker_cpp, "OpenParallelCopyInputs", "whole-lookahead source preopening")
    require(worker_cpp, "TryPrepareParallelDirectory",
            "noninteractive directory lookahead for rolling copies")
    require(worker_cpp, "operation->Opcode == ocCopyDirTime ||",
            "rolling lookahead crosses deferred directory finalizers")
    require(worker_h, "OPFL_PARALLEL_DONE", "non-contiguous rolling file completion marker")
    require(worker_h, "OPFL_PARALLEL_DIR_PREPARED",
            "speculatively prepared directory marker")
    require(worker_cpp, "return *dialogData.CancelWorker ? -2 : -1",
            "rolling cancellation does not enter legacy retry")
    require(worker_cpp, "if (parallelCount < 0)", "parallel cancellation stops script processing")
    require(worker_cpp, "InterlockedExchange", "thread-safe rolling slot cancellation")
    require(worker_cpp, "copied != task->Operation->FileSize",
            "parallel source size-change detection")
    require(worker_cpp, "DeleteParallelCopyOutput", "file-identity-safe rollback cleanup")
    require(worker_cpp, "GetFileInformationByHandleEx(file, FileBasicInfo",
            "handle-safe read-only rollback cleanup")
    require(worker_cpp, "SetTFSandProgressSize(statusTFS, statusProgress)",
            "atomic parallel status rollback")
    forbid(worker_cpp, "task->Progress = progress > 1000 ? 1000 : progress;\n        UpdateParallelCopyProgress(batch)",
           "copy-thread synchronous dialog updates")
    require(worker_cpp, "targetFileIndex == previous->TargetFileIndex",
            "hard-link and path-alias duplicate target rejection")
    require(worker_cpp, "ParallelHandleHasNamedStreams(output, hasNamedStreams)",
            "ADS replacements use the legacy path")
    require(worker_cpp, "if (!task.DeleteOnFailure || !task.OutputIdentityValid ||",
            "only identity-verified batch outputs are removed after failure")
    forbid(worker_cpp, "ReplaceFileW", "file-identity-changing replacement")
    require(worker_cpp, "OperPolicies", "queue stores per-operation scheduling policy")
    require(worker_cpp, "OperOverrides", "queue stores per-operation override")
    require(worker_cpp, "OperWaitReasons", "queue stores structured wait reasons")
    require(dialogs, "Script->OperationSchedulingPolicy", "progress dialog registers operation policy")
    require(dialogs, "Script->OperationSchedulingOverride", "progress dialog registers operation override")
    require(dialogs, "&Script->StorageUse", "progress dialog registers storage use")
    require(dialogs, "Script->StorageUse.StreamDemand", "progress dialog reserves file stream budget")
    progress_init = function_slice(dialogs, "case WM_INITDIALOG:", "case WM_USER_PROGRDLGSTART:")
    progress_start = function_slice(dialogs, "case WM_USER_PROGRDLGSTART:", "case WM_TIMER:")
    require(progress_init, "LayoutActiveProgressStreams(Script->StorageUse.StreamDemand)",
            "initial stream layout before delayed worker start")
    forbid(progress_start, "GetCopyOperationStreamDemand", "stream-demand recomputation after initialization")
    forbid(progress_start, "LayoutActiveProgressStreams(Script->StorageUse.StreamDemand)",
           "initial stream layout after initialization")
    require(dialogs, "StorageUse_GetEndpointLabel", "progress title contains source/destination storage")
    require(dialogs_h, "SchedulingTitleSuffix", "progress dialog keeps a file-transfer title suffix")
    require(dialogs_h, "QueueWaitReason", "progress dialog stores queue wait reason")
    require(dialogs, "IDS_PROGDLGQUEUE_STREAMLIMIT", "progress dialog distinguishes stream-limit waits")
    require(dialogs_h, "LayoutActiveProgressStreams", "dynamic parallel progress dialog layout")
    require(dialogs_h, "CProgressTemplateLayout", "non-cumulative progress dialog layout baseline")
    require(dialogs, "const int centerY = dialogRect.top +",
            "progress dialog captures its current vertical center")
    require(dialogs, "dialogRect.top = centerY - requiredHeight / 2",
            "progress dialog grows evenly upward and downward")
    require(dialogs, "MultiMonEnsureRectVisible(&dialogRect, FALSE)",
            "expanded progress dialog remains inside the monitor work area")
    require(dialogs, "dialogRect.bottom = dialogRect.top + requiredHeight",
            "monitor adjustment preserves required dialog height")
    forbid(function_slice(dialogs, "void CProgressDialog::LayoutActiveProgressStreams", "static void SetProgressStaticTextIfChanged"), "WM_SETREDRAW", "progress stream parent redraw freeze")
    require(dialogs, "SWP_NOREDRAW", "atomic progress stream window repositioning")
    forbid(function_slice(dialogs, "void CProgressDialog::LayoutActiveProgressStreams", "static void SetProgressStaticTextIfChanged"), "LockWindowUpdate", "global progress layout update lock")
    require(dialogs, "SetProgressStaticTextIfChanged", "stable progress labels during chunk updates")
    require(dialogs, "LayoutActiveProgressStreams(displayCount, FALSE)",
            "parallel layout and content update share one redraw transaction")
    require(dialogs, "RDW_ERASE", "vacated progress layout areas are erased")
    require(dialogs, "data != NULL && progressLParam != 1", "regular progress layout reset only at operation boundaries")
    require(dialogs, "if (ActiveParallelProgressStreams == 1)\n                LayoutActiveProgressStreams(1)",
            "parallel layout remains pinned across sequential metadata boundaries")
    require(dialogs, "!ParallelProgressActive", "regular progress does not overwrite parallel file progress")
    require(lang_rc, "WS_CLIPCHILDREN", "progress dialog child repaint isolation")
    require(dialogs, "DelayParallelProgressShow", "no one-stream flash before parallel progress is ready")
    require(dialogs, "IDC_PROGRESS_STREAM8_BAR", "progress dialog supports up to eight visible streams")
    require(texts_rc2, "IDS_COPYMOVE_SCHED_TITLE_STORAGEAWARE", "localized progress mode name")
    require(cfgdlg, "CopyMoveSsdParallelFiles", "SSD stream limit configuration")
    require(cfgdlg, "CopyMoveNvmeParallelFiles", "NVMe stream limit configuration")
    require(dialogs4, "CopyMoveSsdParallelFiles = 2", "default SSD stream limit")
    require(dialogs4, "CopyMoveNvmeParallelFiles = 4", "default NVMe stream limit")
    require(cfgdlg, "class CCfgPageFileOperations : public CCommonPropSheetPage",
            "File Operations property-page class")
    require(dialogs4, "CCfgPageFileOperations::LayoutCopyMoveControls",
            "resizable File Operations page controls")
    require(dialogs4, "IDC_COPYMOVE_SSD_WARNING_ICON", "SSD parallel-files warning icon")
    require(dialogs4, "IDC_COPYMOVE_NVME_WARNING_ICON", "NVMe parallel-files warning icon")
    require(dialogs4, "CopyMoveWarningToolTip", "parallel-files warning tooltip")
    require(dialogs4, "TTF_IDISHWND", "warning tooltip is attached to each icon")
    require(dialogs4, "IDS_COPYMOVE_PARALLEL_SSD_WARNING", "localized SSD tooltip text")
    require(dialogs4, "IDS_COPYMOVE_PARALLEL_NVME_WARNING", "localized NVMe tooltip text")
    require(file_operations_resource, "IDC_COPYMOVE_SSD_WARNING_ICON,148,52,9,14,WS_GROUP | SS_NOTIFY",
            "hover-enabled SSD warning icon")
    require(file_operations_resource, "IDC_COPYMOVE_NVME_WARNING_ICON,148,67,9,14,WS_GROUP | SS_NOTIFY",
            "hover-enabled NVMe warning icon")
    require(mainwnd2, "CONFIG_COPYMOVESSDPARALLELFILES_REG", "SSD stream limit persistence")
    require(mainwnd2, "CONFIG_COPYMOVENVMEPARALLELFILES_REG", "NVMe stream limit persistence")
    require(worker_cpp, "Configuration.CopyMoveSsdParallelFiles", "configured SSD limit is used by the scheduler")
    require(worker_cpp, "Configuration.CopyMoveNvmeParallelFiles", "configured NVMe limit is used by the scheduler")
    require(worker_cpp, "script->GetSpeedLimit", "speed-limited copies retain the legacy transfer path")
    require(worker_cpp, "ParallelCopyDisabled", "dynamic speed-limit stream reservation safety")
    require(worker_cpp, "GetCopyOperationStreamDemand", "eligible batch stream reservation")
    require(worker_cpp, "limit < 2 || script->IsParallelCopyDisabled()",
            "persistent dynamic speed-limit fallback")
    require(worker_cpp, "AddParallelCopyBytes", "parallel transfer speed accounting")
    require(worker_cpp, "script->CopyMoveConflictMode == CMCM_SCAN_AHEAD",
            "scan-ahead capability-demand branch")
    require(worker_cpp, "for (int index = 0; index < script->Count && eligible < limit; ++index)",
            "scan-ahead demand crosses directory and conflict barriers")
    require(worker_cpp, "return eligible >= 2 ? eligible : 1;",
            "scan-ahead reserves configured capability only for two eligible files")
    require(worker_cpp, "if (count >= 2 && count > demand)",
            "legacy stream reservation remains contiguous")

    require(fileswn8, "type == atMove ? SACCESS_READWRITE : SACCESS_READ",
            "F5/F6 Move source includes delete writes")
    require(fileswn8, "copyToSelectedDirs", "copy-to-selected-dirs destinations")
    require(fileswn8, "script->AddStoragePath(selectedTarget->c_str(), SACCESS_WRITE)",
            "each selected destination is a write resource")
    require(fileswn6, "copy ? SACCESS_READ : SACCESS_READWRITE",
            "drop Move source includes delete writes")
    require(fileswn6, "new (std::nothrow) COperations(",
            "drop operation allocation has nonthrowing low-memory behavior")
    require(fileswn6, "free(waitSubjectCopy);",
            "drop operation allocation failure releases constructor arguments")
    require(fileswn6, "free(sourceDirCopy);",
            "drop operation allocation failure releases source argument")
    require(fileswn6, "free(targetPathCopy);",
            "drop operation allocation failure releases target argument")
    require(fileswn6, "RESTORE_DROP_OPERATION_DEBUG_NEW_MACRO",
            "drop operation restores the CRT debug new macro")
    require(dialogs4, "MAKEINTRESOURCEW((ULONG_PTR)IDI_EXCLAMATION)",
            "warning icon uses a wide resource identifier")
    forbid(dialogs4, "(PCWSTR)IDI_EXCLAMATION",
           "byte resource pointer cast to wide string")
    require(fileswn6, "script->AddStoragePath(targetPath, SACCESS_WRITE)",
            "drop destination")

    cfg = cfgdlg
    conflict_dialogs = dialogs3 + dialogs4
    files = fileswn8
    conflict_worker_h = worker_h
    conflict_worker = worker_cpp
    progress = dialogs
    resources = lang_rc + texts_rc2

    assert "CMCP_CURRENT = 0" in cfgdlg
    assert "CMCP_SCAN_AHEAD = 1" in cfgdlg
    assert "CMCP_KEEP_LAST = 2" in cfgdlg
    assert "CopyMoveConflictPreference = CMCP_CURRENT" in dialogs4
    assert "IDC_COPYMOVE_CONFLICT_PREFERENCE" in conflict_dialogs
    assert "IDC_CM_CONFLICTMODE" in conflict_dialogs
    require(files, "script->CopyMoveConflictMode = conflictMode",
            "native disk copy/move enables selected scan-ahead mode")
    require(files, "conflictMode = CMCM_CURRENT",
            "archive and plugin paths remain on legacy conflict handling")
    assert "IDS_COPYMOVE_CONFLICT_MOVE_WARNING" in files
    require(conflict_worker_h, "enum CCopyMoveItemState", "typed item states")
    require(conflict_worker_h, "enum CCopyMoveConflictType", "typed conflict types")
    require(conflict_worker_h, "enum CCopyMoveConflictDecision", "typed conflict decisions")
    require(conflict_worker_h, "CRITICAL_SECTION ConflictCS", "synchronized coordinator state")
    require(conflict_worker, "InitializeConflictDependencies", "directory dependency graph")
    require(conflict_worker, "ocLabelForSkipOfCreateDir", "directory subtree boundary labels")
    require(conflict_worker, "ConflictScannerBody", "read-only scanner thread")
    require(conflict_worker, "FILE_READ_ATTRIBUTES", "metadata-only scanner access")
    require(conflict_worker, "WorkerNotSuspended", "scanner pause support")
    require(conflict_worker, "GetNextConflictReadyItem", "ready-only scheduling")
    require(conflict_worker, "BOOL CanUseParallelCopy",
            "conflict-mode-neutral parallel eligibility")
    require(conflict_worker, "initialLeaseGeneration",
            "scan-ahead initial lease handoff")
    require(conflict_worker, "PlanNextParallelCopy",
            "shared rolling planner for initial and refill leases")
    require(conflict_worker, "RevalidateConflictItem", "pre-side-effect revalidation")
    require(conflict_worker, "ScannedTargetFileIndex", "identity-sensitive target snapshot")
    require(conflict_worker, "ConflictApplyAll", "typed future apply-all decisions")
    require(conflict_worker, "op.Opcode == ocCreateDir && type == cmctDirectoryExists", "automatic scan-ahead directory merge admission")
    require(conflict_worker, "op.ConflictDecision = cmcdOverwrite;", "directory merge preparation authorization")
    require(conflict_worker_h, "IsPreparedParentDirectoryLocked",
            "shared prepared-parent dependency helper")
    require(conflict_worker_h, "AreConflictAncestorsPreparedLocked",
            "shared ancestor dependency traversal")
    require(conflict_worker, "RevalidatePreparedConflictAncestors",
            "prepared ancestor identity validation before side effects")
    require(conflict_worker, "WaitForConflictChange(100)",
            "event-driven rolling scan refill")
    planner = function_slice(
        conflict_worker, "static BOOL PlanNextParallelCopy",
        "static int RunRollingParallelCopy")
    rolling = function_slice(
        conflict_worker, "static int RunRollingParallelCopy",
        "static int RunParallelCopyBatch")
    forbid(planner, "GetDeferredConflictCount",
           "pending-conflict planner retry justification")
    forbid(rolling, "GetDeferredConflictCount",
           "pending-conflict rolling wait justification")
    require(planner, "initialLeaseInvalidated",
            "planner distinguishes invalid initial leases from serial fallback")
    require(planner, "!script->HasReadySerialConflictWork()",
            "rolling yields to ready serial coordinator work")
    forbid(rolling, "plannerFinished", "sticky scan-ahead terminal planner state")
    require(conflict_worker, "IsConflictCoordinatorComplete()",
            "worker uses the full coordinator completion predicate")
    require(rolling, "CloseUnusedParallelCopyInputs(tasks);",
            "one-task fallback closes its pinned source")
    require(rolling, "RollbackPreparedParallelDirectories(script, preparedDirectories);",
            "one-task fallback rolls back speculative directories")
    for function_start, function_end in (
        ("BOOL COperations::GetNextConflictPromptItem", "void COperations::SetConflictChangedEvent"),
        ("BOOL COperations::IsConflictItemReadyLocked", "void COperations::RebuildConflictQueuesLocked"),
        ("void COperations::AdmitConflictSubtreeLocked", "BOOL COperations::BeginConflictScan"),
        ("void COperations::PublishConflictScanResult", "BOOL COperations::GetNextConflictRequest"),
    ):
        dependency_section = function_slice(conflict_worker, function_start, function_end)
        forbid(dependency_section, "for (int parent = op.ConflictParent",
               f"raw parent-state-only dependency traversal in {function_start}")

    require(conflict_worker_h, "CConflictLeaseOutcome", "explicit centralized lease outcomes")
    require(conflict_worker, "FinishParallelCopyLeaseLocked", "atomic lease transition helper")
    require(conflict_worker, "RetractConflictSubtreeLocked", "whole-subtree dependency retraction")
    require(conflict_worker, "ValidateParallelCopyLeaseForSideEffect", "final lease dependency validation")
    require(conflict_worker, "MarkConflictDirectoryPrepared", "coordinator prepared-directory state")
    require(conflict_worker, "RollbackConflictDirectoryPrepared", "truthful prepared-directory rollback")
    require(conflict_worker, "if (!committed)", "parallel done marker guarded by accepted commit")
    require(conflict_worker, "if (!skipped && At(index).Opcode == ocCreateDir)",
            "all successful directory completions admit probed descendants")
    forbid(conflict_worker, "*item < startIndex", "monotonic scan-ahead starvation filter")
    require(conflict_worker_h, "#include <vector>", "direct vector dependency")
    require(conflict_worker_h, "cmisSuppressed", "terminal suppressed subtree state")
    require(conflict_worker, "At(item).ConflictState == cmisConflict && DeferredConflictCount > 0",
            "suppressed descendant conflict accounting")
    require(conflict_worker, "op.ConflictState == cmisDone || op.ConflictState == cmisSuppressed",
            "scanner terminal-state publication guard")
    require(conflict_worker, "op.ConflictIsFinalizer", "post-order finalizer dependency metadata")
    require(conflict_worker, "At(dependency).ConflictState != cmisDone",
            "finalizers wait for terminal descendants")
    require(conflict_worker, "injectConflictDecision",
            "overwrite state restoration is scan-ahead decision scoped")
    require(progress, "Script->CopyMoveConflictMode != CMCM_SCAN_AHEAD",
            "scan-ahead conflict panel is never startup-hidden")
    require(progress, "IDS_COPYMOVE_CONFLICT_COLUMN_TYPE",
            "localized conflict type column")
    require(progress, "ListView_InsertColumn(HConflictOperations, 2",
            "status column follows conflict type")
    require(progress, "ListView_SetItemText(HConflictOperations, row, 1",
            "conflict type is populated in the middle column")
    require(progress, "ConflictPromptActive",
            "one-at-a-time conflict prompt guard")
    require(progress, "PromptNextConflict",
            "automatic conflict prompt flow")
    require(dialogs_h, "ConflictAutomaticPromptConsumed",
            "explicit one-automatic-prompt lifetime state")
    require(progress, "ConflictAutomaticPromptConsumed = TRUE;",
            "first prompt is consumed when opened even if dismissed")
    require(progress, "int index = GetConflictActionIndex();",
            "manual conflict actions retain dismissed prompt index")
    require(progress, "return selectedIndex >= 0 ? selectedIndex : ConflictPromptIndex;",
            "explicit list selection takes priority over a dismissed automatic prompt")
    require(progress, "BOOL selected = GetConflictActionIndex() >= 0;",
            "manual conflict actions remain enabled for retained prompt")
    require(progress, "SetConflictPanelExpanded(!ConflictPanelExpanded)",
            "conflict scan link toggles the panel")
    require(progress, "previousVisibleCount == 0 && pending > 0",
            "panel auto-expands only when conflicts first appear")
    require(progress, "pending == 0",
            "panel collapses when the conflict list becomes empty")
    forbid(function_slice(progress, "void CProgressDialog::SetConflictPanelExpanded", "void CProgressDialog::RefreshConflictOperations"), "WM_SETREDRAW",
           "conflict panel parent redraw freeze")
    require(worker_cpp, "script->SetConflictProgressWindow(data->HProgressDlg);",
            "scan coordinator is connected to progress UI before scanning")
    require(worker_cpp, "if (!script->BeginConflictScan())",
            "scanner active state is published before thread execution")
    require(worker_cpp, "PostMessage(progressWindow, WM_USER_PROGRDLG_CONFLICT_CHANGED",
            "coalesced event-driven conflict UI notification")
    require(progress, "case WM_USER_PROGRDLG_CONFLICT_CHANGED:",
            "progress dialog handles scanner changes independently of timers")
    require(progress, "Script->AcknowledgeConflictProgressNotification();",
            "coalesced scanner notification acknowledgement")
    require(progress, "LayoutConflictPanelExpanded == ConflictPanelExpanded",
            "panel height changes only on state transitions")
    require(progress, "&TemplateLayout.ConflictLink",
            "layout baseline includes the conflict scan link")
    require(dialogs_h, "RECT Footer[3]",
            "layout baseline has one slot per repositioned footer control")
    require(progress, "if (retainedPrompt)",
            "manual decision clears retained dismissed prompt")
    require(dialogs_h, "int ConflictSelectionRowAfterRemoval;",
            "one-shot conflict selection handoff state")
    require(progress, "const BOOL selectedAction = selectedRow >= 0 && index == GetSelectedConflictIndex();",
            "selection advances only for an explicitly selected conflict")
    require(progress, "const int row = min(selectionRowAfterRemoval, (int)conflicts.size() - 1);",
            "next conflict row or final fallback selection")
    require(progress, "ListView_EnsureVisible(HConflictOperations, row, FALSE);",
            "automatically selected next conflict remains visible")
    require(lang_rc, "IDD_PROGRESSDLG DIALOGEX 10, 30, 355, 110", "compact legacy progress template")
    require(lang_rc, "IDD_PROGRESSDLG_SCANAHEAD DIALOGEX 10, 30, 355, 225", "scan-ahead progress template")
    require(lang_rh, "#define IDD_PROGRESSDLG_SCANAHEAD", "scan-ahead progress resource id")
    require(progress, "IDD_PROGRESSDLG_SCANAHEAD : IDD_PROGRESSDLG", "mode-selected progress template")
    require(worker_cpp, "AllocateParallelProgressMessage(progress)",
            "owned scan-ahead parallel progress payload")
    require(worker_cpp, "PostMessage(batch->ProgressDialog, WM_USER_SETDIALOG",
            "nonblocking scan-ahead progress publication")
    forbid(worker_cpp, "PostMessage(batch->ProgressDialog, WM_USER_SETDIALOG, (WPARAM)&progress",
           "stack-backed posted parallel progress payload")
    require(progress, "FreeParallelProgressMessage((CParallelProgressData*)progressWParam)",
            "owned progress payload release on delivery")
    require(worker_cpp, "dlgData.AsyncProgressPublication =",
            "explicit scan-ahead regular-progress publication policy")
    set_progress_dialog = function_slice(
        worker_cpp,
        "void SetProgressDialog(",
        "int CaclProg(")
    require(set_progress_dialog, "AllocateProgressMessage(*data)",
            "owned scan-ahead regular progress payload")
    require(set_progress_dialog, "PostMessage(hProgressDlg, WM_USER_SETDIALOG,",
            "scan-ahead operation text never waits on modal UI")
    require(set_progress_dialog, "(WPARAM)posted, 3",
            "distinct owned regular progress discriminator")
    require(set_progress_dialog, "else\n            SendMessage(hProgressDlg, WM_USER_SETDIALOG, (WPARAM)data, 0);",
            "current conflict mode retains synchronous operation text publication")
    forbid(set_progress_dialog, "PostMessage(hProgressDlg, WM_USER_SETDIALOG, (WPARAM)data",
           "stack-backed regular progress publication")
    require(progress, "FreeProgressMessage(data);",
            "owned regular progress release after cache copy")
    set_dialog_handler = function_slice(progress, "case WM_USER_SETDIALOG:", "//--- worker request to show a dialog")
    require(set_dialog_handler, "while (PeekMessage(&queuedProgress, HWindow, WM_USER_SETDIALOG,",
            "scan-ahead progress backlog coalescing")
    require(set_dialog_handler, "FreeParallelProgressMessage((CParallelProgressData*)progressWParam);",
            "discarded parallel progress payload release")
    require(set_dialog_handler, "FreeProgressMessage((CProgressData*)progressWParam);",
            "discarded regular progress payload release")
    forbid(set_dialog_handler, "RefreshConflictOperations();",
           "expensive conflict list rebuild on every transfer progress message")
    modal_progress = function_slice(
        progress,
        "void CProgressDialog::BeginConflictPromptPresentation",
        "void CProgressDialog::PromptNextConflict")
    forbid(modal_progress, "WM_SETREDRAW", "modal parent redraw freeze")
    forbid(modal_progress, "EnumChildWindows", "modal child redraw freeze")
    forbid(modal_progress, "LockWindowUpdate", "owned-dialog update lock")
    require(progress, "if (ConflictPromptActive)",
            "modal progress payload cache path")
    require(progress, "FreeParallelProgressMessage((CParallelProgressData*)progressWParam)",
            "modal owned parallel payload release")
    require(modal_progress, "SetParallelProgress(parallel, FALSE, FALSE)",
            "single latest parallel snapshot flush after prompt")
    require(modal_progress, "ConflictPromptActive = FALSE;",
            "presentation resumes before deferred controls are updated")
    require(progress, "pendingProgress.lParam == 3",
            "owned regular progress teardown drain")
    require(worker_cpp, "if (dlgData.AsyncProgressPublication)\n            PostMessage(hProgressDlg, WM_USER_SETDIALOG, 0, 0);",
            "numeric scan-ahead progress never waits on modal UI")
    require(conflict_worker_h, "GetNextConflictPromptItem",
            "dependency-eligible conflict prompt selection")
    require(conflict_worker, "item == excludedIndex",
            "dismissed conflict is not immediately reopened")
    require(progress, "const int footerY = transferY + conflictY;",
            "pixel conflict extension combined with mapped transfer exactly once")
    forbid(progress, "MapDialogRect(HWindow, &collapsed)",
           "double mapping of captured pixel conflict extension")
    bulk_actions = function_slice(
        progress,
        "if (command == IDC_PROGRESS_OVERWRITE_ALL || command == IDC_PROGRESS_SKIP_ALL)",
        "if (WorkerNotSuspended == NULL || Worker == NULL)")
    require(bulk_actions, "Script->SetOperationWideConflictDecision(",
            "progress bulk buttons use operation-wide policy")
    forbid(bulk_actions, "GetSelectedConflictIndex",
           "operation-wide bulk policy selection dependency")
    require(conflict_worker_h, "SetOperationWideConflictDecision",
            "dedicated operation-wide conflict policy API")
    operation_bulk = function_slice(
        conflict_worker,
        "void COperations::SetOperationWideConflictDecision",
        "BOOL COperations::GetNextConflictReadyItem")
    require(operation_bulk, "for (int type = cmctFileExists; type <= cmctTypeMismatch; ++type)",
            "all deferrable future conflict policies")
    require(operation_bulk, "op.ConflictState != cmisConflict",
            "all currently pending conflict categories")
    require(progress, "IDC_PROGRESS_OVERWRITE_ALL), pending > 0",
            "bulk overwrite enabled independently of selection")
    require(progress, "IDC_PROGRESS_SKIP_ALL), pending > 0",
            "bulk skip enabled independently of selection")
    require(progress, "ret == IDB_ALL || ret == IDB_SKIPALL",
            "Solve dialog maps typed apply-all results")
    require(progress, "CreateFileW(targetPath.c_str(), FILE_READ_ATTRIBUTES",
            "wide solve metadata target open")
    require(conflict_worker, "HANDLE CancelWorkerEvent", "scanner cancellation event")
    require(conflict_worker, "WaitForMultipleObjects(2, waits",
            "scanner cancellation and pause wait")
    require(conflict_worker, "Windows still permits a narrow pathname",
            "documented residual legacy pathname race")
    require(conflict_worker, "dlgData.InjectedTargetIdentityValid",
            "authorized overwrite carries scanned identity")
    require(conflict_worker, "GetFileInformationByHandle(out, &openedInfo)",
            "move checks the actual opened target handle")
    require(conflict_worker, "openedFileIndex != dlgData.InjectedTargetFileIndex",
            "copy refuses an unexpectedly replaced target")
    require(fileswn6, "type == atMove && script->CopyMoveConflictMode == CMCM_SCAN_AHEAD",
            "scan-ahead disables whole-directory fast move generation")
    require(fileswn6, "fastDirectoryMove = FALSE",
            "scan-ahead moves expand for truthful directory merge semantics")
    forbid(conflict_worker, "op->Opcode == ocCreateDir || op->Opcode == ocMoveDir",
           "whole-directory MoveFile conflicts are not advertised as mergeable")
    require(conflict_worker, "ValidateInjectedTargetIdentity(op, dlgData)",
            "destructive pathname boundaries validate expected identity")
    require(conflict_worker, "if (!ValidateInjectedTargetIdentity(op, dlgData))\n                                    {\n                                        dlgData.ConflictTargetChanged = TRUE;\n                                        return TRUE;\n                                    }\n                                    BOOL chAttr = ClearReadOnlyAttr",
            "copy validates before clearing target attributes")
    require(conflict_worker, "if (!ValidateInjectedTargetIdentity(op, dlgData))\n                    {\n                        dlgData.ConflictTargetChanged = TRUE;\n                        return TRUE;\n                    }\n                    ClearReadOnlyAttr",
            "move validates before clearing target attributes")
    require(conflict_worker, "script->RequeueChangedConflictItem(i)",
            "identity changes requeue rather than cancel")
    require(conflict_worker, "op.ConflictDecision = cmcdNone",
            "changed targets require a new user decision")
    require(progress, '#include "common/widepath.h"',
            "progress conflict metadata uses shared wide path API")
    require(progress, "SalMultiByteToWidePath(operation.TargetName",
            "progress target UTF-8/ACP wide conversion")
    forbid(progress, "SalMultiByteToWidePathUtf8OrAcp",
           "fileswn anonymous wide conversion helper")
    forbid(conflict_worker, "conflictPass", "unsafe two-pass execution")
    assert "IDC_PROGRESS_OPERATIONS" in progress
    assert "IDC_PROGRESS_OVERWRITE_ALL" in progress
    assert "IDC_PROGRESS_SKIP_ALL" in progress
    assert "IDS_COPYMOVE_CONFLICT_SCAN_STATUS" in resources

    print("copy_move_scheduling_contract_tests: ok")




if __name__ == "__main__":
    main()
