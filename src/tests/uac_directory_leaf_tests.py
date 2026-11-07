# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Run the verified-parent mkdir experiment. | Exercise physical NTFS path swaps and junction rejection. | | GPT-6.1 Sol (Codex) |

"""Run the P3a verified-directory-handle fixture with real NTFS I/O."""
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
    source = ROOT / "src/tests/uac_directory_leaf_tests.cpp"

    with tempfile.TemporaryDirectory(prefix="uac-directory-leaf-") as temp:
        folder = Path(temp).resolve()
        exe = folder / "test.exe"
        fixture = folder / "owned-fixture"
        (fixture / "ancestor/parent").mkdir(parents=True)
        (fixture / "destination/nested").mkdir(parents=True)
        junction = fixture / "junction"
        env["SAMANDARIN_P3A_FIXTURE"] = str(fixture)
        junction_result = run([
            "powershell.exe", "-NoProfile", "-Command",
            "$f=$env:SAMANDARIN_P3A_FIXTURE; "
            "New-Item -ItemType Junction -Path (Join-Path $f 'junction') "
            "-Target (Join-Path $f 'destination') -ErrorAction Stop | Out-Null",
        ], cwd=folder, env=env, timeout=15)
        if junction_result.returncode:
            return junction_result.returncode

        try:
            build = run([
                compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                *flags, str(source), "/Fe:" + str(exe), "/link", "ntdll.lib",
            ], cwd=folder, env=env, timeout=35)
            if build.returncode:
                return build.returncode
            return run([str(exe), str(fixture)], cwd=folder, env=env, timeout=15).returncode
        finally:
            junction.rmdir()


if __name__ == "__main__":
    raise SystemExit(main())
