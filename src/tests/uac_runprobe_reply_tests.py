# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise initial RunProbe framing with a local fake peer. | Verify read-only reply, cancellation and pipe handoff without claiming UAC. | #731 | GPT-6.1 Terra |
"""Test actual RunProbe after replacing installation and elevated-launch gates."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    flags = ("/MTd", "/Od", "/D_DEBUG") if args.configuration == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    broker = (ROOT / "src/uacbroker/uacbroker.cpp").read_text(encoding="utf-8-sig")
    declaration = "int RunProbe(const InstallLocation& install, bool drop, bool badVersion"
    assert broker.count(declaration) == 1
    broker = broker.replace(declaration, "HANDLE StartRunProbeFakePeer(const std::wstring&);\n\n" + declaration)
    run_at = broker.index(declaration)
    first = broker.index("    const auto brokerImageLock = install.OpenImage(brokerImage);", run_at)
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
    tests = (ROOT / "src/tests/uac_runprobe_reply_tests.cpp.in").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="uac-runprobe-") as temp:
        fixture = Path(temp) / "uac_runprobe_fixture.cpp"
        fixture.write_text(broker, encoding="utf-8")
        source = Path(temp) / "uac_runprobe_reply_tests.cpp"
        source.write_text(tests, encoding="utf-8")
        exe = Path(temp) / "uac_runprobe_reply_tests.exe"
        build = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                     *flags, "/I" + str(ROOT / "src/uacbroker"), str(source),
                     "/Fe:" + str(exe), "/link", "/MANIFEST:EMBED",
                     "advapi32.lib", "ole32.lib", "shell32.lib", "user32.lib", "uuid.lib", "ntdll.lib"],
                    cwd=temp, env=env, timeout=60)
        if build.returncode:
            return build.returncode
        return run([str(exe)], cwd=temp, env=env, timeout=30).returncode


if __name__ == "__main__":
    raise SystemExit(main())
