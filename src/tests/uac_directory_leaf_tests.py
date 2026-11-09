# | Date | Summary | Reason | Ticket | Model |
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

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--high-integrity", action="store_true")
    mode.add_argument("--overwrite-probe", action="store_true")
    mode.add_argument("--move-source-probe", action="store_true")
    mode.add_argument("--efs-source", type=Path, help="existing encrypted lab file readable by the current account")
    parser.add_argument("--move-target-root", type=Path, help="writable fixture parent on a different fixed NTFS volume")
    args = parser.parse_args()
    if args.move_target_root and not args.move_source_probe:
        parser.error("--move-target-root requires --move-source-probe")
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
                *flags, str(source), "/Fe:" + str(exe), "/link", "ntdll.lib", "ole32.lib", "advapi32.lib", "user32.lib", "uuid.lib",
            ], cwd=folder, env=env, timeout=35)
            if build.returncode:
                return build.returncode
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
                    return run([*arguments, str(owned)], cwd=folder, env=env, timeout=30).returncode
            return run(arguments, cwd=folder, env=env, timeout=15).returncode
        finally:
            junction.rmdir()
            shutil.rmtree("\\\\?\\" + str(fixture))


if __name__ == "__main__":
    raise SystemExit(main())
