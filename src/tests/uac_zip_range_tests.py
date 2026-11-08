# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-08 | Test ZIP64 listing through the real ZIP plugin export. | Detect truncated offsets and whole-archive reads. | | GPT-6.1 Sol (Codex) |
import argparse
import ctypes
import io
import msvcrt
from pathlib import Path
import tempfile
import zipfile
from ctypes import wintypes


READ = ctypes.WINFUNCTYPE(
    wintypes.DWORD, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p, wintypes.DWORD
)
ENTRY = ctypes.WINFUNCTYPE(
    wintypes.DWORD, ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD
)


class ReadSource(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_ulonglong),
        ("context", ctypes.c_void_p),
        ("read", READ),
        ("entry", ENTRY),
    ]


def make_zip64(path):
    device_io = ctypes.WinDLL("kernel32", use_last_error=True).DeviceIoControl
    device_io.argtypes = (
        wintypes.HANDLE, wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD,
        wintypes.LPVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID,
    )
    device_io.restype = wintypes.BOOL
    with path.open("xb") as stream:
        returned = wintypes.DWORD()
        if not device_io(msvcrt.get_osfhandle(stream.fileno()), 0x900C4,
                         None, 0, None, 0, ctypes.byref(returned), None):
            raise ctypes.WinError(ctypes.get_last_error())
        stream.seek(5 * 1024 ** 3)
        with zipfile.ZipFile(stream, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("dír/東.txt", "ZIP64 range check")
            archive.writestr("empty/", "")
            archive.writestr("../text-only.txt", "never extracted by listing")


def invoke(list_zip, stream, read_error=0, entry_error=0):
    size = stream.seek(0, io.SEEK_END)
    names = []
    total = highest = 0

    @READ
    def read(_context, offset, buffer, count):
        nonlocal total, highest
        if read_error:
            return read_error
        if offset > size or count > size - offset:
            return 38
        stream.seek(offset)
        data = stream.read(count)
        if len(data) != count:
            return 38
        ctypes.memmove(buffer, data, count)
        total += count
        highest = max(highest, offset)
        return 0

    @ENTRY
    def entry(_context, name, length):
        if entry_error:
            return entry_error
        names.append(ctypes.string_at(name, length))
        return 0

    error = list_zip(ctypes.byref(ReadSource(size, None, read, entry)))
    return error, names, total, highest


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("plugin", type=Path)
    args = parser.parse_args()
    library = ctypes.WinDLL(str(args.plugin.resolve()))
    list_zip = library.SalamanderListZipSourceV1
    list_zip.argtypes = (ctypes.POINTER(ReadSource),)
    list_zip.restype = wintypes.DWORD
    assert list_zip(None) == 87

    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "large.zip"
        make_zip64(path)
        with path.open("rb") as stream:
            result, names, total, highest = invoke(list_zip, stream)
            assert result == 0, (result, names, total, highest)
            assert names == [r"dír\東.txt".encode(), b"empty/", b"..\\text-only.txt"], names
            assert highest > 2**32 and 0 < total <= 65536, (highest, total)
            assert invoke(list_zip, stream, read_error=109)[0] == 109
            assert invoke(list_zip, stream, entry_error=1223)[0] == 1223
        print(f"ZIP64: read {total} bytes, highest offset {highest}, names preserved")

    with io.BytesIO() as stream:
        with zipfile.ZipFile(stream, "w") as archive:
            archive.writestr("ordinary.txt", "ordinary")
        assert invoke(list_zip, stream)[:2] == (0, [b"ordinary.txt"])
        malformed = bytearray(stream.getvalue())
        central = malformed.index(b"PK\x01\x02")
        malformed[central] = 0
        assert invoke(list_zip, io.BytesIO(malformed))[0] == 13
        split = bytearray(stream.getvalue())
        split[-18] = 1
        assert invoke(list_zip, io.BytesIO(split))[0] == 13

    with io.BytesIO() as stream:
        with zipfile.ZipFile(stream, "w") as archive:
            for i in range(4097):
                archive.writestr(str(i), "")
        assert invoke(list_zip, stream)[0] == 223
    print("PASS: ordinary ZIP, malformed and split rejection, 4096-entry limit, callback failures")


if __name__ == "__main__":
    main()
