# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Probe two processes sharing an isolated application hive. | Prepare independent host and Bug Reporter isolation without editing HKCU. | #731 | GPT-6 |
"""Check two independent processes; does not start Samandarin or Salmon."""
import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import queue
import subprocess
import sys
import threading
import uuid
import winreg

from uac_thin_registry_isolation_probe import HKEY_CURRENT_USER, KEY_ALL_ACCESS, absent, check, registry_api


def child(hive_path, marker, own, peer):
    api = registry_api()
    hive = wintypes.HKEY()
    # Flags zero intentionally permit another process to open this same hive.
    check(api.RegLoadAppKeyW(str(hive_path), ctypes.byref(hive), KEY_ALL_ACCESS, 0, 0), "load shared hive")
    mapped = False
    try:
        check(api.RegOverridePredefKey(HKEY_CURRENT_USER, hive), "redirect HKCU")
        mapped = True
        with winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, marker, 0, winreg.KEY_ALL_ACCESS) as key:
            winreg.SetValueEx(key, own, 0, winreg.REG_DWORD, 1)
            winreg.FlushKey(key)
        print("READY", flush=True)
        if sys.stdin.readline().strip() != "CHECK":
            raise RuntimeError("parent handshake failed")
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, marker, 0, winreg.KEY_READ) as key:
            if winreg.QueryValueEx(key, peer)[0] != 1:
                raise RuntimeError("other process marker missing")
        check(api.RegOverridePredefKey(HKEY_CURRENT_USER, None), "restore HKCU")
        mapped = False
        if not absent(marker):
            raise RuntimeError("test marker escaped into real HKCU")
    finally:
        if mapped:
            api.RegOverridePredefKey(HKEY_CURRENT_USER, None)
        check(api.RegCloseKey(hive), "close application hive")
    print("CLOSED", flush=True)
    return 0


def first_line(process):
    result = queue.Queue()
    threading.Thread(target=lambda: result.put(process.stdout.readline()), daemon=True).start()
    try:
        return result.get(timeout=10).strip()
    except queue.Empty as error:
        raise TimeoutError("child never signaled readiness") from error


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--child", action="store_true")
    parser.add_argument("--hive", type=Path)
    parser.add_argument("--marker")
    parser.add_argument("--own")
    parser.add_argument("--peer")
    args = parser.parse_args()
    if args.child:
        return child(args.hive, args.marker, args.own, args.peer)
    unique = uuid.uuid4().hex
    root = Path(r"C:\Samandarin\builds") / ("uac-thin-shared-hive-" + unique)
    root.mkdir(parents=False, exist_ok=False)
    hive = root / "private-hkcu.dat"
    marker = "Software\\SamandarinSharedProbe-" + unique
    if not absent(marker):
        raise RuntimeError("unexpected marker in real HKCU")
    children = []
    try:
        for own, peer in (("Host", "Reporter"), ("Reporter", "Host")):
            child_process = subprocess.Popen([sys.executable, "-B", str(Path(__file__).resolve()), "--child",
                                               "--hive", str(hive), "--marker", marker,
                                               "--own", own, "--peer", peer],
                                              stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE, text=True, encoding="utf-8")
            children.append(child_process)
            if first_line(child_process) != "READY":
                raise RuntimeError("child failed before signalling shared hive")
        while_both_live = absent(marker)
        for child_process in children:
            child_process.stdin.write("CHECK\n")
            child_process.stdin.flush()
        for child_process in children:
            output, error = child_process.communicate(timeout=10)
            if child_process.returncode or "CLOSED" not in output:
                raise RuntimeError("child failed while checking shared hive: " + error)
    finally:
        for child_process in children:
            if child_process.poll() is None:
                child_process.kill()
                child_process.wait(timeout=5)
    after_both_exit = absent(marker)
    api = registry_api()
    reopened = wintypes.HKEY()
    check(api.RegLoadAppKeyW(str(hive), ctypes.byref(reopened), KEY_ALL_ACCESS, 0, 0), "reopen hive")
    try:
        with winreg.OpenKey(int(reopened.value), marker, 0, winreg.KEY_READ) as key:
            both_persisted = all(winreg.QueryValueEx(key, name)[0] == 1 for name in ("Host", "Reporter"))
    finally:
        check(api.RegCloseKey(reopened), "close reopened hive")
    result = {"ActualHkcuUnchangedWhileBothLive": while_both_live,
              "ActualHkcuUnchangedAfterExit": after_both_exit, "BothMarkersInPrivateHive": both_persisted,
              "HiveReopened": True, "HiveFile": str(hive), "SamandarinStarted": False, "SalmonStarted": False}
    print(json.dumps(result))
    return 0 if all((while_both_live, after_both_exit, both_persisted)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
