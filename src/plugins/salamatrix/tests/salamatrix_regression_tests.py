#!/usr/bin/env python3
"""Fast source-level regression checks for Salamatrix integration contracts."""

from pathlib import Path
import re
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
    texts = read("src/lang/texts.rc2")
    ai_header = read("src/plugins/salamatrixai/salamatrixai.h")
    ai_contract = read("src/plugins/salamatrix/salamatrix_ai.h")
    ai = read("src/plugins/salamatrixai/salamatrixai.cpp")
    bundled = read("src/plugins/salamatrixai/bundledprovider.cpp")
    local_llama = read("src/plugins/salamatrixailocalllama/local_llama.cpp")
    local_llama_header = read("src/plugins/salamatrixailocalllama/local_llama.h")
    local_llama_project = read("src/plugins/salamatrixailocalllama/vcxproj/local_llama.vcxproj")
    ai_rc2 = read("src/plugins/salamatrixai/salamatrixai.rc2")
    automation_header = read("src/plugins/automation/automationplug.h")
    automation = read("src/plugins/automation/automationplug.cpp")
    automation_entry = read("src/plugins/automation/entry.cpp")
    plugins1 = read("src/plugins1.cpp")
    plugins2 = read("src/plugins2.cpp")
    javascriptruntime = read("src/plugins/javascriptruntime/javascriptruntime.cpp")
    pythonruntime = read("src/plugins/pythonruntime/pythonruntime.cpp")
    powershellruntime = read("src/plugins/powershellruntime/powershellruntime.cpp")
    phpruntime = read("src/plugins/phpruntime/phpruntime.cpp")
    javascriptruntime_rc = read("src/plugins/javascriptruntime/javascriptruntime.rc")
    pythonruntime_rc = read("src/plugins/pythonruntime/pythonruntime.rc")
    powershellruntime_rc = read("src/plugins/powershellruntime/powershellruntime.rc")
    phpruntime_rc = read("src/plugins/phpruntime/phpruntime.rc")
    runtime_provider_sources = (
        pythonruntime,
        powershellruntime,
        javascriptruntime,
        phpruntime,
    )
    salamatrix = read("src/plugins/salamatrix/salamatrix.cpp")
    salamatrix_runtime = read("src/plugins/salamatrix/salamatrix_runtime.h")
    salamatrix_ui = read("src/plugins/salamatrix/salamatrix_ui.cpp")
    salamatrix_props = read("src/plugins/salamatrix/vcxproj/salamatrix.props")
    packages = read("src/plugins/salamatrix/salamatrix_packages.cpp")
    python_demo = read("src/extensions/demos/python/main.py")
    powershell_demo = read("src/extensions/demos/powershell/main.ps1")

    require(dialogs, r"HasStablePluginKey\(p->RegKeyName, \"SALAMATRIX\"\).*?IsPluginName\(p->Name, \"Salamatrix Framework\"\)",
            "Salamatrix Framework key/name fallback is missing")
    for key, name in (
        ("JAVASCRIPT.RUNTIME", "JavaScript Runtime"),
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
    require(ai, r'SetPluginHomePageURL\("https://samandarin\.krtkovo\.eu/"\)', "AI homepage URL is not set")
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
    require_absent(ai_header, r"class CLocalBundledAssistantProvider",
                   "bundled local provider must not be part of the mandatory AI helper")
    require(ai_contract, r"BuildRelevantApiDescription",
            "AI prompt API slicing helper is missing")
    require(ai_contract, r"AssistantCanImplement",
            "AI unsupported-capability response helper is missing")
    require(ai_contract, r"missingCapabilities",
            "AI contract does not describe missing capabilities")
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
    require(bundled, r'--single-turn.*--conversation', "bundled provider does not request one-shot conversation mode")
    require(bundled, r'ExtractJsonObject', "bundled provider does not tolerate llama-cli diagnostic output around JSON")
    require(local_llama_header, r'class CLocalBundledAssistantProvider',
            "optional local llama provider declaration is missing")
    require(local_llama, r'g_ai->RegisterProvider\(&g_provider\)',
            "optional local llama provider is not registered with Salamatrix.AI")
    require(local_llama, r'g_ai->UnregisterProvider\(&g_provider\)',
            "optional local llama provider is not unregistered during release")
    require(local_llama, r'FUNCTION_CONFIGURATION',
            "optional local llama provider does not expose configuration")
    require(local_llama, r'install_llama\.ps1|LaunchInstaller',
            "optional local llama provider has no downloader integration")
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
    ):
        require(runtime, r"SetPluginHomePageURL\(\"https://samandarin\.krtkovo\.eu/\"\)", f"{name} runtime homepage URL is not set")
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

    for name, runtime_resource in (
        ("JavaScript", javascriptruntime_rc),
        ("Python", pythonruntime_rc),
        ("PowerShell", powershellruntime_rc),
        ("PHP", phpruntime_rc),
    ):
        require_absent(runtime_resource, r"sal_r\.ico", f"{name} runtime must use the default Plugin Manager icon")
    for name, runtime in zip(
        ("Python", "PowerShell", "JavaScript", "PHP"), runtime_provider_sources):
        require(runtime, r"SetFlagLoadOnSalamanderStart\(TRUE\)",
                f"{name} runtime provider is not loaded on Salamander startup")

    require(salamatrix_props, r"USE_DARKMODELIB=1", "Salamatrix Framework is not built with win32-darkmodelib")
    require(salamatrix, r"ApplyHostDarkModePolicy\(SalamanderGeneral", "Salamatrix host dark-mode policy is not initialized")
    require(salamatrix_runtime, r"DarkModeSetConfiguredColors", "Salamatrix dark-mode scheme colors are not synchronized")
    require(salamatrix_runtime, r"DarkModeMessageBoxW", "Salamatrix runtime message boxes do not use the Unicode dark-mode path")
    require(salamatrix_ui, r"WM_SETTINGCHANGE \|\| message == WM_THEMECHANGED", "Salamatrix dialog theme-change handling is missing")
    require(salamatrix_ui, r"DarkModeRefreshTitleBar\(hwnd\)", "Salamatrix dialog title bar dark-mode refresh is missing")

    require(packages, r"BOOL RuntimeUsable;", "extension package runtime usability state is missing")
    require(packages, r"plugins.*automation.*scripts", "Automation sample-script extension root is missing")
    require(packages, r"salamander\.ui\.progress\.create", "framework progress host dispatch is missing")
    require(packages, r"SALAMATRIX_SERVICE_SCRIPT_RUNNER", "legacy compatibility script runner fallback is missing")
    require(packages, r"RuntimeAdapterFlagCompatibility", "legacy fallback is not limited to compatibility adapters")
    require(packages, r"package->RuntimeUsable = registeredRuntime && availableRuntime",
            "extension package runtime usability is not derived from provider availability")
    require(packages, r"InvokeOnMainThread\(\s*HostDispatchOnMainThread",
            "extension host calls are not marshaled to Salamander's UI thread")
    require(packages, r"MessageHello\).*?CopyResult\(\"\{\\\"ok\\\":true\}\"",
            "extension host does not acknowledge the runtime worker handshake")
    require(packages, r"if \(!package->RuntimeUsable\)\s+continue;.*?BuildMenu",
            "unavailable extension packages are not filtered from the menu")
    require(packages, r"void PackageManager::RegisterToolbarButtons\(\).*?if \(!package->RuntimeUsable\)\s+continue;",
            "unavailable extension packages are not filtered from the toolbar")
    require(python_demo, r"Salamander\.ui\.notify", "Python demo does not show a non-blocking result")
    require_absent(python_demo, r"message_box", "Python demo must not block Salamander with a modal UI call")
    require(powershell_demo, r"\$Salamander\.ui\.Notify", "PowerShell demo does not show a non-blocking result")
    require_absent(powershell_demo, r"MessageBox", "PowerShell demo must not block Salamander with a modal UI call")

    require(plugins1, r"CPluginData::InitDLL", "dynamic menu InitDLL lifecycle is missing")
    require(plugins1, r"PluginIfaceForMenuExt\.BuildMenu", "dynamic menu interface BuildMenu call is missing")
    require(plugins2, r"SupportDynMenuExt.*?BuildMenu", "dynamic menu rebuild path is missing")
    require(salamatrix, r"CPluginInterface::Connect.*?SalamatrixPackages->Refresh\(\)",
            "Salamatrix Connect does not re-evaluate package runtime states")

    print("Salamatrix regression source contracts passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
