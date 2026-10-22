// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// Only the OS clipboard boundary and viewer file I/O are replaced. The selection,
// conversion-table transform, decoder, command, and clipboard encoders are production code.
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include "../common/unicode/ViewerBomText.h"

static int Checks = 0;
static int Failures = 0;
static int UnexpectedDialogs = 0;
static std::map<UINT, HGLOBAL> Clipboard;
static bool ClipboardOpen = false;
static BOOL IdleRefreshStates = FALSE;
static BOOL IdleCheckClipboard = FALSE;

static void Check(bool success, const std::string& message)
{
    ++Checks;
    if (!success)
    {
        ++Failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

static void ClearClipboard()
{
    for (const auto& item : Clipboard)
        GlobalFree(item.second);
    Clipboard.clear();
}

static BOOL TestOpenClipboard(HWND)
{
    Check(!ClipboardOpen, "balanced OpenClipboard");
    ClipboardOpen = true;
    return TRUE;
}

static BOOL TestEmptyClipboard()
{
    Check(ClipboardOpen, "EmptyClipboard requires an open clipboard");
    ClearClipboard();
    return TRUE;
}

static HANDLE TestSetClipboardData(UINT format, HANDLE memory)
{
    Check(ClipboardOpen, "SetClipboardData requires an open clipboard");
    Check(format == CF_TEXT || format == CF_UNICODETEXT, "only text formats are published");
    if (Clipboard.count(format))
        GlobalFree(Clipboard[format]);
    Clipboard[format] = memory;
    return memory;
}

static BOOL TestCloseClipboard()
{
    Check(ClipboardOpen, "balanced CloseClipboard");
    ClipboardOpen = false;
    return TRUE;
}

// Give UTF-8-manifest CI a deterministic regional Windows code page. GetACP and
// all MultiByteToWideChar/WideCharToMultiByte calls remain real Windows APIs.
static int WINAPI TestGetLocaleInfoW(LCID locale, LCTYPE type, LPWSTR value, int capacity)
{
    if (locale == LOCALE_SYSTEM_DEFAULT && type == LOCALE_IDEFAULTANSICODEPAGE)
    {
        const wchar_t regional[] = L"1250";
        if (capacity >= (int)_countof(regional))
        {
            memcpy(value, regional, sizeof(regional));
            return (int)_countof(regional);
        }
        return 0;
    }
    return GetLocaleInfoW(locale, type, value, capacity);
}

enum { vtText, vtHex, APROX_LINE_LEN = 1000 };
enum { IDS_ERRORTITLE, IDS_COPYTOCLIPBOARD, IDS_TEXTCOPIED, IDS_INFOTITLE };
static const char* LoadStr(int) { return "test diagnostic"; }
static const char* GetErrorText(DWORD) { return "test error"; }
static int SalMessageBox(HWND, const char*, const char*, UINT) { ++UnexpectedDialogs; return IDOK; }
static int SalMessageBoxViewerPaintBlocked(HWND, const char*, const char*, UINT) { ++UnexpectedDialogs; return IDOK; }

class CViewerWindow
{
public:
    HWND HWindow = NULL;
    BOOL MouseDrag = FALSE;
    int Type = vtText;
    __int64 StartSelection = 0, EndSelection = 0, FileSize = 0, Seek = 0;
    __int64 TextContentOffset = 0;
    Salamander::Unicode::BomEncoding TextEncoding = Salamander::Unicode::BomEncoding::LegacyBytes;
    BOOL UseCodeTable = FALSE;
    char CodeTable[256] = {};
    unsigned char* Buffer = NULL;
    std::vector<unsigned char> Storage;
    int FatalErrors = 0;
    int PrepareCalls = 0;

    void Load(const std::string& bytes)
    {
        Storage.assign(bytes.begin(), bytes.end());
        Buffer = Storage.data();
        FileSize = (__int64)Storage.size();
        Seek = StartSelection = 0;
        EndSelection = FileSize;
        if (!Storage.empty())
            CodeCharacters(Buffer, Buffer + Storage.size());
    }

    __int64 Prepare(HANDLE*, __int64 offset, __int64 bytes, BOOL& fatalErr)
    {
        ++PrepareCalls;
        fatalErr = FALSE;
        return offset >= 0 && offset < FileSize ? min(bytes, FileSize - offset) : 0;
    }
    BOOL HasDecodedTextMode() const { return Type == vtText && Salamander::Unicode::IsDecodedEncoding(TextEncoding); }
    BOOL HasDecodedTextEncoding() const { return Salamander::Unicode::IsDecodedEncoding(TextEncoding); }
    __int64 TextStartOffset() const { return HasDecodedTextEncoding() ? TextContentOffset : 0; }
    BOOL CheckSelectionIsNotTooBig(HWND) { return TRUE; }
    void FatalFileErrorOccured() { ++FatalErrors; }
    void CodeCharacters(unsigned char*, unsigned char*);
    HGLOBAL GetSelectedText(BOOL& fatalErr);
    HGLOBAL GetSelectedTextW(BOOL& fatalErr, int* textLen);
    BOOL DecodeTextRange(HANDLE*, __int64, __int64, Salamander::Unicode::DecodedRun&, BOOL&, bool flush = true);
    LRESULT CopyCommand();
};

#define HANDLES(expression) (expression)
#define NOHANDLES(expression) (expression)
#define TRACE_E(expression) ((void)0)
#define CALL_STACK_MESSAGE1(expression) ((void)0)
#define OpenClipboard TestOpenClipboard
#define EmptyClipboard TestEmptyClipboard
#define SetClipboardData TestSetClipboardData
#define CloseClipboard TestCloseClipboard
#include "viewer_clipboard_generated.h"
#undef OpenClipboard
#undef EmptyClipboard
#undef SetClipboardData
#undef CloseClipboard

static std::wstring Decode(const std::string& bytes, UINT codePage)
{
    if (bytes.empty())
        return {};
    int length = MultiByteToWideChar(codePage, 0, bytes.data(), (int)bytes.size(), NULL, 0);
    std::wstring result(length, L'\0');
    Check(length > 0 && MultiByteToWideChar(codePage, 0, bytes.data(), (int)bytes.size(),
                                          result.data(), length) == length, "fixture decodes");
    return result;
}

static std::string Encode(const std::wstring& text, UINT codePage)
{
    if (text.empty())
        return {};
    int length = WideCharToMultiByte(codePage, 0, text.data(), (int)text.size(), NULL, 0, NULL, NULL);
    std::string result(length, '\0');
    Check(length > 0 && WideCharToMultiByte(codePage, 0, text.data(), (int)text.size(),
                                          result.data(), length, NULL, NULL) == length, "fixture encodes");
    return result;
}

template<class Character>
static void CheckFormat(UINT format, const std::basic_string<Character>& expected, const std::string& name)
{
    auto found = Clipboard.find(format);
    Check(found != Clipboard.end(), name + " format is present");
    if (found == Clipboard.end())
        return;
    HGLOBAL memory = found->second;
    SIZE_T required = (expected.size() + 1) * sizeof(Character);
    Check(GlobalSize(memory) >= required, name + " allocation includes terminator");
    auto value = (const Character*)GlobalLock(memory);
    Check(value != NULL, name + " memory locks");
    if (value != NULL)
    {
        if (GlobalSize(memory) >= required)
        {
            Check(memcmp(value, expected.data(), expected.size() * sizeof(Character)) == 0,
                  name + " exact content");
            Check(value[expected.size()] == 0, name + " terminator");
        }
        GlobalUnlock(memory);
    }
}

static void CheckExport(CViewerWindow& viewer, const std::string& expectedBytes,
                        const std::wstring& expectedWide, const char* name)
{
    viewer.CopyCommand();
    Check(!ClipboardOpen, std::string(name) + " closes clipboard");
    Check(viewer.FatalErrors == 0, std::string(name) + " no fatal error");
    CheckFormat(CF_TEXT, expectedBytes, std::string(name) + " CF_TEXT");
    CheckFormat(CF_UNICODETEXT, expectedWide, std::string(name) + " CF_UNICODETEXT");
}

static void TestLegacy()
{
    CViewerWindow viewer;
    viewer.Load("ASCII\r\n ");
    CheckExport(viewer, "ASCII\r\n ", L"ASCII\r\n ", "ASCII");
    if (GetEffectiveConversionCodePage() != 1250)
    {
        std::cout << "SKIP: CP1250 fixture cases require ACP1250 or the UTF-8 test manifest.\n";
        return;
    }
    const std::string issue("B\xEC" "lou\xE8" "k\xFD" " k\xF9\xF2\r\n ");
    viewer.Load(issue);
    CheckExport(viewer, issue, L"Běloučký kůň\r\n ", "issue #740 CP1250");
    viewer.StartSelection = 12;
    viewer.EndSelection = 9;
    CheckExport(viewer, issue.substr(9, 3), L"kůň", "reverse partial selection");
    viewer.Load(std::string("\xC3\xA1", 2));
    CheckExport(viewer, std::string("\xC3\xA1", 2), L"\u0102\u02C7", "legacy bytes also valid UTF8");
    viewer.Load(std::string("A\0\xEC" "B", 4));
    CheckExport(viewer, std::string("A\0\xEC" "B", 4), std::wstring(L"A\0ěB", 4), "explicit selection length");
    std::string large;
    std::wstring largeWide;
    for (int i = 0; i < 400; ++i)
    {
        large += issue;
        largeWide += L"Běloučký kůň\r\n ";
    }
    viewer.Load(large);
    int before = viewer.PrepareCalls;
    CheckExport(viewer, large, largeWide, "selection spans read chunks");
    Check(viewer.PrepareCalls - before > 1, "large selection exercised several production copy iterations");

    std::ifstream stream("convert/centeuro/c8521250.tab", std::ios::binary);
    std::string table((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    Check(table.size() == 256, "shipped CP852 conversion table is exactly 256 binary bytes");
    if (table.size() == 256)
    {
        memcpy(viewer.CodeTable, table.data(), 256);
        viewer.UseCodeTable = TRUE;
        viewer.Load(Encode(L"Běloučký kůň\r\n ", 852));
        CheckExport(viewer, issue, L"Běloučký kůň\r\n ", "selected CP852 to CP1250 table");
    }
}

static void TestDecoded()
{
    const std::wstring text = L"Aě日本\U0001F642\r\n ";
    for (auto encoding : {Salamander::Unicode::BomEncoding::Utf8,
                          Salamander::Unicode::BomEncoding::Utf16Le,
                          Salamander::Unicode::BomEncoding::Utf16Be})
    {
        std::string bytes;
        int offset;
        if (encoding == Salamander::Unicode::BomEncoding::Utf8)
        {
            bytes = "\xEF\xBB\xBF" + Encode(text, CP_UTF8);
            offset = 3;
        }
        else
        {
            bool little = encoding == Salamander::Unicode::BomEncoding::Utf16Le;
            bytes = little ? "\xFF\xFE" : "\xFE\xFF";
            for (wchar_t c : text)
            {
                bytes += (char)(little ? c & 255 : c >> 8);
                bytes += (char)(little ? c >> 8 : c & 255);
            }
            offset = 2;
        }
        CViewerWindow viewer;
        viewer.TextEncoding = encoding;
        viewer.TextContentOffset = offset;
        viewer.Load(bytes);
        CheckExport(viewer, Encode(text, CP_ACP), text, "decoded Unicode full selection excludes BOM");
        // Select precisely the two UTF-8 bytes / one UTF-16 code unit for ě.
        viewer.StartSelection = offset + (encoding == Salamander::Unicode::BomEncoding::Utf8 ? 3 : 4);
        viewer.EndSelection = offset + (encoding == Salamander::Unicode::BomEncoding::Utf8 ? 1 : 2);
        CheckExport(viewer, Encode(L"ě", CP_ACP), L"ě", "decoded Unicode partial reverse selection");
    }
}

static void TestUnchangedCallers()
{
    const std::string raw("B\xEC" "lou\xE8" "k\xFD");
    CViewerWindow hex;
    hex.Type = vtHex;
    hex.TextEncoding = Salamander::Unicode::BomEncoding::Utf8;
    hex.Load(raw);
    CheckExport(hex, raw, Decode(raw, CP_ACP), "hex retains existing ACP interpretation");

    const std::wstring wide = L"Běloučký kůň";
    const std::string bytes = Encode(wide, CP_ACP);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes.size() + 1);
    auto destination = (char*)GlobalLock(memory);
    memcpy(destination, bytes.c_str(), bytes.size() + 1);
    GlobalUnlock(memory);
    Check(CopyHTextToClipboard(memory), "default caller succeeds without source code page argument");
    CheckFormat(CF_TEXT, bytes, "default caller CF_TEXT");
    CheckFormat(CF_UNICODETEXT, Decode(bytes, CP_ACP), "default caller CF_UNICODETEXT");

    CViewerWindow empty;
    empty.Load("x");
    empty.EndSelection = 0;
    HGLOBAL existing = Clipboard[CF_UNICODETEXT];
    empty.CopyCommand();
    Check(Clipboard[CF_UNICODETEXT] == existing, "empty selection leaves clipboard untouched");
    empty.EndSelection = 1;
    empty.MouseDrag = TRUE;
    empty.CopyCommand();
    Check(Clipboard[CF_UNICODETEXT] == existing, "active mouse drag leaves clipboard untouched");
}

int main()
{
    std::cout << "ACP=" << GetACP() << ", effective regional code page=" << GetEffectiveConversionCodePage() << '\n';
    TestLegacy();
    TestDecoded();
    TestUnchangedCallers();
    Check(UnexpectedDialogs == 0, "no error dialogs requested");
    Check(IdleRefreshStates && IdleCheckClipboard, "clipboard refresh state preserved");
    ClearClipboard();
    std::cout << Checks << " checks, " << Failures << " failures; OS clipboard was never opened.\n";
    return Failures == 0 ? 0 : 1;
}
