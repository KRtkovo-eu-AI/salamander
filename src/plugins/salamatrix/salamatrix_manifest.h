// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_manifest.h
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
    // Optional command-specific toolbar artwork. When absent, the package
    // icon is used so existing manifests retain their current appearance.
    std::string Icon;
    std::string IconDark;
    // Optional executable name that must be discoverable through PATH before
    // this command can be invoked.
    std::string RequiresExecutable;
    bool ContextMenu;
    bool Toolbar;
    // When true, the toolbar contribution opens a package-local command menu
    // instead of executing this command directly.
    bool ToolbarMenu;
    // Initial menu state. Persistent workers may update it later through the
    // command state host call without changing the command identity.
    bool Enabled;
    bool Visible;

    CExtensionManifestCommand()
        : Menu("plugin"),
          Requires("any"),
          ContextMenu(false),
          Toolbar(false),
          ToolbarMenu(false),
          Enabled(true),
          Visible(true)
    {
    }
};

struct CExtensionManifestViewer
{
    // Salamander viewer masks (for example "*.md" or "*.png;*.jpg").
    // The framework registers these during the Salamatrix connect phase.
    std::vector<std::string> Patterns;
    std::string Handler;
};

struct CExtensionManifestFileSystem
{
    struct Action
    {
        std::string Id;
        std::string Title;
        std::string Handler;
        bool Default;

        Action() : Default(false) {}
    };
    // A package-local stable id. All Salamatrix file systems are exposed
    // through the framework-owned salamatrix: namespace.
    std::string Id;
    std::string Name;
    std::string ListHandler;
    std::string OpenHandler;
    std::string Icon;
    std::string IconDark;
    unsigned int RefreshIntervalMs;
    std::vector<Action> Actions;

    CExtensionManifestFileSystem()
        : RefreshIntervalMs(3000)
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
    // Optional configuration presentation metadata. The key remains the
    // stable storage identifier; labels and groups are display text only.
    std::string Label;
    std::string Description;
    std::string Group;
    int Order;
    int Width;
    bool Multiline;
    CExtensionManifestSettingType Type;
    bool HasDefault;
    std::string StringDefault;
    long long IntegerDefault;
    bool BooleanDefault;

    CExtensionManifestSetting()
        : Order(0),
          Width(250),
          Multiline(false),
          Type(ExtensionManifestSettingString),
          HasDefault(false),
          IntegerDefault(0),
          BooleanDefault(false)
    {
    }
};

struct CExtensionManifestSettingMigrationOperation
{
    std::string FromKey;
    std::string ToKey;
    bool Remove;

    CExtensionManifestSettingMigrationOperation()
        : Remove(false)
    {
    }
};

struct CExtensionManifestSettingMigration
{
    unsigned int FromVersion;
    unsigned int ToVersion;
    std::vector<CExtensionManifestSettingMigrationOperation> Operations;

    CExtensionManifestSettingMigration()
        : FromVersion(0),
          ToVersion(0)
    {
    }
};

struct CExtensionManifestLocale
{
    // BCP-47 language tag (for example "cs" or "en-US") and a package-owned
    // UTF-8 JSON resource path.
    std::string Language;
    std::string File;
};

struct CExtensionManifestLocalizedCommand
{
    std::string Id;
    std::string Title;
};

struct CExtensionManifestLocalizedSetting
{
    std::string Key;
    std::string Label;
    std::string Description;
    std::string Group;
};

struct CExtensionManifestLocaleText
{
    std::string Name;
    std::vector<CExtensionManifestLocalizedCommand> Commands;
    std::vector<CExtensionManifestLocalizedSetting> Settings;
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
    // Distinguishes a legacy manifest with no permission declaration from an
    // explicitly empty (deny-all) capability list.
    bool CapabilitiesDeclared;
    std::vector<std::string> Capabilities;
    // Optional ids of other manifest extensions that must be registered before
    // this package can activate. Runtime adapters remain described by runtime.
    std::vector<std::string> Dependencies;
    // Optional package-owned BCP-47 locale table. The selected JSON resource
    // can provide a name and command-title translations.
    std::vector<CExtensionManifestLocale> Locales;
    // Optional typed settings declarations.  Declarations are metadata only;
    // values remain isolated in Salamatrix.Storage under the manifest id.
    std::vector<CExtensionManifestSetting> Settings;
    unsigned int SettingsVersion;
    std::vector<CExtensionManifestSettingMigration> SettingsMigrations;
    // Optional event allow-list.  When EventsDeclared is true, a runtime may
    // subscribe only to names listed here; an absent member keeps legacy
    // manifests compatible and allows the complete event surface.
    bool EventsDeclared;
    std::vector<std::string> Events;
    std::vector<CExtensionManifestCommand> Commands;
    // Schema v2 contributions. Schema v1 remains accepted and normalizes to
    // empty collections so existing packages retain their behavior.
    std::vector<CExtensionManifestViewer> Viewers;
    std::vector<CExtensionManifestFileSystem> FileSystems;

    CExtensionManifest();

    void Clear();

    /// Parses and validates one complete UTF-8 JSON document.
    bool Parse(const char* json, size_t length, CExtensionManifestError& error);

    /// Entry points must stay inside the extension directory.
    static bool IsSafeRelativeEntryPoint(const std::string& entryPoint);

    /// Parses one UTF-8 locale resource declared by `locales`. Locale files
    /// are deliberately data-only; they cannot add capabilities or commands.
    static bool ParseLocaleText(
        const char* json,
        size_t length,
        CExtensionManifestLocaleText& localized,
        CExtensionManifestError& error);
};
