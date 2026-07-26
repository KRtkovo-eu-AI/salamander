// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "../salamatrix_storage.h"

namespace
{
    class FakeRegistry : public CSalamanderRegistryAbstract
    {
    public:
        std::vector<BYTE> Blob;
        int WriteCount;

        FakeRegistry()
            : WriteCount(0)
        {
        }

        virtual BOOL WINAPI ClearKey(HKEY key)
        {
            UNREFERENCED_PARAMETER(key);
            Blob.clear();
            return TRUE;
        }

        virtual BOOL WINAPI CreateKey(
            HKEY key,
            const char* name,
            HKEY& createdKey)
        {
            UNREFERENCED_PARAMETER(key);
            UNREFERENCED_PARAMETER(name);
            createdKey = NULL;
            return FALSE;
        }

        virtual BOOL WINAPI OpenKey(
            HKEY key,
            const char* name,
            HKEY& openedKey)
        {
            UNREFERENCED_PARAMETER(key);
            UNREFERENCED_PARAMETER(name);
            openedKey = NULL;
            return FALSE;
        }

        virtual void WINAPI CloseKey(HKEY key)
        {
            UNREFERENCED_PARAMETER(key);
        }

        virtual BOOL WINAPI DeleteKey(HKEY key, const char* name)
        {
            UNREFERENCED_PARAMETER(key);
            UNREFERENCED_PARAMETER(name);
            return FALSE;
        }

        virtual BOOL WINAPI GetValue(
            HKEY key,
            const char* name,
            DWORD type,
            void* buffer,
            DWORD bufferSize)
        {
            UNREFERENCED_PARAMETER(key);
            if (strcmp(name, "Storage") != 0 ||
                type != REG_BINARY ||
                Blob.empty() ||
                buffer == NULL ||
                bufferSize < Blob.size())
            {
                return FALSE;
            }
            memcpy(buffer, &Blob[0], Blob.size());
            return TRUE;
        }

        virtual BOOL WINAPI SetValue(
            HKEY key,
            const char* name,
            DWORD type,
            const void* data,
            DWORD dataSize)
        {
            UNREFERENCED_PARAMETER(key);
            if (strcmp(name, "Storage") != 0 ||
                type != REG_BINARY ||
                data == NULL ||
                dataSize == 0)
            {
                return FALSE;
            }
            const BYTE* bytes = static_cast<const BYTE*>(data);
            Blob.assign(bytes, bytes + dataSize);
            ++WriteCount;
            return TRUE;
        }

        virtual BOOL WINAPI DeleteValue(HKEY key, const char* name)
        {
            UNREFERENCED_PARAMETER(key);
            if (strcmp(name, "Storage") != 0)
                return FALSE;
            Blob.clear();
            ++WriteCount;
            return TRUE;
        }

        virtual BOOL WINAPI GetSize(
            HKEY key,
            const char* name,
            DWORD type,
            DWORD& bufferSize)
        {
            UNREFERENCED_PARAMETER(key);
            if (strcmp(name, "Storage") != 0 ||
                type != REG_BINARY ||
                Blob.empty())
            {
                bufferSize = 0;
                return FALSE;
            }
            bufferSize = static_cast<DWORD>(Blob.size());
            return TRUE;
        }
    };

    int Failures = 0;

    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::fprintf(stderr, "FAILED: %s\n", message);
            ++Failures;
        }
    }

    void TestTypesAndIsolation()
    {
        Salamatrix::Storage::StorageService storage;

        Check(
            storage.SetString(
                "Example.One", "same.key", u8"příliš žluťoučký") != FALSE,
            "set UTF-8 string");
        Check(
            storage.SetString(
                "Example.Two", "same.key", "independent") != FALSE,
            "set isolated string");
        Check(
            storage.SetInteger(
                "Example.One", "counter", 0x123456789LL) != FALSE,
            "set 64-bit integer");
        Check(
            storage.SetBoolean(
                "Example.One", "enabled", TRUE) != FALSE,
            "set boolean");

        char text[128];
        Check(
            storage.GetString(
                "example.one",
                "SAME.KEY",
                text,
                sizeof(text),
                NULL) != FALSE &&
                strcmp(text, u8"příliš žluťoučký") == 0,
            "case-insensitive read in first namespace");
        Check(
            storage.GetString(
                "Example.Two",
                "same.key",
                text,
                sizeof(text),
                NULL) != FALSE &&
                strcmp(text, "independent") == 0,
            "same key isolated in second namespace");

        LONGLONG integerValue = 0;
        BOOL booleanValue = FALSE;
        Check(
            storage.GetInteger(
                "Example.One", "counter", &integerValue) != FALSE &&
                integerValue == 0x123456789LL,
            "read 64-bit integer");
        Check(
            storage.GetBoolean(
                "Example.One", "enabled", &booleanValue) != FALSE &&
                booleanValue,
            "read boolean");
        Check(
            storage.GetString(
                "Example.One", "counter", text, sizeof(text), NULL) == FALSE,
            "typed read rejects mismatched type");
    }

    void TestValidationAndMutation()
    {
        Salamatrix::Storage::StorageService storage;
        std::vector<char> oversized(16386, 'x');
        oversized.back() = 0;

        Check(
            storage.SetString("bad/id", "key", "value") == FALSE,
            "reject invalid extension id");
        Check(
            storage.SetString("Good.Id", "bad/key", "value") == FALSE,
            "reject invalid key");
        Check(
            storage.SetString("Good.Id", "key", &oversized[0]) == FALSE,
            "reject oversized string");

        Check(
            storage.SetInteger("Good.Id", "one", 1) != FALSE &&
                storage.SetInteger("Good.Id", "two", 2) != FALSE,
            "prepare mutation values");
        Check(
            storage.DeleteValue("Good.Id", "one") != FALSE &&
                storage.GetValueType("Good.Id", "one") ==
                    Salamatrix::Storage::StorageValueMissing,
            "delete one value");
        Check(
            storage.ClearExtension("Good.Id") != FALSE &&
                storage.GetValueType("Good.Id", "two") ==
                    Salamatrix::Storage::StorageValueMissing,
            "clear extension namespace");
    }

    void TestKeyEnumeration()
    {
        Salamatrix::Storage::StorageService storage;
        Check(storage.SetString("Enumerate.Id", "zeta", "text") != FALSE,
              "enumeration string");
        Check(storage.SetInteger("Enumerate.Id", "number", 7) != FALSE,
              "enumeration integer");
        Check(storage.SetBoolean("Enumerate.Id", "flag", TRUE) != FALSE,
              "enumeration boolean");
        Check(storage.GetKeyCount("Enumerate.Id") == 3,
              "enumeration count");
        char key[32];
        int required = 0;
        Salamatrix::Storage::StorageValueType type =
            Salamatrix::Storage::StorageValueMissing;
        Check(storage.GetKeyAt("Enumerate.Id", 0, key, sizeof(key),
                               &required, &type) != FALSE &&
                  required == static_cast<int>(strlen(key)) + 1 &&
                  type == Salamatrix::Storage::StorageValueString &&
                  strcmp(key, "zeta") == 0,
              "enumeration key and type");
        Check(storage.GetKeyAt("Enumerate.Id", 1, key, sizeof(key),
                               &required, &type) != FALSE &&
                  type == Salamatrix::Storage::StorageValueInteger &&
                  strcmp(key, "number") == 0,
              "enumeration integer type");
        Check(storage.GetKeyAt("Enumerate.Id", 2, key, sizeof(key),
                               &required, &type) != FALSE &&
                  type == Salamatrix::Storage::StorageValueBoolean &&
                  strcmp(key, "flag") == 0,
              "enumeration boolean type");
        Check(storage.GetKeyAt("Enumerate.Id", 0, key, 1, &required, &type) == FALSE &&
                  required > 1 && key[0] == 0,
              "enumeration required-size failure");
        Check(storage.GetKeyAt("Enumerate.Id", 99, key, sizeof(key),
                               &required, &type) == FALSE,
              "enumeration invalid index");
        Check(storage.DeleteValue("Enumerate.Id", "flag") != FALSE &&
                  storage.GetKeyCount("Enumerate.Id") == 2,
              "enumeration after delete");
        Check(storage.ClearExtension("Enumerate.Id") != FALSE &&
                  storage.GetKeyCount("Enumerate.Id") == 0,
              "enumeration after clear");
    }

    void TestConfigurationRoundTrip()
    {
        const HKEY root = reinterpret_cast<HKEY>(1);
        FakeRegistry registry;

        Salamatrix::Storage::StorageService saved;
        saved.SetString("Round.Trip", "text", u8"UTF-8 ✓");
        saved.SetInteger("Round.Trip", "number", -9223372036854775807LL);
        saved.SetBoolean("Other.Extension", "flag", TRUE);
        Check(
            saved.SaveConfiguration(root, &registry) != FALSE &&
                !registry.Blob.empty() &&
                registry.WriteCount == 1,
            "serialize configuration blob");

        Salamatrix::Storage::StorageService loaded;
        Check(
            loaded.LoadConfiguration(root, &registry) != FALSE,
            "load configuration blob");

        char text[64];
        LONGLONG number = 0;
        BOOL flag = FALSE;
        Check(
            loaded.GetString(
                "Round.Trip", "text", text, sizeof(text), NULL) != FALSE &&
                strcmp(text, u8"UTF-8 ✓") == 0,
            "round-trip UTF-8 string");
        Check(
            loaded.GetInteger(
                "Round.Trip", "number", &number) != FALSE &&
                number == -9223372036854775807LL,
            "round-trip signed 64-bit integer");
        Check(
            loaded.GetBoolean(
                "Other.Extension", "flag", &flag) != FALSE &&
                flag,
            "round-trip isolated boolean");

        int writesBefore = registry.WriteCount;
        Check(
            loaded.SaveConfiguration(root, &registry) != FALSE &&
                registry.WriteCount == writesBefore,
            "unchanged loaded storage does not rewrite configuration");

        registry.Blob[0] ^= 0xff;
        Salamatrix::Storage::StorageService corrupted;
        Check(
            corrupted.LoadConfiguration(root, &registry) == FALSE,
            "reject corrupt blob magic");
    }
} // namespace

int main()
{
    TestTypesAndIsolation();
    TestValidationAndMutation();
    TestKeyEnumeration();
    TestConfigurationRoundTrip();

    if (Failures != 0)
    {
        std::fprintf(
            stderr,
            "%d Salamatrix storage test(s) failed.\n",
            Failures);
        return 1;
    }
    std::puts("All Salamatrix storage tests passed.");
    return 0;
}
