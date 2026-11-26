# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Validate test hive redirection before CRT startup. | Check concurrent private-hive processes and global constructors without launching the product. | #731 | GPT-6 |
"""Compile and run only the tiny pre-CRT probe, never Samandarin/Salmon."""
import ctypes
from ctypes import wintypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
import uuid
import winreg

from branch_panel_getters_tests import ROOT, compiler_environment, run
from uac_thin_registry_isolation_probe import KEY_ALL_ACCESS, absent, check, registry_api
from uac_thin_registry_shared_probe import first_line


def main():
    nonce = uuid.uuid4().hex
    root = Path(r"C:\Samandarin\builds") / ("uac-thin-precrt-" + nonce)
    app = root / "app"
    utils = app / "utils"
    utils.mkdir(parents=True)
    output = app / "host-probe.exe"
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    source = ROOT / "src/tests/uac_thin_precrt_probe.cpp"
    build = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                 "/MT", "/GS-", str(source), "/Fe:" + str(output),
                 "/link", "/SUBSYSTEM:CONSOLE", "advapi32.lib", "kernel32.lib"],
                cwd=str(root), env=env, timeout=60)
    if build.returncode:
        return build.returncode
    reporter = utils / "reporter-probe.exe"
    shutil.copyfile(output, reporter)
    marker = "Software\\SamandarinThinProbe-" + nonce
    if not absent(marker):
        raise RuntimeError("unexpected marker in real HKCU")
    children = []
    try:
        for program, reporter_flag in ((output, "0"), (reporter, "1")):
            child_env = os.environ.copy()
            child_env["SAM_UAC_TEST_ISOLATION_NONCE"] = nonce
            child_env["SAM_UAC_TEST_PRECRT_REPORTER"] = reporter_flag
            process = subprocess.Popen([str(program)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, env=child_env, text=True, encoding="utf-8")
            children.append(process)
            if first_line(process) != "READY":
                raise RuntimeError("pre-CRT probe did not reach its constructor")
        while_live = absent(marker)
        api = registry_api()
        hive = wintypes.HKEY()
        hive_path = app / "uac-test-hkcu.dat"
        check(api.RegLoadAppKeyW(str(hive_path), ctypes.byref(hive), KEY_ALL_ACCESS, 0, 0), "inspect live hive")
        try:
            with winreg.OpenKey(int(hive.value), marker, 0, winreg.KEY_READ) as key:
                live = all(winreg.QueryValueEx(key, name)[0] == 1
                           for name in ("Host", "Salmon", "HostCtor", "SalmonCtor"))
        finally:
            check(api.RegCloseKey(hive), "close live inspection")
        for process in children:
            process.stdin.write("X")
            process.stdin.flush()
        for process in children:
            stdout, stderr = process.communicate(timeout=10)
            if process.returncode:
                raise RuntimeError("pre-CRT child failed: " + stderr + stdout)
    finally:
        for process in children:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
    after_exit = absent(marker)
    api = registry_api()
    hive = wintypes.HKEY()
    check(api.RegLoadAppKeyW(str(hive_path), ctypes.byref(hive), KEY_ALL_ACCESS, 0, 0), "reopen test hive")
    try:
        with winreg.OpenKey(int(hive.value), marker, 0, winreg.KEY_READ) as key:
            after = all(winreg.QueryValueEx(key, name)[0] == 1 for name in
                        ("Host", "Salmon", "HostCtor", "SalmonCtor", "HostMain", "SalmonMain",
                         "HostDtor", "SalmonDtor"))
    finally:
        check(api.RegCloseKey(hive), "close test hive")

    # Both failure cases must terminate before the CRT constructor can emit
    # READY: absent private reporter hive and absent host isolation nonce.
    bad_root = root / "negative" / "app"
    bad_utils = bad_root / "utils"
    bad_utils.mkdir(parents=True)
    bad_host, bad_reporter = bad_root / "host-probe.exe", bad_utils / "reporter-probe.exe"
    shutil.copyfile(output, bad_host)
    shutil.copyfile(output, bad_reporter)
    bad_env = os.environ.copy()
    bad_env["SAM_UAC_TEST_ISOLATION_NONCE"] = nonce
    bad_env["SAM_UAC_TEST_PRECRT_REPORTER"] = "1"
    missing_hive = subprocess.run([str(bad_reporter)], input="", capture_output=True, text=True,
                                  env=bad_env, timeout=10)
    if missing_hive.returncode != 70 or "READY" in missing_hive.stdout:
        raise RuntimeError("reporter without a private hive was not stopped before the CRT")
    bad_env.pop("SAM_UAC_TEST_ISOLATION_NONCE")
    bad_env["SAM_UAC_TEST_PRECRT_REPORTER"] = "0"
    missing_nonce = subprocess.run([str(bad_host)], input="", capture_output=True, text=True,
                                   env=bad_env, timeout=10)
    if missing_nonce.returncode != 70 or "READY" in missing_nonce.stdout:
        raise RuntimeError("host without a private nonce was not stopped before the CRT")
    if not absent(marker):
        raise RuntimeError("negative paths modified the real HKCU marker")
    result = {"PrivateHiveDuringBothConstructors": live, "PrivateHiveAfterBothDestructors": after,
              "ActualHkcuAbsentWhileBothLive": while_live, "ActualHkcuAbsentAfterExit": after_exit,
              "MissingHiveAndNonceFailBeforeCrt": True,
              "SamandarinStarted": False, "SalmonStarted": False, "Evidence": str(root)}
    print(result)
    return 0 if all((live, after, while_live, after_exit)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
