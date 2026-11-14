# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Probe a process-local application hive without touching user configuration. | Validate isolation before any real-host GUI testing. | #731 | GPT-6 |
"""Probe HKCU redirection in a child process; does not start Samandarin.

The probe creates one private application hive under C:\\Samandarin\\builds.
It never creates or changes a key in the user's real registry. The parent
checks the unique marker while the child's override is active and again after
the child exits. The hive is retained as test evidence.
"""
import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import sys
import uuid
import winreg


HKEY_CURRENT_USER = ctypes.c_void_p(-2147483647)
KEY_ALL_ACCESS = 0xF003F
REG_PROCESS_APPKEY = 1


def registry_api():
    library = ctypes.WinDLL("advapi32", use_last_error=True)
    library.RegLoadAppKeyW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(wintypes.HKEY),
                                      wintypes.DWORD, wintypes.DWORD, wintypes.DWORD]
    library.RegLoadAppKeyW.restype = wintypes.LONG
    library.RegOverridePredefKey.argtypes = [wintypes.HKEY, wintypes.HKEY]
    library.RegOverridePredefKey.restype = wintypes.LONG
    library.RegCloseKey.argtypes = [wintypes.HKEY]
    library.RegCloseKey.restype = wintypes.LONG
    return library


def check(status, where):
    if status:
        raise OSError(status, where)


def absent(path):
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, path):
            return False
    except FileNotFoundError:
        return True


def child(hive_path, marker, value):
    api = registry_api()
    hive = wintypes.HKEY()
    check(api.RegLoadAppKeyW(str(hive_path), ctypes.byref(hive), KEY_ALL_ACCESS, REG_PROCESS_APPKEY, 0),
          "RegLoadAppKeyW")
    mapped = False
    try:
        check(api.RegOverridePredefKey(HKEY_CURRENT_USER, hive), "RegOverridePredefKey")
        mapped = True
        with winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, marker, 0, winreg.KEY_ALL_ACCESS) as key:
            winreg.SetValueEx(key, "Marker", 0, winreg.REG_SZ, value)
            if winreg.QueryValueEx(key, "Marker")[0] != value:
                raise RuntimeError("marker was not readable through redirected HKCU")
        print("READY", flush=True)
        if sys.stdin.readline().strip() != "CONTINUE":
            raise RuntimeError("parent handshake failed")
        check(api.RegOverridePredefKey(HKEY_CURRENT_USER, None), "restore HKCU")
        mapped = False
        if not absent(marker):
            raise RuntimeError("marker escaped into the actual HKCU")
    finally:
        if mapped:
            api.RegOverridePredefKey(HKEY_CURRENT_USER, None)
        check(api.RegCloseKey(hive), "RegCloseKey")
    print("CLOSED", flush=True)
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--child", action="store_true")
    parser.add_argument("--hive", type=Path)
    parser.add_argument("--marker")
    parser.add_argument("--value")
    args = parser.parse_args()
    if args.child:
        return child(args.hive, args.marker, args.value)
    if sys.platform != "win32":
        raise RuntimeError("Windows is required")
    unique = uuid.uuid4().hex
    root = Path(r"C:\Samandarin\builds") / ("uac-thin-registry-probe-" + unique)
    root.mkdir(parents=False, exist_ok=False)
    hive_path = root / "private-hkcu.dat"
    marker = "Software\\SamandarinIsolationProbe-" + unique
    if not absent(marker):
        raise RuntimeError("unique marker already exists in actual HKCU")
    process = subprocess.Popen([sys.executable, "-B", str(Path(__file__).resolve()), "--child",
                                "--hive", str(hive_path), "--marker", marker, "--value", unique],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True, encoding="utf-8")
    try:
        ready = process.stdout.readline().strip()
        if ready != "READY":
            raise RuntimeError("child did not establish override: " + ready + " " + process.stderr.read())
        while_active = absent(marker)
        process.stdin.write("CONTINUE\n")
        process.stdin.flush()
        output, error = process.communicate(timeout=15)
        if process.returncode or "CLOSED" not in output:
            raise RuntimeError("child could not close mapping: " + error)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=5)
    after = absent(marker)
    api = registry_api()
    hive = wintypes.HKEY()
    check(api.RegLoadAppKeyW(str(hive_path), ctypes.byref(hive), KEY_ALL_ACCESS, REG_PROCESS_APPKEY, 0),
          "reopen application hive")
    try:
        with winreg.OpenKey(int(hive.value), marker, 0, winreg.KEY_READ) as key:
            persisted = winreg.QueryValueEx(key, "Marker")[0] == unique
    finally:
        check(api.RegCloseKey(hive), "close reopened hive")
    result = {"RealHkcuAbsentWhileMapped": while_active, "RealHkcuAbsentAfterChild": after,
              "IsolatedMarkerPersisted": persisted, "HiveClosedAndReopened": True,
              "HiveFile": str(hive_path), "HostStarted": False}
    print(json.dumps(result))
    return 0 if all((while_active, after, persisted)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
