# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test-only event collector for the original broker launch handle mask.

The caller supplies the controller and host SIDs and separately proves the
host PID/account. These named events are diagnostic, not authenticated data.
This module does not launch applications, request elevation, or write files.
"""
import ctypes
from ctypes import wintypes
import re
import uuid

WAIT_OBJECT_0, WAIT_TIMEOUT = 0, 258
EVENT_MODIFY_STATE = 2
_SID = re.compile(r'S-1-(?:\d+-){1,14}\d+\Z', re.IGNORECASE)


class SecurityAttributes(ctypes.Structure):
    _fields_ = [('length', wintypes.DWORD), ('descriptor', ctypes.c_void_p), ('inherit', wintypes.BOOL)]


def apis():
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    security = ctypes.WinDLL('advapi32', use_last_error=True)
    kernel.CreateEventW.argtypes = [ctypes.POINTER(SecurityAttributes), wintypes.BOOL, wintypes.BOOL, wintypes.LPCWSTR]
    kernel.CreateEventW.restype = wintypes.HANDLE
    kernel.OpenEventW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    kernel.OpenEventW.restype = wintypes.HANDLE
    kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel.WaitForSingleObject.restype = wintypes.DWORD
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    kernel.LocalFree.argtypes = [ctypes.c_void_p]
    kernel.LocalFree.restype = ctypes.c_void_p
    security.ConvertStringSecurityDescriptorToSecurityDescriptorW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD,
                                                                               ctypes.POINTER(ctypes.c_void_p),
                                                                               ctypes.POINTER(wintypes.DWORD)]
    security.ConvertStringSecurityDescriptorToSecurityDescriptorW.restype = wintypes.BOOL
    return kernel, security


class LaunchMaskEvents:
    def __init__(self, controller_sid, host_sid):
        if not _SID.fullmatch(controller_sid) or not _SID.fullmatch(host_sid):
            raise ValueError('Expected explicit Windows SID strings')
        self.kernel, security = apis()
        self.base = 'Local\\SamandarinThinLaunch-' + uuid.uuid4().hex
        self.events = []
        descriptor = ctypes.c_void_p()
        sddl = f'D:P(A;;GA;;;{controller_sid})(A;;0x00000002;;;{host_sid})'
        if not security.ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, 1, ctypes.byref(descriptor), None):
            raise ctypes.WinError(ctypes.get_last_error())
        attributes = SecurityAttributes(ctypes.sizeof(SecurityAttributes), descriptor, False)
        try:
            for suffix in [*(f'-bit-{bit}' for bit in range(32)), '-error', '-ready']:
                ctypes.set_last_error(0)
                event = self.kernel.CreateEventW(ctypes.byref(attributes), True, False, self.base + suffix)
                if not event:
                    raise ctypes.WinError(ctypes.get_last_error())
                self.events.append(event)
                if ctypes.get_last_error() == 183:
                    raise RuntimeError('A unique test event already existed')
        except BaseException:
            self.close()
            raise
        finally:
            self.kernel.LocalFree(descriptor)

    def read(self, timeout_ms):
        if len(self.events) != 34:
            raise RuntimeError('Collector is closed')
        if not 0 <= timeout_ms < 0xFFFFFFFF:
            raise ValueError('Use a bounded timeout')
        ready = self.kernel.WaitForSingleObject(self.events[33], timeout_ms)
        if ready == WAIT_TIMEOUT:
            return {'status': 'pending'}
        if ready != WAIT_OBJECT_0:
            raise ctypes.WinError(ctypes.get_last_error())
        error = self.kernel.WaitForSingleObject(self.events[32], 0)
        if error == WAIT_OBJECT_0:
            return {'status': 'error'}
        if error != WAIT_TIMEOUT:
            raise ctypes.WinError(ctypes.get_last_error())
        mask = 0
        for bit, event in enumerate(self.events[:32]):
            state = self.kernel.WaitForSingleObject(event, 0)
            if state == WAIT_OBJECT_0:
                mask |= 1 << bit
            elif state != WAIT_TIMEOUT:
                raise ctypes.WinError(ctypes.get_last_error())
        return {'status': 'ok', 'granted_access': mask}

    def close(self):
        for event in self.events:
            self.kernel.CloseHandle(event)
        self.events.clear()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
