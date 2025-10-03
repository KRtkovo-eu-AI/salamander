// SPDX-FileCopyrightText: 2023-2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023-2024 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#include "precomp.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

// objekt interfacu pluginu, jeho metody se volaji ze Salamandera
CPluginInterface PluginInterface;
// cast interfacu CPluginInterface pro viewer
CPluginInterfaceForViewer InterfaceForViewer;

// globalni data
const char* PluginNameEN = "PrismSharp Text Viewer .NET"; // neprekladane jmeno pluginu
const char* PluginNameShort = "TEXTVIEWER";    // jmeno pluginu (kratce, bez mezer)

HINSTANCE DLLInstance = NULL; // handle k SPL-ku - jazykove nezavisle resourcy
HINSTANCE HLanguage = NULL;   // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;

// definice promenne pro "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// maximum file size (in bytes) allowed for the managed viewer
static const ULONGLONG kMaxTextFileSize = 16ULL * 1024ULL * 1024ULL; // 16 MB

// definice promenne pro "spl_com.h"
int SalamanderVersion = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES;
        if (!InitCommonControlsEx(&initCtrls))
        {
            MessageBox(NULL, "InitCommonControlsEx failed!", "Error", MB_OK | MB_ICONERROR);
            return FALSE; // DLL won't start
        }
    }

    return TRUE; // DLL can be loaded
}

// ****************************************************************************

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

static void ShowStartupError(HWND parent, const char* text)
{
    SalamanderGeneral->SalMessageBox(parent, text, LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
}

static bool IsFileTooLarge(const char* path, ULONGLONG limit)
{
    if (path == NULL || path[0] == '\0')
        return false;

    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attrs))
        return false;

    if (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;

    ULONGLONG size = (static_cast<ULONGLONG>(attrs.nFileSizeHigh) << 32) | attrs.nFileSizeLow;
    return size > limit;
}

//
// ****************************************************************************
// SalamanderPluginGetReqVer
//

#ifdef __BORLANDC__
extern "C"
{
    int WINAPI SalamanderPluginGetReqVer();
    CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander);
};
#endif // __BORLANDC__

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

//
// ****************************************************************************
// SalamanderPluginEntry
//

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    // nastavime SalamanderDebug pro "dbg.h"
    SalamanderDebug = salamander->GetSalamanderDebug();
    // nastavime SalamanderVersion pro "spl_com.h"
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();
    CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBox(salamander->GetParentWindow(),
                   REQUIRE_LAST_VERSION_OF_SALAMANDER,
                   PluginNameEN, MB_OK | MB_ICONERROR);
        return NULL;
    }

    // nechame nacist jazykovy modul (.slg)
    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), PluginNameEN);
    if (HLanguage == NULL)
        return NULL;

    // ziskame obecne rozhrani Salamandera
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();

    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME), FUNCTION_VIEWER,
                                   VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION), PluginNameShort,
                                   NULL, NULL);

    salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

    return &PluginInterface;
}

//
// ****************************************************************************
// CPluginInterface
//

void WINAPI CPluginInterface::About(HWND parent)
{
    char text[1024];
    _snprintf_s(text, _TRUNCATE,
                "%s\n\n%s",
                LoadStr(IDS_PLUGINNAME),
                LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->SalMessageBox(parent, text, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    if (!ManagedBridge_RequestShutdown(parent, force != FALSE))
        return FALSE;

    ManagedBridge_Shutdown();
    return TRUE;
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    static const char* const kBaseExtensions[] = {
        "txt",
        "log",
        "ini",
        "cfg",
        "conf",
        "config",
        "json",
        "jsonc",
        "json5",
        "yaml",
        "yml",
        "xml",
        "html",
        "htm",
        "md",
        "markdown",
        "csv",
        "bat",
        "cmd",
        "ps1",
        "psd1",
        "psm1",
        "cs",
        "cpp",
        "c",
        "cxx",
        "h",
        "hh",
        "hpp",
        "hxx",
        "csproj",
        "fsproj",
        "vbproj",
        "vcxproj",
        "vcproj",
        "axaml",
        "xaml",
        "xlf",
        "nuspec",
        "plist",
        "props",
        "storyboard",
        "targets"
    };

    static const char* const kPrismExtensions[] = {
        "abap",
        "abnf",
        "actionscript",
        "ada",
        "adoc",
        "agda",
        "al",
        "antlr4",
        "apacheconf",
        "apex",
        "apl",
        "applescript",
        "aql",
        "arduino",
        "arff",
        "asciidoc",
        "asm6502",
        "asmatmel",
        "aspnet",
        "atom",
        "autohotkey",
        "autoit",
        "avdl",
        "avisynth",
        "avs",
        "bash",
        "basic",
        "batch",
        "bbcode",
        "bicep",
        "birb",
        "bison",
        "bnf",
        "brainfuck",
        "brightscript",
        "bro",
        "bsl",
        "c",
        "cfc",
        "cfscript",
        "chaiscript",
        "cil",
        "clike",
        "clojure",
        "cmake",
        "cobol",
        "coffee",
        "coffeescript",
        "conc",
        "concurnas",
        "context",
        "coq",
        "cpp",
        "crystal",
        "cs",
        "csharp",
        "cshtml",
        "csp",
        "css",
        "csv",
        "cypher",
        "d",
        "dart",
        "dataweave",
        "dax",
        "dhall",
        "diff",
        "django",
        "docker",
        "dockerfile",
        "dot",
        "dotnet",
        "ebnf",
        "editorconfig",
        "eiffel",
        "ejs",
        "elisp",
        "elixir",
        "elm",
        "emacs",
        "erb",
        "erlang",
        "eta",
        "etlua",
        "factor",
        "false",
        "flow",
        "fortran",
        "fsharp",
        "ftl",
        "g4",
        "gamemakerlanguage",
        "gap",
        "gcode",
        "gdscript",
        "gedcom",
        "gherkin",
        "git",
        "gitignore",
        "glsl",
        "gml",
        "gn",
        "gni",
        "go",
        "graphql",
        "groovy",
        "gv",
        "haml",
        "handlebars",
        "haskell",
        "haxe",
        "hbs",
        "hcl",
        "hgignore",
        "hlsl",
        "hoon",
        "hpkp",
        "hs",
        "hsts",
        "html",
        "http",
        "ichigojam",
        "icon",
        "idr",
        "idris",
        "iecst",
        "ignore",
        "inform7",
        "ini",
        "ino",
        "io",
        "j",
        "java",
        "javadoc",
        "javadoclike",
        "javascript",
        "javastacktrace",
        "jexl",
        "jinja2",
        "jolie",
        "jq",
        "js",
        "jsdoc",
        "json",
        "json5",
        "jsonp",
        "jsstacktrace",
        "jsx",
        "julia",
        "keepalived",
        "keyman",
        "kotlin",
        "kt",
        "kts",
        "kum",
        "kumir",
        "kusto",
        "latex",
        "latte",
        "less",
        "lilypond",
        "liquid",
        "lisp",
        "livescript",
        "llvm",
        "log",
        "lolcode",
        "lua",
        "ly",
        "magma",
        "makefile",
        "markdown",
        "markup",
        "mathematica",
        "mathml",
        "matlab",
        "maxscript",
        "md",
        "mel",
        "mermaid",
        "mizar",
        "mongodb",
        "monkey",
        "moon",
        "moonscript",
        "mscript",
        "n1ql",
        "n4js",
        "n4jsd",
        "nani",
        "naniscript",
        "nasm",
        "nb",
        "neon",
        "nevod",
        "nginx",
        "nim",
        "nix",
        "npmignore",
        "nsis",
        "objc",
        "objectivec",
        "objectpascal",
        "ocaml",
        "opencl",
        "openqasm",
        "oscript",
        "oz",
        "parigp",
        "parser",
        "pascal",
        "pascaligo",
        "pbfasm",
        "pcaxis",
        "pcode",
        "peoplecode",
        "perl",
        "php",
        "phpdoc",
        "plsql",
        "powerquery",
        "powershell",
        "pq",
        "processing",
        "prolog",
        "promql",
        "properties",
        "protobuf",
        "psl",
        "pug",
        "puppet",
        "pure",
        "purebasic",
        "purescript",
        "purs",
        "px",
        "py",
        "python",
        "q",
        "qasm",
        "qml",
        "qore",
        "qs",
        "qsharp",
        "r",
        "racket",
        "razor",
        "rb",
        "rbnf",
        "reason",
        "regex",
        "rego",
        "renpy",
        "rest",
        "rip",
        "rkt",
        "roboconf",
        "robot",
        "robotframework",
        "rpy",
        "rq",
        "rss",
        "ruby",
        "rust",
        "sas",
        "sass",
        "scala",
        "scheme",
        "scss",
        "shell",
        "shellsession",
        "shortcode",
        "sln",
        "smali",
        "smalltalk",
        "smarty",
        "sml",
        "smlnj",
        "sol",
        "solidity",
        "soy",
        "sparql",
        "sqf",
        "sql",
        "squirrel",
        "ssml",
        "stan",
        "stylus",
        "svg",
        "swift",
        "systemd",
        "t4",
        "tap",
        "tcl",
        "tex",
        "textile",
        "toml",
        "tremor",
        "trickle",
        "trig",
        "troy",
        "ts",
        "tsconfig",
        "tsx",
        "tt2",
        "turtle",
        "twig",
        "typescript",
        "typoscript",
        "uc",
        "unrealscript",
        "uri",
        "url",
        "uscript",
        "v",
        "vala",
        "vb",
        "vba",
        "vbnet",
        "velocity",
        "verilog",
        "vhdl",
        "vim",
        "warpscript",
        "wasm",
        "webidl",
        "webmanifest",
        "wiki",
        "wl",
        "wolfram",
        "wren",
        "xeora",
        "xeoracube",
        "xls",
        "xlsx",
        "xml",
        "xojo",
        "xquery",
        "yaml",
        "yang",
        "yml",
        "zig"
    };

    std::vector<std::string> extensions;
    const size_t totalCapacity = (sizeof(kBaseExtensions) / sizeof(kBaseExtensions[0])) +
                                 (sizeof(kPrismExtensions) / sizeof(kPrismExtensions[0]));
    extensions.reserve(totalCapacity);
    std::unordered_set<std::string> seen;
    seen.reserve(totalCapacity);

    auto addExtension = [&extensions, &seen](const char* ext) {
        if (ext == nullptr || *ext == '\0')
        {
            return;
        }

        std::string normalized(ext);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (seen.insert(normalized).second)
        {
            extensions.push_back(std::move(normalized));
        }
    };

    for (const char* ext : kBaseExtensions)
    {
        addExtension(ext);
    }

    for (const char* ext : kPrismExtensions)
    {
        addExtension(ext);
    }

    if (!extensions.empty())
    {
        // Salamander limits the length of a single pattern registration string in the
        // configuration UI, so split the extensions across multiple AddViewer calls.
        constexpr size_t kMaxPatternLength = 200; // conservative chunk to keep entries editable

        std::string pattern;
        pattern.reserve(kMaxPatternLength);

        auto flushPattern = [&]() {
            if (!pattern.empty())
            {
                salamander->AddViewer(pattern.c_str(), FALSE);
                pattern.clear();
            }
        };

        for (const std::string& ext : extensions)
        {
            const std::string token = std::string("*.") + ext;
            const size_t separator = pattern.empty() ? 0 : 1; // semicolon when appending

            if (!pattern.empty() && (pattern.size() + separator + token.size()) > kMaxPatternLength)
            {
                flushPattern();
            }

            if (!pattern.empty())
            {
                pattern.push_back(';');
            }

            // If the token itself would exceed the chunk size, still register it alone.
            if (!pattern.empty() && (pattern.size() + token.size()) > kMaxPatternLength)
            {
                flushPattern();
            }

            pattern.append(token);

            if (pattern.size() >= kMaxPatternLength)
            {
                flushPattern();
            }
        }

        flushPattern();
    }

    if (SalamanderGUI != NULL)
    {
        CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
        if (iconList != NULL)
        {
            if (iconList->Create(16, 16, 1))
            {
                UINT loadFlags = SalamanderGeneral != NULL ? SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
                HICON icon16 = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_TEXTVIEWER), IMAGE_ICON, 16, 16, loadFlags);
                if (icon16 != NULL)
                {
                    iconList->ReplaceIcon(0, icon16);
                    DestroyIcon(icon16);
                    salamander->SetIconListForGUI(iconList);
                    salamander->SetPluginIcon(0);
                    salamander->SetPluginMenuAndToolbarIcon(0);
                    iconList = NULL;
                }
            }

            if (iconList != NULL)
                SalamanderGUI->DestroyIconList(iconList);
        }
    }
}

CPluginInterfaceForViewerAbstract* WINAPI CPluginInterface::GetInterfaceForViewer()
{
    return &InterfaceForViewer;
}

//
// ****************************************************************************
// CPluginInterfaceForViewer
//

BOOL WINAPI CPluginInterfaceForViewer::ViewFile(const char* name, int left, int top, int width, int height,
                                                UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                                BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                                int enumFilesSourceUID, int enumFilesCurrentIndex)
{
    CALL_STACK_MESSAGE1("CPluginInterfaceForViewer::ViewFile()");

    if (name == NULL || name[0] == '\0')
        return FALSE;

    HWND parent = SalamanderGeneral->GetMainWindowHWND();

    if (IsFileTooLarge(name, kMaxTextFileSize))
    {
        SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_FILE_TOO_LARGE), LoadStr(IDS_PLUGINNAME),
                                         MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    RECT placement;
    placement.left = left;
    placement.top = top;
    placement.right = left + width;
    placement.bottom = top + height;

    if (returnLock)
    {
        HANDLE fileLock = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
        if (fileLock == NULL)
        {
            ShowStartupError(parent, LoadStr(IDS_VIEWER_CREATE_EVENT_FAILED));
            return FALSE;
        }

        if (!ManagedBridge_ViewTextFile(parent, name, placement, showCmd, alwaysOnTop, fileLock, true))
        {
            HANDLES(CloseHandle(fileLock));
            return FALSE;
        }

        if (lock != NULL)
            *lock = fileLock;
        if (lockOwner != NULL)
            *lockOwner = TRUE;
        return TRUE;
    }

    return ManagedBridge_ViewTextFile(parent, name, placement, showCmd, alwaysOnTop, NULL, false);
}

BOOL WINAPI CPluginInterfaceForViewer::CanViewFile(const char* name)
{
    if (name == NULL)
        return FALSE;

    const char* extension = strrchr(name, '.');
    if (extension == NULL)
        return FALSE;

    static const char* const kExtensions[] = {
        ".txt", ".log", ".ini", ".cfg", ".json", ".yaml", ".yml", ".xml", ".html", ".htm", ".md",
        ".csv", ".cs", ".cpp", ".c", ".h", ".hpp", ".py", ".js", ".ts", ".css", ".sql", ".bat", ".ps1"
    };

    for (size_t i = 0; i < _countof(kExtensions); ++i)
    {
        if (_stricmp(extension, kExtensions[i]) == 0)
            return TRUE;
    }

    return FALSE;
}
