// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../precomp.h"
#include "../salamatrix_extensions.h"

namespace
{
struct CallbackState
{
    int Count;
    Salamatrix::Extensions::ExtensionAction LastAction;
    Salamatrix::Extensions::ExtensionState CallbackStateValue;

    CallbackState()
        : Count(0),
          LastAction(Salamatrix::Extensions::ExtensionActionActivate),
          CallbackStateValue(Salamatrix::Extensions::ExtensionStateDiscovered)
    {
    }
};

struct ManagementState
{
    int Count;
    Salamatrix::Extensions::ExtensionManagementAction Action;
    std::string ExtensionId;
    std::wstring ManifestPath;
    int MoveDelta;

    ManagementState()
        : Count(0),
          Action(Salamatrix::Extensions::ExtensionManagementInstallManifest),
          MoveDelta(0)
    {
    }
};

BOOL WINAPI LifecycleCallback(
    void* context,
    Salamatrix::Extensions::ExtensionAction action,
    const Salamatrix::Extensions::ExtensionInfo* info)
{
    CallbackState* state = static_cast<CallbackState*>(context);
    if (state == NULL || info == NULL)
        return FALSE;
    ++state->Count;
    state->LastAction = action;
    state->CallbackStateValue = info->State;
    return TRUE;
}

BOOL WINAPI ManagementCallback(
    void* context,
    Salamatrix::Extensions::ExtensionManagementAction action,
    const char* extensionId,
    const wchar_t* manifestPath,
    int moveDelta)
{
    ManagementState* state = static_cast<ManagementState*>(context);
    if (state == NULL)
        return FALSE;
    ++state->Count;
    state->Action = action;
    state->ExtensionId = extensionId != NULL ? extensionId : "";
    state->ManifestPath = manifestPath != NULL ? manifestPath : L"";
    state->MoveDelta = moveDelta;
    return TRUE;
}

int Failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}

Salamatrix::Extensions::ExtensionDescriptor MakeDescriptor(const char* id)
{
    Salamatrix::Extensions::ExtensionDescriptor descriptor;
    strncpy_s(descriptor.Id, _countof(descriptor.Id), id, _TRUNCATE);
    strncpy_s(descriptor.Name, _countof(descriptor.Name), "Test extension", _TRUNCATE);
    strncpy_s(descriptor.RuntimeId, _countof(descriptor.RuntimeId), "test.runtime", _TRUNCATE);
    strncpy_s(descriptor.EntryPoint, _countof(descriptor.EntryPoint), "test.py", _TRUNCATE);
    descriptor.Flags = Salamatrix::Extensions::ExtensionFlagManifest |
                       Salamatrix::Extensions::ExtensionFlagPackage;
    return descriptor;
}

void TestRegistrationAndLifecycle()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    void* owner = &callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("test.extension");

    Check(extensions->RegisterExtension(&descriptor, LifecycleCallback, owner) != FALSE,
          "register extension");
    Check(extensions->GetExtensionCount() == 1, "count registered extension");

    Salamatrix::Extensions::ExtensionInfo info;
    Check(extensions->FindExtension("TEST.EXTENSION", &info) != FALSE,
          "find extension case-insensitively");
    Check(info.State == Salamatrix::Extensions::ExtensionStateDiscovered,
          "initial discovered state");

    Check(extensions->ActivateExtension("test.extension") != FALSE,
          "activate extension");
    Check(callback.Count == 1 &&
              callback.LastAction == Salamatrix::Extensions::ExtensionActionActivate &&
              callback.CallbackStateValue == Salamatrix::Extensions::ExtensionStateActivating,
          "activation callback and state");
    Check(extensions->FindExtension("test.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateActive,
          "active state");

    Check(extensions->DeactivateExtension("test.extension") != FALSE,
          "deactivate extension");
    Check(callback.Count == 2 &&
              callback.LastAction == Salamatrix::Extensions::ExtensionActionDeactivate &&
              callback.CallbackStateValue == Salamatrix::Extensions::ExtensionStateDeactivating,
          "deactivation callback and state");
    Check(extensions->FindExtension("test.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateInactive,
          "inactive state");

    Check(extensions->RegisterExtension(&descriptor, NULL, owner) != FALSE,
          "same owner can refresh descriptor");
    Check(extensions->RegisterExtension(&descriptor, NULL, extensions) == FALSE,
          "different owner cannot replace descriptor");
    Check(extensions->UnregisterOwner(owner) == 1, "unregister owner");
    Check(extensions->GetExtensionCount() == 0, "empty after unregister owner");
    delete extensions;
}

void TestValidation()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor = MakeDescriptor("bad id");
    Check(extensions->RegisterExtension(&descriptor, LifecycleCallback, &callback) == FALSE,
          "reject invalid id characters");
    descriptor = MakeDescriptor("valid.extension");
    descriptor.StructSize = sizeof(descriptor) - 1;
    Check(extensions->RegisterExtension(&descriptor, NULL, &callback) == FALSE,
          "reject undersized descriptor");
    descriptor.StructSize = sizeof(descriptor);
    Check(extensions->RegisterExtension(&descriptor, NULL, &callback) != FALSE,
          "accept valid descriptor");
    Salamatrix::Extensions::ExtensionInfo info;
    info.StructSize = sizeof(info) - 1;
    Check(extensions->FindExtension("valid.extension", &info) == FALSE,
          "reject undersized info");
    delete extensions;
}

void TestUnregisterDeactivatesActiveExtension()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("active.extension");
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "register active extension");
    Check(extensions->ActivateExtension("active.extension") != FALSE,
          "activate extension before unregister");
    Check(extensions->AcquireExtension("active.extension", &callback) != FALSE,
          "acquire active extension unload lease");
    Check(extensions->AcquireExtension("active.extension", extensions) == FALSE,
          "reject unload lease from a different owner");
    extensions->ReleaseExtension("active.extension", &callback);
    Check(extensions->UnregisterExtension(
              "active.extension", &callback) != FALSE,
          "unregister active extension");
    Check(callback.Count == 2 &&
              callback.LastAction ==
                  Salamatrix::Extensions::ExtensionActionDeactivate,
          "unregister deactivates active extension");
    Check(extensions->GetExtensionCount() == 0,
          "active extension removed after deactivation");
    delete extensions;
}

void TestRuntimeAvailabilityState()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("waiting.extension");
    descriptor.Flags |= Salamatrix::Extensions::ExtensionFlagRuntimeUnavailable;
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "register extension with missing runtime");
    Salamatrix::Extensions::ExtensionInfo info;
    Check(extensions->FindExtension("waiting.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateWaitingForRuntime,
          "missing runtime is exposed as waiting state");
    Check(extensions->ActivateExtension("waiting.extension") != FALSE,
          "waiting extension activation is a safe no-op");
    Check(callback.Count == 0, "waiting extension does not start without runtime");
    descriptor.Flags &= ~Salamatrix::Extensions::ExtensionFlagRuntimeUnavailable;
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "refresh extension after runtime becomes available");
    Check(extensions->FindExtension("waiting.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateDiscovered,
          "runtime availability refresh returns extension to discovered state");
    Check(extensions->ActivateExtension("waiting.extension") != FALSE &&
              callback.Count == 1,
          "available runtime activates extension");
    delete extensions;
}

void TestDependencyAvailabilityState()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("dependency.waiting");
    descriptor.Flags |=
        Salamatrix::Extensions::ExtensionFlagDependencyUnavailable;
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "register extension with missing dependency");
    Salamatrix::Extensions::ExtensionInfo info;
    Check(extensions->FindExtension("dependency.waiting", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateWaitingForDependency,
          "missing dependency is exposed as waiting state");
    Check(extensions->ActivateExtension("dependency.waiting") != FALSE,
          "dependency-waiting activation is a safe no-op");
    Check(callback.Count == 0, "dependency-waiting extension does not start");
    descriptor.Flags &=
        ~Salamatrix::Extensions::ExtensionFlagDependencyUnavailable;
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "refresh extension after dependency becomes available");
    Check(extensions->FindExtension("dependency.waiting", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateDiscovered,
          "dependency availability refresh returns extension to discovered state");
    Check(extensions->ActivateExtension("dependency.waiting") != FALSE &&
              callback.Count == 1,
          "available dependency activates extension");
    delete extensions;
}

void TestDisabledState()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("disabled.extension");
    descriptor.Flags |= Salamatrix::Extensions::ExtensionFlagDisabled;
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "register disabled extension");
    Salamatrix::Extensions::ExtensionInfo info;
    Check(extensions->FindExtension("disabled.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateDisabled,
          "disabled extension state is exposed");
    Check(extensions->ActivateExtension("disabled.extension") != FALSE,
          "disabled extension activation is a safe no-op");
    Check(callback.Count == 0, "disabled extension does not start");
    Check(extensions->SetExtensionDisabled("disabled.extension", FALSE) != FALSE,
          "clear disabled state");
    Check(extensions->FindExtension("disabled.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateDiscovered,
          "cleared disabled state returns extension to discovered");
    Check(extensions->ActivateExtension("disabled.extension") != FALSE &&
              callback.Count == 1,
          "re-enabled extension activates");
    Check(extensions->DeactivateExtension("disabled.extension") != FALSE,
          "active extension deactivates before disabling");
    Check(extensions->SetExtensionDisabled("disabled.extension", TRUE) != FALSE,
          "set disabled state after deactivation");
    Check(extensions->FindExtension("disabled.extension", &info) != FALSE &&
              info.State == Salamatrix::Extensions::ExtensionStateDisabled,
          "disabled state persists in registry");
    delete extensions;
}

void TestSettingsSchema()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState callback;
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("settings.extension");
    Check(extensions->RegisterExtension(
              &descriptor, LifecycleCallback, &callback) != FALSE,
          "register extension for settings schema");
    Salamatrix::Extensions::ExtensionSettingInfo settings[2];
    strcpy_s(settings[0].Key, _countof(settings[0].Key), "repositoryUrl");
    settings[0].Type = Salamatrix::Extensions::ExtensionSettingString;
    strcpy_s(settings[1].Key, _countof(settings[1].Key), "autoRefresh");
    settings[1].Type = Salamatrix::Extensions::ExtensionSettingBoolean;
    Check(extensions->SetExtensionSettingsSchema(
              "settings.extension", settings, _countof(settings)) != FALSE,
          "publish extension settings schema");
    Check(extensions->GetExtensionSettingCount("settings.extension") == 2,
          "published settings schema count");
    Salamatrix::Extensions::ExtensionSettingInfo returned;
    Check(extensions->GetExtensionSettingInfo(
              "settings.extension", 1, &returned) != FALSE &&
              strcmp(returned.Key, "autoRefresh") == 0 &&
              returned.Type == Salamatrix::Extensions::ExtensionSettingBoolean,
          "published settings schema item");
    strcpy_s(settings[1].Key, _countof(settings[1].Key), "repositoryUrl");
    Check(extensions->SetExtensionSettingsSchema(
              "settings.extension", settings, _countof(settings)) == FALSE,
          "reject duplicate settings schema keys");

    // Simulate a 1.2 provider compiled before the appended presentation
    // fields existed. The registry must accept and return the legacy prefix.
    struct LegacySettingInfo
    {
        DWORD StructSize;
        char Key[128];
        Salamatrix::Extensions::ExtensionSettingType Type;
    };
    LegacySettingInfo legacy;
    legacy.StructSize = sizeof(LegacySettingInfo);
    strcpy_s(legacy.Key, _countof(legacy.Key), "legacySetting");
    legacy.Type = Salamatrix::Extensions::ExtensionSettingString;
    Check(extensions->SetExtensionSettingsSchema(
              "settings.extension",
              reinterpret_cast<const Salamatrix::Extensions::ExtensionSettingInfo*>(&legacy),
              1) != FALSE,
          "accept legacy setting schema prefix");
    LegacySettingInfo returnedLegacy;
    returnedLegacy.StructSize = sizeof(returnedLegacy);
    Check(extensions->GetExtensionSettingInfo(
              "settings.extension", 0,
              reinterpret_cast<Salamatrix::Extensions::ExtensionSettingInfo*>(&returnedLegacy)) != FALSE &&
              strcmp(returnedLegacy.Key, "legacySetting") == 0,
          "return legacy setting schema prefix");
    delete extensions;
}

void TestManagementAndOrdering()
{
    Salamatrix::Extensions::ExtensionsService* extensions =
        new Salamatrix::Extensions::ExtensionsService();
    CallbackState owners[3];
    Salamatrix::Extensions::ExtensionDescriptor descriptor =
        MakeDescriptor("first.extension");
    Check(extensions->RegisterExtension(
              &descriptor, NULL, &owners[0]) != FALSE,
          "register first ordered extension");
    descriptor = MakeDescriptor("second.extension");
    Check(extensions->RegisterExtension(
              &descriptor, NULL, &owners[1]) != FALSE,
          "register second ordered extension");
    descriptor = MakeDescriptor("third.extension");
    Check(extensions->RegisterExtension(
              &descriptor, NULL, &owners[2]) != FALSE,
          "register third ordered extension");

    const char* order[] = {"third.extension", "first.extension"};
    Check(extensions->ApplyExtensionOrder(order, _countof(order)) != FALSE,
          "apply stable extension order");
    Salamatrix::Extensions::ExtensionInfo info;
    Check(extensions->GetExtensionInfo(0, &info) != FALSE &&
              strcmp(info.Descriptor.Id, "third.extension") == 0,
          "ordered extension moved to the first row");
    Check(extensions->GetExtensionInfo(1, &info) != FALSE &&
              strcmp(info.Descriptor.Id, "first.extension") == 0,
          "second ordered extension follows it");
    Check(extensions->GetExtensionInfo(2, &info) != FALSE &&
              strcmp(info.Descriptor.Id, "second.extension") == 0,
          "unlisted extension retains a stable trailing position");

    ManagementState management;
    Check(extensions->SetManagementCallback(
              ManagementCallback, &management) != FALSE,
          "register extension management callback");
    Check(extensions->InstallExtensionManifest(
              L"C:\\custom\\extension.json") != FALSE &&
              management.Action ==
                  Salamatrix::Extensions::ExtensionManagementInstallManifest &&
              management.ManifestPath == L"C:\\custom\\extension.json",
          "forward custom manifest installation");
    Check(extensions->RemoveManagedExtension("first.extension") != FALSE &&
              management.Action ==
                  Salamatrix::Extensions::ExtensionManagementRemove &&
              management.ExtensionId == "first.extension",
          "forward managed extension removal");
    Check(extensions->MoveManagedExtension("third.extension", 1) != FALSE &&
              management.Action ==
                  Salamatrix::Extensions::ExtensionManagementMove &&
              management.ExtensionId == "third.extension" &&
              management.MoveDelta == 1,
          "forward managed extension move");
    Check(extensions->MoveManagedExtension("third.extension", 2) == FALSE,
          "reject invalid managed extension move delta");
    delete extensions;
}
} // namespace

int main()
{
    TestRegistrationAndLifecycle();
    TestValidation();
    TestUnregisterDeactivatesActiveExtension();
    TestRuntimeAvailabilityState();
    TestDependencyAvailabilityState();
    TestDisabledState();
    TestSettingsSchema();
    TestManagementAndOrdering();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d Salamatrix extension test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All Salamatrix extension tests passed.\n");
    return 0;
}
