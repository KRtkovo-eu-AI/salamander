# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual CtrlPgDn/archive probe dispatch and real ZIP signature I/O."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    import re
    host = read("src/fileswn0.cpp")
    pack = read("src/pack1.cpp")
    conversions = read("src/common/widepath.cpp")
    pieces = {
        "ARC_PROBE_HEADER": (Path(__file__).resolve().parents[1]/"plugins/shared/arcprobe.h").as_posix(),
        "CONVERSIONS": "\n".join(block(conversions, m) for m in (
            "BOOL SalIsExtendedLengthPathW(", "std::wstring SalPathAddExtendedPrefixW(", "std::string SalWideToMultiBytePath(")),
        "UTF8_COPY": "\n".join(block(read("src/salamdr4.cpp"),m) for m in (
            "static int Utf8CharStart(", "static int Utf8PrevCharStart(", "static BOOL IsValidUtf8Text(", "void CopyStringTruncateUtf8(")),
        "UNLOAD_USED": block(block(read("src/fileswn1.cpp"), "BOOL CFilesWindow::CanUnloadPlugin("), "if (used)"),
        "GENERAL_PATH": block(read("src/fileswn1.cpp"),"BOOL CFilesWindowAncestor::GetGeneralPath(").replace("CFilesWindowAncestor::", "CFilesWindow::"),
        "ZIP_PROBE": block(read("src/plugins/zip/main.cpp"), "BOOL CPluginInterfaceForArchiver::CanOpenArchive(").replace("CPluginInterfaceForArchiver::", "ZipProbe::"),
        "CAN_OPEN_VERSION": re.search(r"#define SAL_SDK_VER_CANOPENARCHIVE (\d+)", read("src/plugins/shared/spl_vers.h")).group(1),
        "CAN_OPEN_WRAPPER": block(read("src/plugins1.cpp"), "BOOL CPluginData::CanOpenArchive("),
        "IS_SEVEN": block(pack,"static BOOL IsSevenZipArchiverPlugin("),
        "GET_PLUGIN": block(pack,"CPluginData* PackGetPluginForArchiveFile("),
        "PROBE_PLUGIN": block(pack,"CPluginData* PackProbeArchivePlugin("),
        "TRY_ENTER": block(host,"void CFilesWindow::TryEnterFileAsArchive("),
        "CTRL_PGDN": block(host,"void CFilesWindow::CtrlPageDnOrEnter("),
        "DIRECTORY_TEXT": block(read("src/fileswn1.cpp"),"void CFilesWindow::DirectoryLineSetText("),
    }
    generated = read("src/tests/branch_archive_entry_tests.cpp.in")
    for marker,value in pieces.items():generated=generated.replace("@@"+marker+"@@",value)
    if "@@" in generated:
        raise AssertionError("unexpanded production fixture")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="branch-archive-entry-") as temp:
        directory = Path(temp)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        exe = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual archive-entry route:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/D_CRT_SECURE_NO_WARNINGS",
                             *flags, str(source), "/Fe:" + str(exe), "user32.lib"], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
