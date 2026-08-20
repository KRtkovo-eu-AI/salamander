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


def require_absent(text: str, needle: str, description: str) -> None:
    if needle in text:
        raise AssertionError(f"Unexpected {description}: {needle}")


def main() -> None:
    common_winlib = (ROOT / "src/common/winlib.cpp").read_text(encoding="utf-8")
    common_winlib_h = (ROOT / "src/common/winlib.h").read_text(encoding="utf-8")
    common_dpi = (ROOT / "src/common/winlibdpi.h").read_text(encoding="utf-8")
    common_sheets = (ROOT / "src/common/sheets.cpp").read_text(encoding="utf-8")
    code_tables = (ROOT / "src/codetbl.cpp").read_text(encoding="utf-8")
    plugin_winlib = (ROOT / "src/plugins/shared/winliblt.cpp").read_text(encoding="utf-8")
    plugin_winlib_h = (ROOT / "src/plugins/shared/winliblt.h").read_text(encoding="utf-8")
    pictview_dialogs = (ROOT / "src/plugins/pictview/dialogs.cpp").read_text(encoding="utf-8")
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
    pictview_dark_mode = function_slice(
        pictview_dialogs, "void ApplyPictViewDarkMode(HWND hwnd)",
        "bool ApplyPictViewDarkModeIfSelected(HWND hwnd)")
    require(pictview_dark_mode, "DarkModeAllowDarkScrollbars(hwnd);",
            "dark scrollbar scope for PictView's EXIF description field")
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

    for relative_path in ("src/menu3.cpp", "src/menubar.cpp"):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "GetEffectiveMenuLogFont(",
                f"menu-specific font with UI-font fallback in {relative_path}")
    viewer_chrome = (ROOT / "src/viewer.cpp").read_text(encoding="utf-8")
    require(viewer_chrome, "GetEffectiveDefaultUILogFont(",
            "effective UI font in the Internal Viewer chrome")

    viewer = (ROOT / "src/viewer.cpp").read_text(encoding="utf-8")
    require(viewer, "GetEffectiveMenuLogFont(&menuLogFont, HWindow);",
            "Internal Viewer uses the configured menu font")
    require(viewer, 'SetProp(HWindow, _T("OpenSalamander.UIFont"), MenuFont)',
            "Internal Viewer menu-font handoff to dark menu rendering")

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
    require(viewer3, "GetEffectiveMenuLogFont(&logFont, dpiWindow);",
            "configured menu font in the Internal Viewer menu bar")
    require(viewer3, "UseCustomMenuFont", "custom menu font enables Internal Viewer menu-bar drawing")
    require(viewer3, "WM_UAHMEASUREMENUITEM",
            "Internal Viewer reserves main-menu-equivalent item spacing")
    require(viewer3, "item->mis.itemWidth += margin;",
            "Internal Viewer adds spacing without modifying menu captions")
    require(viewer3, "GetSysColor(COLOR_MENUBAR)",
            "Internal Viewer light menu-bar background matches its menu buttons")

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
    require_after(dark_groupbox, "getControlTextFont(hWnd, isFontCreated)", "GetThemeFont(",
                  "configured control font before theme fallback in dark groupboxes")
    if "lf.lfHeight = static_cast<LONG>(lf.lfHeight * 1.2);" in dark_status:
        raise AssertionError("Dark-mode groupbox captions must not enlarge the configured UI font")
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
    native_viewer = (ROOT / "src/plugins/shared/webviewviewer/native_viewer.cpp").read_text(encoding="utf-8")
    plugin_dark_mode = (ROOT / "src/plugins/shared/plugindarkmode.cpp").read_text(encoding="utf-8")
    require(native_viewer, "ApplyMenuFont();",
            "configured menu font applied to Viewer Frame")
    require(native_viewer, 'SetPropW(window_, L"OpenSalamander.UIFont", menuFont_);',
            "Viewer Frame menu font handoff to dark menu rendering")
    require(native_viewer, "PaintLightMenuBarItem",
            "Viewer Frame applies its menu font to the light-mode menu bar")
    require(native_viewer, "CreateCustomMenuBar()",
            "Viewer Frame uses Salamander's custom menu bar and popup menus")
    require(native_viewer, "case WM_ERASEBKGND:",
            "Viewer Frame paints its initial client area with the configured theme background")
    require(native_viewer, "put_DefaultBackgroundColor(background)",
            "Viewer Frame gives WebView its dark background before the first document paint")
    require(native_viewer, "controller_->put_IsVisible(FALSE);",
            "Viewer Frame keeps WebView hidden until its first themed navigation completes")
    require(native_viewer, "void SetLoadProgress(int percent)",
            "Viewer Frame reports concrete load progress in its status bar")
    require(native_viewer, "const int resetWidth = 42;",
            "Viewer Frame Reset button matches the Internal Viewer width")
    require(native_viewer, "add_NavigationStarting",
            "Viewer Frame advances loading feedback when navigation actually begins")
    require(native_viewer, "data->viewerFont = request.viewerFont;",
            "Prism uses the configured Internal Viewer font")
    require(native_viewer, "padding:0 0 0 1px",
            "Prism removes its document padding to match Internal Viewer spacing")
    require(native_viewer, "--salamander-gutter-width:",
            "Prism sizes its gutter from the document line-number digit count")
    require(native_viewer, "::selection{background:",
            "Prism uses the Internal Viewer selection colors")
    require(native_viewer, "parameters_->viewerFont.lfWeight",
            "Prism preserves the configured Internal Viewer font weight")
    require(native_viewer, "GetTextMetrics(fontDC, &viewerMetrics)",
            "Prism derives line height and character pitch from the same GDI font metrics as Internal Viewer")
    require(native_viewer, "const double charPixelWidth = (std::max)(viewerMetrics.tmAveCharWidth, 1L);",
            "Prism gutter uses the already DPI-adjusted GDI character width directly")
    require_absent(native_viewer, "const double cssScale = static_cast<double>(USER_DEFAULT_SCREEN_DPI)",
                   "Prism does not shrink DPI-adjusted GDI metrics a second time")
    require(native_viewer, "letter-spacing:calc(var(--salamander-char-width) - 1ch)",
            "Prism matches the Internal Viewer fixed-character pitch")
    require(native_viewer, "pre[class*='language-']>code{font:inherit;line-height:inherit;letter-spacing:inherit}",
            "Prism theme code styles cannot override the Internal Viewer font metrics")
    require(native_viewer, "background:linear-gradient(to right,",
            "Prism paints the complete gutter as one continuous background color")
    require(native_viewer, "pre[class*='language-'].line-numbers{--salamander-gutter-width:",
            "Prism overrides the stock 3.8em gutter padding with Internal Viewer geometry")
    require(native_viewer, "padding-right:1px;text-align:right",
            "Prism line numbers use the same one-pixel right gap as Internal Viewer")
    require(native_viewer, "pre.line-numbers .line-numbers-rows>span:before{box-sizing:border-box;position:relative;top:-1px",
            "Prism line-number glyph baselines are optically aligned with code text")
    require(native_viewer, "InstalledPrismLanguages(parameters_->module)",
            "Prism enumerates the syntax highlighters installed beside the plug-in")
    require(native_viewer, "IDM_NV_SYNTAX_AUTOMATIC",
            "Prism exposes automatic syntax detection in its highlighter submenu")
    require(native_viewer, "!AddMenuItem(mainMenu_,\n                          parameters_->syntaxHighlighter.empty()",
            "Prism exposes Syntax Highlighter as the third top-level menu-bar button")
    require(native_viewer, "title += L\" - [\" +",
            "Prism window title reports the active syntax highlighter as a separated suffix")
    require(native_viewer, "parameters_->gui->CreateMenuPopup()",
            "Viewer Frame creates menu-font-aware popup menus")
    require(native_viewer, "view->CheckItem(IDM_NV_LINE_NUMBERS",
            "Viewer Frame exposes checked Prism menu states in its custom popup")
    require(native_viewer, 'std::wstring fileCaption = L"  " + parameters_->fileMenu + L"  ";',
            "left and inter-item padding in Viewer Frame menu bar")
    require(plugin_dark_mode, 'GetPropW(hwnd, L"OpenSalamander.UIFont") != NULL',
            "Viewer Frame keeps menu-font drawing active in light mode")

    internal_viewer = (ROOT / "src/viewer3.cpp").read_text(encoding="utf-8")
    viewer_core = (ROOT / "src/viewer.cpp").read_text(encoding="utf-8")
    viewer_lifecycle = (ROOT / "src/viewer2.cpp").read_text(encoding="utf-8")
    require(internal_viewer, "case WM_INITMENUPOPUP:",
            "Internal Viewer refreshes dynamically populated custom submenus")
    require(internal_viewer, "DestroyViewerMenuControls();\n        DestroyWindow(HWindow);",
            "Internal Viewer releases the custom menu bar before closing its parent window")
    require(viewer_core, "DestroyWindow(menuBarWindow);",
            "Internal Viewer destroys the menu-bar child HWND before deleting its CMenuBar object")
    require(internal_viewer, "if (ShowLineNumbers)\n                {\n                    // The gutter is painted directly",
            "Internal Viewer repaints line-number gutters instead of pixel-scrolling stale rows")
    require(viewer_lifecycle, "WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN",
            "Internal Viewer parent painting excludes its child scrollbars")
    require(internal_viewer, "DarkModeAllowDarkScrollbars(HWindow);",
            "Internal Viewer retains darkmode rendering for its child scrollbars")
    require(internal_viewer, "Width, menuHeight + Height,",
            "Internal Viewer paints the scrollbar corner below its custom menu bar")
    require(viewer_core, "NULL, // WM_ERASEBKGND uses the active Viewer background color",
            "Internal Viewer avoids a COLOR_WINDOW flash before scheme-aware background erasing")
    require(viewer_core, "BitBlt(dc, 0, y, gutterWidth, rowHeight, Bitmap.HMemDC",
            "Internal Viewer presents each complete line-number gutter row from its line bitmap")
    for relative_path in (
        "src/plugins/webview2renderviewer/managed_bridge.cpp",
        "src/plugins/textviewer/managed_bridge.cpp",
    ):
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        require(source, "SALCFG_MENUFONT",
                f"configured menu font passed to Viewer Frame by {relative_path}")
    text_viewer_bridge = (ROOT / "src/plugins/textviewer/managed_bridge.cpp").read_text(encoding="utf-8")
    require(text_viewer_bridge, "SALCFG_VIEWERFONT",
            "Prism receives the configured Internal Viewer document font")

    lang_rc = (ROOT / "src/lang/lang.rc").read_text(encoding="utf-8")
    require(lang_rc, "UI Fonts for Salamander and Plug-ins",
            "visible scope of the UI font setting")
    require(dialogs5, "DrawTextW(dc, text.c_str(), -1, &textExtent,",
            "mounted-volumes group caption measured with its configured control font")
    if "lf.lfHeight = (int)(lf.lfHeight * 1.2);" in dialogs5:
        raise AssertionError("Mounted-volumes caption must use the configured UI font size")
    require(dialogs5, "PostMessage(HWindow, WM_APP_CHANGE_DRIVE_APPLY_CAPTION_FONT, 0, 0);",
            "mounted-volumes caption font restored after property-page font application")
    require(dialogs5, "SendMessage(mountFolders, WM_SETFONT, (WPARAM)groupFont, FALSE);",
            "mounted-volumes checkbox uses the surrounding groupbox font")
    require(dialogs5, "GetPropW(mountFolders, L\"Darkmodelib.Button.UseConfiguredFont\")",
            "mounted-volumes width measurement follows the actual configured or themed font path")
    dialogs2 = (ROOT / "src/dialogs2.cpp").read_text(encoding="utf-8")
    require(dialogs2, "MarkConfiguredButtonFonts(HWindow);",
            "main application marks every button control after page initialization")
    require(dialogs2, '_tcsicmp(className, _T("Button")) == 0',
            "configured UI font includes checkboxes and radio buttons")
    require(dialogs2, "CCommonPropSheetPage::DialogProc(",
            "configuration pages install their caption-font markers")
    require(dialogs2, "SetPropW(child, L\"Darkmodelib.Button.UseConfiguredFont\", (HANDLE)1);",
            "configured caption path also in the default UI font mode")
    require(dialogs2, "ApplyPanelFontToListControls(HWindow);",
            "property-page list controls restore the panel font after UI font application")
    require(plugin_winlib, "MarkConfiguredButtonFonts(hwnd);",
            "plug-in groupbox captions retain their configured dialog font")
    require(plugin_winlib, "SetPropW(child, L\"Darkmodelib.Button.UseConfiguredFont\", (HANDLE)1);",
            "plug-in dark-mode Button controls opt in to their configured font")
    require(plugin_winlib, 'lstrcmpiW(className, L"Button") == 0',
            "plug-in font marker uses Unicode Win32 APIs without TCHAR dependencies")
    menu_bar = (ROOT / "src/menubar.cpp").read_text(encoding="utf-8")
    require(menu_bar, "GetEffectiveMenuLogFont(&menuFont, hDPIWindow);",
            "menu bar uses its custom font or the selected UI font")
    popup_menu = (ROOT / "src/menu3.cpp").read_text(encoding="utf-8")
    require(popup_menu, "GetEffectivePanelContextMenuLogFont(&menuFont, dpiWindow);",
            "panel context menus use their custom font or the selected UI font")
    require(dialogs5, "IDB_MENUFONT", "menu-font selector in Appearance")
    require(dialogs5, "IDB_PANELCONTEXTMENUFONT", "panel-context-menu-font selector in Appearance")
    main_window_config = (ROOT / "src/mainwnd3.cpp").read_text(encoding="utf-8")
    require(main_window_config, "oldUseCustomPanelContextMenuFont",
            "live refresh after changing the panel context-menu font")
    shellsup = (ROOT / "src/shellsup.cpp").read_text(encoding="utf-8")
    require(shellsup, "Native TrackPopupMenuEx always uses the system menu",
            "panel shell context menus use the UI-font-aware renderer on Windows 11")
    darkmode = (ROOT / "src/darkmode.cpp").read_text(encoding="utf-8")
    if "DialogFontMode" in darkmode:
        raise AssertionError("shared darkmode.cpp must not depend on main-app font globals")


if __name__ == "__main__":
    main()
