// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-13 | Preserve external English modules for old plug-ins. | Keep legacy plug-ins using lang\\english.slg functional. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-13 | Load plug-in translations from each plug-in module. | Ship language resources with independently updatable .spl files. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-04 | Add the official language-pack loader API. | English ships in host binaries; non-English languages ship in one signed pack. | | Composer |

#pragma once

// Official language pack file name (PE DLL, Authenticode-signed with the release).
#define SAL_LANGPACK_FILENAME "langpack.dll"

// RCDATA resource name prefix inside the pack: LP_<LANGUAGE>_<MODULE>
// Example: LP_CZECH_SALAMAND, LP_GERMAN_ZIP
#define SAL_LANGPACK_RC_PREFIX "LP_"

// Manifest RCDATA name listing languages (optional, UTF-8 text).
#define SAL_LANGPACK_MANIFEST_NAME "SLPMANIFEST"

// Module key for the main application.
#define SAL_LANGPACK_MODULE_SALAMAND "salamand"

// Resource present in modern host/plugin binaries whose English UI resources are embedded.
#define SAL_LANGPACK_EMBEDDED_ENGLISH_RESOURCE_NAME "OPEN_SAMANDARIN_EMBEDDED_ENGLISH"

// Returns TRUE when 'slgName' is the built-in English language (english / english.slg).
BOOL SalIsEnglishLanguageName(const char* slgName);

// Strips a trailing ".slg" (case-insensitive) into 'out' (capacity 'outSize').
void SalLanguageBaseName(const char* slgName, char* out, int outSize);

// Builds the RCDATA name for a module+language into 'out'.
BOOL SalMakeLangPackResourceName(const char* languageBase, const char* moduleKey,
                                 char* out, int outSize);

// Resolves the official language pack path next to salamand.exe (\lang\langpack.dll).
BOOL SalGetLangPackPath(char* path, int pathSize);

// TRUE when 'hLanguage' must not be FreeLibrary'd (it is the host .exe/.spl itself).
BOOL SalLanguageModuleIsHost(HINSTANCE hostModule, HINSTANCE hLanguage);

// Loads a language module for 'moduleKey' ("salamand", "zip", ...).
// Preference: English from host -> module resources -> official langpack -> lang\*.slg on disk.
// When 'owned' is not NULL, it receives TRUE if the caller must FreeLibrary the result.
HINSTANCE SalLoadLanguageForModule(HINSTANCE hostModule, const char* moduleKey,
                                   const char* slgName, BOOL* owned);

// Loads an .slg PE image from memory via a DELETE_ON_CLOSE temp file + LoadLibrary.
HINSTANCE SalLoadLibraryFromMemory(const void* data, DWORD size);

// Opens the official language pack as a datafile (LoadLibraryEx). Caller FreeLibrary's.
HINSTANCE SalOpenLangPackDataFile();

// Enumerates language base names available in the pack for 'moduleKey'.
// 'callback' returns FALSE to stop. Returns the number of languages reported.
typedef BOOL (*SalLangPackEnumCallback)(const char* languageBase, void* userData);
int SalEnumLangPackLanguages(const char* moduleKey, SalLangPackEnumCallback callback, void* userData);

// Extracts one module language blob from a PE module's resources into a newly allocated buffer.
// Caller frees with free(). Returns FALSE on failure.
BOOL SalExtractLangResourceBlob(HINSTANCE hResourceModule, const char* languageBase,
                                const char* moduleKey, void** outData, DWORD* outSize);

// Enumerates language base names in an arbitrary loaded PE module's resources.
int SalEnumModuleLanguages(HINSTANCE hModule, const char* moduleKey,
                           SalLangPackEnumCallback callback, void* userData);
