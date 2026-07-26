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


def main() -> int:
    dialogs = read("src/dialogs5.cpp")
    texts = read("src/lang/texts.rc2")
    ai_header = read("src/plugins/salamatrixai/salamatrixai.h")
    ai = read("src/plugins/salamatrixai/salamatrixai.cpp")
    plugins1 = read("src/plugins1.cpp")
    plugins2 = read("src/plugins2.cpp")

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
    require(ai, r"ExecuteMenuItem.*?id == CmdOpenAssistant.*?ShowChat", "AI menu command does not open chat")
    require(ai, r"SalamanderGeneral->LoadStr\(DLLInstance, IDS_AI_ASSISTANT_MENU", "AI menu caption does not use Salamander localization")
    require(ai, r"BuildMenu.*?GetAssistantMenuCaption\(\).*?CmdOpenAssistant", "AI BuildMenu does not add the resource-backed command")
    require(ai, r"IsCurrentService\(SALAMATRIX_SERVICE_AI.*?g_ai\).*?UnregisterProvider",
            "AI Release lacks current-service pointer validation")
    require(ai, r"result\.Interface == expected", "AI Release does not compare service pointer identity")
    for global_name in ("g_ai", "g_ui", "g_runtime", "g_runner"):
        require(ai, rf"{re.escape(global_name)}\s*=\s*NULL", f"AI Release does not clear {global_name}")

    require(plugins1, r"CPluginData::InitDLL", "dynamic menu InitDLL lifecycle is missing")
    require(plugins1, r"PluginIfaceForMenuExt\.BuildMenu", "dynamic menu interface BuildMenu call is missing")
    require(plugins2, r"SupportDynMenuExt.*?BuildMenu", "dynamic menu rebuild path is missing")

    print("Salamatrix regression source contracts passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
