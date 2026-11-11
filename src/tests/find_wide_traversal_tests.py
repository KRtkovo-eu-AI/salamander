# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise typed protected Find outcomes. | Preserve wide traversal after reporting scope capacity. | #731 | GPT-6.1 Terra |
# | 2026-11-10 | Run the actual ordinary Find traversal against a long NTFS fixture. | Verify exact protected retries and preservation of readable results. | #731 | GPT-6.1 Sol |
"""Compile the actual ordinary Find traversal with controlled denial seams."""
import shutil
import tempfile
from pathlib import Path

from branch_panel_getters_tests import block, compiler_environment, read, run


def main():
    source = block(read("src/find.cpp"), "void SearchDirectory(")
    template = read("src/tests/find_wide_traversal_tests.cpp.in")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="find-wide-traversal-") as temp:
        build = Path(temp)
        root = build / "root"
        long_parent = root / ("\u6771" * 95) / ("\u5b57" * 95) / ("\u6771" * 95)
        paths = [
            root / "root.txt",
            long_parent / "ordinary" / "deep" / "same.txt",
            long_parent / "ordinary" / "denied-file.txt",
            long_parent / "sibling" / "same.txt",
            long_parent / "denied-directory" / "never-open.txt",
            long_parent / "unavailable-directory" / "never-open.txt",
            root / "later" / "same.txt",
        ]
        extended = Path("\\\\?\\" + str(root))
        try:
            for path in paths:
                destination = extended / path.relative_to(root)
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(b"fixture\n")
            cpp = build / "find_wide.cpp"
            cpp.write_text(template.replace("@@TRAVERSAL@@", source), encoding="utf-8")
            exe = build / "find_wide.exe"
            for configuration, flags in (("Debug", ["/MDd", "/D_DEBUG", "/Od"]), ("Release", ["/MD", "/DNDEBUG", "/O2"])):
                print("Ordinary Find traversal:", configuration, flush=True)
                result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                              str(cpp), "/Fe:" + str(exe)], cwd=build, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
                result = run([str(exe), str(root), str(long_parent)], cwd=build, env=env, timeout=15)
                if result.returncode:
                    return result.returncode
        finally:
            if extended.is_dir():
                shutil.rmtree(extended)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
