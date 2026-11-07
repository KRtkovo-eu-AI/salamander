# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Run the broker transport fixture. | Exercise both live pipe peer checks in native CI. | | GPT-6.1 Sol (Codex) |

"""Build and run the unelevated broker transport fixture."""
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
    source = ROOT / "src/uacbroker/uacbroker_transport_tests.cpp"
    with tempfile.TemporaryDirectory(prefix="uac-transport-") as temp:
        exe = Path(temp) / "test.exe"
        build = run([
            compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
            *flags, str(source), "/Fe:" + str(exe), "/link", "/MANIFEST:EMBED",
            "advapi32.lib", "ole32.lib", "shell32.lib", "uuid.lib",
        ], cwd=temp, env=env, timeout=35)
        if build.returncode:
            return build.returncode
        return run([str(exe)], cwd=temp, env=env, timeout=15).returncode


if __name__ == "__main__":
    raise SystemExit(main())
