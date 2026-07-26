// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_packages.h"

#include <algorithm>
#include <memory>

namespace Salamatrix
{
namespace Packages
{

struct PackageManager::Package
{
    PackageManager* Owner;
    CExtensionManifest Manifest;
    std::wstring Directory;
    std::wstring EntryPoint;
    std::string Id;
    std::string EntryPointUtf8;
    std::string IconPath;
    std::string IconDarkPath;
    std::vector<int> CommandIds;
    std::vector<std::string> CommandIconPaths;
    std::vector<int> MenuIconIndices;
    Runtime::IRuntimeSession* Session;
    HANDLE PumpThread;

    Package(PackageManager* owner)
        : Owner(owner),
          Session(NULL),
          PumpThread(NULL)
    {
    }
};

class PackageManager::MenuExtension : public CPluginInterfaceForMenuExtAbstract
{
private:
    PackageManager* Owner;

public:
    explicit MenuExtension(PackageManager* owner) : Owner(owner) {}

    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask)
    {
        UNREFERENCED_PARAMETER(eventMask);
        if (Owner == NULL)
            return 0;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            for (size_t c = 0; c < package->CommandIds.size(); ++c)
            {
                if (package->CommandIds[c] == id)
                    return MENU_ITEM_STATE_ENABLED;
            }
        }
        return 0;
    }

    virtual BOOL WINAPI ExecuteMenuItem(
        CSalamanderForOperationsAbstract* salamander,
        HWND parent,
        int id,
        DWORD eventMask)
    {
        UNREFERENCED_PARAMETER(salamander);
        UNREFERENCED_PARAMETER(parent);
        UNREFERENCED_PARAMETER(eventMask);
        if (Owner == NULL)
            return FALSE;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            for (size_t c = 0; c < package->CommandIds.size(); ++c)
            {
                if (package->CommandIds[c] == id &&
                    c < package->Manifest.Commands.size())
                {
                    const CExtensionManifestCommand& command =
                        package->Manifest.Commands[c];
                    return Owner->ExecuteCommand(
                        package, command.Id.c_str(), command.Handler.c_str());
                }
            }
        }
        return FALSE;
    }

    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id)
    {
        UNREFERENCED_PARAMETER(parent);
        UNREFERENCED_PARAMETER(id);
        return FALSE;
    }

    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* builder)
    {
        UNREFERENCED_PARAMETER(parent);
        if (Owner == NULL || builder == NULL)
            return;
        int iconCount = 0;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
            for (size_t c = 0; c < Owner->Packages[p]->Manifest.Commands.size(); ++c)
                if (Owner->Packages[p]->Manifest.Commands[c].Menu == "plugin" ||
                    Owner->Packages[p]->Manifest.Commands[c].Menu == "both")
                    ++iconCount;

        CGUIIconListAbstract* icons = NULL;
        if (SalamanderGUI != NULL && iconCount > 0)
        {
            icons = SalamanderGUI->CreateIconList();
            if (icons == NULL || !icons->Create(16, 16, iconCount))
            {
                if (icons != NULL)
                    SalamanderGUI->DestroyIconList(icons);
                icons = NULL;
            }
        }
        if (icons != NULL)
            builder->SetIconListForMenu(icons);

        int imageIndex = 0;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            package->MenuIconIndices.clear();
            for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
            {
                const CExtensionManifestCommand& command =
                    package->Manifest.Commands[c];
                if (command.Menu != "plugin" && command.Menu != "both")
                    continue;
                char title[256];
                StringCchCopyA(title, _countof(title), command.Title.c_str());
                int iconIndex = -1;
                if (icons != NULL)
                {
                    iconIndex = imageIndex++;
                    const std::string& iconPath = package->CommandIconPaths[c];
                    HICON icon = iconPath.empty()
                                     ? NULL
                                     : SalamanderGUI->CreateSVGIcon(
                                           iconPath.c_str(), 16);
                    if (icon != NULL)
                    {
                        icons->ReplaceIcon(iconIndex, icon);
                        DestroyIcon(icon);
                    }
                    else
                        iconIndex = -1;
                }
                package->MenuIconIndices.push_back(iconIndex);
                builder->AddMenuItem(
                    iconIndex, title, 0, package->CommandIds[c], TRUE,
                    MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
            }
        }
    }
};

PackageManager::PackageManager()
    : General(NULL),
      Runtimes(NULL),
      Extensions(NULL),
      Storage(NULL),
      UI(NULL),
      Menu(NULL)
{
}

PackageManager::~PackageManager()
{
    Shutdown();
}

BOOL PackageManager::Initialize(
    CSalamanderGeneralAbstract* general,
    Runtime::IRuntimeService* runtimes,
    Extensions::IExtensionsService* extensions,
    Storage::IStorageService* storage,
    UI::IUIService* ui)
{
    General = general;
    Runtimes = runtimes;
    Extensions = extensions;
    Storage = storage;
    UI = ui;
    if (Menu == NULL)
        Menu = new MenuExtension(this);
    if (Extensions != NULL)
        Extensions->SetRefreshCallback(RefreshCallback, this);
    return General != NULL && Runtimes != NULL && Extensions != NULL;
}

void PackageManager::Shutdown()
{
    if (Extensions != NULL)
        Extensions->SetRefreshCallback(NULL, NULL);
    UnregisterToolbarButtons();
    RemovePackages();
    delete Menu;
    Menu = NULL;
    Roots.clear();
    General = NULL;
    Runtimes = NULL;
    Extensions = NULL;
    Storage = NULL;
    UI = NULL;
}

void PackageManager::LoadConfiguration(HKEY key, CSalamanderRegistryAbstract* registry)
{
    Roots.clear();
    Roots.push_back(ExpandRoot(L"$(SalDir)\\extensions"));
    if (key == NULL || registry == NULL)
        return;
    HKEY rootsKey = NULL;
    if (!registry->OpenKey(key, "ExtensionRoots", rootsKey))
        return;
    char name[16];
    char path[SAL_MAX_PATH];
    for (int index = 1;; ++index)
    {
        _snprintf_s(name, _countof(name), _TRUNCATE, "%d", index);
        if (!registry->GetValue(rootsKey, name, REG_SZ, path, _countof(path)))
            break;
        std::wstring root;
        if (!ToWide(path, &root))
            continue;
        if (root != ExpandRoot(L"$(SalDir)\\extensions"))
            Roots.push_back(ExpandRoot(root));
    }
    registry->CloseKey(rootsKey);
}

void PackageManager::SaveConfiguration(HKEY key, CSalamanderRegistryAbstract* registry)
{
    if (key == NULL || registry == NULL)
        return;
    HKEY rootsKey = NULL;
    if (!registry->CreateKey(key, "ExtensionRoots", rootsKey))
        return;
    registry->ClearKey(rootsKey);
    char name[16];
    for (size_t index = 0; index < Roots.size(); ++index)
    {
        _snprintf_s(name, _countof(name), _TRUNCATE, "%d", static_cast<int>(index + 1));
        std::string root;
        if (ToUtf8(Roots[index], &root))
            registry->SetValue(rootsKey, name, REG_SZ, root.c_str(), -1);
    }
    registry->CloseKey(rootsKey);
}

void PackageManager::Refresh()
{
    RemovePackages();
    for (size_t index = 0; index < Roots.size(); ++index)
        DiscoverRoot(Roots[index]);
    RegisterToolbarButtons();
    if (General != NULL)
        General->PostPluginMenuChanged();
}

void PackageManager::DiscoverRoot(const std::wstring& root)
{
    if (!root.empty())
        DiscoverDirectory(root);
}

void PackageManager::DiscoverDirectory(const std::wstring& directory)
{
    std::wstring pattern = directory + L"\\*";
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            continue;
        std::wstring path = directory + L"\\" + data.cFileName;
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            std::wstring manifestPath = path + L"\\extension.json";
            std::string json;
            if (ReadUtf8File(manifestPath, &json))
            {
                CExtensionManifest manifest;
                CExtensionManifestError error;
                if (manifest.Parse(json.data(), json.size(), error) &&
                    CExtensionManifest::IsSafeRelativeEntryPoint(manifest.EntryPoint))
                {
                    Package* package = new Package(this);
                    package->Manifest = manifest;
                    package->Directory = path;
                    package->EntryPoint = path + L"\\";
                    std::wstring relative;
                    ToWide(manifest.EntryPoint, &relative);
                    package->EntryPoint += relative;
                    package->Id = manifest.Id;
                    ToUtf8(package->EntryPoint, &package->EntryPointUtf8);
                    std::wstring icon = path + L"\\";
                    std::wstring iconRelative;
                    if (!manifest.Icon.empty() && ToWide(manifest.Icon, &iconRelative))
                    {
                        std::string iconUtf8;
                        ToUtf8(icon + iconRelative, &iconUtf8);
                        package->IconPath = iconUtf8;
                    }
                    if (!manifest.IconDark.empty() && ToWide(manifest.IconDark, &iconRelative))
                    {
                        std::string iconUtf8;
                        ToUtf8(icon + iconRelative, &iconUtf8);
                        package->IconDarkPath = iconUtf8;
                    }
                    Extensions::ExtensionDescriptor descriptor;
                    StringCchCopyA(descriptor.Id, _countof(descriptor.Id), manifest.Id.c_str());
                    StringCchCopyA(descriptor.Name, _countof(descriptor.Name), manifest.Name.c_str());
                    StringCchCopyA(descriptor.Version, _countof(descriptor.Version), manifest.Version.c_str());
                    StringCchCopyA(descriptor.RuntimeId, _countof(descriptor.RuntimeId), manifest.RuntimeId.c_str());
                    std::string displayEntryPoint = package->EntryPointUtf8;
                    MakeDisplayEntryPoint(package->EntryPoint, &displayEntryPoint);
                    StringCchCopyA(descriptor.EntryPoint, _countof(descriptor.EntryPoint), displayEntryPoint.c_str());
                    StringCchCopyA(descriptor.IconPath, _countof(descriptor.IconPath), package->IconPath.c_str());
                    StringCchCopyA(descriptor.IconDarkPath, _countof(descriptor.IconDarkPath), package->IconDarkPath.c_str());
                    descriptor.Flags = Extensions::ExtensionFlagManifest |
                                       Extensions::ExtensionFlagPackage |
                                       Extensions::ExtensionFlagPersistent;
                    bool registeredRuntime = false;
                    bool availableRuntime = false;
                    for (int adapterIndex = 0;
                         adapterIndex < Runtimes->GetAdapterCount();
                         ++adapterIndex)
                    {
                        Runtime::IRuntimeAdapter* adapter =
                            Runtimes->GetAdapter(adapterIndex);
                        const Runtime::RuntimeAdapterDescriptor* runtimeDescriptor =
                            adapter != NULL ? adapter->GetDescriptor() : NULL;
                        if (runtimeDescriptor != NULL &&
                            runtimeDescriptor->RuntimeId != NULL &&
                            _stricmp(runtimeDescriptor->RuntimeId,
                                     manifest.RuntimeId.c_str()) == 0 &&
                            runtimeDescriptor->RuntimeVersion >=
                                manifest.MinimumRuntimeVersion)
                        {
                            registeredRuntime = true;
                            availableRuntime = adapter->IsAvailable() != FALSE;
                            break;
                        }
                    }
                    if (!registeredRuntime)
                        descriptor.Flags |= Extensions::ExtensionFlagRuntimeUnavailable;
                    else if (!availableRuntime)
                        descriptor.Flags |= Extensions::ExtensionFlagRuntimeExecutableUnavailable;
                    if (Extensions->RegisterExtension(&descriptor, LifecycleCallback, package))
                    {
                        std::vector<Extensions::ExtensionSettingInfo> settings;
                        for (size_t setting = 0; setting < manifest.Settings.size(); ++setting)
                        {
                            const CExtensionManifestSetting& source = manifest.Settings[setting];
                            Extensions::ExtensionSettingInfo target;
                            StringCchCopyA(target.Key, _countof(target.Key), source.Key.c_str());
                            StringCchCopyA(target.Label, _countof(target.Label), source.Label.c_str());
                            StringCchCopyA(target.Description, _countof(target.Description), source.Description.c_str());
                            StringCchCopyA(target.Group, _countof(target.Group), source.Group.c_str());
                            target.Order = source.Order;
                            target.Width = source.Width;
                            target.Multiline = source.Multiline ? TRUE : FALSE;
                            target.Type = source.Type == ExtensionManifestSettingInteger
                                              ? Extensions::ExtensionSettingInteger
                                              : source.Type == ExtensionManifestSettingBoolean
                                                    ? Extensions::ExtensionSettingBoolean
                                                    : Extensions::ExtensionSettingString;
                            settings.push_back(target);
                        }
                        Extensions->SetExtensionSettingsSchema(
                            package->Id.c_str(),
                            settings.empty() ? NULL : &settings[0],
                            static_cast<int>(settings.size()));
                        for (size_t command = 0; command < manifest.Commands.size(); ++command)
                        {
                            int id = 0x62000000 + static_cast<int>(Packages.size() * 64 + command + 1);
                            package->CommandIds.push_back(id);
                            std::wstring commandIcon;
                            const std::string& declaredIcon =
                                !manifest.Commands[command].Icon.empty()
                                    ? manifest.Commands[command].Icon
                                    : manifest.Icon;
                            std::string commandIconUtf8 = package->IconPath;
                            if (!declaredIcon.empty() && ToWide(declaredIcon, &commandIcon))
                                ToUtf8(path + L"\\" + commandIcon, &commandIconUtf8);
                            package->CommandIconPaths.push_back(commandIconUtf8);
                        }
                        Packages.push_back(package);
                    }
                    else
                    {
                        delete package;
                    }
                }
            }
            DiscoverDirectory(path);
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
}

void PackageManager::RemovePackages()
{
    for (size_t index = 0; index < Packages.size(); ++index)
    {
        Package* package = Packages[index];
        if (Extensions != NULL)
            Extensions->UnregisterExtension(package->Id.c_str(), package);
        if (package->Session != NULL)
        {
            package->Session->Stop();
            if (package->PumpThread != NULL)
            {
                WaitForSingleObject(package->PumpThread, 5000);
                CloseHandle(package->PumpThread);
            }
            package->Session->Release();
        }
        delete package;
    }
    Packages.clear();
}

CPluginInterfaceForMenuExtAbstract* PackageManager::GetMenuExtension()
{
    return Menu;
}

BOOL WINAPI PackageManager::LifecycleCallback(
    void* context, Extensions::ExtensionAction action, const Extensions::ExtensionInfo* info)
{
    Package* package = static_cast<Package*>(context);
    if (package == NULL || package->Owner == NULL || info == NULL)
        return FALSE;
    return action == Extensions::ExtensionActionActivate
               ? package->Owner->Activate(package)
               : package->Owner->Deactivate(package);
}

BOOL WINAPI PackageManager::RefreshCallback(void* context)
{
    PackageManager* manager = static_cast<PackageManager*>(context);
    if (manager == NULL)
        return FALSE;
    manager->Refresh();
    return TRUE;
}

DWORD WINAPI PackageManager::PumpThreadProc(void* context)
{
    Package* package = static_cast<Package*>(context);
    while (package != NULL && package->Session != NULL && package->Session->IsAlive())
        package->Session->Pump(250);
    return 0;
}

BOOL PackageManager::Activate(Package* package)
{
    if (package == NULL || package->Session != NULL)
        return package != NULL;
    Runtime::IRuntimeAdapter* adapter = Runtimes->FindAdapter(
        package->Manifest.RuntimeId.c_str(), package->Manifest.MinimumRuntimeVersion);
    if (adapter == NULL || !adapter->IsAvailable())
        return FALSE;
    Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = package->Id.c_str();
    request.EntryPoint = package->EntryPoint.c_str();
    request.ParentWindow = General->GetMsgBoxParent();
    request.Flags = Runtime::RuntimeExecutionFlagPersistentWorker |
                    Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.HostDispatch = HostDispatch;
    request.HostDispatchContext = package;
    if (!adapter->StartPersistent(&request, &package->Session) || package->Session == NULL)
        return FALSE;
    if (!package->Session->IsAlive())
    {
        package->Session->Release();
        package->Session = NULL;
        return FALSE;
    }
    package->PumpThread = CreateThread(NULL, 0, PumpThreadProc, package, 0, NULL);
    if (package->PumpThread == NULL)
    {
        package->Session->Stop();
        package->Session->Release();
        package->Session = NULL;
        return FALSE;
    }
    return TRUE;
}

BOOL PackageManager::Deactivate(Package* package)
{
    if (package == NULL || package->Session == NULL)
        return TRUE;
    package->Session->Stop();
    if (package->PumpThread != NULL)
    {
        WaitForSingleObject(package->PumpThread, 5000);
        CloseHandle(package->PumpThread);
        package->PumpThread = NULL;
    }
    package->Session->Release();
    package->Session = NULL;
    return TRUE;
}

BOOL PackageManager::ExecuteCommand(Package* package, const char* commandId, const char* handler)
{
    if (package == NULL || commandId == NULL || Runtimes == NULL)
        return FALSE;
    Runtime::IRuntimeAdapter* adapter = Runtimes->FindAdapter(
        package->Manifest.RuntimeId.c_str(), package->Manifest.MinimumRuntimeVersion);
    if (adapter == NULL || !adapter->IsAvailable())
        return FALSE;
    Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = package->Id.c_str();
    request.CommandId = commandId;
    request.CommandHandler = handler;
    request.EntryPoint = package->EntryPoint.c_str();
    request.ParentWindow = General->GetMsgBoxParent();
    request.Flags = Runtime::RuntimeExecutionFlagUseWorkerBootstrap |
                    Runtime::RuntimeExecutionFlagOneShotWorker;
    request.HostDispatch = HostDispatch;
    request.HostDispatchContext = package;
    Runtime::RuntimeExecutionResult result;
    return adapter->Execute(&request, &result) &&
           result.Status == Runtime::RuntimeExecutionStatusSucceeded;
}

BOOL WINAPI PackageManager::HostDispatch(
    void* context, Runtime::Protocol::MessageType type, ULONGLONG requestId,
    const char* payloadJson, char* resultJson, DWORD resultCapacity, DWORD* resultLength)
{
    UNREFERENCED_PARAMETER(requestId);
    if (type != Runtime::Protocol::MessageCall || context == NULL || payloadJson == NULL)
        return FALSE;
    Package* package = static_cast<Package*>(context);
    PackageManager* owner = package->Owner;
    std::string method;
    if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "method", &method))
        return FALSE;
    if (method == "salamander.ui.notify")
    {
        std::string title, message;
        int timeout = 2500;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "message", &message);
        Runtime::Protocol::Json::FindIntegerMember(payloadJson, "timeoutMs", &timeout);
        BOOL shown = owner->UI != NULL && owner->UI->ShowNotification(
            owner->General->GetMsgBoxParent(), title.c_str(), message.c_str(),
            static_cast<DWORD>(timeout > 0 ? timeout : 2500));
        return CopyResult(std::string("{\"ok\":") + (shown ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.set")
    {
        std::string key, raw;
        if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "key", &key) ||
            !Runtime::Protocol::Json::FindRawMember(payloadJson, "value", &raw) ||
            owner->Storage == NULL)
            return FALSE;
        BOOL stored = FALSE;
        if (raw == "true" || raw == "false")
        {
            BOOL value = raw == "true" ? TRUE : FALSE;
            stored = owner->Storage->SetBoolean(package->Id.c_str(), key.c_str(), value);
        }
        else if (!raw.empty() && raw[0] == '"')
        {
            std::string value;
            stored = Runtime::Protocol::Json::FindStringMember(payloadJson, "value", &value) &&
                     owner->Storage->SetString(package->Id.c_str(), key.c_str(), value.c_str());
        }
        else
        {
            LONGLONG value = 0;
            stored = Runtime::Protocol::Json::FindInteger64Member(payloadJson, "value", &value) &&
                     owner->Storage->SetInteger(package->Id.c_str(), key.c_str(), value);
        }
        return CopyResult(stored ? "{\"ok\":true}" : "{\"ok\":false}",
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.remove")
    {
        std::string key;
        if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "key", &key) ||
            owner->Storage == NULL)
            return FALSE;
        BOOL removed = owner->Storage->DeleteValue(package->Id.c_str(), key.c_str());
        return CopyResult(std::string("{\"ok\":true,\"removed\":") +
                              (removed ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    return CopyResult("{\"ok\":false,\"error\":\"unsupported host method\"}",
                      resultJson, resultCapacity, resultLength);
}

void PackageManager::RegisterToolbarButtons()
{
    if (General == NULL)
        return;
    for (size_t p = 0; p < Packages.size(); ++p)
    {
        Package* package = Packages[p];
        for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
        {
            const CExtensionManifestCommand& command = package->Manifest.Commands[c];
            if (!command.Toolbar)
                continue;
            CSalamanderToolbarButton button;
            button.CommandId = package->CommandIds[c];
            button.Title = command.Title.c_str();
            button.IconPath = package->IconPath.c_str();
            button.IconDarkPath = package->IconDarkPath.empty() ? NULL : package->IconDarkPath.c_str();
            button.StableId = package->Id.c_str();
            General->RegisterToolbarButton(&button);
        }
    }
}

void PackageManager::UnregisterToolbarButtons()
{
    if (General == NULL)
        return;
    for (size_t p = 0; p < Packages.size(); ++p)
        for (size_t c = 0; c < Packages[p]->CommandIds.size(); ++c)
            General->UnregisterToolbarButton(Packages[p]->CommandIds[c]);
}

std::wstring PackageManager::ExpandRoot(const std::wstring& root)
{
    if (root != L"$(SalDir)\\extensions" && root.find(L"$(SalDir)") != 0)
        return root;
    std::vector<wchar_t> module(SAL_MAX_PATH);
    DWORD length = GetModuleFileNameW(NULL, &module[0], static_cast<DWORD>(module.size()));
    if (length == 0 || length >= module.size())
        return root;
    std::wstring salDir(&module[0], length);
    size_t slash = salDir.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        salDir.erase(slash);
    return salDir + root.substr(9);
}

BOOL PackageManager::MakeDisplayEntryPoint(
    const std::wstring& entryPoint,
    std::string* display)
{
    if (display == NULL)
        return FALSE;
    std::wstring salDir = ExpandRoot(L"$(SalDir)");
    std::wstring value = entryPoint;
    if (entryPoint.size() > salDir.size() &&
        _wcsnicmp(entryPoint.c_str(), salDir.c_str(), salDir.size()) == 0 &&
        (entryPoint[salDir.size()] == L'\\' || entryPoint[salDir.size()] == L'/'))
    {
        value = entryPoint.substr(salDir.size() + 1);
    }
    return ToUtf8(value, display);
}

BOOL PackageManager::ReadUtf8File(const std::wstring& path, std::string* text)
{
    if (text == NULL)
        return FALSE;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    LARGE_INTEGER size;
    BOOL ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 && size.QuadPart <= 4 * 1024 * 1024;
    if (ok)
    {
        text->assign(static_cast<size_t>(size.QuadPart), '\0');
        DWORD read = 0;
        ok = size.QuadPart == 0 || ReadFile(file, &(*text)[0], static_cast<DWORD>(size.QuadPart), &read, NULL);
        ok = ok && (size.QuadPart == 0 || read == static_cast<DWORD>(size.QuadPart));
    }
    CloseHandle(file);
    return ok;
}

BOOL PackageManager::ToUtf8(const std::wstring& value, std::string* result)
{
    if (result == NULL)
        return FALSE;
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0, NULL, NULL);
    if (length <= 0)
        return FALSE;
    std::vector<char> buffer(static_cast<size_t>(length));
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1,
                            &buffer[0], length, NULL, NULL) <= 0)
        return FALSE;
    result->assign(&buffer[0]);
    return TRUE;
}

BOOL PackageManager::ToWide(const std::string& value, std::wstring* result)
{
    if (result == NULL)
        return FALSE;
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
    if (length <= 0)
        return FALSE;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1,
                            &buffer[0], length) <= 0)
        return FALSE;
    result->assign(&buffer[0]);
    return TRUE;
}

BOOL PackageManager::CopyResult(const std::string& value, char* result, DWORD capacity, DWORD* length)
{
    if (result == NULL || length == NULL || capacity == 0 || value.size() + 1 > capacity)
        return FALSE;
    memcpy(result, value.c_str(), value.size() + 1);
    *length = static_cast<DWORD>(value.size());
    return TRUE;
}

} // namespace Packages
} // namespace Salamatrix
