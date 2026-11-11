# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise the existing-target identity-open branch. | Cover a real NTFS denial after the broker-exists seam. | #731 | GPT-6.1 Terra |
# | 2026-11-12 | Extract the denied F5 call site and skipped-file branch. | Verify outcome routing without claiming a full worker test. | #731 | GPT-6.1 Terra |
# | 2026-11-12 | Extend singleton overwrite regression coverage. | Preserve cancellation, lifetime and prompt behavior. | #731 | GPT-6 (Codex) |
# | 2026-11-11 | Exercise the singleton F5 worker with the real host controller and a pipe peer. | Verify native choices and progress without claiming elevation. | #731 | GPT-6.1 Terra |
"""Run extracted worker against production host code and a fake named-pipe peer."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    args = parser.parse_args()
    inputs = [ROOT / "src/worker.cpp", ROOT / "src/uacbroker/uacbroker.cpp", ROOT / "src/uacbroker/f7pilot.h",
              ROOT / "src/worker.h"]
    before = [hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs]
    worker = read("src/worker.cpp")
    template = (ROOT / "src/tests/uac_single_worker_pipe_tests.cpp.in").read_text(encoding="utf-8")
    code = template.replace("@@DECISION@@", block(worker, "enum class SourceOverwriteResult", ";"))
    code = code.replace("@@DIALOG_PAYLOAD@@", block(read("src/worker.h"), "struct CUacOverwriteProbePrompt", ";"))
    code = code.replace("@@PROMPT_STATE@@", block(worker, "struct SingleProbedOverwritePrompt", ";"))
    code = code.replace("@@RECEIPT@@", block(worker, "struct CreateOnlySourceReceipt", ";"))
    code = code.replace("@@PROMPT_CALLBACK@@", block(worker, "static UacF7::ProbedOverwriteDecision PromptSingleProbedOverwrite("))
    code = code.replace("@@SINGLE_WORKER@@", block(worker, "static SourceOverwriteResult TrySingleProbedOverwrite("))
    code = code.replace("@@PARENT_OVERWRITE@@", block(worker, "static SourceOverwriteResult TryOverwriteDeniedSource("))
    callsite = block(worker, "if (!transferError && !copyAsEncrypted && (err == ERROR_ACCESS_DENIED || protectedExistingMove) && !dlgData.SkipAllFileOpenOut)")
    code = code.replace("@@DENIED_CALLSITE@@", callsite)
    copy_start = worker.index("BOOL DoCopyFile(")
    skip_start = worker.index("SKIP_OPEN:", copy_start) + len("SKIP_OPEN:")
    skip_end = worker.index("return TRUE;", skip_start) + len("return TRUE;")
    code = code.replace("@@SKIP_ACTION@@", worker[skip_start:skip_end])
    target_start = worker.index("out = HANDLES_Q(CreateFileW(WorkerOperationPathW(op->TargetName", copy_start)
    target_end = worker.index("out = NULL;", target_start) + len("out = NULL;")
    code = code.replace("@@TARGET_EXISTS_CALLSITE@@", worker[target_start:target_end])
    assert "@@" not in code

    broker = inputs[1].read_text(encoding="utf-8-sig")
    declaration = "int RunProbe(const InstallLocation& install, bool drop, bool badVersion"
    assert broker.count(declaration) == 1
    broker = broker.replace(declaration, "HANDLE StartRunProbeFakePeer(const std::wstring&);\n\n" + declaration)
    start = broker.index(declaration)
    first = broker.index("    const auto brokerImageLock = install.OpenImage(brokerImage);", start)
    last = broker.index("    auto deadline = GetTickCount64() + timeoutMs;\n    DWORD error = Connect(pipe.get(), deadline, stop);", first)
    seam = ("    // Test seam: no installed-image check or UAC process launch.\n"
            "    (void)install; (void)owner;\n"
            "    const auto child = Adopt(StartRunProbeFakePeer(pipeName));\n"
            "    if (!child) return Failed;\n"
            "    const DWORD pid = GetCurrentProcessId();\n")
    broker = broker[:first] + seam + broker[last:]
    elevation = "if (!InspectPipeClient(pipe.get(), elevated) || !elevated || !LiveInSession(child.get(), pid))"
    assert broker.count(elevation) == 1
    broker = broker.replace(elevation, "if (!InspectPipeClient(pipe.get(), elevated) || !LiveInSession(child.get(), pid))")
    start = broker.index("bool PreparedLeaf::PrepareTarget(")
    stop = broker.index("bool PreparedLeaf::CopyAllocationIdentity(", start)
    prepare = broker[start:stop]
    install = ("InstallLocation install;\n"
               "    if (parent || identityFromListing || !install.Open() || !install.MatchesProcess(GetCurrentProcess(), appImage) ||\n"
               "        !install.OpenImage(appImage) || !install.OpenImage(brokerImage))")
    assert prepare.count(install) == 1
    prepare = prepare.replace(install, "if (parent || identityFromListing) // test seam: installation gate")
    broker = broker[:start] + prepare + broker[stop:]
    start = broker.index("Result PreparedTransfer::ProbeOverwrite(")
    stop = broker.index("DWORD PreparedTransfer::AbortOverwriteProbe(", start)
    host = broker[start:stop]
    install = ("if (FAILED(com.result) || !install.Open() || !install.MatchesProcess(GetCurrentProcess(), appImage) ||\n"
               "        !install.OpenImage(appImage)) return result;")
    assert host.count(install) == 1
    host = host.replace(install, "if (FAILED(com.result)) return result; // test seam: installation gate")
    broker = broker[:start] + host + broker[stop:]
    start = broker.index("Result PreparedTransfer::Execute(")
    stop = broker.index("ArchiveReadSession::~ArchiveReadSession(", start)
    host = broker[start:stop]
    install = ("((HasDeferredTarget() || IsScopedTree()) && !job) || FAILED(com.result) || !install.Open() ||\n"
               "        !install.MatchesProcess(GetCurrentProcess(), appImage) || !install.OpenImage(appImage))")
    assert host.count(install) == 1
    host = host.replace(install, "((HasDeferredTarget() || IsScopedTree()) && !job) || FAILED(com.result)) // test seam: installation gate")
    broker = broker[:start] + host + broker[stop:]
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    flags = ("/MTd", "/Od", "/D_DEBUG") if args.configuration == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    with tempfile.TemporaryDirectory(prefix="uac-single-worker-") as temp:
        (Path(temp) / "uac_single_worker_fixture.cpp").write_text(broker, encoding="utf-8")
        source = Path(temp) / "uac_single_worker_pipe_tests.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "uac_single_worker_pipe_tests.exe"
        build = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                     "/I" + str(ROOT / "src/uacbroker"), str(source), "/Fe:" + str(exe),
                     "/link", "/MANIFEST:EMBED", "advapi32.lib", "ole32.lib", "shell32.lib",
                     "user32.lib", "uuid.lib", "ntdll.lib"], cwd=temp, env=env, timeout=60)
        if build.returncode:
            return build.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=40)
        if result.returncode:
            return result.returncode
    after = [hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs]
    if before != after:
        raise RuntimeError("Production sources changed while the test was running")
    print("Production input SHA-256:", *before)
    print("NOTE: actual worker and host controller, real pipe; fake peer/dialog, no installed-image or UAC gate.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
