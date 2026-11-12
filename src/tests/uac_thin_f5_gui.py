# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Prepare a scoped real-host F5 acceptance runner. | Observe the native worker and dialogs without simulating the transfer engine. | #731 | GPT-6 |
"""Run an ordinary, isolated F5 in the real host; this is not a UAC test.

The test build must opt into SAM_UAC_THIN_COPY_TEST. No protected target,
administrator approval, system-dialog automation or production installation
is used by this runner. Unexpected dialogs cause a failure, not a button click.
An existing test-only REGEDIT4 configuration must be supplied explicitly.
Execution is deliberately disabled until test-only HKCU isolation is proved.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid


WM_COMMAND = 0x0111
WM_CLOSE = 0x0010
WM_SETTEXT = 0x000C
CM_COPYFILES = 727
CM_ACTIVESELECTALL = 842
IDOK = 1
IDRETRY = 4
IDE_PATH = 210
IDS_SUBJECT = 209
IDS_FILENAME = 171
IDS_ERROR = 172
IDF_OPERATION = 155
IDF_SUMMARY = 156
WAIT_OBJECT_0 = 0
CREATE_NEW_CONSOLE = 0x00000010


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def inside(path, base):
    try:
        path.resolve().relative_to(base.resolve())
        return True
    except ValueError:
        return False


class Native:
    def __init__(self):
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        self.user.EnumWindows.argtypes = [self.callback, wintypes.LPARAM]
        self.user.EnumWindows.restype = wintypes.BOOL
        self.user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        self.user.GetWindowThreadProcessId.restype = wintypes.DWORD
        self.user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        self.user.GetClassNameW.restype = ctypes.c_int
        self.user.GetWindowTextLengthW.argtypes = [wintypes.HWND]
        self.user.GetWindowTextLengthW.restype = ctypes.c_int
        self.user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        self.user.GetWindowTextW.restype = ctypes.c_int
        self.user.IsWindowVisible.argtypes = [wintypes.HWND]
        self.user.IsWindowVisible.restype = wintypes.BOOL
        self.user.GetDlgItem.argtypes = [wintypes.HWND, ctypes.c_int]
        self.user.GetDlgItem.restype = wintypes.HWND
        self.user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        self.user.PostMessageW.restype = wintypes.BOOL
        self.user.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
                                                  wintypes.LPARAM, wintypes.UINT, wintypes.UINT,
                                                  ctypes.POINTER(ctypes.c_size_t)]
        self.user.SendMessageTimeoutW.restype = ctypes.c_size_t
        self.kernel.CreateEventW.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.BOOL, wintypes.LPCWSTR]
        self.kernel.CreateEventW.restype = wintypes.HANDLE
        self.kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        self.kernel.WaitForSingleObject.restype = wintypes.DWORD
        self.kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        self.kernel.CloseHandle.restype = wintypes.BOOL

    def text(self, handle):
        count = self.user.GetWindowTextLengthW(handle)
        buf = ctypes.create_unicode_buffer(count + 1)
        self.user.GetWindowTextW(handle, buf, len(buf))
        return buf.value

    def class_name(self, handle):
        buf = ctypes.create_unicode_buffer(256)
        self.user.GetClassNameW(handle, buf, len(buf))
        return buf.value

    def windows(self, pid):
        found = []

        @self.callback
        def visit(handle, unused):
            owner = wintypes.DWORD()
            self.user.GetWindowThreadProcessId(handle, ctypes.byref(owner))
            if owner.value == pid and self.user.IsWindowVisible(handle):
                found.append(handle)
            return True

        if not self.user.EnumWindows(visit, 0):
            raise ctypes.WinError(ctypes.get_last_error())
        return found

    def find_main(self, pid, prefix):
        matches = [h for h in self.windows(pid)
                   if self.class_name(h) == "SalamanderMainWindowVer25" and
                   prefix in self.text(h)]
        if len(matches) > 1:
            raise RuntimeError("more than one matching test main window")
        return matches[0] if matches else None

    def post(self, handle, message, value=0):
        if not self.user.PostMessageW(handle, message, value, 0):
            raise ctypes.WinError(ctypes.get_last_error())

    def set_text(self, handle, text):
        buf = ctypes.create_unicode_buffer(text)
        result = ctypes.c_size_t()
        if not self.user.SendMessageTimeoutW(handle, WM_SETTEXT, 0, ctypes.cast(buf, ctypes.c_void_p).value,
                                              0x0002, 3000, ctypes.byref(result)):
            raise RuntimeError("target edit did not accept WM_SETTEXT")
        if not result.value or self.text(handle) != text:
            raise RuntimeError("target edit did not retain the exact destination")

    def make_event(self, name):
        ctypes.set_last_error(0)
        event = self.kernel.CreateEventW(None, True, False, name)
        if not event:
            raise ctypes.WinError(ctypes.get_last_error())
        if ctypes.get_last_error() == 183:
            self.kernel.CloseHandle(event)
            raise RuntimeError("unique test event unexpectedly already exists")
        return event

    def signalled(self, event):
        return self.kernel.WaitForSingleObject(event, 0) == WAIT_OBJECT_0


def wait_until(condition, process, deadline, label):
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError("test host exited while waiting for " + label)
        value = condition()
        if value:
            return value
        time.sleep(0.05)
    raise TimeoutError("timed out waiting for " + label)


def prepare(root, template, executable, size):
    if root.exists():
        raise FileExistsError("refusing to reuse an existing fixture")
    root.mkdir(parents=False)
    source = root / "source"
    target = root / "target"
    source.mkdir()
    target.mkdir()
    app = root / "app"
    app.mkdir()
    (app / "lang").mkdir()
    # Copy only the minimum explicit test payload, never the neighboring
    # configstorage.ini, user configuration, plugin registry or build tree.
    for relative in ("salamand.exe", "uacbroker.exe", "lang/english.slg"):
        original = executable.parent / relative
        if not original.is_file():
            raise FileNotFoundError("missing required test payload: " + str(original))
        shutil.copyfile(original, app / relative)
        if digest(original) != digest(app / relative):
            raise RuntimeError("test payload copy hash mismatch: " + relative)
    config = app / "config.reg"
    shutil.copyfile(template, config)
    # File storage must be chosen before the program can consider importing
    # -c into the registry. This does NOT itself isolate pre-CRT or child
    # process HKCU access; execution stays disabled until that is proved.
    (app / "configstorage.ini").write_text(
        "[Configuration]\r\nStorageType=RegFile\r\nRegFilePath=" + str(config) + "\r\n",
        encoding="ascii", newline="")
    # The only source entry makes Select All deterministic. A non-repeating
    # byte pattern catches truncation and accidental content substitution.
    source_file = source / "01-copy-東.bin"
    with source_file.open("xb") as stream:
        block = bytes(range(256)) * 4096
        for unused in range(size):
            stream.write(block)
    return source, target, config, source_file, app / "salamand.exe"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="instrumented test salamand.exe")
    parser.add_argument("--config-template", type=Path, required=True, help="test-only REGEDIT4 profile")
    parser.add_argument("--mode", choices=("sync", "async"), required=True)
    parser.add_argument("--fault", choices=("none", "read", "write"), default="none")
    parser.add_argument("--size-mib", type=int, default=8, choices=range(1, 65), metavar="1..64")
    parser.add_argument("--run", action="store_true", help="actually open the isolated interactive test host")
    args = parser.parse_args()
    base = Path(r"C:\Samandarin\builds")
    if not inside(args.exe, base) or not args.exe.is_file() or args.exe.name.lower() != "salamand.exe":
        parser.error("the executable must be an existing test salamand.exe under C:\\Samandarin\\builds")
    if not inside(args.config_template, base) or not args.config_template.is_file():
        parser.error("the configuration template must be an existing test file under C:\\Samandarin\\builds")
    if not args.run:
        print("Preparation only: host was not started; no fixture was created.")
        return 0
    parser.error("interactive run disabled: test-only HKCU isolation is not yet proved")
    if sys.platform != "win32":
        raise RuntimeError("this test requires an isolated interactive Windows desktop")

    run_id = uuid.uuid4().hex
    root = base / ("uac-thin-f5-gui-" + run_id)
    source, target, config, source_file, test_exe = prepare(root, args.config_template, args.exe, args.size_mib)
    expected = digest(source_file)
    prefix = "UAC-Thin-F5-" + run_id
    env = os.environ.copy()
    env["SAM_UAC_TEST_COPY_MODE"] = args.mode
    env["SAM_UAC_TEST_FAIL"] = args.fault
    native = Native()
    events = {}
    process = None
    main_window = None
    try:
        for key in ("ENTER", "SYNC", "ASYNC"):
            name = "Local\\" + prefix + "-" + key
            env["SAM_UAC_TEST_" + key + "_EVENT"] = name
            events[key] = native.make_event(name)
        process = subprocess.Popen([str(test_exe), "-c", str(config), "-t", prefix, "-p", "1",
                                    "-l", str(source), "-r", str(target), "-language", "english"],
                                   cwd=str(test_exe.parent), env=env, creationflags=CREATE_NEW_CONSOLE)
        deadline = time.monotonic() + 45
        main_window = wait_until(lambda: native.find_main(process.pid, prefix), process, deadline, "test main window")
        native.post(main_window, WM_COMMAND, CM_ACTIVESELECTALL)
        native.post(main_window, WM_COMMAND, CM_COPYFILES)

        def copy_dialog():
            matches = []
            for handle in native.windows(process.pid):
                if native.class_name(handle) != "#32770":
                    continue
                path = native.user.GetDlgItem(handle, IDE_PATH)
                subject = native.user.GetDlgItem(handle, IDS_SUBJECT)
                if path and subject and native.user.GetDlgItem(handle, IDOK):
                    matches.append(handle)
            if len(matches) > 1:
                raise RuntimeError("more than one matching copy dialog")
            return matches[0] if matches else None

        dialog = wait_until(copy_dialog, process, deadline, "native Copy dialog")
        subject = native.text(native.user.GetDlgItem(dialog, IDS_SUBJECT))
        if source_file.name not in subject:
            raise RuntimeError("native Copy dialog does not identify the one test source: " + subject)
        native.set_text(native.user.GetDlgItem(dialog, IDE_PATH), str(target) + "\\")
        native.post(dialog, WM_COMMAND, IDOK)

        retried = False
        destination = target / source_file.name
        stable = 0
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError("test host exited during copy")
            for handle in native.windows(process.pid):
                if native.class_name(handle) != "#32770" or handle == dialog:
                    continue
                filename = native.user.GetDlgItem(handle, IDS_FILENAME)
                error = native.user.GetDlgItem(handle, IDS_ERROR)
                retry = native.user.GetDlgItem(handle, IDRETRY)
                if filename and error and retry and args.fault != "none" and not retried:
                    shown = native.text(filename)
                    if source_file.name not in shown:
                        raise RuntimeError("Retry dialog does not identify the test file")
                    native.post(handle, WM_COMMAND, IDRETRY)
                    retried = True
                elif native.user.GetDlgItem(handle, IDF_OPERATION) and native.user.GetDlgItem(handle, IDF_SUMMARY):
                    continue
                else:
                    raise RuntimeError("unexpected test-host dialog: " + native.text(handle))
            try:
                matches = destination.is_file() and digest(destination) == expected
            except OSError:
                matches = False
            stable = stable + 1 if matches else 0
            if stable >= 3:
                break
            time.sleep(0.2)
        else:
            raise TimeoutError("target was not completed")
        enter = native.signalled(events["ENTER"])
        chosen = native.signalled(events[args.mode.upper()])
        if not enter or not chosen:
            raise RuntimeError("the test build did not prove entry to DoCopyFile and its requested loop")
        if args.fault != "none" and not retried:
            raise RuntimeError("expected injected failure and Retry were not observed")
        if digest(source_file) != expected:
            raise RuntimeError("source changed")
        result = {"Fixture": str(root), "HostPid": process.pid, "Mode": args.mode,
                  "Fault": args.fault, "RetryObserved": retried, "SourceSha256": expected,
                  "TargetSha256": digest(destination), "EnteredDoCopyFile": enter,
                  "EnteredRequestedLoop": chosen, "SystemUacTested": False,
                  "OriginalHostSha256": digest(args.exe), "TestHostSha256": digest(test_exe),
                  "TemplateSha256": digest(args.config_template)}
        (root / "result.json").write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")
        print(json.dumps(result, ensure_ascii=False))
        return 0
    finally:
        if process is not None and process.poll() is None:
            if main_window:
                native.post(main_window, WM_CLOSE)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.terminate()  # This is only the process returned by our Popen.
                process.wait(timeout=5)
        for handle in events.values():
            native.kernel.CloseHandle(handle)


if __name__ == "__main__":
    raise SystemExit(main())
