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
    common_winlib_h = (ROOT / "src/common/winlib.h").read_text(encoding="utf-8")
    common_dpi = (ROOT / "src/common/winlibdpi.h").read_text(encoding="utf-8")
    common_sheets = (ROOT / "src/common/sheets.cpp").read_text(encoding="utf-8")
    code_tables = (ROOT / "src/codetbl.cpp").read_text(encoding="utf-8")
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

    appearance_dialog = function_slice(
        dialogs5, "CCfgPageAppearance::DialogProc(", "// CCfgPageChangeDrive")
    require(appearance_dialog,
            "PostMessage(HWindow, WM_APP_APPEARANCE_RESTORE_FONT_PREVIEWS, 0, 0)",
            "deferred restoration of font previews after property-sheet UI font application")
    require(appearance_dialog,
            "SendDlgItemMessage(HWindow, IDE_PANELFONT, WM_SETFONT, (WPARAM)HPanelFont, TRUE)",
            "selected panel font restored in its preview field")
    require(appearance_dialog,
            "SendDlgItemMessage(HWindow, IDE_DIALOGFONT, WM_SETFONT, (WPARAM)HDialogFont, TRUE)",
            "selected UI font restored in its preview field")

    core_dialog = function_slice(
        common_winlib, "CDialog::CDialogProc(", "CWindowsManager::CWindowsManager()")
    require_after(core_dialog, "dlg->DialogProc(uMsg, wParam, lParam)",
                  "WinLibApplyConfiguredDialogFont(hwndDlg)",
                  "core dialog font after concrete WM_INITDIALOG")
    require(common_winlib_h, "virtual BOOL UseConfiguredDialogFont() { return TRUE; }",
            "per-dialog opt-out from the configured UI font")
    require(common_winlib, "UseConfiguredDialogFont() && WinLibGetConfiguredDialogLogFont",
            "template-time configured font guarded by the per-dialog opt-out")
    require(core_dialog, "if (dlg->UseConfiguredDialogFont())",
            "post-init configured font guarded by the per-dialog opt-out")

    require(common_winlib, "WinLibDPICloneResourceDialogWithFont",
            "template-time font in core dialogs")
    require(code_tables, "Data[i]->NameW.c_str()",
            "conversion menu names decoded directly from convert.cfg")
    require(code_tables, "InsertMenuItemW(menu, position, TRUE, &item)",
            "Unicode Win32 insertion for dynamic conversion menu names")
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

    viewer = (ROOT / "src/viewer.cpp").read_text(encoding="utf-8")
    require(viewer, 'SetProp(HWindow, _T("OpenSalamander.UIFont"), StatusFont)',
            "Internal Viewer UI font handoff to dark menu rendering")

    splash = (ROOT / "src/logo.cpp").read_text(encoding="utf-8")
    require(splash, 'strcpy(lf.lfFaceName, "MS Shell Dlg 2")',
            "original fixed font in the Splash screen")
    if "GetEffectiveDefaultUILogFont" in splash:
        raise AssertionError("Splash screen must not inherit the custom UI font")

    dialogs_h = (ROOT / "src/dialogs.h").read_text(encoding="utf-8")
    about_dialog = function_slice(dialogs_h, "class CAboutDialog", "struct CExecuteItem")
    require(about_dialog, "virtual BOOL UseConfiguredDialogFont() { return FALSE; }",
            "fixed resource font in the About dialog")

    find_dialog = (ROOT / "src/finddlg1.cpp").read_text(encoding="utf-8")
    require(find_dialog, "SendMessage(HStatusBar, WM_SETFONT, (WPARAM)HStatusFont, TRUE)",
            "UI font in the Find status bar")

    viewer3 = (ROOT / "src/viewer3.cpp").read_text(encoding="utf-8")
    require(viewer3, "popup.SetTemplateMenu(subMenu)",
            "shared UI-font context menu in the Internal Viewer")
    if "WM_UAHMEASUREMENUITEM" in viewer3:
        raise AssertionError("Internal Viewer must leave native menu measurement to Windows")

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

    dark_status = (ROOT / "src/third_party/darkmodelib/src/DmlibSubclassControl.cpp").read_text(
        encoding="utf-8")
    dark_button = function_slice(dark_status, "static void renderButton(", "static void paintButton(")
    require_after(dark_button, "getControlTextFont(", "GetThemeFont(",
                  "configured control font before theme fallback in dark checkboxes")
    require_after(dark_button, "SendMessage(hWnd, WM_GETFONT", "GetThemeFont(",
                  "dialog-assigned font before theme fallback for ordinary buttons")
    dark_groupbox = function_slice(
        dark_status, "static void paintGroupbox(", "dmlib_subclass::GroupboxSubclass(")
    require_after(dark_groupbox, "getControlTextFont(hWnd, true", "GetThemeFont(",
                  "configured control font before theme fallback in dark groupboxes")
    require(dark_status, "lf.lfHeight = static_cast<LONG>(lf.lfHeight * 1.2);",
            "same 20-percent emphasis as the configured property-page heading")
    require(dark_status, 'GetPropW(hWnd, L"Darkmodelib.Button.UseConfiguredFont")',
            "control-local configured-font opt-in preserving default themed fonts")
    require(dark_groupbox, "::DrawTextW(hdc, buffer.c_str()",
            "configured groupbox font rendered through the selected HDC font")
    require(dark_groupbox, "::DrawThemeTextEx(hTheme, hdc, BP_GROUPBOX",
            "unchanged themed groupbox text path for the default UI font")
    require(dark_status, "case WM_SETFONT:",
            "custom font updates in dark status-bar painting")
    require(dark_status, "pStatusBarData->setFont(reinterpret_cast<HFONT>(wParam))",
            "dark status-bar font synchronization")

    for relative_path in (
        "src/plugins/filecomp/dialogs2.cpp",
        "src/plugins/filecomp/dialogs4.cpp",
        "src/plugins/filecomp/viewwnd.cpp",
    ):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "SetTemplateMenu(",
                f"shared UI-font context menu in {relative_path}")
    filecomp_main = (ROOT / "src/plugins/filecomp/mainwnd.cpp").read_text(encoding="utf-8")
    if "WM_UAHMEASUREMENUITEM" in filecomp_main:
        raise AssertionError("File Comparator must leave native menu measurement to Windows")
    require(filecomp_main, 'SetProp(HWindow, _T("OpenSalamander.UIFont"), EnvFont)',
            "dark native menu-bar UI font handoff in File Comparator")

    lang_rc = (ROOT / "src/lang/lang.rc").read_text(encoding="utf-8")
    require(lang_rc, "UI Fonts for Salamander and Plug-ins",
            "visible scope of the UI font setting")
    require(dialogs5, "DrawTextW(dc, text.c_str(), -1, &textExtent,",
            "mounted-volumes group caption measured with its configured control font")
    require(dialogs5, "lf.lfHeight = (int)(lf.lfHeight * 1.2);",
            "property-page-heading sizing for the mounted-volumes checkbox")
    require(dialogs5, "if (DialogFontMode == DIALOG_FONT_DEFAULT)",
            "unchanged themed measurement for the default UI font")
    dialogs2 = (ROOT / "src/dialogs2.cpp").read_text(encoding="utf-8")
    require(dialogs2, "MarkConfiguredButtonFonts(HWindow);",
            "main application marks every button control after page initialization")
    require(dialogs2, '_tcsicmp(className, _T("Button")) == 0',
            "configured UI font includes checkboxes and radio buttons")
    require(dialogs2, "CCommonPropSheetPage::DialogProc(",
            "configuration pages install their caption-font markers")
    require(dialogs2, "if (DialogFontMode != DIALOG_FONT_DEFAULT)",
            "configured caption path only outside the default UI font mode")
    darkmode = (ROOT / "src/darkmode.cpp").read_text(encoding="utf-8")
    if "DialogFontMode" in darkmode:
        raise AssertionError("shared darkmode.cpp must not depend on main-app font globals")


if __name__ == "__main__":
    main()
