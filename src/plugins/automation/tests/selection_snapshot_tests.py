# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual Automation/Salamatrix immutable selection consumers and all five worker forwarding contracts."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[4]
DEADLINE = time.monotonic() + 90

# Bounded MSVC discovery/build follows the other actual-function native fixtures.
def run(command, **kwargs):
    timeout = min(kwargs.pop("timeout", 45), max(1, DEADLINE - time.monotonic()))
    check = kwargs.pop("check", False)
    process = subprocess.Popen(command, **kwargs)
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
        process.kill()
        process.communicate(timeout=5)
        raise
    result = subprocess.CompletedProcess(command, process.returncode, stdout, stderr)
    if check:
        result.check_returncode()
    return result


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
    result = run(command, check=True, stdout=subprocess.PIPE, timeout=30)
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
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    parser.add_argument("--scope", choices=("Automation", "All"), default="Automation", help="All requires deferred S11 Salamatrix consumers")
    args = parser.parse_args()
    sdk = read("src/plugins/shared/spl_gen.h")
    facade = read("src/plugins/shared/spl_diskselection.h")
    item = read("src/plugins/automation/itemaut.cpp")
    collection = read("src/plugins/automation/itemcoll.cpp")
    pieces = {
        "SDK_ITEM": block(sdk, "struct CSalamanderDiskSelectionItem\n", ";"),
        "CONVERSIONS": "\n".join(block(facade, marker) for marker in (
            "inline std::wstring WideFromPath(", "inline std::string Utf8FromWide(")),
        "ITEM_CLASS": block(read("src/plugins/automation/itemaut.h"), "class CSalamanderPanelItemAutomation :", ";"),
        "ITEM_METHODS": item.split("extern CSalamanderGeneralAbstract* SalamanderGeneral;", 1)[1],
        "SNAPSHOT_CLASS": block(read("src/plugins/automation/itemcoll.h"), "class CSalamanderAutomationItemsSnapshot\n", ";"),
        "COLLECTION_CLASSES": block(read("src/plugins/automation/unkimpl.h"), "class CUnknownImpl :", ";") + "\n" +
            block(read("src/plugins/automation/itemcoll.h"), "class CSalamanderPanelItemCollection :", ";") + "\n" +
            block(read("src/plugins/automation/itemcoll.h"), "class CSalamanderPanelItemEnumerator :", ";"),
        "SNAPSHOT_METHODS": collection.split("extern CSalamanderGeneralAbstract* SalamanderGeneral;", 1)[1],
    }
    if args.scope == "All":
        # S11 dependencies remain deferred; Automation must not import them.
        sides = read("src/plugins/salamatrix/salamatrix_sides.h")
        packages = read("src/plugins/salamatrix/salamatrix_packages.cpp")
        pieces.update({
        "SIDE_CONSTANTS": "\n".join(re.findall(r"^#define SALAMATRIX_SIDE_ITEM_[^\n]+", sides, re.M)),
        "SIDE_TYPES": block(sides, "enum SideReference\n", ";") + "\n" + block(sides, "struct ItemInfo\n", ";"),
        "SIDE_METHODS": "\n".join(block(sides[sides.index("class SidesService :"):], marker) for marker in (
            "int ResolvePanel(", "static BOOL CopyDiskItemInfo(", "static BOOL CopyItemInfo(",
            "virtual BOOL WINAPI GetSelectedItem(", "virtual BOOL WINAPI GetFocusedItem(")),
        "JSON_METHODS": block(packages, "static std::string JsonEscape(") + "\n" + block(packages, "static std::string SideItemJson(const Sides::ItemInfo&") + "\n" + block(packages, "static std::string SideItemJson(const CSalamanderDiskSelectionItem&") + "\n" + block(packages, "static BOOL DiskSideContextJson("),
        })
    if args.scope == "All":
        # Workers are unchanged transport consumers of the same host-owned JSON.
        # Check every provider forwards context rather than synthesizing root + name.
        workers = {
            "JavaScript": "javascriptruntime/runtime/salamatrix_worker.mjs",
            "Python": "pythonruntime/runtime/salamatrix_worker.py",
            "PowerShell": "powershellruntime/runtime/salamatrix_worker.ps1",
            "PHP": "phpruntime/runtime/salamatrix_worker.php",
            "Lua": "luaruntime/runtime/salamatrix_worker.lua",
        }
        for language, path in workers.items():
            worker = read("src/plugins/" + path)
            assert "salamander.sides.context" in worker, language + " lacks shared context dispatch"
            assert re.search(r"(?:hostCall|host_call|Invoke-Host|client->call|_transport\.call)\([^\n]*[\"']salamander\.sides\.context[\"']|Invoke-Host\s+-Method\s+[\"']salamander\.sides\.context[\"']", worker), language + " no longer forwards context"
        print("Shared context forwarding: 5/5 provider contracts passed", flush=True)
    reset = block(collection, "HRESULT STDMETHODCALLTYPE CSalamanderPanelItemEnumerator::Reset(void)")
    assert "m_iItem = 0" in reset and "Capture" not in reset
    enum_constructor = block(collection, "CSalamanderPanelItemEnumerator::CSalamanderPanelItemEnumerator(")
    assert "m_snapshot = std::move(snapshot)" in enum_constructor
    if args.scope == "All":
        context = block(packages, '    if (method == "salamander.sides.context")')
        assert "DiskSideContextJson(SalamanderGeneral, panel, &context)" in context
        assert context.index("GetPanelPath(panel, NULL, 0") < context.index("owner->Sides->GetPath(")
        assert "CopyDiskItemInfo" not in block(packages, "static BOOL DiskSideContextJson(")
    generated = read("src/plugins/automation/tests/selection_snapshot_tests.cpp.in")
    if args.scope == "Automation":
        begin = generated.index("namespace Salamatrix { namespace Sides {")
        end = generated.index("static int Checks", begin)
        generated = generated[:begin] + generated[end:]
        for marker in ("static void TestSides()", "static void TestDynamicJsonContext()"):
            generated = generated.replace(block(generated, marker), "")
        generated = generated.replace("TestAutomation(); TestEnumeration(); TestSides(); TestDynamicJsonContext();", "TestAutomation(); TestEnumeration();")
        print("S11 DEFERRED: TestSides, TestDynamicJsonContext and five provider forwarding contracts", flush=True)
    for name, value in pieces.items():
        generated = generated.replace("@@" + name + "@@", value)
    if "@@" in generated:
        raise AssertionError("unexpanded consumer fixture marker")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="selection-consumers-") as directory:
        directory = Path(directory)
        source = directory / "test.cpp"
        executable = directory / "test.exe"
        source.write_text(generated, encoding="utf-8")
        for configuration in configurations:
            print("Actual immutable selection consumers:", configuration, flush=True)
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/wd4100", "/wd4505",
                             *flags, str(source), "/Fe:" + str(executable), "oleaut32.lib", "comsuppwd.lib" if configuration == "Debug" else "comsuppw.lib"], [str(executable)]):
                result = run(command, cwd=directory, env=environment)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
