# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Check Welcome language choices from embedded packs. | Ensure Manage Configurations lists languages after .slg files are removed. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-13 | Test temporary language-module loading. | Prevent ERROR_SHARING_VIOLATION from hiding packaged translations. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-13 | Check cleanup of English language seeds. | Ensure shipped runtime trees do not retain obsolete english.slg files. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-13 | Guard against duplicate embedded English resources. | Keep host and plugin resource sources linkable. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-13 | Check plug-in-local translation resources. | Prevent regressions to one shared DLL for all plug-in languages. | | openrouter/openai/gpt-6-luna-pro |
"""Source-contract checks for host and plug-in localization packaging."""

from __future__ import annotations

from pathlib import Path

REPO = Path(__file__).resolve().parents[3]


def test_langpack_runtime_sources_exist():
    assert (REPO / "src" / "langpack.h").is_file()
    assert (REPO / "src" / "langpack.cpp").is_file()
    text = (REPO / "src" / "langpack.h").read_text(encoding="utf-8", errors="replace")
    assert 'SAL_LANGPACK_FILENAME "langpack.dll"' in text
    assert "SalLoadLanguageForModule" in text
    assert "SalExtractLangResourceBlob" in text
    assert "SalEnumModuleLanguages" in text
    assert "SalIsEnglishLanguageName" in text

    implementation = (REPO / "src" / "langpack.cpp").read_text(encoding="utf-8", errors="replace")
    assert "LoadLanguageFromResourceModule(hostModule, hostModule" in implementation
    assert "LoadLanguageFromResourceModule(hPack, hostModule" in implementation
    assert "Legacy / 3rd-party .slg" in implementation
    assert "EnumResourceNamesA(hModule, RT_RCDATA, EnumLangPackResNameProc" in implementation
    assert "EnumLangPackResNameProc(HMODULE hModule, LPCSTR lpszType, LPSTR lpszName" in implementation
    assert "EnumLangPackResNameProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName" not in implementation

    memory_loader = implementation.split("HINSTANCE SalLoadLibraryFromMemory", 1)[1].split(
        "HINSTANCE SalOpenLangPackDataFile", 1
    )[0]
    assert "GENERIC_WRITE" in memory_loader
    assert "FILE_SHARE_WRITE" in memory_loader
    assert "GENERIC_READ | DELETE" in memory_loader
    assert "FILE_FLAG_DELETE_ON_CLOSE" in memory_loader
    assert memory_loader.index("CloseHandle(hFile)") < memory_loader.index("GENERIC_READ | DELETE")
    assert memory_loader.index("GENERIC_READ | DELETE") < memory_loader.index("LoadLibrary(tempFile)")

    selector = (REPO / "src" / "dialogs2.cpp").read_text(encoding="utf-8", errors="replace")
    assert "SalEnumModuleLanguages(pluginDLL, moduleKey" in selector
    assert "SalEnumLangPackLanguages(moduleKey" in selector

    manage_dialog = selector.split("void CManageConfigsDialog::InitLanguageCombo()", 1)[1].split(
        "void CManageConfigsDialog::SelectLanguageInCombo", 1
    )[0]
    assert 'MCDAddSLGToLanguageCombo("english.slg", &ctx)' in manage_dialog
    assert "SalEnumLangPackLanguages(SAL_LANGPACK_MODULE_SALAMAND, MCDAddPackedLanguageToCombo" in manage_dialog
    assert 'lstrcpyn(fileName + 1, "lang\\\\*.slg"' in manage_dialog

    plugin_loader = (REPO / "src" / "plugins1.cpp").read_text(encoding="utf-8", errors="replace")
    assert "GetPluginLangPackModuleKey(Plugin->DLLName" in plugin_loader
    assert "const char* lastSeparator = NULL;" in plugin_loader
    assert "moduleStart[-1] != '\\\\' && moduleStart[-1] != '/'" in plugin_loader


def test_host_embeds_english_lang_rc():
    sal = (REPO / "src" / "vcxproj" / "salamand.vcxproj").read_text(encoding="utf-8", errors="replace")
    assert "lang\\lang.rc" in sal or "lang/lang.rc" in sal
    assert "SKIP_LANG_VERSIONINFO" in sal
    assert "langpack.cpp" in sal

    props = (REPO / "src" / "plugins" / "shared" / "vcxproj" / "plugin_base.props").read_text(
        encoding="utf-8", errors="replace"
    )
    assert "EmbedEnglishLangRc" in props
    assert "SKIP_LANG_VERSIONINFO" in props


def test_host_and_serviceexplorer_do_not_compile_duplicate_fallback_resources():
    salamand = (REPO / "src" / "salamand.rc").read_text(encoding="utf-8", errors="replace")
    main_language_texts = (REPO / "src" / "lang" / "texts.rc2").read_text(
        encoding="utf-8", errors="replace"
    )
    for resource_id in (
        "IDS_FILELAUNCH_ERROR_CODE",
        "IDS_FILELAUNCH_PROGRAM_FAILED",
        "IDS_FILELAUNCH_DOCUMENT_FAILED",
        "IDS_FILELOCK_TERMINATE_RETRY",
        "IDS_FILELOCK_USED_BY",
        "IDS_FILELOCK_CONFIRM_TERMINATE",
        "IDS_FILELOCK_TERMINATE_FAILED",
    ):
        assert resource_id not in salamand
        assert resource_id in main_language_texts

    serviceexplorer = (
        REPO / "src" / "plugins" / "serviceexplorer" / "serviceexplorer.rc2"
    ).read_text(encoding="utf-8", errors="replace")
    assert "SERVICEEXPLORER_LANG_FALLBACK" not in serviceexplorer
    assert '#include "lang/lang.rc2"' not in serviceexplorer
    serviceexplorer_language = (
        REPO / "src" / "plugins" / "serviceexplorer" / "lang" / "lang.rc2"
    ).read_text(encoding="utf-8", errors="replace")
    assert "IDD_CFGPAGEVIEWER DIALOGEX" in serviceexplorer_language


def test_versinfo_can_skip_when_embedding():
    vers = (REPO / "src" / "plugins" / "shared" / "versinfo.rc2").read_text(encoding="utf-8", errors="replace")
    assert "SKIP_LANG_VERSIONINFO" in vers


def test_inno_ships_optional_langpack_not_per_language_slg():
    iss = (REPO / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss").read_text(
        encoding="utf-8", errors="replace"
    )
    assert r"lang\langpack.dll" in iss
    assert "additionallanguages" in iss
    assert r"lang\czech.slg" not in iss
    assert r"plugins\zip\lang\*.slg" not in iss
    assert r"plugins\zip\zip.spl" in iss


def test_official_langpack_builder_exists():
    builder = (REPO / "tools" / "localization" / "build_official_langpack.ps1").read_text(
        encoding="utf-8", errors="replace"
    )
    assert 'Module = "salamand"' in builder
    assert 'Target = $outPack' in builder
    assert 'Join-Path $pluginDir.FullName "$module.spl"' in builder
    assert '$pluginTargets[$pluginBinary] = $module' in builder
    assert 'Get-ChildItem -LiteralPath $pluginsRoot -Directory -Recurse' in builder
    assert "GetRcDataNames" in builder
    assert "plugins\\<module>\\<module>.spl" in builder

    build = (REPO / "tools" / "localization" / "build_language_packs.ps1").read_text(
        encoding="utf-8", errors="replace"
    )
    assert "build_official_langpack.ps1" in build
    assert "-BuildRoot $buildRootFull -RemoveEnglishSlg" in build
