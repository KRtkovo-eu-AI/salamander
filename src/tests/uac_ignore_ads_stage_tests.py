# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise ignored ADS staging on NTFS. | Verify Unicode and long-path replacement preparation. | #731 | GPT-6.1 Terra |
"""Compile and exercise isolated ignored-ADS overwrite staging on NTFS."""
import argparse
import os
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
    with tempfile.TemporaryDirectory(prefix="uac-ignore-ads-") as temp:
        folder = Path(temp).resolve()
        exe = folder / "test.exe"
        result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                      str(ROOT / "src/tests/uac_ignore_ads_stage_tests.cpp"), "/Fe:" + str(exe),
                      "/link", "ntdll.lib", "ole32.lib", "advapi32.lib"], cwd=folder, env=env, timeout=35)
        if result.returncode:
            return result.returncode
        for relative in ("short", "unicode-東/" + "/".join(["dír-日-" + "p" * 70] * 4)):
            fixture = (folder / relative).resolve()
            if not fixture.is_relative_to(folder):
                raise ValueError("fixture escaped its owned parent")
            extended = "\\\\?\\" + str(fixture)
            os.makedirs(extended)
            result = run([str(exe), extended], cwd=folder, env=env, timeout=30)
            if result.returncode:
                return result.returncode
            shutil.rmtree("\\\\?\\" + str(folder / relative.split("/")[0]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
