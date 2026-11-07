# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Assert archive-association provenance survives migrations. | Prevent #748 cleanup from replacing user-selected owners and keep per-extension metadata available for #733. | https://github.com/KRtkovo-eu-AI/salamander/issues/748 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-05 | Assert 7-Zip no longer steals TAR/ZIP/UnRAR panel extensions. | Guard the issue #748 association ownership fix. | https://github.com/KRtkovo-eu-AI/salamander/issues/748 | Composer |

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
PACK3 = SRC / "pack3.cpp"
PACK1 = SRC / "pack1.cpp"
DIALOGSP = SRC / "dialogsp.cpp"
FILESWND_H = SRC / "fileswnd.h"
MAINWND2 = SRC / "mainwnd2.cpp"
SEVEN_ZIP = SRC / "plugins" / "7zip" / "7zip.cpp"
TAR_DLL = SRC / "plugins" / "tar" / "tardll.cpp"

REMOVED_DOC_EXTS = (
    "doc",
    "docx",
    "docm",
    "dot",
    "dotx",
    "dotm",
    "xls",
    "xlsx",
    "xlsm",
    "xlt",
    "xltx",
    "xltm",
    "ppt",
    "pptx",
    "pptm",
    "pot",
    "potx",
    "potm",
    "pps",
    "ppsx",
    "ppsm",
    "odt",
    "ods",
    "odp",
    "odg",
    "odf",
    "odm",
    "ott",
    "ots",
    "otp",
    "otg",
    "epub",
    "xpi",
)

# Panel extensions owned by dedicated plugins; 7-Zip must not claim them for Enter.
DEDICATED_PANEL_EXTS = (
    "tgz",
    "tbz",
    "taz",
    "tar",
    "gz",
    "bz",
    "bz2",
    "z",
    "rpm",
    "cpio",
    "deb",
    "zip",
    "jar",
    "rar",
)

TAR_FAMILY_EXTS = (
    "tgz",
    "tbz",
    "taz",
    "tar",
    "gz",
    "bz",
    "bz2",
    "z",
    "rpm",
    "cpio",
    "deb",
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
        re.search(r"#define\s+CURRENT_CONFIG_VERSION\s+9\b", seven_zip) is not None,
        "7-Zip CURRENT_CONFIG_VERSION must be 9",
    )
    connect_start = seven_zip.index("void CPluginInterface::Connect")
    connect_end = seven_zip.index("void CPluginInterface::Event", connect_start)
    connect = seven_zip[connect_start:connect_end]
    require(
        "ConfigVersion < 8" in connect and "ForceRemovePanelArchiver" in connect,
        "Connect must ForceRemovePanelArchiver on ConfigVersion < 8",
    )
    require(
        "ConfigVersion < 9" in connect,
        "Connect must ForceRemovePanelArchiver dedicated extensions on ConfigVersion < 9",
    )
    mainwnd2 = MAINWND2.read_text(encoding="utf-8")
    require(
        'GetPluginDataFromSuffix("zip.spl")' in mainwnd2,
        "post-load migration must also clean imported ZIP-owned document rows",
    )
    require(
        'GetPluginDataFromSuffix("tar.spl")' in mainwnd2
        and "EnsureExtensionsForPlugin" in mainwnd2
        and "tarFamilyExts" in mainwnd2,
        "post-load migration must restore TAR ownership of the tar/gzip family",
    )
    require(
        'GetPluginDataFromSuffix("unrar.spl")' in mainwnd2,
        "post-load migration must strip tar-family extensions from UnRAR-owned rows",
    )
    pack_h = PACK_H.read_text(encoding="utf-8")
    require(
        "EnsureExtensionsForPlugin" in pack_h,
        "CPackerFormatConfig must expose EnsureExtensionsForPlugin",
    )

    pack3 = PACK3.read_text(encoding="utf-8")
    require(
        'SALAMANDER_PPA_USEREXTENSIONS = "User Extension List"' in pack3,
        "archive associations must have a persistent user-extension registry value",
    )
    set_format = function_body(pack3, "BOOL CPackerFormatConfig::SetFormat")
    require(
        "FilterUserExtensions(data->UserExtensions, ext)" in set_format,
        "rewriting a row must retain user provenance for extensions that remain",
    )
    require(
        "void CPackerFormatConfig::MarkFormatAsUserOverride" in pack3
        and "Formats[index]->UserExtensions" in pack3,
        "the configuration editor must be able to mark a row's extensions as user-owned",
    )
    save = function_body(pack3, "BOOL CPackerFormatConfig::Save")
    require(
        "SALAMANDER_PPA_USEREXTENSIONS" in save,
        "user provenance must be persisted with archive-association rows",
    )
    load = function_body(pack3, "BOOL CPackerFormatConfig::Load(HKEY hKey)")
    require(
        "SALAMANDER_PPA_USEREXTENSIONS" in load and "hasUserExtensions" in load,
        "user provenance must be optional when loading pre-provenance configurations",
    )
    remove_for_plugin = function_body(pack3, "void CPackerFormatConfig::RemoveExtensionsForPlugin")
    remove_legacy = function_body(pack3, "void CPackerFormatConfig::RemoveExtensionsFromLegacyRows")
    require(
        "IsExtensionInList(data->UserExtensions" in remove_for_plugin
        and "IsExtensionInList(data->UserExtensions" in remove_legacy,
        "both cleanup passes must retain explicitly user-owned extensions",
    )
    ensure = function_body(pack3, "void CPackerFormatConfig::EnsureExtensionsForPlugin")
    require(
        "IsExtensionInList(data->Ext, extension" in ensure,
        "TAR restoration must not create a duplicate association over an existing owner",
    )
    require(
        "IsUserOverride(i, s + 1)" in plugins1,
        "ForceRemovePanelArchiver must retain explicitly user-owned extensions",
    )
    dialogsp = DIALOGSP.read_text(encoding="utf-8")
    require(
        dialogsp.count("MarkFormatAsUserOverride") >= 3,
        "all archive-association edits in Configuration must record user provenance",
    )

    panel_masks = []
    panel_masks.extend(re.findall(r'AddPanelArchiver\(\s*"([^"]*)"', connect))
    groups = re.search(
        r"panelArchiverExtensionGroups\[\]\s*=\s*\{(.*?)\};",
        connect,
        re.DOTALL,
    )
    require(groups is not None, "panelArchiverExtensionGroups was not found")
    panel_masks.extend(re.findall(r'"([^"]*)"', groups.group(1)))
    require(panel_masks, "no 7-Zip panel association strings found")
    panel_present = set()
    for mask in panel_masks:
        panel_present.update(split_extensions(mask))
    stolen = [ext for ext in DEDICATED_PANEL_EXTS if ext in panel_present]
    require(
        not stolen,
        "7-Zip AddPanelArchiver must not contain dedicated-plugin extensions: "
        + ", ".join(stolen),
    )

    association_masks = list(panel_masks)
    association_masks.extend(re.findall(r'AddCustomUnpacker\(\s*"[^"]*"\s*,\s*"([^"]*)"', connect))
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
    dedicated_list = re.search(
        r"removedDedicatedExts\[\]\s*=\s*\{(.*?)\};",
        connect,
        re.DOTALL,
    )
    require(dedicated_list is not None, "removedDedicatedExts was not found")
    dedicated_removed = set(re.findall(r'"([^"]*)"', dedicated_list.group(1)))
    missing_dedicated = [ext for ext in DEDICATED_PANEL_EXTS if ext not in dedicated_removed]
    require(
        not missing_dedicated,
        "removedDedicatedExts is missing " + ", ".join(missing_dedicated),
    )

    # Custom unpacker must still accept TAR-family masks for Unpack / Ctrl+PgDown.
    unpacker_masks = re.findall(r'AddCustomUnpacker\(\s*"[^"]*"\s*,\s*"([^"]*)"', connect)
    unpacker_present = set()
    for mask in unpacker_masks:
        unpacker_present.update(split_extensions(mask))
    for ext in ("tgz", "tar", "gz", "zip", "rar"):
        require(ext in unpacker_present, f"7-Zip custom unpacker must still list *.{ext}")

    tar_dll = TAR_DLL.read_text(encoding="utf-8")
    tar_connect_start = tar_dll.index("void CPluginInterface::Connect")
    tar_panel = re.search(
        r'AddPanelArchiver\(\s*"([^"]*)"',
        tar_dll[tar_connect_start:],
    )
    require(tar_panel is not None, "TAR AddPanelArchiver was not found")
    tar_panel_exts = split_extensions(tar_panel.group(1))
    missing_tar = [ext for ext in TAR_FAMILY_EXTS if ext not in tar_panel_exts]
    require(
        not missing_tar,
        "TAR AddPanelArchiver is missing " + ", ".join(missing_tar),
    )
    tar_family_list = re.search(
        r"tarFamilyExts\[\]\s*=\s*\{(.*?)\};",
        mainwnd2,
        re.DOTALL,
    )
    require(tar_family_list is not None, "tarFamilyExts was not found in mainwnd2.cpp")
    migrated = set(re.findall(r'"([^"]*)"', tar_family_list.group(1)))
    missing_migrated = [ext for ext in TAR_FAMILY_EXTS if ext not in migrated]
    require(
        not missing_migrated,
        "tarFamilyExts migration list is missing " + ", ".join(missing_migrated),
    )

    plugins_h = PLUGINS_H.read_text(encoding="utf-8")
    require(
        "BOOL CanOpenArchive(const char* fileName)" in plugins_h,
        "archiver encapsulation must wrap CanOpenArchive",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
