# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
PLUGINS = REPOSITORY_ROOT / "src" / "plugins2.cpp"
STARTUP = REPOSITORY_ROOT / "src" / "salamdr1.cpp"
CONFIGURATION = REPOSITORY_ROOT / "src" / "mainwnd2.cpp"
PERSISTENCE = REPOSITORY_ROOT / "src" / "salamdr2.cpp"


def main() -> None:
    plugins = PLUGINS.read_text(encoding="utf-8")
    startup = STARTUP.read_text(encoding="utf-8")
    configuration = CONFIGURATION.read_text(encoding="utf-8")
    persistence = PERSISTENCE.read_text(encoding="utf-8")

    check_viewers = re.search(
        r"void CPlugins::CheckViewerData\(\).*?(?=^BOOL CPlugins::)",
        plugins,
        re.DOTALL | re.MULTILINE,
    )
    if check_viewers is None:
        raise AssertionError("deferred viewer validation was not found")
    check_source = check_viewers.group(0)
    if "if (type < 0)" not in check_source or "Get(-type - 1) == NULL" not in check_source:
        raise AssertionError("plugin viewer references must be recognized explicitly")
    unresolved_branch = re.search(
        r"if \(type < 0\).*?continue;",
        check_source,
        re.DOTALL,
    )
    if unresolved_branch is None or "viewerMasks->Delete" in unresolved_branch.group(0):
        raise AssertionError("unresolved plugin viewer references must be preserved")

    if "Plugins.CheckViewerData()" in configuration:
        raise AssertionError("viewer validation must not run during early LoadConfig")
    read_plugins = startup.index("Plugins.ReadPluginsVer(")
    validate_viewers = startup.index("Plugins.CheckViewerData()", read_plugins)
    load_plugins = startup.index("Plugins.HandleLoadOnStartFlag(", read_plugins)
    save_configuration = startup.index("MainWindow->SaveConfig()", read_plugins)
    if not read_plugins < validate_viewers < load_plugins < save_configuration:
        raise AssertionError(
            "viewer validation must run after plugin reconciliation and before configuration save"
        )

    save_viewers = re.search(
        r"BOOL SaveViewers\(.*?(?=^BOOL LoadEditors\()",
        persistence,
        re.DOTALL | re.MULTILINE,
    )
    if save_viewers is None:
        raise AssertionError("SaveViewers was not found")
    save_source = save_viewers.group(0)
    if "ClearKey(viewersKey)" in save_source:
        raise AssertionError("SaveViewers must not clear all persisted viewers before writing")
    write_complete = save_source.find("if (saved)")
    trailing_cleanup = save_source.find("DeleteKey(viewersKey, buf)")
    if (
        "if (!itemSaved)" not in save_source
        or "return saved;" not in save_source
        or write_complete < 0
        or trailing_cleanup < write_complete
    ):
        raise AssertionError(
            "obsolete viewer rows may be deleted only after a completely successful save"
        )

    load_viewers = re.search(
        r"BOOL LoadViewers\(.*?(?=^BOOL SaveViewers\()",
        persistence,
        re.DOTALL | re.MULTILINE,
    )
    if load_viewers is None:
        raise AssertionError("LoadViewers was not found")
    load_source = load_viewers.group(0)
    if (
        "Skipping invalid viewer configuration record" not in load_source
        or load_source.find("CloseKey(subKey)") < load_source.find(
            "Skipping invalid viewer configuration record"
        )
    ):
        raise AssertionError(
            "an invalid viewer record must be closed and skipped without truncating later rows"
        )
    if "char masks[MAX_GROUPMASK]" not in load_source:
        raise AssertionError("LoadViewers must load Masks into a MAX_GROUPMASK buffer")
    if "GetValue(subKey, VIEWERS_MASKS_REG, REG_SZ, masks, MAX_PATH)" in load_source:
        raise AssertionError("LoadViewers must not cap Masks at MAX_PATH")
    if "command.data()" not in load_source or "SAL_MAX_PATH" not in load_source:
        raise AssertionError(
            "LoadViewers must load viewer command/arguments/init-dir with SAL_MAX_PATH capacity"
        )
    if "new (std::nothrow) CViewerMasksItem(" not in load_source:
        raise AssertionError(
            "LoadViewers must use nothrow allocation so a NULL check remains valid"
        )

    load_editors = re.search(
        r"BOOL LoadEditors\(.*?(?=^BOOL SaveEditors\()",
        persistence,
        re.DOTALL | re.MULTILINE,
    )
    if load_editors is None:
        raise AssertionError("LoadEditors was not found")
    editors_source = load_editors.group(0)
    if "char masks[MAX_GROUPMASK]" not in editors_source:
        raise AssertionError("LoadEditors must load Masks into a MAX_GROUPMASK buffer")
    if "GetValue(subKey, EDITORS_MASKS_REG, REG_SZ, masks, MAX_PATH)" in editors_source:
        raise AssertionError("LoadEditors must not cap Masks at MAX_PATH")
    if "new (std::nothrow) CEditorMasksItem(" not in editors_source:
        raise AssertionError(
            "LoadEditors must use nothrow allocation so a NULL check remains valid"
        )

    if "GetValue(hSubKey, SALAMANDER_HLT_ITEM_MASKS, REG_SZ, masks, MAX_GROUPMASK)" not in configuration:
        raise AssertionError("highlight Masks must load into a MAX_GROUPMASK buffer")
    if "GetValue(hSubKey, SALAMANDER_HLT_ITEM_MASKS, REG_SZ, masks, MAX_PATH)" in configuration:
        raise AssertionError("highlight Masks must not cap the registry value at MAX_PATH")
    if "GetValue(key, PANEL_FILTER, REG_SZ, filter, MAX_GROUPMASK)" not in configuration:
        raise AssertionError("panel filter masks must load into a MAX_GROUPMASK buffer")
    for needle, description in (
        (
            "GetValue(actKey, CONFIG_RECYCLEMASKS_REG, REG_SZ,\n"
            "                     Configuration.RecycleMasks.GetWritableMasksString(), MAX_GROUPMASK)",
            "recycle masks",
        ),
        (
            "GetValue(actKey, CONFIG_CONFIGTIGNOREFILESMASKS_REG, REG_SZ,\n"
            "                     Configuration.CompareIgnoreFilesMasks.GetWritableMasksString(), MAX_GROUPMASK)",
            "compare ignore-files masks",
        ),
        (
            "GetValue(actKey, VIEWER_CONFIGTEXTMASK_REG, REG_SZ,\n"
            "                     Configuration.TextModeMasks.GetWritableMasksString(), MAX_GROUPMASK)",
            "viewer text-mode masks",
        ),
        (
            "GetValue(actKey, VIEWER_CONFIGHEXMASK_REG, REG_SZ,\n"
            "                     Configuration.HexModeMasks.GetWritableMasksString(), MAX_GROUPMASK)",
            "viewer hex-mode masks",
        ),
    ):
        if needle not in configuration:
            raise AssertionError(f"{description} must load into MAX_GROUPMASK, not MAX_PATH")

    print("Viewer mask persistence source-contract tests passed.")


if __name__ == "__main__":
    main()
