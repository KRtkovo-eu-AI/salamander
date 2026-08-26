// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_extensions.h
    Extension descriptor and lifecycle registry.
*/

#pragma once

#include <stddef.h>
#include <string.h>

namespace Salamatrix
{
namespace Extensions
{

#define SALAMATRIX_SERVICE_EXTENSIONS "Salamatrix.Extensions"
#define SALAMATRIX_EXTENSIONS_VERSION_1_0 0x00010000
#define SALAMATRIX_EXTENSIONS_VERSION_1_1 0x00010001
#define SALAMATRIX_EXTENSIONS_VERSION_1_2 0x00010002
#define SALAMATRIX_EXTENSIONS_VERSION_1_3 0x00010003
#define SALAMATRIX_EXTENSIONS_VERSION_1_4 0x00010004

enum ExtensionState
{
    ExtensionStateDiscovered = 1,
    ExtensionStateActivating = 2,
    ExtensionStateActive = 3,
    ExtensionStateDeactivating = 4,
    ExtensionStateInactive = 5,
    ExtensionStateFailed = 6,
    ExtensionStateWaitingForRuntime = 7,
    ExtensionStateWaitingForDependency = 8,
    ExtensionStateDisabled = 9
};

enum ExtensionAction
{
    ExtensionActionActivate = 1,
    ExtensionActionDeactivate = 2
};

enum ExtensionSettingType
{
    ExtensionSettingString = 1,
    ExtensionSettingInteger = 2,
    ExtensionSettingBoolean = 3
};

enum ExtensionFlags
{
    ExtensionFlagNone = 0x00000000,
    ExtensionFlagManifest = 0x00000001,
    ExtensionFlagPersistent = 0x00000002,
    ExtensionFlagCompatibility = 0x00000004,
    ExtensionFlagRuntimeUnavailable = 0x00000008,
    ExtensionFlagDependencyUnavailable = 0x00000010,
    // User-controlled persistent disabled state. This is separate from a
    // missing runtime/dependency so Plugin Manager can explain the reason.
    ExtensionFlagDisabled = 0x00000020,
    // A schema-1 extension package (manifest + entry point + assets), rather
    // than a legacy one-shot Automation script.
    ExtensionFlagPackage = 0x00000040,
    // The provider is registered, but its interpreter/server executable is
    // not available on this machine (for example node.exe or php.exe).
    ExtensionFlagRuntimeExecutableUnavailable = 0x00000080,
    // Declarative contribution metadata. These bits let generic management
    // surfaces describe what a manifest extension adds without parsing its
    // package or depending on the owning runtime provider.
    ExtensionFlagMenuExtension = 0x00000100,
    ExtensionFlagViewer = 0x00000200,
    ExtensionFlagFileSystem = 0x00000400
};

struct ExtensionDescriptor
{
    DWORD StructSize;
    char Id[128];
    char Name[256];
    char Version[64];
    char RuntimeId[128];
    char EntryPoint[32768];
    // Absolute UTF-8 package asset paths (resolved by the discovery layer).
    char IconPath[32768];
    char IconDarkPath[32768];
    DWORD Flags;
    char HomePageUrl[1024];
    // Optional Plugin Manager Security disclosure copied from extension.json.
    char NetworkAccess[32];
    char ExternalProcesses[32];
    char ScriptExecution[32];
    char ActiveWebContent[32];
    char Elevation[32];

    ExtensionDescriptor()
        : StructSize(sizeof(ExtensionDescriptor)),
          Flags(ExtensionFlagNone)
    {
        Id[0] = 0;
        Name[0] = 0;
        Version[0] = 0;
        RuntimeId[0] = 0;
        EntryPoint[0] = 0;
        IconPath[0] = 0;
        IconDarkPath[0] = 0;
        HomePageUrl[0] = 0;
        NetworkAccess[0] = 0;
        ExternalProcesses[0] = 0;
        ScriptExecution[0] = 0;
        ActiveWebContent[0] = 0;
        Elevation[0] = 0;
    }
};

struct ExtensionInfo
{
    DWORD StructSize;
    ExtensionDescriptor Descriptor;
    ExtensionState State;

    ExtensionInfo()
        : StructSize(sizeof(ExtensionInfo)),
          State(ExtensionStateDiscovered)
    {
    }
};

// Data-only schema. Values remain in Salamatrix.Storage; native callers use
// this only to render the same configuration UI for every runtime.
struct ExtensionSettingInfo
{
    DWORD StructSize;
    char Key[128];
    ExtensionSettingType Type;
    // Appended presentation metadata. Existing providers may continue to
    // submit the legacy prefix ending after Type.
    char Label[256];
    char Description[1024];
    char Group[128];
    int Order;
    int Width;
    BOOL Multiline;

    ExtensionSettingInfo()
        : StructSize(sizeof(ExtensionSettingInfo)),
          Type(ExtensionSettingString),
          Order(0),
          Width(250),
          Multiline(FALSE)
    {
        Key[0] = 0;
        Label[0] = 0;
        Description[0] = 0;
        Group[0] = 0;
    }
};

// Minimum prefix accepted by the 1.2 setting-schema methods. Keep this
// before the appended display fields so old providers remain ABI-compatible.
static const DWORD ExtensionSettingInfoLegacySize =
    static_cast<DWORD>(offsetof(ExtensionSettingInfo, Label));

typedef BOOL(WINAPI* ExtensionLifecycleCallback)(
    void* context,
    ExtensionAction action,
    const ExtensionInfo* info);

typedef BOOL(WINAPI* ExtensionRefreshCallback)(void* context);

enum ExtensionManagementAction
{
    ExtensionManagementInstallManifest = 1,
    ExtensionManagementRemove = 2,
    ExtensionManagementMove = 3
};

typedef BOOL(WINAPI* ExtensionManagementCallback)(
    void* context,
    ExtensionManagementAction action,
    const char* extensionId,
    const wchar_t* manifestPath,
    int moveDelta);

class IExtensionsService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual BOOL WINAPI RegisterExtension(
        const ExtensionDescriptor* descriptor,
        ExtensionLifecycleCallback callback,
        void* context) = 0;
    virtual BOOL WINAPI UnregisterExtension(
        const char* extensionId,
        void* context) = 0;
    virtual int WINAPI UnregisterOwner(void* context) = 0;
    virtual BOOL WINAPI ActivateExtension(const char* extensionId) = 0;
    virtual BOOL WINAPI DeactivateExtension(const char* extensionId) = 0;
    virtual int WINAPI GetExtensionCount() const = 0;
    virtual BOOL WINAPI GetExtensionInfo(
        int index,
        ExtensionInfo* info) const = 0;
    virtual BOOL WINAPI FindExtension(
        const char* extensionId,
        ExtensionInfo* info) const = 0;

    /// Optional unload lease appended to the lifecycle ABI. A host callback
    /// acquires a short borrowed lease before touching extension state; unload
    /// waits until all such callbacks have returned.
    virtual BOOL WINAPI AcquireExtension(
        const char* extensionId,
        void* owner)
    {
        (void)extensionId;
        (void)owner;
        return FALSE;
    }

    virtual void WINAPI ReleaseExtension(
        const char* extensionId,
        void* owner)
    {
        (void)extensionId;
        (void)owner;
    }

    /// Appended in 1.1. Changes the runtime registry state only; the owning
    /// discovery layer persists the user's choice in Salamatrix.Storage.
    virtual BOOL WINAPI SetExtensionDisabled(
        const char* extensionId,
        BOOL disabled)
    {
        (void)extensionId;
        (void)disabled;
        return FALSE;
    }

    /// Appended in 1.2. Owners replace their data-only setting schema;
    /// consumers retrieve it for shared configuration UI.
    virtual BOOL WINAPI SetExtensionSettingsSchema(
        const char* extensionId,
        const ExtensionSettingInfo* settings,
        int settingCount)
    {
        (void)extensionId;
        (void)settings;
        (void)settingCount;
        return FALSE;
    }

    virtual int WINAPI GetExtensionSettingCount(const char* extensionId) const
    {
        (void)extensionId;
        return 0;
    }

    virtual BOOL WINAPI GetExtensionSettingInfo(
        const char* extensionId,
        int index,
        ExtensionSettingInfo* setting) const
    {
        (void)extensionId;
        (void)index;
        (void)setting;
        return FALSE;
    }

    /// Requests a framework-owned discovery refresh. Appended so older
    /// extension-service consumers retain the original vtable prefix.
    virtual BOOL WINAPI Refresh()
    {
        return FALSE;
    }

    virtual BOOL WINAPI SetRefreshCallback(
        ExtensionRefreshCallback callback,
        void* context)
    {
        (void)callback;
        (void)context;
        return FALSE;
    }

    /// Appended in 1.3. These requests are implemented by the framework-owned
    /// package manager, which also persists custom manifests, removals and
    /// user ordering. Keeping them on the registry service lets the native
    /// Plugin Manager remain independent of the provider plug-in class.
    virtual BOOL WINAPI InstallExtensionManifest(const wchar_t* manifestPath)
    {
        (void)manifestPath;
        return FALSE;
    }

    virtual BOOL WINAPI RemoveManagedExtension(const char* extensionId)
    {
        (void)extensionId;
        return FALSE;
    }

    virtual BOOL WINAPI MoveManagedExtension(
        const char* extensionId,
        int delta)
    {
        (void)extensionId;
        (void)delta;
        return FALSE;
    }

    virtual BOOL WINAPI SetManagementCallback(
        ExtensionManagementCallback callback,
        void* context)
    {
        (void)callback;
        (void)context;
        return FALSE;
    }

    /// Reorders the current registry snapshot without changing ownership or
    /// lifecycle state. The package manager calls this after discovery so an
    /// automatic refresh cannot overwrite the user's order.
    virtual BOOL WINAPI ApplyExtensionOrder(
        const char* const* extensionIds,
        int extensionCount)
    {
        (void)extensionIds;
        (void)extensionCount;
        return FALSE;
    }

protected:
    virtual ~IExtensionsService() {}
};

class ExtensionsService : public IExtensionsService
{
private:
    enum
    {
        MaxExtensions = 256
    };

    struct Record
    {
        enum
        {
            MaxSettings = 64
        };
        ExtensionInfo Info;
        ExtensionLifecycleCallback Callback;
        void* Context;
    LONG ActiveLeases;
        ExtensionSettingInfo Settings[MaxSettings];
    int SettingCount;

        Record()
            : Callback(NULL),
              Context(NULL),
              ActiveLeases(0),
              SettingCount(0)
        {
        }
    };

    Record Records[MaxExtensions];
    int RecordCount;
    mutable CRITICAL_SECTION Lock;
    CONDITION_VARIABLE LeaseChanged;
    ExtensionRefreshCallback RefreshCallback;
    void* RefreshContext;
    ExtensionManagementCallback ManagementCallback;
    void* ManagementContext;

    ExtensionsService(const ExtensionsService&);
    ExtensionsService& operator=(const ExtensionsService&);

    int FindIndex(const char* extensionId) const
    {
        if (extensionId == NULL)
            return -1;
        for (int index = 0; index < RecordCount; ++index)
        {
            if (_stricmp(
                    Records[index].Info.Descriptor.Id,
                    extensionId) == 0)
            {
                return index;
            }
        }
        return -1;
    }

    static BOOL IsValidId(const char* id)
    {
        if (id == NULL || id[0] == 0)
            return FALSE;
        int length = 0;
        while (id[length] != 0)
        {
            unsigned char character =
                static_cast<unsigned char>(id[length]);
            if (!((character >= 'A' && character <= 'Z') ||
                  (character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9') ||
                  character == '.' || character == '_' || character == '-'))
            {
                return FALSE;
            }
            if (++length >= 128)
                return FALSE;
        }
        return TRUE;
    }

    static void CopyInfo(
        const Record& record,
        ExtensionInfo* info)
    {
        *info = record.Info;
    }

    BOOL RunAction(
        const char* extensionId,
        ExtensionAction action)
    {
        ExtensionLifecycleCallback callback = NULL;
        void* context = NULL;
        ExtensionInfo info;
        int index = -1;

        EnterCriticalSection(&Lock);
        index = FindIndex(extensionId);
        if (index < 0)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        Record& record = Records[index];
        if (action == ExtensionActionActivate)
        {
            if (record.Info.State == ExtensionStateActive ||
                record.Info.State == ExtensionStateActivating)
            {
                LeaveCriticalSection(&Lock);
                return TRUE;
            }
            if (record.Info.State == ExtensionStateDisabled &&
                (record.Info.Descriptor.Flags & ExtensionFlagDisabled) != 0)
            {
                LeaveCriticalSection(&Lock);
                return TRUE;
            }
            if (record.Info.State == ExtensionStateWaitingForRuntime &&
                (record.Info.Descriptor.Flags &
                 (ExtensionFlagRuntimeUnavailable |
                  ExtensionFlagRuntimeExecutableUnavailable)) != 0)
            {
                LeaveCriticalSection(&Lock);
                return TRUE;
            }
            if (record.Info.State == ExtensionStateWaitingForDependency &&
                (record.Info.Descriptor.Flags &
                 ExtensionFlagDependencyUnavailable) != 0)
            {
                LeaveCriticalSection(&Lock);
                return TRUE;
            }
            record.Info.State = ExtensionStateActivating;
        }
        else
        {
            if (record.Info.State == ExtensionStateInactive ||
                record.Info.State == ExtensionStateDiscovered ||
                record.Info.State == ExtensionStateWaitingForRuntime ||
                record.Info.State == ExtensionStateWaitingForDependency ||
                record.Info.State == ExtensionStateDisabled)
            {
                LeaveCriticalSection(&Lock);
                return TRUE;
            }
            record.Info.State = ExtensionStateDeactivating;
        }
        callback = record.Callback;
        context = record.Context;
        CopyInfo(record, &info);
        LeaveCriticalSection(&Lock);

        BOOL succeeded = callback == NULL
                             ? TRUE
                             : callback(context, action, &info);

        EnterCriticalSection(&Lock);
        index = FindIndex(extensionId);
        if (index >= 0)
        {
                Records[index].Info.State = succeeded
                                            ? (action == ExtensionActionActivate
                                                   ? ExtensionStateActive
                                                   : ExtensionStateInactive)
                                            : ((action == ExtensionActionActivate &&
                                                (Records[index].Info.Descriptor.Flags &
                                                 (ExtensionFlagRuntimeUnavailable |
                                                  ExtensionFlagRuntimeExecutableUnavailable)) != 0)
                                                   ? ExtensionStateWaitingForRuntime
                                                   : ((action == ExtensionActionActivate &&
                                                       (Records[index].Info.Descriptor.Flags &
                                                        ExtensionFlagDependencyUnavailable) != 0)
                                                          ? ExtensionStateWaitingForDependency
                                                          : ExtensionStateFailed));
        }
        LeaveCriticalSection(&Lock);
        return succeeded;
    }

public:
    ExtensionsService()
        : RecordCount(0),
          RefreshCallback(NULL),
          RefreshContext(NULL),
          ManagementCallback(NULL),
          ManagementContext(NULL)
    {
        InitializeCriticalSection(&Lock);
        InitializeConditionVariable(&LeaseChanged);
    }

    virtual ~ExtensionsService()
    {
        DeleteCriticalSection(&Lock);
    }

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_EXTENSIONS_VERSION_1_4;
    }

    virtual BOOL WINAPI SetRefreshCallback(
        ExtensionRefreshCallback callback,
        void* context)
    {
        EnterCriticalSection(&Lock);
        RefreshCallback = callback;
        RefreshContext = context;
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI Refresh()
    {
        ExtensionRefreshCallback callback = NULL;
        void* context = NULL;
        EnterCriticalSection(&Lock);
        callback = RefreshCallback;
        context = RefreshContext;
        LeaveCriticalSection(&Lock);
        return callback != NULL ? callback(context) : FALSE;
    }

    virtual BOOL WINAPI SetManagementCallback(
        ExtensionManagementCallback callback,
        void* context)
    {
        EnterCriticalSection(&Lock);
        ManagementCallback = callback;
        ManagementContext = context;
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    BOOL RequestManagement(
        ExtensionManagementAction action,
        const char* extensionId,
        const wchar_t* manifestPath,
        int moveDelta)
    {
        ExtensionManagementCallback callback = NULL;
        void* context = NULL;
        EnterCriticalSection(&Lock);
        callback = ManagementCallback;
        context = ManagementContext;
        LeaveCriticalSection(&Lock);
        return callback != NULL
                   ? callback(context, action, extensionId, manifestPath,
                              moveDelta)
                   : FALSE;
    }

    virtual BOOL WINAPI InstallExtensionManifest(const wchar_t* manifestPath)
    {
        if (manifestPath == NULL || manifestPath[0] == 0)
            return FALSE;
        return RequestManagement(
            ExtensionManagementInstallManifest, NULL, manifestPath, 0);
    }

    virtual BOOL WINAPI RemoveManagedExtension(const char* extensionId)
    {
        if (!IsValidId(extensionId))
            return FALSE;
        return RequestManagement(
            ExtensionManagementRemove, extensionId, NULL, 0);
    }

    virtual BOOL WINAPI MoveManagedExtension(
        const char* extensionId,
        int delta)
    {
        if (!IsValidId(extensionId) || (delta != -1 && delta != 1))
            return FALSE;
        return RequestManagement(
            ExtensionManagementMove, extensionId, NULL, delta);
    }

    virtual BOOL WINAPI ApplyExtensionOrder(
        const char* const* extensionIds,
        int extensionCount)
    {
        if (extensionCount < 0 ||
            (extensionCount > 0 && extensionIds == NULL))
            return FALSE;

        EnterCriticalSection(&Lock);
        int target = 0;
        for (int orderIndex = 0;
             orderIndex < extensionCount && target < RecordCount;
             ++orderIndex)
        {
            if (!IsValidId(extensionIds[orderIndex]))
            {
                LeaveCriticalSection(&Lock);
                return FALSE;
            }
            int current = FindIndex(extensionIds[orderIndex]);
            if (current < target)
                continue;
            if (current > target)
            {
                Record moved = Records[current];
                for (int index = current; index > target; --index)
                    Records[index] = Records[index - 1];
                Records[target] = moved;
            }
            ++target;
        }
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    const char* GetApiSchema() const
    {
        return "{\"methods\":[\"register\",\"unregister\",\"activate\",\"deactivate\",\"setDisabled\",\"setSettingsSchema\",\"getSettingsSchema\",\"list\",\"find\",\"installManifest\",\"removeManaged\",\"moveManaged\"],\"states\":[\"discovered\",\"activating\",\"active\",\"deactivating\",\"inactive\",\"failed\",\"waitingForRuntime\",\"waitingForDependency\",\"disabled\"]}";
    }

    virtual BOOL WINAPI RegisterExtension(
        const ExtensionDescriptor* descriptor,
        ExtensionLifecycleCallback callback,
        void* context)
    {
        if (descriptor == NULL ||
            descriptor->StructSize < sizeof(ExtensionDescriptor) ||
            !IsValidId(descriptor->Id))
        {
            return FALSE;
        }

        EnterCriticalSection(&Lock);
        int existing = FindIndex(descriptor->Id);
        if (existing >= 0)
        {
            if (Records[existing].Context != context)
            {
                LeaveCriticalSection(&Lock);
                return FALSE;
            }
            Records[existing].Info.Descriptor = *descriptor;
            Records[existing].Callback = callback;
            if ((descriptor->Flags & ExtensionFlagDisabled) != 0)
            {
                if (Records[existing].Info.State != ExtensionStateActive &&
                    Records[existing].Info.State != ExtensionStateActivating)
                    Records[existing].Info.State = ExtensionStateDisabled;
            }
            else if ((descriptor->Flags &
                      (ExtensionFlagRuntimeUnavailable |
                       ExtensionFlagRuntimeExecutableUnavailable)) != 0)
            {
                if (Records[existing].Info.State != ExtensionStateActive &&
                    Records[existing].Info.State != ExtensionStateActivating)
                    Records[existing].Info.State = ExtensionStateWaitingForRuntime;
            }
            else if ((descriptor->Flags & ExtensionFlagDependencyUnavailable) != 0)
            {
                if (Records[existing].Info.State != ExtensionStateActive &&
                    Records[existing].Info.State != ExtensionStateActivating)
                    Records[existing].Info.State = ExtensionStateWaitingForDependency;
            }
            else if (Records[existing].Info.State == ExtensionStateDisabled ||
                     Records[existing].Info.State == ExtensionStateWaitingForRuntime)
            {
                Records[existing].Info.State = ExtensionStateDiscovered;
            }
            else if (Records[existing].Info.State == ExtensionStateWaitingForDependency)
            {
                Records[existing].Info.State = ExtensionStateDiscovered;
            }
            LeaveCriticalSection(&Lock);
            return TRUE;
        }
        if (RecordCount >= MaxExtensions)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        Record& record = Records[RecordCount++];
        record.Info = ExtensionInfo();
        record.Info.Descriptor = *descriptor;
        if ((descriptor->Flags & ExtensionFlagDisabled) != 0)
            record.Info.State = ExtensionStateDisabled;
        else if ((descriptor->Flags &
                  (ExtensionFlagRuntimeUnavailable |
                   ExtensionFlagRuntimeExecutableUnavailable)) != 0)
            record.Info.State = ExtensionStateWaitingForRuntime;
        else if ((descriptor->Flags & ExtensionFlagDependencyUnavailable) != 0)
            record.Info.State = ExtensionStateWaitingForDependency;
        record.Callback = callback;
        record.Context = context;
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI UnregisterExtension(
        const char* extensionId,
        void* context)
    {
        EnterCriticalSection(&Lock);
        int index = FindIndex(extensionId);
        if (index < 0 ||
            (context != NULL && Records[index].Context != context))
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        LeaveCriticalSection(&Lock);

        // A registered extension may own a persistent worker, event
        // subscriptions, or native UI handles. Give it the same lifecycle
        // boundary as an explicit DeactivateExtension before deleting its
        // record; otherwise its callback context could become dangling.
        if (!RunAction(extensionId, ExtensionActionDeactivate))
            return FALSE;

        // The lifecycle callback stops persistent workers, but a host call may
        // already be inside RuntimeHostDispatch. Wait for those borrowed
        // leases before deleting the record and its owner pointer.
        EnterCriticalSection(&Lock);
        for (;;)
        {
            index = FindIndex(extensionId);
            if (index < 0 ||
                (context != NULL && Records[index].Context != context))
            {
                LeaveCriticalSection(&Lock);
                return FALSE;
            }
            if (Records[index].ActiveLeases == 0)
                break;
            SleepConditionVariableCS(&LeaseChanged, &Lock, INFINITE);
        }

        index = FindIndex(extensionId);
        if (index < 0 ||
            (context != NULL && Records[index].Context != context))
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        for (int move = index; move + 1 < RecordCount; ++move)
            Records[move] = Records[move + 1];
        Records[--RecordCount] = Record();
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual int WINAPI UnregisterOwner(void* context)
    {
        if (context == NULL)
            return 0;
        int removed = 0;
        for (;;)
        {
            char extensionId[sizeof(Records[0].Info.Descriptor.Id)];
            extensionId[0] = '\0';
            EnterCriticalSection(&Lock);
            for (int index = RecordCount - 1; index >= 0; --index)
            {
                if (Records[index].Context == context)
                {
                    strncpy_s(
                        extensionId,
                        _countof(extensionId),
                        Records[index].Info.Descriptor.Id,
                        _TRUNCATE);
                    break;
                }
            }
            LeaveCriticalSection(&Lock);
            if (extensionId[0] == '\0')
                break;

            // UnregisterExtension performs the deactivation outside the
            // registry lock and verifies the owner again before removal.
            if (UnregisterExtension(extensionId, context))
                ++removed;
            else
                break;
        }
        return removed;
    }

    virtual BOOL WINAPI ActivateExtension(const char* extensionId)
    {
        return RunAction(extensionId, ExtensionActionActivate);
    }

    virtual BOOL WINAPI DeactivateExtension(const char* extensionId)
    {
        return RunAction(extensionId, ExtensionActionDeactivate);
    }

    virtual int WINAPI GetExtensionCount() const
    {
        EnterCriticalSection(&Lock);
        int count = RecordCount;
        LeaveCriticalSection(&Lock);
        return count;
    }

    virtual BOOL WINAPI GetExtensionInfo(
        int index,
        ExtensionInfo* info) const
    {
        if (info == NULL || info->StructSize < sizeof(ExtensionInfo))
            return FALSE;
        EnterCriticalSection(&Lock);
        if (index < 0 || index >= RecordCount)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        CopyInfo(Records[index], info);
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI FindExtension(
        const char* extensionId,
        ExtensionInfo* info) const
    {
        if (info == NULL || info->StructSize < sizeof(ExtensionInfo))
            return FALSE;
        EnterCriticalSection(&Lock);
        int index = FindIndex(extensionId);
        if (index < 0)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        CopyInfo(Records[index], info);
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI SetExtensionDisabled(
        const char* extensionId,
        BOOL disabled)
    {
        if (extensionId == NULL || extensionId[0] == 0)
            return FALSE;
        EnterCriticalSection(&Lock);
        const int index = FindIndex(extensionId);
        if (index < 0 ||
            Records[index].Info.State == ExtensionStateActivating ||
            Records[index].Info.State == ExtensionStateDeactivating)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        Record& record = Records[index];
        if (disabled)
        {
            if (record.Info.State == ExtensionStateActive)
            {
                LeaveCriticalSection(&Lock);
                return FALSE;
            }
            record.Info.Descriptor.Flags |= ExtensionFlagDisabled;
            record.Info.State = ExtensionStateDisabled;
        }
        else
        {
            record.Info.Descriptor.Flags &= ~ExtensionFlagDisabled;
            if (record.Info.State == ExtensionStateDisabled)
                record.Info.State = ExtensionStateDiscovered;
        }
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI SetExtensionSettingsSchema(
        const char* extensionId,
        const ExtensionSettingInfo* settings,
        int settingCount)
    {
        if (extensionId == NULL || settingCount < 0 ||
            settingCount > Record::MaxSettings ||
            (settingCount != 0 && settings == NULL))
        {
            return FALSE;
        }
        EnterCriticalSection(&Lock);
        const int index = FindIndex(extensionId);
        if (index < 0)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        Record& record = Records[index];
        ExtensionSettingInfo normalized[Record::MaxSettings];
        for (int settingIndex = 0; settingIndex < settingCount; ++settingIndex)
        {
            const ExtensionSettingInfo& setting = settings[settingIndex];
            if (setting.StructSize < ExtensionSettingInfoLegacySize ||
                !IsValidId(setting.Key) ||
                (setting.Type != ExtensionSettingString &&
                 setting.Type != ExtensionSettingInteger &&
                 setting.Type != ExtensionSettingBoolean))
            {
                LeaveCriticalSection(&Lock);
                return FALSE;
            }
            for (int existing = 0; existing < settingIndex; ++existing)
            {
                if (_stricmp(settings[existing].Key, setting.Key) == 0)
                {
                    LeaveCriticalSection(&Lock);
                    return FALSE;
                }
            }
            normalized[settingIndex] = ExtensionSettingInfo();
            DWORD copySize = setting.StructSize;
            if (copySize > sizeof(ExtensionSettingInfo))
                copySize = sizeof(ExtensionSettingInfo);
            memcpy(&normalized[settingIndex], &setting, copySize);
            normalized[settingIndex].StructSize = sizeof(ExtensionSettingInfo);
        }
        for (int settingIndex = 0; settingIndex < settingCount; ++settingIndex)
            record.Settings[settingIndex] = normalized[settingIndex];
        for (int settingIndex = settingCount;
             settingIndex < record.SettingCount;
             ++settingIndex)
        {
            record.Settings[settingIndex] = ExtensionSettingInfo();
        }
        record.SettingCount = settingCount;
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual int WINAPI GetExtensionSettingCount(const char* extensionId) const
    {
        EnterCriticalSection(&Lock);
        const int index = FindIndex(extensionId);
        const int count = index >= 0 ? Records[index].SettingCount : 0;
        LeaveCriticalSection(&Lock);
        return count;
    }

    virtual BOOL WINAPI GetExtensionSettingInfo(
        const char* extensionId,
        int index,
        ExtensionSettingInfo* setting) const
    {
        if (setting == NULL || setting->StructSize < ExtensionSettingInfoLegacySize)
            return FALSE;
        EnterCriticalSection(&Lock);
        const int recordIndex = FindIndex(extensionId);
        if (recordIndex < 0 || index < 0 ||
            index >= Records[recordIndex].SettingCount)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        const DWORD requestedSize = setting->StructSize;
        const ExtensionSettingInfo& stored = Records[recordIndex].Settings[index];
        const DWORD copySize = requestedSize < sizeof(ExtensionSettingInfo)
                                    ? requestedSize
                                    : sizeof(ExtensionSettingInfo);
        memcpy(setting, &stored, copySize);
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual BOOL WINAPI AcquireExtension(
        const char* extensionId,
        void* owner)
    {
        EnterCriticalSection(&Lock);
        int index = FindIndex(extensionId);
        if (index < 0 ||
            (owner != NULL && Records[index].Context != owner) ||
            (Records[index].Info.State != ExtensionStateActive &&
             Records[index].Info.State != ExtensionStateActivating))
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        ++Records[index].ActiveLeases;
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    virtual void WINAPI ReleaseExtension(
        const char* extensionId,
        void* owner)
    {
        EnterCriticalSection(&Lock);
        int index = FindIndex(extensionId);
        if (index >= 0 &&
            (owner == NULL || Records[index].Context == owner) &&
            Records[index].ActiveLeases > 0)
        {
            --Records[index].ActiveLeases;
            if (Records[index].ActiveLeases == 0)
                WakeAllConditionVariable(&LeaseChanged);
        }
        LeaveCriticalSection(&Lock);
    }
};

} // namespace Extensions
} // namespace Salamatrix
