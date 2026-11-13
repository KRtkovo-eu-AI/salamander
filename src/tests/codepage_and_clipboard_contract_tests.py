# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Guard typed ListView UTF-8 adapters. | Prevent generic LPARAM dispatch from interpreting unrelated control payloads as LVITEMA. | | openai/gpt-6-luna-pro |
# | 2026-11-12 | Guard the standalone wrapper include path. | Keep the PCH-disabled UTF-8 wrapper build covered. | | openai/gpt-6-luna-pro |
# | 2026-11-12 | Guard Unicode common-control text dispatch. | Prevent ANSI status/list-view messages from corrupting UTF-8 localized text. | | openai/gpt-6-luna-pro |
# | 2026-11-12 | Guard UTF-8 localized resource loading. | Prevent localized narrow strings from reverting to Windows-locale-dependent bytes. | | openai/gpt-6-luna-pro |

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
    localized_strings = function_slice(
        (ROOT / "src/salamdr2.cpp").read_text(encoding="utf-8"),
        "char* LoadStr(int resID, HINSTANCE hInstance)",
        "WCHAR* LoadStrW(int resID, HINSTANCE hInstance)")
    require(localized_strings, "LoadStringW(hInstance, resID, wideBuffer",
            "localized resource loading must start from Unicode strings")
    require(localized_strings, "WideCharToMultiByte(CP_UTF8",
            "localized narrow strings must be explicitly encoded as UTF-8")
    require_absent(localized_strings, "LoadString(hInstance",
                   "localized resource loading must not depend on the Windows ANSI code page")
    require_absent(localized_strings, "LoadStringA(",
                   "localized resource loading must not use the ANSI API explicitly")

    utf8_header = (ROOT / "src/common/utf8wrap.h").read_text(encoding="utf-8")
    utf8_wrappers = (ROOT / "src/common/utf8wrap.cpp").read_text(encoding="utf-8")
    require(utf8_header, "#define SendMessageA SalSendMessageA",
            "ANSI messages must pass through the UTF-8-aware dispatcher")
    require(utf8_header, "SalListViewInsertColumnA((hwnd),",
            "generic list-view column insertion must use its UTF-8-aware wrapper")
    require(utf8_header, "SalListViewInsertItemA", "list-view insertion must use a typed UTF-8 adapter")
    require(utf8_header, "SalListViewSetItemA", "list-view updates must use a typed UTF-8 adapter")
    require(utf8_header, "SalListViewSetItemTextA", "list-view text updates must use a typed UTF-8 adapter")
    require(utf8_header, "#define ListView_InsertItem", "list-view insertion macro must use the typed adapter")
    require(utf8_header, "#define ListView_SetItemText", "list-view text macro must use the typed adapter")
    require(utf8_wrappers, "SB_SETTEXTW",
            "localized status-bar text must be sent through the Unicode message")
    typed_listview_dispatch = function_slice(
        utf8_wrappers,
        "LRESULT SendListViewItemA(",
        "LRESULT WINAPI SalSendMessageA(")
    require(typed_listview_dispatch, "LVM_INSERTITEMW",
            "list-view item insertion must use the Unicode message for UTF-8 text")
    require(typed_listview_dispatch, "LVM_SETITEMTEXTW",
            "list-view subitem text must use the Unicode message for UTF-8 text")
    require(utf8_wrappers, "LVM_INSERTCOLUMNW",
            "localized list-view columns must be inserted through the Unicode message")
    require(utf8_wrappers, "Utf8ToWide",
            "common-control text must be decoded from UTF-8 before the Unicode message")
    require(utf8_wrappers, "IsWindowClassW(hwnd, STATUSCLASSNAMEW)",
            "status-bar message translation must not intercept unrelated window messages")
    require(typed_listview_dispatch, "IsWindowClassW(hwnd, WC_LISTVIEWW)",
            "typed list-view adapters must verify their target control class")
    generic_dispatch = function_slice(
        utf8_wrappers,
        "LRESULT WINAPI SalSendMessageA(",
        "BOOL WINAPI SalExtTextOutA(")
    require_absent(generic_dispatch, "LVM_",
                   "generic LPARAM dispatch must not route list-view structure messages")
    require_absent(generic_dispatch, "LVITEMA",
                   "generic LPARAM dispatch must not interpret payloads as LVITEMA")
    require_absent(generic_dispatch, "LVCOLUMNA",
                   "generic LPARAM dispatch must not interpret payloads as LVCOLUMNA")
    implementation_source = utf8_wrappers
    if implementation_source.index("#define UTF8WRAPPERS_IMPLEMENTATION") > implementation_source.index(r'#include "..\precomp.h"'):
        raise AssertionError("UTF-8 wrapper implementation guard must precede its PCH include")

    utf8_sample = "東京"
    if utf8_sample.encode("utf-8") != bytes.fromhex("e69db1e4baac"):
        raise AssertionError("non-Czech UTF-8 control-text sample is not preserved")
    find_source = (ROOT / "src/finddlg1.cpp").read_text(encoding="utf-8")
    require(find_source, "HDM_GETITEMW",
            "custom Find header painting must retrieve localized text through Unicode")
    require(find_source, "GetTextExtentPoint32W",
            "custom Find header sorting must measure Unicode text")
    require(find_source, "DrawTextW",
            "custom dark Find headers must draw Unicode text")
    require_absent(find_source, "Header_GetItem(cd->hdr.hwndFrom",
                   "custom Find header painting must not use the ANSI getter")

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

    print("Localized UTF-8 controls/resources, code-page recognition, and clipboard OLE source-contract tests passed.")


if __name__ == "__main__":
    main()
