# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-26 | Test separate working directory histories and retained recency. | Protect scope switches and shared history across restart. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
# | 2026-11-26 | Exercise the saved source icon color. | Validate that live and closed tabs resolve their assigned colors. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
# | 2026-11-26 | Exercise persistent working-directory sources. | Run the production metadata code against deterministic storage. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |

"""Compile working directory history code with deterministic registry storage."""

import argparse
from pathlib import Path
import re
import runpy
import shutil
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HELPERS = runpy.run_path(str(ROOT / "src/tests/disk_selection_snapshot_tests.py"))


def check_scope_persistence(config: str) -> None:
    save = r"DirHistory->SaveToRegistry\(actKey, CONFIG_WORKDIRSHISTORY_REG,\s*!Configuration\.SaveWorkDirs\);"
    if not re.search(r"if \(DirHistory != NULL\)\s*(?:\{\s*)?" + save, config):
        raise AssertionError("SaveConfig must save shared history in either scope when history saving is enabled")
    migration = re.search(
        r"if \(Configuration\.SaveWorkDirs\)\s*\{\s*"
        r"const BOOL (\w+) = DirHistory->LoadFromRegistry\(actKey, CONFIG_WORKDIRSHISTORY_REG\);\s*"
        r"(\w+) = !\1 && !UsingSharedWorkDirHistory\(\);",
        config,
    )
    if migration is None or not re.search(
        r"LoadDetachedTabConfig\(salamander\);\s*RestoringPanelPaths = FALSE;\s*"
        rf"if \({migration[2]}\)\s*RestoreLegacySharedDirHistoryFromPanels\(\);",
        config,
    ):
        raise AssertionError("LoadConfig must rebuild only missing legacy shared history after restoring the tabs")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()

    read = HELPERS["read"]
    block = HELPERS["block"]
    source = read("src/salamdr3.cpp")
    declarations = read("src/salamand.h")
    main_window = read("src/mainwnd4.cpp")
    check_scope_persistence(read("src/mainwnd2.cpp"))
    limit = next(line for line in source.splitlines() if "constexpr size_t WorkingDirectoryTabNameLimit =" in line)
    functions = (
        "static std::wstring ShortenWorkingDirectoryTabName(",
        "static CWorkingDirectorySource ResolveWorkingDirectorySource(",
        "static CWorkingDirectorySource LoadWorkingDirectorySource(",
        "static void SaveWorkingDirectorySource(",
        "void CPathHistory::RememberSourceTab(",
        "void CPathHistory::AddPathUnique(",
        "void CPathHistory::SaveToRegistry(",
        "BOOL CPathHistory::LoadFromRegistry(",
    )
    scope_functions = (
        "CPathHistory* CMainWindow::GetDirHistory(",
        "BOOL CMainWindow::UsingSharedWorkDirHistory(",
        "void CMainWindow::HandleWorkDirsHistoryScopeChange(",
        "void CMainWindow::DirHistoryAddPathUnique(",
    )
    replacements = {
        "SOURCE": block(declarations, "struct CWorkingDirectorySource", ";"),
        "PRODUCTION": "\n\n".join([limit, *(block(source, function) for function in functions),
                                  *(block(main_window, function) for function in scope_functions)]),
    }

    generated = read("src/tests/working_directory_history_source_tests.cpp.in")
    for name, contents in replacements.items():
        generated = generated.replace(f"@@{name}@@", contents)
    if "@@" in generated:
        raise AssertionError("Unexpanded marker in generated native test")

    environment = HELPERS["compiler_environment"]()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    if compiler is None:
        raise RuntimeError("MSVC C++ compiler was not found")

    configurations = {
        "Debug": ["/MDd", "/D_DEBUG", "/Od"],
        "Release": ["/MD", "/DNDEBUG", "/O2"],
    }
    with tempfile.TemporaryDirectory(prefix="working-directory-history-") as temp:
        directory = Path(temp)
        test_source = directory / "test.cpp"
        test_executable = directory / "test.exe"
        test_source.write_text(generated, encoding="utf-8")
        for configuration, flags in configurations.items():
            if args.configuration not in (configuration, "Both"):
                continue
            print(f"Working directory history native configuration: {configuration}", flush=True)
            commands = (
                [compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                 str(test_source), f"/Fe:{test_executable}", "ole32.lib", "user32.lib"],
                [str(test_executable)],
            )
            for command in commands:
                result = HELPERS["run"](command, cwd=directory, env=environment)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
