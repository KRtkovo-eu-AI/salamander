// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Add bounded WinHTTP requests for native release and catalog checks. | Replace managed HttpClient and package downloads. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include <string>

namespace SamandarinNative
{
struct HttpResponse
{
    DWORD StatusCode = 0;
    std::wstring FinalUrl;
    std::string Body;
};

bool HttpGet(const std::wstring& url, size_t maximumBytes, HttpResponse* response,
             std::wstring* error, volatile LONG* cancelFlag = nullptr,
             bool appendNoCache = false, bool requireHttps = false);
bool HttpDownloadToFile(const std::wstring& url, const std::wstring& destination,
                        size_t maximumBytes, std::wstring* error,
                        volatile LONG* cancelFlag = nullptr);
bool IsAbsoluteHttpUrl(const std::wstring& url);
} // namespace SamandarinNative
