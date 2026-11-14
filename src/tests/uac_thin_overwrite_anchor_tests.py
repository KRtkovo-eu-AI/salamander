# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Probe a metadata-only overwrite witness on local NTFS. | Check whether native exclusive overwrite can coexist with an identity pin. | #731 | GPT-6 |
"""Probe Win32 sharing semantics; does not exercise the broker, F5 or UAC."""
import ctypes
from ctypes import wintypes
from pathlib import Path
import tempfile

from safe_file_paths_tests import ROOT
from uac_thin_overwrite_semantics_tests import kernel, INVALID_HANDLE

kernel.MoveFileW.argtypes = (wintypes.LPCWSTR, wintypes.LPCWSTR)
kernel.MoveFileW.restype = wintypes.BOOL


def open_file(path, access, sharing, creation, flags=0x80):
    result = kernel.CreateFileW(str(path), access, sharing, None, creation, flags, None)
    return result, 0 if result != INVALID_HANDLE else ctypes.get_last_error()


def close(handle):
    if not kernel.CloseHandle(handle):
        raise ctypes.WinError(ctypes.get_last_error())


def main():
    parent = (ROOT.parent / 'builds').resolve(strict=True)
    volume = ctypes.create_unicode_buffer(32768)
    filesystem = ctypes.create_unicode_buffer(256)
    if not kernel.GetVolumePathNameW(str(parent), volume, len(volume)):
        raise ctypes.WinError(ctypes.get_last_error())
    if not kernel.GetVolumeInformationW(volume, None, 0, None, None, None, filesystem, len(filesystem)):
        raise ctypes.WinError(ctypes.get_last_error())
    if filesystem.value.upper() != 'NTFS':
        print('SKIP: witness experiment requires NTFS.')
        return 0
    root = Path(tempfile.mkdtemp(prefix='thin-overwrite-witness-', dir=parent)).resolve(strict=True)
    if root.parent != parent:
        raise RuntimeError('fixture escaped verified parent')
    try:
        for label, access, overlapped in (('sync-attrs', 0x80, False), ('async-attrs', 0x80, True),
                                          ('sync-read', 0x80000000, False), ('async-read', 0x80000000, True)):
            path = root / (label + '.txt')
            renamed = root / (label + '-renamed.txt')
            with path.open('xb') as stream:
                stream.write(b'old default stream')
            with Path(str(path) + ':meta').open('xb') as stream:
                stream.write(b'old alternate stream')
            before = path.stat()
            witness, err = open_file(path, access, 0x1 | 0x2, 3)
            if err:
                raise OSError(err, 'cannot open witness')
            try:
                moved = bool(kernel.MoveFileW(str(path), str(renamed)))
                rename_error = 0 if moved else ctypes.get_last_error()
                if moved and not kernel.MoveFileW(str(renamed), str(path)):
                    raise ctypes.WinError(ctypes.get_last_error())
                output, overwrite_error = open_file(path, 0x40000000, 0, 2,
                    0x08000000 | (0x40000000 if overlapped else 0))
                try:
                    after = path.stat()
                    if overwrite_error == 0:
                        assert output != INVALID_HANDLE and after.st_ino == before.st_ino
                        assert after.st_size == 0 and not Path(str(path) + ':meta').exists()
                    else:
                        assert output == INVALID_HANDLE and after.st_ino == before.st_ino
                        assert after.st_size == before.st_size and Path(str(path) + ':meta').exists()
                finally:
                    if output != INVALID_HANDLE:
                        close(output)
                print(f'{label}: rename_error={rename_error}, overwrite_error={overwrite_error}')
            finally:
                close(witness)
            path.unlink()
        print('Local NTFS only; no broker, elevation, relative NtCreateFile, concurrency, or network filesystem was tested.')
        return 0
    finally:
        for name in ('sync-attrs.txt', 'async-attrs.txt', 'sync-read.txt', 'async-read.txt',
                     'sync-attrs-renamed.txt', 'async-attrs-renamed.txt',
                     'sync-read-renamed.txt', 'async-read-renamed.txt'):
            path = root / name
            if path.exists():
                path.unlink()
        root.rmdir()


if __name__ == '__main__':
    raise SystemExit(main())
