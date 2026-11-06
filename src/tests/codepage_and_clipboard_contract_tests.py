# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-06 | Lock archive paste shared-memory ownership without IsFakeDataObject. | Requiring the OLE marker let Ctrl+V copy CLIPFAKE; copy path must stay untouched. | https://github.com/KRtkovo-eu-AI/salamander/issues/741 | Composer |

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


def main() -> None:
    recognize = function_slice(
        (ROOT / "src/codetbl.cpp").read_text(encoding="utf-8"),
        "void CCodeTables::RecognizeFileType(",
        "int CCodeTables::GetConversionToWinCodePage(")
    require(recognize, "ShouldPreferWindowsCodePageText(pattern, patternLen,",
            "file-type recognition must reject false ISO-8859-1 scores for CP1250 text")
    require(recognize, "strcpy(codePage, Table->WinCodePage)",
            "false ISO-8859 detections must fall back to the Windows code page")
    require(recognize, "GetStringTypeW(CT_CTYPE1, &character, 1, &type)",
            "recognition must classify converted bytes in the target code page")
    require(recognize, "Table->WinCodePageIdentifier",
            "recognition must use the declared target code page")

    preference = (ROOT / "src/codetbl_utils.cpp").read_text(encoding="utf-8")
    require(preference, 'windowsCodePage != 1250 || _stricmp(recognizedCodePage, "CP852") != 0',
            "CP852 typography override must remain restricted to CP1250")
    require(preference, "quotePairs >= 2",
            "CP852 override must require repeated balanced smart quotes")
    require(preference, "quotePairs >= 1 && separators >= 2",
            "CP852 override must accept combined quote and separator evidence")
    require(preference, "separators >= 4",
            "CP852 override must require repeated standalone separators")

    salshlib = (ROOT / "src/salshlib.cpp").read_text(encoding="utf-8")
    query = function_slice(
        salshlib,
        "HRESULT SafeIDataObjectQueryGetData(",
        "HRESULT SafeIDataObjectGetData(")
    require(query, "__try",
            "clipboard QueryGetData must catch SEH exceptions from OLE proxies")
    require(query, "EXCEPTION_EXECUTE_HANDLER",
            "clipboard QueryGetData must keep Salamander running after a proxy AV")
    require_absent(query, "CCallStack::HandleException(",
                   "clipboard QueryGetData still terminates after writing a bug report")

    fake = function_slice(
        salshlib,
        "BOOL IsFakeDataObject(",
        "STDMETHODIMP CFakeDragDropDataObject::QueryInterface(")
    require(fake, "SafeIDataObjectQueryGetData(pDataObject, &formatEtc)",
            "idle paste-state checks must use the SEH-safe QueryGetData wrapper")
    require_absent(fake, "pDataObject->QueryGetData(",
                   "IsFakeDataObject still calls QueryGetData without SEH protection")

    # Archive clipboard paste (#741): shared-memory ownership must drive unpacking.
    # Do not require IsFakeDataObject() for same-instance paste, and never shell-copy CLIPFAKE.
    paste = function_slice(
        (ROOT / "src/fileswn9.cpp").read_text(encoding="utf-8"),
        "BOOL CFilesWindow::ClipboardPaste(",
        "BOOL CFilesWindow::ClipboardPasteToArcOrFS(")
    require(paste, "Do not require IsFakeDataObject() here",
            "archive paste must not gate unpacking on the private OLE marker")
    require(paste, "pasteFromOurData = TRUE",
            "archive paste must unlock DoPasteOperation from shared memory")
    require(paste, "if (pasteEffect == 0)\n                    pasteEffect = DROPEFFECT_COPY;",
            "archive paste must default to Copy when PreferredDropEffect is missing")
    require(paste, "never shell-copy CLIPFAKE",
            "foreign fake clipboard data must not fall through to shell CLIPFAKE copy")
    require(paste, "SalShExtPastedData.DoPasteOperation(",
            "same-instance archive paste must unpack through SalShExtPastedData")
    require_absent(paste, "IsFakeDataObject(dataObj, NULL, NULL, 0) && SalShExtSharedMemView != NULL",
                   "archive paste still requires IsFakeDataObject before shared-memory ownership")

    # Archive Ctrl+C path must stay on the historical OleSetClipboard + OpenClipboard sequence.
    shellsup = (ROOT / "src/shellsup.cpp").read_text(encoding="utf-8")
    archive_copy = function_slice(
        shellsup,
        "ShellAction::archive::clipcopy_files",
        "if (panel->Is(ptPluginFS))")
    require(archive_copy, "OleSetClipboard(fakeDataObject) == S_OK",
            "archive Ctrl+C must still publish through OleSetClipboard")
    require(archive_copy, "OpenClipboard(MainWindow->HWindow)",
            "archive Ctrl+C must keep PreferredDropEffect via OpenClipboard after OleSetClipboard")
    require(archive_copy, "SetClipboardData(cfPrefDrop, effect)",
            "archive Ctrl+C must keep Win32 PreferredDropEffect publishing")
    require_absent(archive_copy, "SetPreferredDropEffect",
                   "archive Ctrl+C must not use the wrapper-only PreferredDropEffect path that broke clipboard replace")
    require_absent(archive_copy, "CEnumFormatEtcWithExtra",
                   "archive Ctrl+C must not use a custom EnumFormatEtc wrapper")

    copy_paste_get = function_slice(
        salshlib,
        "STDMETHODIMP CFakeCopyPasteDataObject::GetData(",
        "CSalShExtPastedData::CSalShExtPastedData(")
    require(copy_paste_get, "return DV_E_FORMATETC; // presence is reported by QueryGetData()",
            "archive clipboard GetData must keep the historical SalFakeRealPath refusal")
    require_absent(copy_paste_get, "AllocClipboardAnsiStringHGlobal",
                   "archive clipboard GetData must not invent a custom SalFakeRealPath payload")

    print("Code-page recognition and clipboard OLE source-contract tests passed.")


if __name__ == "__main__":
    main()
