# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import json
import re


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
INSTALLER = SRC / "plugins" / "samandarin" / "managed" / "PluginInstaller.cs"
AUTHENTICODE = SRC / "plugins" / "samandarin" / "managed" / "PluginAuthenticode.cs"
RECEIPTS = SRC / "plugins" / "samandarin" / "managed" / "PluginReceipts.cs"
ENTRY = SRC / "plugins" / "samandarin" / "managed" / "EntryPoint.cs"
INNO = ROOT / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss"
STRINGS = SRC / "plugins" / "samandarin" / "lang" / "lang.rc2"
RH2 = SRC / "plugins" / "samandarin" / "samandarin.rh2"
DIALOGS5 = SRC / "dialogs5.cpp"
PLUGINSECURITY = SRC / "pluginsecurity.cpp"
LANG_RC = SRC / "lang" / "lang.rc"
CAPABILITIES = ROOT / "doc" / "runbook-setup" / "plugin-capabilities.json"
RECEIPTS_DEFAULT = ROOT / "doc" / "runbook-setup" / "plugin-receipts.json"
CATALOG_UPDATER = ROOT / "tools" / "catalogs" / "update_stable_plugin_catalog.py"
STABLE_CATALOG = ROOT / "doc" / "catalogs-base" / "plugins-stable.json"
SECURITY_MD = ROOT / "SECURITY.md"
NETWORK_MD = ROOT / "doc" / "network-and-privacy.md"
CATALOG_README = ROOT / "doc" / "catalogs-base" / "README.md"
ISSUE_TEMPLATE = ROOT / ".github" / "ISSUE_TEMPLATE" / "05-plugin_catalog_entry.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def main() -> int:
    installer = read(INSTALLER)
    authenticode = read(AUTHENTICODE)
    receipts = read(RECEIPTS)
    entry = read(ENTRY)
    inno = read(INNO)
    strings = read(STRINGS)
    rh2 = read(RH2)
    dialogs5 = read(DIALOGS5)
    pluginsecurity = read(PLUGINSECURITY)
    lang_rc = read(LANG_RC)
    updater = read(CATALOG_UPDATER)
    catalog_readme = read(CATALOG_README)
    issue_template = read(ISSUE_TEMPLATE)

    require(
        "IsHexSha256(row.PackageSha256)" in installer,
        "CanInstall does not require a catalog SHA-256 digest",
    )
    require(
        "OfficialPackageDescriptor.TryParse(row.WebUrl" in installer,
        "CanInstall does not require an official GitHub package URL",
    )
    require(
        "ComputeSha256(archivePath)" in installer
        and "PluginInstallHashMismatch" in installer,
        "Downloaded archives are not hashed against the catalog digest",
    )
    require(
        "AuthenticodeVerifier.VerifyExtractedPackage" in installer,
        "Official install does not Authenticode-verify extracted binaries",
    )
    require(
        "WinVerifyTrust" in authenticode and "ExpectedPublisherNeedles" in authenticode,
        "Authenticode verifier does not call WinVerifyTrust with a publisher allow-list",
    )
    require(
        "WTD_STATEACTION_VERIFY" in authenticode or "dwStateAction = 1" in authenticode,
        "WinVerifyTrust is not used in verify state",
    )
    require(
        'FileName = "plugin-receipts.json"' in receipts
        and "GetExecutableDirectory" in receipts,
        "Install receipts are not stored next to salamand.exe",
    )
    require(
        "LocalAppData" not in receipts and "configstorage.ini" not in receipts,
        "Receipts must not use LocalAppData or configstorage.ini",
    )
    require(
        "plugin-receipts.json" in inno
        and "onlyifdoesntexist" in inno
        and "users-modify" in inno,
        "Inno Setup does not ship plugin-receipts.json with users-modify and onlyifdoesntexist",
    )
    require(
        "plugin-capabilities.json" in inno,
        "Inno Setup does not ship plugin-capabilities.json",
    )
    require(
        "bundled-plugin-metadata.json" in inno
        and "{#PayloadDir}\\bundled-plugin-metadata.json" in inno,
        "Inno Setup does not ship bundled plugin metadata from the payload",
    )
    require(
        "DeleteFile(ExpandConstant('{app}\\plugin-receipts.json'))" in inno,
        "Uninstall does not delete plugin-receipts.json with user configuration",
    )
    require(
        "IDS_PLUGIN_INSTALL_MISSING_HASH, " in strings
        and "IDS_PLUGIN_INSTALL_HASH_MISMATCH, " in strings
        and "IDS_PLUGIN_INSTALL_UNSIGNED, " in strings
        and "IDS_PLUGIN_INSTALL_UNEXPECTED_PUBLISHER, " in strings,
        "Localized fail-closed install errors are missing",
    )
    require(
        "PluginInstallMissingHash = 125" in entry
        and "#define IDS_PLUGIN_INSTALL_MISSING_HASH 125" in rh2,
        "NativeStringId 125+ is not wired to the new install errors",
    )
    require(
        'CATALOG_SCHEMA_VERSION = 6' in updater,
        "Catalog updater is not on schema version 6",
    )
    require(
        "packageSha256" in updater and "apply_package_trust_fields" in updater,
        "Catalog updater does not preserve packageSha256",
    )
    stable = json.loads(read(STABLE_CATALOG))
    require(stable.get("schemaVersion") == 6, "plugins-stable.json is not schema 6")
    require(
        all("security" in plugin for plugin in stable.get("plugins", [])),
        "Stable catalog entries are missing security metadata",
    )
    require(
        not any(
            plugin.get("packageSha256")
            and not re.fullmatch(r"[0-9a-f]{64}", str(plugin["packageSha256"]))
            for plugin in stable.get("plugins", [])
        ),
        "Stable catalog contains a malformed packageSha256 value",
    )
    capabilities = json.loads(read(CAPABILITIES))
    require(capabilities.get("schemaVersion") == 1, "plugin-capabilities.json schema is not 1")
    require(isinstance(capabilities.get("packages"), list), "plugin-capabilities.json has no packages")
    receipts_default = json.loads(read(RECEIPTS_DEFAULT))
    require(receipts_default.get("schemaVersion") == 1, "plugin-receipts.json schema is not 1")
    require(receipts_default.get("receipts") == [], "Default receipts file is not empty")
    require(
        "IDC_PLUGINSECURITY" in lang_rc and "Security:" in lang_rc,
        "Plugin Manager is missing the Security block",
    )
    require(
        "PluginSecurityFormatForPlugin" in dialogs5
        and '#include "pluginsecurity.h"' in dialogs5,
        "Plugin Manager does not fill the Security block",
    )
    require(
        'IDC_PLUGINSECURITY,"Static",SS_LEFT | SS_NOPREFIX | WS_GROUP,51,223,273,72' in lang_rc,
        "Plugin Manager Security block is too short for wrapped SHA-256 values",
    )
    require(
        "WinVerifyTrust" in pluginsecurity
        and "plugin-receipts.json" in pluginsecurity
        and "bundled-plugin-metadata.json" in pluginsecurity,
        "Native Plugin Manager does not read receipts or verify the on-disk signer",
    )
    require(
        "IsHigherRisk" in entry and "ConfirmSecurityCapabilities" in entry,
        "Plugin Updates does not warn before higher-risk installs",
    )
    require(
        "https://samandarin.krtkovo.eu/catalogs/plugins-stable.json" in entry
        and "https://samandarin.krtkovo.eu/catalogs/extensions-stable.json" in entry
        and "https://samandarin.krtkovo.eu/catalogs/plugins-unofficial.json" in entry
        and "https://samandarin.krtkovo.eu/catalogs/extension-runtimes.json" in entry
        and "RetiredStablePluginSource" in entry,
        "Plugin Updates catalog defaults are not the live krtkovo.eu catalogs",
    )
    require(
        "PluginSecurityUnverified" in entry and "Not cryptographically verified" in strings,
        "Unofficial or unverified packages are not labeled in Plugin Updates",
    )
    require(SECURITY_MD.is_file(), "SECURITY.md is missing")
    require(NETWORK_MD.is_file(), "doc/network-and-privacy.md is missing")
    require(
        "packageSha256" in catalog_readme and "Schema version 6" in catalog_readme,
        "Catalog README does not describe schema 6 / packageSha256",
    )
    require(
        "package-sha256" in issue_template and "network-access" in issue_template,
        "Catalog issue template does not request SHA-256 and security fields",
    )
    require(
        "github.com" in installer
        and "/KRtkovo-eu-AI/salamander-plugins/releases/download/" in installer,
        "Official auto-install URL allow-list changed unexpectedly",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
