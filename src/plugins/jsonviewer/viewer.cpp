// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "jsonviewer.h"

namespace
{
const char kJsonViewerClassName[] = "SalamanderJsonViewer";

std::string FormatNodeText(const JsonNode& node)
{
    std::ostringstream oss;
    if (!node.Key.empty())
    {
        oss << node.Key << ": ";
    }

    switch (node.NodeType)
    {
    case JsonNode::Type::Object:
        oss << "{...}";
        break;
    case JsonNode::Type::Array:
        oss << "[...]";
        break;
    case JsonNode::Type::String:
        oss << '"' << node.Value << '"';
        break;
    case JsonNode::Type::Number:
        oss << node.Value;
        break;
    case JsonNode::Type::Boolean:
        oss << node.Value;
        break;
    case JsonNode::Type::Null:
        oss << "null";
        break;
    }

    return oss.str();
}
}

JsonParser::JsonParser(const std::string& text) : Text(text), Position(0) {}

std::unique_ptr<JsonNode> JsonParser::Parse()
{
    SkipWhitespace();
    auto result = ParseValue(std::string());
    SkipWhitespace();
    if (!End())
    {
        throw std::runtime_error("Unexpected trailing characters in JSON stream");
    }
    return result;
}

std::unique_ptr<JsonNode> JsonParser::ParseValue(const std::string& key)
{
    SkipWhitespace();
    if (End())
    {
        throw std::runtime_error("Unexpected end of input");
    }

    char c = Peek();
    if (c == '{')
        return ParseObject(key);
    if (c == '[')
        return ParseArray(key);
    if (c == '"')
    {
        auto node = std::make_unique<JsonNode>();
        node->Key = key;
        node->NodeType = JsonNode::Type::String;
        node->Value = ParseString();
        return node;
    }
    if (c == '-' || (c >= '0' && c <= '9'))
    {
        auto node = std::make_unique<JsonNode>();
        node->Key = key;
        node->NodeType = JsonNode::Type::Number;
        node->Value = ParseNumber();
        return node;
    }
    if (MatchLiteral("true"))
    {
        auto node = std::make_unique<JsonNode>();
        node->Key = key;
        node->NodeType = JsonNode::Type::Boolean;
        node->Value = "true";
        return node;
    }
    if (MatchLiteral("false"))
    {
        auto node = std::make_unique<JsonNode>();
        node->Key = key;
        node->NodeType = JsonNode::Type::Boolean;
        node->Value = "false";
        return node;
    }
    if (MatchLiteral("null"))
    {
        auto node = std::make_unique<JsonNode>();
        node->Key = key;
        node->NodeType = JsonNode::Type::Null;
        return node;
    }

    throw std::runtime_error("Unable to parse JSON value");
}

std::unique_ptr<JsonNode> JsonParser::ParseObject(const std::string& key)
{
    if (Get() != '{')
        throw std::runtime_error("Expected '{' at beginning of object");

    auto node = std::make_unique<JsonNode>();
    node->Key = key;
    node->NodeType = JsonNode::Type::Object;

    SkipWhitespace();
    if (Peek() == '}')
    {
        Get();
        return node;
    }

    while (true)
    {
        SkipWhitespace();
        if (Peek() != '"')
            throw std::runtime_error("Expected string property name");
        std::string propertyName = ParseString();
        SkipWhitespace();
        if (Get() != ':')
            throw std::runtime_error("Expected ':' after property name");
        SkipWhitespace();
        auto child = ParseValue(propertyName);
        node->Children.push_back(std::move(*child));
        SkipWhitespace();
        char ch = Get();
        if (ch == '}')
            break;
        if (ch != ',')
            throw std::runtime_error("Expected ',' between object members");
    }

    return node;
}

std::unique_ptr<JsonNode> JsonParser::ParseArray(const std::string& key)
{
    if (Get() != '[')
        throw std::runtime_error("Expected '[' at beginning of array");

    auto node = std::make_unique<JsonNode>();
    node->Key = key;
    node->NodeType = JsonNode::Type::Array;

    SkipWhitespace();
    if (Peek() == ']')
    {
        Get();
        return node;
    }

    int index = 0;
    while (true)
    {
        auto child = ParseValue("[" + std::to_string(index) + "]");
        node->Children.push_back(std::move(*child));
        ++index;
        SkipWhitespace();
        char ch = Get();
        if (ch == ']')
            break;
        if (ch != ',')
            throw std::runtime_error("Expected ',' between array items");
        SkipWhitespace();
    }

    return node;
}

std::string JsonParser::ParseString()
{
    if (Get() != '"')
        throw std::runtime_error("Expected string opening quote");

    std::string result;
    while (!End())
    {
        char c = Get();
        if (c == '"')
            return result;
        if (c == '\\')
        {
            if (End())
                throw std::runtime_error("Incomplete escape sequence");
            char esc = Get();
            switch (esc)
            {
            case '"':
            case '\\':
            case '/':
                result.push_back(esc);
                break;
            case 'b':
                result.push_back('\b');
                break;
            case 'f':
                result.push_back('\f');
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case 'u':
            {
                if (Position + 4 > Text.size())
                    throw std::runtime_error("Incomplete Unicode escape");
                std::string hex = Text.substr(Position, 4);
                Position += 4;
                char* endPtr = nullptr;
                long code = strtol(hex.c_str(), &endPtr, 16);
                if (endPtr == nullptr || *endPtr != '\0')
                    throw std::runtime_error("Invalid Unicode escape");
                if (code < 0x80)
                {
                    result.push_back(static_cast<char>(code));
                }
                else
                {
                    // basic UTF-8 encoding for BMP subset
                    if (code < 0x800)
                    {
                        result.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
                        result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    }
                    else
                    {
                        result.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
                        result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                        result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    }
                }
                break;
            }
            default:
                throw std::runtime_error("Unsupported escape sequence in string");
            }
        }
        else
        {
            result.push_back(c);
        }
    }

    throw std::runtime_error("Unterminated string literal");
}

std::string JsonParser::ParseNumber()
{
    size_t start = Position;
    if (Peek() == '-')
        Get();
    if (End())
        throw std::runtime_error("Unexpected end of number");
    if (Peek() == '0')
    {
        Get();
    }
    else
    {
        if (Peek() < '1' || Peek() > '9')
            throw std::runtime_error("Invalid number format");
        while (!End() && isdigit(static_cast<unsigned char>(Peek())))
            Get();
    }

    if (!End() && Peek() == '.')
    {
        Get();
        if (End() || !isdigit(static_cast<unsigned char>(Peek())))
            throw std::runtime_error("Invalid fractional part");
        while (!End() && isdigit(static_cast<unsigned char>(Peek())))
            Get();
    }

    if (!End() && (Peek() == 'e' || Peek() == 'E'))
    {
        Get();
        if (!End() && (Peek() == '+' || Peek() == '-'))
            Get();
        if (End() || !isdigit(static_cast<unsigned char>(Peek())))
            throw std::runtime_error("Invalid exponent");
        while (!End() && isdigit(static_cast<unsigned char>(Peek())))
            Get();
    }

    size_t end = Position;
    return Text.substr(start, end - start);
}

void JsonParser::SkipWhitespace()
{
    while (!End())
    {
        char c = Peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            ++Position;
        else
            break;
    }
}

bool JsonParser::MatchLiteral(const char* literal)
{
    size_t len = strlen(literal);
    if (Position + len > Text.size())
        return false;
    if (Text.compare(Position, len, literal) == 0)
    {
        Position += len;
        return true;
    }
    return false;
}

bool JsonParser::End() const
{
    return Position >= Text.size();
}

char JsonParser::Peek() const
{
    if (End())
        return '\0';
    return Text[Position];
}

char JsonParser::Get()
{
    if (End())
        return '\0';
    return Text[Position++];
}

CJsonViewerWindow::CJsonViewerWindow() : HWnd(NULL), TreeHandle(NULL)
{
}

CJsonViewerWindow::~CJsonViewerWindow()
{
}

ATOM CJsonViewerWindow::EnsureClass()
{
    static ATOM atom = 0;
    if (atom != 0)
        return atom;

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &CJsonViewerWindow::WndProc;
    wc.hInstance = DLLInstance;
    wc.hIcon = LoadIconA(DLLInstance, MAKEINTRESOURCE(IDI_JSONVIEW));
    if (wc.hIcon == NULL)
        wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kJsonViewerClassName;
    wc.cbWndExtra = sizeof(LONG_PTR);
    atom = RegisterClassExA(&wc);
    return atom;
}

bool CJsonViewerWindow::Create(const char* fileName, int left, int top, int width, int height,
                               UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock, BOOL* lockOwner)
{
    if (!EnsureClass())
        return false;

    FileName = fileName != NULL ? fileName : std::string();

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exStyle = alwaysOnTop ? WS_EX_TOPMOST : 0;

    if (width <= 0)
        width = 800;
    if (height <= 0)
        height = 600;

    HWnd = CreateWindowExA(exStyle, kJsonViewerClassName, FileName.c_str(), style,
                           left, top, width, height, NULL, NULL, DLLInstance, this);
    if (!HWnd)
        return false;

    RegisterViewerWindow(HWnd);

    if (returnLock && lock != NULL && lockOwner != NULL)
    {
        *lock = NULL;
        *lockOwner = FALSE;
    }

    ShowWindow(HWnd, showCmd == 0 ? SW_SHOWNORMAL : showCmd);
    UpdateWindow(HWnd);
    return true;
}

LRESULT CALLBACK CJsonViewerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CJsonViewerWindow* self = nullptr;
    if (msg == WM_NCCREATE)
    {
        auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<CJsonViewerWindow*>(createStruct->lpCreateParams);
        self->HWnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<CJsonViewerWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (!self)
        return DefWindowProcA(hwnd, msg, wParam, lParam);

    if (msg == WM_NCDESTROY)
    {
        LRESULT result = self->HandleMessage(msg, wParam, lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        delete self;
        return result;
    }

    return self->HandleMessage(msg, wParam, lParam);
}

LRESULT CJsonViewerWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        TreeHandle = CreateWindowEx(0, WC_TREEVIEWA, "", WS_CHILD | WS_VISIBLE | TVS_HASLINES |
                                                               TVS_LINESATROOT | TVS_HASBUTTONS |
                                                               WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                     0, 0, 0, 0, HWnd, reinterpret_cast<HMENU>(1), DLLInstance, NULL);
        if (!LoadFromFile(FileName.c_str()))
        {
            PostMessage(HWnd, WM_CLOSE, 0, 0);
        }
        else
        {
            PopulateTree();
        }
        return 0;
    case WM_SIZE:
        if (TreeHandle)
        {
            MoveWindow(TreeHandle, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(HWnd);
        return 0;
    case WM_DESTROY:
        TreeHandle = NULL;
        UnregisterViewerWindow(HWnd);
        return 0;
    default:
        return DefWindowProcA(HWnd, msg, wParam, lParam);
    }
}

BOOL CJsonViewerWindow::LoadFromFile(const char* fileName)
{
    std::ifstream file(fileName, std::ios::binary);
    if (!file)
    {
        ShowParseError("Unable to open file");
        return FALSE;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF)
    {
        content.erase(0, 3);
    }

    try
    {
        JsonParser parser(content);
        Root = parser.Parse();
    }
    catch (const std::exception& ex)
    {
        ShowParseError(ex.what());
        Root.reset();
        return FALSE;
    }

    return TRUE;
}

void CJsonViewerWindow::PopulateTree()
{
    if (!TreeHandle || !Root)
        return;

    TreeView_DeleteAllItems(TreeHandle);

    TVINSERTSTRUCTA insert{};
    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = TVI_ROOT;
    std::string text = FormatNodeText(*Root);
    insert.item.mask = TVIF_TEXT;
    insert.item.pszText = const_cast<char*>(text.c_str());
    HTREEITEM rootItem = TreeView_InsertItem(TreeHandle, &insert);

    for (const auto& child : Root->Children)
    {
        PopulateNode(rootItem, child);
    }

    TreeView_Expand(TreeHandle, rootItem, TVE_EXPAND);
}

void CJsonViewerWindow::PopulateNode(HTREEITEM parent, const JsonNode& node)
{
    TVINSERTSTRUCTA insert{};
    insert.hParent = parent;
    insert.hInsertAfter = TVI_LAST;
    std::string text = FormatNodeText(node);
    insert.item.mask = TVIF_TEXT;
    insert.item.pszText = const_cast<char*>(text.c_str());
    HTREEITEM item = TreeView_InsertItem(TreeHandle, &insert);

    for (const auto& child : node.Children)
    {
        PopulateNode(item, child);
    }
}

void CJsonViewerWindow::ShowParseError(const char* message)
{
    if (!message)
        message = "Unknown parsing error";
    SalamanderGeneral->SalMessageBox(HWnd, message, LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
}

