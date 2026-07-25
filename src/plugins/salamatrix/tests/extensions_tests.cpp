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
    descriptor.Flags = Salamatrix::Extensions::ExtensionFlagManifest;
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
} // namespace

int main()
{
    TestRegistrationAndLifecycle();
    TestValidation();
    TestUnregisterDeactivatesActiveExtension();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d Salamatrix extension test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All Salamatrix extension tests passed.\n");
    return 0;
}
