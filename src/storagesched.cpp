// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "storagesched.h"

#include <string.h>

void StorageUse_Reset(COperationStorageUse* use)
{
    if (use == NULL)
        return;
    memset(use, 0, sizeof(*use));
}

int StorageUse_AddClaim(COperationStorageUse* use, const CStorageClaim* claim)
{
    int i;
    CStorageClaim* existing;

    if (use == NULL || claim == NULL)
        return 0;

    for (i = 0; i < use->ClaimCount; i++)
    {
        existing = &use->Claims[i];
        if (existing->Kind == claim->Kind &&
            existing->DeviceNumber == claim->DeviceNumber)
        {
            existing->Access |= claim->Access;
            if (claim->SeekPenalty)
                existing->SeekPenalty = 1;
            if (existing->Media == SMEDIA_UNKNOWN)
                existing->Media = claim->Media;
            if (existing->Bus == SBUS_UNKNOWN)
                existing->Bus = claim->Bus;
            return 1;
        }
    }

    if (use->ClaimCount >= STORAGE_MAX_CLAIMS)
    {
        for (i = 0; i < use->ClaimCount; i++)
        {
            if (use->Claims[i].Kind == SRES_GLOBAL_UNKNOWN)
            {
                use->Claims[i].Access |= claim->Access;
                return 1;
            }
        }
        // Losing a resource claim could permit concurrent I/O on the same
        // physical device. Collapse the operation to the safe global lock.
        CStorageClaim globalUnknown;
        memset(&globalUnknown, 0, sizeof(globalUnknown));
        globalUnknown.Kind = SRES_GLOBAL_UNKNOWN;
        globalUnknown.Access = SACCESS_READWRITE;
        use->Claims[0] = globalUnknown;
        use->ClaimCount = 1;
        return 1;
    }

    use->Claims[use->ClaimCount] = *claim;
    use->ClaimCount++;
    return 1;
}

int StorageUse_GetParallelFileLimit(const COperationStorageUse* use,
                                    int ssdLimit, int nvmeLimit)
{
    if (use == NULL || use->ClaimCount == 0)
        return 1;
    if (ssdLimit < 1)
        ssdLimit = 1;
    if (nvmeLimit < 1)
        nvmeLimit = 1;

    int limit = nvmeLimit > ssdLimit ? nvmeLimit : ssdLimit;
    for (int i = 0; i < use->ClaimCount; i++)
    {
        const CStorageClaim* claim = &use->Claims[i];
        if (claim->Kind != SRES_LOCAL || claim->SeekPenalty ||
            claim->Access == SACCESS_READWRITE)
            return 1;
        int claimLimit = 1;
        if (claim->Media == SMEDIA_NVME)
            claimLimit = nvmeLimit;
        else if (claim->Media == SMEDIA_SSD)
            claimLimit = ssdLimit;
        if (claimLimit < limit)
            limit = claimLimit;
    }
    return limit;
}

static void OpLocalDiskAccess(const CStorageOpView* op, unsigned disk, int* access, int* seekPenalty, int* media)
{
    int i;
    *access = 0;
    *seekPenalty = 0;
    if (media != NULL)
        *media = SMEDIA_UNKNOWN;
    if (op == NULL || op->Claims == NULL)
        return;
    for (i = 0; i < op->Count; i++)
    {
        if (op->Claims[i].Kind == SRES_LOCAL && op->Claims[i].DeviceNumber == disk)
        {
            *access |= op->Claims[i].Access;
            if (op->Claims[i].SeekPenalty)
                *seekPenalty = 1;
            if (media != NULL && *media == SMEDIA_UNKNOWN)
                *media = op->Claims[i].Media;
        }
    }
}

static int OpHasKind(const CStorageOpView* op, int kind)
{
    int i;
    if (op == NULL || op->Claims == NULL)
        return 0;
    for (i = 0; i < op->Count; i++)
    {
        if (op->Claims[i].Kind == kind)
            return 1;
    }
    return 0;
}

static int PairConflicts(const CStorageOpView* a, const CStorageOpView* b)
{
    int i;

    if (a == NULL || b == NULL)
        return 0;
    // A network path or unresolved volume cannot be mapped reliably to a
    // physical device. They must retain the old global-queue safety rule.
    if (OpHasKind(a, SRES_GLOBAL_UNKNOWN) || OpHasKind(b, SRES_GLOBAL_UNKNOWN) ||
        OpHasKind(a, SRES_NETWORK) || OpHasKind(b, SRES_NETWORK) ||
        OpHasKind(a, SRES_UNKNOWN) || OpHasKind(b, SRES_UNKNOWN))
        return 1;

    for (i = 0; i < (a->Claims != NULL ? a->Count : 0); i++)
    {
        unsigned disk;
        int aAccess, aHDD, bAccess, bHDD;
        if (a->Claims[i].Kind != SRES_LOCAL)
            continue;
        disk = a->Claims[i].DeviceNumber;
        OpLocalDiskAccess(a, disk, &aAccess, &aHDD, NULL);
        OpLocalDiskAccess(b, disk, &bAccess, &bHDD, NULL);
        if (bAccess == 0)
            continue;
        if (aHDD || bHDD)
            return 1;
        /* SSD: write vs read-only is a conflict; write vs write is counted later */
        if ((aAccess & SACCESS_WRITE) && (bAccess & SACCESS_READ) && !(bAccess & SACCESS_WRITE))
            return 1;
        if ((bAccess & SACCESS_WRITE) && (aAccess & SACCESS_READ) && !(aAccess & SACCESS_WRITE))
            return 1;
    }
    return 0;
}

static int DeviceStreamLimitExceeded(const CStorageOpView* candidate,
                                     const CStorageOpView* running, int runningCount,
                                     int ssdStreamLimit, int nvmeStreamLimit)
{
    int i, r;
    if (candidate == NULL || candidate->Claims == NULL)
        return 0;

    for (i = 0; i < candidate->Count; i++)
    {
        unsigned disk;
        int candAccess, candHDD, candMedia;
        int streamCount;
        if (candidate->Claims[i].Kind != SRES_LOCAL)
            continue;
        disk = candidate->Claims[i].DeviceNumber;
        OpLocalDiskAccess(candidate, disk, &candAccess, &candHDD, &candMedia);
        if (candHDD || candAccess == 0)
            continue;

        streamCount = candidate->StreamDemand > 0 ? candidate->StreamDemand : 1;
        for (r = 0; r < runningCount; r++)
        {
            int runAccess, runHDD;
            OpLocalDiskAccess(&running[r], disk, &runAccess, &runHDD, NULL);
            if (runHDD)
                continue;
            if (runAccess != 0)
                streamCount += running[r].StreamDemand > 0 ? running[r].StreamDemand : 1;
        }
        int streamLimit = candMedia == SMEDIA_NVME ? nvmeStreamLimit : ssdStreamLimit;
        if (streamLimit < 1)
            streamLimit = COPYMOVE_SSD_MAX_WRITES;
        if (streamCount > streamLimit)
            return 1;
    }
    return 0;
}

static int OperationHasUnknownFallback(const CStorageOpView* operation)
{
    return OpHasKind(operation, SRES_GLOBAL_UNKNOWN) ||
           OpHasKind(operation, SRES_NETWORK) || OpHasKind(operation, SRES_UNKNOWN);
}

static int StorageOperationGetConflictReason(const CStorageOpView* candidate,
                                             const CStorageOpView* running, int runningCount,
                                             int ssdWriteLimit, int nvmeWriteLimit)
{
    if (candidate == NULL)
        return CSWR_NONE;
    if (OperationHasUnknownFallback(candidate) && runningCount > 0)
        return CSWR_UNKNOWN_FALLBACK;
    for (int r = 0; r < runningCount; r++)
    {
        if (PairConflicts(candidate, &running[r]))
            return OperationHasUnknownFallback(&running[r]) ? CSWR_UNKNOWN_FALLBACK :
                                                             CSWR_PHYSICAL_DEVICE_CONFLICT;
    }
    return DeviceStreamLimitExceeded(candidate, running, runningCount,
                                     ssdWriteLimit, nvmeWriteLimit) ?
               CSWR_SSD_NVME_STREAM_LIMIT : CSWR_NONE;
}

int StorageOperationIsFifoBarrier(int policy, int operationOverride)
{
    if (policy != COSP_STORAGE_AWARE && policy != COSP_GLOBAL_SEQUENTIAL && policy != COSP_ASK)
        policy = COSP_STORAGE_AWARE;
    if (operationOverride != COSO_DEFAULT && operationOverride != COSO_START_NOW &&
        operationOverride != COSO_WAIT_ALL)
        operationOverride = COSO_DEFAULT;
    if (operationOverride == COSO_START_NOW)
        return 0;
    return operationOverride == COSO_WAIT_ALL || policy == COSP_GLOBAL_SEQUENTIAL ||
           (policy == COSP_ASK && operationOverride == COSO_DEFAULT);
}

int StorageOperationGetWaitReason(int policy, int operationOverride,
                                  int anyOtherActive, int hasFifoBarrier,
                                  const CStorageOpView* candidate,
                                  const CStorageOpView* running, int runningCount,
                                  int ssdWriteLimit, int nvmeWriteLimit)
{
    if (policy != COSP_STORAGE_AWARE && policy != COSP_GLOBAL_SEQUENTIAL && policy != COSP_ASK)
        policy = COSP_STORAGE_AWARE;
    if (operationOverride != COSO_DEFAULT && operationOverride != COSO_START_NOW &&
        operationOverride != COSO_WAIT_ALL)
        operationOverride = COSO_DEFAULT;

    // An explicit start-now decision bypasses policy, compatibility, and FIFO barriers.
    if (operationOverride == COSO_START_NOW)
        return CSWR_NONE;
    if (hasFifoBarrier)
        return CSWR_EXPLICIT_OR_GLOBAL_WAIT;
    if (operationOverride == COSO_WAIT_ALL || policy == COSP_GLOBAL_SEQUENTIAL)
        return anyOtherActive ? CSWR_EXPLICIT_OR_GLOBAL_WAIT : CSWR_NONE;

    // Ask without an explicit answer has the safe wait-all behavior.
    if (policy == COSP_ASK && operationOverride == COSO_DEFAULT)
        return anyOtherActive ? CSWR_EXPLICIT_OR_GLOBAL_WAIT : CSWR_NONE;
    return StorageOperationGetConflictReason(candidate, running, runningCount,
                                             ssdWriteLimit, nvmeWriteLimit);
}

int StorageOperationConflictsWithRunning(const CStorageOpView* candidate,
                                         const CStorageOpView* running,
                                         int runningCount)
{
    return StorageOperationConflictsWithRunningWithLimits(candidate, running, runningCount,
                                                          COPYMOVE_SSD_MAX_WRITES, 4);
}

int StorageOperationConflictsWithRunningWithLimits(const CStorageOpView* candidate,
                                                   const CStorageOpView* running, int runningCount,
                                                   int ssdWriteLimit, int nvmeWriteLimit)
{
    return StorageOperationGetConflictReason(candidate, running, runningCount,
                                             ssdWriteLimit, nvmeWriteLimit) != CSWR_NONE;
}

int CopyMoveShouldStartPaused(int mode, int startOnIdle, int anyNonAutoPaused,
                              const CStorageOpView* candidate,
                              const CStorageOpView* running, int runningCount)
{
    return CopyMoveShouldStartPausedWithLimits(mode, startOnIdle, anyNonAutoPaused,
                                               candidate, running, runningCount,
                                               COPYMOVE_SSD_MAX_WRITES, 4);
}

int CopyMoveShouldStartPausedWithLimits(int mode, int startOnIdle, int anyNonAutoPaused,
                                        const CStorageOpView* candidate,
                                        const CStorageOpView* running, int runningCount,
                                        int ssdWriteLimit, int nvmeWriteLimit)
{
    if (mode != CMS_SEQUENTIAL && mode != CMS_STORAGE_AWARE && mode != CMS_MANUAL)
        mode = CMS_STORAGE_AWARE;
    if (startOnIdle)
        return anyNonAutoPaused ? 1 : 0;
    return StorageOperationConflictsWithRunningWithLimits(candidate, running, runningCount,
                                                          ssdWriteLimit, nvmeWriteLimit);
}

#ifdef __cplusplus
static const char* StorageMediaName(int media)
{
    switch (media)
    {
    case SMEDIA_HDD:
        return "HDD";
    case SMEDIA_SSD:
        return "SSD";
    case SMEDIA_NVME:
        return "NVMe";
    default:
        return "?";
    }
}

static const char* StorageBusName(int bus)
{
    switch (bus)
    {
    case SBUS_IDE_ATA:
        return "IDE/ATA";
    case SBUS_SATA:
        return "SATA";
    case SBUS_USB:
        return "USB";
    case SBUS_NVME:
        return "NVMe";
    case SBUS_SCSI:
        return "SCSI";
    case SBUS_SAS:
        return "SAS";
    case SBUS_RAID:
        return "RAID";
    case SBUS_ISCSI:
        return "iSCSI";
    case SBUS_SD_MMC:
        return "SD/MMC";
    case SBUS_VIRTUAL:
        return "Virtual";
    case SBUS_SPACES:
        return "Storage Spaces";
    case SBUS_FIREWIRE:
        return "FireWire";
    case SBUS_SSA:
        return "SSA";
    case SBUS_FIBRE_CHANNEL:
        return "Fibre Channel";
    case SBUS_SCM:
        return "SCM";
    case SBUS_UFS:
        return "UFS";
    default:
        return "";
    }
}

static std::string StorageClaimName(const CStorageClaim& claim)
{
    if (claim.Kind == SRES_NETWORK)
        return "?";
    if (claim.Kind != SRES_LOCAL)
        return "?";

    std::string name = StorageMediaName(claim.Media);
    const char* bus = StorageBusName(claim.Bus);
    if (bus[0] != 0 && name != bus)
    {
        name += " (";
        name += bus;
        name += ')';
    }
    return name;
}

std::string StorageUse_GetEndpointLabel(const COperationStorageUse* use, int access)
{
    if (use == NULL)
        return "?";

    std::string first;
    int different = 0;
    for (int i = 0; i < use->ClaimCount; i++)
    {
        const CStorageClaim& claim = use->Claims[i];
        if ((claim.Access & access) == 0)
            continue;
        std::string name = StorageClaimName(claim);
        if (first.empty())
            first = name;
        else if (name != first)
            different = 1;
    }
    if (first.empty())
        return "?";
    return different ? first + " +" : first;
}
#endif
