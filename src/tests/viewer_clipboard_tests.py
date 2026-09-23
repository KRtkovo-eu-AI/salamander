# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

"""Run production viewer clipboard code against an isolated in-memory clipboard."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]
DEADLINE = time.monotonic() + 105


def run(command, **kwargs):
    timeout = min(kwargs.pop("timeout", 60), max(1, DEADLINE - time.monotonic()))
    return subprocess.run(command, timeout=timeout, **kwargs)


def source(path, revision):
    if revision:
        result = run(["git", "show", f"{revision}:{path}"], cwd=ROOT,
                     check=True, stdout=subprocess.PIPE, timeout=10)
        return result.stdout.decode("utf-8-sig").replace("\r\n", "\n")
    return (ROOT / path).read_text(encoding="utf-8-sig")


def function(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end] + "\n"


def generate(output, revision):
    clipboard = source("src/salamdr4.cpp", revision)
    declarations = source("src/consts.h", revision)
    viewer = source("src/viewer2.cpp", revision)
    commands = source("src/viewer3.cpp", revision)
    codepages = source("src/codetbl_utils.cpp", revision)
    signature = declarations.index("BOOL CopyHTextToClipboard(")
    fragments = [declarations[signature:declarations.index(";", signature) + 1]]
    fragments += [function(clipboard, marker) for marker in (
        "DWORD AddUnicodeToClipboard(", "DWORD AddMultibyteToClipboard(",
        "BOOL CopyHTextToClipboardW(", "BOOL CopyHTextToClipboard(")]
    fragments += ["#define GetLocaleInfoW TestGetLocaleInfoW"]
    fragments += [function(codepages, marker) for marker in (
        "DWORD GetConversionAutoCodePage(", "DWORD GetSystemLocaleAnsiCodePage(",
        "DWORD GetEffectiveConversionCodePage(")]
    fragments += ["#undef GetLocaleInfoW"]
    fragments += [function(viewer, marker) for marker in (
        "void CViewerWindow::CodeCharacters(",
        "HGLOBAL\nCViewerWindow::GetSelectedText(",
        "HGLOBAL\nCViewerWindow::GetSelectedTextW(")]
    fragments += [function(source("src/viewer.cpp", revision), "BOOL CViewerWindow::DecodeTextRange(")]
    command = function(commands, "        case CM_COPYTOCLIP:")
    fragments += ["LRESULT CViewerWindow::CopyCommand()\n{\n" +
                  command[command.index("{") + 1:]]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        "// SPDX-FileCopyrightText: 2023 Open Salamander Authors\n"
        "// SPDX-License-Identifier: GPL-2.0-or-later\n"
        "// Generated verbatim from production functions, with the command body wrapped for testing.\n"
        + "\n".join(fragments), encoding="utf-8")


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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generate", type=Path, help="generate the production fragment for MSBuild")
    parser.add_argument("--baseline-ref", help="read production sources with git show instead of the working tree")
    parser.add_argument("--system-acp", action="store_true", help="exercise the system ACP instead of using the UTF-8 manifest")
    args = parser.parse_args()
    if args.generate:
        generate(args.generate, args.baseline_ref)
        return 0
    if os.name != "nt":
        raise RuntimeError("These clipboard encoding tests require Windows")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    if compiler is None:
        raise RuntimeError("MSVC compiler was not found after toolchain initialization")
    with tempfile.TemporaryDirectory(prefix="viewer-clipboard-tests-") as temporary:
        build = Path(temporary)
        generate(build / "viewer_clipboard_generated.h", args.baseline_ref)
        executable = build / "viewer_clipboard_tests.exe"
        run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
             str(ROOT / "src/tests/viewer_clipboard_tests.cpp"),
             str(ROOT / "src/common/unicode/ViewerBomText.cpp"), "/I" + str(build),
             "/Fe:" + str(executable), "user32.lib"], cwd=build, env=environment, check=True)
        if not args.system_acp:
            manifest_tool = shutil.which("mt", path=environment.get("PATH"))
            run([manifest_tool, "-nologo", "-manifest", str(ROOT / "tools/utfnames/utfnames.manifest"),
                 "-outputresource:" + str(executable) + ";#1"], cwd=build,
                env=environment, check=True, timeout=10)
        return run([str(executable)], cwd=ROOT, env=environment, timeout=20).returncode


if __name__ == "__main__":
    raise SystemExit(main())
