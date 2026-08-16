# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def function_slice(text: str, start: str, end: str) -> str:
    first = text.find(start)
    last = text.find(end, first + len(start))
    if first < 0 or last < 0:
        raise AssertionError(f"Cannot locate function section: {start}")
    return text[first:last]


def require_after(text: str, first: str, second: str, description: str) -> None:
    first_pos = text.find(first)
    second_pos = text.find(second)
    if first_pos < 0 or second_pos < 0 or first_pos >= second_pos:
        raise AssertionError(f"Invalid ordering for {description}")


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    common_winlib = (ROOT / "src/common/winlib.cpp").read_text(encoding="utf-8")
    common_dpi = (ROOT / "src/common/winlibdpi.h").read_text(encoding="utf-8")
    common_sheets = (ROOT / "src/common/sheets.cpp").read_text(encoding="utf-8")
    plugin_winlib = (ROOT / "src/plugins/shared/winliblt.cpp").read_text(encoding="utf-8")
    plugin_winlib_h = (ROOT / "src/plugins/shared/winliblt.h").read_text(encoding="utf-8")
    dialogs5 = (ROOT / "src/dialogs5.cpp").read_text(encoding="utf-8")

    font_preview = function_slice(
        dialogs5, "void CCfgPageAppearance::LoadFontPreview(",
        "void CCfgPageAppearance::LoadControls()")
    require(font_preview, '_snprintf_s(buf, _TRUNCATE, "%d pt. %s (%s)",',
            "constant and safe UI font description format")
    if "LoadStr(IDS_FONTDESCRIPTION)" in font_preview:
        raise AssertionError("UI font description must not use a resource as a printf format")

    core_dialog = function_slice(
        common_winlib, "CDialog::CDialogProc(", "CWindowsManager::CWindowsManager()")
    require_after(core_dialog, "dlg->DialogProc(uMsg, wParam, lParam)",
                  "WinLibApplyConfiguredDialogFont(hwndDlg)",
                  "core dialog font after concrete WM_INITDIALOG")

    require(common_winlib, "WinLibDPICloneResourceDialogWithFont",
            "template-time font in core dialogs")
    require(plugin_winlib, "WinLibDPICloneResourceDialogWithFont",
            "template-time font in plug-in dialogs")
    require(common_sheets, "PSP_DLGINDIRECT",
            "template-time font in core property pages")
    require(common_sheets, "holderFaceName = holderLogFont.lfFaceName",
            "custom font in the Configuration holder template")
    require(plugin_winlib, "PSP_DLGINDIRECT",
            "template-time font in plug-in property pages")
    apply_font = function_slice(
        common_dpi, "WinLibDPIApplyDialogFont(HWND", "WinLibDPISkipDialogTemplateField("
    )
    if "SetWindowPos(" in apply_font:
        raise AssertionError("Post-create dialog font fallback must not rescale geometry")

    core_sheet = function_slice(
        common_sheets, "CPropSheetPage::CPropSheetPageProc(", "CPropertyDialog::Execute()")
    require_after(core_sheet, "dlg->DialogProc(uMsg, wParam, lParam)",
                  "WinLibApplyConfiguredDialogFont(hwndDlg)",
                  "core property-page font after concrete WM_INITDIALOG")

    plugin_dialog = function_slice(
        plugin_winlib, "CDialog::CDialogProc(", "CPropSheetPage::CPropSheetPage(")
    require_after(plugin_dialog, "dlg->DialogProc(uMsg, wParam, lParam)",
                  "WinLibApplyDialogFont(hwndDlg)",
                  "plug-in dialog font after concrete WM_INITDIALOG")

    plugin_sheet = function_slice(
        plugin_winlib, "CPropSheetPage::CPropSheetPageProc(", "CPropertyDialog::Execute()")
    require_after(plugin_sheet, "dlg->DialogProc(uMsg, wParam, lParam)",
                  "WinLibApplyDialogFont(hwndDlg)",
                  "plug-in property-page font after concrete WM_INITDIALOG")

    mainwnd1 = (ROOT / "src/mainwnd1.cpp").read_text(encoding="utf-8")
    require(mainwnd1, "void GetEffectiveDefaultUILogFont(",
            "application-wide effective UI font helper")
    for relative_path in (
        "src/editwnd.cpp", "src/fileswn1.cpp", "src/tabwnd.cpp", "src/toolbar2.cpp"):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "GetEffectiveDefaultUILogFont(",
                f"effective UI font in {relative_path}")

    mainwnd1 = (ROOT / "src/mainwnd1.cpp").read_text(encoding="utf-8")
    panel_font = function_slice(mainwnd1, "BOOL CreatePanelFont()", "BOOL CreateEnvFonts()")
    require(panel_font, "GetSystemGUIFont(&lf)",
            "system fallback independent of the UI font for panel content")
    if "GetEffectiveDefaultUILogFont" in panel_font:
        raise AssertionError("Panel content must not inherit the UI font")

    for relative_path in ("src/menu3.cpp", "src/menubar.cpp", "src/viewer.cpp"):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "GetEffectiveDefaultUILogFont(",
                f"effective UI font in window chrome from {relative_path}")

    splash = (ROOT / "src/logo.cpp").read_text(encoding="utf-8")
    require(splash, 'strcpy(lf.lfFaceName, "MS Shell Dlg 2")',
            "original fixed font in the Splash screen")
    if "GetEffectiveDefaultUILogFont" in splash:
        raise AssertionError("Splash screen must not inherit the custom UI font")

    find_dialog = (ROOT / "src/finddlg1.cpp").read_text(encoding="utf-8")
    require(find_dialog, "SendMessage(HStatusBar, WM_SETFONT, (WPARAM)HStatusFont, TRUE)",
            "UI font in the Find status bar")

    viewer3 = (ROOT / "src/viewer3.cpp").read_text(encoding="utf-8")
    require(viewer3, "popup.SetTemplateMenu(subMenu)",
            "shared UI-font context menu in the Internal Viewer")
    require(viewer3, "MeasureViewerMenuBarItem(",
            "UI-font metrics in the Internal Viewer native menu bar")

    require(plugin_winlib_h, "WinLibGetDefaultUILogFontForDPI",
            "shared plug-in DPI-aware UI font getter")
    for relative_path in (
        "src/plugins/automation/guiform.cpp",
        "src/plugins/filecomp/dlg_com.cpp",
        "src/plugins/ftp/fs1.cpp",
        "src/plugins/mmviewer/mmviewer.cpp",
        "src/plugins/regedt/dialogs.cpp",
        "src/plugins/winscp/windows/VCLCommon.cpp",
    ):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "WinLibGetDefaultUILogFont",
                f"shared UI font in {relative_path}")

    sftp_fs1 = (ROOT / "src/plugins/sftp/src/fs1.cpp").read_text(encoding="utf-8")
    require(sftp_fs1, "WinLibApplyDialogFont(HWindow)",
            "UI font in the raw SFTP connection dialog")

    pictview_status = (ROOT / "src/plugins/pictview/statsbar.cpp").read_text(encoding="utf-8")
    require(pictview_status, "WinLibGetDefaultUILogFont(HWindow, &logFont)",
            "UI font in the PictView status bar")

    for relative_path in (
        "src/plugins/filecomp/dialogs2.cpp",
        "src/plugins/filecomp/dialogs4.cpp",
        "src/plugins/filecomp/viewwnd.cpp",
    ):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "SetTemplateMenu(",
                f"shared UI-font context menu in {relative_path}")
    filecomp_main = (ROOT / "src/plugins/filecomp/mainwnd.cpp").read_text(encoding="utf-8")
    require(filecomp_main, "MeasureFileCompMenuBarItem(",
            "UI-font metrics in the File Comparator native menu bar")
    require(filecomp_main, 'SetProp(HWindow, _T("OpenSalamander.UIFont"), EnvFont)',
            "dark native menu-bar UI font handoff in File Comparator")

    lang_rc = (ROOT / "src/lang/lang.rc").read_text(encoding="utf-8")
    require(lang_rc, "UI Fonts for Salamander and Plug-ins",
            "visible scope of the UI font setting")


if __name__ == "__main__":
    main()
