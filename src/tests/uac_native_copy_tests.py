# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise interrupted read-only F6 stages and source retirement. | Verify recovery after real process exits at each publication boundary. | | GPT-6.1 Sol |
# | 2026-11-11 | Run native copy and source-retirement tests in owned NTFS fixtures. | Verify large copies and read-only cleanup including hard process exit. | | GPT-6.1 Sol |
"""Exercise native F5/F6 data transfer without elevation or a VM."""
import argparse
from pathlib import Path
import shutil
import stat
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    flags = ("/MTd", "/Od", "/D_DEBUG") if args.configuration == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    with tempfile.TemporaryDirectory(prefix="uac-native-copy-") as directory:
        folder = Path(directory).resolve()
        exe = folder / "test.exe"
        fixture = folder / "owned-fixture"
        fixture.mkdir()
        try:
            result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                          str(ROOT / "src/tests/uac_native_copy_tests.cpp"), "/Fe:" + str(exe), "/link",
                          "ntdll.lib", "ole32.lib", "advapi32.lib", "user32.lib"], cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            result = run([str(exe), str(fixture), "--run"], cwd=folder, env=env, timeout=25)
            if result.returncode:
                return result.returncode
            result = run([str(exe), str(fixture), "--crash"], cwd=folder, env=env, timeout=10)
            if result.returncode != 86:
                raise AssertionError(f"copy did not reach its hard-exit checkpoint: {result.returncode}")
            result = run([str(exe), str(fixture), "--verify-crash"], cwd=folder, env=env, timeout=10)
            if result.returncode:
                return result.returncode
            for phase in ("before", "retiring", "finished"):
                result = run([str(exe), str(fixture), "--move-crash-" + phase], cwd=folder, env=env, timeout=10)
                if result.returncode != 87:
                    raise AssertionError(f"read-only move did not reach {phase} checkpoint: {result.returncode}")
                result = run([str(exe), str(fixture), "--verify-move-crash-" + phase], cwd=folder, env=env, timeout=10)
                if result.returncode:
                    return result.returncode
            return 0
        finally:
            def clear_readonly(function, path, _error):
                Path(path).chmod(stat.S_IWRITE)
                function(path)
            shutil.rmtree("\\\\?\\" + str(fixture), onerror=clear_readonly)


if __name__ == "__main__":
    raise SystemExit(main())
