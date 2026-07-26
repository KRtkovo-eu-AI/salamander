// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    extensionmanifest.h
    Strict JSON parser and validated model for Salamatrix extension manifests.
*/

#pragma once

#ifndef SAL_MAX_PATH
#define SAL_MAX_PATH 32768
#endif

#include <stddef.h>
#include <string>
#include <vector>

struct CExtensionManifestError
{
    size_t Offset;
    size_t Line;
    size_t Column;
    std::string Message;

    CExtensionManifestError()
        : Offset(0),
          Line(1),
          Column(1)
    {
    }
};

struct CExtensionManifestCommand
{
    std::string Id;
    std::string Title;
    std::string Handler;
    std::string Menu;
    std::string Requires;
    bool ContextMenu;
    bool Toolbar;

    CExtensionManifestCommand()
        : Menu("plugin"),
          Requires("any"),
          ContextMenu(false),
          Toolbar(false)
    {
    }
};

enum CExtensionManifestSettingType
{
    ExtensionManifestSettingString = 1,
    ExtensionManifestSettingInteger = 2,
    ExtensionManifestSettingBoolean = 3
};

struct CExtensionManifestSetting
{
    std::string Key;
    CExtensionManifestSettingType Type;
    bool HasDefault;
    std::string StringDefault;
    long long IntegerDefault;
    bool BooleanDefault;

    CExtensionManifestSetting()
        : Type(ExtensionManifestSettingString),
          HasDefault(false),
          IntegerDefault(0),
          BooleanDefault(false)
    {
    }
};

class CExtensionManifest
{
public:
    unsigned int SchemaVersion;
    std::string Id;
    std::string Name;
    std::string Version;
    std::string Description;
    std::string RuntimeId;
    unsigned long MinimumRuntimeVersion;
    std::string EntryPoint;
    // Optional package-owned toolbar artwork. Paths are relative to the
    // manifest directory and must stay inside the extension package.
    std::string Icon;
    std::string IconDark;
    std::vector<std::string> Capabilities;
    // Optional ids of other manifest extensions that must be registered before
    // this package can activate. Runtime adapters remain described by runtime.
    std::vector<std::string> Dependencies;
    // Optional typed settings declarations.  Declarations are metadata only;
    // values remain isolated in Salamatrix.Storage under the manifest id.
    std::vector<CExtensionManifestSetting> Settings;
    // Optional event allow-list.  When EventsDeclared is true, a runtime may
    // subscribe only to names listed here; an absent member keeps legacy
    // manifests compatible and allows the complete event surface.
    bool EventsDeclared;
    std::vector<std::string> Events;
    std::vector<CExtensionManifestCommand> Commands;

    CExtensionManifest();

    void Clear();

    /// Parses and validates one complete UTF-8 JSON document.
    bool Parse(const char* json, size_t length, CExtensionManifestError& error);

    /// Entry points must stay inside the extension directory.
    static bool IsSafeRelativeEntryPoint(const std::string& entryPoint);
};
