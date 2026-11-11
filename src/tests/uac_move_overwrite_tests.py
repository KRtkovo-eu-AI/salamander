# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Run identity-preserving move overwrite and recovery tests. | Exercise the real NTFS backend on Unicode and extended-length paths. | #731 | GPT-6.1 Sol |
"""Verify live-source overwrite publication, cancellation and exact recovery."""
import argparse
import os
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
    with tempfile.TemporaryDirectory(prefix="uac-move-overwrite-") as temp:
        folder = Path(temp).resolve()
        exe = folder / "test.exe"
        build = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                     str(ROOT / "src/tests/uac_move_overwrite_tests.cpp"), "/Fe:" + str(exe),
                     "/link", "ntdll.lib", "ole32.lib", "advapi32.lib"], cwd=folder, env=env, timeout=35)
        if build.returncode:
            return build.returncode
        for name in ("short", "unicode-東/" + "/".join(["dír-日-" + "p" * 70] * 4)):
            fixture = (folder / name).resolve()
            if not fixture.is_relative_to(folder):
                raise ValueError("fixture escaped its owned parent")
            extended = "\\\\?\\" + str(fixture)
            os.makedirs(extended)
            try:
                result = run([str(exe), extended], cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
            finally:
                def writable_retry(function, path, _error):
                    os.chmod(path, stat.S_IWRITE)
                    function(path)
                branch = folder / name.split("/")[0]
                if branch.parent != folder:
                    raise ValueError("cleanup escaped the owned fixture parent")
                shutil.rmtree("\\\\?\\" + str(branch), onerror=writable_retry)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
