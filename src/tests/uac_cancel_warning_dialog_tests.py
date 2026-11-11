# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Test recovery warnings after cancellation through the actual UI filter. | Guard the prior transfer warning and ordinary prompt suppression. | #731 | GPT-6.1 Sol |
"""Extract the production cancellation filter and warning cases into a Win32 HWND harness."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    dialogs = ROOT / "src/dialogs.cpp"
    source_bytes = dialogs.read_bytes()
    source_hash = hashlib.sha256(source_bytes).hexdigest()
    text = source_bytes.decode("utf-8-sig").replace("\r\n", "\n")
    match = re.search(r"if\s*\(CancelWorker\s*&&\s*wParam\s*!=\s*WORKER_DIALOG_UAC_TREE_RESULT[\s\S]*?\)\s*return TRUE;", text)
    if not match:
        raise RuntimeError("Production cancellation gate not found")
    gate = match.group(0)
    exception = "&&\n            wParam != WORKER_DIALOG_UAC_DISK_RECOVERY_UNKNOWN"
    if gate.count(exception) != 1:
        raise RuntimeError("Recovery warning exception missing or unexpected")
    transfer = block(text, "case WORKER_DIALOG_UAC_TRANSFER_UNKNOWN:")
    recovery = block(text, "case WORKER_DIALOG_UAC_DISK_RECOVERY_UNKNOWN:")
    template = (ROOT / "src/tests/uac_cancel_warning_dialog_tests.cpp.in").read_text(encoding="utf-8")
    generated = (template.replace("@CANCEL_GATE@", gate).replace("@TRANSFER_CASE@", transfer)
                 .replace("@RECOVERY_CASE@", recovery))
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configs = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-cancel-warning-") as temp:
        for config in configs:
            flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
            for negative in (False, True):
                stem = config + ("-negative" if negative else "")
                cpp, exe = Path(temp) / (stem + ".cpp"), Path(temp) / (stem + ".exe")
                cpp.write_text(generated.replace(exception, "", 1) if negative else generated, encoding="utf-8")
                print(stem, flush=True)
                result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                              *flags, str(cpp), "/Fe:" + str(exe), "/link", "user32.lib"],
                             cwd=temp, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
                try:
                    result = subprocess.run([str(exe)], cwd=temp, env=env, timeout=7)
                except subprocess.TimeoutExpired:
                    print(f"TIMEOUT: {stem}", flush=True)
                    return 1
                if result.returncode != (1 if negative else 0):
                    raise RuntimeError("Unexpected positive/negative-control result")
                if negative:
                    print("PASS: negative control detected the suppressed recovery warning", flush=True)
    if hashlib.sha256(dialogs.read_bytes()).hexdigest() != source_hash:
        raise RuntimeError("Production dialogs.cpp changed during test")
    print("dialogs.cpp SHA-256:", source_hash)
    print("NOTE: real extracted gate and warning cases; CProgressDialog and SalMessageBox are test seams.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
