# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual Branch ANSI rename fallback and wide rename I/O."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    production = read("src/fileswn5.cpp")
    conversions = read("src/common/widepath.cpp")
    route = block(production, "void CFilesWindow::RenameFileInternal(")
    route = route[route.index("{")+1:route.index("    *tryAgain = TRUE;")]
    begin = block(production, "void CFilesWindow::QuickRenameBegin(")
    creation = begin[begin.index("    HWND hWnd = NULL;"):begin.index("    if (hWnd == NULL)\n    {\n        TRACE_E")]
    pieces = {
        "CONVERSIONS": "\n".join(block(conversions, marker) for marker in (
            "BOOL SalIsExtendedLengthPathW(", "std::wstring SalPathRemoveExtendedPrefixW(",
            "std::wstring SalPathAddExtendedPrefixW(", "BOOL IsValidPathUtf8Text(",
            "std::wstring SalMultiByteToWidePath(", "std::string SalWideToMultiBytePath(", "BOOL SalPathAppendW(")),
        "CONVERT_NAME": block(production, "    std::wstring MultiByteFileNameToWideBest("),
        "DIALOG_TEXT": block(production, "    std::string WidePathToDialogText("),
        "CREATE_EDIT": creation,
        "RENAME_WIDE": block(production, "void CFilesWindow::RenameFileInternalW("),
        "BRANCH_ROUTE": route,
        "QUICK_KEY": block(production, "BOOL CFilesWindow::HandeQuickRenameWindowKey("),
    }
    generated = read("src/tests/branch_rename_fallback_tests.cpp.in")
    for marker, value in pieces.items():
        generated = generated.replace("@@"+marker+"@@", value)
    if "@@" in generated:
        raise AssertionError("unexpanded production fixture")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="branch-rename-fallback-") as temp:
        directory = Path(temp)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        exe = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual Branch ANSI rename fallback:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/D_CRT_SECURE_NO_WARNINGS",
                             *flags, str(source), "/Fe:" + str(exe), "user32.lib"], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
