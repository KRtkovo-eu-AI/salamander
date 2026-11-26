# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Report scoped CopyAttrs coverage accurately. | Distinguish extracted completion from full F5 verification. | #731 | GPT-6 |
# | 2026-11-14 | Distinguish real API comparison from the stubbed completion branch. | Report the exact extent of filesystem verification. | #731 | GPT-6 |
# | 2026-11-14 | Include the completion connection guard. | Test both brokered and ordinary targets after a loss. | #731 | GPT-6 |
# | 2026-11-13 | Compare scoped and ordinary attribute finish behavior. | Preserve native plain-copy failure handling without pathname access. | #731 | GPT-6 |
"""Test the extracted F5 completion branch, not the full worker or UAC."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    config = parser.parse_args().configuration
    production = ROOT / "src/worker.cpp"
    before = hashlib.sha256(production.read_bytes()).hexdigest()
    worker = read("src/worker.cpp")
    start = worker.index("#ifdef SAM_UAC_THIN_COPY_TEST\n                        if (!thin.CheckCompletionConnection(IDS_ERRORCHANGINGATTRS))")
    end = worker.index("\n                    if (script->CopyAttrs) // verify", start)
    actual = worker[start:end]
    template = (ROOT / "src/tests/uac_thin_attribute_finish_tests.cpp.in").read_text(encoding="utf-8")
    code = template.replace("@@FINISH_BRANCH@@", actual)
    if "@@" in code:
        raise RuntimeError("Missing test template replacement")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    base = ROOT.parent / "builds"
    with tempfile.TemporaryDirectory(prefix="uac-thin-attribute-finish-", dir=base) as temp:
        source = Path(temp) / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "test.exe"
        built = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", *flags,
                     str(source), "/Fe:" + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(production.read_bytes()).hexdigest() != before:
        raise RuntimeError("worker.cpp changed during test")
    print("worker.cpp SHA-256:", before)
    print("Separate real-file API comparison; extracted completion branch uses filesystem, dialog, close and broker stubs.")
    print("CopyAttrs completion is exercised with a simulated scoped target and dialog.")
    print("This does not exercise the complete DoCopyFile worker, actual identity or ACLs, or UAC.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
