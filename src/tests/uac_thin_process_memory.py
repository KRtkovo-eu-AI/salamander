# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Add explicit process-private-memory sampling. | Report inaccessible or mismatched processes without inventing results. | #731 | GPT-6 |
"""Read-only process-memory sample; no broker discovery or elevation."""
import ctypes
from ctypes import wintypes
import os


class Counters(ctypes.Structure):
    _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD),
                ("PeakWorkingSetSize", ctypes.c_size_t), ("WorkingSetSize", ctypes.c_size_t),
                ("QuotaPeakPagedPoolUsage", ctypes.c_size_t), ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t), ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                ("PagefileUsage", ctypes.c_size_t), ("PeakPagefileUsage", ctypes.c_size_t),
                ("PrivateUsage", ctypes.c_size_t)]


class IoCounters(ctypes.Structure):
    _fields_ = [("ReadOperationCount", ctypes.c_ulonglong), ("WriteOperationCount", ctypes.c_ulonglong),
                ("OtherOperationCount", ctypes.c_ulonglong), ("ReadTransferCount", ctypes.c_ulonglong),
                ("WriteTransferCount", ctypes.c_ulonglong), ("OtherTransferCount", ctypes.c_ulonglong)]


def sample(pid, expected_image):
    """Sample a known PID only when its executable matches the expected image.

    The caller must obtain the broker PID from its own authenticated launch;
    process name enumeration is insufficient, particularly across UAC accounts.
    A denied handle is an unavailable measurement, never a numeric zero.
    """
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    kernel.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR,
                                                 ctypes.POINTER(wintypes.DWORD)]
    kernel.QueryFullProcessImageNameW.restype = wintypes.BOOL
    kernel.GetProcessIoCounters.argtypes = [wintypes.HANDLE, ctypes.POINTER(IoCounters)]
    kernel.GetProcessIoCounters.restype = wintypes.BOOL
    psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(Counters), wintypes.DWORD]
    psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
    handle = kernel.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
    if not handle:
        return {"status": "unavailable", "stage": "OpenProcess", "winerror": ctypes.get_last_error()}
    try:
        capacity = wintypes.DWORD(32768)
        name = ctypes.create_unicode_buffer(capacity.value)
        if not kernel.QueryFullProcessImageNameW(handle, 0, name, ctypes.byref(capacity)):
            return {"status": "unavailable", "stage": "QueryFullProcessImageNameW",
                    "winerror": ctypes.get_last_error()}
        if os.path.normcase(os.path.realpath(name.value)) != os.path.normcase(os.path.realpath(expected_image)):
            return {"status": "image_mismatch"}
        counters = Counters()
        counters.cb = ctypes.sizeof(counters)
        if not psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), ctypes.sizeof(counters)):
            return {"status": "unavailable", "stage": "GetProcessMemoryInfo",
                    "winerror": ctypes.get_last_error()}
        result = {"status": "measured", "pid": pid, "private_bytes": counters.PrivateUsage,
                  "working_set_bytes": counters.WorkingSetSize}
        io = IoCounters()
        if kernel.GetProcessIoCounters(handle, ctypes.byref(io)):
            result["process_read_bytes"] = io.ReadTransferCount
            result["process_write_bytes"] = io.WriteTransferCount
        else:
            result["io_error"] = ctypes.get_last_error()
        return result
    finally:
        kernel.CloseHandle(handle)
