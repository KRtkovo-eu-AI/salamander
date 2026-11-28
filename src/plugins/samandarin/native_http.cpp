// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Add bounded WinHTTP requests for native release and catalog checks. | Replace managed HttpClient and package downloads. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_http.h"
#include "native_utils.h"

#include <winhttp.h>
#include <algorithm>
#include <limits>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace SamandarinNative
{
namespace
{
struct ParsedUrl
{
    std::wstring Scheme;
    std::wstring Host;
    std::wstring PathAndQuery;
    INTERNET_PORT Port = INTERNET_DEFAULT_HTTPS_PORT;
    bool Secure = false;
};

bool IsCancelled(volatile LONG* cancelFlag)
{
    return cancelFlag != nullptr && InterlockedCompareExchange(const_cast<LONG*>(cancelFlag), 0, 0) != 0;
}

void SetError(std::wstring* error, const std::wstring& message)
{
    if (error != nullptr) *error = message;
}

std::wstring Win32Error(const wchar_t* operation, DWORD code)
{
    return std::wstring(operation) + L" (" + std::to_wstring(code) + L")";
}

bool CrackUrl(const std::wstring& url, ParsedUrl* parsed)
{
    if (parsed == nullptr || url.size() > static_cast<size_t>((std::numeric_limits<DWORD>::max)())) return false;
    URL_COMPONENTS components = {};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &components)) return false;
    if (components.dwSchemeLength == 0 || components.dwHostNameLength == 0) return false;
    parsed->Scheme.assign(components.lpszScheme, components.dwSchemeLength);
    parsed->Host.assign(components.lpszHostName, components.dwHostNameLength);
    parsed->Secure = components.nScheme == INTERNET_SCHEME_HTTPS;
    if (!parsed->Secure && components.nScheme != INTERNET_SCHEME_HTTP) return false;
    parsed->Port = components.nPort;
    if (components.dwUrlPathLength != 0)
    {
        parsed->PathAndQuery.assign(components.lpszUrlPath, components.dwUrlPathLength);
    }
    if (components.dwExtraInfoLength != 0)
    {
        parsed->PathAndQuery.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }
    if (parsed->PathAndQuery.empty()) parsed->PathAndQuery = L"/";
    return true;
}

std::wstring GetOrigin(const std::wstring& url)
{
    ParsedUrl parsed;
    if (!CrackUrl(url, &parsed)) return std::wstring();
    std::wstring origin = parsed.Scheme + L"://" + parsed.Host;
    if ((parsed.Secure && parsed.Port != INTERNET_DEFAULT_HTTPS_PORT) ||
        (!parsed.Secure && parsed.Port != INTERNET_DEFAULT_HTTP_PORT))
    {
        origin += L":" + std::to_wstring(parsed.Port);
    }
    return origin;
}

std::wstring ResolveRedirect(const std::wstring& baseUrl, const std::wstring& location)
{
    if (location.empty()) return std::wstring();
    if (location.find(L"://") != std::wstring::npos) return location;
    const std::wstring origin = GetOrigin(baseUrl);
    if (origin.empty()) return std::wstring();
    if (location.rfind(L"//", 0) == 0)
    {
        ParsedUrl parsed;
        if (!CrackUrl(baseUrl, &parsed)) return std::wstring();
        return parsed.Scheme + L":" + location;
    }
    if (location[0] == L'/') return origin + location;
    const size_t query = baseUrl.find_first_of(L"?#");
    const std::wstring withoutQuery = query == std::wstring::npos ? baseUrl : baseUrl.substr(0, query);
    const size_t slash = withoutQuery.find_last_of(L'/');
    if (slash == std::wstring::npos) return origin + L"/" + location;
    return withoutQuery.substr(0, slash + 1) + location;
}

std::wstring AddRefreshQuery(const std::wstring& url)
{
    const std::wstring withoutFragment = url.substr(0, url.find(L'#'));
    const std::wstring fragment = url.size() == withoutFragment.size() ? L"" : url.substr(withoutFragment.size());
    const wchar_t separator = withoutFragment.find(L'?') == std::wstring::npos ? L'?' : L'&';
    return withoutFragment + separator + L"samandarinRefresh=" + std::to_wstring(GetTickCount64()) + fragment;
}

std::wstring QueryHeaderString(HINTERNET request, DWORD query)
{
    DWORD bytes = 0;
    WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &bytes, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes < sizeof(wchar_t)) return std::wstring();
    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
    if (!WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &bytes,
                             WINHTTP_NO_HEADER_INDEX))
    {
        return std::wstring();
    }
    return std::wstring(buffer.data());
}

bool GetContentLength(HINTERNET request, ULONGLONG* length)
{
    if (length == nullptr) return false;
    const std::wstring value = QueryHeaderString(request, WINHTTP_QUERY_CONTENT_LENGTH);
    if (value.empty()) return false;
    wchar_t* end = nullptr;
    const ULONGLONG parsed = _wcstoui64(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != L'\0') return false;
    *length = parsed;
    return true;
}

bool IsRedirect(DWORD status)
{
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

bool OpenResponse(const std::wstring& initialUrl, HINTERNET* sessionOut, HINTERNET* connectionOut,
                  HINTERNET* requestOut, std::wstring* finalUrl, DWORD* statusOut,
                  std::wstring* error, volatile LONG* cancelFlag, bool requireHttps)
{
    if (sessionOut == nullptr || connectionOut == nullptr || requestOut == nullptr ||
        finalUrl == nullptr || statusOut == nullptr)
    {
        SetError(error, L"The HTTP request could not be initialized.");
        return false;
    }
    *sessionOut = nullptr;
    *connectionOut = nullptr;
    *requestOut = nullptr;
    std::wstring currentUrl = initialUrl;

    for (int redirect = 0; redirect <= 10; ++redirect)
    {
        if (IsCancelled(cancelFlag))
        {
            SetError(error, L"The HTTP request was cancelled.");
            return false;
        }
        ParsedUrl parsed;
        if (!CrackUrl(currentUrl, &parsed) || (requireHttps && !parsed.Secure))
        {
            SetError(error, L"The HTTP address is invalid or does not use HTTPS.");
            return false;
        }

        HINTERNET session = WinHttpOpen(L"SamandarinUpdateNotifier/0.17",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (session == nullptr)
        {
            SetError(error, Win32Error(L"WinHTTP could not open a session", GetLastError()));
            return false;
        }
        WinHttpSetTimeouts(session, 5000, 5000, 5000, 15000);
        DWORD decompression = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
        WinHttpSetOption(session, WINHTTP_OPTION_DECOMPRESSION, &decompression, sizeof(decompression));

        HINTERNET connection = WinHttpConnect(session, parsed.Host.c_str(), parsed.Port, 0);
        if (connection == nullptr)
        {
            const DWORD code = GetLastError();
            WinHttpCloseHandle(session);
            SetError(error, Win32Error(L"WinHTTP could not connect", code));
            return false;
        }
        DWORD flags = parsed.Secure ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET request = WinHttpOpenRequest(connection, L"GET", parsed.PathAndQuery.c_str(), nullptr,
                                               WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (request == nullptr)
        {
            const DWORD code = GetLastError();
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            SetError(error, Win32Error(L"WinHTTP could not create a request", code));
            return false;
        }
        DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
        WinHttpSetTimeouts(request, 5000, 5000, 5000, 15000);

        static const wchar_t headers[] =
            L"User-Agent: SamandarinUpdateNotifier/0.17\r\n"
            L"Accept: application/json, */*\r\n"
            L"Accept-Encoding: gzip, deflate\r\n";
        if (!WinHttpSendRequest(request, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(request, nullptr))
        {
            const DWORD code = GetLastError();
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            SetError(error, Win32Error(L"WinHTTP could not receive a response", code));
            return false;
        }

        DWORD status = 0;
        DWORD statusSize = sizeof(status);
        if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX))
        {
            const DWORD code = GetLastError();
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            SetError(error, Win32Error(L"WinHTTP could not read the response status", code));
            return false;
        }

        if (IsRedirect(status))
        {
            const std::wstring location = QueryHeaderString(request, WINHTTP_QUERY_LOCATION);
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            if (location.empty() || redirect == 10)
            {
                SetError(error, L"The HTTP redirect could not be followed.");
                return false;
            }
            currentUrl = ResolveRedirect(currentUrl, location);
            if (currentUrl.empty())
            {
                SetError(error, L"The HTTP redirect address is invalid.");
                return false;
            }
            continue;
        }

        if (status < 200 || status >= 300)
        {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            SetError(error, L"HTTP response " + std::to_wstring(status));
            return false;
        }

        *sessionOut = session;
        *connectionOut = connection;
        *requestOut = request;
        *finalUrl = currentUrl;
        *statusOut = status;
        return true;
    }
    SetError(error, L"Too many HTTP redirects.");
    return false;
}

void CloseRequest(HINTERNET session, HINTERNET connection, HINTERNET request)
{
    if (request != nullptr) WinHttpCloseHandle(request);
    if (connection != nullptr) WinHttpCloseHandle(connection);
    if (session != nullptr) WinHttpCloseHandle(session);
}

bool ReadResponse(HINTERNET request, size_t maximumBytes, std::string* body,
                  std::wstring* error, volatile LONG* cancelFlag)
{
    if (body == nullptr) return false;
    body->clear();
    ULONGLONG contentLength = 0;
    if (GetContentLength(request, &contentLength) && contentLength > maximumBytes)
    {
        SetError(error, L"The HTTP response exceeds the allowed size.");
        return false;
    }
    for (;;)
    {
        if (IsCancelled(cancelFlag))
        {
            SetError(error, L"The HTTP request was cancelled.");
            return false;
        }
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available))
        {
            SetError(error, Win32Error(L"WinHTTP could not read response data", GetLastError()));
            return false;
        }
        if (available == 0) return true;
        if (available > maximumBytes || body->size() > maximumBytes - available)
        {
            SetError(error, L"The HTTP response exceeds the allowed size.");
            return false;
        }
        const size_t start = body->size();
        body->resize(start + available);
        DWORD read = 0;
        if (!WinHttpReadData(request, &(*body)[start], available, &read))
        {
            body->resize(start);
            SetError(error, Win32Error(L"WinHTTP could not read response data", GetLastError()));
            return false;
        }
        body->resize(start + read);
        if (read == 0) return true;
    }
}

bool ReadResponseToFile(HINTERNET request, HANDLE file, size_t maximumBytes,
                        std::wstring* error, volatile LONG* cancelFlag)
{
    ULONGLONG contentLength = 0;
    if (GetContentLength(request, &contentLength) && contentLength > maximumBytes)
    {
        SetError(error, L"The package exceeds the allowed download size.");
        return false;
    }
    size_t total = 0;
    for (;;)
    {
        if (IsCancelled(cancelFlag))
        {
            SetError(error, L"The package download was cancelled.");
            return false;
        }
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available))
        {
            SetError(error, Win32Error(L"WinHTTP could not read package data", GetLastError()));
            return false;
        }
        if (available == 0) return true;
        if (available > maximumBytes || total > maximumBytes - available)
        {
            SetError(error, L"The package exceeds the allowed download size.");
            return false;
        }
        const DWORD chunkSize = (std::min)(available, static_cast<DWORD>(64 * 1024));
        std::vector<char> buffer(chunkSize);
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), chunkSize, &read))
        {
            SetError(error, Win32Error(L"WinHTTP could not read package data", GetLastError()));
            return false;
        }
        if (read == 0) return true;
        DWORD written = 0;
        if (!WriteFile(file, buffer.data(), read, &written, nullptr) || written != read)
        {
            SetError(error, Win32Error(L"The package file could not be written", GetLastError()));
            return false;
        }
        total += read;
    }
}
} // namespace

bool HttpGet(const std::wstring& url, size_t maximumBytes, HttpResponse* response,
             std::wstring* error, volatile LONG* cancelFlag, bool appendNoCache, bool requireHttps)
{
    if (response == nullptr) return false;
    *response = HttpResponse();
    const std::wstring requestUrl = appendNoCache ? AddRefreshQuery(url) : url;
    HINTERNET session = nullptr, connection = nullptr, request = nullptr;
    std::wstring finalUrl;
    DWORD status = 0;
    if (!OpenResponse(requestUrl, &session, &connection, &request, &finalUrl, &status,
                      error, cancelFlag, requireHttps))
    {
        return false;
    }
    const bool success = ReadResponse(request, maximumBytes, &response->Body, error, cancelFlag);
    if (success)
    {
        response->StatusCode = status;
        response->FinalUrl = finalUrl;
    }
    CloseRequest(session, connection, request);
    return success;
}

bool HttpDownloadToFile(const std::wstring& url, const std::wstring& destination,
                        size_t maximumBytes, std::wstring* error, volatile LONG* cancelFlag)
{
    HINTERNET session = nullptr, connection = nullptr, request = nullptr;
    std::wstring finalUrl;
    DWORD status = 0;
    if (!OpenResponse(url, &session, &connection, &request, &finalUrl, &status,
                      error, cancelFlag, true))
    {
        return false;
    }
    HANDLE file = CreateFileW(AddLongPathPrefix(destination).c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        const DWORD code = GetLastError();
        CloseRequest(session, connection, request);
        SetError(error, Win32Error(L"The package file could not be created", code));
        return false;
    }
    bool success = ReadResponseToFile(request, file, maximumBytes, error, cancelFlag);
    if (!FlushFileBuffers(file)) success = false;
    CloseHandle(file);
    CloseRequest(session, connection, request);
    if (!success) DeleteFileW(AddLongPathPrefix(destination).c_str());
    return success;
}

bool IsAbsoluteHttpUrl(const std::wstring& url)
{
    ParsedUrl parsed;
    return CrackUrl(url, &parsed);
}
} // namespace SamandarinNative
