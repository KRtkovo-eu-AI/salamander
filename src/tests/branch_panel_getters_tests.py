# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual panel path getters with isolated panel storage; no UI or disk mutation."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
DEADLINE = time.monotonic() + 95


def run(command, **kwargs):
    timeout = min(kwargs.pop("timeout", 60), max(1, DEADLINE - time.monotonic()))
    return subprocess.run(command, timeout=timeout, **kwargs)


def compiler_environment():
    environment = os.environ.copy()
    if shutil.which("cl", path=environment.get("PATH")):
        return environment
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / (
        "Microsoft Visual Studio/Installer/vswhere.exe")
    result = run([str(vswhere), "-latest", "-products", "*", "-requires",
                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property",
                  "installationPath"], check=True, stdout=subprocess.PIPE, timeout=10)
    installation = result.stdout.decode("utf-8-sig").strip()
    if not installation:
        raise RuntimeError("MSVC C++ tools were not found")
    setup = Path(installation) / "Common7/Tools/VsDevCmd.bat"
    command = f'cmd /d /s /c "call "{setup}" -arch=x64 -host_arch=x64 >nul && set"'
    result = run(command, check=True, stdout=subprocess.PIPE, timeout=15)
    for line in result.stdout.decode(errors="replace").splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            environment[key.upper()] = value
    return environment


def read(path, revision=None):
    if revision:
        return run(["git", "show", f"{revision}:{path}"], cwd=ROOT, check=True,
                   stdout=subprocess.PIPE, timeout=10).stdout.decode("utf-8-sig")
    return (ROOT / path).read_text(encoding="utf-8-sig")


def block(source, marker, trailing=""):
    start = source.index(marker)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + trailing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="test production getters from an earlier git revision")
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    production = read("src/branch_view.cpp", args.source_ref)
    header = read("src/branch_view.h", args.source_ref)
    conversions = read("src/common/widepath.cpp", args.source_ref)
    builder = read("src/fileswn6.cpp", args.source_ref)
    first = builder.index("            std::string branchName;", builder.index("BOOL CFilesWindow::BuildScriptMain("))
    last = builder.index("            i++;", first)
    relative = builder.index("                    std::string branchTarget;", first)
    relative_end = builder.index("                    if (!BuildScriptFile(", relative)
    shell = read("src/shellsup.cpp", args.source_ref)
    properties = shell[shell.index("    case saProperties:"):]
    properties_begin = properties.index("                IContextMenu2* menu = CreatePanelShellMenu")
    properties_end = properties.index("                if (menu != NULL)", properties_begin)
    gate_begin = shell.index("    // S27d3a: selected Properties")
    gate_end = shell.index("    CALL_STACK_MESSAGE5", gate_begin)
    pieces = {
        "CUT_MARK": block(shell[shell.index("    case saCopyToClipboard:"):], "                    if (action != saCopyToClipboard)"),
        "CUT_CLEAR": block(read("src/fileswnb.cpp",args.source_ref),"void CFilesWindow::ClearCutToClipFlag("),
        "CLIPBOARD_EXPORT": block(shell[shell.index("    case saCopyToClipboard:"):], "                if (panel->Is(ptDisk))"),
        "SHELL_PATHS": block(shell,"static std::vector<std::wstring> GetPanelShellPaths("),
        "SHELL_DIRECTORY": block(shell,"static std::wstring GetPanelShellDirectoryW("),
        "PROPERTIES_DISPATCH": properties[properties_begin:properties_end],
        "SHELL_GATE": shell[gate_begin:gate_end],
        "COPY_RELATIVE": builder[relative:relative_end],
        "COPY_LABELS": block(builder, "                    for (std::vector<int>::reverse_iterator directory = branchCreatedDirectories.rbegin();"),
        "CONVERSION_UTF8_ACP": block(builder,"std::wstring SalMultiByteToWidePathUtf8OrAcp("),
        "COPY_PREP": builder[first:last],
        "RECYCLE_HEADER": str(ROOT / "src/operationrecycle.h").replace(chr(92), "/"),
        "RECYCLE_QUEUE": block(block(read("src/fileswn8.cpp", args.source_ref),"BOOL CFilesWindow::DeleteThroughRecycleBin("), "    if (Is(ptDisk))"),
        "NOTIFY": block(block(read("src/fileswn7.cpp", args.source_ref), "void CFilesWindow::AcceptChangeOnPathNotification("),"    if (IsBranchView())"),
        "FIND_BASENAME": block(conversions, "const wchar_t* SalPathFindFileNameW("),
        "PROGRAM_HEADER": str(ROOT / "src/branchprogram.h").replace(chr(92), "/"),
        "FILEACTION_HEADER": str(ROOT / "src/fileactionpath.h").replace(chr(92), "/"),
        "EXECUTE": block(block(read("src/fileswn2.cpp", args.source_ref), "void CFilesWindow::Execute(int index)"), "            if (IsBranchView())"),
        "ARCHIVE_READ": block(read("src/fileswn2.cpp", args.source_ref), "static HANDLE OpenArchiveFileForRead("),
        "ENTRY": block(header, "struct Entry\n", ";"),
        "JOIN": block(production, "std::wstring Join("),
        "ENTRY_FULL_PATH": block(production, "std::wstring Entry::FullPath()"),
        "ORDINARY_JOIN": block(conversions, "BOOL SalPathAppendW("),
        "CONVERSION": block(conversions, "BOOL SalIsExtendedLengthPathW(") + "\n" + block(conversions, "std::wstring SalPathRemoveExtendedPrefixW(") + "\n" + block(conversions, "std::wstring SalPathAddExtendedPrefixW(") + "\n" + block(conversions, "std::string SalWideToMultiBytePath(") + "\n" + block(conversions, "BOOL IsValidPathUtf8Text(") + "\n" + block(conversions, "std::wstring SalMultiByteToWidePath("),
        "TEMP_HEADER": str(ROOT / "src/panel_temporary_selection.h").replace(chr(92), "/"),
        "RENAME_METADATA": block(production, "void CFilesWindow::BranchItemRenamed("),
        "TEMP_SELECTION": block(read("src/fileswn0.cpp", args.source_ref), "void CFilesWindow::SelectFocusedItemAndGetName(") + "\n" + block(read("src/fileswn0.cpp", args.source_ref), "void CFilesWindow::UnselectItemWithName("),
        "NAVIGATION": block(production, "BOOL CFilesWindow::OpenBranchItemDirectory()"),
        "CACHE": block(production, "const char* CFilesWindow::GetItemCacheKeyPtr(") + "\n" + block(production, "const CFileData* CFilesWindow::GetBranchViewFileByCacheKey("),
        "VIEW_POLICY": block(read("src/fileswn5.cpp", args.source_ref).split("// S27a transition:", 1)[1], "    if (IsBranchView())"),
        "GETTERS": "\n".join(block(production, "std::wstring CFilesWindow::" + name + "(")
                            for name in ("GetItemDirectoryW", "GetItemFullPathW", "GetItemIdentityW", "GetItemRelativePathW")),
    }
    generated = read("src/tests/branch_panel_getters_tests.cpp.in")
    for name, value in pieces.items():
        generated = generated.replace("@@" + name + "@@", value)
    if "@@" in generated:
        raise AssertionError("unexpanded fixture placeholder")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="branch-panel-getters-") as directory:
        directory = Path(directory)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        executable = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual Branch panel getters:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(executable), "user32.lib"], [str(executable), str(directory)]):
                result = run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
