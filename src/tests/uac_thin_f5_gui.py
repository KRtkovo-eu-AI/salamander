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
import winreg
import uac_thin_process_memory
from uac_thin_registry_isolation_probe import KEY_ALL_ACCESS, absent, check, registry_api


WM_COMMAND = 0x0111
WM_CLOSE = 0x0010
WM_SETTEXT = 0x000C
CM_COPYFILES = 727
CM_ACTIVESELECTALL = 842
IDOK = 1
IDCANCEL = 2
IDRETRY = 4
IDYES = 6
IDNO = 7
IDB_SKIP = 173
IDE_PATH = 210
IDS_SUBJECT = 209
IDS_FILENAME = 171
IDS_ERROR = 172
IDS_SOURCENAME = 181
IDS_TARGETNAME = 183
IDS_MSGBOX_TEXT = 2477
IDF_OPERATION = 155
IDF_SUMMARY = 156
IDB_PAUSERESUME = 158
IDC_MORE = 212
IDC_CM_SPEEDLIMIT = 225
IDE_CM_SPEEDLIMIT = 226
IDC_CM_SPEEDLIMITUNITS = 227
IDC_CM_TRANSFERMODE = 1818
BM_CLICK = 0x00F5
BM_GETCHECK = 0x00F0
CB_GETCURSEL = 0x0147
CB_SETCURSEL = 0x014E
GW_OWNER = 4
WAIT_OBJECT_0 = 0
CREATE_NEW_CONSOLE = 0x00000010


def digest(path):
    value = hashlib.sha256()
    with open(windows_long(path), "rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def windows_long(path):
    """Use an explicit extended path for our own long test fixtures."""
    value = str(path)
    if os.name == "nt" and len(value) >= 248 and not value.startswith("\\\\?\\"):
        return "\\\\?\\" + value
    return value


def exists(path):
    return os.path.exists(windows_long(path))


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
        self.user.IsWindowEnabled.argtypes = [wintypes.HWND]
        self.user.IsWindowEnabled.restype = wintypes.BOOL
        self.user.IsWindow.argtypes = [wintypes.HWND]
        self.user.IsWindow.restype = wintypes.BOOL
        self.user.GetDlgItem.argtypes = [wintypes.HWND, ctypes.c_int]
        self.user.GetDlgItem.restype = wintypes.HWND
        self.user.GetWindow.argtypes = [wintypes.HWND, wintypes.UINT]
        self.user.GetWindow.restype = wintypes.HWND
        self.user.GetPropW.argtypes = [wintypes.HWND, wintypes.LPCWSTR]
        self.user.GetPropW.restype = wintypes.HANDLE
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
        self.kernel.SetEvent.argtypes = [wintypes.HANDLE]
        self.kernel.SetEvent.restype = wintypes.BOOL

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

    def send(self, handle, message, value=0):
        result = ctypes.c_size_t()
        if not self.user.SendMessageTimeoutW(handle, message, value, 0, 0x0002, 3000,
                                              ctypes.byref(result)):
            raise RuntimeError("native control did not respond")
        return result.value

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

    def broker_property(self, windows):
        pairs = set()
        for window in windows:
            if not window:
                continue
            owner = self.user.GetPropW(window, "SamUacThinTestOwner")
            pid = self.user.GetPropW(window, "SamUacThinTestBrokerPid")
            if owner and pid:
                pairs.add((int(owner), int(pid)))
        if len(pairs) > 1:
            raise RuntimeError("multiple authenticated test broker markers are ambiguous")
        return next(iter(pairs)) if pairs else None


def wait_until(condition, process, deadline, label):
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError("test host exited while waiting for " + label)
        value = condition()
        if value:
            return value
        time.sleep(0.05)
    raise TimeoutError("timed out waiting for " + label)


def configure_native_copy(native, dialog, process, deadline, speed_kib):
    """Select the original sequential worker and optional native speed limit."""
    more = native.user.GetDlgItem(dialog, IDC_MORE)
    mode = native.user.GetDlgItem(dialog, IDC_CM_TRANSFERMODE)
    limit = native.user.GetDlgItem(dialog, IDC_CM_SPEEDLIMIT)
    if not more or not mode or not limit:
        raise RuntimeError("the native Copy dialog lacks expected options")
    if not native.user.IsWindowVisible(mode):
        native.post(dialog, WM_COMMAND, IDC_MORE)
        wait_until(lambda: native.user.IsWindowVisible(mode), process, deadline, "Copy options")
    native.send(mode, CB_SETCURSEL, 0)  # CMS_SEQUENTIAL
    if native.send(mode, CB_GETCURSEL) != 0:
        raise RuntimeError("sequential transfer mode was not selected")
    if speed_kib:
        if native.send(limit, BM_GETCHECK) != 1:
            native.post(limit, BM_CLICK)
            wait_until(lambda: native.send(limit, BM_GETCHECK) == 1, process, deadline, "speed limit toggle")
        number = native.user.GetDlgItem(dialog, IDE_CM_SPEEDLIMIT)
        units = native.user.GetDlgItem(dialog, IDC_CM_SPEEDLIMITUNITS)
        if not number or not units:
            raise RuntimeError("speed-limit controls are unavailable")
        native.set_text(number, str(speed_kib))
        native.send(units, CB_SETCURSEL, 1)  # KiB/s
        if native.send(units, CB_GETCURSEL) != 1:
            raise RuntimeError("speed-limit unit did not remain selected")
    elif native.send(limit, BM_GETCHECK) == 1:
        native.post(limit, BM_CLICK)
        wait_until(lambda: native.send(limit, BM_GETCHECK) == 0, process, deadline, "speed limit disabled")


def find_progress(native, pid):
    matches = [h for h in native.windows(pid)
               if native.class_name(h) == "#32770" and
               native.user.GetDlgItem(h, IDF_OPERATION) and
               native.user.GetDlgItem(h, IDF_SUMMARY) and
               native.user.GetDlgItem(h, IDB_PAUSERESUME)]
    if len(matches) > 1:
        raise RuntimeError("more than one progress window belongs to the test host")
    return matches[0] if matches else None


def confirm_own_cancel(native, process, progress, deadline):
    def own_question():
        matches = []
        for handle in native.windows(process.pid):
            if native.class_name(handle) != "#32770" or native.user.GetWindow(handle, GW_OWNER) != progress:
                continue
            text = native.user.GetDlgItem(handle, IDS_MSGBOX_TEXT)
            if text and native.text(text) == "Do you want to cancel this operation?" and \
                    native.user.GetDlgItem(handle, IDYES) and native.user.GetDlgItem(handle, IDNO):
                matches.append(handle)
        if len(matches) > 1:
            raise RuntimeError("ambiguous cancellation confirmation")
        return matches[0] if matches else None

    native.post(progress, WM_COMMAND, IDCANCEL)
    question = wait_until(own_question, process, deadline, "own cancel confirmation")
    native.post(question, WM_COMMAND, IDYES)
    wait_until(lambda: not native.user.IsWindow(question), process, deadline, "cancel confirmation close")
    pause = native.user.GetDlgItem(progress, IDB_PAUSERESUME)
    if not pause:
        raise RuntimeError("cancelled progress window lost its pause control")
    # CProgressDlg signals CancelWorkerEvent before disabling Pause.
    wait_until(lambda: not native.user.IsWindowEnabled(pause), process, deadline, "native cancellation signal")


def own_hive_ready(hive_path, marker):
    """Check both child markers only through the private hive's own handle."""
    if not hive_path.is_file():
        return False
    api = registry_api()
    hive = wintypes.HKEY()
    status = api.RegLoadAppKeyW(str(hive_path), ctypes.byref(hive), KEY_ALL_ACCESS, 0, 0)
    if status:
        return False
    try:
        try:
            with winreg.OpenKey(int(hive.value), marker, 0, winreg.KEY_READ) as key:
                return all(winreg.QueryValueEx(key, value)[0] == 1 for value in ("Host", "Salmon"))
        except FileNotFoundError:
            return False
    finally:
        check(api.RegCloseKey(hive), "close test hive observation")


def prepare(root, template, executable, size, long_path, collision, include_reporter=False):
    if root.exists():
        raise FileExistsError("refusing to reuse an existing fixture")
    root.mkdir(parents=False)
    source = root / "source"
    target = root / "target"
    source.mkdir()
    target.mkdir()
    if long_path:
        for index in range(4):
            component = str(index) + "東" + ("long" * 17)
            source = source / component
            target = target / component
            os.mkdir(windows_long(source))
            os.mkdir(windows_long(target))
    app = root / "app"
    app.mkdir()
    (app / "lang").mkdir()
    # Copy only the minimum explicit test payload, never the neighboring
    # configstorage.ini, user configuration, plugin registry or build tree.
    payloads = ["salamand.exe", "uacbroker.exe", "lang/english.slg"]
    if include_reporter:
        (app / "utils").mkdir()
        payloads.append("utils/salmon.exe")
    for relative in payloads:
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
    # The only source entry makes Select All deterministic. Each 4-KiB block
    # has a distinct deterministic pattern, so reordered blocks do not match.
    source_file = source / "01-copy-東.bin"
    with open(windows_long(source_file), "xb") as stream:
        for megabyte in range(size):
            for block in range(256):
                seed = ("uac-thin-f5-%d-%d" % (megabyte, block)).encode("ascii")
                stream.write(hashlib.sha256(seed).digest() * 128)
    if collision != "none":
        with open(windows_long(target / source_file.name), "xb") as stream:
            stream.write(b"uac-thin-f5-original-target-sentinel")
    return source, target, config, source_file, app / "salamand.exe"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="instrumented test salamand.exe")
    parser.add_argument("--config-template", type=Path, required=True, help="test-only REGEDIT4 profile")
    parser.add_argument("--expected-host-sha256", help="SHA-256 of the verified isolated test build")
    parser.add_argument("--expected-reporter-sha256", help="SHA-256 of its verified isolated reporter")
    parser.add_argument("--expected-broker-image", type=Path, help="verified test broker image for memory samples")
    parser.add_argument("--mode", choices=("sync", "async"), required=True)
    parser.add_argument("--fault", choices=("none", "read", "write"), default="none")
    parser.add_argument("--deny-open", choices=("source", "target", "both"),
                        help="inject one access denial per selected side in the test-only worker")
    parser.add_argument("--scenario", choices=("copy", "pause", "cancel", "speed"), default="copy")
    parser.add_argument("--cancel-after-handle", choices=("source", "target"),
                        help="test-only gate after a broker handle is accepted")
    parser.add_argument("--replace-before-retry", choices=("source", "target"),
                        help="replace only the test fixture after its worker handle is closed")
    parser.add_argument("--collision", choices=("none", "replace", "skip"), default="none")
    parser.add_argument("--long-path", action="store_true", help="prepare multi-component Unicode paths beyond MAX_PATH")
    parser.add_argument("--speed-kib", type=int, default=1024, choices=range(1024, 8193), metavar="1024..8192")
    parser.add_argument("--size-mib", type=int, default=8, choices=range(1, 65), metavar="1..64")
    parser.add_argument("--run", action="store_true", help="actually open the isolated interactive test host")
    args = parser.parse_args()
    base = Path(r"C:\Samandarin\builds")
    if not inside(args.exe, base) or not args.exe.is_file() or args.exe.name.lower() != "salamand.exe":
        parser.error("the executable must be an existing test salamand.exe under C:\\Samandarin\\builds")
    if not inside(args.config_template, base) or not args.config_template.is_file():
        parser.error("the configuration template must be an existing test file under C:\\Samandarin\\builds")
    if args.fault != "none" and args.scenario != "copy":
        parser.error("retry faults and pause/cancel/speed are separate test scenarios")
    if args.collision != "none" and (args.scenario != "copy" or args.fault != "none"):
        parser.error("collision, fault and progress cases are separate test scenarios")
    if args.cancel_after_handle and args.scenario != "cancel":
        parser.error("handle-gated cancellation requires --scenario cancel")
    if args.replace_before_retry and (args.fault == "none" or args.scenario != "copy" or args.collision != "none"):
        parser.error("retry replacement requires a standalone read or write fault")
    if not args.run:
        print("Preparation only: host was not started; no fixture was created.")
        return 0
    parser.error("interactive run disabled: test-only HKCU isolation is not yet proved")
    if sys.platform != "win32":
        raise RuntimeError("this test requires an isolated interactive Windows desktop")

    run_id = uuid.uuid4().hex
    root = base / ("uac-thin-f5-gui-" + run_id)
    reporter_source = args.exe.parent / "utils" / "salmon.exe"
    if not reporter_source.is_file() or not args.expected_host_sha256 or not args.expected_reporter_sha256:
        raise RuntimeError("the independently verified test host and reporter images are required")
    if digest(args.exe).lower() != args.expected_host_sha256.lower() or \
            digest(reporter_source).lower() != args.expected_reporter_sha256.lower():
        raise RuntimeError("test host or reporter hash does not match the verified build")
    magic = "SAM_UAC_TEST_ISOLATION_NONCE".encode("utf-16le")
    if magic not in args.exe.read_bytes() or magic not in reporter_source.read_bytes():
        raise RuntimeError("test host and reporter must both contain the isolation hook")
    source, target, config, source_file, test_exe = prepare(root, args.config_template, args.exe, args.size_mib,
                                                           args.long_path, args.collision, include_reporter=True)
    expected = digest(source_file)
    original_target = digest(target / source_file.name) if args.collision != "none" else None
    prefix = "UAC-Thin-F5-" + run_id
    # Do not let a shell's earlier acceptance settings select an unintended
    # test hook, path, marker, or fault in this new process.
    env = {key: value for key, value in os.environ.items() if not key.upper().startswith("SAM_UAC_TEST_")}
    env["SAM_UAC_TEST_COPY_MODE"] = args.mode
    env["SAM_UAC_TEST_FAIL"] = args.fault
    if args.deny_open:
        env["SAM_UAC_TEST_DENY_OPEN"] = args.deny_open
    if args.cancel_after_handle:
        env["SAM_UAC_TEST_HANDLE_ROLE"] = args.cancel_after_handle
    if args.replace_before_retry:
        env["SAM_UAC_TEST_RETRY_ROLE"] = args.replace_before_retry
    env["SAM_UAC_TEST_ISOLATION_NONCE"] = run_id
    marker = "Software\\SamandarinThinProbe-" + run_id
    if not absent(marker):
        raise RuntimeError("test isolation marker already exists in actual HKCU")
    isolated_profile = root / "profile"
    for relative in ("AppData/Roaming", "AppData/Local", "Temp"):
        (isolated_profile / relative).mkdir(parents=True)
    env["USERPROFILE"] = str(isolated_profile)
    env["APPDATA"] = str(isolated_profile / "AppData" / "Roaming")
    env["LOCALAPPDATA"] = str(isolated_profile / "AppData" / "Local")
    env["TEMP"] = env["TMP"] = str(isolated_profile / "Temp")
    native = Native()
    events = {}
    process = None
    main_window = None
    try:
        for key in ("ENTER", "SYNC", "ASYNC"):
            name = "Local\\" + prefix + "-" + key
            env["SAM_UAC_TEST_" + key + "_EVENT"] = name
            events[key] = native.make_event(name)
        if args.cancel_after_handle:
            for key in ("HANDLE_EVENT", "HANDLE_RELEASE"):
                name = "Local\\" + prefix + "-" + key
                env["SAM_UAC_TEST_" + key] = name
                events[key] = native.make_event(name)
        if args.replace_before_retry:
            for key in ("RETRY_EVENT", "RETRY_RELEASE"):
                name = "Local\\" + prefix + "-" + key
                env["SAM_UAC_TEST_" + key] = name
                events[key] = native.make_event(name)
        process = subprocess.Popen([str(test_exe), "-c", str(config), "-t", prefix, "-p", "1",
                                    "-l", str(source), "-r", str(target), "-language", "english"],
                                   cwd=str(test_exe.parent), env=env, creationflags=CREATE_NEW_CONSOLE)
        deadline = time.monotonic() + 90
        main_window = wait_until(lambda: native.find_main(process.pid, prefix), process, deadline, "test main window")
        wait_until(lambda: own_hive_ready(test_exe.parent / "uac-test-hkcu.dat", marker),
                   process, deadline, "host and Salmon private hive markers")
        if not absent(marker):
            raise RuntimeError("isolation marker appeared in actual HKCU")
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
        speed_kib = args.speed_kib if args.scenario in ("pause", "cancel", "speed") else 0
        configure_native_copy(native, dialog, process, deadline, speed_kib)
        started = time.monotonic()
        native.post(dialog, WM_COMMAND, IDOK)

        retried = False
        collision_answered = False
        replaced_before_retry = False
        replacement_hash = None
        original_backup_hash = None
        replacement_error = None
        backup_path = None
        action = False
        pause_observation = None
        memory_samples = []
        last_sample = 0.0
        destination = target / source_file.name
        stable = 0
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError("test host exited during copy")
            progress = find_progress(native, process.pid)
            if args.replace_before_retry and native.signalled(events["RETRY_EVENT"]) and not replaced_before_retry:
                if not retried:
                    raise RuntimeError("retry gate appeared before the injected failure was acknowledged")
                tested = source_file if args.replace_before_retry == "source" else destination
                if not exists(tested):
                    raise RuntimeError("expected original test object is absent at retry gate")
                backup_path = root / ("before-retry-" + args.replace_before_retry + ".bin")
                if exists(backup_path):
                    raise RuntimeError("refusing to overwrite an existing backup")
                original_backup_hash = digest(tested)
                os.rename(windows_long(tested), windows_long(backup_path))
                with open(windows_long(tested), "xb") as replacement:
                    replacement.write(b"uac-thin-f5-retry-replacement-must-remain-unchanged")
                replacement_hash = digest(tested)
                replaced_before_retry = True
                if not native.kernel.SetEvent(events["RETRY_RELEASE"]):
                    raise ctypes.WinError(ctypes.get_last_error())
            now = time.monotonic()
            if now - last_sample >= 0.5:
                host_sample = uac_thin_process_memory.sample(process.pid, str(test_exe))
                broker_sample = {"status": "not_observed", "reason": "no authenticated broker PID"}
                broker_marker = native.broker_property((progress, main_window))
                if broker_marker:
                    expected_broker = args.expected_broker_image or (test_exe.parent / "uacbroker.exe")
                    broker_sample = uac_thin_process_memory.sample(broker_marker[1], str(expected_broker))
                    if broker_marker != native.broker_property((progress, main_window)):
                        broker_sample = {"status": "unavailable", "reason": "session marker changed during sample"}
                    else:
                        broker_sample["pid_provenance"] = "test-only marker after pipe and token authentication"
                memory_samples.append({"seconds": now - started, "host": host_sample, "broker": broker_sample})
                last_sample = now
            accepted = not args.cancel_after_handle or native.signalled(events["HANDLE_EVENT"])
            if progress and native.signalled(events["ENTER"]) and accepted and not action:
                if args.scenario == "pause":
                    native.post(progress, WM_COMMAND, IDB_PAUSERESUME)
                    pause_button = native.user.GetDlgItem(progress, IDB_PAUSERESUME)
                    wait_until(lambda: native.text(pause_button).replace("&", "") == "Resume",
                               process, deadline, "paused native progress dialog")
                    time.sleep(0.5)  # allow any already-issued I/O to drain
                    before = uac_thin_process_memory.sample(process.pid, str(test_exe))
                    paused_at = time.monotonic()
                    time.sleep(1.0)
                    after = uac_thin_process_memory.sample(process.pid, str(test_exe))
                    pause_observation = {"seconds": time.monotonic() - paused_at,
                                         "before": before, "after": after,
                                         "note": "I/O counters are process-wide, not per worker"}
                    native.post(progress, WM_COMMAND, IDB_PAUSERESUME)
                    wait_until(lambda: native.text(pause_button).replace("&", "") == "Pause",
                               process, deadline, "resumed native progress dialog")
                    action = True
                elif args.scenario == "cancel":
                    confirm_own_cancel(native, process, progress, deadline)
                    if args.cancel_after_handle and not native.kernel.SetEvent(events["HANDLE_RELEASE"]):
                        raise ctypes.WinError(ctypes.get_last_error())
                    action = True
            for handle in native.windows(process.pid):
                if native.class_name(handle) != "#32770":
                    continue
                if handle == dialog and native.user.GetDlgItem(handle, IDE_PATH):
                    continue
                filename = native.user.GetDlgItem(handle, IDS_FILENAME)
                error = native.user.GetDlgItem(handle, IDS_ERROR)
                retry = native.user.GetDlgItem(handle, IDRETRY)
                old_name = native.user.GetDlgItem(handle, IDS_SOURCENAME)
                new_name = native.user.GetDlgItem(handle, IDS_TARGETNAME)
                if old_name and new_name and args.collision != "none" and not collision_answered:
                    if source_file.name not in native.text(old_name) or source_file.name not in native.text(new_name):
                        raise RuntimeError("overwrite dialog does not identify the test objects")
                    decision = IDYES if args.collision == "replace" else IDB_SKIP
                    if not native.user.GetDlgItem(handle, decision):
                        raise RuntimeError("native overwrite decision is unavailable")
                    native.post(handle, WM_COMMAND, decision)
                    collision_answered = True
                elif filename and error and replaced_before_retry and "1006" in native.text(error):
                    shown = native.text(filename)
                    if source_file.name not in shown:
                        raise RuntimeError("identity error does not identify the test file")
                    replacement_error = native.text(error)
                    # Leave the native error untouched. In particular, Skip's
                    # cleanup behavior is not assumed safe in this probe.
                elif filename and error and retry and args.fault != "none" and not retried:
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
                if args.scenario == "cancel":
                    matches = action and not find_progress(native, process.pid) and not exists(destination)
                elif args.replace_before_retry:
                    tested = source_file if args.replace_before_retry == "source" else destination
                    matches = replacement_error and exists(tested) and exists(backup_path) and \
                              digest(tested) == replacement_hash and digest(backup_path) == original_backup_hash
                else:
                    wanted = original_target if args.collision == "skip" else expected
                    matches = (args.collision == "none" or collision_answered) and \
                              not find_progress(native, process.pid) and exists(destination) and digest(destination) == wanted
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
        if args.collision != "skip" and (not enter or not chosen):
            raise RuntimeError("the test build did not prove entry to DoCopyFile and its requested loop")
        if args.collision != "none" and not collision_answered:
            raise RuntimeError("expected native overwrite decision was not observed")
        if args.fault != "none" and not retried:
            raise RuntimeError("expected injected failure and Retry were not observed")
        if args.scenario in ("pause", "cancel") and not action:
            raise RuntimeError("the requested native progress action was not observed")
        if args.replace_before_retry and not replacement_error:
            raise RuntimeError("expected identity error 1006 was not observed")
        if args.replace_before_retry != "source" and digest(source_file) != expected:
            raise RuntimeError("source changed")
        result = {"Fixture": str(root), "HostPid": process.pid, "Mode": args.mode, "Scenario": args.scenario,
                  "Fault": args.fault, "DenyOpen": args.deny_open, "Collision": args.collision, "LongPath": args.long_path,
                  "CancelAfterHandle": args.cancel_after_handle,
                  "HandleGateObserved": bool(args.cancel_after_handle and native.signalled(events["HANDLE_EVENT"])),
                  "RetryReplacement": args.replace_before_retry, "ReplacementSha256": replacement_hash,
                  "BackupSha256": original_backup_hash, "IdentityError": replacement_error,
                  "CollisionAnswered": collision_answered, "OriginalTargetSha256": original_target,
                  "RetryObserved": retried, "SourceSha256": expected,
                  "TargetSha256": digest(destination) if exists(destination) else None,
                  "TargetAbsent": not exists(destination), "SpeedLimitKiB": speed_kib,
                  "ElapsedSeconds": time.monotonic() - started, "PauseObservation": pause_observation,
                  "MemorySamples": memory_samples, "EnteredDoCopyFile": enter,
                  "EnteredRequestedLoop": chosen, "SystemUacTested": False,
                  "OriginalHostSha256": digest(args.exe), "TestHostSha256": digest(test_exe),
                  "TemplateSha256": digest(args.config_template),
                  "PrivateHiveHostAndSalmonObserved": True,
                  "ActualHkcuMarkerAbsentWhileRunning": True}
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
    if not absent(marker):
        raise RuntimeError("isolation marker appeared in actual HKCU after host exit")
    result["ActualHkcuMarkerAbsentAfterExit"] = True
    (root / "result.json").write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
