# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
SPL_ARC = SRC / "plugins" / "shared" / "spl_arc.h"
SPL_VERS = SRC / "plugins" / "shared" / "spl_vers.h"
PLUGINS_H = SRC / "plugins.h"
PLUGINS1 = SRC / "plugins1.cpp"
FILESWN0 = SRC / "fileswn0.cpp"
PACK_H = SRC / "pack.h"
PACK1 = SRC / "pack1.cpp"
FILESWND_H = SRC / "fileswnd.h"
SEVEN_ZIP = SRC / "plugins" / "7zip" / "7zip.cpp"

REMOVED_DOC_EXTS = (
    "doc",
    "docx",
    "xls",
    "xlsx",
    "ppt",
    "pptx",
    "odt",
    "ods",
    "epub",
    "xpi",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace : index + 1]
    raise AssertionError("unterminated function: " + signature)


def split_extensions(mask):
    parts = []
    for raw in mask.replace("*.", "").lower().split(";"):
        ext = raw.strip().lstrip(".")
        if ext:
            parts.append(ext)
    return set(parts)


def main() -> int:
    spl_arc = SPL_ARC.read_text(encoding="utf-8")
    class_match = re.search(
        r"class CPluginInterfaceForArchiverAbstract\b.*?\{(.*)^\};",
        spl_arc,
        re.DOTALL | re.MULTILINE,
    )
    require(class_match is not None, "CPluginInterfaceForArchiverAbstract was not found")
    methods = re.findall(r"virtual\s+\w[\w\s\*&]*\s+WINAPI\s+(\w+)\s*\(", class_match.group(1))
    require(methods, "no WINAPI methods found on CPluginInterfaceForArchiverAbstract")
    require(
        methods[-1] == "CanOpenArchive",
        "CanOpenArchive must be the last method in CPluginInterfaceForArchiverAbstract, got "
        + methods[-1],
    )
    require(
        "PrematureDeleteTmpCopy" in methods
        and methods.index("PrematureDeleteTmpCopy") == len(methods) - 2,
        "CanOpenArchive must be appended after PrematureDeleteTmpCopy",
    )

    spl_vers = SPL_VERS.read_text(encoding="utf-8")
    require(
        re.search(r"#define\s+LAST_VERSION_OF_SALAMANDER\s+106\b", spl_vers) is not None,
        "LAST_VERSION_OF_SALAMANDER must be 106",
    )
    require(
        re.search(r"#define\s+SAL_SDK_VER_CANOPENARCHIVE\s+106\b", spl_vers) is not None,
        "SAL_SDK_VER_CANOPENARCHIVE must be 106",
    )

    plugins1 = PLUGINS1.read_text(encoding="utf-8")
    can_open = function_body(plugins1, "BOOL CPluginData::CanOpenArchive")
    require(
        "BuiltForVersion < SAL_SDK_VER_CANOPENARCHIVE" in can_open,
        "CPluginData::CanOpenArchive must gate calls on BuiltForVersion",
    )
    require(
        "InitDLL(" in can_open,
        "CPluginData::CanOpenArchive must load the plugin before reading BuiltForVersion",
    )
    require(
        can_open.find("InitDLL(") < can_open.find("BuiltForVersion < SAL_SDK_VER_CANOPENARCHIVE"),
        "BuiltForVersion gate must run after InitDLL so unloaded plugins are probed",
    )

    fileswn0 = FILESWN0.read_text(encoding="utf-8")
    ctrl = function_body(fileswn0, "void CFilesWindow::CtrlPageDnOrEnter")
    require(
        "TryEnterFileAsArchive" in ctrl,
        "Ctrl+PgDown on a non-archive disk file must call TryEnterFileAsArchive",
    )
    require(
        "ExecuteAssociation" not in ctrl,
        "CtrlPageDnOrEnter must not call ExecuteAssociation",
    )
    try_enter = function_body(fileswn0, "void CFilesWindow::TryEnterFileAsArchive")
    require(
        "ExecuteAssociation" not in try_enter,
        "TryEnterFileAsArchive must not call ExecuteAssociation",
    )
    require(
        "PackProbeArchivePlugin" in try_enter and "ChangePathToArchive" in try_enter,
        "TryEnterFileAsArchive must probe plugins and open with ChangePathToArchive",
    )
    require(
        "IDS_FILEISNOTARCHIVE" in try_enter,
        "failed probes must show IDS_FILEISNOTARCHIVE",
    )

    pack_h = PACK_H.read_text(encoding="utf-8")
    require(
        re.search(
            r"BOOL PackList\([^;]*CPluginData\*\s*forcedPlugin\s*=\s*NULL\s*\)",
            pack_h,
            re.DOTALL,
        )
        is not None,
        "PackList must accept an optional forced plugin",
    )
    fileswnd_h = FILESWND_H.read_text(encoding="utf-8")
    require(
        re.search(
            r"BOOL ChangePathToArchive\([^;]*CPluginData\*\s*forcedPlugin\s*=\s*NULL\s*\)",
            fileswnd_h,
            re.DOTALL,
        )
        is not None,
        "ChangePathToArchive must accept an optional forced plugin",
    )

    pack1 = PACK1.read_text(encoding="utf-8")
    probe = function_body(pack1, "CPluginData* PackProbeArchivePlugin")
    require(
        "IsSevenZipArchiverPlugin" in probe,
        "probe order must identify the 7-Zip plugin",
    )
    require(
        probe.rfind("sevenZip") > probe.find("IsSevenZipArchiverPlugin"),
        "7-Zip must be tried last in PackProbeArchivePlugin",
    )

    seven_zip = SEVEN_ZIP.read_text(encoding="utf-8")
    require(
        re.search(r"#define\s+CURRENT_CONFIG_VERSION\s+7\b", seven_zip) is not None,
        "7-Zip CURRENT_CONFIG_VERSION must be 7",
    )
    connect_start = seven_zip.index("void CPluginInterface::Connect")
    connect_end = seven_zip.index("void CPluginInterface::Event", connect_start)
    connect = seven_zip[connect_start:connect_end]
    require(
        "ConfigVersion < 7" in connect and "ForceRemovePanelArchiver" in connect,
        "Connect must ForceRemovePanelArchiver on ConfigVersion < 7",
    )

    association_masks = []
    association_masks.extend(re.findall(r'AddPanelArchiver\(\s*"([^"]*)"', connect))
    association_masks.extend(re.findall(r'AddCustomUnpacker\(\s*"[^"]*"\s*,\s*"([^"]*)"', connect))
    groups = re.search(
        r"panelArchiverExtensionGroups\[\]\s*=\s*\{(.*?)\};",
        connect,
        re.DOTALL,
    )
    require(groups is not None, "panelArchiverExtensionGroups was not found")
    association_masks.extend(re.findall(r'"([^"]*)"', groups.group(1)))
    require(association_masks, "no 7-Zip panel/unpacker association strings found")
    present = set()
    for mask in association_masks:
        present.update(split_extensions(mask))
    leftover = [ext for ext in REMOVED_DOC_EXTS if ext in present]
    require(
        not leftover,
        "7-Zip AddPanelArchiver/AddCustomUnpacker must not contain " + ", ".join(leftover),
    )
    for ext in REMOVED_DOC_EXTS:
        require(
            f'ForceRemovePanelArchiver("{ext}")' in connect
            or f'ForceRemovePanelArchiver("{ext}");' in connect
            or f'"{ext}"' in connect,
            f"Connect upgrade must ForceRemovePanelArchiver {ext}",
        )
    removed_list = re.search(
        r"removedDocumentExts\[\]\s*=\s*\{(.*?)\};",
        connect,
        re.DOTALL,
    )
    require(removed_list is not None, "removedDocumentExts was not found")
    removed = set(re.findall(r'"([^"]*)"', removed_list.group(1)))
    missing_force = [ext for ext in REMOVED_DOC_EXTS if ext not in removed]
    require(
        not missing_force,
        "ForceRemovePanelArchiver list is missing " + ", ".join(missing_force),
    )

    plugins_h = PLUGINS_H.read_text(encoding="utf-8")
    require(
        "BOOL CanOpenArchive(const char* fileName)" in plugins_h,
        "archiver encapsulation must wrap CanOpenArchive",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
