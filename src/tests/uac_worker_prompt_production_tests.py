# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Extend singleton overwrite regression coverage. | Preserve cancellation, lifetime and prompt behavior. | #731 | GPT-6 (Codex) |
# | 2026-11-11 | Exercise real typed prompt and IDOK guard inside a bounded native harness. | Detect stack-lifetime regressions without mocking Windows messages. | #731 | GPT-6.1 Sol |
"""Extract production branches into a model dialog with real Windows messages."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    dialogs = ROOT / "src/dialogs.cpp"
    worker = ROOT / "src/worker.h"
    wide_header = ROOT / "src/common/widepath.h"
    wide_source = ROOT / "src/common/widepath.cpp"
    paths = (dialogs, worker, wide_header, wide_source)
    before = [hashlib.sha256(path.read_bytes()).hexdigest() for path in paths]
    text = dialogs.read_text(encoding="utf-8-sig")
    marker = "case WORKER_DIALOG_UAC_OVERWRITE_PROBE:"
    guard_marker = "if (WorkerOverwriteProbePromptActive)"
    if text.count(marker) != 1 or text.count(guard_marker) != 1:
        raise RuntimeError("Production prompt or completion guard is absent or ambiguous")
    prompt = block(worker.read_text(encoding="utf-8-sig"), "struct CUacOverwriteProbePrompt", ";")
    path_class = block(wide_header.read_text(encoding="utf-8-sig"), "class CPathBuffer", ";")
    wide_lines = wide_source.read_text(encoding="utf-8-sig").splitlines()
    path_impl = [line for line in wide_lines if line.startswith(("CPathBuffer::CPathBuffer()", "char* CPathBuffer::Data()", "int CPathBuffer::Capacity()"))]
    if len(path_impl) != 3:
        raise RuntimeError("Expected the production CPathBuffer constructor, Data and Capacity")
    path_buffer = path_class + "\nstatic const int SAL_MAX_EXTENDED_PATH = 32768;\n" + "\n".join(path_impl)
    remap_names = block(text, "char* RemapNames(")
    case = block(text, marker)
    guard = block(text, guard_marker)
    invalidation = "WorkerOverwriteProbePromptInvalidated = TRUE;"
    if guard.count(invalidation) != 1:
        raise RuntimeError("Expected exactly one completion-triggered invalidation")
    template = (ROOT / "src/tests/uac_worker_prompt_production_tests.cpp.in").read_text(encoding="utf-8")
    generated = (template.replace("@PROMPT_STRUCT@", prompt).replace("@PATH_BUFFER@", path_buffer)
                 .replace("@REMAP_NAMES@", remap_names).replace("@PROMPT_CASE@", case).replace("@IDOK_GUARD@", guard))
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-production-prompt-") as temp:
        source = Path(temp) / "extracted.cpp"
        source.write_text(generated, encoding="utf-8")
        for config in configurations:
            flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
            exe = Path(temp) / (config + ".exe")
            print(config, flush=True)
            result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                          *flags, str(source), "/Fe:" + str(exe), "/link", "user32.lib"],
                         cwd=temp, env=env, timeout=30)
            if result.returncode:
                return result.returncode
            for sent in ("posted", "sent"):
                for cancel in ("normal", "cancel"):
                    for remap in ("plain", "remap"):
                        for variant in ("single", "batch"):
                            try:
                                result = subprocess.run([str(exe), sent, cancel, remap, variant, "normal"], cwd=temp, env=env, timeout=7)
                            except subprocess.TimeoutExpired:
                                print(f"TIMEOUT: {config} {sent} {cancel} {remap} {variant}", flush=True)
                                return 1
                            if result.returncode:
                                return result.returncode
            try:
                result = subprocess.run([str(exe), "posted", "normal", "plain", "single", "oom"], cwd=temp, env=env, timeout=7)
            except subprocess.TimeoutExpired:
                print(f"TIMEOUT: {config} allocation failure", flush=True)
                return 1
            if result.returncode:
                return result.returncode
            # Negative control changes only the generated test copy. With a
            # queued Yes preceding WM_CLOSE it must expose the missing guard.
            control_source = Path(temp) / "missing_invalidation.cpp"
            control_exe = Path(temp) / (config + "-control.exe")
            control_source.write_text(generated.replace(invalidation,
                "WorkerOverwriteProbePromptInvalidated = FALSE;", 1), encoding="utf-8")
            result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                          *flags, str(control_source), "/Fe:" + str(control_exe), "/link", "user32.lib"],
                         cwd=temp, env=env, timeout=30)
            if result.returncode:
                return result.returncode
            try:
                result = subprocess.run([str(control_exe), "posted", "normal", "remap", "single", "normal"], cwd=temp, env=env, timeout=7)
            except subprocess.TimeoutExpired:
                print(f"TIMEOUT: {config} negative control", flush=True)
                return 1
            if result.returncode != 1:
                raise RuntimeError("Negative control failed to detect the missing invalidation")
            print("PASS: negative control detected stale Yes before queued WM_CLOSE", flush=True)
    after = [hashlib.sha256(path.read_bytes()).hexdigest() for path in paths]
    if before != after:
        raise RuntimeError("Extracted production source changed during test")
    print("Production input SHA-256:", *before)
    print("NOTE: real extracted prompt, IDOK guard, RemapNames and CPathBuffer; CProgressDialog and COverwriteDlg are test seams.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
