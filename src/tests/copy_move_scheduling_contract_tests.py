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
    require(lang_rc, "IDC_COPYMOVE_OPERATION_POLICY", "General operation policy combo")
    require(lang_rc, "IDC_COPYMOVE_TRANSFER_PREFERENCE", "General transfer preference combo")
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
    require(dialogs, "StorageUse_GetEndpointLabel", "progress title contains source/destination storage")
    require(dialogs_h, "SchedulingTitleSuffix", "progress dialog keeps a file-transfer title suffix")
    require(dialogs_h, "QueueWaitReason", "progress dialog stores queue wait reason")
    require(dialogs, "IDS_PROGDLGQUEUE_STREAMLIMIT", "progress dialog distinguishes stream-limit waits")
    require(dialogs_h, "LayoutActiveProgressStreams", "dynamic parallel progress dialog layout")
    require(dialogs, "ParallelLayoutBaseDialogHeight", "non-cumulative progress dialog layout baseline")
    require(dialogs, "const int centerY = dialogRect.top +",
            "progress dialog captures its current vertical center")
    require(dialogs, "dialogRect.top = centerY - dialogHeight / 2",
            "progress dialog grows evenly upward and downward")
    require(dialogs, "MultiMonEnsureRectVisible(&dialogRect, FALSE)",
            "expanded progress dialog remains inside the monitor work area")
    require(dialogs, "dialogRect.top = dialogRect.bottom - dialogHeight",
            "oversized progress dialog keeps its footer visible")
    require(dialogs, "WM_SETREDRAW", "flicker-free progress dialog stream layout")
    require(dialogs, "SWP_NOREDRAW", "atomic progress stream window repositioning")
    require(dialogs, "LockWindowUpdate(HWindow)", "progress layout update lock")
    require(dialogs, "SetProgressStaticTextIfChanged", "stable progress labels during chunk updates")
    require(dialogs, "LayoutActiveProgressStreams(displayCount, FALSE)",
            "parallel layout and content update share one redraw transaction")
    require(dialogs, "RDW_ERASE", "vacated progress layout areas are erased")
    require(dialogs, "data != NULL && lParam != 1", "regular progress layout reset only at operation boundaries")
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
    require(dialogs4, "LayoutCopyMoveControls", "resizable General page copy/move controls")
    require(dialogs4, "IDC_COPYMOVE_SSD_WARNING_ICON", "SSD parallel-files warning icon")
    require(dialogs4, "IDC_COPYMOVE_NVME_WARNING_ICON", "NVMe parallel-files warning icon")
    require(dialogs4, "CopyMoveWarningToolTip", "parallel-files warning tooltip")
    require(dialogs4, "TTF_IDISHWND", "warning tooltip is attached to each icon")
    require(dialogs4, "IDS_COPYMOVE_PARALLEL_SSD_WARNING", "localized SSD tooltip text")
    require(dialogs4, "IDS_COPYMOVE_PARALLEL_NVME_WARNING", "localized NVMe tooltip text")
    require(lang_rc, "IDC_COPYMOVE_SSD_WARNING_ICON,114,273,9,14,WS_GROUP | SS_NOTIFY",
            "hover-enabled SSD warning icon")
    require(lang_rc, "IDC_COPYMOVE_NVME_WARNING_ICON,264,273,10,14,WS_GROUP | SS_NOTIFY",
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
    require(worker_cpp, "if (count >= 2 && count > demand)",
            "stream reservation is independent of aggregate file size")

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

    print("copy_move_scheduling_contract_tests: ok")


if __name__ == "__main__":
    main()
