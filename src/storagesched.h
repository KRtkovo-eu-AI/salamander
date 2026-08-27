// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    // Copy/Move operation scheduling (Configuration.CopyMoveScheduling)
    enum
    {
        CMS_SEQUENTIAL = 0,
        CMS_STORAGE_AWARE = 1,
        CMS_MANUAL = 2
    };

    enum
    {
        SRES_LOCAL = 0,
        SRES_NETWORK = 1,
        SRES_UNKNOWN = 2,
        SRES_GLOBAL_UNKNOWN = 3
    };

    enum
    {
        SACCESS_READ = 1,
        SACCESS_WRITE = 2,
        SACCESS_READWRITE = 3
    };

    // Display-only storage classification. Scheduling continues to use the
    // conservative SeekPenalty flag, because a bus type alone does not reveal
    // the performance characteristics of a particular device.
    enum
    {
        SMEDIA_UNKNOWN = 0,
        SMEDIA_HDD = 1,
        SMEDIA_SSD = 2,
        SMEDIA_NVME = 3
    };

    enum
    {
        SBUS_UNKNOWN = 0,
        SBUS_IDE_ATA = 1,
        SBUS_SATA = 2,
        SBUS_USB = 3,
        SBUS_NVME = 4,
        SBUS_SCSI = 5,
        SBUS_SAS = 6,
        SBUS_RAID = 7,
        SBUS_ISCSI = 8,
        SBUS_SD_MMC = 9,
        SBUS_VIRTUAL = 10,
        SBUS_SPACES = 11,
        SBUS_FIREWIRE = 12,
        SBUS_SSA = 13,
        SBUS_FIBRE_CHANNEL = 14,
        SBUS_SCM = 15,
        SBUS_UFS = 16
    };

#define COPYMOVE_SSD_MAX_WRITES 2
#define STORAGE_MAX_DISKS 16
#define STORAGE_MAX_CLAIMS 32
#define STORAGE_SOFTKEY_MAX 320

    struct CStorageClaim
    {
        int Kind;                // SRES_*
        unsigned DeviceNumber;   // physical disk number (SRES_LOCAL)
        int SeekPenalty;         // 1 = HDD-like exclusive lock
        int Access;              // SACCESS_*
        int Media;               // SMEDIA_* (display only)
        int Bus;                 // SBUS_* (display only)
        char SoftKey[STORAGE_SOFTKEY_MAX]; // UNC share, volume GUID, or empty
    };

    struct COperationStorageUse
    {
        struct CStorageClaim Claims[STORAGE_MAX_CLAIMS];
        int ClaimCount;
        int StreamDemand; // reserved file-transfer slots on each written device
    };

    struct CStorageOpView
    {
        const struct CStorageClaim* Claims;
        int Count;
        int StreamDemand;
    };

    void StorageUse_Reset(struct COperationStorageUse* use);
    int StorageUse_AddClaim(struct COperationStorageUse* use, const struct CStorageClaim* claim);
    int StorageUse_GetParallelFileLimit(const struct COperationStorageUse* use,
                                        int ssdLimit, int nvmeLimit);

    // 1 if candidate conflicts with any currently running operation.
    int StorageOperationConflictsWithRunning(const struct CStorageOpView* candidate,
                                             const struct CStorageOpView* running,
                                             int runningCount);
    int StorageOperationConflictsWithRunningWithLimits(const struct CStorageOpView* candidate,
                                                       const struct CStorageOpView* running,
                                                       int runningCount, int ssdWriteLimit,
                                                       int nvmeWriteLimit);

    // 1 if the new operation should start auto-paused.
    // anyNonAutoPaused: at least one queued operation is running or manually paused.
    int CopyMoveShouldStartPaused(int mode, int startOnIdle, int anyNonAutoPaused,
                                  const struct CStorageOpView* candidate,
                                  const struct CStorageOpView* running, int runningCount);
    int CopyMoveShouldStartPausedWithLimits(int mode, int startOnIdle, int anyNonAutoPaused,
                                            const struct CStorageOpView* candidate,
                                            const struct CStorageOpView* running, int runningCount,
                                            int ssdWriteLimit, int nvmeWriteLimit);

#ifdef __cplusplus
}

// Returns a compact, non-localized technical name such as "HDD (SATA)" for
// the requested end of an operation. UI code supplies localized mode names.
std::string StorageUse_GetEndpointLabel(const COperationStorageUse* use, int access);
#endif
