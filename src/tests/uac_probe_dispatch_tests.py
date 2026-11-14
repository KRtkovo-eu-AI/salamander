# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Run probe frames through the broker dispatcher fixture. | Exercise message boundaries without interactive UAC. | #731 | GPT-6.1 Terra |
"""Exercise the real transfer-job pipe after the authentication seam."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--inject-recovery-1006", action="store_true",
                        help="test-only injected 1006 instead of calling Recover")
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    flags = ("/MTd", "/Od", "/D_DEBUG") if args.configuration == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    original = (ROOT / "src/uacbroker/uacbroker_job_tests.cpp").read_text(encoding="utf-8-sig")
    assert original.count("int wmain()") == 1
    original = original.replace("int wmain()", "int UnusedOriginalFixtureMain()")
    start = original.index("class JobPipe\n")
    end = original.index("\n};", start)
    job = original[start:end]
    assert job.count("private:") == 1
    original = original[:start] + job.replace("private:", "public:") + original[end:]
    tests = (ROOT / "src/tests/uac_probe_dispatch_tests.cpp.in").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="uac-probe-pipe-") as temp:
        if args.inject_recovery_1006:
            broker = (ROOT / "src/uacbroker/uacbroker.cpp").read_text(encoding="utf-8-sig")
            recovery = ("UacZipUpdate::Recover(targetParent, expectedTargetParent, targetLeaf, journal.get(), kind, ProtectedObject,\n"
                        "                                         UacZipUpdate::RecoveryIntent::Replace, pipe, deadline)")
            assert broker.count(recovery) == 1
            # Deliberately skip only this recovery call to force its result;
            # the existing journal is still opened by the real dispatcher.
            broker = broker.replace(recovery, "static_cast<DWORD>(ERROR_FILE_INVALID)")
            (Path(temp) / "uacbroker.cpp").write_text(broker, encoding="utf-8")
        source = Path(temp) / "uac_probe_dispatch_tests.cpp"
        source.write_text(original + "\n" + tests, encoding="utf-8")
        exe = Path(temp) / "uac_probe_dispatch_tests.exe"
        build = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/DSAM_UAC_PROBE_FAULT_TEST",
                     *flags, *(["/DSAM_UAC_FAULT_RECOVERY_1006"] if args.inject_recovery_1006 else []),
                     "/I" + str(ROOT / "src/uacbroker"), str(source),
                     "/Fe:" + str(exe), "/link", "/MANIFEST:EMBED",
                     "advapi32.lib", "ole32.lib", "shell32.lib", "uuid.lib", "ntdll.lib"],
                    cwd=temp, env=env, timeout=60)
        if build.returncode:
            return build.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
