// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include "../salamatrix/salamatrix_ui.h"

namespace RuntimeConfiguration
{
struct Settings
{
    bool UseCustomExecutable;
    std::wstring CustomExecutablePath;

    Settings() : UseCustomExecutable(false) {}
};

struct TextIds
{
    int Title;
    int ExecutableInUse;
    int NotFound;
    int UseCustom;
    int CustomExecutable;
    int FileFilter;
    int OK;
    int Cancel;
    int UIUnavailable;
    int CustomRequired;
    int CustomInvalid;
};

inline const char* LoadText(
    CSalamanderGeneralAbstract* general, HINSTANCE module, int id)
{
    const char* text = general != NULL ? general->LoadStr(module, id) : NULL;
    return text != NULL ? text : "";
}

inline bool WideToUtf8(const std::wstring& value, std::string& result)
{
    result.clear();
    if (value.empty())
        return true;
    int required = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(),
        static_cast<int>(value.size()), NULL, 0, NULL, NULL);
    if (required <= 0)
        return false;
    result.resize(static_cast<size_t>(required));
    return WideCharToMultiByte(
               CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(),
               static_cast<int>(value.size()), &result[0], required,
               NULL, NULL) == required;
}

inline bool Utf8ToWide(const char* value, std::wstring& result)
{
    result.clear();
    if (value == NULL || value[0] == '\0')
        return true;
    int required = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, NULL, 0);
    if (required <= 1)
        return false;
    std::vector<wchar_t> buffer(static_cast<size_t>(required));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value, -1,
            &buffer[0], required) != required)
        return false;
    result.assign(&buffer[0]);
    return true;
}

inline bool IsRegularFile(const std::wstring& path)
{
    if (path.empty())
        return false;
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline void Load(
    HKEY key, CSalamanderRegistryAbstract* registry, Settings& settings)
{
    settings = Settings();
    if (key == NULL || registry == NULL)
        return;

    DWORD useCustom = FALSE;
    if (registry->GetValue(
            key, "UseCustomExecutable", REG_DWORD,
            &useCustom, sizeof(useCustom)))
        settings.UseCustomExecutable = useCustom != FALSE;

    DWORD size = 0;
    if (!registry->GetSize(
            key, "CustomExecutablePath", REG_SZ, size) || size == 0)
        return;
    std::vector<char> path(static_cast<size_t>(size) + 1, '\0');
    if (registry->GetValue(
            key, "CustomExecutablePath", REG_SZ, &path[0], size))
        Utf8ToWide(&path[0], settings.CustomExecutablePath);
}

inline void Save(
    HKEY key, CSalamanderRegistryAbstract* registry,
    const Settings& settings)
{
    if (key == NULL || registry == NULL)
        return;
    DWORD useCustom = settings.UseCustomExecutable ? TRUE : FALSE;
    registry->SetValue(
        key, "UseCustomExecutable", REG_DWORD,
        &useCustom, sizeof(useCustom));
    std::string path;
    if (WideToUtf8(settings.CustomExecutablePath, path))
        registry->SetValue(
            key, "CustomExecutablePath", REG_SZ,
            path.c_str(), static_cast<DWORD>(path.size() + 1));
}

inline Salamatrix::UI::IUIService* QueryUI(
    CSalamanderGeneralAbstract* general)
{
    if (general == NULL)
        return NULL;
    CSalamanderServiceQuery query = {};
    query.ServiceId = SALAMATRIX_SERVICE_UI;
    query.MinimumVersion = SALAMATRIX_UI_VERSION_1_2;
    CSalamanderServiceResult result = {};
    return general->QueryService(&query, &result)
               ? static_cast<Salamatrix::UI::IUIService*>(result.Interface)
               : NULL;
}

struct DialogContext
{
    Salamatrix::UI::IControl* UseCustom;
    Salamatrix::UI::IControl* CustomPath;
    Salamatrix::UI::IControl* EffectivePath;
    std::string AutomaticPath;
    std::string NotFound;
};

inline BOOL WINAPI DialogEvent(
    void* context, const Salamatrix::UI::DialogEvent* event)
{
    DialogContext* dialog = static_cast<DialogContext*>(context);
    if (dialog == NULL || event == NULL ||
        dialog->UseCustom == NULL || dialog->CustomPath == NULL ||
        dialog->EffectivePath == NULL)
        return TRUE;

    if (strcmp(event->ControlId, "use-custom") == 0)
    {
        dialog->CustomPath->SetEnabled(event->Checked);
        dialog->CustomPath->SetRequired(event->Checked);
        char path[4096] = {};
        dialog->CustomPath->GetText(path, _countof(path));
        dialog->EffectivePath->SetText(
            event->Checked
                ? (path[0] != '\0' ? path : dialog->NotFound.c_str())
                : (dialog->AutomaticPath.empty()
                       ? dialog->NotFound.c_str()
                       : dialog->AutomaticPath.c_str()));
    }
    else if (strcmp(event->ControlId, "custom-path") == 0 &&
             dialog->UseCustom->GetChecked())
    {
        dialog->EffectivePath->SetText(
            event->Text[0] != '\0' ? event->Text : dialog->NotFound.c_str());
    }
    return TRUE;
}

inline Salamatrix::UI::IControl* AddControl(
    Salamatrix::UI::IDialog* dialog,
    Salamatrix::UI::ControlKind kind,
    const char* id,
    const char* text,
    int x, int y, int width, int height,
    BOOL readOnly = FALSE,
    BOOL checked = FALSE,
    int dialogResult = 0,
    const char* fileFilter = NULL)
{
    Salamatrix::UI::ControlOptions options;
    options.Id = id;
    options.Text = text;
    options.ReadOnly = readOnly;
    options.Checked = checked;
    options.DialogResult = dialogResult;
    options.FileFilter = fileFilter;
    Salamatrix::UI::ControlLayout layout;
    layout.HasBounds = TRUE;
    layout.X = x;
    layout.Y = y;
    layout.Width = width;
    layout.Height = height;
    return dialog->AddControlEx(kind, options, layout);
}

inline bool ShowDialog(
    HWND parent,
    CSalamanderGeneralAbstract* general,
    HINSTANCE module,
    const TextIds& textIds,
    const std::wstring& automaticPath,
    const std::wstring& effectivePath,
    Settings& settings)
{
    Salamatrix::UI::IUIService* ui = QueryUI(general);
    if (ui == NULL)
    {
        if (general != NULL)
            general->SalMessageBox(
                parent,
                LoadText(general, module, textIds.UIUnavailable),
                LoadText(general, module, textIds.Title),
                MB_OK | MB_ICONWARNING);
        return false;
    }

    std::string automaticUtf8;
    std::string effectiveUtf8;
    std::string customUtf8;
    bool candidateUseCustom = settings.UseCustomExecutable;
    std::wstring candidatePath = settings.CustomExecutablePath;
    if (!WideToUtf8(automaticPath, automaticUtf8) ||
        !WideToUtf8(effectivePath, effectiveUtf8) ||
        !WideToUtf8(candidatePath, customUtf8))
        return false;

    for (;;)
    {
        Salamatrix::UI::DialogOptions options;
        options.Title = LoadText(general, module, textIds.Title);
        options.Parent = parent;
        options.Width = 420;
        options.Height = 146;
        Salamatrix::UI::IDialog* dialog = ui->CreateSalamatrixDialog(options);
        if (dialog == NULL)
            return false;

        AddControl(
            dialog, Salamatrix::UI::ControlKindLabel,
            "effective-label",
            LoadText(general, module, textIds.ExecutableInUse),
            8, 6, 404, 14);

        const char* notFound = LoadText(general, module, textIds.NotFound);
        const char* effective = effectiveUtf8.empty()
                                    ? notFound
                                    : effectiveUtf8.c_str();
        Salamatrix::UI::IControl* effectivePath = AddControl(
            dialog, Salamatrix::UI::ControlKindTextBox,
            "effective-path", effective,
            8, 21, 404, 20, TRUE);
        Salamatrix::UI::IControl* useCustom = AddControl(
            dialog, Salamatrix::UI::ControlKindCheckBox,
            "use-custom",
            LoadText(general, module, textIds.UseCustom),
            8, 47, 404, 18, FALSE,
            candidateUseCustom ? TRUE : FALSE);
        AddControl(
            dialog, Salamatrix::UI::ControlKindLabel,
            "custom-label",
            LoadText(general, module, textIds.CustomExecutable),
            8, 69, 404, 14);
        Salamatrix::UI::IControl* customPath = AddControl(
            dialog, Salamatrix::UI::ControlKindFilePicker,
            "custom-path", customUtf8.c_str(),
            8, 84, 404, 20, FALSE, FALSE, 0,
            LoadText(general, module, textIds.FileFilter));
        AddControl(
            dialog, Salamatrix::UI::ControlKindButton,
            "ok", LoadText(general, module, textIds.OK),
            272, 116, 66, 18, FALSE, FALSE, IDOK);
        AddControl(
            dialog, Salamatrix::UI::ControlKindButton,
            "cancel", LoadText(general, module, textIds.Cancel),
            346, 116, 66, 18, FALSE, FALSE, IDCANCEL);

        if (useCustom == NULL || customPath == NULL || effectivePath == NULL)
        {
            dialog->Release();
            return false;
        }
        customPath->SetEnabled(candidateUseCustom ? TRUE : FALSE);
        customPath->SetRequired(candidateUseCustom ? TRUE : FALSE);
        customPath->SetValidationMessage(
            LoadText(general, module, textIds.CustomRequired));
        DialogContext context = {
            useCustom, customPath, effectivePath,
            automaticUtf8, notFound};
        dialog->SetEventCallback(DialogEvent, &context);
        int result = dialog->ShowModal();
        if (result != IDOK)
        {
            dialog->Release();
            return false;
        }

        char selectedPath[4096] = {};
        customPath->GetText(selectedPath, _countof(selectedPath));
        bool useSelected = useCustom->GetChecked() != FALSE;
        std::wstring selectedWide;
        bool converted = Utf8ToWide(selectedPath, selectedWide);
        dialog->Release();
        if (useSelected && (!converted || !IsRegularFile(selectedWide)))
        {
            general->SalMessageBox(
                parent,
                LoadText(general, module, textIds.CustomInvalid),
                LoadText(general, module, textIds.Title),
                MB_OK | MB_ICONWARNING);
            customUtf8.assign(selectedPath);
            effectiveUtf8.clear();
            candidateUseCustom = true;
            candidatePath = selectedWide;
            continue;
        }

        bool changed = settings.UseCustomExecutable != useSelected ||
                       settings.CustomExecutablePath != selectedWide;
        settings.UseCustomExecutable = useSelected;
        settings.CustomExecutablePath = selectedWide;
        return changed;
    }
}
} // namespace RuntimeConfiguration
