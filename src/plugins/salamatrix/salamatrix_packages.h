// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include "salamatrix_manifest.h"
#include "salamatrix_script_runner.h"
#include "salamatrix_runtime_api.h"
#include "salamatrix_extensions.h"
#include "salamatrix_sides.h"
#include "salamatrix_storage.h"
#include "salamatrix_ui.h"

namespace Salamatrix
{
namespace Packages
{

class PackageManager
{
private:
    struct Package;
    class MenuExtension;

    CSalamanderGeneralAbstract* General;
    Runtime::IRuntimeService* Runtimes;
    Extensions::IExtensionsService* Extensions;
    Sides::ISidesService* Sides;
    Storage::IStorageService* Storage;
    UI::IUIService* UI;
    std::vector<std::wstring> Roots;
    std::vector<std::wstring> CustomPackages;
    std::vector<std::string> ExtensionOrder;
    std::vector<std::string> RemovedExtensions;
    std::vector<Package*> Packages;
    MenuExtension* Menu;
    BOOL RefreshInProgress;
    BOOL RefreshPending;
    LONG ActiveHostDispatches;

    PackageManager(const PackageManager&);
    PackageManager& operator=(const PackageManager&);

public:
    PackageManager();
    ~PackageManager();

    BOOL Initialize(
        CSalamanderGeneralAbstract* general,
        Runtime::IRuntimeService* runtimes,
        Extensions::IExtensionsService* extensions,
        Sides::ISidesService* sides,
        Storage::IStorageService* storage,
        UI::IUIService* ui);
    void Shutdown();
    void LoadConfiguration(HKEY key, CSalamanderRegistryAbstract* registry);
    void SaveConfiguration(HKEY key, CSalamanderRegistryAbstract* registry);
    void Refresh();

    CPluginInterfaceForMenuExtAbstract* GetMenuExtension();

private:
    static BOOL WINAPI LifecycleCallback(
        void* context,
        Extensions::ExtensionAction action,
        const Extensions::ExtensionInfo* info);
    static BOOL WINAPI RefreshCallback(void* context);
    static BOOL WINAPI ManagementCallback(
        void* context,
        Extensions::ExtensionManagementAction action,
        const char* extensionId,
        const wchar_t* manifestPath,
        int moveDelta);
    static DWORD WINAPI PumpThreadProc(void* context);
    static BOOL WINAPI HostDispatch(
        void* context,
        Runtime::Protocol::MessageType type,
        ULONGLONG requestId,
        const char* payloadJson,
        char* resultJson,
        DWORD resultCapacity,
        DWORD* resultLength);
    static BOOL WINAPI HostDispatchOnMainThread(void* context);
    static BOOL WINAPI RuntimeDialogEventCallback(
        void* context, const UI::DialogEvent* event);

    void DiscoverRoot(const std::wstring& root);
    void DiscoverDirectory(
        const std::wstring& directory,
        const std::wstring* onlyPackage = NULL);
    void RemovePackages();
    BOOL InstallManifest(const wchar_t* manifestPath);
    BOOL RemoveExtension(const char* extensionId);
    BOOL MoveExtension(const char* extensionId, int delta);
    void ApplyUserOrder();
    bool IsRemoved(const std::string& extensionId) const;
    BOOL Activate(Package* package);
    BOOL Deactivate(Package* package);
    void ReleaseProgress(Package* package);
    void ReleaseDialogs(Package* package);
    BOOL ExecuteCommand(
        Package* package,
        CSalamanderForOperationsAbstract* operations,
        const char* commandId,
        const char* handler);
    void RegisterToolbarButtons();
    void UnregisterToolbarButtons();
    void FinishHostDispatch();

    static std::wstring ExpandRoot(const std::wstring& root);
    static BOOL MakeDisplayEntryPoint(
        const std::wstring& entryPoint,
        std::string* display);
    static BOOL ReadUtf8File(const std::wstring& path, std::string* text);
    static BOOL ToUtf8(const std::wstring& value, std::string* result);
    static BOOL ToWide(const std::string& value, std::wstring* result);
    static BOOL CopyResult(const std::string& value, char* result, DWORD capacity, DWORD* length);
};

} // namespace Packages
} // namespace Salamatrix
