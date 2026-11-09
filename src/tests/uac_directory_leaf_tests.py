# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-09 | Support checked ARM64 UAC test builds. | Distinguish cross-compilation from native Windows runtime evidence. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Allow time for the fragmented NTFS fixture. | Keep the optional multi-range F6 probe bounded on slower disks. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Accept a separate NTFS fixture root for F6. | Exercise move publication and recovery without touching existing files. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Accept an existing EFS fixture in the native suite. | Avoid creating a certificate during automated testing. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Link COM interface identifiers for Find stream tests. | Exercise the host stream using the native suite. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Add an optional NTFS source-retirement probe. | Verify the boundary of a held source handle. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Add an optional NTFS overwrite probe. | Measure rename behavior on the Windows build under test. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Link Windows access-control APIs for the staged tree tests. | Exercise real NTFS integrity descriptors. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Clean up the owned fixture with extended paths. | Remove long Unicode files even after a native assertion fails. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Link GUID generation for the copy staging fixture. | Exercise the real NTFS copy executor. | | GPT-6.1 Sol (Codex) |
# | 2026-11-07 | Run the verified-parent mkdir experiment. | Exercise physical NTFS path swaps and junction rejection. | | GPT-6.1 Sol (Codex) |

"""Run the P3a verified-directory-handle fixture with real NTFS I/O."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, require_arm64_image, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--high-integrity", action="store_true")
    mode.add_argument("--overwrite-probe", action="store_true")
    mode.add_argument("--move-source-probe", action="store_true")
    mode.add_argument("--efs-source", type=Path, help="existing encrypted lab file readable by the current account")
    parser.add_argument("--move-target-root", type=Path, help="writable fixture parent on a different fixed NTFS volume")
    parser.add_argument("--expect-arm64", action="store_true",
                        help="require the prepared MSVC environment to produce native ARM64 images")
    parser.add_argument("--build-only", action="store_true", help="compile and check the images without running them")
    args = parser.parse_args()
    if args.move_target_root and not args.move_source_probe:
        parser.error("--move-target-root requires --move-source-probe")
    if args.build_only and (args.high_integrity or args.overwrite_probe or args.move_source_probe or args.efs_source):
        parser.error("test modes require executing the tests")
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
        junction_created = False
        try:
            build = run([
                compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                *flags, str(source), "/Fe:" + str(exe), "/link", "ntdll.lib", "ole32.lib", "advapi32.lib", "user32.lib", "uuid.lib",
            ], cwd=folder, env=env, timeout=35)
            if build.returncode:
                return build.returncode
            if args.expect_arm64:
                require_arm64_image(exe)
            if args.build_only:
                return 0
            junction_result = run([
                "powershell.exe", "-NoProfile", "-Command",
                "$f=$env:SAMANDARIN_P3A_FIXTURE; "
                "New-Item -ItemType Junction -Path (Join-Path $f 'junction') "
                "-Target (Join-Path $f 'destination') -ErrorAction Stop | Out-Null",
            ], cwd=folder, env=env, timeout=15)
            if junction_result.returncode:
                return junction_result.returncode
            junction_created = True

            arguments = [str(exe), str(fixture)]
            if args.high_integrity:
                arguments.append("--high-integrity")
            if args.overwrite_probe:
                arguments.append("--overwrite-probe")
            if args.move_source_probe:
                arguments.append("--move-source-probe")
            if args.efs_source:
                arguments.extend(("--efs-source", str(args.efs_source.resolve(strict=True))))
            if args.move_target_root:
                parent = args.move_target_root.resolve(strict=True)
                with tempfile.TemporaryDirectory(prefix="uac-move-target-", dir=parent) as target:
                    owned = Path(target).resolve(strict=True)
                    if owned.parent != parent:
                        raise ValueError("F6 target fixture is outside its requested parent")
                    return run([*arguments, str(owned)], cwd=folder, env=env, timeout=45).returncode
            return run(arguments, cwd=folder, env=env, timeout=45 if args.move_source_probe else 15).returncode
        finally:
            if junction_created:
                junction.rmdir()
            shutil.rmtree("\\\\?\\" + str(fixture))


if __name__ == "__main__":
    raise SystemExit(main())
