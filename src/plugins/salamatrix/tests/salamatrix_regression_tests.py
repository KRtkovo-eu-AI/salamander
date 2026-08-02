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
    texts = read("src/lang/texts.rc2")
    ai_header = read("src/plugins/salamatrixai/salamatrixai.h")
    ai_contract = read("src/plugins/salamatrix/salamatrix_ai.h")
    ui_contract = read("src/plugins/salamatrix/salamatrix_ui.h")
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
    runtime_protocol = read("src/plugins/salamatrix/salamatrix_runtime_protocol.h")
    ai_rc2 = read("src/plugins/salamatrixai/salamatrixai.rc2")
    automation_header = read("src/plugins/automation/automationplug.h")
    automation = read("src/plugins/automation/automationplug.cpp")
    automation_entry = read("src/plugins/automation/entry.cpp")
    plugins1 = read("src/plugins1.cpp")
    plugins2 = read("src/plugins2.cpp")
    mainwnd1 = read("src/mainwnd1.cpp")
    mainwnd2 = read("src/mainwnd2.cpp")
    mainwnd3 = read("src/mainwnd3.cpp")
    toolbar4 = read("src/toolbar4.cpp")
    toolbar8 = read("src/toolbar8.cpp")
    main_menu = read("src/menu4.cpp")
    javascriptruntime = read("src/plugins/javascriptruntime/javascriptruntime.cpp")
    pythonruntime = read("src/plugins/pythonruntime/pythonruntime.cpp")
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
    salamatrix_runtime = read("src/plugins/salamatrix/salamatrix_runtime.h")
    salamatrix_ui = read("src/plugins/salamatrix/salamatrix_ui.cpp")
    salamatrix_props = read("src/plugins/salamatrix/vcxproj/salamatrix.props")
    salamatrix_project = read(
        "src/plugins/salamatrix/vcxproj/salamatrix.vcxproj")
    manifest = read("src/plugins/salamatrix/salamatrix_manifest.cpp")
    packages = read("src/plugins/salamatrix/salamatrix_packages.cpp")
    api_docs = read("src/plugins/salamatrix/salamatrix_api_docs.h")
    general_contract = read("src/plugins/shared/spl_gen.h")
    general_impl = read("src/zip.cpp")
    setup = read("doc/runbook-setup/inno_setup_salamander_x64.iss")
    runtime_package_verifier = read("tools/verify_runtime_packages.ps1")
    python_demo = read("src/extensions/demos/python/main.py")
    lua_demo = read("src/extensions/demos/lua/main.lua")
    lua_demo_manifest = json.loads(
        read("src/extensions/demos/lua/extension.json"))
    powershell_demo = read("src/extensions/demos/powershell/main.ps1")
    javascript_worker = read(
        "src/plugins/javascriptruntime/runtime/salamatrix_worker.mjs")
    python_worker = read(
        "src/plugins/pythonruntime/runtime/salamatrix_worker.py")
    powershell_worker = read(
        "src/plugins/powershellruntime/runtime/salamatrix_worker.ps1")
    php_worker = read(
        "src/plugins/phpruntime/runtime/salamatrix_worker.php")
    lua_worker = read(
        "src/plugins/luaruntime/runtime/salamatrix_worker.lua")
    navigator = read("src/extensions/git-worktree-navigator/main.ps1")
    navigator_manifest = json.loads(
        read("src/extensions/git-worktree-navigator/extension.json"))
    lock_inspector = read("src/extensions/file-lock-inspector/main.ps1")
    lock_inspector_manifest = json.loads(
        read("src/extensions/file-lock-inspector/extension.json"))
    menu_builder = read("src/extensions/extension-menu-builder/main.ps1")
    menu_builder_manifest = json.loads(
        read("src/extensions/extension-menu-builder/extension.json"))

    require(dialogs, r"HasStablePluginKey\(p->RegKeyName, \"SALAMATRIX\"\).*?IsPluginName\(p->Name, \"Salamatrix Framework\"\)",
            "Salamatrix Framework key/name fallback is missing")
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
                     r'Python\.CPython.*?PowerShell.*?PHP\.CLI',
            "bundled model lacks strict contracts for all four runtime facades")
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
            r'pull_requests\[0\]\.head\.repo\.full_name != github\.repository.*?'
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
    for name, runtime in zip(
        ("Python", "PowerShell", "JavaScript", "PHP", "Lua"), runtime_provider_sources):
        require(runtime, r"SetFlagLoadOnSalamanderStart\(TRUE\)",
                f"{name} runtime provider is not loaded on Salamander startup")
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
    if "ui.progress" not in lua_demo_manifest.get("capabilities", []):
        raise AssertionError(
            "Lua demo manifest does not declare the progress capability")
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
    require(runtime_package_verifier,
            r"Name\s*=\s*'luaruntime'.*?extension-runtimes\\luaruntime.*?salamatrix_worker\.lua.*?lua\.exe.*?lua\.dll.*?LICENSE-LUA\.txt",
            "runtime package verifier does not validate the Lua provider layout")

    require(salamatrix_props, r"USE_DARKMODELIB=1", "Salamatrix Framework is not built with win32-darkmodelib")
    require(salamatrix, r"ApplyHostDarkModePolicy\(SalamanderGeneral", "Salamatrix host dark-mode policy is not initialized")
    require(salamatrix_runtime, r"DarkModeSetConfiguredColors", "Salamatrix dark-mode scheme colors are not synchronized")
    require(salamatrix_runtime, r"DarkModeMessageBoxW", "Salamatrix runtime message boxes do not use the Unicode dark-mode path")
    require(salamatrix_ui, r"WM_SETTINGCHANGE \|\| message == WM_THEMECHANGED", "Salamatrix dialog theme-change handling is missing")
    require(salamatrix_ui, r"DarkModeRefreshTitleBar\(hwnd\)", "Salamatrix dialog title bar dark-mode refresh is missing")
    require(salamatrix_ui, r"ApplyDarkScrollbarScopes\(BOOL dark\).*?DarkModeAllowDarkScrollbars\(control->WindowHandle\).*?DarkModeDisallowDarkScrollbars\(control->WindowHandle\)",
            "Salamatrix dialogs do not scope the host dark scrollbar hook to controls")
    require(salamatrix_ui, r"PostMessage\(hwnd, WM_SALAMATRIX_APPLY_DARK_SCROLLBARS",
            "Salamatrix dialogs apply dark scrollbar scopes during WM_INIT reentrantly")
    require(salamatrix_ui, r'RegisterNativeDialog\(dialog\).*?'
                           r'WM_NCDESTROY.*?UnregisterNativeDialog\(dialog\)',
            "Salamatrix UI provider does not track active native dialog lifetimes")
    require(salamatrix_ui, r'CloseAllNativeDialogs\(\).*?'
                           r'ClosingAllNativeDialogs = TRUE.*?'
                           r'while \(!OpenNativeDialogs\.empty\(\)\).*?'
                           r'dialog->Close\(\)',
            "Salamatrix UI provider cannot close active dialogs before DLL unload")
    require(salamatrix, r'CPluginInterface::Release.*?'
                        r'CloseAllNativeDialogs\(\).*?'
                        r'DestroyRuntimeServices\(\)',
            "Salamatrix unloads native dialog procedures before their HWNDs")

    require(packages, r"BOOL RuntimeUsable;", "extension package runtime usability state is missing")
    require(packages, r"plugins.*automation.*scripts", "Automation sample-script extension root is missing")
    require(packages, r"salamander\.ui\.progress\.create", "framework progress host dispatch is missing")
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
    require(setup, r"plugins\\salamatrix\\salamatrix-automation-api\.html",
            "installer does not package the Automation API HTML reference")
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
        dialog_resources,
        r"IDD_PLUGINS DIALOGEX[^\n]*\n"
        r"STYLE (?=[^\n]*DS_MODALFRAME)(?=[^\n]*WS_THICKFRAME)"
        r"(?![^\n]*(?:WS_MINIMIZEBOX|WS_MAXIMIZEBOX))[^\n]*\n"
        r".*?IDC_PLUGINS_GRIP.*?SBS_SIZEBOXBOTTOMRIGHTALIGN",
        "Plugin Manager does not use the Configuration-style resizable dialog frame and grip")
    require(
        dialogs,
        r"GripWindow = new CTPHGripWindow.*?"
        r"DarkModeApplyWindow\(GripWindow->HWindow\)",
        "Plugin Manager resize grip is not dark-mode aware")
    require(
        dialogs,
        r"void CPluginsDlg::LayoutControls\(\).*?"
        r"RDW_INVALIDATE \| RDW_ERASE \| "
        r"RDW_ALLCHILDREN \| RDW_UPDATENOW",
        "Plugin Manager does not fully repaint after resizing")
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
    require_absent(python_demo, r"message_box", "Python demo must not block Salamander with a modal UI call")
    require(powershell_demo, r"\$Salamander\.ui\.Notify", "PowerShell demo does not show a non-blocking result")
    require_absent(powershell_demo, r"MessageBox", "PowerShell demo must not block Salamander with a modal UI call")
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
        powershell_worker,
        r'ScriptMethod Appearance.*?salamander\.host\.appearance',
        "PowerShell runtime does not expose Salamander appearance")
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
            "generated Automation API HTML reference is stale: "
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
