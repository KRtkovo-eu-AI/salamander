// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <fstream>
#include <string>
#include <vector>
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui.h"
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_host.h"
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_controls.h"
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_win32_host.h"

struct PreviewControl
{
    struct Option { std::string Name, Type, Value; };
    struct Column { std::string Title; int Width; };
    std::string Kind, Id, Text;
    int X = 0, Y = 0, Width = 0, Height = 0;
    DWORD Style = 0;
    BOOL Checked = FALSE;
    std::vector<Option> Options;
    std::vector<std::string> Items;
    std::vector<Column> Columns;
    int SelectedIndex = -1;
    BOOL HasSelection = FALSE;
    BOOL Required = FALSE;
    std::string ValidationMessage;
};

struct PreviewModel
{
    std::string Title;
    int Width = 320, Height = 180;
    std::vector<PreviewControl> Controls;
};

static bool Hex(const std::string& value, std::string& result)
{
    if ((value.size() & 1) != 0) return false;
    result.clear(); result.reserve(value.size() / 2);
    for (size_t i = 0; i < value.size(); i += 2)
    {
        char* end = NULL;
        const std::string pair = value.substr(i, 2);
        long byte = strtol(pair.c_str(), &end, 16);
        if (end == NULL || *end != 0 || byte < 0 || byte > 255) return false;
        result.push_back(static_cast<char>(byte));
    }
    return true;
}

static std::vector<std::string> Split(const std::string& line)
{
    std::vector<std::string> fields; size_t start = 0;
    for (;;)
    {
        size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab == std::string::npos ? tab : tab - start));
        if (tab == std::string::npos) return fields;
        start = tab + 1;
    }
}

static bool ReadLine(std::ifstream& input, std::string& line)
{
    if (!std::getline(input, line)) return false;
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
    return true;
}

static bool LoadModel(const wchar_t* path, PreviewModel& model)
{
    std::ifstream input(path, std::ios::binary);
    std::string line;
    if (!input || !ReadLine(input, line) || line != "SMXPREVIEW1") return false;
    if (!ReadLine(input, line) || !Hex(line, model.Title)) return false;
    if (!ReadLine(input, line)) return false;
    std::vector<std::string> size = Split(line);
    if (size.size() != 2) return false;
    model.Width = atoi(size[0].c_str()); model.Height = atoi(size[1].c_str());
    while (ReadLine(input, line))
    {
        std::vector<std::string> field = Split(line);
        if (!field.empty() && field[0] == "O")
        {
            if (model.Controls.empty() || field.size() != 4) return false;
            PreviewControl::Option option;
            if (!Hex(field[1], option.Name) || !Hex(field[3], option.Value)) return false;
            option.Type = field[2]; model.Controls.back().Options.push_back(option); continue;
        }
        if (!field.empty() && field[0] == "I")
        {
            if (model.Controls.empty() || field.size() != 2) return false;
            std::string item; if (!Hex(field[1], item)) return false;
            model.Controls.back().Items.push_back(item); continue;
        }
        if (!field.empty() && field[0] == "C")
        {
            if (model.Controls.empty() || field.size() != 3) return false;
            PreviewControl::Column column; if (!Hex(field[1], column.Title)) return false;
            column.Width = atoi(field[2].c_str()); model.Controls.back().Columns.push_back(column); continue;
        }
        if (!field.empty() && field[0] == "S")
        {
            if (model.Controls.empty() || field.size() != 2) return false;
            model.Controls.back().SelectedIndex = atoi(field[1].c_str()); model.Controls.back().HasSelection = TRUE; continue;
        }
        if (!field.empty() && field[0] == "V")
        {
            if (model.Controls.empty() || field.size() != 3) return false;
            model.Controls.back().Required = field[1] == "1" ? TRUE : FALSE;
            if (!Hex(field[2], model.Controls.back().ValidationMessage)) return false; continue;
        }
        if (field.size() != 10) return false;
        PreviewControl control;
        if (!Hex(field[0], control.Kind) || !Hex(field[1], control.Id) || !Hex(field[2], control.Text)) return false;
        control.X = atoi(field[3].c_str()); control.Y = atoi(field[4].c_str());
        control.Width = atoi(field[5].c_str()); control.Height = atoi(field[6].c_str());
        control.Style = static_cast<DWORD>(strtoul(field[7].c_str(), NULL, 10));
        control.Checked = field[8] == "1" ? TRUE : FALSE;
        model.Controls.push_back(control);
    }
    return model.Width > 0 && model.Height > 0;
}

static const PreviewControl::Option* FindOption(const PreviewControl& control, const char* name)
{
    for (size_t i = 0; i < control.Options.size(); ++i)
        if (control.Options[i].Name == name) return &control.Options[i];
    return NULL;
}

static BOOL BoolOption(const PreviewControl& control, const char* name, BOOL fallback = FALSE)
{
    const PreviewControl::Option* value = FindOption(control, name);
    return value != NULL ? (value->Value == "true" ? TRUE : FALSE) : fallback;
}

static int IntOption(const PreviewControl& control, const char* name, int fallback = 0)
{
    const PreviewControl::Option* value = FindOption(control, name);
    return value != NULL ? atoi(value->Value.c_str()) : fallback;
}

static const char* StringOption(const PreviewControl& control, const char* name)
{
    const PreviewControl::Option* value = FindOption(control, name);
    return value != NULL ? value->Value.c_str() : NULL;
}

static Salamatrix::UI::ControlKind Kind(const std::string& value)
{
    using namespace Salamatrix::UI;
    if (value == "textbox") return ControlKindTextBox; if (value == "checkbox") return ControlKindCheckBox;
    if (value == "radio") return ControlKindRadioButton; if (value == "combobox") return ControlKindComboBox;
    if (value == "button") return ControlKindButton; if (value == "listview") return ControlKindListView;
    if (value == "treeview") return ControlKindTreeView; if (value == "tabcontrol") return ControlKindTabControl;
    if (value == "folderpicker") return ControlKindFolderPicker; if (value == "filepicker") return ControlKindFilePicker;
    if (value == "groupbox") return ControlKindGroupBox; if (value == "statictext") return ControlKindStaticText;
    if (value == "hyperlink") return ControlKindHyperLink; if (value == "progressbar") return ControlKindProgressBar;
    if (value == "arrowbutton") return ControlKindArrowButton; if (value == "textarrowbutton") return ControlKindTextArrowButton;
    if (value == "colorarrowbutton") return ControlKindColorArrowButton; if (value == "toolbarheader") return ControlKindToolbarHeader;
    return ControlKindLabel;
}

static int ShowPreview(const PreviewModel& model)
{
    Salamatrix::UI::INativeDialogHost* host = Salamatrix::UI::GetWin32NativeDialogHost();
    Salamatrix::UI::SetNativeDialogHost(host);
    Salamatrix::UI::DialogOptions options;
    options.Title = model.Title.c_str(); options.Width = static_cast<short>(model.Width); options.Height = static_cast<short>(model.Height);
    Salamatrix::UI::NativeDialog dialog(options);
    for (size_t i = 0; i < model.Controls.size(); ++i)
    {
        const PreviewControl& source = model.Controls[i];
        Salamatrix::UI::ControlOptions control; control.Id = source.Id.c_str(); control.Text = source.Text.c_str();
        control.ReadOnly = BoolOption(source, "readOnly"); control.Checked = BoolOption(source, "checked", source.Checked);
        control.DialogResult = IntOption(source, "dialogResult"); control.KeepOpen = BoolOption(source, "keepOpen");
        control.Multiline = BoolOption(source, "multiline"); control.FileFilter = StringOption(source, "filter"); control.FileSave = BoolOption(source, "save");
        Salamatrix::UI::ControlLayout layout; layout.HasBounds = TRUE; layout.X = source.X; layout.Y = source.Y; layout.Width = source.Width; layout.Height = source.Height;
        Salamatrix::UI::IControl* added = dialog.AddControlEx(Kind(source.Kind), control, layout);
        if (added == NULL) { Salamatrix::UI::SetNativeDialogHost(NULL); return 3; }
        if (source.Style != 0) added->SetStyleFlags(source.Style);
        for (size_t item = 0; item < source.Items.size(); ++item) added->AddItem(source.Items[item].c_str());
        for (size_t column = 0; column < source.Columns.size(); ++column) added->AddColumn(source.Columns[column].Title.c_str(), source.Columns[column].Width);
        if (source.HasSelection) added->SetSelectedIndex(source.SelectedIndex);
        added->SetRequired(source.Required); added->SetValidationMessage(source.ValidationMessage.c_str());
        const char* pathSeparator = StringOption(source, "pathSeparator"); if (pathSeparator != NULL && pathSeparator[0] != 0) added->SetPathSeparator(pathSeparator[0]);
        const char* toolTip = StringOption(source, "toolTip"); if (toolTip != NULL) added->SetToolTipText(toolTip);
        const char* actionOpen = StringOption(source, "actionOpen"); if (actionOpen != NULL) added->SetActionOpen(actionOpen);
        const char* actionHint = StringOption(source, "actionHint"); if (actionHint != NULL) added->SetActionShowHint(actionHint);
        if (FindOption(source, "actionCommand") != NULL) added->SetActionPostCommand(static_cast<WORD>(IntOption(source, "actionCommand")));
        if (FindOption(source, "progress") != NULL) added->SetProgress(IntOption(source, "progress"), StringOption(source, "progressText"));
        if (FindOption(source, "progressCurrent") != NULL || FindOption(source, "progressTotal") != NULL) added->SetProgressValues(IntOption(source, "progressCurrent"), IntOption(source, "progressTotal"), StringOption(source, "progressText"));
        if (FindOption(source, "indeterminateDuration") != NULL) added->SetIndeterminateTiming(IntOption(source, "indeterminateDuration"), IntOption(source, "indeterminateInterval", 50));
        if (FindOption(source, "textColor") != NULL || FindOption(source, "backgroundColor") != NULL) added->SetColor(IntOption(source, "textColor"), IntOption(source, "backgroundColor", 0xFFFFFF));
        const char* align = StringOption(source, "alignControlId"); if (align != NULL) added->SetToolbarHeader(align, IntOption(source, "buttonMask"));
    }
    int result = dialog.ShowModal();
    return result;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    INITCOMMONCONTROLSEX common = { sizeof(common), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&common);
    int count = 0; wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &count);
    bool validate = args != NULL && count == 3 && wcscmp(args[1], L"--validate") == 0;
    bool themed = args != NULL && count == 4 && wcscmp(args[1], L"--theme") == 0 &&
        (wcscmp(args[2], L"light") == 0 || wcscmp(args[2], L"dark") == 0);
    BOOL dark = themed && wcscmp(args[2], L"dark") == 0 ? TRUE : FALSE;
    const wchar_t* path = args == NULL ? NULL : validate ? args[2] : themed ? args[3] : count == 2 ? args[1] : NULL;
    PreviewModel model; bool loaded = path != NULL && LoadModel(path, model);
    if (args != NULL) LocalFree(args);
    if (!loaded) return 2;
    Salamatrix::UI::SetWin32NativeDialogDarkMode(dark);
    return validate ? 0 : ShowPreview(model);
}
