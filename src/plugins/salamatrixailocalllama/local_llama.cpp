// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include <shellapi.h>

// The provider implementation is shared source with the former in-process
// prototype, but is linked only by this optional companion plug-in. The main
// SalamatrixAI plug-in therefore remains model-free and can be installed alone.
#define SALAMATRIXAI_BUNDLED_PROVIDER_HAS_PRECOMP
#include "../salamatrixai/bundledprovider.cpp"
#undef SALAMATRIXAI_BUNDLED_PROVIDER_HAS_PRECOMP

HINSTANCE DLLInstance = NULL;
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;
CLocalLlamaPluginInterface LocalLlamaPluginInterface;

namespace
{
CLocalBundledAssistantProvider g_provider;
Salamatrix::AI::IAssistantService* g_ai = NULL;
bool g_registered = false;
bool g_released = false;
LocalLlamaModel g_selectedModel = LocalLlamaModelQwen15B;
const char* const CONFIG_SELECTED_MODEL = "SelectedModel";

static bool IsInterfaceModuleLoaded(const void* interfacePointer)
{
    if (interfacePointer == NULL)
        return false;

    MEMORY_BASIC_INFORMATION memory = {};
    if (VirtualQuery(interfacePointer, &memory, sizeof(memory)) != sizeof(memory) ||
        memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;

    const void* vtable = *static_cast<void* const*>(interfacePointer);
    if (vtable == NULL)
        return false;

    HMODULE module = NULL;
    return GetModuleHandleExW(
               GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
               reinterpret_cast<LPCWSTR>(vtable), &module) != FALSE;
}

static const char* LoadLocalLlamaString(int resourceId)
{
    if (SalamanderGeneral == NULL || DLLInstance == NULL)
        return "";
    const char* value = SalamanderGeneral->LoadStr(DLLInstance, resourceId);
    return value != NULL ? value : "";
}

static void* Query(const char* serviceId, DWORD minimumVersion)
{
    if (SalamanderGeneral == NULL)
        return NULL;
    CSalamanderServiceQuery query = {};
    query.ServiceId = serviceId;
    query.MinimumVersion = minimumVersion;
    CSalamanderServiceResult result = {};
    return SalamanderGeneral->QueryService(&query, &result) ? result.Interface : NULL;
}

static void EnsureProvider()
{
    if (g_released || g_registered)
        return;
    g_ai = static_cast<Salamatrix::AI::IAssistantService*>(
        Query(SALAMATRIX_SERVICE_AI, SALAMATRIX_AI_VERSION_1_0));
    if (g_ai != NULL) {
        g_registered = g_ai->RegisterProvider(&g_provider) != FALSE;
    }
}

static std::wstring ModuleDirectory()
{
    std::vector<wchar_t> module(SAL_MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(DLLInstance, module.data(),
                                      static_cast<DWORD>(module.size()));
    if (length == 0 || length >= module.size())
        return std::wstring();
    std::wstring path(module.data(), length);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return std::wstring();
    path.resize(slash);
    return path;
}

static std::wstring RuntimeDirectory()
{
    std::wstring path = ModuleDirectory();
    if (!path.empty())
        path += L"\\runtime";
    return path;
}

static bool IsLocalLlamaRegularFile(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static std::wstring SelectedModelPath()
{
    const std::wstring runtime = RuntimeDirectory();
    if (runtime.empty())
        return std::wstring();
    std::wstring path = runtime + L"\\" +
                        GetSelectedLocalLlamaModelFileName();
    if (g_selectedModel == LocalLlamaModelQwen05B &&
        !IsLocalLlamaRegularFile(path))
    {
        const std::wstring legacy = runtime + L"\\salamatrix.gguf";
        if (IsLocalLlamaRegularFile(legacy))
            return legacy;
    }
    return path;
}

static bool HasLlamaRuntime()
{
    const std::wstring runtime = RuntimeDirectory();
    return !runtime.empty() &&
           IsLocalLlamaRegularFile(runtime + L"\\llama-cli.exe");
}

static bool HasSelectedModel()
{
    return IsLocalLlamaRegularFile(SelectedModelPath());
}

static bool HasInstalledAssets()
{
    return HasLlamaRuntime() && HasSelectedModel();
}

static std::wstring QuoteArgument(const std::wstring& value)
{
    return L"\"" + value + L"\"";
}

static bool LaunchInstaller(HWND parent)
{
    const std::wstring module = ModuleDirectory();
    if (module.empty())
        return false;
    const std::wstring script = module + L"\\runtime\\install_llama.ps1";
    if (!IsLocalLlamaRegularFile(script))
        return false;
    const wchar_t* model =
        g_selectedModel == LocalLlamaModelQwen05B ? L"0.5B" : L"1.5B";
    const std::wstring parameters =
        L"-NoLogo -NoProfile -ExecutionPolicy Bypass -File " +
        QuoteArgument(script) + L" -Destination " + QuoteArgument(module) +
        L" -Model " + QuoteArgument(model);
    HINSTANCE result = ShellExecuteW(parent, L"runas", L"powershell.exe",
                                     parameters.c_str(), module.c_str(),
                                     SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

static void OpenRuntimeFolder(HWND parent)
{
    const std::wstring runtime = RuntimeDirectory();
    if (!runtime.empty())
        ShellExecuteW(parent, L"open", runtime.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

static const char* SelectedModelDisplayName()
{
    return LoadLocalLlamaString(
        g_selectedModel == LocalLlamaModelQwen05B
            ? IDS_LLAMA_MODEL_05B
            : IDS_LLAMA_MODEL_15B);
}

static std::string FormatStatusText(const char* format)
{
    std::string text = format != NULL ? format : "";
    const size_t placeholder = text.find("%s");
    if (placeholder != std::string::npos)
        text.replace(placeholder, 2, SelectedModelDisplayName());
    return text;
}

static std::string AssetStatus()
{
    if (!HasLlamaRuntime())
        return LoadLocalLlamaString(IDS_LLAMA_STATUS_RUNTIME_MISSING);
    return FormatStatusText(LoadLocalLlamaString(
        HasSelectedModel() ? IDS_LLAMA_STATUS_READY
                           : IDS_LLAMA_STATUS_MODEL_MISSING));
}

struct ConfigurationContext
{
    Salamatrix::UI::IControl* Model;
    Salamatrix::UI::IControl* Status;
    HWND Parent;
};

static BOOL WINAPI ConfigurationEvent(
    void* context, const Salamatrix::UI::DialogEvent* event)
{
    ConfigurationContext* configuration =
        static_cast<ConfigurationContext*>(context);
    if (configuration == NULL || event == NULL ||
        configuration->Model == NULL || configuration->Status == NULL)
        return TRUE;
    if (strcmp(event->ControlId, "model") == 0)
    {
        const int selected = configuration->Model->GetSelectedIndex();
        g_selectedModel = selected == 1 ? LocalLlamaModelQwen05B
                                        : LocalLlamaModelQwen15B;
        configuration->Status->SetText(AssetStatus().c_str());
    }
    else if (strcmp(event->ControlId, "download") == 0)
    {
        configuration->Status->SetText(
            LaunchInstaller(configuration->Parent)
                ? LoadLocalLlamaString(IDS_LLAMA_DOWNLOAD_STARTED)
                : LoadLocalLlamaString(IDS_LLAMA_DOWNLOAD_FAILED));
    }
    else if (strcmp(event->ControlId, "refresh") == 0)
        configuration->Status->SetText(AssetStatus().c_str());
    else if (strcmp(event->ControlId, "folder") == 0)
        OpenRuntimeFolder(configuration->Parent);
    return TRUE;
}

static void ShowConfiguration(HWND parent)
{
    Salamatrix::UI::IUIService* ui = static_cast<Salamatrix::UI::IUIService*>(
        Query(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0));
    if (ui == NULL)
    {
        SalamanderGeneral->SalMessageBox(
            parent, LoadLocalLlamaString(IDS_LLAMA_UI_UNAVAILABLE),
            LoadLocalLlamaString(IDS_LLAMA_TITLE),
            MB_OK | MB_ICONWARNING);
        return;
    }
    Salamatrix::UI::DialogOptions options;
    options.Title = LoadLocalLlamaString(IDS_LLAMA_TITLE);
    options.Parent = parent;
    options.Width = 360;
    options.Height = 126;
    Salamatrix::UI::IDialog* dialog = ui->CreateSalamatrixDialog(options);
    if (dialog == NULL)
        return;

    Salamatrix::UI::ControlLayout modelLabelLayout;
    modelLabelLayout.HasBounds = TRUE;
    modelLabelLayout.X = 8; modelLabelLayout.Y = 6;
    modelLabelLayout.Width = 344; modelLabelLayout.Height = 14;
    Salamatrix::UI::ControlOptions modelLabelOptions;
    modelLabelOptions.Id = "model-label";
    modelLabelOptions.Text = LoadLocalLlamaString(IDS_LLAMA_MODEL_LABEL);
    dialog->AddControlEx(
        Salamatrix::UI::ControlKindLabel,
        modelLabelOptions, modelLabelLayout);

    Salamatrix::UI::ControlLayout modelLayout;
    modelLayout.HasBounds = TRUE;
    modelLayout.X = 8; modelLayout.Y = 22;
    modelLayout.Width = 344; modelLayout.Height = 22;
    Salamatrix::UI::ControlOptions modelOptions;
    modelOptions.Id = "model";
    Salamatrix::UI::IControl* model = dialog->AddControlEx(
        Salamatrix::UI::ControlKindComboBox, modelOptions, modelLayout);
    if (model != NULL)
    {
        model->AddItem(LoadLocalLlamaString(IDS_LLAMA_MODEL_15B));
        model->AddItem(LoadLocalLlamaString(IDS_LLAMA_MODEL_05B));
        model->SetSelectedIndex(
            g_selectedModel == LocalLlamaModelQwen05B ? 1 : 0);
    }

    Salamatrix::UI::ControlLayout statusLayout;
    statusLayout.HasBounds = TRUE;
    statusLayout.X = 8; statusLayout.Y = 50;
    statusLayout.Width = 344; statusLayout.Height = 38;
    Salamatrix::UI::ControlOptions statusOptions;
    statusOptions.Id = "status";
    const std::wstring assetStatus = AssetStatus();
    statusOptions.Text = assetStatus.c_str();
    statusOptions.ReadOnly = TRUE;
    statusOptions.Multiline = TRUE;
    Salamatrix::UI::IControl* status = dialog->AddControlEx(
        Salamatrix::UI::ControlKindTextBox, statusOptions, statusLayout);

    Salamatrix::UI::ControlOptions downloadOptions;
    downloadOptions.Id = "download";
    downloadOptions.Text = LoadLocalLlamaString(IDS_LLAMA_DOWNLOAD);
    downloadOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout downloadLayout;
    downloadLayout.HasBounds = TRUE;
    downloadLayout.X = 8; downloadLayout.Y = 98; downloadLayout.Width = 72; downloadLayout.Height = 16;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, downloadOptions, downloadLayout);

    Salamatrix::UI::ControlOptions refreshOptions;
    refreshOptions.Id = "refresh";
    refreshOptions.Text = LoadLocalLlamaString(IDS_LLAMA_REFRESH);
    refreshOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout refreshLayout;
    refreshLayout.HasBounds = TRUE;
    refreshLayout.X = 88; refreshLayout.Y = 98; refreshLayout.Width = 60; refreshLayout.Height = 16;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, refreshOptions, refreshLayout);

    Salamatrix::UI::ControlOptions folderOptions;
    folderOptions.Id = "folder";
    folderOptions.Text = LoadLocalLlamaString(IDS_LLAMA_OPEN_FOLDER);
    folderOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout folderLayout;
    folderLayout.HasBounds = TRUE;
    folderLayout.X = 156; folderLayout.Y = 98; folderLayout.Width = 86; folderLayout.Height = 16;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, folderOptions, folderLayout);

    Salamatrix::UI::ControlOptions closeOptions;
    closeOptions.Id = "close";
    closeOptions.Text = LoadLocalLlamaString(IDS_LLAMA_CLOSE);
    closeOptions.DialogResult = IDCANCEL;
    Salamatrix::UI::ControlLayout closeLayout;
    closeLayout.HasBounds = TRUE;
    closeLayout.X = 290; closeLayout.Y = 98; closeLayout.Width = 62; closeLayout.Height = 16;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, closeOptions, closeLayout);

    ConfigurationContext context = { model, status, parent };
    dialog->SetEventCallback(ConfigurationEvent, &context);
    dialog->ShowModal();
    dialog->Release();
}
}

LocalLlamaModel GetSelectedLocalLlamaModel()
{
    return g_selectedModel;
}

const wchar_t* GetSelectedLocalLlamaModelFileName()
{
    return g_selectedModel == LocalLlamaModelQwen05B
               ? L"qwen2.5-coder-0.5b-instruct-q4_k_m.gguf"
               : L"qwen2.5-coder-1.5b-instruct-q4_k_m.gguf";
}

void WINAPI CLocalLlamaPluginInterface::About(HWND parent)
{
    SalamanderGeneral->SalMessageBox(
        parent,
        LoadLocalLlamaString(IDS_LLAMA_ABOUT),
        LoadLocalLlamaString(IDS_LLAMA_TITLE),
        MB_OK | MB_ICONINFORMATION);
}

void WINAPI CLocalLlamaPluginInterface::Configuration(HWND parent)
{
    ShowConfiguration(parent);
}

BOOL WINAPI CLocalLlamaPluginInterface::Release(HWND parent, BOOL force)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(force);
    if (!g_released && g_registered && SalamanderGeneral != NULL &&
        IsInterfaceModuleLoaded(g_ai)) {
        CSalamanderServiceQuery query = {};
        query.ServiceId = SALAMATRIX_SERVICE_AI;
        query.MinimumVersion = SALAMATRIX_AI_VERSION_1_0;
        CSalamanderServiceResult result = {};
        if (SalamanderGeneral->QueryService(&query, &result) && result.Interface == g_ai)
            g_ai->UnregisterProvider(&g_provider);
    }
    g_registered = false;
    g_ai = NULL;
    g_released = true;
    return TRUE;
}

void WINAPI CLocalLlamaPluginInterface::LoadConfiguration(HWND parent, HKEY regKey,
                                                  CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    DWORD selected = static_cast<DWORD>(LocalLlamaModelQwen15B);
    const bool loaded =
        registry != NULL &&
        regKey != NULL &&
        registry->GetValue(regKey, CONFIG_SELECTED_MODEL, REG_DWORD,
                           &selected, sizeof(selected)) &&
        selected <= static_cast<DWORD>(LocalLlamaModelQwen05B);
    if (loaded)
        g_selectedModel = static_cast<LocalLlamaModel>(selected);
    else
    {
        const std::wstring runtime = RuntimeDirectory();
        const bool hasRecommended =
            !runtime.empty() && IsLocalLlamaRegularFile(
                runtime + L"\\qwen2.5-coder-1.5b-instruct-q4_k_m.gguf");
        const bool hasLegacyLightweight =
            !runtime.empty() &&
            (IsLocalLlamaRegularFile(runtime + L"\\salamatrix.gguf") ||
            IsLocalLlamaRegularFile(
                runtime + L"\\qwen2.5-coder-0.5b-instruct-q4_k_m.gguf"));
        g_selectedModel = hasLegacyLightweight && !hasRecommended
                              ? LocalLlamaModelQwen05B
                              : LocalLlamaModelQwen15B;
    }
}

void WINAPI CLocalLlamaPluginInterface::SaveConfiguration(HWND parent, HKEY regKey,
                                                 CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    if (registry != NULL && regKey != NULL)
    {
        const DWORD selected = static_cast<DWORD>(g_selectedModel);
        registry->SetValue(regKey, CONFIG_SELECTED_MODEL, REG_DWORD,
                           &selected, sizeof(selected));
    }
}

void WINAPI CLocalLlamaPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    UNREFERENCED_PARAMETER(parent);
    EnsureProvider();
    if (SalamanderGUI == NULL || salamander == NULL)
        return;
    CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
    if (iconList == NULL || !iconList->Create(16, 16, 1)) {
        if (iconList != NULL) SalamanderGUI->DestroyIconList(iconList);
        return;
    }
    const UINT loadFlags = SalamanderGeneral != NULL ?
        SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
    HICON icon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON),
                                  IMAGE_ICON, 16, 16, loadFlags);
    if (icon != NULL) {
        iconList->ReplaceIcon(0, icon);
        DestroyIcon(icon);
        salamander->SetIconListForGUI(iconList);
        salamander->SetPluginIcon(0);
        salamander->SetPluginMenuAndToolbarIcon(0);
        iconList = NULL;
    }
    if (iconList != NULL)
        SalamanderGUI->DestroyIconList(iconList);
}

void WINAPI CLocalLlamaPluginInterface::Event(int event, DWORD param)
{
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(param);
    EnsureProvider();
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);
    if (reason == DLL_PROCESS_ATTACH) {
        DLLInstance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

int WINAPI SalamanderPluginGetReqVer() { return LAST_VERSION_OF_SALAMANDER; }

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
        return NULL;
    salamander->SetBasicPluginData(
        "Salamatrix AI Local LLaMA",
        FUNCTION_AUTOMATIONFRAMEWORK | FUNCTION_CONFIGURATION,
        VERSINFO_VERSION_NO_PLATFORM,
        VERSINFO_COPYRIGHT,
        VERSINFO_DESCRIPTION,
        VERSINFO_INTERNAL,
        NULL,
        NULL);
    SalamanderGeneral->SetFlagLoadOnSalamanderStart(TRUE);
    salamander->SetPluginHomePageURL("https://samandarin.net/");
    LocalLlamaPluginInterface.Event(0, 0);
    return &LocalLlamaPluginInterface;
}
