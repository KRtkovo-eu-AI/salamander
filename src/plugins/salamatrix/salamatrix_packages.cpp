// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_packages.h"
#include "salamatrix_api_docs.h"
#include "../../darkmode.h"

#include <algorithm>
#include <memory>
#include <sstream>

namespace Salamatrix
{
namespace Packages
{

namespace
{
const int CommandOpenAutomationApiReference = 0x61ffffff;

struct MainThreadDispatch
{
    void* Context;
    Runtime::Protocol::MessageType Type;
    ULONGLONG RequestId;
    const char* PayloadJson;
    char* ResultJson;
    DWORD ResultCapacity;
    DWORD* ResultLength;
};

static __declspec(thread) MainThreadDispatch* CurrentMainThreadDispatch = NULL;

static std::string JsonEscape(const char* value)
{
    std::string result;
    const unsigned char* current =
        reinterpret_cast<const unsigned char*>(value != NULL ? value : "");
    while (*current != 0)
    {
        switch (*current)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (*current < 0x20)
            {
                char escaped[7];
                _snprintf_s(
                    escaped, _countof(escaped), _TRUNCATE,
                    "\\u%04x", static_cast<unsigned int>(*current));
                result += escaped;
            }
            else
            {
                result.push_back(static_cast<char>(*current));
            }
            break;
        }
        ++current;
    }
    return result;
}

static std::string CurrentSalamanderLocale(
    CSalamanderGeneralAbstract* general, WORD* languageId = NULL)
{
    UNREFERENCED_PARAMETER(general);
    WORD id = SalamanderLanguageID;
    if (id == 0)
        id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    if (languageId != NULL)
        *languageId = id;

    WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
    if (LCIDToLocaleName(
            MAKELCID(id, SORT_DEFAULT), localeName,
            _countof(localeName), 0) == 0)
    {
        return "en-US";
    }
    int length = WideCharToMultiByte(
        CP_UTF8, 0, localeName, -1, NULL, 0, NULL, NULL);
    if (length <= 1)
        return "en-US";
    std::vector<char> utf8(static_cast<size_t>(length));
    if (WideCharToMultiByte(
            CP_UTF8, 0, localeName, -1, &utf8[0], length,
            NULL, NULL) == 0)
    {
        return "en-US";
    }
    return std::string(&utf8[0]);
}

static int LocaleMatchScore(
    const std::string& available, const std::string& preferred)
{
    const size_t availableSeparator = available.find('-');
    const std::string availablePrimary =
        available.substr(0, availableSeparator);
    if (_stricmp(available.c_str(), preferred.c_str()) == 0)
        return 4;
    const size_t preferredSeparator = preferred.find('-');
    const std::string preferredPrimary =
        preferred.substr(0, preferredSeparator);
    if (_stricmp(availablePrimary.c_str(), preferredPrimary.c_str()) == 0)
        return availableSeparator == std::string::npos ? 3 : 2;
    return _stricmp(availablePrimary.c_str(), "en") == 0 ? 1 : 0;
}

static const char* FindLocalizedCommandTitle(
    const CExtensionManifestLocaleText& localized,
    const std::string& commandId)
{
    for (size_t index = 0; index < localized.Commands.size(); ++index)
    {
        if (_stricmp(
                localized.Commands[index].Id.c_str(),
                commandId.c_str()) == 0)
        {
            return localized.Commands[index].Title.c_str();
        }
    }
    return NULL;
}

static const CExtensionManifestLocalizedSetting* FindLocalizedSetting(
    const CExtensionManifestLocaleText& localized,
    const std::string& key)
{
    for (size_t index = 0; index < localized.Settings.size(); ++index)
    {
        if (_stricmp(
                localized.Settings[index].Key.c_str(),
                key.c_str()) == 0)
        {
            return &localized.Settings[index];
        }
    }
    return NULL;
}

static Automation::IScriptRunner* QueryScriptRunner(
    CSalamanderGeneralAbstract* general)
{
    if (general == NULL)
        return NULL;
    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    memset(&query, 0, sizeof(query));
    memset(&result, 0, sizeof(result));
    query.ServiceId = SALAMATRIX_SERVICE_SCRIPT_RUNNER;
    query.MinimumVersion = SALAMATRIX_SCRIPT_RUNNER_VERSION_1_0;
    if (!general->QueryService(&query, &result) || result.Interface == NULL)
        return NULL;
    return static_cast<Automation::IScriptRunner*>(result.Interface);
}
}

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
    BOOL RuntimeUsable;
    CSalamanderForOperationsAbstract* Operations;
    UI::IProgressDialog* Progress;
    ULONGLONG ProgressId;
    std::vector<int> CommandIds;
    std::vector<std::string> CommandIconPaths;
    std::vector<int> MenuIconIndices;
    Runtime::IRuntimeSession* Session;
    HANDLE PumpThread;

    Package(PackageManager* owner)
        : Owner(owner),
          RuntimeUsable(FALSE),
          Operations(NULL),
          Progress(NULL),
          ProgressId(0),
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
        if (id == CommandOpenAutomationApiReference)
        {
            char path[SAL_MAX_PATH];
            return Documentation::GetAutomationApiReferencePath(
                       Owner->General, path, _countof(path))
                       ? MENU_ITEM_STATE_ENABLED
                       : 0;
        }
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (!package->RuntimeUsable)
                continue;
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
        UNREFERENCED_PARAMETER(eventMask);
        if (Owner == NULL)
            return FALSE;
        if (id == CommandOpenAutomationApiReference)
            return Documentation::OpenAutomationApiReference(
                Owner->General, parent);
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (!package->RuntimeUsable)
                continue;
            for (size_t c = 0; c < package->CommandIds.size(); ++c)
            {
                if (package->CommandIds[c] == id &&
                    c < package->Manifest.Commands.size())
                {
                    const CExtensionManifestCommand& command =
                        package->Manifest.Commands[c];
                    return Owner->ExecuteCommand(
                        package, salamander,
                        command.Id.c_str(), command.Handler.c_str());
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
        {
            if (!Owner->Packages[p]->RuntimeUsable)
                continue;
            for (size_t c = 0; c < Owner->Packages[p]->Manifest.Commands.size(); ++c)
                if (Owner->Packages[p]->Manifest.Commands[c].Menu == "plugin" ||
                    Owner->Packages[p]->Manifest.Commands[c].Menu == "both")
                    ++iconCount;
        }

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
        bool packageMenuItemAdded = false;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (!package->RuntimeUsable)
                continue;
            package->MenuIconIndices.clear();
            int packageMenuCommandCount = 0;
            for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
            {
                const std::string& menu = package->Manifest.Commands[c].Menu;
                if (menu == "plugin" || menu == "both")
                    ++packageMenuCommandCount;
            }
            if (packageMenuCommandCount > 1)
            {
                char packageTitle[256];
                StringCchCopyA(
                    packageTitle, _countof(packageTitle),
                    package->Manifest.Name.c_str());
                if (Owner->General != NULL)
                    Owner->General->DuplicateAmpersands(
                        packageTitle, _countof(packageTitle));
                builder->AddSubmenuStart(
                    -1, packageTitle, 0, FALSE,
                    MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
            }
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
                    const bool usesPackageIcon = command.Icon.empty();
                    const bool useDarkPackageIcon =
                        DarkModeIsWindowsDarkSchemeSelected() &&
                        usesPackageIcon && !package->IconDarkPath.empty();
                    const char* preferredPath = useDarkPackageIcon
                                                    ? package->IconDarkPath.c_str()
                                                    : iconPath.c_str();
                    HICON icon = iconPath.empty()
                                     ? NULL
                                     : SalamanderGUI->CreateSVGIcon(
                                           preferredPath, 16);
                    if (icon == NULL && useDarkPackageIcon)
                        icon = SalamanderGUI->CreateSVGIcon(iconPath.c_str(), 16);
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
                packageMenuItemAdded = true;
            }
            if (packageMenuCommandCount > 1)
                builder->AddSubmenuEnd();
        }
        if (packageMenuItemAdded)
            builder->AddMenuItem(-1, NULL, 0, 0, FALSE, 0, 0,
                                 MENU_SKILLLEVEL_ALL);
        builder->AddMenuItem(
            -1, "Automation API &Reference...", 0,
            CommandOpenAutomationApiReference, TRUE,
            MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
    }
};

PackageManager::PackageManager()
    : General(NULL),
      Runtimes(NULL),
      Extensions(NULL),
      Sides(NULL),
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
    Sides::ISidesService* sides,
    Storage::IStorageService* storage,
    UI::IUIService* ui)
{
    General = general;
    Runtimes = runtimes;
    Extensions = extensions;
    Sides = sides;
    Storage = storage;
    UI = ui;
    if (Menu == NULL)
        Menu = new MenuExtension(this);
    if (Extensions != NULL)
        Extensions->SetRefreshCallback(RefreshCallback, this);
    return General != NULL && Runtimes != NULL && Extensions != NULL &&
           Sides != NULL;
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
    Sides = NULL;
    Storage = NULL;
    UI = NULL;
}

void PackageManager::LoadConfiguration(HKEY key, CSalamanderRegistryAbstract* registry)
{
    Roots.clear();
    Roots.push_back(ExpandRoot(L"$(SalDir)\\extensions"));
    Roots.push_back(ExpandRoot(L"$(SalDir)\\plugins\\automation\\scripts"));
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
        if (root != ExpandRoot(L"$(SalDir)\\extensions") &&
            root != ExpandRoot(L"$(SalDir)\\plugins\\automation\\scripts"))
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
                    const std::string baseName = manifest.Name;
                    const std::string preferred =
                        CurrentSalamanderLocale(General);
                    int selectedLocale = -1;
                    int bestLocaleScore = 0;
                    for (size_t localeIndex = 0;
                         localeIndex < manifest.Locales.size();
                         ++localeIndex)
                    {
                        const int score = LocaleMatchScore(
                            manifest.Locales[localeIndex].Language,
                            preferred);
                        if (score > bestLocaleScore)
                        {
                            selectedLocale =
                                static_cast<int>(localeIndex);
                            bestLocaleScore = score;
                        }
                    }
                    if (selectedLocale >= 0)
                    {
                        std::wstring localeRelative;
                        std::string localeJson;
                        CExtensionManifestLocaleText localized;
                        CExtensionManifestError localeError;
                        if (ToWide(
                                manifest.Locales[selectedLocale].File,
                                &localeRelative) &&
                            ReadUtf8File(
                                path + L"\\" + localeRelative,
                                &localeJson) &&
                            CExtensionManifest::ParseLocaleText(
                                localeJson.data(), localeJson.size(),
                                localized, localeError))
                        {
                            if (!localized.Name.empty())
                                manifest.Name = localized.Name;
                            for (size_t commandIndex = 0;
                                 commandIndex < manifest.Commands.size();
                                 ++commandIndex)
                            {
                                const char* title =
                                    FindLocalizedCommandTitle(
                                        localized,
                                        manifest.Commands[commandIndex].Id);
                                if (title != NULL)
                                    manifest.Commands[commandIndex].Title = title;
                                else if (
                                    manifest.Commands[commandIndex].Title ==
                                    baseName)
                                    manifest.Commands[commandIndex].Title =
                                        manifest.Name;
                            }
                            for (size_t settingIndex = 0;
                                 settingIndex < manifest.Settings.size();
                                 ++settingIndex)
                            {
                                CExtensionManifestSetting& setting =
                                    manifest.Settings[settingIndex];
                                const CExtensionManifestLocalizedSetting*
                                    translated = FindLocalizedSetting(
                                        localized, setting.Key);
                                if (translated == NULL)
                                    continue;
                                if (!translated->Label.empty())
                                    setting.Label = translated->Label;
                                if (!translated->Description.empty())
                                    setting.Description =
                                        translated->Description;
                                if (!translated->Group.empty())
                                    setting.Group = translated->Group;
                            }
                        }
                    }
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

                    if (!registeredRuntime &&
                        _stricmp(manifest.RuntimeId.c_str(), "Automation.JScript") == 0 &&
                        QueryScriptRunner(General) != NULL)
                    {
                        // The legacy JScript provider is owned by Automation.
                        // Its public ScriptRunner service is sufficient for
                        // one-shot extension commands even if the compatibility
                        // adapter registration raced framework startup.
                        descriptor.Flags &=
                            ~(Extensions::ExtensionFlagRuntimeUnavailable |
                              Extensions::ExtensionFlagRuntimeExecutableUnavailable);
                        registeredRuntime = true;
                        availableRuntime = true;
                    }
                    package->RuntimeUsable = registeredRuntime && availableRuntime;
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
        ReleaseProgress(package);
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

void PackageManager::ReleaseProgress(Package* package)
{
    if (package == NULL || package->Progress == NULL)
        return;
    package->Progress->Close();
    if (UI != NULL)
        UI->DestroyProgressDialog(package->Progress);
    package->Progress = NULL;
    package->ProgressId = 0;
}

BOOL PackageManager::ExecuteCommand(
    Package* package,
    CSalamanderForOperationsAbstract* operations,
    const char* commandId,
    const char* handler)
{
    if (package == NULL || commandId == NULL || Runtimes == NULL)
        return FALSE;
    Runtime::IRuntimeAdapter* adapter = Runtimes->FindAdapter(
        package->Manifest.RuntimeId.c_str(), package->Manifest.MinimumRuntimeVersion);
    if (adapter == NULL || !adapter->IsAvailable())
    {
        if (_stricmp(package->Manifest.RuntimeId.c_str(), "Automation.JScript") == 0)
        {
            Automation::IScriptRunner* scriptRunner = QueryScriptRunner(General);
            if (scriptRunner != NULL)
            {
                Automation::GeneratedScriptRequest compatibilityRequest;
                compatibilityRequest.EntryPoint = package->EntryPoint.c_str();
                compatibilityRequest.RuntimeId = package->Manifest.RuntimeId.c_str();
                compatibilityRequest.ExtensionId = package->Id.c_str();
                compatibilityRequest.ParentWindow = General->GetMsgBoxParent();
                compatibilityRequest.TimeoutMs = 120000;
                compatibilityRequest.Operation = operations;
                Automation::GeneratedScriptResult compatibilityResult;
                BOOL executed = scriptRunner->ExecuteGenerated(
                    &compatibilityRequest, &compatibilityResult);
                package->Operations = NULL;
                return executed;
            }
        }
        return FALSE;
    }
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
    package->Operations = operations;
    ReleaseProgress(package);
    Runtime::IRuntimeSession* session = NULL;
    if (!adapter->StartPersistent(&request, &session) || session == NULL)
    {
        const Runtime::RuntimeAdapterDescriptor* adapterDescriptor =
            adapter->GetDescriptor();
        if (adapterDescriptor == NULL ||
            (adapterDescriptor->Flags & Runtime::RuntimeAdapterFlagCompatibility) == 0)
        {
            package->Operations = NULL;
            return FALSE;
        }
        Automation::IScriptRunner* scriptRunner = QueryScriptRunner(General);
        if (scriptRunner != NULL)
        {
            Automation::GeneratedScriptRequest compatibilityRequest;
            compatibilityRequest.EntryPoint = request.EntryPoint;
            compatibilityRequest.RuntimeId = package->Manifest.RuntimeId.c_str();
            compatibilityRequest.ExtensionId = package->Id.c_str();
            compatibilityRequest.ParentWindow = request.ParentWindow;
            compatibilityRequest.TimeoutMs = request.TimeoutMs;
            compatibilityRequest.Operation = operations;
            Automation::GeneratedScriptResult compatibilityResult;
            BOOL executed = scriptRunner->ExecuteGenerated(
                &compatibilityRequest, &compatibilityResult);
            package->Operations = NULL;
            return executed;
        }
        package->Operations = NULL;
        return FALSE;
    }

    const ULONGLONG startedAt = GetTickCount64();
    while (session->IsAlive() && GetTickCount64() - startedAt < request.TimeoutMs)
        session->Pump(250);

    if (session->IsAlive())
        session->Stop();
    DWORD exitCode = 1;
    BOOL succeeded = session->GetExitCode(&exitCode) && exitCode == 0;
    session->Release();
    ReleaseProgress(package);
    package->Operations = NULL;
    return succeeded;
}

BOOL WINAPI PackageManager::HostDispatch(
    void* context, Runtime::Protocol::MessageType type, ULONGLONG requestId,
    const char* payloadJson, char* resultJson, DWORD resultCapacity, DWORD* resultLength)
{
    UNREFERENCED_PARAMETER(requestId);
    if (context == NULL || payloadJson == NULL)
        return FALSE;
    Package* package = static_cast<Package*>(context);
    PackageManager* owner = package->Owner;
    if (type == Runtime::Protocol::MessageHello)
        return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
    if (type != Runtime::Protocol::MessageCall)
        return FALSE;
    if (CurrentMainThreadDispatch == NULL && owner->General != NULL)
    {
        MainThreadDispatch call = {
            context, type, requestId, payloadJson,
            resultJson, resultCapacity, resultLength};
        return owner->General->InvokeOnMainThread(
            HostDispatchOnMainThread, &call, 120000);
    }
    std::string method;
    if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "method", &method))
        return FALSE;
    if (method == "salamander.host.language")
    {
        WORD languageId = 0;
        const std::string locale =
            CurrentSalamanderLocale(owner->General, &languageId);
        return CopyResult(
            std::string("{\"ok\":true,\"locale\":\"") +
                JsonEscape(locale.c_str()) +
                "\",\"languageId\":" +
                std::to_string(static_cast<unsigned int>(languageId)) + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.context")
    {
        std::string sideName;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Sides::SideReference side = Sides::SideReferenceSource;
        if (_stricmp(sideName.c_str(), "left") == 0)
            side = Sides::SideReferenceLeft;
        else if (_stricmp(sideName.c_str(), "right") == 0)
            side = Sides::SideReferenceRight;
        else if (_stricmp(sideName.c_str(), "target") == 0)
            side = Sides::SideReferenceTarget;
        if (owner->Sides == NULL)
            return FALSE;
        char path[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
        path[0] = '\0';
        int pathType = 0;
        if (!owner->Sides->GetPath(
                side, path, _countof(path), &pathType))
            return FALSE;
        return CopyResult(
            std::string("{\"ok\":true,\"path\":\"") +
                JsonEscape(path) +
                "\",\"pathType\":" + std::to_string(pathType) +
                ",\"selectedCount\":" +
                std::to_string(owner->Sides->GetSelectedItemCount(side)) +
                ",\"selectedItems\":[],\"focusedItem\":null}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.createTab")
    {
        if (owner->Sides == NULL)
            return FALSE;
        std::string sideName;
        std::string path;
        int index = -1;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "side", &sideName) ||
            !Runtime::Protocol::Json::FindStringMember(
                payloadJson, "path", &path))
        {
            return FALSE;
        }
        Runtime::Protocol::Json::FindIntegerMember(
            payloadJson, "index", &index);
        Sides::SideReference side = Sides::SideReferenceSource;
        if (_stricmp(sideName.c_str(), "left") == 0)
            side = Sides::SideReferenceLeft;
        else if (_stricmp(sideName.c_str(), "right") == 0)
            side = Sides::SideReferenceRight;
        else if (_stricmp(sideName.c_str(), "target") == 0)
            side = Sides::SideReferenceTarget;
        ULONGLONG tabId = 0;
        const BOOL created = owner->Sides->CreateTab(
            side, path.c_str(), index, &tabId);
        char id[32];
        _ui64toa_s(tabId, id, _countof(id), 10);
        return CopyResult(
            std::string("{\"created\":") +
                (created ? "true" : "false") +
                ",\"tabId\":\"" + id + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.clipboard.copyText")
    {
        std::string text;
        BOOL showEcho = FALSE;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "text", &text) ||
            owner->UI == NULL)
        {
            return FALSE;
        }
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "showEcho", &showEcho);
        const BOOL copied = owner->UI->CopyTextToClipboard(
            text.c_str(), showEcho, owner->General->GetMsgBoxParent());
        return CopyResult(
            std::string("{\"ok\":true,\"copied\":") +
                (copied ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
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
        return CopyResult(std::string("{\"ok\":true,\"shown\":") +
                              (shown ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.messageBox")
    {
        std::string title, message;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "message", &message);
        int result = owner->UI != NULL
                         ? owner->UI->ShowMessageBox(
                               owner->General->GetMsgBoxParent(),
                               message.c_str(), title.c_str(),
                               MB_OK | MB_ICONINFORMATION)
                         : 0;
        return CopyResult(
            std::string("{\"ok\":true,\"result\":") +
                std::to_string(result) + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.progress.create" ||
        method == "salamander.ui.progress.update" ||
        method == "salamander.ui.progress.step" ||
        method == "salamander.ui.progress.setTotals" ||
        method == "salamander.ui.progress.setPositions" ||
        method == "salamander.ui.progress.setTitle" ||
        method == "salamander.ui.progress.setCancelEnabled" ||
        method == "salamander.ui.progress.cancelled" ||
        method == "salamander.ui.progress.close")
    {
        if (owner->UI == NULL)
            return CopyResult(
                "{\"ok\":false,\"error\":\"progress service unavailable\"}",
                resultJson, resultCapacity, resultLength);

        if (method == "salamander.ui.progress.create")
        {
            if (package->Operations == NULL)
                return CopyResult(
                    "{\"ok\":false,\"error\":\"progress requires an operation context\"}",
                    resultJson, resultCapacity, resultLength);
            owner->ReleaseProgress(package);
            std::string title;
            Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
            BOOL twoProgressBars = FALSE;
            BOOL fileProgress = FALSE;
            BOOL cancelEnabled = TRUE;
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "twoProgressBars", &twoProgressBars);
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "fileProgress", &fileProgress);
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "cancelEnabled", &cancelEnabled);
            UI::ProgressDialogOptions options;
            options.Title = title.empty() ? "Salamatrix" : title.c_str();
            options.Parent = owner->General->GetMsgBoxParent();
            options.TwoProgressBars = twoProgressBars;
            options.FileProgress = fileProgress;
            options.CancelEnabled = cancelEnabled;
            package->Progress = owner->UI->CreateProgressDialog(package->Operations);
            if (package->Progress == NULL)
                return CopyResult(
                    "{\"ok\":false,\"error\":\"progress dialog unavailable\"}",
                    resultJson, resultCapacity, resultLength);
            package->Progress->Open(options);
            LONGLONG total = 0;
            if (Runtime::Protocol::Json::FindInteger64Member(
                    payloadJson, "total", &total) && total >= 0)
            {
                LONGLONG total2 = 0;
                if (Runtime::Protocol::Json::FindInteger64Member(
                        payloadJson, "total2", &total2) && total2 >= 0)
                {
                    CQuadWord first;
                    CQuadWord second;
                    first.SetUI64(static_cast<unsigned __int64>(total));
                    second.SetUI64(static_cast<unsigned __int64>(total2));
                    package->Progress->SetTotals(first, second);
                }
                else
                {
                    CQuadWord first;
                    first.SetUI64(static_cast<unsigned __int64>(total));
                    package->Progress->SetTotal(first);
                }
            }
            package->ProgressId = package->ProgressId == static_cast<ULONGLONG>(-1)
                                      ? 1
                                      : package->ProgressId + 1;
            if (package->ProgressId == 0)
                package->ProgressId = 1;
            char idText[32];
            _ui64toa_s(package->ProgressId, idText, _countof(idText), 10);
            return CopyResult(
                std::string("{\"ok\":true,\"progressId\":\"") +
                    idText + "\"}",
                resultJson, resultCapacity, resultLength);
        }

        std::string idText;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "progressId", &idText))
            return FALSE;
        char* idEnd = NULL;
        ULONGLONG progressId = _strtoui64(idText.c_str(), &idEnd, 10);
        if (idEnd == idText.c_str() || *idEnd != '\0' ||
            package->Progress == NULL || package->ProgressId != progressId)
            return FALSE;
        UI::IProgressDialog* progress = package->Progress;
        if (method == "salamander.ui.progress.close")
        {
            owner->ReleaseProgress(package);
            return CopyResult(
                "{\"ok\":true,\"closed\":true}",
                resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.cancelled")
            return CopyResult(
                std::string("{\"ok\":true,\"cancelled\":") +
                    (progress->IsCancelled() ? "true}" : "false}"),
                resultJson, resultCapacity, resultLength);
        if (method == "salamander.ui.progress.setTitle")
        {
            std::string title;
            if (!Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "title", &title))
                return FALSE;
            progress->SetTitle(title.c_str());
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setCancelEnabled")
        {
            BOOL enabled = TRUE;
            Runtime::Protocol::Json::FindBoolMember(payloadJson, "enabled", &enabled);
            progress->SetCancelEnabled(enabled);
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setTotals")
        {
            LONGLONG total = 0;
            LONGLONG total2 = 0;
            if (!Runtime::Protocol::Json::FindInteger64Member(payloadJson, "total", &total) ||
                !Runtime::Protocol::Json::FindInteger64Member(payloadJson, "total2", &total2) ||
                total < 0 || total2 < 0)
                return FALSE;
            CQuadWord first;
            CQuadWord second;
            first.SetUI64(static_cast<unsigned __int64>(total));
            second.SetUI64(static_cast<unsigned __int64>(total2));
            progress->SetTotals(first, second);
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        BOOL delayedPaint = TRUE;
        Runtime::Protocol::Json::FindBoolMember(payloadJson, "delayedPaint", &delayedPaint);
        BOOL continued = TRUE;
        if (method == "salamander.ui.progress.step")
        {
            int amount = 1;
            Runtime::Protocol::Json::FindIntegerMember(payloadJson, "amount", &amount);
            continued = progress->Step(amount, delayedPaint);
        }
        else
        {
            LONGLONG position = 0;
            if (!Runtime::Protocol::Json::FindInteger64Member(payloadJson, "position", &position) ||
                position < 0)
                return FALSE;
            CQuadWord first;
            first.SetUI64(static_cast<unsigned __int64>(position));
            LONGLONG total = 0;
            if (Runtime::Protocol::Json::FindInteger64Member(payloadJson, "total", &total) && total >= 0)
            {
                CQuadWord value;
                value.SetUI64(static_cast<unsigned __int64>(total));
                progress->SetTotal(value);
            }
            std::string text;
            Runtime::Protocol::Json::FindStringMember(payloadJson, "text", &text);
            if (!text.empty())
                progress->AddText(text.c_str(), delayedPaint);
            LONGLONG position2 = 0;
            if (Runtime::Protocol::Json::FindInteger64Member(payloadJson, "position2", &position2) && position2 >= 0)
            {
                CQuadWord second;
                second.SetUI64(static_cast<unsigned __int64>(position2));
                continued = progress->SetPositions(first, second, delayedPaint);
            }
            else
                continued = progress->SetPosition(first, delayedPaint);
        }
        return CopyResult(
            std::string("{\"ok\":true,\"continued\":") +
                (continued && !progress->IsCancelled() ? "true}" : "false}"),
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

BOOL WINAPI PackageManager::HostDispatchOnMainThread(void* context)
{
    MainThreadDispatch* call = static_cast<MainThreadDispatch*>(context);
    if (call == NULL)
        return FALSE;
    MainThreadDispatch* previous = CurrentMainThreadDispatch;
    CurrentMainThreadDispatch = call;
    BOOL result = HostDispatch(
        call->Context, call->Type, call->RequestId, call->PayloadJson,
        call->ResultJson, call->ResultCapacity, call->ResultLength);
    CurrentMainThreadDispatch = previous;
    return result;
}

void PackageManager::RegisterToolbarButtons()
{
    if (General == NULL)
        return;
    for (size_t p = 0; p < Packages.size(); ++p)
    {
        Package* package = Packages[p];
        if (!package->RuntimeUsable)
            continue;
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
