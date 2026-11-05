# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-05 | Contract-test shell-extension SEH survival. | Faulty third-party shell extensions must not TerminateProcess Salamander. | https://github.com/KRtkovo-eu-AI/salamander/issues/773 | Composer |

"""Source-contract checks for surviving faulty in-process shell extensions (#773).

Historically most IContextMenu / icon / overlay SEH filters called
CCallStack::HandleException, which writes a bug report and TerminateProcess's.
That turns a single bad third-party shell extension into a Salamander crash
(right-click empty panel space is the common repro). The icon-reader path
already swallowed SEH; the same survival pattern must cover context menus,
HandleMenuMsg, GetCommandString, overlays, and shell thumbnail providers.
"""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def function_slice(text: str, start: str, end: str) -> str:
    first = text.find(start)
    last = text.find(end, first + len(start))
    if first < 0 or last < 0:
        raise AssertionError(f"Cannot locate function section: {start}")
    return text[first:last]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def require_absent(text: str, needle: str, description: str) -> None:
    if needle in text:
        raise AssertionError(f"Unexpected {description}: {needle}")


def require_survives(section: str, where: str) -> None:
    require(section, "SHELL_EXTENSION_EXCEPTION_FILTER",
            f"{where} must use the shell-extension survival SEH filter")
    require_absent(section, "CCallStack::HandleException(",
                   f"{where} still terminates via HandleException after a shell-extension AV")


def main() -> None:
    callstk = (ROOT / "src/callstk.h").read_text(encoding="utf-8")
    require(callstk, "#define SHELL_EXTENSION_EXCEPTION_FILTER EXCEPTION_EXECUTE_HANDLER",
            "callstk.h must define the shared shell-extension SEH survival filter")
    require(callstk, "issue #773",
            "callstk.h must document why shell-extension SEH must not TerminateProcess")

    shellib = (ROOT / "src/shellib.cpp").read_text(encoding="utf-8")
    get_menu_new = function_slice(
        shellib,
        "void GetMenuNewAux(IContextMenu2* contextMenu2, HMENU m, int minCmd, int maxCmd)",
        "void GetNewOrBackgroundMenu(")
    require_survives(get_menu_new, "GetMenuNewAux (background/New menu = empty-panel right-click)")

    shellsup = (ROOT / "src/shellsup.cpp").read_text(encoding="utf-8")
    for start, end, where in (
        ("void AuxInvokeCommand(CFilesWindow* panel, CMINVOKECOMMANDINFO* ici)",
         "void AuxInvokeAndRelease(",
         "AuxInvokeCommand"),
        ("HRESULT AuxGetCommandString(",
         "static BOOL ShellContextMenuContainsVerb(",
         "AuxGetCommandString"),
        ("void ShellActionAux5(UINT flags, CFilesWindow* panel, HMENU h)",
         "void ShellActionAux6(",
         "ShellActionAux5 QueryContextMenu"),
        ("void ExecuteAssociationAux2(IContextMenu2* menu, HMENU h, DWORD flags)",
         "void ExecuteAssociationAux3(",
         "ExecuteAssociationAux2 QueryContextMenu"),
    ):
        require_survives(function_slice(shellsup, start, end), where)

    cascade = function_slice(
        shellsup,
        "static void InitializeShellCascadeMenus(",
        "static BOOL ContainsTextI(")
    require_survives(cascade, "InitializeShellCascadeMenus HandleMenuMsg")

    fileswn9 = (ROOT / "src/fileswn9.cpp").read_text(encoding="utf-8")
    require_survives(
        function_slice(fileswn9,
                       "BOOL SafeInvokeCommand(IContextMenu2* menu, CMINVOKECOMMANDINFO& ici)",
                       "BOOL CFilesWindow::ClipboardPaste("),
        "SafeInvokeCommand")

    fileswn0 = (ROOT / "src/fileswn0.cpp").read_text(encoding="utf-8")
    require_survives(
        function_slice(fileswn0,
                       "BOOL CFilesWindow::SafeHandleContextMenuMsg2(",
                       "void CFilesWindow::RegisterDragDrop("),
        "SafeHandleContextMenuMsg2")
    filesbx1 = (ROOT / "src/filesbx1.cpp").read_text(encoding="utf-8")
    require(filesbx1, "SafeHandleContextMenuMsg2(",
            "panel WindowProc must forward ContextMenu messages to the SEH-safe helper")
    require_absent(filesbx1, "Parent->ContextMenu->HandleMenuMsg(",
                   "panel WindowProc must not call ContextMenu HandleMenuMsg without SEH")

    finddlg2 = (ROOT / "src/finddlg2.cpp").read_text(encoding="utf-8")
    require_survives(
        function_slice(finddlg2,
                       "void ContextMenuQuery(IContextMenu2* contextMenu, HMENU h)",
                       "BOOL CFindDialog::SafeHandleContextMenuMsg("),
        "Find ContextMenuQuery")
    require_survives(
        function_slice(finddlg2,
                       "BOOL CFindDialog::SafeHandleContextMenuMsg(",
                       "void CFindDialog::OnDrag("),
        "Find SafeHandleContextMenuMsg")
    finddlg1 = (ROOT / "src/finddlg1.cpp").read_text(encoding="utf-8")
    require(finddlg1, "SafeHandleContextMenuMsg(",
            "Find WindowProc must forward ContextMenu messages to the SEH-safe helper")

    shiconov = (ROOT / "src/shiconov.cpp").read_text(encoding="utf-8")
    require(shiconov, "SHELL_EXTENSION_EXCEPTION_FILTER",
            "icon-overlay shell extensions must use the survival SEH filter")
    require_absent(shiconov, "TerminateProcess(GetCurrentProcess(), 1)",
                   "icon-overlay SEH must not TerminateProcess after a faulty overlay AV")

    wic = (ROOT / "src/plugins/pictview/wic/WicBackend.cpp").read_text(encoding="utf-8")
    require(wic, "TryGetShellThumbnailBitmap",
            "shell thumbnail path must expose an SEH-safe entry point")
    require(wic, "EXCEPTION_EXECUTE_HANDLER",
            "shell thumbnail SEH must keep Salamander alive after a provider AV")
    thumb = function_slice(wic, "HRESULT TryGetShellThumbnailBitmap(",
                           "HRESULT TryOpenShellThumbnail(")
    require_absent(thumb, "CCallStack::HandleException(",
                   "shell thumbnail SEH still terminates via HandleException")

    folders_fs2 = (ROOT / "src/plugins/folders/fs2.cpp").read_text(encoding="utf-8")
    require(folders_fs2, "EXCEPTION_EXECUTE_HANDLER",
            "Folders FS context menus must catch shell-extension SEH")
    require(folders_fs2, "Faulty shell extension threw an exception in Folders QueryContextMenu.",
            "Folders FS QueryContextMenu must log and continue after a shell-extension AV")

    print("Shell-extension crash-protection source-contract tests passed.")


if __name__ == "__main__":
    main()
