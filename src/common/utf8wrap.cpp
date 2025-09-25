#define UTF8WRAPPERS_IMPLEMENTATION
#include "precomp.h"

#include <cstring>
#include <string>
#include <vector>

namespace
{
bool UsingUtf8ACP()
{
    static int cached = -1;
    if (cached == -1)
        cached = (GetACP() == CP_UTF8) ? 1 : 0;
    return cached == 1;
}

struct Utf8Decoded
{
    std::wstring wide;
    std::vector<int> codepointByteEnds;
    std::vector<int> codepointWideEnds;
    std::vector<int> byteToCodepoint;
};

void DecodeUtf8(const char* str, int length, Utf8Decoded& out)
{
    out.wide.clear();
    out.codepointByteEnds.clear();
    out.codepointWideEnds.clear();
    if (str == NULL)
    {
        out.byteToCodepoint.clear();
        return;
    }
    if (length < 0)
        length = (int)strlen(str);
    out.byteToCodepoint.assign(length, -1);

    int index = 0;
    int codepointIndex = 0;
    while (index < length)
    {
        int start = index;
        unsigned char c = static_cast<unsigned char>(str[index]);
        unsigned int code = 0xFFFD;
        int advance = 1;

        if (c < 0x80)
        {
            code = c;
        }
        else if ((c & 0xE0) == 0xC0 && index + 1 < length)
        {
            unsigned char b1 = static_cast<unsigned char>(str[index + 1]);
            if ((b1 & 0xC0) == 0x80)
            {
                unsigned int candidate = ((c & 0x1F) << 6) | (b1 & 0x3F);
                if (candidate >= 0x80)
                {
                    code = candidate;
                    advance = 2;
                }
            }
        }
        else if ((c & 0xF0) == 0xE0 && index + 2 < length)
        {
            unsigned char b1 = static_cast<unsigned char>(str[index + 1]);
            unsigned char b2 = static_cast<unsigned char>(str[index + 2]);
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80)
            {
                unsigned int candidate = ((c & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
                if (candidate >= 0x800 && !(candidate >= 0xD800 && candidate <= 0xDFFF))
                {
                    code = candidate;
                    advance = 3;
                }
            }
        }
        else if ((c & 0xF8) == 0xF0 && index + 3 < length)
        {
            unsigned char b1 = static_cast<unsigned char>(str[index + 1]);
            unsigned char b2 = static_cast<unsigned char>(str[index + 2]);
            unsigned char b3 = static_cast<unsigned char>(str[index + 3]);
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80)
            {
                unsigned int candidate = ((c & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
                if (candidate >= 0x10000 && candidate <= 0x10FFFF)
                {
                    code = candidate;
                    advance = 4;
                }
            }
        }

        for (int b = 0; b < advance && start + b < length; ++b)
        {
            out.byteToCodepoint[start + b] = codepointIndex;
        }

        index += advance;
        out.codepointByteEnds.push_back(index);

        if (code <= 0xFFFF)
        {
            out.wide.push_back(static_cast<WCHAR>(code));
            out.codepointWideEnds.push_back(static_cast<int>(out.wide.size()) - 1);
        }
        else
        {
            code -= 0x10000;
            WCHAR high = static_cast<WCHAR>((code >> 10) + 0xD800);
            WCHAR low = static_cast<WCHAR>((code & 0x3FF) + 0xDC00);
            out.wide.push_back(high);
            out.wide.push_back(low);
            out.codepointWideEnds.push_back(static_cast<int>(out.wide.size()) - 1);
        }
        ++codepointIndex;
    }
}

bool Utf8ToWide(const char* str, int count, std::wstring& wide)
{
    if (str == NULL)
    {
        wide.clear();
        return true;
    }
    if (count == 0)
    {
        wide.clear();
        return true;
    }
    if (count == -1)
    {
        int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
        if (required == 0)
            required = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        if (required == 0)
            return false;
        wide.resize(required - 1);
        if (required > 1)
        {
            int written = MultiByteToWideChar(CP_UTF8, 0, str, -1, wide.data(), required);
            if (written > 0)
                wide.resize(written - 1);
            else
                wide.clear();
        }
        else
            wide.clear();
        return true;
    }
    if (count < 0)
        count = (int)strlen(str);
    if (count == 0)
    {
        wide.clear();
        return true;
    }
    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, count, NULL, 0);
    if (required == 0)
        required = MultiByteToWideChar(CP_UTF8, 0, str, count, NULL, 0);
    if (required == 0)
        return false;
    wide.resize(required);
    int written = MultiByteToWideChar(CP_UTF8, 0, str, count, wide.data(), required);
    if (written > 0 && written <= required)
        wide.resize(written);
    else
        wide.clear();
    return written > 0;
}

} // namespace

BOOL WINAPI SalExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* rect, LPCSTR str, UINT count, const INT* dx)
{
    if (!UsingUtf8ACP())
        return ::ExtTextOutA(hdc, x, y, options, rect, str, count, dx);

    std::wstring wide;
    if (!Utf8ToWide(str, static_cast<int>(count), wide))
        return ::ExtTextOutA(hdc, x, y, options, rect, str, count, dx);

    return ::ExtTextOutW(hdc, x, y, options, rect, wide.empty() ? L"" : wide.c_str(), static_cast<UINT>(wide.size()), dx);
}

BOOL WINAPI SalTextOutA(HDC hdc, int x, int y, LPCSTR str, int count)
{
    if (!UsingUtf8ACP())
        return ::TextOutA(hdc, x, y, str, count);

    std::wstring wide;
    if (!Utf8ToWide(str, count, wide))
        return ::TextOutA(hdc, x, y, str, count);

    return ::TextOutW(hdc, x, y, wide.empty() ? L"" : wide.c_str(), static_cast<int>(wide.size()));
}

BOOL WINAPI SalGetTextExtentPoint32A(HDC hdc, LPCSTR str, int count, LPSIZE size)
{
    if (!UsingUtf8ACP())
        return ::GetTextExtentPoint32A(hdc, str, count, size);

    std::wstring wide;
    if (!Utf8ToWide(str, count, wide))
        return ::GetTextExtentPoint32A(hdc, str, count, size);

    return ::GetTextExtentPoint32W(hdc, wide.empty() ? L"" : wide.c_str(), static_cast<int>(wide.size()), size);
}

BOOL WINAPI SalGetTextExtentExPointA(HDC hdc, LPCSTR str, int count, int maxExtent, LPINT fit, LPINT dx, LPSIZE size)
{
    if (!UsingUtf8ACP())
        return ::GetTextExtentExPointA(hdc, str, count, maxExtent, fit, dx, size);

    Utf8Decoded decoded;
    DecodeUtf8(str, count, decoded);

    SIZE measured = {0, 0};
    if (decoded.wide.empty())
    {
        if (size)
            *size = measured;
        if (fit)
            *fit = 0;
        if (dx && count > 0)
            ZeroMemory(dx, sizeof(int) * count);
        return TRUE;
    }

    std::vector<int> wideDx(decoded.wide.size());
    BOOL ok = ::GetTextExtentExPointW(hdc, decoded.wide.data(), static_cast<int>(decoded.wide.size()), maxExtent, NULL, wideDx.data(), &measured);
    if (!ok)
        return FALSE;

    if (size)
        *size = measured;

    std::vector<int> codepointWidths(decoded.codepointWideEnds.size());
    for (size_t i = 0; i < decoded.codepointWideEnds.size(); ++i)
    {
        codepointWidths[i] = wideDx[decoded.codepointWideEnds[i]];
    }

    if (fit)
    {
        int byteFit = 0;
        if (maxExtent > 0)
        {
            int wideFit = 0;
            for (size_t i = 0; i < wideDx.size(); ++i)
            {
                if (wideDx[i] <= maxExtent)
                    wideFit = static_cast<int>(i) + 1;
                else
                    break;
            }
            int cpFit = 0;
            for (size_t i = 0; i < decoded.codepointWideEnds.size(); ++i)
            {
                if (decoded.codepointWideEnds[i] < wideFit)
                    cpFit = static_cast<int>(i) + 1;
                else
                    break;
            }
            if (cpFit > 0)
                byteFit = decoded.codepointByteEnds[cpFit - 1];
        }
        else if (maxExtent == 0)
        {
            byteFit = 0;
        }
        else
        {
            if (!decoded.codepointByteEnds.empty())
                byteFit = decoded.codepointByteEnds.back();
        }
        if (count >= 0 && byteFit > count)
            byteFit = count;
        *fit = byteFit;
    }

    if (dx)
    {
        if (count < 0)
            count = static_cast<int>(decoded.byteToCodepoint.size());
        for (int i = 0; i < count; ++i)
        {
            int cpIndex = (i >= 0 && i < static_cast<int>(decoded.byteToCodepoint.size())) ? decoded.byteToCodepoint[i] : -1;
            if (cpIndex >= 0 && cpIndex < static_cast<int>(codepointWidths.size()))
                dx[i] = codepointWidths[cpIndex];
            else
                dx[i] = 0;
        }
    }

    return TRUE;
}

int WINAPI SalDrawTextA(HDC hdc, LPCSTR str, int count, LPRECT rect, UINT format)
{
    if (!UsingUtf8ACP())
        return ::DrawTextA(hdc, str, count, rect, format);

    std::wstring wide;
    if (!Utf8ToWide(str, count, wide))
        return ::DrawTextA(hdc, str, count, rect, format);

    return ::DrawTextW(hdc, wide.empty() ? L"" : wide.c_str(), static_cast<int>(wide.size()), rect, format);
}

int WINAPI SalDrawTextExA(HDC hdc, LPTSTR str, int count, LPRECT rect, UINT format, LPDRAWTEXTPARAMS params)
{
    if (!UsingUtf8ACP())
        return ::DrawTextExA(hdc, str, count, rect, format, params);

    std::wstring wide;
    if (!Utf8ToWide(str, count, wide))
        return ::DrawTextExA(hdc, str, count, rect, format, params);

    std::wstring mutableWide = wide;
    LPWSTR buffer = mutableWide.empty() ? nullptr : &mutableWide[0];
    if (buffer == nullptr)
    {
        static WCHAR zero = 0;
        buffer = &zero;
    }

    int res = ::DrawTextExW(hdc, buffer, static_cast<int>(mutableWide.size()), rect, format, params);

    if ((format & DT_MODIFYSTRING) != 0 && str != NULL)
    {
        // attempt to propagate modifications back to the ANSI buffer
        const WCHAR* wideSrc = buffer;
        int bytesAvailable = count;
        if (bytesAvailable < 0 && str != NULL)
            bytesAvailable = (int)strlen(str) + 1;
        if (bytesAvailable <= 0)
            bytesAvailable = 0;
        if (bytesAvailable > 0)
        {
            int converted = WideCharToMultiByte(CP_UTF8, 0, wideSrc, -1, str, bytesAvailable, NULL, NULL);
            if (converted == 0 && bytesAvailable > 0)
                str[bytesAvailable - 1] = '\0';
        }
    }

    return res;
}

