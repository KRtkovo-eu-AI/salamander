# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Compare native overwrite and handle truncation on NTFS. | Prevent changing ADS and hidden-file behavior when integrating thin F5. | #731 | GPT-6 |
"""Test exact local Windows overwrite primitives, not the F5 worker or UAC."""
import ctypes
from ctypes import wintypes
import hashlib
from pathlib import Path
import tempfile

from safe_file_paths_tests import ROOT


kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.CreateFileW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p,
                               wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE)
kernel.CreateFileW.restype = wintypes.HANDLE
kernel.SetEndOfFile.argtypes = (wintypes.HANDLE,)
kernel.SetEndOfFile.restype = wintypes.BOOL
kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
kernel.CloseHandle.restype = wintypes.BOOL
kernel.SetFileAttributesW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD)
kernel.SetFileAttributesW.restype = wintypes.BOOL
kernel.GetVolumePathNameW.argtypes = (wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD)
kernel.GetVolumePathNameW.restype = wintypes.BOOL
kernel.GetVolumeInformationW.argtypes = (wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD,
                                         ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
                                         wintypes.LPWSTR, wintypes.DWORD)
kernel.GetVolumeInformationW.restype = wintypes.BOOL
INVALID_HANDLE = ctypes.c_void_p(-1).value


def overwrite(path, create_always):
    handle = kernel.CreateFileW(str(path), 0x40000000, 0, None,
                                2 if create_always else 3, 0x08000000, None)
    if handle == INVALID_HANDLE:
        return ctypes.get_last_error()
    try:
        if not create_always and not kernel.SetEndOfFile(handle):
            return ctypes.get_last_error()
        return 0
    finally:
        if not kernel.CloseHandle(handle):
            raise ctypes.WinError(ctypes.get_last_error())


def main():
    base = (ROOT.parent / 'builds').resolve(strict=True)
    volume, filesystem = ctypes.create_unicode_buffer(32768), ctypes.create_unicode_buffer(256)
    if not kernel.GetVolumePathNameW(str(base), volume, len(volume)):
        raise ctypes.WinError(ctypes.get_last_error())
    if not kernel.GetVolumeInformationW(volume, None, 0, None, None, None, filesystem, len(filesystem)):
        raise ctypes.WinError(ctypes.get_last_error())
    if filesystem.value.upper() != 'NTFS':
        print('SKIP: fixture volume is not NTFS; stream behavior is provider-dependent.')
        return 0
    root = Path(tempfile.mkdtemp(prefix='thin-overwrite-semantics-', dir=base)).resolve(strict=True)
    if root.parent != base:
        raise RuntimeError('fixture directory escaped its verified parent')
    created = []
    results = {}
    try:
        for label, has_stream, attrs in [('plain', False, 0), ('stream', True, 0),
                                          ('hidden', False, 2), ('readonly', False, 1)]:
            for create_always in (True, False):
                path = root / (label + ('-native.txt' if create_always else '-candidate.txt'))
                with path.open('xb') as file:
                    created.append(path)
                    file.write(b'original-default-data')
                stream = Path(str(path) + ':meta')
                if has_stream:
                    with stream.open('xb') as file:
                        file.write(b'original-alternate-stream')
                if attrs and not kernel.SetFileAttributesW(str(path), attrs):
                    raise ctypes.WinError(ctypes.get_last_error())
                before = path.stat()
                error = overwrite(path, create_always)
                after = path.stat()
                results[label, create_always] = (error, before, after, stream.exists(),
                    hashlib.sha256(stream.read_bytes()).digest() if stream.exists() else None)
                assert before.st_ino == after.st_ino, (label, create_always, 'identity')
                assert before.st_ctime_ns == after.st_ctime_ns, (label, create_always, 'creation time')
        for native in (True, False):
            error, before, after, _, _ = results['plain', native]
            assert not error and before.st_size == 21 and after.st_size == 0
        native, candidate = results['stream', True], results['stream', False]
        assert not native[0] and not candidate[0] and not native[3] and candidate[3]
        assert candidate[4] == hashlib.sha256(b'original-alternate-stream').digest()
        assert results['hidden', True][0] == 5 and results['hidden', True][2].st_size == 21
        assert results['hidden', False][0] == 0 and results['hidden', False][2].st_size == 0
        for native in (True, False):
            error, _, after, _, _ = results['readonly', native]
            assert error == 5 and after.st_size == 21
        print('PASS plain identity and size; native overwrite removes ADS while handle truncation keeps it')
        print('PASS hidden target differs; read-only target denies both unadjusted operations')
        print('Local NTFS primitives only; does not establish F5, UAC, ACL, timing or other filesystem parity.')
        return 0
    finally:
        for path in created:
            if path.exists():
                if not kernel.SetFileAttributesW(str(path), 0x80):
                    raise ctypes.WinError(ctypes.get_last_error())
                path.unlink()
        root.rmdir()


if __name__ == '__main__':
    raise SystemExit(main())
