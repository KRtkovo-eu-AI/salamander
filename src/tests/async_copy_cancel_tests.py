# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Combine the actual transport check with a real peer exit. | Verify pending native I/O drains after an external process dies. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Extract the native thin pause wait. | Test synchronous connection-loss handling directly. | #731 | gpt-6.1-terra |
# | 2026-11-14 | Exercise worker handling of a disconnected thin job. | Check pending I/O is drained before reporting loss. | #731 | gpt-6.1-terra |
# | 2026-11-13 | Test cancellation after issuing real asynchronous I/O. | Ensure the native copy drain sees pending reads and writes. | #731 | GPT-6 |
"""Exercise extracted native async copy state against real pending pipe I/O."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def extract(text, start, end):
    first = text.index(start)
    return text[first:text.index(end, first)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    config = parser.parse_args().configuration
    worker_path = ROOT / "src/worker.cpp"
    header_path = ROOT / "src/worker.h"
    broker_path = ROOT / "src/uacbroker/uacbroker.cpp"
    before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in (worker_path, header_path, broker_path)}
    worker = worker_path.read_text(encoding="utf-8-sig")
    header = header_path.read_text(encoding="utf-8-sig")
    broker = broker_path.read_text(encoding="utf-8-sig")
    async_params = extract(worker, "struct CAsyncCopyParams\n{", "// **********************************************************************************\n")
    enums = extract(worker, "enum CCopy_BlkState\n", "struct CCopy_Context\n")
    issue = extract(worker, "BOOL CCopy_Context::StartReading(", "int CCopy_Context::FindBlock(")
    free = extract(worker, "void CCopy_Context::FreeBlock(", "void CCopy_Context::DiscardBlocksBehindEOF(")
    drain = extract(worker, "void CCopy_Context::CancelOpPhase1()", "BOOL CCopy_Context::RetryCopyReadErr(")
    suspend = extract(worker, "BOOL CCopy_Context::HandleSuspModeAndCancel(", "void DoCopyFileLoopAsync(")
    wait_resume = extract(worker, "DWORD CThinCopyWorker::WaitTransferResume()", "bool CThinCopyWorker::CancelBeforeElevation()")
    transport = extract(broker, "DWORD ThinCopySession::CheckTransport() const", "\n#endif\n\nbool ThinCopySession::Matches(")
    sync_loop = extract(worker, "void DoCopyFileLoopOrig(", "enum CCopy_BlkState")
    write_retry = sync_loop.split("case IDRETRY: // on a network", 1)[1].split("if (out != NULL)", 1)[0]
    read_retry = sync_loop.split("            RETRY_COPY:", 1)[1].split("if (in != NULL)", 1)[0]
    write_guard = extract(write_retry, "#ifdef SAM_UAC_THIN_COPY_TEST", "#endif") + "#endif\n"
    read_guard = extract(read_retry, "#ifdef SAM_UAC_THIN_COPY_TEST", "#endif") + "#endif\n"
    async_read = extract(worker, "BOOL CCopy_Context::HandleReadingErr(", "BOOL CCopy_Context::RetryCopyWriteErr(")
    async_write = extract(worker, "BOOL CCopy_Context::HandleWritingErr(", "BOOL CCopy_Context::HandleSuspModeAndCancel(")
    async_read_retry = async_read.split("case IDRETRY:", 1)[1].split("if (RetryCopyReadErr(", 1)[0]
    async_write_retry = async_write.split("case IDRETRY:", 1)[1].split("if (RetryCopyWriteErr(", 1)[0]
    async_read_guard = extract(async_read_retry, "#ifdef SAM_UAC_THIN_COPY_TEST", "#endif") + "#endif\n"
    async_write_guard = extract(async_write_retry, "#ifdef SAM_UAC_THIN_COPY_TEST", "#endif") + "#endif\n"
    buffer = re.search(r"^#define ASYNC_COPY_BUF_SIZE \([^\r\n]*", header, re.MULTILINE)
    if not buffer:
        raise RuntimeError("native copy buffer definition missing")
    template = (ROOT / "src/tests/async_copy_cancel_tests.cpp.in").read_text(encoding="utf-8")
    code = (template.replace("@@BUFFER@@", buffer.group(0)).replace("@@ASYNC@@", async_params)
            .replace("@@ENUMS@@", enums).replace("@@ISSUE@@", issue)
            .replace("@@FREE@@", free).replace("@@DRAIN@@", drain).replace("@@SUSPEND@@", suspend)
            .replace("@@WAIT_RESUME@@", wait_resume).replace("@@TRANSPORT@@", transport)
            .replace("@@SYNC_WRITE_RETRY_GUARD@@", write_guard).replace("@@SYNC_READ_RETRY_GUARD@@", read_guard)
            .replace("@@ASYNC_WRITE_RETRY_GUARD@@", async_write_guard)
            .replace("@@ASYNC_READ_RETRY_GUARD@@", async_read_guard))
    if "@@" in code:
        raise RuntimeError("unexpanded native test placeholder")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    with tempfile.TemporaryDirectory(prefix="async-copy-cancel-", dir=ROOT.parent / "builds") as temp:
        source = Path(temp) / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "test.exe"
        built = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", "/wd4201",
                     *flags, str(source), "/Fe:" + str(exe)], cwd=temp, env=env, timeout=50)
        if built.returncode:
            return built.returncode
        result = run([str(exe), temp], cwd=temp, env=env, timeout=25)
        if result.returncode:
            return result.returncode
    for path, digest in before.items():
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise RuntimeError(str(path) + " changed during test")
    print("worker.cpp SHA-256:", before[worker_path])
    print("uacbroker.cpp SHA-256:", before[broker_path])
    print("Actual extracted native issue and drain methods; real pending named-pipe I/O.")
    print("Not a complete F5 transfer, broker, UI, elevated session, or filesystem compatibility test.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
