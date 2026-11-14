# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Clean extended-length transport fixture paths. | Keep native test results independent of the temporary-directory path limit. | #731 | GPT-6.1 Sol |
# | 2026-11-09 | Support checked ARM64 UAC test builds. | Distinguish cross-compilation from native Windows runtime evidence. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Pass the application, broker and probe to the resource check. | Verify the actual built images. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Run the optional installed-image protocol check. | Test real PE resources without starting either image. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Run the broker job dispatcher fixture. | Verify repeated NTFS requests on a real pipe in native CI. | | GPT-6.1 Sol (Codex) |
# | 2026-11-07 | Run the broker transport fixture. | Exercise both live pipe peer checks in native CI. | | GPT-6.1 Sol (Codex) |

"""Build and run the unelevated broker transport fixture."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, require_arm64_image, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--protocol-images", nargs=4, type=Path, metavar=("HOST", "BROKER", "PROBE", "STALE_BROKER"))
    parser.add_argument("--expect-arm64", action="store_true",
                        help="require the prepared MSVC environment to produce native ARM64 images")
    parser.add_argument("--build-only", action="store_true", help="compile and check the images without running them")
    args = parser.parse_args()
    if args.build_only and args.protocol_images:
        parser.error("--protocol-images requires executing the tests")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    flags = ("/MTd", "/Od", "/D_DEBUG") if args.configuration == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    sources = ("uacbroker_transport_tests", "uacbroker_job_tests")
    with tempfile.TemporaryDirectory(prefix="uac-transport-") as temp:
        try:
            for name in sources:
                source = ROOT / "src/uacbroker" / (name + ".cpp")
                exe = Path(temp) / (name + ".exe")
                build = run([
                    compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                    *flags, str(source), "/Fe:" + str(exe), "/link", "/MANIFEST:EMBED",
                    "advapi32.lib", "ole32.lib", "shell32.lib", "uuid.lib", "ntdll.lib",
                ], cwd=temp, env=env, timeout=35)
                if build.returncode:
                    return build.returncode
                if args.expect_arm64:
                    require_arm64_image(exe)
                if args.build_only:
                    continue
                result = run([str(exe)], cwd=temp, env=env, timeout=15)
                if result.returncode:
                    return result.returncode
                if name == sources[0] and args.protocol_images:
                    images = [str(path.resolve(strict=True)) for path in args.protocol_images]
                    result = run([str(exe), "--protocol-images", *images], cwd=temp, env=env, timeout=15)
                    if result.returncode:
                        return result.returncode
        finally:
            # The job fixture creates extended-length paths only below target.
            # Resolve and check that fixed child before recursive cleanup.
            root = Path(temp).resolve(strict=True)
            target = root / "target"
            if target.exists() and target.resolve(strict=True) == target:
                shutil.rmtree("\\\\?\\" + str(target))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
