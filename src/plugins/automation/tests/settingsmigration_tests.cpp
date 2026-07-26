// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "../salamatrixsettings.h"

static int g_failures = 0;
#define CHECK(x) do { if (!(x)) { ++g_failures; fprintf(stderr, "FAIL: %s\\n", #x); } } while (0)

static CExtensionManifestSettingMigration Rename(
    unsigned int from, unsigned int to, const char* source, const char* destination)
{
    CExtensionManifestSettingMigration migration;
    migration.FromVersion = from;
    migration.ToVersion = to;
    CExtensionManifestSettingMigrationOperation operation;
    operation.FromKey = source;
    operation.ToKey = destination;
    migration.Operations.push_back(operation);
    return migration;
}

static CExtensionManifestSettingMigration Remove(
    unsigned int from, unsigned int to, const char* source)
{
    CExtensionManifestSettingMigration migration;
    migration.FromVersion = from;
    migration.ToVersion = to;
    CExtensionManifestSettingMigrationOperation operation;
    operation.FromKey = source;
    operation.Remove = true;
    migration.Operations.push_back(operation);
    return migration;
}

class FailingStorage : public Salamatrix::Storage::IStorageService
{
public:
    Salamatrix::Storage::StorageService Inner;
    bool FailSet;
    bool FailDelete;

    FailingStorage() : FailSet(false), FailDelete(false) {}
    virtual DWORD WINAPI GetVersion() const { return Inner.GetVersion(); }
    virtual Salamatrix::Storage::StorageValueType WINAPI GetValueType(const char* e, const char* k) const { return Inner.GetValueType(e, k); }
    virtual BOOL WINAPI GetString(const char* e, const char* k, char* b, int n, int* r) const { return Inner.GetString(e, k, b, n, r); }
    virtual BOOL WINAPI GetInteger(const char* e, const char* k, LONGLONG* v) const { return Inner.GetInteger(e, k, v); }
    virtual BOOL WINAPI GetBoolean(const char* e, const char* k, BOOL* v) const { return Inner.GetBoolean(e, k, v); }
    virtual BOOL WINAPI SetString(const char* e, const char* k, const char* v) { return FailSet ? FALSE : Inner.SetString(e, k, v); }
    virtual BOOL WINAPI SetInteger(const char* e, const char* k, LONGLONG v) { return FailSet ? FALSE : Inner.SetInteger(e, k, v); }
    virtual BOOL WINAPI SetBoolean(const char* e, const char* k, BOOL v) { return FailSet ? FALSE : Inner.SetBoolean(e, k, v); }
    virtual BOOL WINAPI DeleteValue(const char* e, const char* k) { return FailDelete ? FALSE : Inner.DeleteValue(e, k); }
    virtual BOOL WINAPI ClearExtension(const char* e) { return Inner.ClearExtension(e); }
};

static void TestTypedAndDestinationPreservation()
{
    Salamatrix::Storage::StorageService storage;
    storage.SetString("Test.Settings", "oldString", "text");
    storage.SetInteger("Test.Settings", "oldInteger", 42);
    storage.SetBoolean("Test.Settings", "oldBoolean", TRUE);
    storage.SetString("Test.Settings", "newString", "keep");
    std::vector<CExtensionManifestSettingMigration> migrations;
    migrations.push_back(Rename(0, 1, "oldString", "newString"));
    migrations.push_back(Rename(1, 2, "oldInteger", "newInteger"));
    migrations.push_back(Rename(2, 3, "oldBoolean", "newBoolean"));
    CHECK(ApplySalamatrixSettingsMigrations(&storage, "Test.Settings", 3, migrations));
    CHECK(storage.GetValueType("Test.Settings", "oldString") == Salamatrix::Storage::StorageValueMissing);
    CHECK(storage.GetValueType("Test.Settings", "oldInteger") == Salamatrix::Storage::StorageValueMissing);
    CHECK(storage.GetValueType("Test.Settings", "oldBoolean") == Salamatrix::Storage::StorageValueMissing);
    char value[32]; int required = 0;
    CHECK(storage.GetString("Test.Settings", "newString", value, sizeof(value), &required));
    CHECK(strcmp(value, "keep") == 0);
    LONGLONG integer = 0; BOOL boolean = FALSE;
    CHECK(storage.GetInteger("Test.Settings", "newInteger", &integer) && integer == 42);
    CHECK(storage.GetBoolean("Test.Settings", "newBoolean", &boolean) && boolean == TRUE);
}

static void TestRetryAndValidation()
{
    FailingStorage storage;
    storage.Inner.SetString("Test.Retry", "old", "value");
    std::vector<CExtensionManifestSettingMigration> migrations;
    migrations.push_back(Rename(0, 1, "old", "new"));
    storage.FailSet = true;
    CHECK(!ApplySalamatrixSettingsMigrations(&storage, "Test.Retry", 1, migrations));
    storage.FailSet = false;
    CHECK(ApplySalamatrixSettingsMigrations(&storage, "Test.Retry", 1, migrations));
    storage.FailDelete = true;
    storage.SetString("Test.Retry", "old2", "value2");
    std::vector<CExtensionManifestSettingMigration> next;
    next.push_back(Rename(1, 2, "old2", "new2"));
    CHECK(!ApplySalamatrixSettingsMigrations(&storage, "Test.Retry", 2, next));
    storage.FailDelete = false;
    CHECK(ApplySalamatrixSettingsMigrations(&storage, "Test.Retry", 2, next));

    Salamatrix::Storage::StorageService high;
    high.SetInteger("Test.High", "salamatrix.settings.version", 9);
    CHECK(ApplySalamatrixSettingsMigrations(&high, "Test.High", 2, migrations));
    std::vector<CExtensionManifestSettingMigration> bad;
    bad.push_back(Remove(0, 1, "missing"));
    CHECK(ApplySalamatrixSettingsMigrations(&high, "Test.High", 9, bad));
    Salamatrix::Storage::StorageService missing;
    CHECK(!ApplySalamatrixSettingsMigrations(&missing, "Test.Missing", 2, bad));

    Salamatrix::Storage::StorageService stamp;
    CHECK(ApplySalamatrixSettingsMigrations(
        &stamp, "Test.Stamp", 2,
        std::vector<CExtensionManifestSettingMigration>()) == TRUE);
    LONGLONG stampedVersion = 0;
    CHECK(stamp.GetInteger(
              "Test.Stamp", "salamatrix.settings.version", &stampedVersion) &&
          stampedVersion == 2);
}

int main()
{
    TestTypedAndDestinationPreservation();
    TestRetryAndValidation();
    if (g_failures != 0)
        return 1;
    puts("All settings migration tests passed.");
    return 0;
}
