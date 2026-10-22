// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace UndeletePaths
{
inline std::wstring Wide(const char* path) { return SalamanderDiskSelection::WideFromPath(path); }
inline std::string Utf8(const wchar_t* path) { return SalamanderDiskSelection::Utf8FromWide(path); }
inline std::wstring Join(const std::wstring& parent, const std::wstring& name)
{
    return parent + (!parent.empty() && parent.back() != L'\\' ? L"\\" : L"") + name;
}
inline std::wstring Comparable(const std::wstring& path)
{
    if (path.compare(0, 8, L"\\\\?\\UNC\\") == 0) return L"\\\\" + path.substr(8);
    if (path.compare(0, 4, L"\\\\?\\") == 0) return path.substr(4);
    return path;
}
inline std::wstring Absolute(const std::wstring& path)
{
    if (path.empty() || path.compare(0, 4, L"\\\\?\\") == 0) return path;
    DWORD capacity = GetFullPathNameW(path.c_str(), 0, NULL, NULL);
    if (!capacity || capacity > SAL_MAX_PATH) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return std::wstring(); }
    std::vector<wchar_t> buffer(capacity);
    DWORD length = GetFullPathNameW(path.c_str(), capacity, buffer.data(), NULL);
    return length > 0 && length < capacity ? std::wstring(buffer.data()) : std::wstring();
}
inline std::wstring IoPath(const std::wstring& path)
{
    if (path.size() < MAX_PATH || path.compare(0, 4, L"\\\\?\\") == 0) return path;
    DWORD capacity = GetFullPathNameW(path.c_str(), 0, NULL, NULL);
    if (!capacity || capacity > SAL_MAX_PATH) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return std::wstring(); }
    std::vector<wchar_t> buffer(capacity);
    DWORD length = GetFullPathNameW(path.c_str(), capacity, buffer.data(), NULL);
    if (!length || length >= capacity) return std::wstring();
    std::wstring result(buffer.data());
    return result.compare(0, 2, L"\\\\") == 0 ? L"\\\\?\\UNC\\" + result.substr(2) : L"\\\\?\\" + result;
}
inline std::wstring ReadText(HWND control)
{
    int length = GetWindowTextLengthW(control);
    std::vector<wchar_t> text(static_cast<size_t>(length) + 1);
    GetWindowTextW(control, text.data(), static_cast<int>(text.size()));
    return text.data();
}
inline std::wstring FocusedImagePath(CSalamanderGeneralAbstract* general, int panel)
{
    CSalamanderDiskSelection selection;
    if (!selection.Capture(general, panel, SALDISKSELECTION_FOCUSED_ONLY) || selection.GetCount() != 1 || selection.GetItem(0)->IsDir)
        return std::wstring();
    return selection.GetItem(0)->FullPathW;
}
// The existing image filesystem is ACP/MAX_PATH based. Refuse lossy input at
// its boundary; the dialog still displays the complete, exact Unicode path.
inline BOOL LegacyImagePath(const std::wstring& path, char* output, int capacity)
{
    if (output && capacity > 0) output[0] = 0;
    if (!output || capacity <= 0 || path.empty()) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    const UINT cp = GetACP();
    BOOL replaced = FALSE;
    int length = WideCharToMultiByte(cp, cp == CP_UTF8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS,
                                    path.c_str(), -1, NULL, 0, NULL, cp == CP_UTF8 ? NULL : &replaced);
    if (!length || replaced) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return FALSE; }
    if (length > capacity) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return FALSE; }
    std::vector<char> encoded(length);
    if (!WideCharToMultiByte(cp, cp == CP_UTF8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS,
                             path.c_str(), -1, encoded.data(), length, NULL, cp == CP_UTF8 ? NULL : &replaced) || replaced)
    { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return FALSE; }
    int wideLength = MultiByteToWideChar(cp, 0, encoded.data(), -1, NULL, 0);
    std::vector<wchar_t> decoded(wideLength > 0 ? wideLength : 1);
    if (!wideLength || !MultiByteToWideChar(cp, 0, encoded.data(), -1, decoded.data(), wideLength) || path != decoded.data())
    { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return FALSE; }
    memcpy(output, encoded.data(), length);
    return TRUE;
}
}
