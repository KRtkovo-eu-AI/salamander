# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-06 | Compile actual Unload ownership through refusal and reload. | Independently count duplicate HMODULE load references and allocation-failure preservation. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
# | 2026-11-06 | Validate merged SDK prefixes and native shutdown ordering. | Keep provider modules mapped through refusal/retry while preserving native callback gates. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
"""Compile full actual Unload/UnloadAll and exact InitDLL acquisition fragment, with explicit host doubles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
DEADLINE = time.monotonic() + 95

def run(command, **kwargs):
    timeout = min(kwargs.pop("timeout", 45), max(1, DEADLINE - time.monotonic()))
    return subprocess.run(command, timeout=timeout, **kwargs)

def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")

def block(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]

def abi_check():
    def methods(text):
        text = re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)
        return {m.group(1): [re.sub(r"\s+", " ", x).strip()
                for x in re.findall(r"virtual\s+.*?=\s*0\s*;", block(text, m.group(0)), re.S)]
                for m in re.finditer(r"class\s+(\w+Abstract)\s*\{", text)}
    sdk = read("src/plugins/shared/spl_gen.h")
    current = methods(sdk)
    golden = json.loads(read("src/tests/disk_selection_snapshot_abi.json"))
    for name, old in golden["interfaces"].items():
        assert current.get(name, [])[:len(old)] == old, f"Released SDK prefix changed: {name}"
    assert hashlib.sha256(read("src/plugins/shared/spl_com.h").encode()).hexdigest() == golden["spl_com_sha256"], "CFileData/shared layout changed"
    for reference in ("053d7f6e75da5114e8037917d25f1bd4aa00a639", "304beb54b3a9463df483c48dac3d9ba405f4fc16"):
        available = run(["git", "rev-parse", "--verify", reference + "^{commit}"], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
        if available.returncode:
            continue
        previous = run(["git", "show", reference + ":src/plugins/shared/spl_gen.h"], cwd=ROOT, stdout=subprocess.PIPE, check=True, timeout=10).stdout.decode("utf-8-sig")
        for name, old in methods(previous).items():
            assert current.get(name, [])[:len(old)] == old, f"Integration SDK prefix changed: {reference}:{name}"
    general = current["CSalamanderGeneralAbstract"]
    assert "GetSalamanderForOperations(HWND panel)" in general[-1]
    concrete = block(read("src/plugins.h"), "class CSalamanderGeneral :")
    names = lambda text: re.findall(r"\bWINAPI\s+(\w+)\s*\(", text)
    concrete = re.sub(r"//[^\n]*|/\*.*?\*/", "", concrete, flags=re.S)
    concrete_virtuals = "\n".join(re.findall(r"virtual\s+.*?;", concrete, re.S))
    # Existing overrides inherit their SDK slots; the historical concrete header
    # declares GetMouseWheelScrollChars earlier. Compare each exact declaration
    # sequence with its own baseline rather than reordering legacy overrides.
    concrete_declarations = [re.sub(r"\s+", " ", x).strip()
                             for x in re.findall(r"virtual\s+.*?;", concrete, re.S)]
    for reference in ("a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449", "053d7f6e75da5114e8037917d25f1bd4aa00a639", "304beb54b3a9463df483c48dac3d9ba405f4fc16"):
        available = run(["git", "rev-parse", "--verify", reference + "^{commit}"], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
        if available.returncode:
            continue
        baseline = run(["git", "show", reference + ":src/plugins.h"], cwd=ROOT, stdout=subprocess.PIPE, check=True, timeout=10).stdout.decode("utf-8-sig")
        previous = re.sub(r"//[^\n]*|/\*.*?\*/", "", block(baseline, "class CSalamanderGeneral :"), flags=re.S)
        previous_declarations = [re.sub(r"\s+", " ", x).strip()
                                 for x in re.findall(r"virtual\s+.*?;", previous, re.S)]
        assert concrete_declarations[:len(previous_declarations)] == previous_declarations, f"Concrete virtual declaration prefix changed: {reference}"
    assert names(general[-1]) == names(concrete_declarations[-1]) == ["GetSalamanderForOperations"], "New method is not appended in both declarations"
    assert all(name in sdk for name in ("SALAMANDER_SERVICE_VIEWER_ENUMERATION", "SALAMANDER_SERVICE_PANEL_ITEM_PATHS", "SALAMANDER_SERVICE_DISK_SELECTION"))
    dispatch = block(read("src/zip.cpp"), "BOOL CSalamanderGeneral::QueryService")
    assert all(name in dispatch for name in ("GetViewerEnumerationService", "GetPanelItemPathsService", "GetDiskSelectionService"))
    print(f"SDK ABI: {len(golden['interfaces'])} released interfaces; {len(general)} general methods; CFileData unchanged", flush=True)

def integration_contract():
    source = read("src/plugins1.cpp")
    unload = block(source, "BOOL CPluginData::Unload(")
    assert "BOOL freeLibrary" in unload
    assert unload.index("PrepareNativeProcessExit(TRUE)") < unload.index("DeleteManager.PluginMayBeUnloaded") < unload.index("SalamanderGeneral.Clear()") < unload.index("DeferredModules.push_back(DLL)")
    assert unload.index("DeferredModules.reserve(") < unload.index("PrepareNativeProcessExit(TRUE)")
    failure = block(unload, "if (!PrepareNativeProcessExit(TRUE))")
    assert "return FALSE;" in failure and "FreeLibrary" not in failure and "DLL =" not in failure
    bridge = read("src/plugins/samandarin/managed_bridge.cpp")
    assert "gRuntimeHost->Stop()" not in bridge
    assert "gAssemblyPath" not in bridge and "gCurrentVersion" not in bridge and "QueryPluginDirectory" not in bridge
    assert "GetSamandarinManagedAssemblyPath(DLLInstance)" in bridge
    assert "runtimeInfo->IsLoadable(&loadable)" in bridge and "ShowClrStartError(parent, hr)" in bridge
    finalize = block(bridge, 'extern "C" __declspec(dllexport) BOOL __stdcall Samandarin_PrepareForProcessExit')
    assert finalize.index("WaitForSamandarinInitialization") < finalize.index("gNativeCallbacks.TryClose()") < finalize.index("gRuntimeHost->Release()") < finalize.index("HANDLES_FINALIZE_FOR_PROCESS_EXIT()")
    assert 'ExecuteCommand(L"Shutdown"' not in finalize
    assert "if (!finalize)" in finalize and "return TRUE;" in block(finalize, "if (!finalize)")
    print("Managed/native integration contracts: PASS", flush=True)

def compiler_environment():
    env = os.environ.copy()
    if shutil.which("cl", path=env.get("PATH")):
        return env
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    installation = run([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], check=True, stdout=subprocess.PIPE, timeout=10).stdout.decode("utf-8-sig").strip()
    assert installation, "MSVC tools unavailable"
    setup = Path(installation) / "Common7/Tools/VsDevCmd.bat"
    result = run(f'cmd /d /s /c "call "{setup}" -arch=x64 -host_arch=x64 >nul && set"', check=True, stdout=subprocess.PIPE, timeout=30)
    for line in result.stdout.decode(errors="replace").splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            env[key.upper()] = value
    return env

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"))
    args = parser.parse_args()
    abi_check()
    integration_contract()
    source = read("src/tests/plugin_shutdown_integration_tests.cpp.in")
    production = read("src/plugins1.cpp")
    initialize = block(production, "BOOL CPluginData::InitDLL(")
    begin = initialize.index("    if (DLL == NULL && IsGood())")
    end = initialize.index("        if (DLL == NULL) // error", begin)
    # This is the actual admission/path/load fragment, not a reimplementation.
    # ABI negotiation after acquisition and unrelated architecture UI are out of scope.
    acquisition = initialize[begin:end]
    assert "DLL = HANDLES(LoadLibrary(s));" in acquisition
    load_phase = ("BOOL CPluginData::InitDLLLoadPhase(BOOL waitCursor)\n{\n" + acquisition
                  + "        (void)refreshUNCRootPaths;\n"
                  + "        if (DLL != NULL) PluginIface.Present = TRUE;\n"
                  + "    }\n    return DLL != NULL;\n}")
    replacements = {
        "PREPARE": block(read("src/plugins1.cpp"), "BOOL CPluginData::PrepareNativeProcessExit("),
        "FREE": block(read("src/plugins1.cpp"), "void CPluginData::FreeDeferredModule("),
        "UNLOAD_ALL": block(read("src/plugins2.cpp"), "BOOL CPlugins::UnloadAll("),
        "UNLOAD": block(production, "BOOL CPluginData::Unload("),
        "INIT_LOAD_PHASE": load_phase,
    }
    for name, text in replacements.items():
        source = source.replace("@@" + name + "@@", text)
    assert "@@" not in source
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="plugin-shutdown-integration-") as temp:
        directory = Path(temp)
        generated = directory / "shutdown.cpp"
        generated.write_text(source, encoding="utf-8")
        for config, flags in (("Debug", ["/MDd", "/D_DEBUG", "/Od"]), ("Release", ["/MD", "/DNDEBUG", "/O2"])):
            if args.configuration and args.configuration != config:
                continue
            print("Native shutdown integration:", config, flush=True)
            executable = directory / "shutdown.exe"
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/D_CRT_SECURE_NO_WARNINGS", *flags, str(generated), "/Fe:" + str(executable), "user32.lib"], [str(executable)]):
                result = run(command, cwd=directory, env=env)
                if result.returncode:
                    return result.returncode
            native = directory / "native-shutdown.exe"
            result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags, str(ROOT / "src/tests/samandarin_native_shutdown_tests.cpp"), "/Fe:" + str(native), "user32.lib"], cwd=directory, env=env)
            if result.returncode:
                return result.returncode
            unicode_directory = directory / ("Unicode-" + "\u65e5\u010d\U0001f600" * 35)
            unicode_directory.mkdir()
            unicode_executable = unicode_directory / "native-shutdown.exe"
            shutil.copy2(native, unicode_executable)
            result = run([str(unicode_executable)], cwd=directory, env=env, timeout=15)
            if result.returncode:
                return result.returncode
            long_directory = Path("\\\\?\\" + str(directory / ("Long-" + "\u65e5" * 90) / ("Unicode-" + "\u010d" * 90)))
            long_directory.mkdir(parents=True)
            long_executable = long_directory / "native-shutdown.exe"
            assert len(str(long_executable)) > 260
            shutil.copy2(native, long_executable)
            try:
                result = run([str(long_executable)], executable=str(long_executable), cwd=directory, env=env, timeout=15)
                if result.returncode:
                    return result.returncode
            finally:
                # Extended-path cleanup stays within this test's freshly allocated root.
                assert str(long_directory).startswith("\\\\?\\" + str(directory) + "\\")
                shutil.rmtree(long_directory.parent)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
