// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../extensionmanifest.h"

#include <stdio.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(expression) \
    do \
    { \
        if (!(expression)) \
        { \
            fprintf(stderr, "%s(%d): check failed: %s\n", __FILE__, __LINE__, #expression); \
            ++g_failures; \
        } \
    } while (0)

static bool Parse(
    const char* json,
    CExtensionManifest& manifest,
    CExtensionManifestError& error)
{
    return manifest.Parse(json, strlen(json), error);
}

static void TestCompleteManifest()
{
    const char* json =
        "{"
        "\"schemaVersion\":1,"
        "\"id\":\"Example.Package\","
        "\"name\":\"Example \\u0161cript\","
        "\"version\":\"1.2.3\","
        "\"runtime\":{\"id\":\"Python.CPython\",\"minimumVersion\":\"3.12\"},"
        "\"entryPoint\":\"scripts/main.py\","
        "\"icon\":\"assets/icon.svg\",\"iconDark\":\"assets/icon-dark.svg\","
        "\"capabilities\":[\"panels.read\",\"ui.dialogs\"],"
        "\"commands\":["
        "{\"id\":\"Example.First\",\"title\":\"First\",\"menu\":\"both\","
        "\"contextMenu\":true,\"toolbar\":true,\"requires\":\"selection\",\"handler\":\"first\"},"
        "{\"id\":\"Example.Second\",\"title\":\"Second\",\"placement\":\"context\"}"
        "]"
        "}";

    CExtensionManifest manifest;
    CExtensionManifestError error;
    CHECK(Parse(json, manifest, error));
    CHECK(manifest.SchemaVersion == 1);
    CHECK(manifest.Id == "Example.Package");
    CHECK(manifest.Name == "Example \xc5\xa1"
                           "cript");
    CHECK(manifest.RuntimeId == "Python.CPython");
    CHECK(manifest.MinimumRuntimeVersion == 0x0003000c);
    CHECK(manifest.EntryPoint == "scripts/main.py");
    CHECK(manifest.Icon == "assets/icon.svg");
    CHECK(manifest.IconDark == "assets/icon-dark.svg");
    CHECK(manifest.Capabilities.size() == 2);
    CHECK(manifest.Commands.size() == 2);
    CHECK(manifest.Commands[0].Id == "Example.First");
    CHECK(manifest.Commands[0].Menu == "both");
    CHECK(manifest.Commands[0].ContextMenu);
    CHECK(manifest.Commands[0].Toolbar);
    CHECK(manifest.Commands[1].Menu == "context");
}

static void TestDefaults()
{
    const char* json =
        "{\"id\":\"Example.Default\",\"runtime\":\"Automation.JScript\","
        "\"entryPoint\":\"main.js\"}";

    CExtensionManifest manifest;
    CExtensionManifestError error;
    CHECK(Parse(json, manifest, error));
    CHECK(manifest.Name == "Example.Default");
    CHECK(manifest.Commands.size() == 1);
    CHECK(manifest.Commands[0].Id == "Example.Default");
    CHECK(manifest.Commands[0].Title == "Example.Default");
    CHECK(manifest.Commands[0].Menu == "plugin");
    CHECK(manifest.Commands[0].Requires == "any");
}

static void TestInvalidDocuments()
{
    const char* invalid[] = {
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"../bad.js\"}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"C:\\\\bad.js\"}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\",\"id\":\"Again\"}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\"} trailing",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\",\"commands\":{}}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\",\"commands\":["
        "{\"id\":\"Same\"},{\"id\":\"Same\"}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\",\"icon\":\"icon.png\"}",
        "{\"schemaVersion\":2,\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\"}",
        "{\"id\":\"Bad\",\"runtime\":{\"id\":\"JS\",\"minimumVersion\":\"1.x\"},"
        "\"entryPoint\":\"main.js\"}",
        "{\"id\":\"Bad space\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\"}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"commands\":[{\"menu\":\"somewhere\"}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"name\":\"unterminated}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"name\":\"\xc0\xaf\"}"};

    for (size_t i = 0; i < _countof(invalid); ++i)
    {
        CExtensionManifest manifest;
        CExtensionManifestError error;
        CHECK(!Parse(invalid[i], manifest, error));
        CHECK(!error.Message.empty());
    }
}

int main()
{
    TestCompleteManifest();
    TestDefaults();
    TestInvalidDocuments();

    if (g_failures != 0)
    {
        fprintf(stderr, "%d manifest parser test(s) failed\n", g_failures);
        return 1;
    }

    puts("All extension manifest tests passed.");
    return 0;
}
