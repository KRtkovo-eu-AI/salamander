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
        "\"dependencies\":[\"org.opensalamander.Core\",\"org.opensalamander.Shared\"],"
        "\"locales\":{\"en\":\"locales/en.json\",\"cs-CZ\":\"locales/cs-CZ.json\"},"
        "\"settings\":["
        "{\"key\":\"repositoryUrl\",\"type\":\"string\",\"label\":\"Repository\","
        "\"description\":\"Package source\",\"group\":\"General\",\"order\":10,"
        "\"width\":360,\"multiline\":true,\"default\":\"https://example.test\"},"
        "{\"key\":\"autoRefresh\",\"type\":\"boolean\",\"default\":true},"
        "{\"key\":\"maxItems\",\"type\":\"integer\",\"default\":42}"
        "],"
        "\"events\":[\"pathChanged\",\"selectionChanged\",\"fileChanged\"],"
        "\"commands\":["
        "{\"id\":\"Example.First\",\"title\":\"First\",\"menu\":\"both\","
        "\"contextMenu\":true,\"toolbar\":true,\"requires\":\"selection\",\"handler\":\"first\","
        "\"icon\":\"assets/first.svg\",\"iconDark\":\"assets/first-dark.svg\"},"
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
    CHECK(manifest.Dependencies.size() == 2);
    CHECK(manifest.Dependencies[0] == "org.opensalamander.Core");
    CHECK(manifest.Locales.size() == 2);
    CHECK(manifest.Locales[1].Language == "cs-CZ");
    CHECK(manifest.Locales[1].File == "locales/cs-CZ.json");
    CHECK(manifest.Settings.size() == 3);
    CHECK(manifest.Settings[0].Key == "repositoryUrl");
    CHECK(manifest.Settings[0].Label == "Repository");
    CHECK(manifest.Settings[0].Description == "Package source");
    CHECK(manifest.Settings[0].Group == "General");
    CHECK(manifest.Settings[0].Order == 10);
    CHECK(manifest.Settings[0].Width == 360);
    CHECK(manifest.Settings[0].Multiline);
    CHECK(manifest.Settings[0].Type == ExtensionManifestSettingString);
    CHECK(manifest.Settings[0].HasDefault);
    CHECK(manifest.Settings[0].StringDefault == "https://example.test");
    CHECK(manifest.Settings[1].Type == ExtensionManifestSettingBoolean);
    CHECK(manifest.Settings[1].BooleanDefault);
    CHECK(manifest.Settings[2].Type == ExtensionManifestSettingInteger);
    CHECK(manifest.Settings[2].IntegerDefault == 42);
    CHECK(manifest.EventsDeclared);
    CHECK(manifest.Events.size() == 3);
    CHECK(manifest.Events[0] == "pathChanged");
    CHECK(manifest.Commands.size() == 2);
    CHECK(manifest.Commands[0].Id == "Example.First");
    CHECK(manifest.Commands[0].Menu == "both");
    CHECK(manifest.Commands[0].ContextMenu);
    CHECK(manifest.Commands[0].Toolbar);
    CHECK(manifest.Commands[0].Icon == "assets/first.svg");
    CHECK(manifest.Commands[0].IconDark == "assets/first-dark.svg");
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
    CHECK(!manifest.EventsDeclared);
}

static void TestLocaleText()
{
    const char* json =
        "{\"name\":\"Obr\\u00e1zkov\\u00e9 n\\u00e1stroje\","
        "\"settings\":{\"repositoryUrl\":{\"label\":\"Zdroj\","
        "\"description\":\"Adresa bal\\u00ed\\u010dku\",\"group\":\"Obecn\\u00e9\"}},"
        "\"commands\":{\"Example.Resize\":\"Zm\\u011bnit velikost\"}}";
    CExtensionManifestLocaleText localized;
    CExtensionManifestError error;
    const bool parsedLocale = CExtensionManifest::ParseLocaleText(
        json, strlen(json), localized, error);
    CHECK(parsedLocale);
    if (parsedLocale)
    {
        CHECK(localized.Name == "Obr\xc3\xa1zkov\xc3\xa9 n\xc3\xa1stroje");
        CHECK(localized.Commands.size() == 1);
        if (localized.Commands.size() == 1)
        {
            CHECK(localized.Commands[0].Id == "Example.Resize");
            CHECK(localized.Commands[0].Title == "Zm\xc4\x9bnit velikost");
        }
        CHECK(localized.Settings.size() == 1);
        if (localized.Settings.size() == 1)
        {
            CHECK(localized.Settings[0].Key == "repositoryUrl");
            CHECK(localized.Settings[0].Label == "Zdroj");
            CHECK(localized.Settings[0].Group == "Obecn\xc3\xa9");
        }
    }

    const char* invalid = "{\"commands\":{\"bad id\":42}}";
    CHECK(!CExtensionManifest::ParseLocaleText(
        invalid, strlen(invalid), localized, error));
    CHECK(!error.Message.empty());
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
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"commands\":[{\"icon\":\"icon.png\"}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"commands\":[{\"iconDark\":\"../icon.svg\"}]}",
        "{\"schemaVersion\":2,\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\"}",
        "{\"id\":\"Bad\",\"runtime\":{\"id\":\"JS\",\"minimumVersion\":\"1.x\"},"
        "\"entryPoint\":\"main.js\"}",
        "{\"id\":\"Bad space\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\"}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"commands\":[{\"menu\":\"somewhere\"}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"events\":[\"unknownEvent\"]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"events\":[\"pathChanged\",\"pathChanged\"]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"settings\":[{\"key\":\"autoRefresh\",\"type\":\"boolean\",\"default\":\"yes\"}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"settings\":[{\"key\":\"size\",\"type\":\"string\",\"width\":119}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"settings\":[{\"key\":\"size\",\"type\":\"string\",\"order\":1.5}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"settings\":[{\"key\":\"same\",\"type\":\"string\"},{\"key\":\"SAME\",\"type\":\"string\"}]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"dependencies\":[\"org.good\",\"ORG.GOOD\"]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"dependencies\":[\"../unsafe\"]}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"locales\":{\"cs\":\"locales/cs.txt\"}}",
        "{\"id\":\"Bad\",\"runtime\":\"JS\",\"entryPoint\":\"main.js\","
        "\"locales\":{\"cs_CZ\":\"locales/cs.json\"}}",
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
    TestLocaleText();
    TestInvalidDocuments();

    if (g_failures != 0)
    {
        fprintf(stderr, "%d manifest parser test(s) failed\n", g_failures);
        return 1;
    }

    puts("All extension manifest tests passed.");
    return 0;
}
