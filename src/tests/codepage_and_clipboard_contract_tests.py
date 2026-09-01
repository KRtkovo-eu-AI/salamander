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

    print("Code-page recognition and clipboard OLE source-contract tests passed.")


if __name__ == "__main__":
    main()
