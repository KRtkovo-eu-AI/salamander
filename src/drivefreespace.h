// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>

enum DriveFreeSpaceMode
{
    dfsOff = 0,
    dfsOnce = 1,
    dfsUpdated = 2
};

enum DriveFreeSpaceState
{
    dfsUnknown,
    dfsPending,
    dfsFresh,
    dfsStale,
    dfsUnavailable
};

struct DriveFreeSpaceValue
{
    bool Available = false;
    bool Pending = false;
    bool Stale = false;
    ULONGLONG Bytes = 0;
    FILETIME LastSuccess = {};
    DriveFreeSpaceState State = dfsUnknown;
};

// This never queries the filesystem or a network provider on the calling thread.
// A successful Once value is a snapshot, so it is always marked stale.
DriveFreeSpaceValue DriveFreeSpaceQuery(wchar_t driveLetter, UINT driveType,
                                       const wchar_t* remoteUNC, bool disconnected,
                                       DriveFreeSpaceMode mode, HWND notifyWindow,
                                       UINT notifyMessage);
void DriveFreeSpaceInvalidate(wchar_t driveLetter = 0);
void DriveFreeSpaceMarkStale(wchar_t driveLetter = 0);
void DriveFreeSpaceSetPolicies(DriveFreeSpaceMode removable, DriveFreeSpaceMode remote);
void DriveFreeSpaceShutdown();

// Call before regular application initialization. A matching private helper
// command line is fully handled here, even if its arguments are invalid.
bool DriveFreeSpaceHelperDispatch(int& exitCode);
