# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
ZIP = ROOT / "plugins" / "zip"


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


def main() -> int:
    common_cpp = (ZIP / "common.cpp").read_text(encoding="utf-8")
    add_cpp = (ZIP / "add.cpp").read_text(encoding="utf-8")
    list_cpp = (ZIP / "list.cpp").read_text(encoding="utf-8")

    zip_bytes = function_body(common_cpp, "static bool ZipBytesToWide")
    require(
        "CP_UTF8" in zip_bytes and "MB_ERR_INVALID_CHARS" in zip_bytes,
        "ZIP path/name UTF-8 decoding must reject invalid sequences before ACP fallback",
    )

    zip_path = function_body(common_cpp, "static std::wstring ZipPathToWide")
    require(
        "ZipBytesToWide(CP_UTF8" in zip_path and "ZipBytesToWide(CP_ACP" in zip_path,
        "ZIP file I/O must try UTF-8 paths first and fall back to ACP only on error",
    )

    oem_page = function_body(common_cpp, "static UINT ZipLegacyOemCodePage")
    require(
        "GetOEMCP()" in oem_page
        and "CP_UTF8" in oem_page
        and "LOCALE_IDEFAULTCODEPAGE" in oem_page,
        "ZIP OEM decoding must use the locale DOS code page when the process ACP is UTF-8",
    )

    decode = function_body(common_cpp, "static bool ZipDecodeZipNameToUtf8")
    require(
        "ZipLegacyOemCodePage" in decode and "GPF_UTF8" in decode and "ZipWideToUtf8" in decode,
        "ZIP listing must decode OEM and UTF-8 entry names into UTF-8 for the panel",
    )
    require(
        "CP_OEMCP" not in decode and "CP_ACP" not in decode,
        "ZIP listing must not decode legacy names through CP_OEMCP/CP_ACP while the process is UTF-8",
    )
    require(
        "WideCharToMultiByte(CP_ACP" not in decode,
        "ZIP UTF-8 names must stay UTF-8 instead of being transcoded to ACP",
    )

    process = function_body(common_cpp, "int CZipCommon::ProcessName")
    require(
        "ZipDecodeZipNameToUtf8" in process,
        "ProcessName must decode archive names through ZipDecodeZipNameToUtf8",
    )
    require(
        "OemToChar" not in process,
        "ProcessName must not finish by converting names with OemToChar into ACP",
    )
    require(
        "WideCharToMultiByte(CP_ACP" not in process,
        "ProcessName must not convert UTF-8 ZIP names to ACP for the panel",
    )

    encode = function_body(common_cpp, "int ZipEncodeEntryName")
    require(
        "ZipWideToOemLossless" not in encode and "ZipLegacyOemCodePage" not in encode,
        "ZIP packing must store UTF-8 names instead of OEM while the process ACP is UTF-8",
    )
    require(
        "storeUtf8" in encode and "ZipWideIsAscii" in encode and "unixPath" in encode,
        "ZIP packing must store UTF-8 with the UTF-8 flag when the name is not ASCII",
    )
    require(
        "CharToOem" not in encode,
        "ZIP packing must not run CharToOem on UTF-8 panel names",
    )

    export_name = function_body(add_cpp, "int CZipPack::ExportName")
    require(
        "ZipEncodeEntryName" in export_name and "GPF_UTF8" in export_name,
        "ExportName must encode panel UTF-8 names and set GPF_UTF8 when needed",
    )
    require(
        "CharToOem" not in export_name,
        "ExportName must not treat UTF-8 panel names as ACP via CharToOem",
    )

    local_header = function_body(add_cpp, "int CZipPack::ExportLocalHeader")
    require(
        local_header.index("ExportName") < local_header.rindex("localHeader->Flag = fileInfo->Flag"),
        "local ZIP headers must copy GPF_UTF8 after the entry name is encoded",
    )

    central_header = function_body(add_cpp, "int CZipPack::WriteCentralHeader")
    require(
        central_header.index("ExportName") < central_header.rindex("centralHeader->Flag = fileInfo->Flag"),
        "central ZIP headers must copy GPF_UTF8 after the entry name is encoded",
    )

    require("CharToOem" not in add_cpp, "the ZIP packer must not call CharToOem on file names")

    require(
        "DupZipUtf8NameW" in list_cpp and "file.NameW = DupZipUtf8NameW(name, nameLen)" in list_cpp,
        "ZIP listing must expose decoded UTF-8 leaf names through CFileData::NameW",
    )
    dup = function_body(list_cpp, "static wchar_t* DupZipUtf8NameW")
    require(
        "CP_UTF8" in dup and "MB_ERR_INVALID_CHARS" in dup,
        "ZIP NameW copies must decode the already converted UTF-8 panel name",
    )

    zip7 = ROOT / "plugins" / "7zip"
    zip_item_h = (zip7 / "7za" / "cpp" / "7zip" / "Archive" / "Zip" / "ZipItem.h").read_text(
        encoding="utf-8"
    )
    zip_item_cpp = (zip7 / "7za" / "cpp" / "7zip" / "Archive" / "Zip" / "ZipItem.cpp").read_text(
        encoding="utf-8"
    )
    client_cpp = (zip7 / "7zclient.cpp").read_text(encoding="utf-8")

    require(
        "GetZipLegacyOemCodePage" in zip_item_h and "GetZipLegacyAnsiCodePage" in zip_item_h,
        "7-Zip ZIP listing must declare locale OEM/ANSI helpers instead of CP_OEMCP/CP_ACP",
    )
    require(
        "CP_OEMCP" not in zip_item_h and "CP_ACP" not in zip_item_h,
        "7-Zip GetCodePage must not use CP_OEMCP/CP_ACP while the process is UTF-8",
    )

    oem_7z = function_body(zip_item_cpp, "UINT GetZipLegacyOemCodePage")
    require(
        "GetOEMCP()" in oem_7z
        and "CP_UTF8" in oem_7z
        and "LOCALE_IDEFAULTCODEPAGE" in oem_7z,
        "7-Zip OEM decoding must use the locale DOS code page when the process OEMCP is UTF-8",
    )

    add_file_dir = function_body(client_cpp, "BOOL C7zClient::AddFileDir")
    require(
        "UnicodeStringToMultiByte" in add_file_dir and "CP_UTF8" in add_file_dir,
        "7-Zip panel listing must convert archive paths to UTF-8",
    )
    require(
        "GetAnsiString(propVariant.bstrVal)" not in add_file_dir,
        "7-Zip panel listing must not convert kpidPath through ACP",
    )
    require(
        "DupUtf8NameW" in add_file_dir,
        "7-Zip listing must expose decoded UTF-8 leaf names through CFileData::NameW",
    )

    print("ZIP pack/list filename encoding preserves Unicode names")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
