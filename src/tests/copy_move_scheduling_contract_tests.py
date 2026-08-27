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

    require(sched_h, "CMS_STORAGE_AWARE", "storage-aware scheduling mode")
    require(sched_h, "CMS_SEQUENTIAL", "sequential scheduling mode")
    require(sched_h, "CMS_MANUAL", "manual scheduling mode")
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
    require(add_op, "CopyMoveShouldStartPaused", "AddOperation uses the scheduler policy")
    require(add_op, "runningViewsOverflow", "operations beyond the view limit are safely queued")
    resume = function_slice(
        worker_cpp,
        "void COperationsQueue::TryResumeCompatible(",
        "void COperationsQueue::OperationEnded(")
    require(resume, "OperPaused[j] == 0", "resume considers currently running operations")
    require(resume, "StorageOperationConflictsWithRunning", "storage-aware resume uses conflict checks")
    require(resume, "runningViewsOverflow", "resume does not ignore operations beyond the view limit")
    require(resume, "PostMessage(OperDlgs[i], WM_COMMAND, CM_RESUMEOPER, 0)",
            "compatible waiters are resumed while others may still run")

    require(salamdr7, "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS", "physical disk extents")
    require(salamdr7, "IOCTL_STORAGE_GET_DEVICE_NUMBER", "device-number fallback")
    require(salamdr7, "StoragePathUtf8OrAcpToWide", "UTF-8 first storage path conversion")
    require(salamdr7, "GetVolumePathNameW", "wide volume root resolution")
    require(salamdr7, "CreateFileW", "wide storage device open")
    require(salamdr7, "AddNetworkShareClaim", "UNC/mapped-drive network identity")
    require(salamdr7, "void AddPathToStorageUse(", "path-to-resource helper")

    require(cfgdlg, "CopyMoveScheduling", "configuration field")
    require(dialogs4, "CopyMoveScheduling = CMS_STORAGE_AWARE", "storage-aware default")
    require(mainwnd2, "CONFIG_COPYMOVESCHEDULING_REG", "registry persistence")
    require(lang_rc, "IDC_COPYMOVE_SCHEDULING", "General page combo")
    forbid(lang_rc, "IDC_COPYMOVE_PARALLEL_WARNING", "inline parallel-files warning label")
    require(texts_rc2, "IDS_COPYMOVE_SCHED_STORAGEAWARE", "localized mode names")
    require(dialogs3, "IDC_CM_TRANSFERMODE",
            "per-operation transfer-mode selector")
    require(dialogs3, "IDC_CM_STARTONIDLE), TRUE",
            "StartOnIdle remains independent of the transfer mode")
    require(fileswn8, "CopyMoveLastTransferMode",
            "Keep-last preference is applied to the Copy/Move dialog")
    require(worker_h, "CopyMoveTransferMode",
            "transfer mode is captured in the operation script")
    require(worker_h, "CParallelProgressData", "parallel file progress payload")
    require(worker_h, "int DisplayCount", "operation-stable parallel row count")
    require(worker_h, "BOOL ResetSlots", "atomic parallel batch row replacement")
    require(worker_cpp, "RunParallelCopyBatch", "parallel copy dispatcher")
    require(worker_cpp, "replaces it atomically as soon as the next task is launched",
            "rolling parallel progress slot reuse")
    forbid(worker_cpp, "ShrinkFinishedSlots", "per-file progress dialog resizing")
    require(worker_cpp, "CreateFileW", "wide API for parallel copy streams")
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
    require(worker_cpp, "int mode = CMS_STORAGE_AWARE",
            "queue remains storage-aware irrespective of per-operation mode")
    require(dialogs, "&Script->StorageUse", "progress dialog registers storage use")
    require(dialogs, "Script->StorageUse.StreamDemand", "progress dialog reserves file stream budget")
    require(dialogs, "StorageUse_GetEndpointLabel", "progress title contains source/destination storage")
    require(dialogs_h, "SchedulingTitleSuffix", "progress dialog keeps a storage-aware title suffix")
    require(dialogs_h, "LayoutActiveProgressStreams", "dynamic parallel progress dialog layout")
    require(dialogs, "ParallelLayoutBaseDialogHeight", "non-cumulative progress dialog layout baseline")
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
    require(lang_rc, "IDC_COPYMOVE_SSD_WARNING_ICON,114,257,9,14,WS_GROUP | SS_NOTIFY",
            "hover-enabled SSD warning icon")
    require(lang_rc, "IDC_COPYMOVE_NVME_WARNING_ICON,264,257,10,14,WS_GROUP | SS_NOTIFY",
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
    require(worker_cpp, "FileAllocationInfo", "parallel target extent preallocation")
    require(worker_cpp, "firstWindowSize < CQuadWord(COPYMOVE_MIN_PARALLEL_BATCH_SIZE, 0)",
            "small batches avoid parallel thread overhead")

    require(fileswn8, "type == atMove ? SACCESS_READWRITE : SACCESS_READ",
            "F5/F6 Move source includes delete writes")
    require(fileswn8, "copyToSelectedDirs", "copy-to-selected-dirs destinations")
    require(fileswn8, "script->AddStoragePath(selectedTarget->c_str(), SACCESS_WRITE)",
            "each selected destination is a write resource")
    require(fileswn6, "copy ? SACCESS_READ : SACCESS_READWRITE",
            "drop Move source includes delete writes")
    require(fileswn6, "script->AddStoragePath(targetPath, SACCESS_WRITE)",
            "drop destination")

    print("copy_move_scheduling_contract_tests: ok")


if __name__ == "__main__":
    main()
