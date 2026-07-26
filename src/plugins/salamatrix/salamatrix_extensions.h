// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_extensions.h
    Extension descriptor and lifecycle registry.
*/

#pragma once

namespace Salamatrix
{
namespace Extensions
{

#define SALAMATRIX_SERVICE_EXTENSIONS "Salamatrix.Extensions"
#define SALAMATRIX_EXTENSIONS_VERSION_1_0 0x00010000
#define SALAMATRIX_EXTENSIONS_VERSION_1_1 0x00010001

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
    ExtensionFlagDisabled = 0x00000020
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

typedef BOOL(WINAPI* ExtensionLifecycleCallback)(
    void* context,
    ExtensionAction action,
    const ExtensionInfo* info);

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
        ExtensionInfo Info;
        ExtensionLifecycleCallback Callback;
        void* Context;
        LONG ActiveLeases;

        Record()
            : Callback(NULL),
              Context(NULL),
              ActiveLeases(0)
        {
        }
    };

    Record Records[MaxExtensions];
    int RecordCount;
    mutable CRITICAL_SECTION Lock;
    CONDITION_VARIABLE LeaseChanged;

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
                 ExtensionFlagRuntimeUnavailable) != 0)
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
                                                 ExtensionFlagRuntimeUnavailable) != 0)
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
        : RecordCount(0)
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
        return SALAMATRIX_EXTENSIONS_VERSION_1_1;
    }

    const char* GetApiSchema() const
    {
        return "{\"methods\":[\"register\",\"unregister\",\"activate\",\"deactivate\",\"setDisabled\",\"list\",\"find\"],\"states\":[\"discovered\",\"activating\",\"active\",\"deactivating\",\"inactive\",\"failed\",\"waitingForRuntime\",\"waitingForDependency\",\"disabled\"]}";
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
            else if ((descriptor->Flags & ExtensionFlagRuntimeUnavailable) != 0)
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
        else if ((descriptor->Flags & ExtensionFlagRuntimeUnavailable) != 0)
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
