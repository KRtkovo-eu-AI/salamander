#!/usr/bin/env python3
"""Fast source-level regression checks for Salamatrix integration contracts."""

from pathlib import Path
import json
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[4]


def read(relative: str) -> str:
    path = ROOT / relative
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise AssertionError(f"cannot read {relative}: {exc}") from exc


def require(text: str, pattern: str, message: str) -> None:
    if not re.search(pattern, text, re.MULTILINE | re.DOTALL):
        raise AssertionError(message)


def require_absent(text: str, pattern: str, message: str) -> None:
    if re.search(pattern, text, re.MULTILINE | re.DOTALL):
        raise AssertionError(message)


def main() -> int:
    dialogs = read("src/dialogs5.cpp")
    dialog_resources = read("src/lang/lang.rc")
    configuration_header = read("src/cfgdlg.h")
    configuration_defaults = read("src/dialogs4.cpp")
    texts = read("src/lang/texts.rc2")
    ai_header = read("src/plugins/salamatrixai/salamatrixai.h")
    ai_contract = read("src/plugins/salamatrix/salamatrix_ai.h")
    ui_contract = read("src/salamatrix-sdk/native-ui-runtime/salamatrix_ui.h")
    ui_implementation = read("src/salamatrix-sdk/native-ui-runtime/salamatrix_ui.cpp")
    ui_controls = read("src/salamatrix-sdk/native-ui-runtime/salamatrix_ui_controls.cpp")
    ui_salamander_host = read(
        "src/plugins/salamatrix/salamatrix_ui_salamander_host.cpp")
    salamatrix_project = read("src/plugins/salamatrix/vcxproj/salamatrix.vcxproj")
    studio_host_project = read(
        "src/tools/salamatrix-studio/preview-host/SalamatrixStudio.Host.vcxproj")
    studio_host_build = read("src/tools/salamatrix-studio/build-host.mjs")
    studio_host = read("src/tools/salamatrix-studio/preview-host/main.cpp")
    studio_package = json.loads(read("src/tools/salamatrix-studio/package.json"))
    studio_package_lock = json.loads(
        read("src/tools/salamatrix-studio/package-lock.json"))
    studio_extension = read("src/tools/salamatrix-studio/src/extension.ts")
    studio_explorer = read("src/tools/salamatrix-studio/src/projectExplorer.ts")
    studio_manifest_editor = read(
        "src/tools/salamatrix-studio/src/manifestEditor.ts")
    studio_menu_model = read("src/tools/salamatrix-studio/src/menuModel.ts")
    studio_scaffold = read("src/tools/salamatrix-studio/src/extensionScaffold.ts")
    populate_build_dir = read("src/vcxproj/!populate_build_dir.cmd")
    setup_x64_inf = read("tools/setup_x64.inf")
    inno_setup_x64 = read(
        "doc/runbook-setup/inno_setup_salamander_x64.iss")
    codesign = read("tools/codesign/codesign_certum.ps1")
    ai = read("src/plugins/salamatrixai/salamatrixai.cpp")
    bundled = read("src/plugins/salamatrixai/bundledprovider.cpp")
    local_llama = read("src/plugins/salamatrixailocalllama/local_llama.cpp")
    local_llama_header = read("src/plugins/salamatrixailocalllama/local_llama.h")
    local_llama_project = read("src/plugins/salamatrixailocalllama/vcxproj/local_llama.vcxproj")
    local_llama_installer = read("src/plugins/salamatrixailocalllama/runtime/install_llama.ps1")
    local_llama_rc2 = read("src/plugins/salamatrixailocalllama/local_llama.rc2")
    native_test_runner = read("tools/run_native_tests.ps1")
    pr_tests_workflow = read(".github/workflows/pr-tests.yml")
    pr_test_report_workflow = read(".github/workflows/pr-test-report.yml")
    pr_msbuild_workflow = read(".github/workflows/pr-msbuild.yml")
    hardware_wrapper_project = read(
        "src/extensions/hardware-monitor/hardview-lib/HardwareWrapper/HardwareWrapper.vcxproj")
    runtime_protocol = read("src/plugins/salamatrix/salamatrix_runtime_protocol.h")
    ai_rc2 = read("src/plugins/salamatrixai/salamatrixai.rc2")
    automation_header = read("src/plugins/automation/automationplug.h")
    automation = read("src/plugins/automation/automationplug.cpp")
    automation_bridge = read("src/plugins/automation/salamatrixbridge.cpp")
    automation_salamatrix = read("src/plugins/automation/salamatrixaut.cpp")
    automation_version = read("src/plugins/automation/versinfo.rh2")
    automation_entry = read("src/plugins/automation/entry.cpp")
    automation_scriptlist = read("src/plugins/automation/scriptlist.cpp")
    plugins_header = read("src/plugins.h")
    plugins1 = read("src/plugins1.cpp")
    plugins2 = read("src/plugins2.cpp")
    filesbx2 = read("src/filesbx2.cpp")
    filesmap = read("src/filesmap.cpp")
    fileswn3 = read("src/fileswn3.cpp")
    fileswn4 = read("src/fileswn4.cpp")
    fileswn5 = read("src/fileswn5.cpp")
    fileswn0 = read("src/fileswn0.cpp")
    fileswn2 = read("src/fileswn2.cpp")
    fileswnb = read("src/fileswnb.cpp")
    fs_contract = read("src/plugins/shared/spl_fs.h")
    viewer_configuration = read("src/salamdr2.cpp")
    mainwnd1 = read("src/mainwnd1.cpp")
    mainwnd2 = read("src/mainwnd2.cpp")
    mainwnd3 = read("src/mainwnd3.cpp")
    shutdown_translations = "\n".join(
        read(f"translations/{language}/salamand.slt")
        for language in (
            "chinesesimplified", "czech", "dutch", "french", "german",
            "hungarian", "romanian", "russian", "slovak", "spanish"))
    toolbar4 = read("src/toolbar4.cpp")
    toolbar8 = read("src/toolbar8.cpp")
    main_menu = read("src/menu4.cpp")
    samandarin_entry = read("src/plugins/samandarin/managed/EntryPoint.cs")
    samandarin_managed_project = read(
        "src/plugins/samandarin/managed/Samandarin.Managed.csproj")
    javascriptruntime = read("src/plugins/javascriptruntime/javascriptruntime.cpp")
    pythonruntime = read("src/plugins/pythonruntime/pythonruntime.cpp")
    python_discovery = read(
        "src/plugins/pythonruntime/python_executable_discovery.h")
    powershellruntime = read("src/plugins/powershellruntime/powershellruntime.cpp")
    phpruntime = read("src/plugins/phpruntime/phpruntime.cpp")
    luaruntime = read("src/plugins/luaruntime/luaruntime.cpp")
    javascriptruntime_rc = read("src/plugins/javascriptruntime/javascriptruntime.rc")
    pythonruntime_rc = read("src/plugins/pythonruntime/pythonruntime.rc")
    powershellruntime_rc = read("src/plugins/powershellruntime/powershellruntime.rc")
    phpruntime_rc = read("src/plugins/phpruntime/phpruntime.rc")
    luaruntime_rc = read("src/plugins/luaruntime/luaruntime.rc")
    runtime_configuration = read(
        "src/plugins/shared/runtime_configuration.h")
    runtime_provider_sources = (
        pythonruntime,
        powershellruntime,
        javascriptruntime,
        phpruntime,
        luaruntime,
    )
    salamatrix = read("src/plugins/salamatrix/salamatrix.cpp")
    native_viewer = read("src/plugins/shared/webviewviewer/native_viewer.cpp")
    plugin_darkmode_header = read("src/plugins/shared/plugindarkmode.h")
    plugin_darkmode = read("src/plugins/shared/plugindarkmode.cpp")
    webview2_targets = read(
        "src/plugins/shared/webviewviewer/WebView2.Native.targets")
    webview2_viewer_project = read(
        "src/plugins/webview2renderviewer/vcxproj/webview2renderviewer.vcxproj")
    text_viewer_project = read(
        "src/plugins/textviewer/vcxproj/textviewer.vcxproj")
    salamatrix_runtime = read("src/plugins/salamatrix/salamatrix_runtime.h")
    extensions_contract = read(
        "src/plugins/salamatrix/salamatrix_extensions.h")
    salamatrix_ui = ui_implementation
    salamatrix_props = read("src/plugins/salamatrix/vcxproj/salamatrix.props")
    viewer_darkmode_props = (
        salamatrix_props,
        read("src/plugins/textviewer/vcxproj/textviewer.props"),
        read("src/plugins/webview2renderviewer/vcxproj/webview2renderviewer.props"),
    )
    salamatrix_project = read(
        "src/plugins/salamatrix/vcxproj/salamatrix.vcxproj")
    salamatrix_version = read("src/plugins/salamatrix/versinfo.rh2")
    manifest = read("src/plugins/salamatrix/salamatrix_manifest.cpp")
    packages = read("src/plugins/salamatrix/salamatrix_packages.cpp")
    panel_tooltips = read("src/fileswn9.cpp")
    manifest = read("src/plugins/salamatrix/salamatrix_manifest.cpp")
    api_docs = read("src/plugins/salamatrix/salamatrix_api_docs.h")
    general_contract = read("src/plugins/shared/spl_gen.h")
    base_contract = read("src/plugins/shared/spl_base.h")
    general_impl = read("src/zip.cpp")
    setup = read("doc/runbook-setup/inno_setup_salamander_x64.iss")
    runtime_package_verifier = read("tools/verify_runtime_packages.ps1")
    javascript_demo = read("src/extensions/demos/javascript-node/main.mjs")
    javascript_demo_manifest = json.loads(
        read("src/extensions/demos/javascript-node/extension.json"))
    require(
        plugin_darkmode_header,
        r"PluginDarkMode_ApplyMenuBar\(HWND hwnd\).*?"
        r"PluginDarkMode_ApplyStatusBar\(HWND hwnd\)",
        "Plugin dark-mode facade does not expose menu/status bar theming")
    require(
        plugin_darkmode,
        r"setWindowMenuBarSubclass\(hwnd\).*?setStatusBarCtrlSubclass\(hwnd\)",
        "Plugin dark-mode facade does not use win32-darkmodelib for menu/status bars")
    require(
        native_viewer,
        r"PluginDarkMode_ApplyMenuBar\(window_\).*?"
        r"PluginDarkMode_ApplyStatusBar\(status_\).*?"
        r"PluginDarkMode_ApplyListTreeThemeRecursive\(window_\)",
        "Shared viewer frame does not theme its menu and status bars")
    require(
        native_viewer,
        r"PluginDarkMode_HandleCtlColor\(message, wParam, lParam, &colorResult\)",
        "Shared viewer frame does not dark-theme its status chrome controls")
    require(
        native_viewer,
        r"WM_NV_APPLY_ZOOM.*?case WM_NV_APPLY_ZOOM:\s*"
        r"ApplyZoomEdit\(\).*?"
        r"message\.wParam == VK_RETURN.*?"
        r"GetDlgCtrlID\(message\.hwnd\) == IDC_NV_ZOOM_EDIT.*?"
        r"SendMessageW\(viewerWindow, WM_NV_APPLY_ZOOM",
        "Enter does not apply a manually entered viewer zoom value")
    for viewer_props in viewer_darkmode_props:
        require(
            viewer_props,
            r"USE_DARKMODELIB=1;_DARKMODELIB_NO_INI_CONFIG",
            "A shared viewer-frame consumer compiles darkmodelib menu/status support as a no-op")
    for viewer_project in (salamatrix_project, text_viewer_project,
                           webview2_viewer_project):
        require(
            viewer_project,
            r"darkmode_backend_darkmodelib\.cpp.*?"
            r"third_party\\darkmodelib\\src\\Darkmodelib\.cpp.*?"
            r"third_party\\darkmodelib\\src\\DmlibSubclassControl\.cpp.*?"
            r"third_party\\darkmodelib\\src\\DmlibSubclassWindow\.cpp",
            "A shared viewer-frame consumer does not link the darkmodelib backend")
    require(
        native_viewer,
        r"LoadLibraryExW\(modulePath\.data\(\).*?"
        r"GetProcAddress\(loader, \"CreateCoreWebView2EnvironmentWithOptions\"\).*?"
        r"HRESULT_FROM_WIN32\(ERROR_MOD_NOT_FOUND\)",
        "The shared native viewer still makes WebView2Loader.dll a mandatory plugin import")
    require_absent(
        native_viewer,
        r"HRESULT hr = CreateCoreWebView2EnvironmentWithOptions\(",
        "The shared native viewer directly imports WebView2Loader.dll")
    require(
        native_viewer,
        r"GetModuleFileNameW\(nullptr, modulePath\.data\(\).*?"
        r'L"utils\\\\WebView2Loader\.dll"',
        "The shared native viewer does not resolve WebView2Loader under utils")
    require(
        webview2_targets,
        r"DestinationFolder=\"\$\(OutDir\)\.\.\\\.\.\\utils\".*?"
        r"Delete Files=\"\$\(OutDir\)WebView2Loader\.dll(?:;|\")",
        "WebView2Loader is not staged once under utils")
    require(
        inno_setup_x64,
        r'Source: "\{#PayloadDir\}\\utils\\WebView2Loader\.dll"; '
        r'DestDir: "\{app\}\\utils"; Flags: ignoreversion',
        "Setup does not install the shared WebView2Loader under utils")
    if len(re.findall(r'^Source: .*WebView2Loader\.dll', inno_setup_x64,
                      re.MULTILINE)) != 1:
        raise AssertionError(
            "Setup installs duplicate per-plugin WebView2Loader copies")
    require(
        native_viewer,
        r'ModuleDirectory\(nullptr\) \+ L"utils\\\\MarkdigRenderer\.exe"',
        "The native viewer does not resolve shared MarkdigRenderer under utils")
    require(
        salamatrix_project,
        r'--output &quot;\$\(IntDir\)MarkdigRenderer&quot;.*?'
        r'DestinationFolder="\$\(OutDir\)\.\.\\\.\.\\utils".*?'
        r'Delete Files="\$\(OutDir\)MarkdigRenderer\.exe',
        "Salamatrix does not stage the shared MarkdigRenderer under utils")
    require_absent(
        webview2_viewer_project,
        r'dotnet publish.*?MarkdigRenderer|MarkdigRenderer\.csproj',
        "webview2renderviewer still produces a private MarkdigRenderer copy")
    if len(re.findall(r'^Source: .*MarkdigRenderer\.exe', inno_setup_x64,
                      re.MULTILINE)) != 1:
        raise AssertionError(
            "Setup installs duplicate per-plugin MarkdigRenderer copies")
    require(
        fs_contract,
        r"#define FS_SERVICE_NO_REFRESH_WAIT_CURSOR 0x04000000",
        "Plugin FS contract does not expose opt-in refresh wait-cursor suppression")
    require(
        fileswn2,
        r"ChangePathToPluginFS.*?ShouldShowWaitCursorForRefresh\(\).*?IDC_WAIT",
        "Plugin FS path changes still force a redundant wait cursor")
    require(
        manifest,
        r"refreshIntervalMs\", 3000, 0, 60000",
        "Manifest validation still rejects refreshIntervalMs=0")
    require(
        packages,
        r"GetSupportedServices\(\).*?FS_SERVICE_NO_REFRESH_WAIT_CURSOR",
        "Salamatrix FS does not suppress the redundant refresh wait cursor")
    require(
        packages,
        r"needsPersistentWorker.*?!package->Manifest.EventsDeclared.*?"
        r"!package->Manifest.Events.empty\(\).*?ActivateExtension",
        "Salamatrix still starts every extension runtime during splash startup")
    if (sum(source.count("ShouldShowWaitCursorForRefresh()")
           for source in (fileswn0, fileswnb)) < 3):
        raise AssertionError(
            "Panel refresh wait cursor is not guarded in both refresh paths")
    runtime_process_sources = (
        automation_bridge,
        read("src/plugins/javascriptruntime/javascriptruntime.cpp"),
        read("src/plugins/pythonruntime/pythonruntime.cpp"),
        read("src/plugins/powershellruntime/powershellruntime.cpp"),
        read("src/plugins/phpruntime/phpruntime.cpp"),
        read("src/plugins/luaruntime/luaruntime.cpp"))
    if not all("STARTF_USESTDHANDLES | STARTF_FORCEOFFFEEDBACK" in source
               for source in runtime_process_sources):
        raise AssertionError(
            "A Salamatrix runtime can still trigger the system startup cursor")
    python_demo = read("src/extensions/demos/python/main.py")
    python_demo_manifest = json.loads(
        read("src/extensions/demos/python/extension.json"))
    lua_demo = read("src/extensions/demos/lua/main.lua")
    lua_demo_manifest = json.loads(
        read("src/extensions/demos/lua/extension.json"))
    powershell_demo = read("src/extensions/demos/powershell/main.ps1")
    powershell_demo_manifest = json.loads(
        read("src/extensions/demos/powershell/extension.json"))
    php_demo = read("src/extensions/demos/php/main.php")
    php_demo_manifest = json.loads(
        read("src/extensions/demos/php/extension.json"))
    javascript_worker = read(
        "src/plugins/javascriptruntime/runtime/salamatrix_worker.mjs")
    python_worker = read(
        "src/plugins/pythonruntime/runtime/salamatrix_worker.py")
    powershell_worker = read(
        "src/plugins/powershellruntime/runtime/salamatrix_worker.ps1")
    php_worker = read(
        "src/plugins/phpruntime/runtime/salamatrix_worker.php")
    lua_runtime = read("src/plugins/luaruntime/luaruntime.cpp")
    lua_worker = read(
        "src/plugins/luaruntime/runtime/salamatrix_worker.lua")
    navigator = read("src/extensions/git-worktree-navigator/main.ps1")
    navigator_manifest = json.loads(
        read("src/extensions/git-worktree-navigator/extension.json"))
    lock_inspector = read("src/extensions/file-lock-inspector/main.ps1")
    lock_inspector_manifest = json.loads(
        read("src/extensions/file-lock-inspector/extension.json"))
    process_explorer = read("src/extensions/process-explorer/main.ps1")
    process_explorer_manifest = json.loads(
        read("src/extensions/process-explorer/extension.json"))
    hardware_monitor = read("src/extensions/hardware-monitor/main.ps1")
    hardware_monitor_manifest = json.loads(
        read("src/extensions/hardware-monitor/extension.json"))
    event_viewer = read("src/extensions/event-viewer/main.ps1")
    event_viewer_manifest = json.loads(
        read("src/extensions/event-viewer/extension.json"))
    menu_builder = read("src/extensions/extension-menu-builder/main.ps1")
    menu_builder_manifest = json.loads(
        read("src/extensions/extension-menu-builder/extension.json"))
    bundled_one_shot_manifests = (
        javascript_demo_manifest, python_demo_manifest, lua_demo_manifest,
        powershell_demo_manifest, php_demo_manifest, navigator_manifest,
        lock_inspector_manifest, process_explorer_manifest,
        hardware_monitor_manifest, event_viewer_manifest,
        menu_builder_manifest)
    if any(item.get("events") != []
           for item in bundled_one_shot_manifests):
        raise AssertionError(
            "A bundled one-shot extension still starts a worker at startup")

    update_check = re.search(
        r"public static async Task CheckForUpdatesAsync\(.*?"
        r"(?=\n    public static void Shutdown\(\))",
        samandarin_entry,
        re.MULTILINE | re.DOTALL)
    if update_check is None or update_check.group(0).count(
            "CheckSemaphore.WaitAsync") != 1:
        raise AssertionError(
            "Samandarin update check must acquire its semaphore exactly once")
    require(
        samandarin_entry,
        r"private void AddImageListImage\(string key, Image source\).*?"
        r"var bitmap = CreateImageListBitmap\(source, _pluginImages\.ImageSize\);.*?"
        r"_pluginImageListSources\.Add\(key, bitmap\);.*?"
        r"_pluginImages\.Images\.Add\(key, bitmap\);.*?"
        r"private void ClearImageListImages\(\).*?"
        r"_pluginImages\.Images\.Clear\(\);.*?bitmap\.Dispose\(\);",
        "Samandarin Plugin Updates does not retain ImageList source bitmaps")
    require_absent(
        samandarin_entry,
        r"using var bitmap\s*=\s*CreateImageListBitmap",
        "Samandarin Plugin Updates disposes ImageList source bitmaps too early")
    salamander_solution = read("src/vcxproj/salamand.sln")
    samandarin_managed_guid = r"\{B2CAEA75-EAA8-4B2F-AF57-E187CDDFD710\}"
    for configuration in ("Debug", "Release", "Release clean"):
        require(
            salamander_solution,
            samandarin_managed_guid + rf"\.{configuration}\|x64\.ActiveCfg = {configuration}\|x64.*?" +
            samandarin_managed_guid + rf"\.{configuration}\|x64\.Build\.0 = {configuration}\|x64",
            f"Samandarin.Managed {configuration}|x64 solution mapping does not build the x64 output")
    samandarin_solution = read("src/plugins/samandarin/vcxproj/samandarin.sln")
    standalone_managed_guid = r"\{E9F70914-3011-4D4F-9E79-3A9540A5E0F3\}"
    for solution_configuration, project_configuration in (("Debug", "Debug"), ("Release", "Release"), ("Release clean", "Release clean"), ("SDK", "Release")):
        require(
            samandarin_solution,
            standalone_managed_guid + rf"\.{solution_configuration}\|x64\.ActiveCfg = {project_configuration}\|x64.*?" +
            standalone_managed_guid + rf"\.{solution_configuration}\|x64\.Build\.0 = {project_configuration}\|x64",
            f"Standalone Samandarin solution maps {solution_configuration}|x64 managed build away from x64")
    bootstrap_guid = r"\{0D11429A-8289-4BC2-B0CE-FCBD66A9F271\}"
    for solution_text, solution_name in ((salamander_solution, "main"), (samandarin_solution, "standalone Samandarin")):
        for configuration in ("Debug", "Release", "Release clean"):
            require(
                solution_text,
                bootstrap_guid + rf"\.{configuration}\|x64\.ActiveCfg = {configuration}\|x64.*?" +
                bootstrap_guid + rf"\.{configuration}\|x64\.Build\.0 = {configuration}\|x64",
                f"{solution_name} solution maps ManagedBootstrap {configuration}|x64 away from x64")
    require(
        samandarin_managed_project,
        r"CleanStagedSamandarinManagedAssembly.*?AfterTargets=\"Clean\".*?"
        r"Condition=\"'\$\(Platform\)' == 'x64'\".*?"
        r"Delete Files=\"\$\(SamandarinStagedPluginDir\)Samandarin\.Managed\.dll\"",
        "Cleaning Samandarin.Managed does not remove its staged plugin assembly")
    require(
        samandarin_entry + samandarin_managed_project,
        r"DefaultPluginImageResource = \"OpenSalamander\.Plugin\.png\".*?"
        r"Image\.FromStream\(stream\).*?"
        r"res\\plugin\.png.*?OpenSalamander\.Plugin\.png",
        "Samandarin Plugin Updates does not use src/res/plugin.png by default")
    require(
        samandarin_entry,
        r"private async Task RefreshAsync\(\).*?"
        r"BindRows\(\);.*?SetLoadingState\(false\);.*?"
        r"StartCatalogImageLoad\(_rows\);",
        "Samandarin Plugin Updates still blocks the initial list on catalog icons")
    require_absent(
        samandarin_entry,
        r"SetLoadingState\(false\);\s*"
        r"await EnsureCatalogImagesAsync",
        "Samandarin Plugin Updates still downloads icons serially before refresh completes")
    require(
        samandarin_entry,
        r"CatalogImageDownloadConcurrency\s*=\s*[2-9][0-9]*.*?"
        r"HashSet<string>\(StringComparer\.OrdinalIgnoreCase\).*?"
        r"SemaphoreSlim\(CatalogImageDownloadConcurrency\).*?"
        r"Task\.WhenAll\(downloads\).*?"
        r"SendAsync\(request, cancellationToken\).*?"
        r"MaxConnectionsPerServer\s*=\s*"
        r"PluginUpdatesDialog\.CatalogImageDownloadConcurrency",
        "Samandarin catalog icons are not deduplicated and loaded with bounded concurrency")

    require(dialogs, r"HasStablePluginKey\(p->RegKeyName, \"SALAMATRIX\"\).*?IsPluginName\(p->Name, \"Salamatrix Framework\"\)",
            "Salamatrix Framework key/name fallback is missing")
    require(
        salamatrix,
        r"SetBasicPluginData\(PluginNameEN,.*?"
        r"FUNCTION_AUTOMATIONFRAMEWORK\s*\|.*?"
        r"FUNCTION_DYNAMICMENUEXT\s*\|.*?"
        r"FUNCTION_LOADSAVECONFIGURATION",
        "Salamatrix does not advertise package configuration persistence")
    require(
        salamatrix,
        r"FUNCTION_FILESYSTEM.*?PluginNameShort,\s*NULL,\s*\"salamatrix\"\).*?"
        r"GetPluginFSName\(SalamatrixFSName,\s*0\)",
        "Salamatrix advertises a file system without the mandatory FS name")
    require(
        packages,
        r"ChangePanelPathToPluginFS\(panel,\s*SalamatrixFSName,",
        "Salamatrix file system does not use its host-assigned FS name")
    require(
        plugins1,
        r"supportFS\s*&&\s*fsName\s*==\s*NULL.*?Error\s*=\s*TRUE",
        "host no longer treats a missing advertised FS name as a fatal load error")
    require(
        salamatrix,
        r"CPluginInterface::LoadConfiguration.*?"
        r"SalamatrixPackages->LoadConfiguration\(regKey, registry\).*?"
        r"CPluginInterface::SaveConfiguration.*?"
        r"SalamatrixPackages->SaveConfiguration\(regKey, registry\)",
        "Salamatrix does not forward host configuration callbacks to packages")
    require(
        packages,
        r'StringListLoader::Load\(\s*key, "ExtensionOrder".*?'
        r'StringListLoader::Load\(\s*key, "RemovedExtensions".*?'
        r'StringListLoader::Load\(\s*key, "ExtensionManifests".*?'
        r'StringListSaver::Save\(\s*key, "ExtensionOrder".*?'
        r'StringListSaver::Save\(\s*key, "RemovedExtensions".*?'
        r'StringListSaver::Save\(\s*key, "ExtensionManifests"',
        "Salamatrix does not persist extension order, removals and custom manifests")
    for key, name in (
        ("JAVASCRIPT.RUNTIME", "JavaScript Runtime"),
        ("LUA.RUNTIME", "Lua Runtime"),
        ("PHP.RUNTIME", "PHP Runtime"),
        ("POWERSHELL.RUNTIME", "PowerShell Runtime"),
        ("PYTHON.RUNTIME", "Python Runtime"),
    ):
        require(dialogs, rf"HasStablePluginKey\(p->RegKeyName, \"{re.escape(key)}\"\)",
                f"runtime key fallback is missing: {key}")
        require(dialogs, rf"IsPluginName\(p->Name, \"{re.escape(name)}\"\)",
                f"runtime display-name fallback is missing: {name}")
    require(dialogs, r"HasStablePluginKey\(p->RegKeyName, \"SALAMATRIX\.AI\"\).*?IsPluginName\(p->Name, \"Salamatrix AI\"\)",
            "Salamatrix AI key/name fallback is missing")
    require(dialogs, r"if \(supportAutomationFramework\).*?isExtensionHelper.*?IDS_PLUGINFUNCEXTENSIONHELPER",
            "Functions mapping does not include the helper label")
    require(dialogs, r"if \(p->MenuItems\.Count > 0 \|\| p->SupportDynMenuExt\)",
            "AI Menu Extension is still hidden in Functions")
    require(texts, r'IDS_PLUGINFUNCEXTENSIONHELPER\s*,\s*"Extension Helper Tool"',
            "helper label is not exactly Extension Helper Tool")

    require(ai_header, r"enum\s*\{\s*CmdOpenAssistant\s*=\s*1\s*\}",
            "AI command id 1 is missing")
    require(ai, r"\bCmdOpenAssistant\b", "AI command symbol is missing")
    require(ai, r"ExecuteMenuItem.*?id == CmdOpenAssistant.*?ShowChat", "AI menu command does not open chat")
    require(ai_rc2, r'IDS_AI_ASSISTANT_MENU\s+1000', "AI menu resource id is missing")
    require(ai_header, r'#define IDI_PLUGINICON\s+1030', "AI plugin icon resource id is missing")
    require(ai_rc2, r'#define IDI_PLUGINICON\s+1030', "AI resource icon id is missing")
    require(ai_rc2, r'IDI_PLUGINICON\s+ICON\s+"[.][.]\\\\[.][.]\\\\res\\\\sal_r\.ico"', "AI menu icon is not sal_r.ico")
    require(ai_rc2, r'IDS_AI_ASSISTANT_MENU\s+"Ask Salamatrix AI\.\.\."', "AI menu resource text is not exact")
    require(ai, r'#include\s+"versinfo\.rh2"', "AI implementation does not include versinfo.rh2")
    require(ai_rc2, r'#include\s+"versinfo\.rh2"', "AI rc2 does not include versinfo.rh2")
    require(ai_rc2, r'#include\s+"versinfo\.rc2"', "AI rc2 does not include versinfo.rc2")
    require(ai, r'SetBasicPluginData\(\s*"Salamatrix AI",\s*FUNCTION_AUTOMATIONFRAMEWORK \| FUNCTION_DYNAMICMENUEXT,\s*VERSINFO_VERSION_NO_PLATFORM,\s*VERSINFO_COPYRIGHT,\s*VERSINFO_DESCRIPTION,\s*VERSINFO_INTERNAL,\s*NULL,\s*NULL\)',
            "AI SetBasicPluginData does not use versinfo macros")
    require(ai, r'SetPluginHomePageURL\("https://samandarin\.net/"\)', "AI homepage URL is not set")
    require(ai, r"SalamanderGeneral->LoadStr\(DLLInstance, IDS_AI_ASSISTANT_MENU", "AI menu caption does not use Salamander localization")
    require(ai, r'SalamanderPluginEntry\(.*?SalamanderGUI\s*=\s*salamander->GetSalamanderGUI\(\)', "AI plugin entry does not initialize SalamanderGUI")
    require(ai, r'caption\s*==\s*NULL.*\?\s*"Ask Salamatrix AI\.\.\."\s*:\s*caption',
            "AI menu caption fallback to Ask Salamatrix AI... is missing or unstable")
    require(ai, r'BuildMenu.*?salamander->AddMenuItem\(-1,\s*GetAssistantMenuCaption\(\),\s*0,\s*CmdOpenAssistant',
            "AI BuildMenu does not add fully specified assistant command")
    require(ai, r"BuildMenu.*?GetAssistantMenuCaption\(\).*?CmdOpenAssistant", "AI BuildMenu does not add the resource-backed command")
    require(ai, r"BuildMenu.*?MENU_SKILLLEVEL_ALL", "AI BuildMenu does not use MENU_SKILLLEVEL_ALL")
    require(ai, r"CPluginInterface::Connect.*?CreateIconList\(\).*?ReplaceIcon\(0.*?SetIconListForGUI.*?SetPluginIcon\(0\).*?SetPluginMenuAndToolbarIcon\(0\)", "AI plugin does not register sal_r.ico in the Plugin Manager icon list")
    require(ai, r"GetMenuItemState\(int id,\s*DWORD eventMask\).*?return .*?CmdOpenAssistant.*MENU_ITEM_STATE_ENABLED",
            "AI menu command is not always exposed as enabled")
    require(ai, r"IsCurrentService\(SALAMATRIX_SERVICE_AI.*?g_ai\).*?UnregisterProvider",
            "AI Release lacks current-service pointer validation")
    require(ai, r"IsInterfaceModuleLoaded.*?VirtualQuery.*?"
                r"GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS",
            "AI Release cannot detect an interface whose provider DLL was unloaded")
    require(ai, r"g_chat != NULL && IsInterfaceModuleLoaded\(g_chat->Dialog\).*?"
                r"g_chat->Dialog->Close",
            "AI Release can close a dialog through an unloaded UI service")
    require_absent(ai_header, r"class CLocalBundledAssistantProvider",
                   "bundled local provider must not be part of the mandatory AI helper")
    require(ai_contract, r"BuildRelevantApiDescription",
            "AI prompt API slicing helper is missing")
    require(ai_contract, r"AssistantCanImplement",
            "AI unsupported-capability response helper is missing")
    require(ai_contract, r"missingCapabilities",
            "AI contract does not describe missing capabilities")
    require(ai_contract, r'contextCall.*?selectedItems.*?javascriptNodeExample',
            "panel API slice does not explain how generated scripts obtain selected item paths")
    require(ai_contract, r'CopyValidationMessage\(response,\s*validation\)',
            "assistant service discards the concrete static-validation error")
    require(ai_contract, r'this\.selectedItems does not exist.*?'
                         r'MD5 processing of selected file paths is implementable',
            "assistant validator does not reject grounded-selection and MD5 semantic hallucinations")
    require(bundled, r'm_descriptor\.ProviderId\s*=\s*"local\.bundled"',
            "bundled local AI provider id is missing")
    require(bundled, r'SALAMATRIX_AI_BUNDLED_COMMAND.*?llama-cli\.exe',
            "bundled provider does not support the colocated llama.cpp executable")
    require(bundled, r'SALAMATRIX_AI_BUNDLED_MODEL.*?salamatrix\.gguf',
            "bundled provider does not support the colocated GGUF model")
    require(bundled, r'IsRegularFile\(m_command\).*?IsRegularFile\(m_model\)',
            "bundled provider availability does not require both runtime assets")
    require(bundled, r'CreateProcessW\(NULL, commandLine\.data\(\).*?TerminateProcess\(process\.hProcess, 1\)',
            "bundled provider does not isolate and bound the llama.cpp process")
    require(bundled, r'PeekNamedPipe\(parentOut, NULL, 0, NULL, &available, NULL\)',
            "bundled provider uses the complete PeekNamedPipe signature")
    require(bundled, r'ResolveBundledAsset.*?legacyRoots.*?salamatrixai',
            "bundled provider does not support the legacy companion asset layout")
    require(bundled, r'120000', "bundled provider timeout is not capped at two minutes")
    require(bundled, r'CreateUtf8PromptFile', "bundled provider does not pass the prompt through a UTF-8 file")
    require(bundled, r'EscapeQwenChatControlTokens.*?'
                     r'<\|im_start\|>system.*?<\|im_start\|>user.*?'
                     r'<\|im_start\|>assistant',
            "bundled provider does not safely render the Qwen chat template")
    require(bundled, r'-f.*--json-schema-file.*'
                     r'--no-conversation.*--no-jinja.*--single-turn',
            "bundled provider can apply the JSON grammar to a Qwen chat control token")
    require_absent(bundled, r'L" -sysf |L" --conversation|L" --jinja',
                   "bundled provider still delegates Qwen chat rendering to llama.cpp")
    require(bundled, r'\\"capabilities\\":.*?\\"maxItems\\":10.*?'
                     r'\\"missingCapabilities\\":.*?\\"maxItems\\":16',
            "bundled output schema permits unbounded repeated capability generation")
    require_absent(bundled, r'\\"maxLength\\":',
                   "bundled output schema uses string repetition bounds rejected by llama.cpp grammar")
    require(bundled, r'--repeat-penalty 1\.20.*?--repeat-last-n 512.*?-n 4096',
            "bundled output generation is not bounded against repetition")
    require(bundled, r'test, hello, or similarly vague input.*?'
                     r'minimal side-effect-free script',
            "bundled model turns vague test requests into unrelated API demonstrations")
    require(bundled, r'md5NodeScript.*?createHash.*?writeFile',
            "bundled JavaScript prompt lacks a verified MD5 recipe")
    require(bundled, r'BuildStrictInputContract.*?'
                     r'SalamatrixAssistantInput/1\.0.*?'
                     r'contextJson.*?existingScript.*?repairFeedback',
            "bundled model input is not described by a typed strict contract")
    require(bundled, r'RuntimeInterfaceContract.*?JavaScript\.Node.*?'
                     r'Python\.CPython.*?PowerShell.*?PHP\.CLI.*?Lua',
            "bundled model lacks strict contracts for all five runtime facades")
    require(bundled, r'Lua chunk.*?Salamander is an injected global table',
            "bundled model does not receive the Lua facade conventions")
    require(ai_contract,
            r'fullFrameworkTerms.*?extensions.*?clipboard.*?application.*?ai',
            "full-framework AI requests do not receive every public API slice")
    require(ai_contract,
            r'extensionManifest.*?schema.*?schemaVersionAlias.*?capabilityValues.*?generatedPackage',
            "AI extension slice does not describe manifest packaging and capabilities")
    require(ai_contract,
            r'statictext.*?toolbarheader.*?styleFlags.*?buttonMask',
            "AI UI contract omits framework-native controls or extended options")
    require(bundled, r'this\.selectedItems does not exist',
            "bundled JavaScript contract permits an invented selection property")
    require(bundled, r'BuildStrictOutputSchema.*?'
                     r'draft/2020-12/schema.*?additionalProperties',
            "bundled model output does not have a strict JSON Schema contract")
    require(bundled, r'const std::string outputSchema = BuildStrictOutputSchema.*?'
                     r'\[OUTPUT CONTRACT.*?outputSchema.*?'
                     r'CreateUtf8PromptFile\(outputSchema',
            "prompt and llama.cpp grammar do not share one output schema instance")
    require(bundled, r'Contract priority: OUTPUT > RUNTIME > INSTALLED API > TASK',
            "strict interface contract does not define instruction priority")
    require(bundled, r'IsAssistantJsonObject.*?ExtractJsonObject',
            "bundled provider does not distinguish assistant JSON from echoed contract objects")
    require(bundled, r'found = true.*?return found',
            "bundled provider does not select the final assistant JSON object")
    require(bundled, r'parseableOutput = output.*?parseableOutput \+= diagnostics.*?'
                     r'ExtractJsonObject\(parseableOutput',
            "bundled provider does not parse generated JSON from both llama-cli streams")
    require_absent(bundled, r'exitCode\s*!=\s*0\s*\|\|',
                   "bundled provider rejects a complete JSON response because llama-cli saw stdin EOF")
    require(bundled, r'ReadAvailablePipe\(parentOut, output, outputCallback, outputContext\).*?'
                     r'ReadAvailablePipe\(parentErr, diagnostics, outputCallback, outputContext\)',
            "bundled llama stdout/stderr are no longer streamed to the visible console")
    require(bundled, r'failureOutput \+= diagnostics',
            "bundled llama failures hide the process diagnostics needed to diagnose invalid output")
    require(bundled, r'additionalProperties.*?false.*?'
                     r'estimatedEffects.*?additionalProperties.*?false',
            "bundled output schema does not close the response and effect objects")
    require(local_llama_header, r'class CLocalBundledAssistantProvider',
            "optional local llama provider declaration is missing")
    require(local_llama, r'g_ai->RegisterProvider\(&g_provider\)',
            "optional local llama provider is not registered with Salamatrix.AI")
    require(local_llama, r'g_ai->UnregisterProvider\(&g_provider\)',
            "optional local llama provider is not unregistered during release")
    require(local_llama, r'IsInterfaceModuleLoaded.*?VirtualQuery.*?'
                         r'GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS.*?'
                         r'IsInterfaceModuleLoaded\(g_ai\).*?'
                         r'g_ai->UnregisterProvider',
            "local llama Release can call an AI service from an unloaded provider DLL")
    require(local_llama, r'FUNCTION_CONFIGURATION',
            "optional local llama provider does not expose configuration")
    require(local_llama, r'install_llama\.ps1|LaunchInstaller',
            "optional local llama provider has no downloader integration")
    require(local_llama, r'CONFIG_SELECTED_MODEL.*?'
                         r'regKey != NULL.*?'
                         r'GetValue\(regKey, CONFIG_SELECTED_MODEL.*?'
                         r'SetValue\(regKey, CONFIG_SELECTED_MODEL',
            "local llama model selection is not persistent")
    require(local_llama, r'if \(registry != NULL && regKey != NULL\).*?'
                         r'SetValue\(regKey, CONFIG_SELECTED_MODEL',
            "local llama model selection writes through a null registry key")
    require(local_llama, r'qwen2\.5-coder-1\.5b-instruct-q4_k_m\.gguf.*?'
                         r'qwen2\.5-coder-0\.5b-instruct-q4_k_m\.gguf',
            "provider does not resolve both selectable model files")
    require(local_llama_rc2, r'1\.5B.*?Recommended.*?'
                             r'0\.5B.*?English prompts only',
            "localized model choices do not communicate recommendation and language limit")
    require(local_llama_installer, r"ValidateSet\('0\.5B', '1\.5B'\).*?"
                                   r'CC324AF070C2ECBFD324A30884D2F951A7FF756ABA85CB811A6EC436933BB046',
            "downloader does not expose and verify both Qwen model profiles")
    require(local_llama_installer, r'\$modelDownloadPath\s*=.*?'
                                   r'Download-VerifiedFile.*?-Path \$modelDownloadPath.*?'
                                   r'Move-Item -LiteralPath \$modelDownloadPath',
            "model download path is not kept separate from the validated Model parameter")
    require_absent(local_llama_installer, r'(?im)^\s*\$model\s*=',
                   "case-insensitive PowerShell variable collides with the Model parameter")
    require(local_llama_installer, r'Start-BitsTransfer',
            "large model downloader does not use Windows BITS")
    require_absent(local_llama_installer, r'wget(?:\.exe)?',
                   "large model downloader still depends on wget")
    require(native_test_runner, r'Get-Command \$Name -CommandType Application.*?'
                                r'Select-Object -First 1.*?'
                                r'return \[string\]\$command\.Source',
            "native CI runner does not resolve one deterministic application path")
    require(native_test_runner, r"catch \{.*?'test-infrastructure'.*?"
                                r"'native-test-runner'.*?CreateElement\('testsuite'\)",
            "native CI runner does not report infrastructure failures as JUnit")
    require(pr_tests_workflow, r'checks:\s*write.*?'
                               r'Report native and source-contract tests.*?'
                               r'dorny/test-reporter@v3.*?'
                               r'reporter:\s*java-junit.*?'
                               r'use-actions-summary:\s*false.*?'
                               r'Report Python tests.*?'
                               r'dorny/test-reporter@v3.*?'
                               r'reporter:\s*python-xunit.*?'
                               r'use-actions-summary:\s*false',
            "same-repository PR workflow does not publish both Test Reporter checks")
    require(pr_tests_workflow, r'head\.repo\.full_name == github\.repository',
            "direct Test Reporter checks are not limited to writable PR tokens")
    require(pr_tests_workflow,
            r'Show test counts on the PR workflow check.*?'
            r'steps\.native_report\.outputs\.passed !=.*?'
            r'steps\.python_report\.outputs\.passed !=.*?'
            r'CHECK_RUN_ID:\s*\$\{\{\s*job\.check_run_id\s*\}\}.*?'
            r'steps\.native_report\.outputs\.passed.*?'
            r'steps\.python_report\.outputs\.passed.*?'
            r'output\s*=\s*@\{.*?title\s*=\s*\$title.*?'
            r'check-runs/\$env:CHECK_RUN_ID',
            "PR workflow check does not display aggregate Test Reporter counts")
    require(pr_tests_workflow,
            r'repository:\s*\$\{\{\s*github\.event\.pull_request\.head\.repo\.full_name\s*\|\|\s*github\.repository\s*\}\}.*?'
            r'ref:\s*\$\{\{\s*github\.event\.pull_request\.head\.sha\s*\|\|\s*github\.sha\s*\}\}',
            "PR tests do not explicitly check out the selected source branch commit")
    require(pr_test_report_workflow,
            r'pull_requests\[0\] != null.*?'
            r'workflow_run\.head_repository\.full_name != github\.repository.*?'
            r'dorny/test-reporter@v3.*?'
            r'use-actions-summary:\s*false.*?'
            r'dorny/test-reporter@v3.*?'
            r'use-actions-summary:\s*false',
            "workflow_run Test Reporter fallback is not limited to fork PRs")
    require(runtime_protocol, r'valueEnd = position.*?'
                              r"json\[valueEnd - 1\] == '\\n'.*?"
                              r'value->assign\(json, valueStart, valueEnd - valueStart\)',
            "raw JSON member parsing does not trim insignificant trailing whitespace")
    require(local_llama_project, r'CopySalamatrixAILocalLlamaInstaller',
            "optional local llama project does not stage the downloader script")
    require_absent(local_llama_project, r'SalamatrixAIAssetRoot|SalamatrixAIBundledAsset',
                   "optional local llama project still packages model assets")
    require(ai, r'Ask is deliberately preview-only',
            "AI Ask action still performs implicit Run/Save/Export actions")
    require(ai, r'No executable automation was generated',
            "AI preview does not explain unsupported requests")
    require(ai, r'LastRuntimeId.*?RunAssistantScript\(runtimeId',
            "AI dialog does not retain the generated response runtime for explicit Run")
    for symbol in (
        "EscapeAssistantContext",
        "LoadAssistantString",
        "BuildAssistantPanelContext",
        "AssistantTemporaryScript",
        "CreateAssistantTemporaryScript",
        "AssistantUtf8ToWide",
        "GetAssistantRuntimeExtension",
        "MakeAssistantExtensionId",
        "SaveAssistantExtensionPackage",
        "SaveAssistantScript",
        "RunAssistantScript",
        "AskForRefinement",
    ):
        require(ai, rf"\b{re.escape(symbol)}\b", f"SalamatrixAI migration is missing {symbol}")
    require(ai, r"g_sides\s*=\s*static_cast<Salamatrix::Sides::ISidesService.*?Query\(\s*SALAMATRIX_SERVICE_SIDES",
            "AI panel context does not query Salamatrix.Sides")
    require(ai, r"GenerateWithRepair", "AI chat lost the bounded repair/refinement generation path")
    require(ai, r"options\.Modeless\s*=\s*TRUE.*?options\.Resizable\s*=\s*TRUE.*?options\.Taskbar\s*=\s*TRUE",
            "AI chat is not configured as a modeless taskbar-resizable window")
    require(ai, r"SetResizeCallback\(ChatResize.*?SetCloseCallback\(ChatClosed",
            "AI chat does not install modeless resize and lifetime callbacks")
    require(ai, r'providerChoice->AddItem\("auto"\).*?SetSelectedIndex\(configuredIndex\)',
            "AI chat does not keep auto as the default provider")
    require(ai, r'CONFIG_LAST_PROVIDER.*?g_lastProvider.*?"auto".*?'
                r'LoadConfiguration.*?GetValue\(regKey, CONFIG_LAST_PROVIDER.*?'
                r'SaveConfiguration.*?SetValue\(regKey, CONFIG_LAST_PROVIDER',
            "AI chat does not persist the selected provider")
    require(ai, r'CONFIG_LAST_RUNTIME.*?g_lastRuntime.*?'
                r'LoadConfiguration.*?CONFIG_LAST_RUNTIME.*?'
                r'SaveConfiguration.*?CONFIG_LAST_RUNTIME',
            "AI chat does not persist the selected runtime")
    require(ai, r'ControlId, "provider".*?RememberChoice\(chat->ProviderChoice.*?'
                r'ControlId, "runtime".*?RememberChoice\(chat->RuntimeChoice',
            "AI chat does not remember provider/runtime selection changes")
    require(ai, r'providerStatus \+= "\\r\\n"',
            "AI provider status does not render one provider per line")
    require(ai, r'std::string providerStatus;.*?!provider->IsAvailable\(\).*?'
                r'providerStatus \+= " \(ready\)"',
            "AI provider status still exposes unavailable providers")
    require(ai, r'AddControlEx\(Salamatrix::UI::ControlKindButton, runOptions, runLayout\).*?'
                r'AddControlEx\(Salamatrix::UI::ControlKindButton, exportOptions, exportLayout\)',
            "AI Run button text can be invalidated by the later Export button allocation")
    require_absent(ai, r'Provider \(auto selects the best available\)',
                   "AI provider label still contains explanatory auto-selection text")
    require(ai, r"static ChatContext\* g_chat",
            "AI chat does not retain modeless window lifetime state")
    require(ui_contract, r"virtual BOOL WINAPI SetResizeCallback.*?virtual BOOL WINAPI SetCloseCallback",
            "UI dialog contract does not expose modeless lifecycle callbacks")
    require(ui_contract, r"virtual BOOL WINAPI SetBounds\(",
            "UI control contract does not expose resizeable bounds")
    require(ui_contract, r"ControlKindSplitter\s*=\s*11",
            "UI control contract does not expose draggable splitters")
    require(ui_implementation, r"GetNativeDialogHost\(\).*?AttachStaticText.*?"
                r"AttachHyperLink.*?AttachProgressBar.*?AttachToolbarHeader",
            "NativeDialog does not route Salamatrix-specific controls through the SDK host boundary")
    require(ui_controls, r"AttachNativeStaticText.*?AttachNativeHyperLink.*?"
                r"AttachNativeProgressBar.*?AttachNativeColorArrowButton.*?"
                r"AttachNativeToolbarHeader",
            "shared SDK control implementations are incomplete")
    for project, consumer in ((salamatrix_project, "Salamatrix.SPL"),
                              (studio_host_project, "Studio preview host")):
        require(project, r"salamatrix_ui\.cpp.*?salamatrix_ui_controls\.cpp",
                f"{consumer} does not compile the shared NativeDialog and control sources")
    require(studio_host_project,
            r"<RuntimeLibrary>MultiThreaded</RuntimeLibrary>",
            "Studio preview host does not statically link the release C/C++ runtime")
    require(studio_host_build,
            r"dumpbin\.exe.*?forbiddenRuntime.*?api-ms-win-crt-.*?forbiddenImports",
            "Studio preview host build does not reject dynamic MSVC/UCRT imports")
    require_absent(studio_host, r"CreateWindowExW\(.*?PreviewProc",
                   "Studio preview host still contains its old independent Win32 renderer")
    studio_version = studio_package.get("version", "0.0.0")
    try:
        studio_version_parts = tuple(int(part) for part in studio_version.split("."))
    except (AttributeError, ValueError):
        studio_version_parts = ()
    if studio_version_parts < (0, 1, 1):
        raise AssertionError("Salamatrix Studio VSIX version is older than 0.1.1")
    if studio_package_lock.get("version") != studio_version or \
            studio_package_lock.get("packages", {}).get("", {}).get(
                "version") != studio_version:
        raise AssertionError("Salamatrix Studio package versions are inconsistent")
    studio_commands = {
        item.get("command") for item in
        studio_package.get("contributes", {}).get("commands", [])
    }
    for command in ("salamatrixStudio.createExtension",
                    "salamatrixStudio.addExistingExtensionFolder"):
        if command not in studio_commands:
            raise AssertionError(f"Salamatrix Studio does not contribute {command}")
    require(studio_extension,
            r"createExtensionProject\(\).*?findScaffoldConflicts.*?"
            r"createDirectory.*?writeFile",
            "Studio new-extension workflow does not preflight before writing")
    for section in ("Overview", "Menu Builder", "Dialogs", "Source Files"):
        if f"'{section}'" not in studio_explorer:
            raise AssertionError(
                f"Studio project explorer does not expose section {section}")
    require(studio_manifest_editor,
            r"enableGeneratedActions.*?integrateMenuDispatch.*?writeGeneratedActions",
            "Studio does not keep generated menu actions behind explicit migration")
    require(studio_menu_model,
            r"synchronizeMenuDocument.*?\.\.\.document.*?existing\.get\(handler\)",
            "Studio menu model does not preserve unknown project/action fields")
    for runtime in ("PowerShell", "Python.CPython", "JavaScript.Node",
                    "PHP.CLI", "Lua", "Automation.JScript"):
        if runtime not in studio_scaffold:
            raise AssertionError(
                f"Studio extension scaffolder omits runtime {runtime}")
    require(populate_build_dir,
            r"Release_x64.*?Microsoft\.VC143\.CRT.*?vcruntime140_1\.dll",
            "Release x64 populate does not copy vcruntime140_1.dll")
    require(setup_x64_inf,
            r"%0\\vcruntime140_1\.dll,%1\\vcruntime140_1\.dll",
            "legacy x64 payload manifest does not contain vcruntime140_1.dll")
    require(inno_setup_x64,
            r'Source: "\{#PayloadDir\}\\vcruntime140_1\.dll"; '
            r'DestDir: "\{app\}"; Flags: ignoreversion',
            "Inno Setup x64 installer does not package vcruntime140_1.dll")
    require(codesign,
            r"'vcruntime140_1\.dll'",
            "Microsoft vcruntime140_1.dll is not excluded from product signing")
    require(salamatrix_ui, r"SplitterSubclassProc.*?IDC_SIZENS.*?"
                r"WM_SALAMATRIX_SPLITTER_MOVED.*?NotifyChanged",
            "native splitters do not report drag movement")
    require(ai, r'"history-splitter".*?"console-splitter".*?'
                r'HistoryPaneHeight.*?ConsolePaneHeight',
            "AI chat does not provide independently resizable text panes")
    require(salamatrix_ui, r'GetClientRect\(window, &clientRect\).*?'
                r'SendMessage\(window, WM_SIZE.*?ShowWindow\(window, SW_SHOWNORMAL\)',
            "modeless dialogs are shown before their first responsive layout pass")
    require(salamatrix_ui, r'ControlKindComboBox.*?SelectedIndex.*?CB_SETCURSEL',
            "preselected combo-box values are not restored during native dialog creation")
    require(ai_contract, r"Specific validation error:.*validation\.Message",
            "AI repair loop does not pass the concrete contract validation error back to the model")
    require(ai_contract, r"script must contain executable source code, not a placeholder",
            "AI validation accepts placeholder scripts")
    require(ai, r"CopyTextToClipboard", "AI generated script is no longer copied for review")
    require(ai, r"PostPluginMenuChanged", "AI package export does not refresh the existing menu/discovery surface")
    require(ai, r"RefreshExtensions", "AI package export does not request manifest discovery refresh")
    require(ai, r"RuntimeExecutionFlagOneShotWorker", "AI runtime execution fallback is missing")
    require(ai_rc2, r"IDS_AI_REFINE_PROMPT.*?What should be changed", "AI refinement prompt localization is missing")
    require_absent(ai, r"\.\./automation/extensionmanifest\.h", "AI directly depends on Automation's internal manifest parser")
    require(ai, r"result\.Interface == expected", "AI Release does not compare service pointer identity")
    for global_name in ("g_ai", "g_ui", "g_runtime", "g_runner"):
        require(ai, rf"{re.escape(global_name)}\s*=\s*NULL", f"AI Release does not clear {global_name}")
    require_absent(automation_header, r"\bCmdAskAssistant\b",
                   "Automation enum still contains CmdAskAssistant")
    require_absent(automation, r"\bCmdAskAssistant\b",
                   "Automation command dispatch still contains CmdAskAssistant")
    require(automation_entry, r"SetFlagLoadOnSalamanderStart\(TRUE\)",
            "Automation legacy runtime provider is not loaded on startup")
    require_absent(automation, r"IDS_ASKASSISTANT",
                   "Automation menu still references IDS_ASKASSISTANT")
    require_absent(automation, r"AddMenuItem\([^\n]*IDS_ASKASSISTANT",
                   "Automation BuildMenu still adds Ask AI menu item")
    require(automation, r"AppendFocusedItemName",
            "AppendFocusedItemName was removed")
    require_absent(
        automation,
        r"EscapeAssistantContext|LoadAssistantString|BuildAssistantPanelContext|SaveAssistantScript|AssistantTemporaryScript|AssistantWin32Path|WriteAssistantUtf8File|CreateAssistantTemporaryScript|GetAssistantRuntimeExtension|MakeAssistantExtensionId|AssistantUtf8ToWide|SaveAssistantExtensionPackage|RunAssistantScript",
        "Automation still contains removed AI assistant helpers")

    for name, runtime, registration_var in (
        ("JavaScript", javascriptruntime, "JavaScriptRegistration"),
        ("Python", pythonruntime, "PythonRegistration"),
        ("PowerShell", powershellruntime, "PowerShellRegistration"),
        ("PHP", phpruntime, "PHPRegistration"),
        ("Lua", luaruntime, "LuaRegistration"),
    ):
        require(runtime, r"SetPluginHomePageURL\(\"https://samandarin\.net/\"\)", f"{name} runtime homepage URL is not set")
        require(runtime, r"static void UnregisterRuntimeProvider\(",
                f"{name} runtime release guard helper is missing")
        require(runtime, r"SalamanderGeneral->QueryService\(&query, &serviceResult\)",
                f"{name} runtime release guard does not query SALAMATRIX_SERVICE_RUNTIME")
        require(runtime, r"runtimeService\s*!=\s*registration\.GetService\(\)",
                f"{name} runtime release guard does not compare against registration.GetService()")
        require(runtime, r"registration = Salamatrix::Runtime::RuntimeProviderRegistration\(\)",
                f"{name} runtime release guard does not clear local registration state")
        require(runtime, rf"UnregisterRuntimeProvider\(\s*{re.escape(registration_var)}\s*\)",
                f"{name} runtime Release does not call safe registration unregister")
        require(runtime,
                r"FUNCTION_AUTOMATIONFRAMEWORK\s*\|\s*FUNCTION_CONFIGURATION\s*\|\s*FUNCTION_LOADSAVECONFIGURATION",
                f"{name} runtime does not expose Plugin Manager configuration")
        require(runtime,
                r"RuntimeConfiguration::ShowDialog.*?InvalidateExecutablePath",
                f"{name} runtime configuration does not refresh executable resolution")
        require(runtime,
                r"RuntimeConfiguration::Load.*?RuntimeConfiguration::Save",
                f"{name} runtime does not persist custom executable settings")
        require(runtime,
                r"RuntimeSettings\.UseCustomExecutable.*?CustomExecutablePath.*?return;.*?GetEnvironmentString",
                f"{name} runtime does not prefer the configured executable")

    for name, runtime_resource in (
        ("JavaScript", javascriptruntime_rc),
        ("Python", pythonruntime_rc),
        ("PowerShell", powershellruntime_rc),
        ("PHP", phpruntime_rc),
        ("Lua", luaruntime_rc),
    ):
        require_absent(runtime_resource, r"sal_r\.ico", f"{name} runtime must use the default Plugin Manager icon")
        for resource_id in (
            "IDS_RUNTIME_CONFIG_TITLE",
            "IDS_RUNTIME_EXECUTABLE_IN_USE",
            "IDS_RUNTIME_USE_CUSTOM",
            "IDS_RUNTIME_CUSTOM_EXECUTABLE",
            "IDS_RUNTIME_FILE_FILTER",
            "IDS_RUNTIME_OK",
            "IDS_RUNTIME_CANCEL",
        ):
            require(runtime_resource, resource_id,
                    f"{name} runtime configuration text is not localizable: {resource_id}")
    require(runtime_configuration,
            r"options\.Width\s*=\s*420.*?options\.Height\s*=\s*146",
            "runtime configuration dialog is no longer compact")
    require(runtime_configuration,
            r"ControlKindTextBox.*?ControlKindCheckBox.*?ControlKindFilePicker.*?IDOK.*?IDCANCEL",
            "runtime configuration dialog controls are incomplete")
    require(runtime_configuration,
            r'"UseCustomExecutable".*?"CustomExecutablePath"',
            "runtime executable selection is not persisted")
    require(runtime_configuration + pythonruntime,
            r'ExecutableValidator executableValidator = NULL.*?'
            r'!executableValidator\(selectedWide\).*?'
            r'RuntimeConfiguration::ShowDialog\(.*?'
            r'PythonExecutableDiscovery::IsUsablePythonInterpreter',
            "Python custom executable does not use provider-specific validation")
    require(python_discovery,
            r'PythonProbeTimeoutMs\s*=\s*3000.*?'
            r' -I -S -c .*?sys\.version_info\.major == 3.*?'
            r'CREATE_NO_WINDOW.*?WaitForSingleObject\(.*?'
            r'PythonProbeTimeoutMs.*?TerminateProcess',
            "Python discovery does not safely probe Python 3 with a bounded hidden process")
    require(pythonruntime + python_discovery,
            r'UseCustomExecutable.*?IsUsablePythonInterpreter.*?'
            r'GetEnvironmentString\(m_pszEnvironmentVariable.*?'
            r'FindUsableExecutable.*?m_pszCandidateOne.*?m_pszCandidateTwo.*?'
            r'FindUsableExecutableInPath',
            "Python discovery does not validate custom, environment, and all PATH candidates")
    require(pythonruntime_rc,
            r'IDS_RUNTIME_CUSTOM_INVALID\s+"[^"]*Python 3 interpreter',
            "Python invalid custom executable message is not specific or localizable")
    require(runtime_configuration,
            r'SALAMATRIX_UI_VERSION_1_2.*?'
            r'if \(strcmp\(event->ControlId, "use-custom"\) == 0\).*?'
            r'CustomPath->SetEnabled\(event->Checked\).*?'
            r'customPath->SetEnabled\(candidateUseCustom \? TRUE : FALSE\)',
            "runtime custom executable picker does not follow its checkbox")
    require(ui_contract + salamatrix_ui,
            r'SALAMATRIX_UI_VERSION_1_2.*?'
            r'virtual BOOL WINAPI SetEnabled\(BOOL enabled\).*?'
            r'EnableWindow\(WindowHandle, enabled\).*?'
            r'EnableWindow\(BrowseWindowHandle, enabled\)',
            "Salamatrix file picker cannot disable both the path and browse button")
    require(salamatrix_runtime,
            r'GetVersion\(\) const.*?SALAMATRIX_UI_VERSION_1_4.*?'
            r'RegisterServiceOwned\(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_4',
            "Salamatrix does not publish the controls-showcase UI contract version")
    for name, runtime in zip(
        ("Python", "PowerShell", "JavaScript", "PHP", "Lua"), runtime_provider_sources):
        require(runtime, r"SetFlagLoadOnSalamanderStart\(TRUE\)",
                f"{name} runtime provider is not loaded on Salamander startup")
        require(runtime, r'InvocationJson.*?(?:invocation-json|InvocationJson)',
                f"{name} runtime provider does not propagate invocation JSON")
    require(luaruntime, r'"Lua".*?"lua".*?"\.lua".*?SALAMATRIX_LUA',
            "Lua runtime descriptor or interpreter override is missing")
    require(luaruntime, r'salamatrix_worker\.lua',
            "Lua runtime does not resolve its worker bootstrap")
    require(luaruntime, r'runtime\\\\lua\.exe',
            "Lua runtime does not prefer its bundled interpreter")
    require(lua_worker, r'send_frame\("hello",\s*0,\s*\{protocol\s*=\s*1,\s*runtime\s*=\s*"lua"\}\)',
            "Lua worker does not perform the SMX1 hello handshake")
    require(lua_worker, r'salamander\.commands\.register.*?salamander\.storage\.set.*?salamander\.ui\.dialog\.create',
            "Lua worker does not expose the shared command/storage/dialog facade")
    runtime_workers = {
        "JavaScript": javascript_worker,
        "Python": python_worker,
        "PowerShell": powershell_worker,
        "PHP": php_worker,
        "Lua": lua_worker,
    }
    for name, worker in runtime_workers.items():
        require(worker, r'salamander\.host\.language',
                f"{name} worker does not expose host language")
        require(worker, r'salamander\.host\.appearance',
                f"{name} worker does not expose host appearance")
        require(worker, r'salamander\.ui\.messageBox.*?buttons.*?icon',
                f"{name} worker does not expose message-box buttons and icon")
        require(worker, r'salamander\.ui\.controls',
                f"{name} worker does not expose the framework controls showcase")
        require(worker, r'invocation',
                f"{name} worker does not expose role invocation context")
        require(worker, r'salamander\.fileSystem\.addItem',
                f"{name} worker does not expose flat file-system item publication")
        require(worker, r'salamander\.fileSystem\.addItems.*?addedCount',
                f"{name} worker does not expose batch file-system publication")
    require(manifest,
            r'SchemaVersion != 1 && SchemaVersion != 2.*?viewers.*?fileSystems.*?File-system actions',
            "manifest schema 2 does not validate Viewer and flat FS contributions")
    require(salamatrix + packages,
            r'FUNCTION_VIEWER.*?FUNCTION_FILESYSTEM.*?RegisterViewerMasks.*?GetFileSystemExtension',
            "Salamatrix does not publish native Viewer and FS roles")
    require(
        plugins_header + plugins1 + dialogs + viewer_configuration + packages,
        r'ViewerLabels.*?AddViewerWithLabel.*?ViewerLabels.*?'
        r'CB_SETITEMDATA.*?CB_GETITEMDATA.*?VIEWERS_LABEL_REG',
        "registered extension Viewers are not separate persistent configuration choices")
    require(
        base_contract,
        r'SetIconListForGUI\(CGUIIconListAbstract\* iconList\) = 0;.*?'
        r'AddViewerWithLabel\(const char\* masks, BOOL force,.*?\n};',
        "labeled Viewer registration is not append-only in the public connect ABI")
    require(
        general_contract,
        r'CSalamanderPluginViewerData.*?'
        r'SALAMANDER_PLUGIN_VIEWER_SELECTION_MAGIC.*?'
        r'CSalamanderPluginViewerSelectionData.*?ViewerLabel',
        "selected plug-in Viewer identity has no versioned viewer-data contract")
    require(
        fileswn5 + plugins_header + plugins1,
        r'plugin->ViewFile\(.*?viewer->ViewerLabel.*?'
        r'const char\* viewerLabel = NULL.*?'
        r'CSalamanderPluginViewerSelectionData selectionData.*?'
        r'SelectionMagic = SALAMANDER_PLUGIN_VIEWER_SELECTION_MAGIC.*?'
        r'PluginIfaceForViewer\.ViewFile\(.*?viewerData',
        "Alt+F3 does not carry the selected named Viewer to the plug-in")
    require(
        packages,
        r'viewerData->Size >= sizeof\(CSalamanderPluginViewerSelectionData\).*?'
        r'SelectionMagic ==.*?SALAMANDER_PLUGIN_VIEWER_SELECTION_MAGIC.*?'
        r'RunViewer\(name, invocation\.c_str\(\), viewerLabel\).*?'
        r'if \(viewerLabel != NULL && viewerLabel\[0\] != 0\).*?'
        r'_stricmp\(label\.c_str\(\), viewerLabel\) == 0.*?'
        r'viewer\.Handler\.c_str\(\)',
        "selected named Viewer is still re-matched only by the file mask")
    require(
        packages,
        r'RegisteredViewers.*?AddViewerWithLabel\(group\.c_str\(\), FALSE.*?'
        r'firstRegistration.*?AddViewerWithLabel\(group\.c_str\(\), TRUE',
        "new extension Viewer masks are not registered once while preserving user removals")
    require(
        extensions_contract + salamatrix_runtime,
        r'SALAMATRIX_EXTENSIONS_VERSION_1_4.*?'
        r'ExtensionFlagMenuExtension.*?ExtensionFlagViewer.*?'
        r'ExtensionFlagFileSystem.*?'
        r'RegisterServiceOwned\(SALAMATRIX_SERVICE_EXTENSIONS,\s*'
        r'SALAMATRIX_EXTENSIONS_VERSION_1_4',
        "Extensions 1.4 does not publish contribution metadata")
    require(
        packages,
        r'!manifest\.Commands\.empty\(\).*?ExtensionFlagMenuExtension.*?'
        r'!manifest\.Viewers\.empty\(\).*?ExtensionFlagViewer.*?'
        r'!manifest\.FileSystems\.empty\(\).*?ExtensionFlagFileSystem',
        "manifest contribution flags are not derived during package discovery")
    require(
        packages,
        r'void PackageManager::RefreshContributionFlags\(Package\* package\).*?'
        r'ExtensionFlagMenuExtension.*?RegisterExtension.*?'
        r'commands\.register.*?RefreshContributionFlags\(package\).*?'
        r'commands\.unregister.*?RefreshContributionFlags\(package\)',
        "dynamic command registration does not refresh Menu Extension metadata")
    require(
        automation_scriptlist,
        r'!pScript->m_salamatrixManifestCommands\.empty\(\).*?'
        r'ExtensionFlagMenuExtension',
        "Automation compatibility registration omits its menu contribution")
    require(
        dialogs,
        r'ExtensionFlagViewer.*?IDS_PLUGINFUNCFILEVIEWER.*?'
        r'ExtensionFlagMenuExtension.*?IDS_PLUGINFUNCMENUEXTENSION.*?'
        r'ExtensionFlagFileSystem.*?IDS_PLUGINFUNCFILESYSTEM.*?'
        r'IDC_PLUGINFUNCTIONS.*?extensionFunctions',
        "Plugin Manager does not render extension contributions in Functions")
    viewer_registration = re.search(
        r'void PackageManager::RegisterViewerMasks\(.*?'
        r'(?=\nvoid PackageManager::SetRefreshDeferred)',
        packages, re.MULTILINE | re.DOTALL)
    if viewer_registration is None:
        raise AssertionError("Viewer registration implementation is missing")
    require_absent(
        viewer_registration.group(0), r'RuntimeUsable',
        "Viewer registration still depends on runtime-provider startup order")
    require(
        viewer_registration.group(0), r'ExtensionFlagDisabled',
        "Viewer registration does not exclude disabled extensions")
    require(packages, r'FileSystemListing.*?salamander\.fileSystem\.addItem.*?4096',
            "flat FS dispatcher does not bound runtime-provided items")
    require(
        packages,
        r'salamander\.fileSystem\.addItems.*?FindRawMember.*?items.*?'
        r'originalCount.*?PendingFileSystemItems\.resize\(originalCount\)',
        "batch FS publication is not parsed and rolled back atomically")
    require(
        packages,
        r'backgroundFileSystemItem.*?salamander\.fileSystem\.addItem.*?'
        r'FileSystemListing.*?!backgroundFileSystemItem',
        "FS items are still synchronously dispatched through the UI thread")
    require(
        packages,
        r'InterlockedExchange\(&package->Stopping,\s*TRUE\).*?'
        r'WaitForThreadWithSentMessageDispatch.*?'
        r'package->Session->Release\(\)',
        "package shutdown can release a runtime session before its pump thread exits")
    require_absent(
        packages,
        r'WaitForSingleObject\(package->PumpThread,\s*5000\)',
        "package shutdown still frees a live pump thread after a timed wait")
    require(
        automation_scriptlist,
        r'InterlockedExchange\(&m_lRuntimeStopping,\s*TRUE\).*?'
        r'WaitForThreadWithSentMessageDispatch.*?'
        r'm_pRuntimeSession->Release\(\)',
        "Automation shutdown can release a runtime session before its pump thread exits")
    require_absent(
        automation_scriptlist,
        r'(?:WaitForSingleObject\(m_hRuntimePumpThread,\s*5000\)|'
        r'TerminateThread\(m_hRuntimePumpThread)',
        "Automation still times out or terminates a live runtime pump thread")
    require(packages, r'FS_SERVICE_CONTEXTMENU.*?ContextMenu\(',
            "flat FS does not expose native actions")
    require(
        packages,
        r'DupStr\("\.\."\).*?FS_SERVICE_GETNEXTDIRLINEHOTPATH.*?'
        r'FS_SERVICE_GETPATHFORMAINWNDTITLE.*?GetNextDirectoryLineHotPath.*?'
        r'GetPathForMainWindowTitle',
        "flat FS does not expose up-directory, breadcrumb and title path services")
    require(
        packages,
        r'if \(isDir == 2\).*?GetParentPath\(\).*?ChangePanelPathToPluginFS\(.*?'
        r'SalamatrixFileSystemItemData\* data',
        "hierarchical FS does not navigate the native up-directory item to its parent")
    require(ui_contract + salamatrix_ui + salamatrix_runtime + packages,
            r'SALAMATRIX_UI_VERSION_1_4.*?'
            r'ShowControlsShowcase.*?ShowNativeControlsShowcase.*?'
            r'salamander\.ui\.controls.*?ShowControlsShowcase',
            "controls showcase is not owned and dispatched by Salamatrix Framework")
    require(ui_contract + salamatrix_ui,
            r'ControlKindStaticText.*?ControlKindHyperLink.*?'
            r'ControlKindProgressBar.*?ControlKindArrowButton.*?'
            r'ControlKindTextArrowButton.*?ControlKindColorArrowButton.*?'
            r'ControlKindToolbarHeader.*?AttachStaticText.*?AttachHyperLink.*?'
            r'AttachProgressBar.*?ChangeToArrowButton.*?AttachButton.*?'
            r'AttachColorArrowButton.*?AttachToolbarHeader',
            "Salamatrix UI does not expose every host control demonstrated by DemoPlug")
    require(salamatrix_ui,
            r'options\.Width = 463.*?options\.Height = 236.*?'
            r'"CGUIStaticTextAbstract", 6, 4, 254, 108.*?'
            r'"CGUIProgressBarAbstract", 6, 118, 254, 66.*?'
            r'"CGUIHyperLinkAbstract", 269, 4, 185, 48.*?'
            r'"close", "Close", 403, 213, 50, 14',
            "Salamatrix controls showcase no longer matches DemoPlug geometry")
    require(salamatrix_ui,
            r'host->PrepareTheme.*?ApplyNativeDialogDarkMode.*?'
            r'AttachStaticText.*?AttachHyperLink.*?AttachProgressBar.*?'
            r'ApplyNativeDialogDarkMode\(hwnd\)',
            "host controls are not re-themed after attachment for dark mode")
    require(php_worker,
            r'function call\(\$method, \$arguments = array\(\)\).*?'
            r"salamander\.ui\.controls', array\(\)",
            "PHP no-argument controls call still violates the worker call signature")
    require(javascript_worker,
            r'salamander\.ui\.inputBox.*?\{\s*prompt,\s*title,\s*initial\s*\}',
            "JavaScript worker does not use the shared input-box payload")
    require_absent(javascript_worker, r'initialValue',
                   "JavaScript worker still uses the obsolete input-box field")
    for wire_method in ("column", "selection", "validation", "clearItems"):
        require(javascript_worker,
                rf'salamander\.ui\.dialog\.{wire_method}',
                f"JavaScript worker does not use dialog.{wire_method}")
    for obsolete_method in ("addColumn", "setSelectedIndex", "setValidation"):
        require_absent(
            javascript_worker,
            rf'salamander\.ui\.dialog\.{obsolete_method}',
            f"JavaScript worker still uses unsupported dialog.{obsolete_method}")
    require(javascript_worker,
            r'kind\s*===\s*"filepicker".*?payload\.filter.*?payload\.save',
            "JavaScript file-picker control drops filter or save options")
    require(lua_demo,
            r'Salamander\.ui\.notify.*?Salamander\.ui\.progress.*?progress\.update.*?progress\.is_cancelled.*?progress\.close.*?Salamander\.storage\.set\("lastRun",\s*"Lua"\)',
            "Lua demo does not exercise the shared notify/progress/storage flow")
    if "ui.dialogs" not in lua_demo_manifest.get("capabilities", []):
        raise AssertionError(
            "Lua demo manifest does not declare the canonical UI capability")
    manifest_paths = list((ROOT / "src/extensions").rglob("extension.json"))
    manifest_paths.extend((ROOT / "src/tools/salamatrix-studio/examples").rglob(
        "extension.json"))
    manifest_paths.extend((ROOT / "src/plugins/automation/sample-scripts").rglob(
        "extension.json"))
    for manifest_path in manifest_paths:
        package_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if package_manifest.get("schema") not in {1, 2} or \
                "schemaVersion" in package_manifest:
            raise AssertionError(
                f"extension demo does not use canonical schema: {manifest_path}")
        aliases = {"ui.notify", "ui.progress"}.intersection(
            package_manifest.get("capabilities", []))
        if aliases:
            raise AssertionError(
                f"extension demo uses non-canonical capabilities {aliases}: "
                f"{manifest_path}")
        for command in package_manifest.get("commands", []):
            if command.get("menu", "plugin") not in {
                    "plugin", "context", "both", "none"}:
                raise AssertionError(
                    f"extension demo uses an invalid menu placement: "
                    f"{manifest_path}")
    demo_roles = {
        "Node": (javascript_demo_manifest, javascript_demo),
        "Python": (python_demo_manifest, python_demo),
        "PowerShell": (powershell_demo_manifest, powershell_demo),
        "PHP": (php_demo_manifest, php_demo),
        "Lua": (lua_demo_manifest, lua_demo),
    }
    demo_versions = {
        "Node": "1.4.1",
        "Python": "1.4.2",
        "PowerShell": "1.4.1",
        "PHP": "1.4.1",
        "Lua": "1.4.1",
    }
    viewer_patterns = set()
    for runtime_name, (demo_manifest, demo_source) in demo_roles.items():
        viewers = demo_manifest.get("viewers", [])
        file_systems = demo_manifest.get("fileSystems", [])
        if (demo_manifest.get("schema") != 2 or not viewers or
                demo_manifest.get("version") != demo_versions[runtime_name] or
                not viewers[0].get("name") or
                viewers[0].get("handler") != "viewDemo" or
                not file_systems or
                file_systems[0].get("listHandler") != "listDemoMachines"):
            raise AssertionError(
                f"{runtime_name} demo does not declare named schema-2 Viewer/FS roles")
        viewer_patterns.update(viewers[0].get("patterns", []))
        for handler in ("viewDemo", "listDemoMachines",
                        "inspectDemoMachine", "toggleDemoMachine"):
            if handler not in demo_source:
                raise AssertionError(
                    f"{runtime_name} demo does not implement {handler}")
    if len(viewer_patterns) != len(demo_roles):
        raise AssertionError("demo Viewer masks must be distinct across runtimes")
    for runtime_name, demo_source, obsolete_default in (
        ("Node", javascript_demo, r'storage\.get\(key, false\)'),
        ("Python", python_demo, r'storage\.get\(key, False\)'),
        ("PowerShell", powershell_demo, r'storage\.Get\(\$key, \$false\)'),
        ("PHP", php_demo, r'storage->get\(\$key, false\)'),
        ("Lua", lua_demo, r'storage\.get\(key, false\)'),
    ):
        require_absent(
            demo_source, obsolete_default,
            f"{runtime_name} FS toggle ignores the item's initial running state")
    require(setup, r"extension-runtimes\\luaruntime\\luaruntime\.spl.*?IsPluginSelected\('luaruntime'\)",
            "x64 installer does not package LuaRuntime.SPL")
    require(setup, r"extension-runtimes\\luaruntime\\runtime\\salamatrix_worker\.lua.*?IsPluginSelected\('luaruntime'\)",
            "x64 installer does not package the Lua worker")
    for bundled_asset in ("lua.exe", "lua.dll", "LICENSE-LUA.txt"):
        require(setup,
                rf"extension-runtimes\\luaruntime\\runtime\\{re.escape(bundled_asset)}.*?IsPluginSelected\('luaruntime'\)",
                f"x64 installer does not package bundled Lua asset {bundled_asset}")
    require(setup, r"AddPluginDependency\('luaruntime',\s*'salamatrix'\)",
            "x64 installer does not select Salamatrix for Lua Runtime")
    require(
        lua_runtime,
        r'AppendBase64Utf8QuotedArgument.*?'
        r'--invocation-json-base64.*?request->InvocationJson',
        "Lua runtime passes Unicode invocation JSON through narrow Windows argv")
    require(
        lua_worker,
        r'function decode_base64.*?invocation_json_base64.*?decode_json',
        "Lua worker does not restore Base64-protected Unicode invocation JSON")
    require(runtime_package_verifier,
            r"Name\s*=\s*'luaruntime'.*?extension-runtimes\\luaruntime.*?salamatrix_worker\.lua.*?lua\.exe.*?lua\.dll.*?LICENSE-LUA\.txt",
            "runtime package verifier does not validate the Lua provider layout")

    require(salamatrix_props, r"USE_DARKMODELIB=1", "Salamatrix Framework is not built with win32-darkmodelib")
    require(salamatrix, r"ApplyHostDarkModePolicy\(SalamanderGeneral", "Salamatrix host dark-mode policy is not initialized")
    require(salamatrix_runtime, r"DarkModeSetConfiguredColors", "Salamatrix dark-mode scheme colors are not synchronized")
    require(salamatrix_runtime, r"DarkModeMessageBoxW", "Salamatrix runtime message boxes do not use the Unicode dark-mode path")
    require(salamatrix_ui, r"WM_SETTINGCHANGE \|\| message == WM_THEMECHANGED", "Salamatrix dialog theme-change handling is missing")
    require(ui_salamander_host, r"DarkModeRefreshTitleBar\(window\)", "Salamatrix dialog title bar dark-mode refresh is missing")
    require(
        ui_salamander_host,
        r'SalamanderGUI->AttachStaticText.*?'
        r'SalamanderGUI->AttachHyperLink.*?'
        r'SalamanderGUI->AttachProgressBar.*?'
        r'SalamanderGUI->ChangeToArrowButton.*?'
        r'SalamanderGUI->AttachButton.*?'
        r'SalamanderGUI->AttachColorArrowButton.*?'
        r'SalamanderGUI->AttachToolbarHeader',
        "in-process Salamatrix dialogs do not use Salamander's native CGUI controls")
    require_absent(
        ui_salamander_host,
        r'AttachNative(?:StaticText|HyperLink|ProgressBar|Button|ColorArrowButton|ToolbarHeader)',
        "in-process Salamatrix dialogs incorrectly use the standalone preview fallback controls")
    require(
        salamatrix_ui,
        r'control->KeepOpen \|\| control->DialogResult == 0',
        "zero-result action buttons do not remain open for the legacy Automation facade")
    require(
        local_llama,
        r'closeOptions\.Id = "close";.*?'
        r'closeOptions\.DialogResult = IDCANCEL;',
        "Local Llama configuration Close button does not close its modal dialog")
    require(
        packages,
        r'interactiveModalCall.*?salamander\.ui\.dialog\.show.*?'
        r'salamander\.ui\.controls.*?salamander\.ui\.messageBox.*?'
        r'salamander\.ui\.pickFile.*?salamander\.ui\.pickFolder.*?'
        r'interactiveModalCall \? INFINITE : 120000',
        "modal extension UI calls can time out while the user keeps a dialog open")
    require(salamatrix_ui + ui_salamander_host, r"ApplyDarkScrollbarScopes\(BOOL dark\).*?SetDarkScrollbars.*?DarkModeAllowDarkScrollbars\(window\).*?DarkModeDisallowDarkScrollbars\(window\)",
            "Salamatrix dialogs do not scope the host dark scrollbar hook to controls")
    require(salamatrix_ui, r"PostMessage\(hwnd, WM_SALAMATRIX_APPLY_DARK_SCROLLBARS",
            "Salamatrix dialogs apply dark scrollbar scopes during WM_INIT reentrantly")
    require(salamatrix_ui, r'RegisterNativeDialog\(dialog\).*?'
                           r'WM_NCDESTROY.*?UnregisterNativeDialog\(dialog\)',
            "Salamatrix UI provider does not track active native dialog lifetimes")
    require(salamatrix_ui, r'CloseAllNativeDialogs\(\).*?'
                           r'ClosingAllNativeDialogs = TRUE.*?'
                           r'while \(!OpenNotificationWindows\.empty\(\)\).*?'
                           r'DestroyWindow\(window\).*?'
                           r'while \(!OpenNativeDialogs\.empty\(\)\).*?'
                           r'dialog->Close\(\)',
            "Salamatrix UI provider cannot close active windows before DLL unload")
    require(salamatrix_ui, r'RegisterNotificationWindow\(window\)',
            "Salamatrix UI provider does not track notification windows")
    require(salamatrix_ui, r'WM_NCDESTROY.*?UnregisterNotificationWindow\(window\)',
            "Salamatrix UI provider retains destroyed notification windows")
    require(salamatrix, r'CPluginInterface::Release.*?'
                        r'CloseAllNativeDialogs\(\).*?'
                        r'DestroyRuntimeServices\(\)',
            "Salamatrix unloads native dialog procedures before their HWNDs")

    require(packages, r"BOOL RuntimeUsable;", "extension package runtime usability state is missing")
    require(packages, r"plugins.*automation.*scripts", "Automation sample-script extension root is missing")
    require(packages, r"salamander\.ui\.progress\.create", "framework progress host dispatch is missing")
    require(
        packages,
        r'salamander\.ui\.pickFile.*?PickFile\(.*?'
        r'salamander\.ui\.pickFolder.*?PickFolder\(',
        "framework file and folder picker host dispatch is missing")
    require(
        packages + menu_builder,
        r'salamander\.ui\.renderIcon.*?CreateSVGIcon.*?'
        r'Get-BuilderPreviewImage.*?item\.Image',
        "Extension Menu Builder preview does not render command SVG icons")
    require(packages, r"SALAMATRIX_SERVICE_SCRIPT_RUNNER", "legacy compatibility script runner fallback is missing")
    require(packages, r"RuntimeAdapterFlagCompatibility", "legacy fallback is not limited to compatibility adapters")
    require(
        packages,
        r"\(!registeredRuntime \|\| !availableRuntime\).*?"
        r"Automation\.JScript.*?QueryScriptRunner.*?"
        r"registeredRuntime = true;.*?availableRuntime = true;",
        "Automation.JScript does not remain usable through its ScriptRunner fallback")
    require(packages, r"package->RuntimeUsable = registeredRuntime && availableRuntime",
            "extension package runtime usability is not derived from provider availability")
    require(
        packages,
        r"void PackageManager::Refresh\(\).*?"
        r"UnregisterToolbarButtons\(\);.*?RemovePackages\(\);.*?"
        r"RegisterToolbarButtons\(\);",
        "package refresh does not rebuild Extension Bar registrations cleanly")
    require(packages, r"InvokeOnMainThread\(\s*HostDispatchOnMainThread",
            "extension host calls are not marshaled to Salamander's UI thread")
    require(
        packages,
        r'salamander\.sides\.context.*?'
        r'GetSelectedItemCount.*?GetSelectedItem.*?'
        r'GetFocusedItem.*?'
        r'selectedItems.*?focusedItem',
        "process runtimes do not receive the selected and focused panel items")
    require_absent(
        packages,
        r'"selectedItems":\[\],"focusedItem":null',
        "process-runtime panel context still discards selection and focus")
    require(packages, r"MessageHello\).*?CopyResult\(\"\{\\\"ok\\\":true\}\"",
            "extension host does not acknowledge the runtime worker handshake")
    require(packages, r"if \(!package->RuntimeUsable\)\s+continue;.*?BuildMenu",
            "unavailable extension packages are not filtered from the menu")
    require(packages, r"Automation API &Reference\.\.\.",
            "Salamatrix plugin menu does not expose the installed Automation API reference")
    require(packages, r"OpenAutomationApiReference",
            "Salamatrix plugin menu does not open the Automation API reference")
    require(api_docs, r"plugins.*?salamatrix.*?salamatrix-automation-api\.html.*?"
                r"OpenFileInConfiguredViewer",
            "Automation API reference is not resolved from the installed Salamatrix plugin directory")
    require(general_contract, r"SetPanelsDetached.*?"
                r"OpenFileInConfiguredViewer\(HWND parent",
            "configured-viewer SDK method is not append-only")
    require(general_impl, r"OpenFileInConfiguredViewer.*?"
                r"ViewFileInt\(parent, fileName, FALSE, 0xFFFFFFFF",
            "plugin documentation does not use the same configured-viewer selection path as built-in documentation")
    require(ai, r'"api-reference".*?OpenAutomationApiReference',
            "Salamatrix AI window has no Automation API reference button")
    require(ai, r'PHP\|\*\.php\|Lua\|\*\.lua\|All files',
            "Salamatrix AI Save script picker does not expose the Lua extension")
    require(setup, r"plugins\\salamatrix\\salamatrix-automation-api\.html",
            "installer does not package the Automation API HTML reference")
    for document in (
            "salamatrix-ui.html",
            "salamatrix-platform.html",
            "salamatrix-runtime-providers.html",
            "salamatrix-runtime-provider-development.html",
            "salamatrix-gap-analysis.html"):
        require(salamatrix_props, re.escape(document),
                f"Salamatrix build does not stage {document}")
        require(setup, re.escape(document),
                f"installer does not package {document}")
    require(packages, r"void PackageManager::RegisterToolbarButtons\(\).*?if \(!package->RuntimeUsable\)\s+continue;",
            "unavailable extension packages are not filtered from the toolbar")
    require(
        manifest + general_contract + packages,
        r"toolbarMenu.*?ToolbarMenu.*?"
        r"CSalamanderToolbarMenuItem.*?"
        r"button\.MenuItems",
        "package toolbar menus are not registered through the public toolbar API")
    require(
        toolbar8,
        r"GetToolbarButtonInfo\(.*?&menu\).*?"
        r"TLBI_STYLE_WHOLEDROPDOWN.*?TLBI_STYLE_DROPDOWN",
        "Extension Bar menu buttons are not rendered as dropdowns")
    require(
        plugins2,
        r"ExecuteToolbarButton\(.*?MenuItems.*?"
        r"RenderSVGIconBitmapFromFile.*?"
        r"MENU_MASK_IMAGEINDEX.*?"
        r"MENU_TRACK_RETURNCMD.*?"
        r"ExecuteMenuItem2",
        "Extension Bar menu buttons do not render icons and execute popup commands")
    require(
        plugins2 + packages,
        r"toolbarIdCount.*?ToolbarButtons\[index\]\.ToolbarId == toolbarId.*?"
        r"NextToolbarButtonId = toolbarId.*?"
        r"RefreshInProgress \|\| ActiveHostDispatches.*?ActiveExecutions.*?"
        r"RefreshPending.*?FinishHostDispatch.*?FinishExecution",
        "Extension Bar IDs are not recycled or package refresh can invalidate an active package operation")
    require(
        packages,
        r"class PackageManager::ExecutionGuard.*?"
        r"Owner->BeginExecution\(\).*?"
        r"Owner->FinishExecution\(\).*?"
        r"MenuExtension.*?ExecutionGuard execution\(Owner\).*?"
        r"RunViewer.*?ExecutionGuard execution\(this\).*?"
        r"ListFileSystem.*?ExecutionGuard execution\(this\).*?"
        r"ExecuteFileSystemAction.*?ExecutionGuard execution\(this\)",
        "package operations are not protected from a reentrant extension catalog refresh")
    require(
        packages,
        r"!action\.Refresh.*?QueueFileSystemAction.*?"
        r"CreateThread\(.*?FileSystemActionThreadProc.*?"
        r"ExecuteFileSystemActionNow.*?FinishExecution",
        "non-refreshing FS modal actions still block the panel callback thread")
    require(
        packages,
        r"ExecutionsIdleEvent\(CreateEvent.*?Shutdown.*?"
        r"WaitForThreadWithSentMessageDispatch.*?BeginExecution.*?"
        r"ResetEvent.*?FinishExecution.*?SetEvent",
        "asynchronous FS actions are not joined safely during shutdown")
    require(
        packages,
        r"FileSystemActionGeneration.*?FileSystemActionPending.*?"
        r"InterlockedCompareExchange.*?generation.*?"
        r"FileSystemActionThreadProc.*?task->Generation",
        "repeated FS modal actions can queue duplicate dialogs")
    require(
        packages,
        r"FileSystemExecutionLock.*?FileSystemActionExecutionLock.*?"
        r"ExecuteFileSystemActionNow.*?FileSystemActionExecutionLock",
        "modal FS actions can deadlock the UI by holding the listing lock")
    require(
        general_contract + plugins2 + packages,
        r"CSalamanderToolbarMenuItem.*?IconPath.*?IconDarkPath.*?"
        r"itemIconsEnd.*?"
        r"menuItem\.IconPath.*?menuItem\.IconDarkPath",
        "Extension Bar popup commands do not support per-item light/dark icons")
    require(
        packages,
        r"packageIconIndex.*?AddSubmenuStart\(\s*"
        r"packageIconIndex",
        "multi-command extension submenus do not display the package icon")
    require(
        mainwnd3,
        r"WM_USER_TBDROPDOWN.*?CM_EXTTOOLBAR_MIN.*?"
        r"ExecuteToolbarButton\(.*?&r",
        "Extension Bar dropdown notifications are not routed to toolbar menus")
    require_absent(
        toolbar4,
        r"FillExtensionTII|GetToolbarButtonCount",
        "extension buttons can still leak into native Top/Middle/Panel toolbars")
    require(
        toolbar8,
        r"CExtensionBar::CreateExtensionButtons.*?"
        r"GetExtensionBarVisible.*?InsertItem2",
        "Extension Bar does not own and filter extension buttons")
    require(
        mainwnd1,
        r"ToggleExtensionBar.*?BANDID_EXTENSIONBAR.*?"
        r"InsertExtensionBarBand",
        "Extension Bar lifecycle is not integrated with the main rebar")
    require(
        mainwnd2,
        r"Show Extension Bar.*?ExtensionBarVisible.*?"
        r"ExtensionBarIndex",
        "Extension Bar visibility or placement is not persisted")
    require(
        mainwnd3 + main_menu,
        r"CM_TOGGLEEXTENSIONBAR",
        "Options - Show cannot toggle Extension Bar")
    require(
        dialogs,
        r"IDS_PLUGIN_RUNTIME_LABEL.*?"
        r"IDS_PLUGIN_SHOWINEXTENSIONBAR.*?"
        r"SetExtensionBarVisible",
        "Plugin Manager does not expose localized Extension Bar controls")
    require(
        dialogs,
        r"void CPluginsDlg::OnSelChanged\(\).*?"
        r"else if \(extension != NULL\).*?EnableButtons\(NULL\);\s*\}\s*"
        r"else\s*\{.*?EnableButtons\(NULL\);\s*\}\s*"
        r"EnableHeader\(\);\s*\}",
        "Plugin Manager does not refresh extension move buttons for every selection")
    require(
        dialogs,
        r"void CPluginsDlg::EnableHeader\(\).*?"
        r"TLBHDRMASK_TOP \| TLBHDRMASK_UP.*?"
        r"TLBHDRMASK_DOWN \| TLBHDRMASK_BOTTOM",
        "Plugin Manager does not enable move-to-top and move-to-bottom buttons")
    require(
        dialogs,
        r"void CPluginsDlg::OnMove\(BOOL up, BOOL toEnd\).*?"
        r"toEnd \? \(up \? 0 : Plugins\.GetCount\(\) - 1\).*?"
        r"while \(movedExtensionIndex != newExtensionIndex &&\s*"
        r"service->MoveManagedExtension\(extensionId, direction\)\)",
        "Plugin Manager does not move plugins and extensions to list boundaries")
    require(
        dialogs,
        r"TLBHDRMASK_SORT \| TLBHDRMASK_TOP \|\s*"
        r"TLBHDRMASK_UP \| TLBHDRMASK_DOWN \|\s*"
        r"TLBHDRMASK_BOTTOM.*?"
        r"case TLBHDR_TOP:.*?OnMove\(TRUE, TRUE\).*?"
        r"case TLBHDR_BOTTOM:.*?OnMove\(FALSE, TRUE\)",
        "Plugin Manager header does not expose boundary move commands")
    require(
        dialog_resources,
        r'IDD_PLUGINS DIALOGEX.*?CAPTION "Plugins and Extensions Manager".*?'
        r'"Installed &Plugins and Extensions: \(total: %d, loaded: %d\)"',
        "Plugin Manager does not use the combined plugins and extensions title")
    require(
        dialog_resources,
        r"IDD_PLUGINS DIALOGEX[^\n]*\n"
        r"STYLE (?=[^\n]*DS_MODALFRAME)(?=[^\n]*WS_THICKFRAME)"
        r"(?![^\n]*(?:WS_MINIMIZEBOX|WS_MAXIMIZEBOX))[^\n]*\n"
        r".*?IDC_PLUGINS_GRIP.*?SBS_SIZEBOXBOTTOMRIGHTALIGN",
        "Plugin Manager does not use the Configuration-style resizable dialog frame and grip")
    require(
        dialogs,
        r"RESTORE_PLUGIN_MANAGER_GRIP_DEBUG_NEW_MACRO.*?"
        r"GripWindow = new \(std::nothrow\) CTPHGripWindow.*?"
        r"#define new new \(_NORMAL_BLOCK, __FILE__, __LINE__\).*?"
        r"if \(GripWindow == NULL\).*?"
        r"DarkModeApplyWindow\(GripWindow->HWindow\)",
        "Plugin Manager resize grip does not preserve nothrow OOM handling and dark mode")
    require(
        dialogs,
        r"void CPluginsDlg::LayoutControls\(\).*?"
        r"RDW_INVALIDATE \| RDW_ERASE \| "
        r"RDW_ALLCHILDREN \| RDW_UPDATENOW",
        "Plugin Manager does not fully repaint after resizing")
    require(
        configuration_header + configuration_defaults + mainwnd2 + dialogs,
        r"PluginsManagerWidth.*?PluginsManagerHeight.*?"
        r"PluginsManagerWidth = 0.*?PluginsManagerHeight = 0.*?"
        r"Plugins Manager Width.*?Plugins Manager Height.*?"
        r"CONFIG_PLUGINS_MANAGER_WIDTH.*?SetValue.*?"
        r"CONFIG_PLUGINS_MANAGER_HEIGHT.*?SetValue.*?"
        r"CONFIG_PLUGINS_MANAGER_WIDTH.*?GetValue.*?"
        r"CONFIG_PLUGINS_MANAGER_HEIGHT.*?GetValue.*?"
        r"Configuration\.PluginsManagerWidth.*?SetWindowPos.*?"
        r"GetWindowPlacement.*?rcNormalPosition.*?"
        r"Configuration\.PluginsManagerWidth = width.*?"
        r"Configuration\.PluginsManagerHeight = height",
        "Plugin Manager size is not restored and persisted through main configuration")
    require_absent(
        dialogs,
        r"_snprintf_s\([^;]*LoadStr\(IDS_PLUGIN_SHOWINEXTENSIONBAR\)",
        "localized Extension Bar text is still used as a printf format")
    require(
        dialogs,
        r'extensionNamePlaceholder.*?"%\.\*s%s%s"',
        "localized Extension Bar placeholder is not expanded as inert data")
    require(
        mainwnd3,
        r"ExtensionBar = new \(std::nothrow\) CExtensionBar.*?"
        r"if \(ExtensionBar == NULL\)",
        "Extension Bar allocation guard is not backed by nothrow allocation")
    require(
        mainwnd3,
        r"CWaitWindow closingProgress\(\s*"
        r"HWindow, IDS_CLOSINGEXTENSIONS.*?"
        r"closingProgress\.Create\(\).*?"
        r"ssdpSavingConfiguration.*?"
        r"SaveConfig\(closingProgress\.HWindow, ordinaryClose\).*?"
        r"Plugins\.UnloadAll\(closingProgress\.HWindow,\s*"
        r"&shutdownProgressService\).*?"
        r"ssdpClosingPanels.*?"
        r"ConfirmDetachedWindowClose\(closingProgress\.HWindow.*?"
        r"ssdpFinishingShutdown.*?"
        r"DiskCache\.PrepareForShutdown\(\).*?"
        r"DestroyWindow\(closingProgress\.HWindow\).*?"
        r"DestroyWindow\(HWindow\)",
        "ordinary close does not show localized progress across the full shutdown sequence")
    for resource_id in range(14196, 14199):
        occurrences = len(re.findall(
            rf"^{resource_id},1,\"[^\"]+\"$",
            shutdown_translations,
            re.MULTILINE))
        if occurrences != 10:
            raise AssertionError(
                f"shutdown status {resource_id} is not localized in all 10 translations")
    require(
        general_contract,
        r'SALAMANDER_SERVICE_STARTUP_PROGRESS.*?'
        r'class CSalamanderStartupProgressAbstract.*?'
        r'ReportStartupProgress',
        "shared SDK does not expose the temporary startup progress service")
    require(
        plugins2,
        r'CWaitWindow startupProgress\(.*?IDS_STARTUP_LOADINGPLUGINS.*?'
        r'startupProgress\.Create\(\).*?'
        r'RegisterService\(\s*SALAMANDER_SERVICE_STARTUP_PROGRESS.*?'
        r'Event\(PLUGINEVENT_STARTUPBATCHBEGIN, 0\).*?'
        r'InitDLL\(progressParent, TRUE\).*?'
        r'Event\(PLUGINEVENT_STARTUPCOMPLETE, 0\).*?'
        r'Event\(PLUGINEVENT_STARTUPBATCHCOMPLETE, 0\).*?'
        r'ssppFinishingStartup.*?UnregisterService\(.*?'
        r'DestroyWindow\(startupProgress\.HWindow\)',
        "load-on-start plugins and extensions are not covered by one progress window")
    for phase in (
            "ssppDiscoveringExtensions", "ssppRegisteringExtensions",
            "ssppRegisteringFileSystems", "ssppRegisteringMenuCommands",
            "ssppActivatingExtensions", "ssppRegisteringToolbarButtons",
            "ssppRegisteringViewers"):
        require(
            packages, phase,
            f"Salamatrix startup does not report {phase}")
    require(
        packages,
        r'query\.ServiceId = SALAMANDER_SERVICE_STARTUP_PROGRESS.*?'
        r'QueryService\(&query, &result\).*?ReportStartupProgress',
        "Salamatrix package manager does not use the temporary host progress service")
    require(
        salamatrix,
        r'IsLoadOnStartBatchActive\(\).*?'
        r'SetRefreshDeferred\(IsLoadOnStartBatchActive\(\)\).*?'
        r'PLUGINEVENT_STARTUPBATCHBEGIN.*?SetRefreshDeferred\(TRUE\).*?'
        r'PLUGINEVENT_STARTUPBATCHCOMPLETE.*?CompleteStartupRefreshBatch\(\)',
        "load-on-start runtime registrations are not coalesced into one catalog refresh")
    require(
        packages,
        r'CompleteStartupRefreshBatch\(\).*?RefreshPending = FALSE.*?'
        r'ResolveDependenciesAndActivate\(\)',
        "startup runtime completion still requires destructive catalog rediscovery")
    require_absent(
        salamatrix,
        r'PLUGINEVENT_CONFIGURATIONCHANGED.*?SalamatrixPackages->Refresh\(\)',
        "ordinary configuration changes still tear down and rediscover live extension file systems")
    require(
        packages,
        r'if \(RefreshDeferred\).*?RefreshPending = TRUE.*?'
        r'SALAMANDER_SERVICE_SHUTDOWN_PROGRESS.*?'
        r'QueryService\(&query, &result\).*?RefreshPending = FALSE',
        "catalog refreshes are not deferred during startup and suppressed during shutdown")
    for resource_id in range(14320, 14330):
        occurrences = len(re.findall(
            rf"^{resource_id},1,\"[^\"]+\"$",
            shutdown_translations,
            re.MULTILINE))
        if occurrences != 10:
            raise AssertionError(
                f"extension/startup status {resource_id} is not localized in all 10 translations")
    require(
        general_contract,
        r'SALAMANDER_SERVICE_SHUTDOWN_PROGRESS.*?'
        r'class CSalamanderShutdownProgressAbstract.*?'
        r'ReportShutdownProgress',
        "shared SDK does not expose the temporary shutdown progress service")
    require(
        mainwnd3,
        r'CShutdownProgressService.*?'
        r'RegisterService\(\s*SALAMANDER_SERVICE_SHUTDOWN_PROGRESS.*?'
        r'ssdpSavingConfiguration.*?'
        r'SaveConfig\(closingProgress\.HWindow, ordinaryClose\).*?'
        r'Plugins\.UnloadAll\(closingProgress\.HWindow,\s*'
        r'&shutdownProgressService\).*?ssdpClosingPanels.*?'
        r'ssdpFinishingShutdown.*?UnregisterService\(\s*'
        r'SALAMANDER_SERVICE_SHUTDOWN_PROGRESS',
        "application shutdown does not report its real phases through one progress service")
    require(
        plugins1,
        r'SupportLoadSave.*?!UnloadingPluginsForMainWindowClose',
        "shutdown still repeats every plug-in configuration save after the complete live SaveConfig pass")
    require(
        plugins2,
        r'CPlugins::UnloadAll\(.*?ReportShutdownProgress\(\s*'
        r'ssdpUnloadingPlugins, Data\[i\]->Name',
        "plug-in unload progress does not identify each loaded plug-in")
    for phase in (
            "ssdpUnregisteringToolbarButtons",
            "ssdpUnregisteringExtensions",
            "ssdpStoppingExtensionRuntimes",
            "ssdpClosingExtensionWindows"):
        require(
            packages, phase,
            f"Salamatrix shutdown does not report {phase}")
    require(
        packages,
        r'query\.ServiceId = SALAMANDER_SERVICE_SHUTDOWN_PROGRESS.*?'
        r'QueryService\(&query, &result\).*?ReportShutdownProgress',
        "Salamatrix package manager does not use the temporary shutdown progress service")
    for resource_id in range(14330, 14337):
        occurrences = len(re.findall(
            rf"^{resource_id},1,\"[^\"]+\"$",
            shutdown_translations,
            re.MULTILINE))
        if occurrences != 10:
            raise AssertionError(
                f"extension/shutdown status {resource_id} is not localized in all 10 translations")
    require(
        salamatrix_version,
        r'#define VERSINFO_MAJOR\s+0.*?'
        r'#define VERSINFO_MINORA\s+7.*?'
        r'#define VERSINFO_MINORB\s+10',
        "Salamatrix version was not bumped for modal dialog lifetime safety")
    require(
        automation_salamatrix,
        r'ApplyPositions\(BOOL delayedPaint\).*?'
        r'SetPositions\(m_nPos, m_nTotalPos, delayedPaint\).*?'
        r'AddText\(static_cast<const char\*>\(textA\), TRUE\).*?'
        r'AddText is cached.*?ApplyPositions\(FALSE\).*?Step\(step, FALSE\)',
        "Automation Salamatrix progress does not coalesce text and position into one repaint")
    require(
        automation_version,
        r'#define VERSINFO_MAJOR\s+2.*?'
        r'#define VERSINFO_MINORA\s+8.*?'
        r'#define VERSINFO_MINORB\s+0',
        "Automation version was not bumped for cached Salamatrix services")
    require(
        automation_bridge,
        r'void CAutomationSalamatrixBridge::Refresh.*?'
        r'QueryService\(salamander, SALAMATRIX_SERVICE_AUTOMATION_ADAPTER.*?'
        r'if \(m_bQueried && m_pGeneral == salamander.*?'
        r'm_pRuntimeService == runtimeService.*?return;.*?'
        r'Reset\(\);.*?RegisterRuntimeAdapters\(\);',
        "Automation rebuilds unchanged Salamatrix services and runtime adapters on every API getter")
    require(
        plugins2,
        r"Extension Bar Hidden.*?GetExtensionBarVisible.*?"
        r"SetExtensionBarVisible",
        "per-extension Extension Bar visibility is not persisted")
    require(
        plugins2,
        r"CompositeExtensionToolbarBitmap.*?"
        r"ImageList_GetBkColor\(hotImageList\).*?"
        r"CompositeExtensionToolbarBitmap\(hotBitmap, hotBackground\).*?"
        r"ImageList_Add\(hotImageList",
        "Extension Bar SVG alpha is not flattened onto its light/dark background")
    require(python_demo, r"Salamander\.ui\.notify", "Python demo does not show a non-blocking result")
    require(
        python_demo,
        r'if handler == "run":.*?CPython extension package is running',
        "Python demo executes its Run UI during lifecycle activation")
    require(python_demo, r'handler == "viewDemo".*?message_box.*?SystemExit',
            "Python Viewer demo does not isolate its modal preview from ordinary commands")
    require(powershell_demo, r"\$Salamander\.ui\.Notify", "PowerShell demo does not show a non-blocking result")
    require(powershell_demo, r"command_handler -eq 'viewDemo'.*?MessageBox.*?return",
            "PowerShell Viewer demo does not isolate its modal preview from ordinary commands")
    require(
        navigator,
        r"source_side\.Context\(\)",
        "Git Worktree Navigator does not use the source-panel context")
    require(
        navigator,
        r"worktree.*?list.*?--porcelain",
        "Git Worktree Navigator does not derive worktrees from the source panel")
    require(
        navigator,
        r"Open-NavigatorTab.*?source_side.*?"
        r"Open-NavigatorTab.*?target_side",
        "Git Worktree Navigator does not integrate with both Salamander sides")
    require(
        navigator,
        r"status.*?--porcelain=v1.*?cannotRemoveDirty.*?worktree.*?remove",
        "Git Worktree Navigator removal is not guarded by a clean status check")
    require_absent(
        navigator, r"worktree.*?remove.*?--force",
        "Git Worktree Navigator must not force-remove worktrees")
    require(
        navigator,
        r"for-each-ref.*?refs/heads.*?refs/remotes.*?"
        r"Switch-NavigatorBranch.*?'switch'.*?"
        r"Invoke-NavigatorFetch.*?'fetch'.*?"
        r"Invoke-NavigatorPull.*?'pull'.*?'--ff-only'.*?"
        r"Invoke-NavigatorPush.*?'push'",
        "Git Worktree Navigator branch and remote operations are incomplete")
    require(
        navigator,
        r"Invoke-NavigatorCommit.*?'add'.*?'--all'.*?"
        r"'diff'.*?'--cached'.*?'commit'.*?'-m'",
        "Git Worktree Navigator commit flow is incomplete")
    require(
        navigator,
        r"SetPreferredAppModeDelegate.*?"
        r"AllowDarkModeForWindowDelegate.*?"
        r"Set-ExtensionDarkMode.*?"
        r"UseVisualStyleBackColor\s*=\s*\$false.*?"
        r"FlatStyle\s*=\s*'Flat'.*?"
        r"MouseOverBackColor.*?MouseDownBackColor.*?"
        r"EnableHeadersVisualStyles\s*=\s*\$false.*?"
        r"DwmSetWindowAttribute.*?"
        r"application\.Appearance\(\).*?windowsDarkMode",
        "Git Worktree Navigator does not follow Salamander's explicit Windows dark scheme")
    require(
        navigator,
        r"application\.Appearance\(\).*?EnableImmersiveDarkMode\(\).*?"
        r"Application\]::EnableVisualStyles\(\)",
        "Git Worktree Navigator enables WinForms visual styles before its dark-mode process opt-in")
    require(
        navigator,
        r"\$theme\s*=\s*'Explorer'.*?"
        r"\$theme\s*=\s*'DarkMode_Explorer'.*?"
        r"SetWindowTheme\(.*?\$theme.*?"
        r"SendMessage\(.*?0x031A",
        "Git Worktree Navigator does not refresh controls with class-appropriate visual themes")
    require_absent(
        navigator,
        r"FolderBrowserDialog|System\.Windows\.Forms\.MessageBox",
        "Git Worktree Navigator still opens an unthemed WinForms system dialog")
    if any(command.get("requiresExecutable") != "git.exe"
           for command in navigator_manifest.get("commands", [])):
        raise AssertionError(
            "Git Worktree Navigator commands are not gated by git.exe")
    require(
        manifest + packages,
        r"requiresExecutable.*?RequiresExecutable.*?"
        r"SearchPathW.*?command\.Enabled = false",
        "manifest executable requirements do not disable unavailable commands")
    require(
        packages,
        r"GetMenuItemState.*?Commands\[c\]\.Enabled.*?"
        r"button\.Enabled = command\.Enabled",
        "unavailable extension commands are not disabled in menus and toolbars")
    require(
        general_contract + plugins2 + toolbar8,
        r"BOOL Enabled.*?button->Enabled.*?"
        r"TLBI_STATE_GRAYED",
        "Extension Bar does not propagate disabled command state")
    require(
        general_contract + plugins2,
        r"CSalamanderToolbarButton.*?MenuItems.*?MenuItemCount.*?"
        r"offsetof\(\s*CSalamanderToolbarButton,\s*MenuItemCount\).*?"
        r"button->StructSize >= menuEnd",
        "Extension Bar popup fields are not appended with an action-button ABI fallback")
    command_ids = {
        command.get("id") for command in navigator_manifest.get("commands", [])
    }
    if "OpenSalamander.GitWorktreeNavigator.commit" not in command_ids:
        raise AssertionError(
            "Git Worktree Navigator does not expose Commit in the extension menu")
    if ("OpenSalamander.GitWorktreeNavigator.createLocalRepository"
            not in command_ids):
        raise AssertionError(
            "Git Worktree Navigator does not expose local repository creation")
    navigator_toolbar_commands = [
        command for command in navigator_manifest.get("commands", [])
        if command.get("toolbar")
    ]
    if (len(navigator_toolbar_commands) != 1 or
            not navigator_toolbar_commands[0].get("toolbarMenu")):
        raise AssertionError(
            "Git Worktree Navigator toolbar button does not open its package menu")
    navigator_menu_commands = [
        command for command in navigator_manifest.get("commands", [])
        if command.get("menu") in {"plugin", "both"}
    ]
    normal_icons = [command.get("icon") for command in navigator_menu_commands]
    dark_icons = [command.get("iconDark") for command in navigator_menu_commands]
    if (not all(normal_icons) or not all(dark_icons) or
            len(set(normal_icons)) != len(normal_icons) or
            len(set(dark_icons)) != len(dark_icons)):
        raise AssertionError(
            "Git Worktree Navigator menu commands do not declare distinct light/dark icons")
    for relative_icon in normal_icons + dark_icons:
        if not (ROOT / "src/extensions/git-worktree-navigator" /
                relative_icon).is_file():
            raise AssertionError(
                f"Git Worktree Navigator menu icon is missing: {relative_icon}")
    require(
        navigator,
        r"Invoke-NavigatorCreateLocalRepository.*?"
        r"'rev-parse', '--show-toplevel'.*?"
        r"alreadyRepository.*?"
        r"Arguments @\('init'\).*?"
        r"repositoryCreated",
        "Git Worktree Navigator local repository creation is incomplete")
    require(
        powershell_worker,
        r"Payload\.PSObject\.Properties\['ok'\].*?"
        r"\$null\s+-ne\s+\$okProperty",
        "PowerShell runtime assumes every successful host result has an ok field")
    require_absent(
        powershell_worker,
        r"frame\.Payload\.ok\s+-eq\s+\$false",
        "PowerShell runtime still reads an optional ok field under StrictMode")
    expected_locales = {
        "en", "cs", "de", "es", "fr", "hu",
        "nl", "ro", "ru", "sk", "zh-CN",
    }
    if set(menu_builder_manifest.get("locales", {})) != expected_locales:
        raise AssertionError(
            "Extension Menu Builder does not declare every supported locale")
    builder_command_ids = {
        command.get("id")
        for command in menu_builder_manifest.get("commands", [])
    }
    builder_english_keys = None
    for locale, relative in menu_builder_manifest["locales"].items():
        localized = json.loads(read(
            "src/extensions/extension-menu-builder/" + relative))
        if set(localized.get("commands", {})) != builder_command_ids:
            raise AssertionError(
                f"menu builder command localization is incomplete: {locale}")
        localized_keys = set(localized.get("strings", {}))
        if builder_english_keys is None:
            builder_english_keys = localized_keys
        elif localized_keys != builder_english_keys:
            raise AssertionError(
                f"menu builder strings differ from English: {locale}")
    require(
        menu_builder,
        r"Save-BuilderProject.*?extension\.json.*?menu-builder\.json.*?"
        r"actions\.json.*?Get-GeneratedRuntimeScript",
        "Extension Menu Builder does not emit an editable runnable extension")
    require(
        menu_builder,
        r"iconDark.*?Copy-BuilderAsset.*?toolbarMenu.*?"
        r"Preview menu|preview",
        "Extension Menu Builder does not expose per-command icons and menu preview")
    require(
        menu_builder,
        r"focusedItem\.path.*?selectedItems.*?activePanel\.path.*?"
        r"targetPanel\.path",
        "generated custom menus do not expand Salamander panel tokens")
    require(
        setup,
        r"AddPlugin\('extensionmenubuilder',\s*'Extension Menu Builder'",
        "x64 installer does not offer Extension Menu Builder")
    require(
        setup,
        r"extensions\\extension-menu-builder.*?"
        r"IsPluginSelected\('extensionmenubuilder'\)",
        "x64 installer does not package Extension Menu Builder")
    require(
        setup,
        r"AddPluginDependency\('extensionmenubuilder',\s*"
        r"'powershellruntime'\)",
        "x64 installer does not select the PowerShell runtime for Extension Menu Builder")
    require(
        salamatrix_project,
        r"ExtensionMenuBuilderFiles.*?extension-menu-builder.*?"
        r"Copy SourceFiles=\"@\(ExtensionMenuBuilderFiles\)\".*?"
        r"extensions\\extension-menu-builder",
        "Salamatrix build does not stage Extension Menu Builder")
    if set(navigator_manifest.get("locales", {})) != expected_locales:
        raise AssertionError(
            "Git Worktree Navigator does not declare every supported locale")
    english_keys = None
    for locale, relative in navigator_manifest["locales"].items():
        localized = json.loads(read(
            "src/extensions/git-worktree-navigator/" + relative))
        if not localized.get("name") or not localized.get("commands"):
            raise AssertionError(f"navigator locale metadata is incomplete: {locale}")
        if set(localized["commands"]) != command_ids:
            raise AssertionError(
                f"navigator localized commands are incomplete: {locale}")
        keys = set(localized.get("strings", {}))
        if english_keys is None:
            english_keys = keys
        elif keys != english_keys:
            raise AssertionError(f"navigator runtime strings are incomplete: {locale}")
    if set(lock_inspector_manifest.get("locales", {})) != expected_locales:
        raise AssertionError(
            "File Lock Inspector does not declare every supported locale")
    inspector_english_keys = None
    for locale, relative in lock_inspector_manifest["locales"].items():
        localized = json.loads(read(
            "src/extensions/file-lock-inspector/" + relative))
        if not localized.get("name") or not localized.get("commands"):
            raise AssertionError(
                f"lock inspector locale metadata is incomplete: {locale}")
        keys = set(localized.get("strings", {}))
        if inspector_english_keys is None:
            inspector_english_keys = keys
        elif keys != inspector_english_keys:
            raise AssertionError(
                f"lock inspector runtime strings are incomplete: {locale}")
    require(
        lock_inspector,
        r"rstrtmgr\.dll.*?RmStartSession.*?RmRegisterResources.*?"
        r"RmGetList.*?RmEndSession",
        "File Lock Inspector does not use a complete Restart Manager session")
    require(
        lock_inspector,
        r"source_side\.Context\(\).*?selectedItems.*?focusedItem.*?"
        r"isDirectory.*?Show-InspectorWindow",
        "File Lock Inspector does not inspect the selected or focused files")
    if set(process_explorer_manifest.get("locales", {})) != expected_locales:
        raise AssertionError(
            "Process Explorer does not declare every supported locale")
    process_english_keys = None
    for locale, relative in process_explorer_manifest["locales"].items():
        localized = json.loads(read(
            "src/extensions/process-explorer/" + relative))
        file_system = localized.get("fileSystems", {}).get("processes", {})
        if (not localized.get("name") or not localized.get("description") or
                set(file_system.get("columns", {})) !=
                {"pid", "status", "userName", "cpu", "memory"} or
                set(file_system.get("actions", {})) !=
                {"endTask", "endProcessTree", "openFileLocation", "properties"} or
                set(localized.get("commands", {})) !=
                {"OpenSalamander.ProcessExplorer.open"}):
            raise AssertionError(
                f"process explorer locale metadata is incomplete: {locale}")
        keys = set(localized.get("strings", {}))
        if process_english_keys is None:
            process_english_keys = keys
        elif keys != process_english_keys:
            raise AssertionError(
                f"process explorer runtime strings are incomplete: {locale}")
    process_file_system = process_explorer_manifest["fileSystems"][0]
    if (process_file_system.get("defaultFileIcon") != "default.ico" or
            not (ROOT / "src/extensions/process-explorer/default.ico").is_file()):
        raise AssertionError(
            "Process Explorer does not declare its packaged default item icon")
    columns = process_file_system.get("columns", [])
    if [column.get("id") for column in columns] != [
            "pid", "status", "cpu", "memory", "userName"] or not all(
                columns[index].get("numeric") for index in (0, 2, 3)):
        raise AssertionError("Process Explorer detailed columns are incomplete")
    actions = process_explorer_manifest["fileSystems"][0].get("actions", [])
    if ([action.get("id") for action in actions if not action.get("separator")] !=
            ["endTask", "endProcessTree", "openFileLocation", "properties"] or
            not actions[2].get("separator") or
            actions[3].get("refresh", True) or actions[4].get("refresh", True)):
        raise AssertionError("Process Explorer context actions are incomplete")
    commands = process_explorer_manifest.get("commands", [])
    if (len(commands) != 1 or not commands[0].get("toolbar") or
            commands[0].get("menu") != "none" or
            commands[0].get("path") !=
            "salamatrix:OpenSalamander.ProcessExplorer!processes" or
            commands[0].get("handler")):
        raise AssertionError("Process Explorer toolbar command is incomplete")
    require(
        process_explorer,
        r"Get-Process -IncludeUserName.*?MainModule.*?ModuleName.*?"
        r"FileName.*?GetExecutablePath\(.*?\$process\.Id.*?"
        r"Test-ProcessSuspended.*?UserName.*?"
        r"GetPrivateWorkingSet.*?compactName=.*?columns=@\{.*?pid=.*?"
        r"status=.*?userName=.*?cpu=.*?memory=.*?fileIcon.*?"
        r"knownExecutablePaths.*?minimumSampleSeconds.*?ProcessorCount.*?"
        r"TotalProcessorTime",
        "Process Explorer does not publish Task Manager fields and executable icons")
    require(
        process_explorer,
        r"GetPrivateWorkingSet.*?\[uint64\]\$privateWorkingSet\)\.ToString\(.*?"
        r"InvariantCulture.*?memory=\$memoryText",
        "Process Explorer does not publish its memory value as raw bytes")
    if not columns[3].get("size"):
        raise AssertionError(
            "Process Explorer memory column does not declare byte-size semantics")
    require(
        manifest + packages,
        r'ReadBoolean\(columnValue, "size".*?column\.Numeric = true.*?'
        r'SALCFG_SIZEFORMAT.*?PrintDiskSize.*?Columns\[index\]\.Size',
        "Salamatrix size columns do not follow the user's panel size format")
    require(
        process_explorer,
        r"List\[hashtable\].*?\$item = @\{.*?\.Add\(\$item\).*?"
        r"file_system\.AddItems",
        "Process Explorer does not publish its snapshot in one batch")
    require(
        process_explorer,
        r"QueryFullProcessImageName.*?CreateToolhelp32Snapshot.*?"
        r"TerminateProcess.*?ShellExecuteEx.*?ui\.FileProperties",
        "Process Explorer process actions are not functional")
    require(
        packages,
        r"salamander\.ui\.fileProperties.*?SHObjectProperties.*?"
        r"SHOP_FILEPATH",
        "File Properties is not hosted by Salamander through SHObjectProperties")
    for worker_source, marker in (
            (powershell_worker, "FileProperties"),
            (python_worker, "file_properties"),
            (javascript_worker, "fileProperties"),
            (php_worker, "fileProperties"),
            (lua_worker, "file_properties")):
        if marker not in worker_source or "salamander.ui.fileProperties" not in worker_source:
            raise AssertionError("File Properties runtime facade parity is incomplete")
    require(
        packages,
        r"command\.Path.*?ManifestAllowsCapability.*?panels\.write.*?"
        r"ChangePanelPath.*?PANEL_SOURCE",
        "Declarative toolbar path does not navigate the active panel natively")
    require(
        plugins1 + plugins2,
        r"ExecuteToolbarCommand.*?PluginIfaceForMenuExt\.ExecuteMenuItem.*?"
        r"ExecuteMenuItem2.*?plugin->ExecuteToolbarCommand",
        "Registered Extension Bar commands still depend on plugin-menu visibility")
    require(
        packages,
        r"PrivateExtractIconsW.*?HasSimplePluginIcon.*?FileIcon.*?"
        r"SalamatrixExtractFileIcon",
        "Salamatrix FS items do not resolve native file icons")
    require(
        manifest + packages,
        r"defaultFileIcon.*?DefaultFileIcon.*?"
        r"SalamatrixExtractFileIcon\(DefaultFileIcon",
        "Salamatrix FS does not apply its manifest default file icon fallback")
    require(
        packages,
        r"CompareFilesFromFS.*?Item\.Id.*?strcmp",
        "Salamatrix FS icon-cache comparator does not use stable item identity")
    icon_compare = re.search(
        r"virtual int WINAPI CompareFilesFromFS\(.*?"
        r"(?=\n    virtual void WINAPI SetupView\()",
        packages, re.MULTILINE | re.DOTALL)
    if (icon_compare is None or
            "SalamatrixFsTransferActCustomData" in icon_compare.group(0)):
        raise AssertionError(
            "Salamatrix FS icon-cache ordering still changes with the active sort column")
    require(
        packages,
        r"SalamatrixFileSystemNameText.*?CompactName.*?COLUMN_ID_CUSTOM",
        "Salamatrix FS compact names are not selected by panel view mode")
    require(
        fileswn4,
        r"nameColumn->ID == COLUMN_ID_CUSTOM.*?nameColumn->GetText",
        "Salamatrix FS compact names are not selected by panel view mode")
    require(
        filesmap,
        r"nameColumn->ID == COLUMN_ID_CUSTOM.*?nameColumn->GetText.*?"
        r"GetTextExtentPoint32\(dc, s, len",
        "Salamatrix FS item widths ignore the displayed compact name")
    require(
        filesbx2,
        r"index == 0.*?column->ID == COLUMN_ID_NAME.*?panel->SortType == stName.*?"
        r"column->ID == COLUMN_ID_CUSTOM.*?panel->SortType == stCustom.*?"
        r"index == 0.*?st = stName.*?column->ID == COLUMN_ID_CUSTOM\).*?"
        r"ChangeCustomSortType",
        "Plugin custom columns do not select or display their active sort order")
    require(
        fileswn3,
        r"FillPluginCustomSortCache.*?GetText.*?SortType == stCustom.*?"
        r"Is\(ptPluginFS\).*?SortPluginCustomNameExtAux",
        "Plugin custom columns are not sorted through their text callbacks")
    require(
        fileswn3,
        r"ParsePluginCustomNumber.*?strtod.*?KiB.*?multiplier",
        "Numeric plugin custom columns ignore numeric values or memory units")
    if "SHGetFileInfoW" in packages:
        raise AssertionError(
            "Salamatrix file icons still depend on extension associations")
    require(
        packages,
        r"FindStringMember\(itemJson, \"fileIcon\", &item\.FileIcon\)",
        "Salamatrix FS item parser drops the native file icon path")
    require(
        packages,
        r"if \(currentPath.*?CachedItems\.clear\(\).*?"
        r"if \(succeeded\).*?CacheReady = TRUE",
        "Failed background listings can leave the FS in an infinite retry loop")
    require(
        packages,
        r"action\.Separator.*?AppendMenuW\(menu, MF_SEPARATOR.*?"
        r"AppendMenuW\(menu, MF_STRING.*?action\.Refresh",
        "Salamatrix FS context menu does not support localized separators and refresh policy")
    require(
        packages,
        r"FSE_PATHCHANGED.*?CacheReady.*?StartThrobber",
        "Salamatrix FS does not show panel loading activity for an empty cache")
    require(
        packages,
        r"SalamatrixFileSystemColumnText.*?InsertColumn",
        "Salamatrix FS does not validate and render custom item columns")
    require(
        packages,
        r"ListingFileSystem.*?FindRawMember\(itemJson, \"columns\"",
        "Salamatrix FS does not validate custom item column payloads")
    require(
        packages,
        r"class PackageManager::OpenFileSystem.*?RefreshThreadProc.*?"
        r"RefreshInBackground.*?StartBackgroundRefresh.*?"
        r"ListCurrentPath.*?CachedItems.*?StartBackgroundRefresh",
        "Salamatrix FS listing is not cached and executed asynchronously")
    require(
        packages,
        r"virtual ~OpenFileSystem\(\).*?ShuttingDown.*?"
        r"RefreshThreadStarting.*?SwitchToThread.*?"
        r"RefreshThreadStopping.*?"
        r"WaitForThreadWithSentMessageDispatch.*?DeleteCriticalSection",
        "Salamatrix FS close can free the cache while a refresh thread handle is being published")
    require(
        packages,
        r"StartBackgroundRefresh.*?RefreshThreadStarting.*?"
        r"InterlockedExchangePointer.*?ShuttingDown.*?CREATE_SUSPENDED.*?"
        r"InterlockedExchangePointer.*?RefreshThreadRunningState.*?ResumeThread",
        "Salamatrix FS refresh worker can start before its join handle is published")
    require(
        packages,
        r"virtual ~OpenFileSystem\(\).*?"
        r"CancelFileSystemListingForShutdown\(RefreshPackageId\).*?"
        r"WaitForThreadWithSentMessageDispatch.*?"
        r"SALAMANDER_SERVICE_SHUTDOWN_PROGRESS.*?"
        r"package->Session->Stop\(\)",
        "shutdown can wait for an extension-FS listing before cancelling its runtime call")
    listing_body = re.search(
        r"virtual BOOL WINAPI ListCurrentPath\(.*?"
        r"(?=\n    virtual BOOL WINAPI TryCloseOrDetach)",
        packages, re.MULTILINE | re.DOTALL)
    if listing_body is None or "Owner->ListFileSystem" in listing_body.group(0):
        raise AssertionError(
            "Salamatrix FS still executes runtime listing synchronously on the UI path")
    require(
        setup,
        r"extensions\\process-explorer.*?IsPluginSelected\('processexplorer'\)",
        "x64 installer does not package Process Explorer")
    require(
        setup,
        r"AddPluginDependency\('processexplorer',\s*'powershellruntime'\)",
        "x64 installer does not select the Process Explorer runtime")
    require(
        setup,
        r"AddPlugin\('processexplorer',\s*'Process Explorer'",
        "x64 installer does not offer Process Explorer")
    require(
        salamatrix_project,
        r"ProcessExplorerFiles.*?process-explorer.*?"
        r"Copy SourceFiles=\"@\(ProcessExplorerFiles\)\".*?"
        r"extensions\\process-explorer",
        "Salamatrix build does not stage Process Explorer")
    if "panels.write" not in hardware_monitor_manifest.get("capabilities", []):
        raise AssertionError(
            "Hardware Monitor toolbar path command cannot change the active panel")
    if hardware_monitor_manifest["fileSystems"][0].get("openHandler"):
        raise AssertionError(
            "Hardware Monitor incorrectly uses an action handler to list a directory")
    require(
        hardware_monitor,
        r"invocation\.path.*?categoryId.*?viewId.*?file_system\.AddItems",
        "Hardware Monitor does not list category contents from its FS path")
    if "param([hashtable]$Strings)" in hardware_monitor:
        raise AssertionError(
            "Hardware Monitor rejects ConvertFrom-Json localization objects")
    require(
        hardware_monitor,
        r"Win32_PhysicalMemoryArray.*?MemoryDevices.*?"
        r"mem-slots-used.*?mem-slots-free.*?ConfiguredClockSpeed",
        "Hardware Monitor does not expose occupied and free RAM slots")
    require(
        hardware_monitor,
        r"pagefile-\$i.*?slotName.*?-replace '\[\\\\/\]'.*?"
        r"'cpu'.*?viewId -eq 'usage'.*?Get-CpuUsageInfo.*?"
        r"'memory'.*?viewId -eq 'usage'.*?Get-PhysicalMemoryInfo",
        "Hardware Monitor does not expose safe RAM slots and nested usage views")
    require(
        packages,
        r"opened->GetPath\(\).*?data->Item\.Id.*?ChangePanelPathToPluginFS",
        "Salamatrix FS does not navigate into extension-provided directories")
    require(
        packages,
        r"GetSupportedServices\(\).*?FS_SERVICE_VIEWFILE.*?ViewFile\(.*?ExecuteDefault",
        "Salamatrix FS leaf items do not advertise and dispatch their default action")
    require(
        packages,
        r"Invocation\(.*?parentWindow.*?GetMainWindowHWND\(\)",
        "Salamatrix FS actions do not receive their invoking parent window")
    require(
        packages,
        r"ScopedExclusiveSRWLock\(SRWLOCK\* lock, HWND mainWindow\).*?"
        r"MsgWaitForMultipleObjects\(.*?QS_SENDMESSAGE.*?PeekMessage.*?"
        r"ExecuteFileSystemAction\(.*?GetMainWindowHWND\(\)",
        "Salamatrix silently drops FS actions while a background listing is active")
    require(
        packages,
        r"isDir == 2.*?GetParentPath\(\).*?ChangePanelPathToPluginFS",
        "Salamatrix FS parent item skips directly to the global root")
    require(
        packages,
        r"if \(forceRefresh\).*?RefreshRequested.*?"
        r"FSE_ACTIVATEREFRESH \|\|.*?FSE_TIMER.*?ShouldRefreshPeriodically.*?"
        r"RequestDataRefresh",
        "Salamatrix FS does not limit timer refreshes by virtual path depth")
    require(
        packages,
        r"GetPanelItem\(\s*panel, &enumeration, &isDir\).*?panelItemIndex.*?"
        r"panelItemIds.*?AppendPanelNavigation\(&invocation, panel, data\)",
        "Salamatrix FS actions do not receive the current panel order")
    require(
        packages,
        r"maxNavigationItems = 64.*?firstItem.*?lastItem.*?"
        r"for \(size_t index = firstItem; index < lastItem; \+\+index\)",
        "Salamatrix passes an unbounded panel order on the worker command line")
    require(
        packages,
        r"if \(executed && selected != NULL && selected->Refresh\).*?"
        r"opened->ExecuteDefault\(file, panel\)",
        "Salamatrix FS default actions do not respect refresh=false")
    if hardware_monitor_manifest["fileSystems"][0].get("refreshDepth") != 2:
        raise AssertionError(
            "Hardware Monitor refreshes static root or category paths")
    if hardware_monitor_manifest["fileSystems"][0].get("refreshPaths") != [
            "cpu/usage", "memory/usage", "sensors/temperatures",
            "sensors/fans", "sensors/voltages", "sensors/all"]:
        raise AssertionError(
            "Hardware Monitor does not limit timer refreshes to dynamic views")
    require(
        packages,
        r"PeriodicRefreshPaths\.empty\(\).*?relativePath.*?"
        r"PeriodicRefreshPaths\[index\]",
        "Salamatrix ignores file-system refreshPaths")
    root_items = hardware_monitor_manifest["fileSystems"][0].get("rootItems", [])
    if [item.get("id") for item in root_items] != [
            "cpu", "gpu", "memory", "motherboard", "network", "sensors",
            "storage", "device-manager"]:
        raise AssertionError(
            "Hardware Monitor root categories are not declared for synchronous listing")
    expected_device_actions = {
        "updateDriver", "disableDevice", "uninstallDevice", "scanDevices",
        "deviceProperties"}
    for language, relative in hardware_monitor_manifest.get("locales", {}).items():
        localized = json.loads(read(
            "src/extensions/hardware-monitor/" + relative))
        localized_fs = localized.get("fileSystems", {}).get("hardware-info", {})
        if not localized_fs.get("rootItems", {}).get("device-manager"):
            raise AssertionError(
                f"Hardware Monitor {language} locale omits Device Manager")
        if set(localized_fs.get("actions", {})) != expected_device_actions:
            raise AssertionError(
                f"Hardware Monitor {language} locale has incomplete device actions")
        if not localized.get("categories", {}).get("deviceManager"):
            raise AssertionError(
                f"Hardware Monitor {language} locale omits the device category")
        localized_strings = localized.get("strings", {})
        if not all(localized_strings.get(key) for key in (
                "disableDevice", "uninstallDevice", "confirmDeviceAction")):
            raise AssertionError(
                f"Hardware Monitor {language} locale has incomplete confirmations")
    require(
        packages,
        r"Path\.c_str\(\), provider\.c_str\(\).*?rootItems\.empty.*?"
        r"AddItem\(dir, pluginData, item",
        "Salamatrix FS does not synchronously list manifest rootItems")
    require(
        packages,
        r"Invocation\(\"list\".*?RefreshPath\.c_str\(\)",
        "Salamatrix FS does not pass the current virtual path to list handlers")
    if not all(marker in hardware_monitor for marker in (
            "Get-CimInstance Win32_PnPEntity", "PNPClass", "PNPDeviceID",
            "'device-manager'", "deviceProperties", "DeviceProperties_RunDLL",
            "LaunchDeviceProperties", "DevicePropertiesRunDll",
            "invocation.parentWindow",
            "updateDriver", "DiShowUpdateDevice", "disableDevice",
            "/disable-device", "uninstallDevice", "/remove-device",
            "scanDevices", "/scan-devices")):
        raise AssertionError(
            "Hardware Monitor Device Manager hierarchy and actions are incomplete")
    require_absent(
        hardware_monitor,
        r"WaitForInputIdle|MainWindowHandle|SetWindowPos|rundll32\.exe",
        "Hardware Monitor still polls and repositions Device Properties")
    require(
        hardware_monitor,
        r"viewId\.StartsWith\('device-class-'\).*?Substring.*?"
        r"Get-DeviceManagerItems \$deviceClass",
        "Hardware Monitor does not decode a selected PnP class before listing devices")
    require(
        manifest + packages + json.dumps(hardware_monitor_manifest),
        r"itemIdPrefix.*?ItemIdPrefix.*?data->Item\.Id\.compare",
        "Device Manager actions are not scoped to matching FS items")
    event_fs = event_viewer_manifest["fileSystems"][0]
    if event_fs.get("refreshIntervalMs") != 0:
        raise AssertionError(
            "Event Viewer still queues background listings behind its modal dialog")
    if [item.get("id") for item in event_fs.get("rootItems", [])] != [
            "custom-views", "windows-logs", "applications-services"]:
        raise AssertionError("Event Viewer root log hierarchy is incomplete")
    if not all(marker in event_viewer for marker in (
            "Get-WinEvent -ListLog", "IsClassicLog", "Get-WinEvent -LogName",
            "-MaxEvents 250", "EventRecordID=$recordId", "Show-EventDetails",
            "ToXml()", "$Salamander.ui.Dialog", "'textbox'",
            "previousEvent", "nextEvent", "dialog.Show()", "$true)",
            "panelItemIds", "panelItemIndex", "$nextIndex",
            "-MaxEvents 1")):
        raise AssertionError(
            "Event Viewer does not expose log hierarchy, events, and native details")
    require_absent(
        event_viewer,
        r"function Get-AdjacentEvent|EventRecordID [<>] \$RecordId",
        "Event Properties navigation ignores the current panel sort order")
    require_absent(
        event_viewer,
        r"Show-EventDetails.*?Get-WinEvent -LogName \$LogName -MaxEvents 500",
        "Event Properties still blocks its initial display on a 500-event query")
    require(
        event_viewer,
        r"AddControl\('label', 'metadata'.*?width=350;height=70",
        "Event Properties metadata is not rendered as a bounded multiline label")
    runtime_workers = [
        read("src/plugins/javascriptruntime/runtime/salamatrix_worker.mjs"),
        read("src/plugins/pythonruntime/runtime/salamatrix_worker.py"),
        read("src/plugins/powershellruntime/runtime/salamatrix_worker.ps1"),
        read("src/plugins/phpruntime/runtime/salamatrix_worker.php"),
        read("src/plugins/luaruntime/runtime/salamatrix_worker.lua")]
    if not all("resizable" in worker.lower() for worker in runtime_workers):
        raise AssertionError(
            "Resizable native dialogs are not exposed by every runtime provider")
    require(
        packages,
        r'FindBoolMember\(\s*payloadJson, "resizable", &options\.Resizable\)',
        "Salamatrix host ignores the runtime dialog resizable option")
    require(
        packages,
        r'dialog\.destroy.*?DestroyDialog\(dialog\).*?'
        r'InterlockedExchange\(&package->FileSystemActionPending, FALSE\)',
        "Closing a modal FS action still waits for worker teardown before reopening")
    for demo_manifest in (
            javascript_demo_manifest, python_demo_manifest,
            lua_demo_manifest, php_demo_manifest, powershell_demo_manifest):
        if demo_manifest["fileSystems"][0].get("refreshIntervalMs") != 0:
            raise AssertionError(
                "A demo FS still refreshes continuously without a data change")
    if "resizable=false" not in read(
            "src/plugins/salamatrix/salamatrix_ai.h"):
        raise AssertionError(
            "Model-visible UI contract omits the resizable dialog option")
    for language, relative in event_viewer_manifest.get("locales", {}).items():
        localized = json.loads(read("src/extensions/event-viewer/" + relative))
        if not localized.get("descriptionLabel"):
            raise AssertionError(
                f"Event Viewer {language} locale omits the description label")
    require(
        event_viewer,
        r"subPath = @\(\).*?parts\.Count -gt 2.*?"
        r"subPath\[0\].*?-replace '\^log-'.*?Get-WinEvent -LogName \$logName.*?"
        r"decodedPath.*?-replace '\^log-node-'.*?decodedPath -join '/'",
        "Event Viewer does not decode nested log ids before querying Windows")
    require(
        setup,
        r"extensions\\event-viewer.*?IsPluginSelected\('eventviewer'\).*?"
        r"AddPluginDependency\('eventviewer',\s*'powershellruntime'\).*?"
        r"AddPlugin\('eventviewer',\s*'Event Viewer'",
        "x64 installer does not package Event Viewer with its runtime")
    require(
        salamatrix_project,
        r"EventViewerFiles.*?event-viewer.*?"
        r"Copy SourceFiles=\"@\(EventViewerFiles\)\".*?extensions\\event-viewer",
        "Salamatrix build does not stage Event Viewer")
    require(
        read("src/plugins/salamatrix/salamatrix_poc.h"),
        r"CreatePocRuntimeServices.*?new \(std::nothrow\) Runtime::RuntimeServices.*?"
        r"RunAllPoc.*?Runtime::RuntimeServices\* services",
        "Salamatrix PoC still places the multi-megabyte RuntimeServices object on the stack")
    require(
        setup,
        r"extensions\\hardware-monitor.*?IsPluginSelected\('hardwaremonitor'\)",
        "x64 installer does not package Hardware Monitor")
    require(
        setup,
        r"AddPluginDependency\('hardwaremonitor',\s*'powershellruntime'\)",
        "x64 installer does not select the Hardware Monitor runtime")
    require(
        salamatrix_project,
        r"HardwareMonitorFiles.*?hardware-monitor.*?"
        r"Copy SourceFiles=\"@\(HardwareMonitorFiles\)\".*?"
        r"extensions\\hardware-monitor",
        "Salamatrix build does not stage Hardware Monitor")
    require(
        salamatrix_project,
        r"HardwareMonitorFiles.*?Exclude=.*?copy-sensor-dlls\.bat.*?"
        r"Delete Files=.*?copy-sensor-dlls\.bat",
        "Salamatrix build ships the Hardware Monitor developer copy helper")
    require(
        hardware_monitor,
        r"salamanderRoot.*?msvcp140\.dll.*?env:PATH.*?Add-Type.*?finally.*?"
        r"env:PATH = \$originalPath",
        "Hardware Monitor does not resolve its shared VC runtime from Salamander")
    require(
        hardware_monitor,
        r"GetAllSensorsPacked.*?FreePackedSensors.*?"
        r"Get-HardViewSensorInfo.*?ValidateSet\('Temperature', 'Fan', 'Voltage', 'All'\).*?"
        r"BitConverter\]::ToDouble.*?Temperature.*?RPM.*?Throughput.*?B/s",
        "Hardware Monitor does not enumerate and format all HardView sensors")
    require(
        hardware_monitor,
        r"viewId -eq 'temperatures'.*?Get-HardViewSensorInfo.*?'Temperature'.*?"
        r"viewId -eq 'fans'.*?Get-HardViewSensorInfo.*?'Fan'.*?"
        r"viewId -eq 'voltages'.*?Get-HardViewSensorInfo.*?'Voltage'.*?"
        r"viewId -eq 'all'.*?Get-HardViewSensorInfo.*?'All'.*?"
        r"id='temperatures'.*?id='fans'.*?id='voltages'.*?id='all'",
        "Hardware Monitor does not expose focused and complete sensor views")
    require(
        hardware_monitor,
        r"Get-SmartStorageInfo.*?DiskInfoToolkit\.StorageManager.*?"
        r"ReloadStorages.*?SmartAttributes.*?RawValueULong.*?"
        r"Get-PhysicalDisk.*?Get-StorageReliabilityCounter.*?"
        r"HealthStatus.*?PowerOnHours.*?"
        r"components.Count -gt 3.*?detailId.*?"
        r"viewId -eq 'smart'.*?Get-SmartStorageInfo \$strings \$detailId",
        "Hardware Monitor does not expose read-only SMART and NVMe information")
    require(
        hardware_monitor,
        r"Get-SmartStorageCacheView.*?directory=\$true.*?"
        r"Where-Object \{ \$_\.id -eq \$DiskId \}",
        "Hardware Monitor does not group SMART information by physical disk")
    require(
        hardware_monitor,
        r"Get-HidDeviceInfo.*?HidSharp\.DeviceList.*?GetHidDevices.*?"
        r"VendorID.*?ProductID.*?viewId -eq 'hid'",
        "Hardware Monitor does not expose installed HID device information")
    require(
        hardware_monitor,
        r"Get-SmbiosMemoryInfo.*?LibreHardwareMonitor\.Hardware\.SMBios.*?"
        r"MemoryDevices.*?ConfiguredVoltage.*?viewId -eq 'spd'",
        "Hardware Monitor does not expose safe SPD-like SMBIOS module information")
    require(
        hardware_monitor,
        r"ConvertTo-SafeHardwareItemName.*?Replace\('\\', ' - '\).*?"
        r"Replace\('/', ' - '\).*?subItem\.name = ConvertTo-SafeHardwareItemName",
        "Hardware Monitor does not sanitize FS item names before atomic AddItems")
    for redundant_library in (
            "HardwareWrapper.deps.json", "HardwareWrapper.runtimeconfig.json",
            "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll",
            "hostpolicy.dll"):
        require_absent(
            hardware_wrapper_project,
            rf'<Copy SourceFiles=.*?{re.escape(redundant_library)}',
            f"Hardware Monitor still stages redundant {redundant_library}")
        require(
            hardware_wrapper_project,
            rf'RedundantHardwareMonitorFile Include=.*?{re.escape(redundant_library)}',
            f"Hardware Monitor does not clean stale {redundant_library}")
    require(
        lock_inspector,
        r"CloseMainWindow.*?confirmEnd.*?\.Kill\(\)",
        "File Lock Inspector does not separate graceful close from forced termination")
    require(
        lock_inspector,
        r"target_side\.CreateTab",
        "File Lock Inspector cannot open the process location")
    require(
        lock_inspector,
        r"clipboard\.CopyText",
        "File Lock Inspector cannot copy its report")
    inspector_commands = lock_inspector_manifest.get("commands", [])
    if not inspector_commands or not inspector_commands[0].get("toolbar"):
        raise AssertionError(
            "File Lock Inspector is not available in Extension Bar")
    require(
        lock_inspector,
        r"SetPreferredAppModeDelegate.*?"
        r"AllowDarkModeForWindowDelegate.*?"
        r"Set-ExtensionDarkMode.*?"
        r"UseVisualStyleBackColor\s*=\s*\$false.*?"
        r"FlatStyle\s*=\s*'Flat'.*?"
        r"MouseOverBackColor.*?MouseDownBackColor.*?"
        r"EnableHeadersVisualStyles\s*=\s*\$false.*?"
        r"DwmSetWindowAttribute.*?"
        r"application\.Appearance\(\).*?windowsDarkMode",
        "File Lock Inspector does not follow Salamander's explicit Windows dark scheme")
    require(
        lock_inspector,
        r"application\.Appearance\(\).*?EnableImmersiveDarkMode\(\).*?"
        r"Application\]::EnableVisualStyles\(\)",
        "File Lock Inspector enables WinForms visual styles before its dark-mode process opt-in")
    require(
        lock_inspector,
        r"\$theme\s*=\s*'Explorer'.*?"
        r"\$theme\s*=\s*'DarkMode_Explorer'.*?"
        r"SetWindowTheme\(.*?\$theme.*?"
        r"SendMessage\(.*?0x031A",
        "File Lock Inspector does not refresh controls with class-appropriate visual themes")
    require_absent(
        lock_inspector,
        r"System\.Windows\.Forms\.MessageBox",
        "File Lock Inspector still opens an unthemed WinForms message box")
    require(
        lock_inspector,
        r"if \(\$paths\.Count -eq 0\).*?"
        r"ui\.MessageBox\(.*?selectFile.*?"
        r"Initialize-RestartManager",
        "File Lock Inspector does not explain unsupported panel selections")
    require(
        salamatrix,
        r"SalamanderLanguageID\s*=\s*salamander->GetCurrentSalamanderLanguageID",
        "framework does not capture Salamander's selected language")
    require(
        packages,
        r"CurrentSalamanderLocale.*?manifest\.Locales.*?ParseLocaleText",
        "framework package locales do not use Salamander's selected language")
    require(
        packages,
        r"packageMenuCommandCount\s*>\s*1.*?"
        r"Manifest\.Name.*?"
        r"AddSubmenuStart.*?"
        r"AddMenuItem.*?"
        r"AddSubmenuEnd",
        "multi-command extensions do not get a named extension submenu")
    require(
        packages,
        r'salamander\.host\.language.*?languageId.*?locale',
        "framework package host does not expose the selected Salamander language")
    require(
        packages,
        r'salamander\.host\.appearance.*?'
        r'DarkModeIsWindowsDarkSchemeSelected\(\).*?windowsDarkMode',
        "framework package host does not expose the explicit Windows dark scheme")
    require(
        panel_tooltips,
        r"Is\(ptPluginFS\).*?PluginData\.GetInfoLineContent\(.*?"
        r"AppendTipText\(text, textSize, info\).*?return;",
        "plugin file-system tooltips do not reuse item information-line details")
    require(
        panel_tooltips,
        r"ValidFileData\s*&\s*\(VALID_DATA_DATE\s*\|\s*VALID_DATA_TIME\).*?"
        r"FormatTipFileTime.*?ValidFileData\s*&\s*VALID_DATA_SIZE",
        "generic panel tooltips render invalid date or size metadata")
    require(
        packages,
        r"GetInfoLineContent\(.*?data->Item\.ColumnValues.*?"
        r"Columns\[index\]\.Name\.c_str\(\).*?value\.c_str\(\).*?"
        r"return buffer\[0\] != '\\0';",
        "Salamatrix FS tooltips do not expose declared item column values")
    require(
        powershell_worker,
        r'ScriptMethod Appearance.*?salamander\.host\.appearance',
        "PowerShell runtime does not expose Salamander appearance")
    require(
        packages,
        r'salamander\.host\.windowIcon.*?'
        r'DarkModeIsWindowsDarkSchemeSelected\(\).*?'
        r'CreateSVGIcon\(preferredPath, 32\).*?'
        r'GetMainWindowHWND\(\).*?SerializeWindowIcon\(icon\)',
        "Salamatrix does not provide extension/main-window icon fallback to workers")
    require(
        powershell_worker,
        r"Invoke-Host -Method 'salamander\.host\.windowIcon'.*?"
        r"ExtensionWindowIconFilter : IMessageFilter.*?"
        r"WM_SETICON.*?ICON_BIG.*?ICON_SMALL.*?"
        r"Application\]::AddMessageFilter\(\$filter\)",
        "PowerShell worker does not apply the extension icon to top-level windows")
    require(
        powershell_worker + packages,
        r'ScriptMethod MessageBox.*?buttons.*?icon.*?'
        r'MB_YESNO.*?MB_ICONWARNING',
        "PowerShell message boxes cannot use themed confirmations and error icons")
    require(
        setup,
        r"AddPlugin\('gitworktreenavigator',\s*'Git Worktree Navigator'",
        "x64 installer does not offer Git Worktree Navigator as a plugin")
    require(
        setup,
        r"extensions\\git-worktree-navigator.*?"
        r"IsPluginSelected\('gitworktreenavigator'\)",
        "x64 installer does not package the selected Git Worktree Navigator")
    require(
        setup,
        r"AddPluginDependency\('powershellruntime',\s*'salamatrix'\).*?"
        r"AddPluginDependency\('gitworktreenavigator',\s*"
        r"'powershellruntime'\)",
        "x64 installer does not include Git Worktree Navigator dependencies")
    require(
        setup,
        r"AddPlugin\('filelockinspector',\s*'File Lock Inspector'",
        "x64 installer does not offer File Lock Inspector as a plugin")
    require(
        setup,
        r"Salamatrix Progress Demo\\\*.*?recursesubdirs.*?"
        r"IsPluginSelected\('salamatrixdemos'\)",
        "x64 installer omits the Automation.JScript extension icon")
    require(
        setup,
        r"extensions\\file-lock-inspector.*?"
        r"IsPluginSelected\('filelockinspector'\)",
        "x64 installer does not package the selected File Lock Inspector")
    require(
        setup,
        r"AddPluginDependency\('powershellruntime',\s*'salamatrix'\).*?"
        r"AddPluginDependency\('filelockinspector',\s*"
        r"'powershellruntime'\)",
        "x64 installer does not include File Lock Inspector dependencies")

    require(
        pr_msbuild_workflow,
        r"matrix\.platform.*?-ne 'x64'.*?owner\.Name -eq 'HardwareWrapper'.*?continue",
        "PR build does not exclude the x64-only HardwareWrapper on other platforms")
    require(
        hardware_wrapper_project,
        r"<TargetFramework>net9\.0</TargetFramework>.*?"
        r"<MSBuildWarningsAsMessages>.*?MSB3277</MSBuildWarningsAsMessages>",
        "HardwareWrapper does not narrowly allow the prebuilt HardView framework warning")
    require(
        hardware_wrapper_project,
        r'HardwareWrapper\.cpp">.*?<DisableSpecificWarnings>4267;',
        "upstream HardwareWrapper narrowing warnings are not scoped to its source file")
    require_absent(
        hardware_wrapper_project,
        r"<ItemDefinitionGroup(?:(?!</ItemDefinitionGroup>).)*"
        r"<DisableSpecificWarnings>4267;",
        "HardwareWrapper suppresses narrowing warnings for the entire project")

    require(plugins1, r"CPluginData::InitDLL", "dynamic menu InitDLL lifecycle is missing")
    require(plugins1, r"PluginIfaceForMenuExt\.BuildMenu", "dynamic menu interface BuildMenu call is missing")
    require(plugins2, r"SupportDynMenuExt.*?BuildMenu", "dynamic menu rebuild path is missing")
    require(salamatrix, r"CPluginInterface::Connect.*?SalamatrixPackages->Refresh\(\)",
            "Salamatrix Connect does not re-evaluate package runtime states")

    generated_docs = subprocess.run(
        [sys.executable, "-B",
         str(ROOT / "tools" / "generate_salamatrix_automation_reference.py"),
         "--check"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if generated_docs.returncode != 0:
        raise AssertionError(
            "generated Salamatrix HTML documentation is stale: "
            + generated_docs.stderr.strip()
        )

    print("Salamatrix regression source contracts passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
