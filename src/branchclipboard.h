// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <winnetwk.h>
#include <lm.h>
#include <string>
#include <vector>

inline std::wstring BranchClipboardPlainPath(const std::wstring& path)
{
    if (path.compare(0, 8, L"\\\\?\\UNC\\") == 0) return L"\\\\" + path.substr(8);
    if (path.compare(0, 4, L"\\\\?\\") == 0) return path.substr(4);
    return path;
}

inline bool BranchShareRelativePath(const std::wstring& path, std::wstring share, std::wstring& relative)
{
    while (share.size() > 3 && share.back() == L'\\') share.pop_back();
    if (share.empty() || path.size() < share.size() ||
        CompareStringOrdinal(path.c_str(), (int)share.size(), share.c_str(), (int)share.size(), TRUE) != CSTR_EQUAL ||
        (path.size() > share.size() && share.back() != L'\\' && path[share.size()] != L'\\')) return false;
    size_t start = share.size();
    if (start < path.size() && path[start] == L'\\') ++start;
    relative = path.substr(start);
    return true;
}

inline bool BranchClipboardUNCPath(const std::wstring& input, std::wstring& result, unsigned int depth = 0)
{
    const std::wstring path = BranchClipboardPlainPath(input);
    if (path.compare(0, 2, L"\\\\") == 0) { result = path; return true; }
    if (path.size() < 3 || path[1] != L':' || depth >= 10) return false;
    DWORD bytes = 0;
    DWORD status = WNetGetUniversalNameW(path.c_str(), UNIVERSAL_NAME_INFO_LEVEL, NULL, &bytes);
    if (status == ERROR_MORE_DATA && bytes != 0)
    {
        std::vector<BYTE> buffer(bytes);
        status = WNetGetUniversalNameW(path.c_str(), UNIVERSAL_NAME_INFO_LEVEL, buffer.data(), &bytes);
        if (status == NO_ERROR)
        {
            result = ((UNIVERSAL_NAME_INFOW*)buffer.data())->lpUniversalName;
            return true;
        }
    }
    // SUBST keeps a DOS-device target beginning with \??\; physical volumes do not.
    const std::wstring drive = path.substr(0, 2);
    std::vector<wchar_t> target(4096);
    DWORD targetLength = 0;
    while ((targetLength = QueryDosDeviceW(drive.c_str(), target.data(), (DWORD)target.size())) == 0)
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || target.size() >= 65536) break;
        target.resize(target.size() * 2);
    }
    if (targetLength != 0 && wcsncmp(target.data(), L"\\??\\", 4) == 0)
    {
        std::wstring resolved = target.data() + 4;
        if (!resolved.empty() && resolved.back() != L'\\') resolved.push_back(L'\\');
        resolved += path.substr(3);
        if (BranchClipboardUNCPath(resolved, result, depth + 1)) return true;
    }
    std::vector<wchar_t> computer(256);
    DWORD computerSize = (DWORD)computer.size();
    if (!GetComputerNameW(computer.data(), &computerSize)) return false;
    std::wstring best;
    size_t bestLength = 0;
    bool bestHidden = true;
    DWORD resume = 0;
    do
    {
        SHARE_INFO_2* shares = NULL;
        DWORD count = 0, total = 0;
        status = NetShareEnum(NULL, 2, (LPBYTE*)&shares, MAX_PREFERRED_LENGTH, &count, &total, &resume);
        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            for (DWORD i = 0; i < count; ++i)
            {
                const SHARE_INFO_2& share = shares[i];
                if ((share.shi2_type & STYPE_MASK) != STYPE_DISKTREE || share.shi2_path == NULL || share.shi2_netname == NULL) continue;
                const std::wstring local = share.shi2_path;
                const std::wstring remote = share.shi2_netname;
                std::wstring relative;
                const bool hidden = !remote.empty() && remote.back() == L'$';
                if (!BranchShareRelativePath(path, local, relative) ||
                    (!best.empty() && (hidden && !bestHidden || hidden == bestHidden && local.size() <= bestLength))) continue;
                best = L"\\\\" + std::wstring(computer.data()) + L"\\" + remote;
                if (!relative.empty()) best += L"\\" + relative;
                bestLength = local.size();
                bestHidden = hidden;
            }
        }
        if (shares != NULL) NetApiBufferFree(shares);
    } while (status == ERROR_MORE_DATA);
    if (best.empty()) return false;
    result = best;
    return true;
}
