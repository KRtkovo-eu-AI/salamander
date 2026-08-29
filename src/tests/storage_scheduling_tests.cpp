// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../storagesched.h"

#include <cstring>
#include <iostream>

namespace
{
int Failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << "\n";
        Failures++;
    }
}

CStorageClaim LocalDisk(unsigned disk, int seekPenalty, int access)
{
    CStorageClaim claim;
    std::memset(&claim, 0, sizeof(claim));
    claim.Kind = SRES_LOCAL;
    claim.DeviceNumber = disk;
    claim.SeekPenalty = seekPenalty;
    claim.Access = access;
    return claim;
}

CStorageClaim LocalMedia(unsigned disk, int media, int access)
{
    CStorageClaim claim = LocalDisk(disk, 0, access);
    claim.Media = media;
    return claim;
}

void SetSoftKey(CStorageClaim* claim, const char* key)
{
    size_t i;
    if (claim == NULL || key == NULL)
        return;
    for (i = 0; i < STORAGE_SOFTKEY_MAX - 1 && key[i] != 0; i++)
        claim->SoftKey[i] = key[i];
    claim->SoftKey[i] = 0;
}

CStorageClaim NetworkShare(const char* share, int access)
{
    CStorageClaim claim;
    std::memset(&claim, 0, sizeof(claim));
    claim.Kind = SRES_NETWORK;
    claim.Access = access;
    SetSoftKey(&claim, share);
    return claim;
}

CStorageClaim UnknownVolume(const char* guid, int access)
{
    CStorageClaim claim;
    std::memset(&claim, 0, sizeof(claim));
    claim.Kind = SRES_UNKNOWN;
    claim.Access = access;
    SetSoftKey(&claim, guid);
    return claim;
}

CStorageClaim GlobalUnknown(int access)
{
    CStorageClaim claim;
    std::memset(&claim, 0, sizeof(claim));
    claim.Kind = SRES_GLOBAL_UNKNOWN;
    claim.Access = access;
    return claim;
}

CStorageOpView ViewOf(const COperationStorageUse& use)
{
    CStorageOpView view;
    view.Claims = use.Claims;
    view.Count = use.ClaimCount;
    view.StreamDemand = use.StreamDemand;
    return view;
}

COperationStorageUse UseFrom(const CStorageClaim& a)
{
    COperationStorageUse use;
    StorageUse_Reset(&use);
    StorageUse_AddClaim(&use, &a);
    return use;
}

COperationStorageUse UseFrom2(const CStorageClaim& a, const CStorageClaim& b)
{
    COperationStorageUse use;
    StorageUse_Reset(&use);
    StorageUse_AddClaim(&use, &a);
    StorageUse_AddClaim(&use, &b);
    return use;
}

int Conflicts(const COperationStorageUse& candidate, const COperationStorageUse* running, int runningCount)
{
    CStorageOpView cand = ViewOf(candidate);
    CStorageOpView runViews[8];
    int i;
    for (i = 0; i < runningCount; i++)
        runViews[i] = ViewOf(running[i]);
    return StorageOperationConflictsWithRunning(&cand, runViews, runningCount);
}

void TestSameHddConflicts()
{
    COperationStorageUse a = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(2, 0, SACCESS_WRITE));
    COperationStorageUse b = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(3, 0, SACCESS_WRITE));
    Check(Conflicts(b, &a, 1) != 0, "two operations reading the same HDD must conflict");
}

void TestSameDeviceNumberIsOneResource()
{
    COperationStorageUse a = UseFrom(LocalDisk(7, 1, SACCESS_WRITE));
    COperationStorageUse b = UseFrom(LocalDisk(7, 1, SACCESS_READ));
    Check(Conflicts(b, &a, 1) != 0,
          "different volumes on the same physical HDD DeviceNumber must share one lock");
}

void TestIndependentDisksRunTogether()
{
    COperationStorageUse a = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(10, 0, SACCESS_WRITE));
    COperationStorageUse b = UseFrom2(LocalDisk(2, 1, SACCESS_READ), LocalDisk(11, 0, SACCESS_WRITE));
    Check(Conflicts(b, &a, 1) == 0, "independent HDD-to-SSD copies must not conflict");
}

void TestTwoHddToSameNvmeAllowed()
{
    COperationStorageUse a = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(10, 0, SACCESS_WRITE));
    COperationStorageUse b = UseFrom2(LocalDisk(2, 1, SACCESS_READ), LocalDisk(10, 0, SACCESS_WRITE));
    Check(Conflicts(b, &a, 1) == 0, "two HDD sources may write the same NVMe destination");
}

void TestThirdWriteToNvmeWaits()
{
    COperationStorageUse running[2];
    running[0] = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(10, 0, SACCESS_WRITE));
    running[1] = UseFrom2(LocalDisk(2, 1, SACCESS_READ), LocalDisk(10, 0, SACCESS_WRITE));
    COperationStorageUse c = UseFrom2(LocalDisk(3, 1, SACCESS_READ), LocalDisk(10, 0, SACCESS_WRITE));
    Check(Conflicts(c, running, 2) != 0, "a third concurrent write to the same SSD must wait");
}

void TestSsdWriteVersusReadConflicts()
{
    COperationStorageUse writer = UseFrom(LocalDisk(10, 0, SACCESS_WRITE));
    COperationStorageUse reader = UseFrom(LocalDisk(10, 0, SACCESS_READ));
    Check(Conflicts(reader, &writer, 1) != 0, "SSD write versus read-only must conflict");
    Check(Conflicts(writer, &reader, 1) != 0, "SSD read-only versus write must conflict");
}

void TestSsdReadsDoNotConflict()
{
    COperationStorageUse a = UseFrom(LocalDisk(10, 0, SACCESS_READ));
    COperationStorageUse b = UseFrom(LocalDisk(10, 0, SACCESS_READ));
    Check(Conflicts(b, &a, 1) == 0, "SSD reads must not block each other");
}

void TestNetworkShareSerializes()
{
    COperationStorageUse a = UseFrom(NetworkShare("\\\\server\\share\\", SACCESS_WRITE));
    COperationStorageUse b = UseFrom(NetworkShare("\\\\SERVER\\share\\", SACCESS_READ));
    COperationStorageUse c = UseFrom(NetworkShare("\\\\server\\other\\", SACCESS_WRITE));
    Check(Conflicts(b, &a, 1) != 0, "the same UNC share must serialize");
    Check(Conflicts(c, &a, 1) != 0, "network shares must use the global conservative fallback");
}

void TestParallelFileLimitUsesSlowestEndpoint()
{
    COperationStorageUse hddToNvme = UseFrom2(LocalDisk(1, 1, SACCESS_READ),
                                              LocalMedia(2, SMEDIA_NVME, SACCESS_WRITE));
    Check(StorageUse_GetParallelFileLimit(&hddToNvme, 2, 4) == 1,
          "an HDD source must keep file transfer sequential");

    COperationStorageUse ssdToNvme = UseFrom2(LocalMedia(1, SMEDIA_SSD, SACCESS_READ),
                                              LocalMedia(2, SMEDIA_NVME, SACCESS_WRITE));
    Check(StorageUse_GetParallelFileLimit(&ssdToNvme, 2, 4) == 2,
          "SSD to NVMe must use the slower endpoint limit");

    COperationStorageUse nvmeToNvme = UseFrom2(LocalMedia(1, SMEDIA_NVME, SACCESS_READ),
                                               LocalMedia(2, SMEDIA_NVME, SACCESS_WRITE));
    Check(StorageUse_GetParallelFileLimit(&nvmeToNvme, 2, 4) == 4,
          "independent NVMe endpoints may use the NVMe limit");

    COperationStorageUse sameNvme = UseFrom2(LocalMedia(1, SMEDIA_NVME, SACCESS_READ),
                                             LocalMedia(1, SMEDIA_NVME, SACCESS_WRITE));
    Check(StorageUse_GetParallelFileLimit(&sameNvme, 2, 4) == 1,
          "copying within one physical NVMe must avoid competing read/write streams");

    COperationStorageUse network = UseFrom(NetworkShare("\\\\server\\share\\", SACCESS_READ));
    Check(StorageUse_GetParallelFileLimit(&network, 2, 4) == 1,
          "network transfer must retain the legacy pipeline");
}

void TestNetworkUsesConservativeFallback()
{
    COperationStorageUse net = UseFrom(NetworkShare("\\\\server\\share\\", SACCESS_WRITE));
    COperationStorageUse local = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(2, 0, SACCESS_WRITE));
    Check(Conflicts(local, &net, 1) != 0, "network storage must use the global conservative fallback");
}

void TestConfiguredNvmeWriteLimit()
{
    CStorageClaim nvme = LocalDisk(10, 0, SACCESS_WRITE);
    nvme.Media = SMEDIA_NVME;
    COperationStorageUse running[4];
    for (int i = 0; i < 4; i++)
        running[i] = UseFrom(nvme);
    COperationStorageUse candidate = UseFrom(nvme);
    CStorageOpView candidateView = ViewOf(candidate);
    CStorageOpView runningViews[4];
    for (int i = 0; i < 4; i++)
        runningViews[i] = ViewOf(running[i]);
    Check(StorageOperationConflictsWithRunningWithLimits(&candidateView, runningViews, 4, 2, 4) != 0,
          "configured NVMe write limit must queue the fifth writer");
    Check(StorageOperationConflictsWithRunningWithLimits(&candidateView, runningViews, 4, 2, 5) == 0,
          "a larger configured NVMe write limit must allow the fifth writer");
}

void TestParallelStreamDemandUsesDeviceBudget()
{
    CStorageClaim nvme = LocalMedia(10, SMEDIA_NVME, SACCESS_WRITE);
    COperationStorageUse running = UseFrom(nvme);
    COperationStorageUse candidate = UseFrom(nvme);
    running.StreamDemand = 2;
    candidate.StreamDemand = 2;
    CStorageOpView candidateView = ViewOf(candidate);
    CStorageOpView runningView = ViewOf(running);
    Check(StorageOperationConflictsWithRunningWithLimits(&candidateView, &runningView, 1, 2, 4) == 0,
          "two two-stream operations may share a four-stream NVMe budget");
    candidate.StreamDemand = 3;
    candidateView = ViewOf(candidate);
    Check(StorageOperationConflictsWithRunningWithLimits(&candidateView, &runningView, 1, 2, 4) != 0,
          "aggregate file streams must not exceed the NVMe budget");
}

void TestUnknownGuidUsesConservativeFallback()
{
    COperationStorageUse a = UseFrom(UnknownVolume("\\\\?\\volume{aaa}\\", SACCESS_WRITE));
    COperationStorageUse b = UseFrom(UnknownVolume("\\\\?\\volume{aaa}\\", SACCESS_READ));
    COperationStorageUse c = UseFrom(UnknownVolume("\\\\?\\volume{bbb}\\", SACCESS_WRITE));
    Check(Conflicts(b, &a, 1) != 0, "the same unresolved volume GUID must serialize");
    Check(Conflicts(c, &a, 1) != 0, "different unresolved volume GUIDs must use the global conservative fallback");
}

void TestGlobalUnknownWaitsForAnyRunning()
{
    COperationStorageUse running = UseFrom(LocalDisk(1, 0, SACCESS_READ));
    COperationStorageUse unknown = UseFrom(GlobalUnknown(SACCESS_WRITE));
    Check(Conflicts(unknown, &running, 1) != 0, "global-unknown must wait while anything is running");
    Check(Conflicts(unknown, NULL, 0) == 0, "global-unknown may start when the queue is idle");
}

void TestSameVolumeMoveIsReadWrite()
{
    COperationStorageUse move = UseFrom(LocalDisk(1, 1, SACCESS_READWRITE));
    COperationStorageUse copyFrom = UseFrom(LocalDisk(1, 1, SACCESS_READ));
    Check(Conflicts(copyFrom, &move, 1) != 0, "a same-volume move must occupy the HDD exclusively");
}

void TestSchedulingModes()
{
    COperationStorageUse a = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(2, 0, SACCESS_WRITE));
    COperationStorageUse b = UseFrom2(LocalDisk(3, 1, SACCESS_READ), LocalDisk(4, 0, SACCESS_WRITE));
    CStorageOpView cand = ViewOf(b);
    CStorageOpView run = ViewOf(a);

    Check(CopyMoveShouldStartPaused(CMS_SEQUENTIAL, 0, 1, &cand, &run, 1) == 0,
          "Sequential applies to file streams inside one operation, not the operation queue");
    Check(CopyMoveShouldStartPaused(CMS_SEQUENTIAL, 0, 0, &cand, NULL, 0) == 0,
          "Sequential may start when the operation queue is idle");
    Check(CopyMoveShouldStartPaused(CMS_MANUAL, 0, 1, &cand, &run, 1) == 0,
          "Keep-last preference must not serialize independent operations");
    Check(CopyMoveShouldStartPaused(CMS_MANUAL, 1, 1, &cand, &run, 1) != 0,
          "StartOnIdle must wait for running operations regardless of transfer mode");
    Check(CopyMoveShouldStartPaused(CMS_STORAGE_AWARE, 0, 1, &cand, &run, 1) == 0,
          "Storage-aware must start independent disks immediately");
    Check(CopyMoveShouldStartPaused(CMS_STORAGE_AWARE, 1, 1, &cand, &run, 1) != 0,
          "Storage-aware with StartOnIdle must wait for all other operations");

    COperationStorageUse sameHdd = UseFrom2(LocalDisk(1, 1, SACCESS_READ), LocalDisk(5, 0, SACCESS_WRITE));
    CStorageOpView sameView = ViewOf(sameHdd);
    Check(CopyMoveShouldStartPaused(CMS_STORAGE_AWARE, 0, 1, &sameView, &run, 1) != 0,
          "Storage-aware must queue a second operation on the same HDD");
}

int WaitReason(int policy, int operationOverride, int anyOtherActive, int hasBarrier,
               const COperationStorageUse& candidate,
               const COperationStorageUse* running = NULL, int runningCount = 0,
               int ssdLimit = 2, int nvmeLimit = 4)
{
    CStorageOpView candidateView = ViewOf(candidate);
    CStorageOpView runningViews[8];
    for (int i = 0; i < runningCount; i++)
        runningViews[i] = ViewOf(running[i]);
    return StorageOperationGetWaitReason(policy, operationOverride, anyOtherActive, hasBarrier,
                                         &candidateView, runningViews, runningCount,
                                         ssdLimit, nvmeLimit);
}

void TestOperationPoliciesAndOverrides()
{
    COperationStorageUse running = UseFrom(LocalDisk(1, 1, SACCESS_READ));
    COperationStorageUse independent = UseFrom(LocalDisk(2, 1, SACCESS_READ));
    COperationStorageUse conflict = UseFrom(LocalDisk(1, 1, SACCESS_WRITE));

    Check(WaitReason(COSP_STORAGE_AWARE, COSO_DEFAULT, 1, 0, independent, &running, 1) == CSWR_NONE,
          "storage-aware policy must admit independent devices");
    Check(WaitReason(COSP_GLOBAL_SEQUENTIAL, COSO_DEFAULT, 1, 0, independent, &running, 1) ==
              CSWR_EXPLICIT_OR_GLOBAL_WAIT,
          "global sequential policy must wait while another operation is active");
    Check(WaitReason(COSP_ASK, COSO_DEFAULT, 1, 0, independent, &running, 1) ==
              CSWR_EXPLICIT_OR_GLOBAL_WAIT,
          "unresolved ask policy must wait safely");
    Check(WaitReason(COSP_GLOBAL_SEQUENTIAL, COSO_START_NOW, 1, 0, conflict, &running, 1) == CSWR_NONE,
          "start-now override must bypass policy and storage conflicts");
    Check(WaitReason(COSP_STORAGE_AWARE, COSO_WAIT_ALL, 1, 0, independent, &running, 1) ==
              CSWR_EXPLICIT_OR_GLOBAL_WAIT,
          "wait-all override must wait for every active operation");
}

void TestFifoBarrierSemantics()
{
    COperationStorageUse independent = UseFrom(LocalDisk(2, 1, SACCESS_READ));
    Check(WaitReason(COSP_STORAGE_AWARE, COSO_DEFAULT, 0, 1, independent) ==
              CSWR_EXPLICIT_OR_GLOBAL_WAIT,
          "an earlier queued barrier must prevent default operations from overtaking it");
    Check(WaitReason(COSP_STORAGE_AWARE, COSO_START_NOW, 1, 1, independent) == CSWR_NONE,
          "explicit start-now must be able to bypass a FIFO barrier");
}

void TestStructuredWaitReasons()
{
    COperationStorageUse hdd = UseFrom(LocalDisk(1, 1, SACCESS_READ));
    COperationStorageUse hddConflict = UseFrom(LocalDisk(1, 1, SACCESS_WRITE));
    Check(WaitReason(COSP_STORAGE_AWARE, COSO_DEFAULT, 1, 0, hddConflict, &hdd, 1) ==
              CSWR_PHYSICAL_DEVICE_CONFLICT,
          "same physical HDD must report a physical-device conflict");

    COperationStorageUse nvme = UseFrom(LocalMedia(10, SMEDIA_NVME, SACCESS_WRITE));
    COperationStorageUse nvmeRunning[2] = {nvme, nvme};
    Check(WaitReason(COSP_STORAGE_AWARE, COSO_DEFAULT, 1, 0, nvme,
                     nvmeRunning, 2, 2, 2) == CSWR_SSD_NVME_STREAM_LIMIT,
          "exhausted solid-state stream budget must have its own reason");

    COperationStorageUse unknown = UseFrom(GlobalUnknown(SACCESS_WRITE));
    Check(WaitReason(COSP_STORAGE_AWARE, COSO_DEFAULT, 1, 0, unknown, &hdd, 1) ==
              CSWR_UNKNOWN_FALLBACK,
          "unresolved storage must report the conservative fallback reason");
}

void TestClaimMerge()
{
    COperationStorageUse use;
    StorageUse_Reset(&use);
    CStorageClaim read = LocalDisk(1, 1, SACCESS_READ);
    CStorageClaim write = LocalDisk(1, 1, SACCESS_WRITE);
    StorageUse_AddClaim(&use, &read);
    StorageUse_AddClaim(&use, &write);
    Check(use.ClaimCount == 1, "the same disk must merge into one claim");
    Check(use.Claims[0].Access == SACCESS_READWRITE, "merged claim must be read+write");
}

void TestClaimLimitFallsBackToGlobalUnknown()
{
    COperationStorageUse use;
    StorageUse_Reset(&use);
    for (unsigned i = 0; i < STORAGE_MAX_CLAIMS; i++)
    {
        CStorageClaim claim = LocalDisk(i, 0, SACCESS_READ);
        StorageUse_AddClaim(&use, &claim);
    }
    CStorageClaim overflow = LocalDisk(STORAGE_MAX_CLAIMS, 0, SACCESS_WRITE);
    StorageUse_AddClaim(&use, &overflow);
    Check(use.ClaimCount == 1, "too many resource claims must collapse to one safe global lock");
    Check(use.Claims[0].Kind == SRES_GLOBAL_UNKNOWN,
          "too many resource claims must not silently omit a physical disk");
}

void TestStorageEndpointLabel()
{
    CStorageClaim claim = LocalDisk(7, 1, SACCESS_READ);
    claim.Media = SMEDIA_HDD;
    claim.Bus = SBUS_SATA;
    COperationStorageUse use = UseFrom(claim);
    Check(StorageUse_GetEndpointLabel(&use, SACCESS_READ) == "HDD (SATA)",
          "title storage label must include media and known bus type");
}
} // namespace

int main()
{
    TestSameHddConflicts();
    TestSameDeviceNumberIsOneResource();
    TestIndependentDisksRunTogether();
    TestTwoHddToSameNvmeAllowed();
    TestThirdWriteToNvmeWaits();
    TestConfiguredNvmeWriteLimit();
    TestParallelStreamDemandUsesDeviceBudget();
    TestSsdWriteVersusReadConflicts();
    TestSsdReadsDoNotConflict();
    TestNetworkShareSerializes();
    TestNetworkUsesConservativeFallback();
    TestParallelFileLimitUsesSlowestEndpoint();
    TestUnknownGuidUsesConservativeFallback();
    TestGlobalUnknownWaitsForAnyRunning();
    TestSameVolumeMoveIsReadWrite();
    TestSchedulingModes();
    TestOperationPoliciesAndOverrides();
    TestFifoBarrierSemantics();
    TestStructuredWaitReasons();
    TestClaimMerge();
    TestClaimLimitFallsBackToGlobalUnknown();
    TestStorageEndpointLabel();

    if (Failures != 0)
    {
        std::cerr << Failures << " storage scheduling tests failed.\n";
        return 1;
    }
    std::cout << "storage_scheduling_tests: ok\n";
    return 0;
}
