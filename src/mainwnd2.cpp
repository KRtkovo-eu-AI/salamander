// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include <shlwapi.h>
#undef PathIsPrefix // otherwise, collision with CSalamanderGeneral::PathIsPrefix

#include <string>
#include <vector>

#include "toolbar.h"
#include "stswnd.h"
#include "plugins.h"
#include "fileswnd.h"
#include "tabwnd.h"
#include "mainwnd.h"
#include "configstorage.h"
#include "cfgdlg.h"
#include "usermenu.h"
#include "viewer.h"
#include "zip.h"
#include "pack.h"
#include "find.h"
#include "dialogs.h"
#include "logo.h"
#include "tasklist.h"
#include "pwdmngr.h"

extern void ShowFileError(HWND hParent, int errTextID, const char* fileName, DWORD err);

extern const char* SalamanderConfigurationVersions[SALCFG_ROOTS_COUNT];

static const char* DetectProductName(const char* root);
static BOOL MCDIsGeneratedConfigDisplayName(const char* root, const char* version, const char* displayName);
static BOOL MCDGetCurrentInstancePath(char* path, int pathSize);


static void SaveInstalledPluginVersionsToBootstrap()
{
    char fileName[SAL_MAX_PATH];
    if (!ConfigurationStorage.GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return;

    const char* section = "InstalledPlugins";
    WritePrivateProfileString(section, NULL, NULL, fileName);

    char key[32];
    char count[32];
    int savedCount = 0;

    Plugins.EnterDataCS();
    int pluginCount = Plugins.GetCount();
    for (int i = 0; i < pluginCount; i++)
    {
        CPluginData* plugin = Plugins.Get(i);
        if (plugin == NULL || plugin->DLLName == NULL || plugin->Version == NULL)
            continue;

        char pluginPath[SAL_MAX_PATH];
        if (PathIsRelative(plugin->DLLName))
        {
            _snprintf_s(pluginPath, _TRUNCATE, "plugins/%s", plugin->DLLName);
            for (char* slash = pluginPath; *slash != 0; slash++)
            {
                if (*slash == '\\')
                    *slash = '/';
            }
        }
        else
            strcpy_s(pluginPath, plugin->DLLName);

        savedCount++;
        _snprintf_s(key, _TRUNCATE, "Plugin%dPath", savedCount);
        WritePrivateProfileString(section, key, pluginPath, fileName);
        _snprintf_s(key, _TRUNCATE, "Plugin%dVersion", savedCount);
        WritePrivateProfileString(section, key, plugin->Version, fileName);
    }
    Plugins.LeaveDataCS();

    _snprintf_s(count, _TRUNCATE, "%d", savedCount);
    WritePrivateProfileString(section, "Count", count, fileName);
}

static const char* MCDSubKeyFromLocation(const char* location)
{
    static const char prefix[] = "reg:\\HKEY_CURRENT_USER\\";
    const size_t prefixLen = sizeof(prefix) - 1;
    if (location == NULL || _strnicmp(location, prefix, prefixLen) != 0)
        return NULL;
    const char* subkey = location + prefixLen;
    return subkey[0] != 0 ? subkey : NULL;
}

static BOOL MCDCopyRegistryKey(CSalamanderRegistryExAbstract* inReg, HKEY inKey,
                               CSalamanderRegistryExAbstract* outReg, HKEY outKey)
{
    char name[MAX_PATH];

    for (DWORD keyIndex = 0;; keyIndex++)
    {
        HKEY inSubKey, outSubKey;
        if (!inReg->EnumKey(inKey, keyIndex, name, SizeOf(name)))
            break;
        if (!inReg->OpenKey(inKey, name, inSubKey))
            return FALSE;
        if (!outReg->CreateKey(outKey, name, outSubKey))
        {
            inReg->CloseKey(inSubKey);
            return FALSE;
        }
        BOOL copied = MCDCopyRegistryKey(inReg, inSubKey, outReg, outSubKey);
        inReg->CloseKey(inSubKey);
        outReg->CloseKey(outSubKey);
        if (!copied)
            return FALSE;
    }

    for (DWORD valIndex = 0;; valIndex++)
    {
        DWORD valType;
        DWORD dataSize = 0;
        if (!inReg->EnumValue(inKey, valIndex, name, SizeOf(name), &valType, NULL, NULL))
            break;
        if (!inReg->GetSize(inKey, name, valType, dataSize))
            return FALSE;

        BYTE stackData[512];
        LPBYTE data = stackData;
        if (dataSize > sizeof(stackData))
        {
            data = (LPBYTE)malloc(dataSize);
            if (data == NULL)
                return FALSE;
        }

        BOOL ok = inReg->GetValue(inKey, name, valType, data, dataSize) &&
                  outReg->SetValue(outKey, name, valType, data, dataSize);
        if (data != stackData)
            free(data);
        if (!ok)
            return FALSE;
    }
    return TRUE;
}

static BOOL MCDCopyRegistryBranchToTarget(CSalamanderRegistryExAbstract* inReg, const char* inSubkey,
                                          CSalamanderRegistryExAbstract* outReg, const char* outSubkey,
                                          BOOL clearTarget)
{
    if (inReg == NULL || outReg == NULL || inSubkey == NULL || outSubkey == NULL ||
        inSubkey[0] == 0 || outSubkey[0] == 0)
        return FALSE;

    HKEY inKey;
    if (!inReg->OpenKey(HKEY_CURRENT_USER, inSubkey, inKey))
        return FALSE;

    HKEY outKey;
    if (!outReg->CreateKey(HKEY_CURRENT_USER, outSubkey, outKey))
    {
        inReg->CloseKey(inKey);
        return FALSE;
    }
    if (clearTarget)
        outReg->ClearKey(outKey);

    BOOL ret = MCDCopyRegistryKey(inReg, inKey, outReg, outKey);
    outReg->CloseKey(outKey);
    inReg->CloseKey(inKey);
    return ret;
}

static BOOL MCDSetConfigValue(CSalamanderRegistryExAbstract* registry, const char* subkey,
                              const char* valueName, DWORD type, const void* data, DWORD dataSize)
{
    if (registry == NULL || subkey == NULL || subkey[0] == 0)
        return FALSE;

    HKEY hRootKey;
    if (!registry->CreateKey(HKEY_CURRENT_USER, subkey, hRootKey))
        return FALSE;

    BOOL ret = FALSE;
    HKEY hCfgKey;
    if (registry->CreateKey(hRootKey, SALAMANDER_CONFIG_REG, hCfgKey))
    {
        ret = registry->SetValue(hCfgKey, valueName, type, data, dataSize);
        registry->CloseKey(hCfgKey);
    }
    registry->CloseKey(hRootKey);
    return ret;
}

static void MCDApplyWelcomeTargetMetadata(CSalamanderRegistryExAbstract* registry, const char* targetSubkey,
                                          const char* customConfigName, const char* customLanguage, BOOL markWelcomeProcessed)
{
    char effectiveName[256];
    effectiveName[0] = 0;
    if (customConfigName != NULL && customConfigName[0] != 0)
        strncpy_s(effectiveName, customConfigName, _TRUNCATE);
    else
        sprintf_s(effectiveName, DetectProductName(targetSubkey), SalamanderConfigurationVersions[0]);
    MCDSetConfigValue(registry, targetSubkey, "ConfigDisplayName", REG_SZ,
                      effectiveName, (DWORD)(strlen(effectiveName) + 1));

    if (customLanguage != NULL && customLanguage[0] != 0)
        MCDSetConfigValue(registry, targetSubkey, "Language", REG_SZ,
                          customLanguage, (DWORD)(strlen(customLanguage) + 1));

    if (markWelcomeProcessed)
    {
        DWORD processed = 1;
        MCDSetConfigValue(registry, targetSubkey, "WelcomeProcessed", REG_DWORD, &processed, sizeof(processed));
        char instancePath[MAX_PATH];
        if (MCDGetCurrentInstancePath(instancePath, SizeOf(instancePath)))
            MCDSetConfigValue(registry, targetSubkey, "WelcomeProcessedInstancePath", REG_SZ,
                              instancePath, (DWORD)(strlen(instancePath) + 1));
    }
}

static BOOL MCDParseRegFileToMemory(HWND parent, const char* fileName, CSalamanderRegistryExAbstract* memReg, BOOL showErrors = TRUE)
{
    if (fileName == NULL || fileName[0] == 0 || memReg == NULL)
        return FALSE;

    HANDLE file = HANDLES_Q(CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0));
    if (file == INVALID_HANDLE_VALUE)
    {
        if (showErrors) ShowFileError(parent, IDS_IMPORTCFG_OPENERR, fileName, GetLastError());
        return FALSE;
    }

    BOOL ret = FALSE;
    DWORD err = 0;
    CQuadWord size;
    LPTSTR buf = NULL;
    BOOL haveSize = SalGetFileSize(file, size, err);
    if (haveSize && size <= CQuadWord(10000000, 0))
    {
        buf = (LPTSTR)malloc((DWORD)size.Value + sizeof(WCHAR));
        if (buf != NULL)
        {
            DWORD bytesRead = 0;
            if (ReadFile(file, buf, (DWORD)size.Value, &bytesRead, NULL))
            {
                *(WCHAR*)((LPBYTE)buf + bytesRead) = 0;
                if (ConvertIfNeeded(&buf, bytesRead) != 0)
                {
                    eRPE_ERROR regerr = Parse(buf, memReg, TRUE);
                    if (regerr == RPE_OK)
                        ret = TRUE;
                    else
                    {
                        int errTextID = IDS_IMPORTCFG_REGERR;
                        switch (regerr)
                        {
                        case RPE_NOT_REG_FILE: errTextID = IDS_IMPORTCFG_NOTREG; break;
                        case RPE_ROOT_INVALID_KEY:
                        case RPE_INVALID_KEY:
                        case RPE_VALUE_MISSING_QUOTE:
                        case RPE_VALUE_MISSING_ASSIG:
                        case RPE_VALUE_INVALID_TYPE:
                        case RPE_VALUE_DWORD:
                        case RPE_VALUE_STRING:
                        case RPE_VALUE_HEX:
                        case RPE_INVALID_MBCS:
                        case RPE_INVALID_FORMAT:
                            errTextID = IDS_IMPORTCFG_INVALIDFORMAT;
                            break;
                        }
                        if (showErrors) ShowFileError(parent, errTextID, fileName, 0);
                    }
                }
            }
            else
                if (showErrors) ShowFileError(parent, IDS_IMPORTCFG_OPENERR, fileName, GetLastError());
        }
    }
    else if (haveSize)
        if (showErrors) ShowFileError(parent, IDS_IMPORTCFG_TOOBIG, fileName, 0);
    else
        if (showErrors) ShowFileError(parent, IDS_IMPORTCFG_OPENERR, fileName, err);

    HANDLES(CloseHandle(file));
    if (buf != NULL)
        free(buf);
    return ret;
}

static const char* MCDFindSourceSubkeyInRegistry(CSalamanderRegistryExAbstract* registry)
{
    if (registry == NULL)
        return NULL;

    for (int i = 0; i < SALCFG_ROOTS_COUNT; i++)
    {
        HKEY key;
        if (registry->OpenKey(HKEY_CURRENT_USER, SalamanderConfigurationRoots[i], key))
        {
            registry->CloseKey(key);
            return SalamanderConfigurationRoots[i];
        }
    }
    return NULL;
}

static int MCDRootIndexFromSubkey(const char* subkey)
{
    if (subkey == NULL)
        return -1;
    for (int i = 0; i < SALCFG_ROOTS_COUNT; i++)
        if (_stricmp(SalamanderConfigurationRoots[i], subkey) == 0)
            return i;
    return -1;
}

static BOOL MCDRestartSalamanderAfterWelcome(HWND parent)
{
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    char initDir[MAX_PATH];
    strncpy_s(initDir, exePath, _TRUNCATE);
    char* slash = strrchr(initDir, '\\');
    if (slash != NULL)
        *slash = 0;

    SHELLEXECUTEINFO se;
    memset(&se, 0, sizeof(se));
    se.cbSize = sizeof(se);
    se.nShow = SW_SHOWNORMAL;
    se.hwnd = parent;
    se.lpFile = exePath;
    se.lpDirectory = initDir;

    return ShellExecuteEx(&se);
}

static BOOL MCDGetCurrentInstancePath(char* path, int pathSize)
{
    if (path == NULL || pathSize <= 0)
        return FALSE;
    DWORD len = GetModuleFileName(NULL, path, pathSize);
    if (len == 0 || len >= (DWORD)pathSize)
    {
        path[0] = 0;
        return FALSE;
    }
    return TRUE;
}

static BOOL MCDReadRegistryString(CSalamanderRegistryExAbstract* registry, HKEY key,
                                  const char* valueName, char* value, int valueSize)
{
    if (value == NULL || valueSize <= 0)
        return FALSE;
    value[0] = 0;
    DWORD size = valueSize;
    if (!registry->GetValue(key, valueName, REG_SZ, value, size) || value[0] == 0)
        return FALSE;
    value[valueSize - 1] = 0;
    return TRUE;
}

static BOOL MCDReadRegistryConfigLanguage(CSalamanderRegistryExAbstract* registry, HKEY rootKey,
                                          char* language, int languageSize)
{
    if (language == NULL || languageSize <= 0)
        return FALSE;
    language[0] = 0;
    HKEY cfgKey;
    if (registry->OpenKey(rootKey, "Configuration", cfgKey))
    {
        MCDReadRegistryString(registry, cfgKey, "Language", language, languageSize);
        registry->CloseKey(cfgKey);
        char* dot = strrchr(language, '.');
        if (dot != NULL && _stricmp(dot, ".slg") == 0)
            *dot = 0;
    }
    return language[0] != 0;
}

BOOL MCDReadFileConfigurationInfo(const char* fileName, CFoundConfig& cfg, BOOL showErrors)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.Exists = TRUE;
    cfg.IsPortable = TRUE;
    cfg.RootIndex = -1;
    strncpy_s(cfg.StorageTypeStr, LoadStr(IDS_MCD_STORAGE_FILE), _TRUNCATE);
    strncpy_s(cfg.Location, fileName != NULL ? fileName : "", _TRUNCATE);

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (fileName != NULL && GetFileAttributesEx(fileName, GetFileExInfoStandard, &fad))
        cfg.LastUpdate = fad.ftLastWriteTime;

    CSalamanderRegistryExAbstract* sourceReg = REG_MemRegistryFactory();
    if (sourceReg == NULL || !MCDParseRegFileToMemory(NULL, fileName, sourceReg, showErrors))
    {
        if (sourceReg != NULL)
            sourceReg->Release();
        _snprintf_s(cfg.DisplayName, _TRUNCATE, LoadStr(IDS_MCD_FILEPREFIX), fileName != NULL ? fileName : "");
        strncpy_s(cfg.Version, SalamanderConfigurationVersions[0], _TRUNCATE);
        return FALSE;
    }

    const char* sourceSubkey = MCDFindSourceSubkeyInRegistry(sourceReg);
    int rootIndex = MCDRootIndexFromSubkey(sourceSubkey);
    if (rootIndex >= 0)
    {
        cfg.IsCurrentVersion = (rootIndex == 0);
        strncpy_s(cfg.Version, SalamanderConfigurationVersions[rootIndex], _TRUNCATE);

        HKEY rootKey;
        if (sourceReg->OpenKey(HKEY_CURRENT_USER, sourceSubkey, rootKey))
        {
            HKEY cfgKey;
            char customName[256];
            customName[0] = 0;
            if (sourceReg->OpenKey(rootKey, "Configuration", cfgKey))
            {
                MCDReadRegistryString(sourceReg, cfgKey, "ConfigDisplayName", customName, SizeOf(customName));
                sourceReg->CloseKey(cfgKey);
            }
            if (customName[0] != 0)
                strncpy_s(cfg.DisplayName, customName, _TRUNCATE);
            else
                sprintf_s(cfg.DisplayName, DetectProductName(sourceSubkey), SalamanderConfigurationVersions[rootIndex]);
            cfg.IsGeneratedName = MCDIsGeneratedConfigDisplayName(sourceSubkey, SalamanderConfigurationVersions[rootIndex], cfg.DisplayName);
            MCDReadRegistryConfigLanguage(sourceReg, rootKey, cfg.Language, SizeOf(cfg.Language));
            sourceReg->CloseKey(rootKey);
        }
    }

    if (cfg.DisplayName[0] == 0)
        _snprintf_s(cfg.DisplayName, _TRUNCATE, LoadStr(IDS_MCD_FILEPREFIX), fileName != NULL ? fileName : "");
    if (cfg.Version[0] == 0)
        strncpy_s(cfg.Version, SalamanderConfigurationVersions[0], _TRUNCATE);
    sourceReg->Release();
    return rootIndex >= 0;
}

static BOOL MCDSetValueInSubkey(CSalamanderRegistryExAbstract* registry, const char* subkey,
                                const char* valueName, DWORD type, const void* data, DWORD dataSize)
{
    HKEY key;
    if (registry == NULL || subkey == NULL || subkey[0] == 0 ||
        !registry->CreateKey(HKEY_CURRENT_USER, subkey, key))
        return FALSE;
    BOOL ret = registry->SetValue(key, valueName, type, data, dataSize);
    registry->CloseKey(key);
    return ret;
}

static BOOL MCDIsSamandarin01To06Version(const char* version)
{
    const char* samandarin = version != NULL ? StrIStr(version, "Samandarin") : NULL;
    if (samandarin == NULL)
        return FALSE;

    const char* minor = StrIStr(samandarin, "0.");
    if (minor == NULL || minor[2] < '1' || minor[2] > '6')
        return FALSE;

    char next = minor[3];
    return next == 0 || next < '0' || next > '9';
}

static BOOL MCDShouldMigrateSamandarin01To06Scheme(const char* subkey, const char* version)
{
    // Match the concrete Samandarin version text in either the registry/file root
    // path or the display version. Do not depend on SalamanderConfigurationRoots[]
    // indices because adding a newer root shifts older versions down.
    return MCDIsSamandarin01To06Version(subkey) || MCDIsSamandarin01To06Version(version);
}

static void MCDNormalizeSamandarin01To06ColorScheme(CSalamanderRegistryExAbstract* registry, const char* targetSubkey)
{
    if (registry == NULL || targetSubkey == NULL || targetSubkey[0] == 0)
        return;

    char colorsSubkey[MAX_PATH];
    _snprintf_s(colorsSubkey, _TRUNCATE, "%s\\Colors", targetSubkey);

    HKEY colorsKey;
    if (!registry->OpenKey(HKEY_CURRENT_USER, colorsSubkey, colorsKey))
        return;

    DWORD scheme = 4;
    DWORD useWinDark = 0;
    BOOL schemeLoaded = registry->GetValue(colorsKey, "Color Scheme", REG_DWORD, &scheme, sizeof(scheme));
    BOOL useWinDarkLoaded = registry->GetValue(colorsKey, "Use Windows Dark Mode", REG_DWORD, &useWinDark, sizeof(useWinDark));
    registry->CloseKey(colorsKey);

    if (!schemeLoaded)
        return;

    DWORD normalizedScheme = scheme;
    if (scheme == 4 && useWinDarkLoaded && useWinDark != 0)
        normalizedScheme = 5;
    else if (scheme == 5 && (!useWinDarkLoaded || useWinDark == 0))
        normalizedScheme = 4;

    if (normalizedScheme != scheme)
        MCDSetValueInSubkey(registry, colorsSubkey, "Color Scheme", REG_DWORD, &normalizedScheme, sizeof(normalizedScheme));
}

static void MCDApplyCleanTargetDefaults(CSalamanderRegistryExAbstract* registry, const char* targetSubkey)
{
    char versionSubkey[MAX_PATH];
    _snprintf_s(versionSubkey, _TRUNCATE, "%s\\Version", targetSubkey);
    // Keep clean targets new enough for early language loading, but one version behind
    // so the first real start still runs the standard plug-in auto-install path.
    DWORD configVersion = THIS_CONFIG_VERSION - 1;
    MCDSetValueInSubkey(registry, versionSubkey, SALAMANDER_VERSIONREG_REG, REG_DWORD,
                        &configVersion, sizeof(configVersion));

    if (Configuration.SLGName[0] != 0)
        MCDSetConfigValue(registry, targetSubkey, CONFIG_LANGUAGE_REG, REG_SZ,
                          Configuration.SLGName, (DWORD)(strlen(Configuration.SLGName) + 1));
    MCDSetConfigValue(registry, targetSubkey, CONFIG_USEALTLANGFORPLUGINS_REG, REG_DWORD,
                      &Configuration.UseAsAltSLGInOtherPlugins, sizeof(DWORD));
    MCDSetConfigValue(registry, targetSubkey, CONFIG_ALTLANGFORPLUGINS_REG, REG_SZ,
                      Configuration.AltPluginSLGName, (DWORD)(strlen(Configuration.AltPluginSLGName) + 1));
    DWORD langChanged = FALSE;
    MCDSetConfigValue(registry, targetSubkey, CONFIG_LANGUAGECHANGED_REG, REG_DWORD,
                      &langChanged, sizeof(langChanged));

    DWORD isMyDocs = TRUE;
    MCDSetConfigValue(registry, targetSubkey, "If Path Is Inaccessible Go To My Docs", REG_DWORD, &isMyDocs, sizeof(isMyDocs));

    DWORD showBottomToolbar = TRUE;
    MCDSetConfigValue(registry, targetSubkey, "Show Bottom ToolBar", REG_DWORD, &showBottomToolbar, sizeof(showBottomToolbar));

    char docsPath[MAX_PATH];
    docsPath[0] = 0;
    if (GetMyDocumentsOrDesktopPath(docsPath, SizeOf(docsPath)))
        MCDSetConfigValue(registry, targetSubkey, "If Path Is Inaccessible Go To", REG_SZ, docsPath, (DWORD)(strlen(docsPath) + 1));
    else
        MCDSetConfigValue(registry, targetSubkey, "If Path Is Inaccessible Go To", REG_SZ, "", 1);

    char colorsSubkey[MAX_PATH];
    _snprintf_s(colorsSubkey, _TRUNCATE, "%s\\Colors", targetSubkey);
    DWORD scheme = DarkModeShouldUseDarkColors() ? 5 : 0;
    DWORD useWinDark = (scheme == 5) ? 1 : 0;
    MCDSetValueInSubkey(registry, colorsSubkey, "Color Scheme", REG_DWORD, &scheme, sizeof(scheme));
    MCDSetValueInSubkey(registry, colorsSubkey, "Use Windows Dark Mode", REG_DWORD, &useWinDark, sizeof(useWinDark));
}

static BOOL MCDLoadSourceIntoTargetRegistry(HWND parent, const CFoundConfig& srcCfg,
                                            CSalamanderRegistryExAbstract* targetReg,
                                            const char* targetSubkey,
                                            BOOL targetIsFileStorage)
{
    if (targetReg == NULL || targetSubkey == NULL || targetSubkey[0] == 0)
        return FALSE;

    if (srcCfg.RootIndex == -1 && !srcCfg.IsPortable)
    {
        HKEY targetKey;
        if (targetReg->CreateKey(HKEY_CURRENT_USER, targetSubkey, targetKey))
        {
            targetReg->ClearKey(targetKey);
            targetReg->CloseKey(targetKey);
            MCDApplyCleanTargetDefaults(targetReg, targetSubkey);
            return TRUE;
        }
        return FALSE;
    }

    if (srcCfg.IsPortable)
    {
        CSalamanderRegistryExAbstract* sourceReg = REG_MemRegistryFactory();
        if (sourceReg == NULL)
            return FALSE;

        BOOL ret = FALSE;
        if (MCDParseRegFileToMemory(parent, srcCfg.Location, sourceReg))
        {
            const char* sourceSubkey = MCDFindSourceSubkeyInRegistry(sourceReg);
            if (sourceSubkey != NULL)
            {
                ret = MCDCopyRegistryBranchToTarget(sourceReg, sourceSubkey, targetReg, targetSubkey, TRUE);
                if (ret && MCDShouldMigrateSamandarin01To06Scheme(sourceSubkey, srcCfg.Version))
                    MCDNormalizeSamandarin01To06ColorScheme(targetReg, targetSubkey);
            }
            else
            {
                char text[MAX_PATH + 300];
                _snprintf_s(text, _TRUNCATE, LoadStr(IDS_IMPORTCFG_NOTOURVER), srcCfg.Location);
                SalMessageBox(parent, text, LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            }
        }
        sourceReg->Release();
        return ret;
    }

    const char* sourceSubkey = MCDSubKeyFromLocation(srcCfg.Location);
    if (sourceSubkey == NULL)
        return FALSE;
    if (_stricmp(sourceSubkey, targetSubkey) == 0 && !targetIsFileStorage)
        return TRUE; // selected source is already the registry target; keep it and only apply metadata

    CSalamanderRegistryExAbstract* sourceReg = REG_SysRegistryFactory();
    if (sourceReg == NULL)
        return FALSE;
    BOOL ret = MCDCopyRegistryBranchToTarget(sourceReg, sourceSubkey, targetReg, targetSubkey, TRUE);
    if (ret && MCDShouldMigrateSamandarin01To06Scheme(sourceSubkey, srcCfg.Version))
        MCDNormalizeSamandarin01To06ColorScheme(targetReg, targetSubkey);
    sourceReg->Release();
    return ret;
}

BOOL MCDApplyConfigurationSelection(HWND parent, const CManageConfigsDialog& dlg, BOOL markWelcomeProcessed, const char*& loadConfiguration)
{
    loadConfiguration = NULL;
    if (dlg.SelectedSourceIndex < 0 || dlg.SelectedSourceIndex >= dlg.ConfigsCount ||
        !dlg.Configs[dlg.SelectedSourceIndex].Exists)
        return FALSE;

    const CFoundConfig& srcCfg = dlg.Configs[dlg.SelectedSourceIndex];
    const char* targetSubkey = SalamanderConfigurationRoots[0]; // Registry target is intentionally the current-version configuration root.

    if (dlg.StorageType == cstRegFile)
    {
        if (dlg.RegFilePath[0] == 0)
            return FALSE;

        CSalamanderRegistryExAbstract* targetReg = REG_MemRegistryFactory();
        if (targetReg == NULL)
            return FALSE;

        BOOL ret = MCDLoadSourceIntoTargetRegistry(parent, srcCfg, targetReg, targetSubkey, TRUE);
        if (ret)
        {
            MCDApplyWelcomeTargetMetadata(targetReg, targetSubkey, dlg.CustomConfigName, dlg.CustomLanguage, markWelcomeProcessed);
            char clearKeyName[MAX_PATH];
            _snprintf_s(clearKeyName, _TRUNCATE, "HKEY_CURRENT_USER\\%s", targetSubkey);
            ret = targetReg->Dump(dlg.RegFilePath, clearKeyName);
            if (ret)
                loadConfiguration = targetSubkey;
            else
                ShowFileError(parent, IDS_EXPORTCFG_FILEERR, dlg.RegFilePath, GetLastError());
        }
        targetReg->Release();
        return ret;
    }

    CSalamanderRegistryExAbstract* targetReg = REG_SysRegistryFactory();
    if (targetReg == NULL)
        return FALSE;

    BOOL ret = MCDLoadSourceIntoTargetRegistry(parent, srcCfg, targetReg, targetSubkey, FALSE);
    if (ret)
    {
        MCDApplyWelcomeTargetMetadata(targetReg, targetSubkey, dlg.CustomConfigName, dlg.CustomLanguage, markWelcomeProcessed);
        loadConfiguration = targetSubkey;
    }
    targetReg->Release();
    return ret;
}

//
// ConfigVersion - version number of the loaded configuration
//
// 0 = no configuration found - default values are used
// 1 = version 1.52 and older
// 2 = 1.6b1
// 3 = 1.6b1.x
// 4 = 1.6b3.x
// 5 = 1.6b3.x          needed for proper conversion of packer configuration between our versions
// 6 = 1.6b4.x
// 7 = 1.6b5.x          needed for proper conversion of supported plug-in functions (see CPlugins::Load)
// 8 = 1.6b5.x          better overlook ;-)
// 9 = 1.6b5.x          because of switching from exe name to variable for custom packers
// 10 = 1.6b6           due to renaming "XXX (Internal)" to "XXX (Plug-in)" in Pack and Unpack dialogs
//                      and to set the ANSI version of the "list of files" for ACE32 and PKZIP25 (un)packers
// 11 = 1.6b7           added CheckVer plug-in - ensure automatic installation
// 12 = 2.0             auto-close salopen.exe + added PEViewer plug-in - ensure automatic installation
// 13 = 2.5b1           added missing conversion of custom packer configuration - reflect change for LHA
// 14 = 2.5b1           new Advanced Options in Find dialog. Switched to CFilterCriteria. Converted inverse filter mask.
// 15 = 2.5b2           newer version so that plug-ins load (upgrade registry records)
// 16 = 2.5b2           added coloring of Encrypted files and folders (added when loading configuration + included in default configuration)
// 17 = 2.5b2           added *.xml mask to internal viewer settings - "force text mode"
// 18 = 2.5b3           only to transfer plug-in configuration from version 2.5b2
// 19 = 2.5b4           only to transfer plug-in configuration from version 2.5b3
// 20 = 2.5b5           only to transfer plug-in configuration from version 2.5b4
// 21 = 2.5b6           only to transfer plug-in configuration from version 2.5b5(a)
// 22 = 2.5b6           panel filters - unified into one history
// 23 = 2.5b6           new panel view (Tiles)
// 24 = 2.5b7           only to transfer plug-in configuration from version 2.5b6
// 25 = 2.5b7           plugins: show in plugin bar -> transfer variable into CPluginData
// 26 = 2.5b8           only to transfer plug-in configuration from version 2.5b7
// 27 = 2.5b9           only to transfer plug-in configuration from version 2.5b8
// 28 = 2.5b9           new color scheme from old DOS Navigator -> convert 'scheme'
// 29 = 2.5b10          only to transfer plug-in configuration from version 2.5b9
// 30 = 2.5b11          only to transfer plug-in configuration from version 2.5b10
// 31 = 2.5b11          introduced Floppy section in Drives configuration - need to force reading icons for Removable
// 32 = 2.5b11          Find: "Local Settings\\Temporary Internet Files" is searched by default
// 33 = 2.5b12          only to transfer plug-in configuration from version 2.5b11
// 34 = 2.5b12          modification of external packer/unpacker PKZIP25 (external Win32 version)
// 35 = 2.5RC1          only to transfer plug-in configuration from version 2.5b12 (internal only, we shipped RC1 instead)
// 36 = 2.5RC2          only to transfer plug-in configuration from version 2.5RC1
// 37 = 2.5RC3          only to transfer plug-in configuration from version 2.5RC2
// 38 = 2.5RC3          renamed Servant Salamander to Altap Salamander
// 39 = 2.5             only to transfer plug-in configuration from version 2.5RC3
// 40 = 2.51            only to transfer plug-in configuration from version 2.5
// 41 = 2.51            configuration version containing a list of disabled icon overlay handlers (see CONFIG_DISABLEDCUSTICOVRLS_REG)
// 42 = 2.52b1          only to transfer plug-in configuration from version 2.51
// 43 = 2.52b2          only to transfer plug-in configuration from version 2.52 beta 1
// 44 = 2.52b2          changed viewer, editor and archiver extensions to lowercase (uppercase extensions are obsolete in Windows)
// 45 = 2.52b2          introduced password manager, forced FTP client load so it registers to use the Password Manager, see SetPluginUsesPasswordManager
// 46 = 2.52 (DB30)     only to transfer plug-in configuration from version 2.52 beta 2
// 47 = 2.52 (IB31)     support for Sal/Env variables like $(SalDir) or $[USERPROFILE] in hot paths; need to escape old hot paths
// 48 = 2.52            only to transfer plug-in configuration from version 2.52 (DB30)
// 49 = 2.53b1 (DB33)   only to transfer plug-in configuration from version 2.52
// 50 = 2.53b1 (DB36)   only to transfer plug-in configuration from version 2.53b1 (DB33)
// 51 = 2.53b1 (PB38)   only to transfer plug-in configuration from version 2.53b1 (DB36)
// 52 = 2.53b1 (DB39)   only to transfer plug-in configuration from version 2.53b1 (PB38)
// 53 = 2.53b1 (DB41)   only to transfer plug-in configuration from version 2.53b1 (DB39)
// 54 = 2.53b1 (PB44)   only to transfer plug-in configuration from version 2.53b1 (DB41)
// 55 = 2.53b1 (DB46)   only to transfer plug-in configuration from version 2.53b1 (PB44)
// 56 = 2.53b1          only to transfer plug-in configuration from version 2.53b1 (DB46)
// 57 = 2.53 (DB52)     only to transfer plug-in configuration from version 2.53b1
// 58 = 2.53b2 (IB55)   only to transfer plug-in configuration from version 2.53 (DB52)
// 59 = 2.53b2          only to transfer plug-in configuration from version 2.53b2 (IB55)
// 60 = 2.53 (DB60)     only to transfer plug-in configuration from version 2.53b2
// 61 = 2.53            only to transfer plug-in configuration from version 2.53 (DB60)
// 62 = 2.54b1 (DB66)   only to transfer plug-in configuration from version 2.53
// 63 = 2.54            only to transfer plug-in configuration from version 2.54b1 (DB66)
// 64 = 2.55b1 (DB72)   only to transfer plug-in configuration from version 2.54
// 65 = 2.55b1 (DB72)   external archivers: identify by UID instead of Title (translated according to language version, so cannot be used for identification) - switching language caused external archiver paths to be lost
// 66 = 3.00b1 (PB75)   only to transfer plug-in configuration from version 2.55b1 (DB72)
// 67 = 3.00b1 (DB76)   only to transfer plug-in configuration from version 3.00b1 (PB75)
// 68 = 3.00b1 (PB79)   only to transfer plug-in configuration from version 3.00b1 (DB76)
// 69 = 3.00b1 (DB80)   only to transfer plug-in configuration from version 3.00b1 (PB79)
// 70 = 3.00b1 (DB83)   only to transfer plug-in configuration from version 3.00b1 (DB80)
// 71 = 3.00b1 (PB87)   only to transfer plug-in configuration from version 3.00b1 (DB83)
// 72 = 3.00b1 (DB88)   only to transfer plug-in configuration from version 3.00b1 (PB87)
// 73 = 3.00b1          only to transfer plug-in configuration from version 3.00b1 (DB88)
// 74 = 3.00b2 (DB94)   only to transfer plug-in configuration from version 3.00b1
// 75 = 3.00b2          only to transfer plug-in configuration from version 3.00b2 (DB94)
// 76 = 3.00b3 (DB100)  only to transfer plug-in configuration from version 3.00b2
// 77 = 3.00b3 (PB103)  only to transfer plug-in configuration from version 3.00b3 (DB100)
// 78 = 3.00b3 (DB105)  only to transfer plug-in configuration from version 3.00b3 (PB103)
// 79 = 3.00b3          only to transfer plug-in configuration from version 3.00b3 (DB105)
// 80 = 3.00b4 (DB111)  only to transfer plug-in configuration from version 3.00b3
// 81 = 3.00b4 (DB111)  RAR 5.0 needs a new switch on the command line because of file list encoding
// 82 = 3.00b4          only to transfer plug-in configuration from version 3.00b4 (DB111)
// 83 = 3.00b5 (DB117)  only to transfer plug-in configuration from version 3.00b4
// 84 = 3.0             only to transfer plug-in configuration from version 3.00b5 (DB117)
// 85 = 3.10b1 (DB123)  only to transfer plug-in configuration from version 3.0
// 86 = 3.01            only to transfer plug-in configuration from version 3.10b1 (DB123)
// 87 = 3.10b1 (DB129)  only to transfer plug-in configuration from version 3.01
// 88 = 3.02            only to transfer plug-in configuration from version 3.10b1 (DB129)
// 89 = 3.10b1 (DB135)  only to transfer plug-in configuration from version 3.02
// 90 = 3.03            only to transfer plug-in configuration from version 3.10b1 (DB135)
// 91 = 3.10b1 (DB141)  only to transfer plug-in configuration from version 3.03
// 92 = 3.04            only to transfer plug-in configuration from version 3.10b1 (DB141)
// 93 = 3.10b1 (DB147)  only to transfer plug-in configuration from version 3.04
// 94 = 3.05            only to transfer plug-in configuration from version 3.10b1 (DB147)
// 95 = 3.10b1 (DB153)  only to transfer plug-in configuration from version 3.05
// 96 = 3.06            only to transfer plug-in configuration from version 3.10b1 (DB153)
// 97 = 3.10b1 (DB159)  only to transfer plug-in configuration from version 3.06
// 98 = 3.10b1 (DB162)  only to transfer plug-in configuration from version 3.10b1 (DB159)
// 99 = 3.07            only to transfer plug-in configuration from version 3.10b1 (DB162)
// 100 = 4.00b1 (DB168) only to transfer plug-in configuration from version 3.07
// 101 = 3.08           only to transfer plug-in configuration from version 4.00b1 (DB168) - by mistake 3.08 and DB171 share the same version number 101, sorry, I will be more careful next time
// 101 = 4.00b1 (DB171) only to transfer plug-in configuration from version 4.00b1 (DB168); this is the last VC2008 build, later versions are VC2019
// 102 = 4.00b1 (DB177) only to transfer plug-in configuration from version 4.00b1 (DB171)
// 103 = 4.00           only to transfer plug-in configuration from version 4.00b1 (DB177)
// 104 = 5.00           only to transfer plug-in configuration from version 4.00, first Open Salamander release
// 104 = 5.0-samandarin-0.1
// 105 = 5.0-samandarin-0.2
// 106 = 5.0-samandarin-0.3
// 107 = 5.0-samandarin-0.4
// 108 = 5.0-samandarin-0.5
// 109 = 5.0-samandarin-0.6
// 110 = 5.0-samandarin-0.7
// 111 = 5.0-samandarin-0.8
// 112 = 5.0-samandarin-0.9
// 113 = 5.0-samandarin-0.10
// 114 = 5.0-samandarin-0.11
// 115 = 5.0-samandarin-0.12
// 116 = 5.0-samandarin-0.13
//
// When increasing configuration version, add one to THIS_CONFIG_VERSION
//
// When upgrading to a new program version, THIS_CONFIG_VERSION must be incremented by 1
// so that new plug-ins are auto-installed and the plugins.ver counter resets.
//

const DWORD THIS_CONFIG_VERSION = 116;

// Configuration roots for individual Open Salamander versions.
// The root of the current (youngest) configuration is at index 0.
// Then follow other roots towards older versions of the program.
// The last index contains NULL and serves as a terminator when working with the array.
// When creating a new configuration version (should be stored separately in the registry from the previous one)
// simply insert the path at index 0.

// !!! Keep the corresponding lines in SalamanderConfigurationVersions up to date
const char* SalamanderConfigurationRoots[SALCFG_ROOTS_COUNT + 1] =
    {
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.13",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.12",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.11",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.10",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.9",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.8",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.7",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.6",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.5",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.4",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.3",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.2",
        "Software\\Open Salamander Samandarin\\5.0-samandarin-0.1",
        "Software\\Altap\\Altap Salamander 4.0",
        "Software\\Altap\\Altap Salamander 4.0 beta 1 (DB177)",
        "Software\\Altap\\Altap Salamander 4.0 beta 1 (DB171)",
        "Software\\Altap\\Altap Salamander 3.08",
        "Software\\Altap\\Altap Salamander 4.0 beta 1 (DB168)",
        "Software\\Altap\\Altap Salamander 3.07",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB162)",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB159)",
        "Software\\Altap\\Altap Salamander 3.06",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB153)",
        "Software\\Altap\\Altap Salamander 3.05",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB147)",
        "Software\\Altap\\Altap Salamander 3.04",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB141)",
        "Software\\Altap\\Altap Salamander 3.03",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB135)",
        "Software\\Altap\\Altap Salamander 3.02",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB129)",
        "Software\\Altap\\Altap Salamander 3.01",
        "Software\\Altap\\Altap Salamander 3.1 beta 1 (DB123)",
        "Software\\Altap\\Altap Salamander 3.0",
        "Software\\Altap\\Altap Salamander 3.0 beta 5 (DB117)",
        "Software\\Altap\\Altap Salamander 3.0 beta 4",
        "Software\\Altap\\Altap Salamander 3.0 beta 4 (DB111)",
        "Software\\Altap\\Altap Salamander 3.0 beta 3",
        "Software\\Altap\\Altap Salamander 3.0 beta 3 (DB105)",
        "Software\\Altap\\Altap Salamander 3.0 beta 3 (PB103)",
        "Software\\Altap\\Altap Salamander 3.0 beta 3 (DB100)",
        "Software\\Altap\\Altap Salamander 3.0 beta 2",
        "Software\\Altap\\Altap Salamander 3.0 beta 2 (DB94)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (DB88)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (PB87)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (DB83)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (DB80)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (PB79)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (DB76)",
        "Software\\Altap\\Altap Salamander 3.0 beta 1 (PB75)",
        "Software\\Altap\\Altap Salamander 2.55 beta 1 (DB 72)",
        "Software\\Altap\\Altap Salamander 2.54",
        "Software\\Altap\\Altap Salamander 2.54 beta 1 (DB 66)",
        "Software\\Altap\\Altap Salamander 2.53",
        "Software\\Altap\\Altap Salamander 2.53 (DB 60)",
        "Software\\Altap\\Altap Salamander 2.53 beta 2",
        "Software\\Altap\\Altap Salamander 2.53 beta 2 (IB 55)",
        "Software\\Altap\\Altap Salamander 2.53 (DB 52)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (DB 46)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (PB 44)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (DB 41)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (DB 39)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (PB 38)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (DB 36)",
        "Software\\Altap\\Altap Salamander 2.53 beta 1 (DB 33)",
        "Software\\Altap\\Altap Salamander 2.52",
        "Software\\Altap\\Altap Salamander 2.52 (DB 30)",
        "Software\\Altap\\Altap Salamander 2.52 beta 2",
        "Software\\Altap\\Altap Salamander 2.52 beta 1",
        "Software\\Altap\\Altap Salamander 2.51",
        "Software\\Altap\\Altap Salamander 2.5",
        "Software\\Altap\\Altap Salamander 2.5 RC3",
        "Software\\Altap\\Servant Salamander 2.5 RC3",
        "Software\\Altap\\Servant Salamander 2.5 RC2",
        "Software\\Altap\\Servant Salamander 2.5 RC1",
        "Software\\Altap\\Servant Salamander 2.5 beta 12",
        "Software\\Altap\\Servant Salamander 2.5 beta 11",
        "Software\\Altap\\Servant Salamander 2.5 beta 10",
        "Software\\Altap\\Servant Salamander 2.5 beta 9",
        "Software\\Altap\\Servant Salamander 2.5 beta 8",
        "Software\\Altap\\Servant Salamander 2.5 beta 7",
        "Software\\Altap\\Servant Salamander 2.5 beta 6",
        "Software\\Altap\\Servant Salamander 2.5 beta 5",
        "Software\\Altap\\Servant Salamander 2.5 beta 4",
        "Software\\Altap\\Servant Salamander 2.5 beta 3",
        "Software\\Altap\\Servant Salamander 2.5 beta 2",
        "Software\\Altap\\Servant Salamander 2.5 beta 1",
        "Software\\Altap\\Servant Salamander 2.1 beta 1",
        "Software\\Altap\\Servant Salamander 2.0",
        "Software\\Altap\\Servant Salamander 1.6 beta 7",
        "Software\\Altap\\Servant Salamander 1.6 beta 6",
        "Software\\Altap\\Servant Salamander", // 1.6 beta 1 to 1.6 beta 5
        "Software\\Salamander"                 // oldest versions (1.52 and older)
};
const char* SalamanderConfigurationVersions[SALCFG_ROOTS_COUNT] =
    {
        "5.0 Samandarin 0.13",
        "5.0 Samandarin 0.12",
        "5.0 Samandarin 0.11",
        "5.0 Samandarin 0.10",
        "5.0 Samandarin 0.9",
        "5.0 Samandarin 0.8",
        "5.0 Samandarin 0.7",
        "5.0 Samandarin 0.6",
        "5.0 Samandarin 0.5",
        "5.0 Samandarin 0.4",
        "5.0 Samandarin 0.3",
        "5.0 Samandarin 0.2",
        "5.0 Samandarin 0.1",
        "4.0",
        "4.0 beta 1 (DB177)",
        "4.0 beta 1 (DB171)",
        "3.08",
        "4.0 beta 1 (DB168)",
        "3.07",
        "3.1 beta 1 (DB162)",
        "3.1 beta 1 (DB159)",
        "3.06",
        "3.1 beta 1 (DB153)",
        "3.05",
        "3.1 beta 1 (DB147)",
        "3.04",
        "3.1 beta 1 (DB141)",
        "3.03",
        "3.1 beta 1 (DB135)",
        "3.02",
        "3.1 beta 1 (DB129)",
        "3.01",
        "3.1 beta 1 (DB123)",
        "3.0",
        "3.0 beta 5 (DB117)",
        "3.0 beta 4",
        "3.0 beta 4 (DB111)",
        "3.0 beta 3",
        "3.0 beta 3 (DB105)",
        "3.0 beta 3 (PB103)",
        "3.0 beta 3 (DB100)",
        "3.0 beta 2",
        "3.0 beta 2 (DB94)",
        "3.0 beta 1",
        "3.0 beta 1 (DB88)",
        "3.0 beta 1 (PB87)",
        "3.0 beta 1 (DB83)",
        "3.0 beta 1 (DB80)",
        "3.0 beta 1 (PB79)",
        "3.0 beta 1 (DB76)",
        "3.0 beta 1 (PB75)",
        "2.55 beta 1 (DB72)",
        "2.54",
        "2.54 beta 1 (DB66)",
        "2.53",
        "2.53 (DB60)",
        "2.53 beta 2",
        "2.53 beta 2 (IB55)",
        "2.53 (DB52)",
        "2.53 beta 1",
        "2.53 beta 1 (DB46)",
        "2.53 beta 1 (PB44)",
        "2.53 beta 1 (DB41)",
        "2.53 beta 1 (DB39)",
        "2.53 beta 1 (PB38)",
        "2.53 beta 1 (DB36)",
        "2.53 beta 1 (DB33)",
        "2.52",
        "2.52 (DB30)",
        "2.52 beta 2",
        "2.52 beta 1",
        "2.51",
        "2.5",
        "2.5 RC3",
        "2.5 RC3",
        "2.5 RC2",
        "2.5 RC1",
        "2.5 beta 12",
        "2.5 beta 11",
        "2.5 beta 10",
        "2.5 beta 9",
        "2.5 beta 8",
        "2.5 beta 7",
        "2.5 beta 6",
        "2.5 beta 5",
        "2.5 beta 4",
        "2.5 beta 3",
        "2.5 beta 2",
        "2.5 beta 1",
        "2.1 beta 1",
        "2.0",
        "1.6 beta 7",
        "1.6 beta 6",
        "1.6 beta 1-5",
        "1.52"};

const char* SALAMANDER_ROOT_REG = NULL; // will be set in salamdr1.cpp

const char* SALAMANDER_SAVE_IN_PROGRESS = "Save In Progress"; // value exists only during configuration save (detects interrupted saves -> corrupted configuration)
BOOL IsSetSALAMANDER_SAVE_IN_PROGRESS = FALSE;                // TRUE = the registry contains SALAMANDER_SAVE_IN_PROGRESS (detect interrupted configuration saving)

const char* SALAMANDER_COPY_IS_OK = "Copy Is OK"; // backup key only: value exists only if the key was copied completely

const char* SALAMANDER_AUTO_IMPORT_CONFIG = "AutoImportConfig"; // value exists only during upgrade: installer overwrites the old version with the new and stores this value pointing to the old configuration key from which the configuration should be imported

const char* FINDDIALOG_WINDOW_REG = "Find Dialog Window";
const char* SALAMANDER_WINDOW_REG = "Window";
const char* WINDOW_LEFT_REG = "Left";
const char* WINDOW_RIGHT_REG = "Right";
const char* WINDOW_TOP_REG = "Top";
const char* WINDOW_BOTTOM_REG = "Bottom";
const char* WINDOW_SPLIT_REG = "Split Position";
const char* WINDOW_BEFOREZOOMSPLIT_REG = "Before Zoom Split Position";
const char* WINDOW_SHOW_REG = "Show";
const char* WINDOW_DETACHED_PANELS_REG = "Detached Panels";
const char* WINDOW_DETACHED_LEFT_REG = "Detached Left";
const char* WINDOW_DETACHED_RIGHT_REG = "Detached Right";
const char* WINDOW_DETACHED_TOP_REG = "Detached Top";
const char* WINDOW_DETACHED_BOTTOM_REG = "Detached Bottom";
const char* WINDOW_DETACHED_SHOW_REG = "Detached Show";
const char* FINDDIALOG_NAMEWIDTH_REG = "Name Width";
const char* SALAMANDER_AUTOCONFIGDRIVES_REG = "Autoconfig Search Paths";

const char* SALAMANDER_LEFTP_REG = "Left Panel";
const char* SALAMANDER_RIGHTP_REG = "Right Panel";
const char* PANEL_PATH_REG = "Path";
const char* PANEL_VIEW_REG = "View Type";
const char* PANEL_SORT_REG = "Sort Type";
const char* PANEL_SORT_CUSTOM_DATA_REG = "Sort Custom Data";
const char* PANEL_REVERSE_REG = "Reverse Sort";
const char* PANEL_DIRLINE_REG = "Directory Line";
const char* PANEL_STATUS_REG = "Status Line";
const char* PANEL_HEADER_REG = "Header Line";
const char* PANEL_FILTER_ENABLE = "Enable Filter";
const char* PANEL_FILTER_INVERSE = "Inverse Filter";
const char* PANEL_FILTERHISTORY_REG = "Filter History";
const char* PANEL_FILTER = "Filter";
const char* PANEL_TABCOUNT_REG = "Tab Count";
const char* PANEL_ACTIVETAB_REG = "Active Tab";
const char* PANEL_TABCOLOR_REG = "Tab Color";
const char* PANEL_TABPREFIX_REG = "Tab Prefix";
const char* PANEL_TABLOCKED_REG = "Tab Locked";

const char* SALAMANDER_DEFDIRS_REG = "Default Directories";

const char* SALAMANDER_CONFIG_REG = "Configuration";
const char* CONFIG_SKILLLEVEL_REG = "Skill Level";
const char* CONFIG_FILENAMEFORMAT_REG = "File Name Format";
const char* CONFIG_SIZEFORMAT_REG = "Size Format";
const char* CONFIG_SELECTION_REG = "Select/Deselect Directories";
const char* CONFIG_LONGNAMES_REG = "Use Long File Names";
const char* CONFIG_RECYCLEBIN_REG = "Use Recycle Bin";
const char* CONFIG_RECYCLEMASKS_REG = "Use Recycle Bin For";
const char* CONFIG_SAVEONEXIT_REG = "Save Configuration On Exit";
const char* CONFIG_SHOWGREPERRORS_REG = "Show Errors In Find Files";
const char* CONFIG_FINDFULLROW_REG = "Show Full Row In Find Files";
const char* CONFIG_MINBEEPWHENDONE_REG = "Use Speeker Beep";
const char* CONFIG_INTRN_VIEWER_REG = "Internal Viewer";
const char* CONFIG_VIEWER_REG = "External Viewer";
const char* CONFIG_EDITOR_REG = "External Editor";
const char* CONFIG_CMDLINE_REG = "Command Line";
const char* CONFIG_CMDLFOCUS_REG = "Command Line Focused";
const char* CONFIG_CLOSESHELL_REG = "Close Shell Window";
const char* CONFIG_COMMANDLINEAPP_REG = "Command Line Application";
const char* CONFIG_COMMANDLINEARGS_REG = "Command Line Arguments";
const char* CONFIG_USECUSTOMPANELFONT_REG = "Use Custom Panel Font";
const char* CONFIG_PANELFONT_REG = "Panel Font";
const char* CONFIG_NAMEDHISTORY_REG = "Named History";
const char* CONFIG_LOOKINHISTORY_REG = "Look In History";
const char* CONFIG_GREPHISTORY_REG = "Grep History";
const char* CONFIG_VIEWERHISTORY_REG = "Viewer History";
const char* CONFIG_COMMANDHISTORY_REG = "Command History";
const char* CONFIG_SELECTHISTORY_REG = "Select History";
const char* CONFIG_COPYHISTORY_REG = "Copy History";
const char* CONFIG_CHANGEDIRHISTORY_REG = "ChangeDir History";
const char* CONFIG_FILELISTHISTORY_REG = "File List History";
const char* CONFIG_CREATEDIRHISTORY_REG = "Create Directory History";
const char* CONFIG_QUICKRENAMEHISTORY_REG = "Quick Rename History";
const char* CONFIG_EDITNEWHISTORY_REG = "Edit New History";
const char* CONFIG_CONVERTHISTORY_REG = "Convert History";
const char* CONFIG_FILTERHISTORY_REG = "Filter History";
const char* CONFIG_WORKDIRSHISTORY_REG = "Working Directories";
const char* CONFIG_FILELISTNAME_REG = "Make File List Name";
const char* CONFIG_FILELISTAPPEND_REG = "Make File List Append";
const char* CONFIG_FILELISTDESTINATION_REG = "Make File List Destination";
const char* CONFIG_COPYFINDTEXT_REG = "Copy Find Text";
const char* CONFIG_CLEARREADONLY_REG = "Clear Readonly Attribute";
const char* CONFIG_PRIMARYCONTEXTMENU_REG = "Primary Context Menu";
const char* CONFIG_NOTHIDDENSYSTEM_REG = "Hide Hidden and System Files and Directories";
const char* CONFIG_RIGHT_FOCUS_REG = "Right Panel Focused";
const char* CONFIG_SHOWCHDBUTTON_REG = "Show Change Drive Button";
const char* CONFIG_ALWAYSONTOP_REG = "Always On Top";
//const char *CONFIG_FASTDIRMOVE_REG = "Fast Directory Move";
const char* CONFIG_SORTUSESLOCALE_REG = "Sort Uses Locale";
const char* CONFIG_SORTDETECTNUMBERS_REG = "Sort Detects Numbers";
const char* CONFIG_SORTNEWERONTOP_REG = "Sort Newer On Top";
const char* CONFIG_SORTDIRSBYNAME_REG = "Sort Dirs By Name";
const char* CONFIG_SORTDIRSBYEXT_REG = "Sort Dirs By Ext";
const char* CONFIG_SAVEHISTORY_REG = "Save History";
const char* CONFIG_SAVEWORKDIRS_REG = "Save Working Dirs";
const char* CONFIG_WORKDIRS_HISTORY_SCOPE_REG = "Working Dirs History Scope";
const char* CONFIG_ENABLECMDLINEHISTORY_REG = "Enable CmdLine History";
const char* CONFIG_SAVECMDLINEHISTORY_REG = "Save CmdLine History";
const char* CONFIG_BACKSPACEACTION_REG = "Backspace Action";
//const char *CONFIG_LANTASTICCHECK_REG = "Lantastic Check";
const char* CONFIG_USESALOPEN_REG = "Use salopen.exe";
const char* CONFIG_NETWAREFASTDIRMOVE_REG = "Netware Fast Dir Move";
const char* CONFIG_ASYNCCOPYALG_REG = "Async Copy Alg On Network";
const char* CONFIG_RELOAD_ENV_VARS_REG = "Reload Environment Variables";
const char* CONFIG_PATH_AUTOCOMPLETE_REG = "Path Auto Complete";
const char* CONFIG_CREATEDIR_AUTOCOMPLETE_REG = "Create Directory Auto Complete";
const char* CONFIG_QUICKRENAME_SELALL_REG = "Quick Rename Select All";
const char* CONFIG_EDITNEW_SELALL_REG = "Edit New File Select All";
const char* CONFIG_SHIFTFORHOTPATHS_REG = "Use Shift For GoTo HotPath";
const char* CONFIG_ONLYONEINSTANCE_REG = "Only One Instance";
const char* CONFIG_STATUSAREA_REG = "Status Area";
const char* CONFIG_SINGLECLICK_REG = "Single Click";
//const char *CONFIG_SHOWTIPOFTHEDAY_REG = "Show tip of the Day";
//const char *CONFIG_LASTTIPOFTHEDAY_REG = "Last tip of the Day";
const char* CONFIG_TOPTOOLBAR_REG = "Top ToolBar";
const char* CONFIG_MIDDLETOOLBAR_REG = "Middle ToolBar";
const char* CONFIG_LEFTTOOLBAR_REG = "Left ToolBar";
const char* CONFIG_RIGHTTOOLBAR_REG = "Right ToolBar";
const char* CONFIG_TOPTOOLBARVISIBLE_REG = "Show Top ToolBar";
const char* CONFIG_PLGTOOLBARVISIBLE_REG = "Show Plugins Bar";
const char* CONFIG_EXTENSIONBARVISIBLE_REG = "Show Extension Bar";
const char* CONFIG_MIDDLETOOLBARVISIBLE_REG = "Show Middle ToolBar";
const char* CONFIG_USERMENUTOOLBARVISIBLE_REG = "Show User Menu ToolBar";
const char* CONFIG_HOTPATHSBARVISIBLE_REG = "Hot Paths Bar";
const char* CONFIG_DRIVEBARVISIBLE_REG = "Show Drive Bar";
const char* CONFIG_DRIVEBAR2VISIBLE_REG = "Show Drive Bar2";
const char* CONFIG_TREEVIEWVISIBLE_REG = "Show Tree View";
const char* CONFIG_PANELTOOLTIPS_REG = "Show Panel Tooltips";
const char* CONFIG_BOTTOMTOOLBARVISIBLE_REG = "Show Bottom ToolBar";
const char* CONFIG_EXPLORERLOOK_REG = "Explorer Look";
const char* CONFIG_FULLROWSELECT_REG = "Full Row Select";
const char* CONFIG_FULLROWHIGHLIGHT_REG = "Full Row Highlight";
const char* CONFIG_USEICONTINCTURE_REG = "Use Icon Tincture";
const char* CONFIG_PANELS_USETABS_REG = "Use Panel Tabs";
const char* CONFIG_SHOWPANELCAPTION_REG = "Show Panel Caption";
const char* CONFIG_SHOWPANELZOOM_REG = "Show Panel Zoom";
const char* CONFIG_INFOLINECONTENT_REG = "Information Line Content";
const char* CONFIG_IFPATHISINACCESSIBLEGOTOISMYDOCS_REG = "If Path Is Inaccessible Go To My Docs";
const char* CONFIG_IFPATHISINACCESSIBLEGOTO_REG = "If Path Is Inaccessible Go To";
const char* CONFIG_HOTPATH_AUTOCONFIG = "Auto Configurate Hot Paths";
const char* CONFIG_LASTUSEDSPEEDLIM_REG = "Speed Limit";
const char* CONFIG_QUICKSEARCHENTER_REG = "Quick Search Enter Alt";
const char* CONFIG_CHD_SHOWMOUNTFOLDERS = "Change Drive Show Mount Folders";
const char* CONFIG_CHD_MOUNTFOLDERS_MODE = "Change Drive Mount Folders Mode";
const char* CONFIG_CHD_MOUNTFOLDERS_NAME = "Change Drive Mount Folders Name";
const char* CONFIG_CHD_MOUNTFOLDERS_DRIVEBAR = "Change Drive Mount Folders Drive Bar";
const char* CONFIG_CHD_SHOWMYDOC = "Change Drive Show My Documents";
const char* CONFIG_CHD_SHOW3DOBJECTS = "Change Drive Show 3D Objects";
const char* CONFIG_CHD_SHOWDESKTOP = "Change Drive Show Desktop";
const char* CONFIG_CHD_SHOWDOWNLOADS = "Change Drive Show Downloads";
const char* CONFIG_CHD_SHOWMUSIC = "Change Drive Show Music";
const char* CONFIG_CHD_SHOWPICTURES = "Change Drive Show Pictures";
const char* CONFIG_CHD_SHOWVIDEOS = "Change Drive Show Videos";
const char* CONFIG_CHD_SHOWANOTHER = "Change Drive Show Another";
const char* CONFIG_CHD_SHOWCLOUDSTOR = "Change Drive Show Cloud Storages";
const char* CONFIG_CHD_SHOWNET = "Change Drive Network";
const char* CONFIG_CURRRENTTIPINDEX = "Current Tip Index";
const char* CONFIG_SEARCHFILECONTENT = "Search File Content";
const char* CONFIG_FINDOPTIONS_REG = "Find Options";
const char* CONFIG_FINDIGNORE_REG = "Find Ignore";
#ifdef _WIN64
const char* CONFIG_LASTPLUGINVER = "Plugins.ver Version (x64)";
const char* CONFIG_LASTPLUGINVER_OP = "Plugins.ver Version (x86)";
#else  // _WIN64
const char* CONFIG_LASTPLUGINVER = "Plugins.ver Version (x86)";
const char* CONFIG_LASTPLUGINVER_OP = "Plugins.ver Version (x64)";
#endif // _WIN64
const char* CONFIG_LANGUAGE_REG = "Language";
const char* CONFIG_SHOWSPLASHSCREEN_REG = "Show Splash Screen";
const char* CONFIG_CONVERSIONTABLE_REG = "Conversion Table";
const char* CONFIG_TITLEBARSHOWPATH_REG = "Title bar show path";
const char* CONFIG_TITLEBARMODE_REG = "Title bar mode";
const char* CONFIG_TABCAPTIONMODE_REG = "Tab caption mode";
const char* CONFIG_TABCAPTIONALIGNMENT_REG = "Tab caption alignment";
const char* CONFIG_TABMINWIDTH_REG = "Tab min width";
const char* CONFIG_TABMAXWIDTH_REG = "Tab max width";
const char* CONFIG_TABACTIVEBORDER_REG = "Tab active border";
const char* CONFIG_TABACTIVEBORDERCOLOR_REG = "Tab active border color";
const char* CONFIG_TABCLOSEBUTTONACTIVE_REG = "Tab close button active";
const char* CONFIG_TABCLOSEBUTTONALL_REG = "Tab close button all";
const char* CONFIG_TITLEBARPREFIX_REG = "Title bar prefix";
const char* CONFIG_TITLEBARPREFIXTEXT_REG = "Title bar prefix text";
const char* CONFIG_MAINWINDOWICONINDEX_REG = "Main window icon index";
const char* CONFIG_CLICKQUICKRENAME_REG = "Click to Quick Rename";
const char* CONFIG_VISIBLEDRIVES_REG = "Visible Drives";
const char* CONFIG_SEPARATEDDRIVES_REG = "Separated Drives";

const char* CONFIG_COMPAREBYTIME_REG = "Compare By Time";
const char* CONFIG_COMPAREBYSIZE_REG = "Compare By Size";
const char* CONFIG_COMPAREBYCONTENT_REG = "Compare By Content";
const char* CONFIG_COMPAREBYATTR_REG = "Compare By Attr";
const char* CONFIG_COMPAREBYSUBDIRS_REG = "Compare By Subdirs";
const char* CONFIG_COMPAREBYSUBDIRSATTR_REG = "Compare By Subdirs Attr";
const char* CONFIG_COMPAREONEPANELDIRS_REG = "Compare One Panel Dirs";
const char* CONFIG_COMPAREMOREOPTIONS_REG = "Compare More Options";
const char* CONFIG_COMPAREIGNOREFILES_REG = "Compare Ignore Files";
const char* CONFIG_COMPAREIGNOREDIRS_REG = "Compare Ignore Dirs";
const char* CONFIG_CONFIGTIGNOREFILESMASKS_REG = "Compare Ignore Files Masks";
const char* CONFIG_CONFIGTIGNOREDIRSMASKS_REG = "Compare Ignore Dirs Masks";
const char* CONFIG_THUMBNAILSIZE_REG = "Thumbnail Size";
const char* CONFIG_ALTLANGFORPLUGINS_REG = "Alternate Language for Plugins";
const char* CONFIG_USEALTLANGFORPLUGINS_REG = "Use Alternate Language for Plugins";
const char* CONFIG_LANGUAGECHANGED_REG = "Language Changed";
const char* CONFIG_ENABLECUSTICOVRLS_REG = "Enable Custom Icon Overlays";
const char* CONFIG_DISABLEDCUSTICOVRLS_REG = "Disabled Custom Icon Overlays";
const char* CONFIG_COPYMOVEOPTIONS_REG = "Copy Move Options";
const char* CONFIG_KEEPPLUGINSSORTED_REG = "Keep Plugins Sorted";
const char* CONFIG_SHOWSLGINCOMPLETE_REG = "Show Translation Is Incomplete";

const char* CONFIG_EDITNEWFILE_USEDEFAULT_REG = "Edit New File Use Default";
const char* CONFIG_EDITNEWFILE_DEFAULT_REG = "Edit New File Default";

//const char *CONFIG_SPACESELCALCSPACE = "Space Selecting";
const char* CONFIG_COUNTSIZESTAYONFILESYSTEM = "Count Size Stay On File System";
const char* CONFIG_USETIMERESOLUTION = "Use Time Resolution";
const char* CONFIG_TIMERESOLUTION = "Time Resolution";
const char* CONFIG_IGNOREDSTSHIFTS = "Ignore DST Shifts";

const char* CONFIG_USEDRAGDROPMINTIME = "Use DragDrop Min Time";
const char* CONFIG_DRAGDROPMINTIME = "DragDrop Min Time";

// configuration dialog pages
const char* CONFIG_LASTFOCUSEDPAGE = "Last Focused Page";
const char* CONFIG_VIEWANDEDITEXPAND = "Viewers And Editors Expanded";
const char* CONFIG_PACKEPAND = "Packers And Unpackers Expanded";
const char* CONFIG_CONFIGURATION_HEIGHT = "Configuration Height";
const char* CONFIG_CONFIGURATION_WIDTH = "Configuration Width";
const char* CONFIG_CONFIGURATION_TREE_WIDTH = "Configuration Tree Width";
const char* CONFIG_CONFIGURATION_VIEWS_RIGHT_WIDTH = "Configuration Views Right Width";

const char* CONFIG_MENUINDEX_REG = "Menu Index";
const char* CONFIG_MENUBREAK_REG = "Menu Break";
const char* CONFIG_MENUWIDTH_REG = "Menu Width";
const char* CONFIG_TOOLBARINDEX_REG = "ToolBar Index";
const char* CONFIG_TOOLBARBREAK_REG = "ToolBar Break";
const char* CONFIG_TOOLBARWIDTH_REG = "ToolBar Width";
const char* CONFIG_PLUGINSBARINDEX_REG = "PluginsBar Index";
const char* CONFIG_PLUGINSBARBREAK_REG = "PluginsBar Break";
const char* CONFIG_PLUGINSBARWIDTH_REG = "PluginsBar Width";
const char* CONFIG_EXTENSIONBARINDEX_REG = "ExtensionBar Index";
const char* CONFIG_EXTENSIONBARBREAK_REG = "ExtensionBar Break";
const char* CONFIG_EXTENSIONBARWIDTH_REG = "ExtensionBar Width";
const char* CONFIG_USERMENUINDEX_REG = "User Menu Index";
const char* CONFIG_USERMENUBREAK_REG = "User Menu Break";
const char* CONFIG_USERMENUWIDTH_REG = "User Menu Width";
const char* CONFIG_USERMENULABELS_REG = "User Menu Labels";
const char* CONFIG_HOTPATHSINDEX_REG = "Hot Paths Index";
const char* CONFIG_HOTPATHSBREAK_REG = "Hot Paths Break";
const char* CONFIG_HOTPATHSWIDTH_REG = "Hot Paths Width";
const char* CONFIG_DRIVEBARINDEX_REG = "Drive Bar Index";
const char* CONFIG_DRIVEBARBREAK_REG = "Drive Bar Break";
const char* CONFIG_DRIVEBARWIDTH_REG = "Drive Bar Width";
const char* CONFIG_TREEVIEWWIDTH_REG = "Tree View Width";
const char* CONFIG_TREEVIEWAUTOHIDE_REG = "Tree View Auto Hide";
const char* CONFIG_DETACHEDTREEVIEWWIDTH_REG = "Detached Tree View Width";
const char* CONFIG_DETACHEDTREEVIEWAUTOHIDE_REG = "Detached Tree View Auto Hide";
const char* CONFIG_GRIPSVISIBLE_REG = "Grips Visible";

const char* SALAMANDER_CONFIRMATION_REG = "Confirmation";
const char* CONFIG_CNFRM_FILEDIRDEL = "Files or Dirs Del";
const char* CONFIG_CNFRM_NEDIRDEL = "Non-empty Dir Del";
const char* CONFIG_CNFRM_FILEOVER = "File Overwrite";
const char* CONFIG_CNFRM_DIROVER = "Directory Overwrite";
const char* CONFIG_CNFRM_SHFILEDEL = "SH File Del";
const char* CONFIG_CNFRM_SHDIRDEL = "SH Dir Del";
const char* CONFIG_CNFRM_SHFILEOVER = "SH File Overwrite";
const char* CONFIG_CNFRM_NTFSPRESS = "NTFS Compress and Uncompress";
const char* CONFIG_CNFRM_NTFSCRYPT = "NTFS Encrypt and Decrypt";
const char* CONFIG_CNFRM_DAD = "Drag and Drop";
const char* CONFIG_CNFRM_CLOSEARCHIVE = "Close Archive";
const char* CONFIG_CNFRM_CLOSEFIND = "Close Find";
const char* CONFIG_CNFRM_STOPFIND = "Stop Find";
const char* CONFIG_CNFRM_CREATETARGETPATH = "Create Target Path";
const char* CONFIG_CNFRM_ALWAYSONTOP = "Always on Top";
const char* CONFIG_CNFRM_ONSALCLOSE = "Close Salamander";
const char* CONFIG_CNFRM_DETACHCLOSE = "Close Detached Window";
const char* CONFIG_CNFRM_SENDEMAIL = "Send Email";
const char* CONFIG_CNFRM_ADDTOARCHIVE = "Add To Archive";
const char* CONFIG_CNFRM_CREATEDIR = "Create Dir";
const char* CONFIG_CNFRM_CHANGEDIRTC = "Change Dir TC";
const char* CONFIG_CNFRM_SHOWNAMETOCOMP = "Show Names To Compare";
const char* CONFIG_CNFRM_DSTSHIFTSIGNORED = "DST Shifts Ignored";
const char* CONFIG_CNFRM_DSTSHIFTSOCCURED = "DST Shifts Occured";
const char* CONFIG_CNFRM_COPYMOVEOPTIONSNS = "Copy Move Options Not Supported";
const char* CONFIG_CNFRM_CHANGEDIRHISTORYERR = "Change Dir History Error";
const char* CONFIG_CNFRM_CONFIRMDELETEEXTINFO = "Confirm Delete Extended Info";

const char* SALAMANDER_DRVSPEC_REG = "Drive Special Settings";
const char* CONFIG_DRVSPEC_FLOPPY_MON = "Floppy Automatic Refresh";
const char* CONFIG_DRVSPEC_FLOPPY_SIMPLE = "Floppy Simple Icons";
const char* CONFIG_DRVSPEC_REMOVABLE_MON = "Removable Automatic Refresh";
const char* CONFIG_DRVSPEC_REMOVABLE_SIMPLE = "Removable Simple Icons";
const char* CONFIG_DRVSPEC_FIXED_MON = "Fixed Automatic Refresh";
const char* CONFIG_DRVSPEC_FIXED_SIMPLE = "Fixed Simple Icons";
const char* CONFIG_DRVSPEC_REMOTE_MON = "Remote Automatic Refresh";
const char* CONFIG_DRVSPEC_REMOTE_SIMPLE = "Remote Simple Icons";
const char* CONFIG_DRVSPEC_REMOTE_ACT = "Remote Do Not Refresh on Activation";
const char* CONFIG_DRVSPEC_CDROM_MON = "CDROM Automatic Refresh";
const char* CONFIG_DRVSPEC_CDROM_SIMPLE = "CDROM Simple Icons";

const char* SALAMANDER_HOTPATHS_REG = "Hot Paths";

const char* SALAMANDER_VIEWTEMPLATES_REG = "View Templates";

const char* SALAMANDER_VIEWER_REG = "Viewer";
const char* VIEWER_FINDFORWARD_REG = "Forward Direction";
const char* VIEWER_FINDWHOLEWORDS_REG = "Whole Words";
const char* VIEWER_FINDCASESENSITIVE_REG = "Case Sensitive";
const char* VIEWER_FINDTEXT_REG = "Find Text";
const char* VIEWER_FINDHEXMODE_REG = "HEX-mode";
const char* VIEWER_FINDREGEXP_REG = "Regular Expression";
const char* VIEWER_CONFIGCRLF_REG = "EOL CRLF";
const char* VIEWER_CONFIGCR_REG = "EOL CR";
const char* VIEWER_CONFIGLF_REG = "EOL LF";
const char* VIEWER_CONFIGNULL_REG = "EOL NULL";
const char* VIEWER_CONFIGTABSIZE_REG = "Tabelator Size";
const char* VIEWER_CONFIGDEFMODE_REG = "Default Mode";
const char* VIEWER_CONFIGTEXTMASK_REG = "Text Masks";
const char* VIEWER_CONFIGHEXMASK_REG = "Hex Masks";
const char* VIEWER_CONFIGUSECUSTOMFONT_REG = "Viewer Use Custom Font";
const char* VIEWER_CONFIGFONT_REG = "Viewer Font";
const char* VIEWER_WRAPTEXT_REG = "Wrap Text";
const char* VIEWER_SHOWNUMBERS_REG = "Show Line Numbers";
const char* VIEWER_SHOWSTATUS_REG = "Show Status Bar";
const char* VIEWER_ZOOMPERCENT_REG = "Zoom Percent";
const char* VIEWER_CPAUTOSELECT_REG = "Auto-Select";
const char* VIEWER_DEFAULTCONVERT_REG = "Default Convert";
const char* VIEWER_AUTOCOPYSELECTION_REG = "Auto-Copy Selection";
const char* VIEWER_GOTOOFFSETISHEX_REG = "Go to Offset Is Hex";

const char* VIEWER_CONFIGSAVEWINPOS_REG = "Save Window Position";
const char* VIEWER_CONFIGWNDLEFT_REG = "Left";
const char* VIEWER_CONFIGWNDRIGHT_REG = "Right";
const char* VIEWER_CONFIGWNDTOP_REG = "Top";
const char* VIEWER_CONFIGWNDBOTTOM_REG = "Bottom";
const char* VIEWER_CONFIGWNDSHOW_REG = "Show";

const char* SALAMANDER_USERMENU_REG = "User Menu";
const char* USERMENU_ITEMNAME_REG = "Item Name";
const char* USERMENU_COMMAND_REG = "Command";
const char* USERMENU_ARGUMENTS_REG = "Arguments";
const char* USERMENU_INITDIR_REG = "Initial Directory";
const char* USERMENU_SHELL_REG = "Execute Through Shell";
const char* USERMENU_USEWINDOW_REG = "Open Shell Window";
const char* USERMENU_CLOSE_REG = "Close Shell Window";
const char* USERMENU_SEPARATOR_REG = "Separator";
const char* USERMENU_SHOWINTOOLBAR_REG = "Show In Toolbar";
const char* USERMENU_TYPE_REG = "Type";
const char* USERMENU_ICON_REG = "Icon";

const char* SALAMANDER_VIEWERS_REG = "Viewers";
const char* SALAMANDER_ALTVIEWERS_REG = "Alternative Viewers";
const char* VIEWERS_MASKS_REG = "Masks";
const char* VIEWERS_COMMAND_REG = USERMENU_COMMAND_REG;
const char* VIEWERS_ARGUMENTS_REG = USERMENU_ARGUMENTS_REG;
const char* VIEWERS_INITDIR_REG = USERMENU_INITDIR_REG;
const char* VIEWERS_TYPE_REG = "Type";

const char* SALAMANDER_IZIP_REG = "Internal ZIP Packer";

const char* SALAMANDER_EDITORS_REG = "Editors";
const char* EDITORS_MASKS_REG = VIEWERS_MASKS_REG;
const char* EDITORS_COMMAND_REG = USERMENU_COMMAND_REG;
const char* EDITORS_ARGUMENTS_REG = USERMENU_ARGUMENTS_REG;
const char* EDITORS_INITDIR_REG = USERMENU_INITDIR_REG;

const char* SALAMANDER_VERSION_REG = "Version";
const char* SALAMANDER_VERSIONREG_REG = "Configuration";

const char* SALAMANDER_CUSTOMCOLORS_REG = "Custom Colors";

// colors
const char* SALAMANDER_COLORS_REG = "Colors";
const char* SALAMANDER_CLR_FOCUS_ACTIVE_NORMAL_REG = "Focus Active Normal";
const char* SALAMANDER_CLR_FOCUS_ACTIVE_SELECTED_REG = "Focus Active Selected";
const char* SALAMANDER_CLR_FOCUS_INACTIVE_NORMAL_REG = "Focus Inactive Normal";
const char* SALAMANDER_CLR_FOCUS_INACTIVE_SELECTED_REG = "Focus Inactive Selected";
const char* SALAMANDER_CLR_FOCUS_BK_INACTIVE_NORMAL_REG = "Focus Bk Inactive Normal";
const char* SALAMANDER_CLR_FOCUS_BK_INACTIVE_SELECTED_REG = "Focus Bk Inactive Selected";

const char* SALAMANDER_CLR_ITEM_FG_NORMAL_REG = "Item Fg Normal";
const char* SALAMANDER_CLR_ITEM_FG_SELECTED_REG = "Item Fg Selected";
const char* SALAMANDER_CLR_ITEM_FG_FOCUSED_REG = "Item Fg Focused";
const char* SALAMANDER_CLR_ITEM_FG_FOCSEL_REG = "Item Fg Focused and Selected";
const char* SALAMANDER_CLR_ITEM_FG_HIGHLIGHT_REG = "Item Fg Highlight";

const char* SALAMANDER_CLR_ITEM_BK_NORMAL_REG = "Item Bk Normal";
const char* SALAMANDER_CLR_ITEM_BK_SELECTED_REG = "Item Bk Selected";
const char* SALAMANDER_CLR_ITEM_BK_FOCUSED_REG = "Item Bk Focused";
const char* SALAMANDER_CLR_ITEM_BK_FOCSEL_REG = "Item Bk Focused and Selected";
const char* SALAMANDER_CLR_ITEM_BK_HIGHLIGHT_REG = "Item Bk Highlight";

const char* SALAMANDER_CLR_ICON_BLEND_SELECTED_REG = "Icon Blend Selected";
const char* SALAMANDER_CLR_ICON_BLEND_FOCUSED_REG = "Icon Blend Focused";
const char* SALAMANDER_CLR_ICON_BLEND_FOCSEL_REG = "Icon Blend Focused and Selected";

const char* SALAMANDER_CLR_PROGRESS_FG_NORMAL_REG = "Progress Fg Normal";
const char* SALAMANDER_CLR_PROGRESS_FG_SELECTED_REG = "Progress Fg Selected";
const char* SALAMANDER_CLR_PROGRESS_BK_NORMAL_REG = "Progress Bk Normal";
const char* SALAMANDER_CLR_PROGRESS_BK_SELECTED_REG = "Progress Bk Selected";

const char* SALAMANDER_CLR_VIEWER_FG_NORMAL_REG = "Viewer Fg Normal";
const char* SALAMANDER_CLR_VIEWER_BK_NORMAL_REG = "Viewer Bk Normal";
const char* SALAMANDER_CLR_VIEWER_FG_SELECTED_REG = "Viewer Fg Selected";
const char* SALAMANDER_CLR_VIEWER_BK_SELECTED_REG = "Viewer Bk Selected";

const char* SALAMANDER_CLR_HOT_PANEL_REG = "Hot Panel";
const char* SALAMANDER_CLR_HOT_ACTIVE_REG = "Hot Active";
const char* SALAMANDER_CLR_HOT_INACTIVE_REG = "Hot Inactive";

const char* SALAMANDER_CLR_ACTIVE_CAPTION_FG_REG = "Active Caption Fg";
const char* SALAMANDER_CLR_ACTIVE_CAPTION_BK_REG = "Active Caption Bk";
const char* SALAMANDER_CLR_INACTIVE_CAPTION_FG_REG = "Inactive Caption Fg";
const char* SALAMANDER_CLR_INACTIVE_CAPTION_BK_REG = "Inactive Caption Bk";

const char* SALAMANDER_CLR_THUMBNAIL_FRAME_NORMAL_REG = "Thumbnail Frame Normal";
const char* SALAMANDER_CLR_THUMBNAIL_FRAME_SELECTED_REG = "Thumbnail Frame Selected";
const char* SALAMANDER_CLR_THUMBNAIL_FRAME_FOCUSED_REG = "Thumbnail Frame Focused";
const char* SALAMANDER_CLR_THUMBNAIL_FRAME_FOCSEL_REG = "Thumbnail Frame Focused and Selected";

const char* SALAMANDER_CLR_AUTOCOMPLETE_PATH_FG_REG = "Autocomplete Path Fg";
const char* SALAMANDER_CLR_AUTOCOMPLETE_PATH_BK_REG = "Autocomplete Path Bk";
const char* SALAMANDER_CLR_AUTOCOMPLETE_LIST_FG_REG = "Autocomplete List Fg";
const char* SALAMANDER_CLR_AUTOCOMPLETE_LIST_BK_REG = "Autocomplete List Bk";

const char* SALAMANDER_HLT = "Panel Items Hilighting";
const char* SALAMANDER_HLT_ITEM_MASKS = "Masks";
const char* SALAMANDER_HLT_ITEM_ATTR = "Attributes";
const char* SALAMANDER_HLT_ITEM_VALIDATTR = "Valid Attributes";
const char* SALAMANDER_HLT_ITEM_FG_NORMAL_REG = "Item Fg Normal";
const char* SALAMANDER_HLT_ITEM_FG_SELECTED_REG = "Item Fg Selected";
const char* SALAMANDER_HLT_ITEM_FG_FOCUSED_REG = "Item Fg Focused";
const char* SALAMANDER_HLT_ITEM_FG_FOCSEL_REG = "Item Fg Focused and Selected";
const char* SALAMANDER_HLT_ITEM_FG_HIGHLIGHT_REG = "Item Fg Highlight";
const char* SALAMANDER_HLT_ITEM_BK_NORMAL_REG = "Item Bk Normal";
const char* SALAMANDER_HLT_ITEM_BK_SELECTED_REG = "Item Bk Selected";
const char* SALAMANDER_HLT_ITEM_BK_FOCUSED_REG = "Item Bk Focused";
const char* SALAMANDER_HLT_ITEM_BK_FOCSEL_REG = "Item Bk Focused and Selected";
const char* SALAMANDER_HLT_ITEM_BK_HIGHLIGHT_REG = "Item Bk Highlight";

const char* SALAMANDER_CLRSCHEME_REG = "Color Scheme";
const char* SALAMANDER_CLR_USE_WIN_DARK_REG = "Use Windows Dark Mode";

// Plugins
const char* SALAMANDER_PLUGINS = "Plugins";
const char* SALAMANDER_PLUGINS_NAME = "Name";
const char* SALAMANDER_PLUGINS_DLLNAME = "DLL";
const char* SALAMANDER_PLUGINS_VERSION = "Version";
const char* SALAMANDER_PLUGINS_COPYRIGHT = "Copyright";
const char* SALAMANDER_PLUGINS_EXTENSIONS = "Extensions";
const char* SALAMANDER_PLUGINS_DESCRIPTION = "Description";
const char* SALAMANDER_PLUGINS_LASTSLGNAME = "LastSLGName";
const char* SALAMANDER_PLUGINS_HOMEPAGE = "HomePage";
//const char *SALAMANDER_PLUGINS_PLGICONS = "PluginIcons";
const char* SALAMANDER_PLUGINS_PLGICONLIST = "PluginIconList";
const char* SALAMANDER_PLUGINS_PLGICONINDEX = "PluginIconIndex";
const char* SALAMANDER_PLUGINS_PLGSUBMENUICONINDEX = "SubmenuIconIndex";
const char* SALAMANDER_PLUGINS_SUBMENUINPLUGINSBAR = "SubmenuInPluginsBar";
const char* SALAMANDER_PLUGINS_THUMBMASKS = "ThumbnailMasks";
const char* SALAMANDER_PLUGINS_REGKEYNAME = "Configuration Key";
const char* SALAMANDER_PLUGINS_FSNAME = "FS Name";
const char* SALAMANDER_PLUGINS_FUNCTIONS = "Functions";
const char* SALAMANDER_PLUGINS_LOADONSTART = "Load On Start";
const char* SALAMANDER_PLUGINS_MENU = "Menu";
const char* SALAMANDER_PLUGINS_MENUITEMNAME = "Name";
const char* SALAMANDER_PLUGINS_MENUITEMSTATE = "State";
const char* SALAMANDER_PLUGINS_MENUITEMID = "ID";
const char* SALAMANDER_PLUGINS_MENUITEMSKILLLEVEL = "Skill";
const char* SALAMANDER_PLUGINS_MENUITEMICONINDEX = "Icon";
const char* SALAMANDER_PLUGINS_MENUITEMTYPE = "Type";
const char* SALAMANDER_PLUGINS_MENUITEMHOTKEY = "HotKey";
const char* SALAMANDER_PLUGINS_FSCMDNAME = "FS Cmd Name";
const char* SALAMANDER_PLUGINS_FSCMDICON = "FS Cmd Icon";
const char* SALAMANDER_PLUGINS_FSCMDVISIBLE = "FS Cmd Visible";
const char* SALAMANDER_PLUGINS_ISNETHOOD = "Is Nethood";
const char* SALAMANDER_PLUGINS_USESPASSWDMAN = "Uses Password Manager";

// Plugins: the following eight strings are only for converting configuration from version 6 and older
const char* SALAMANDER_PLUGINS_PANELVIEW = "Panel List";
const char* SALAMANDER_PLUGINS_PANELEDIT = "Panel Pack";
const char* SALAMANDER_PLUGINS_CUSTPACK = "Custom Pack";
const char* SALAMANDER_PLUGINS_CUSTUNPACK = "Custom Unpack";
const char* SALAMANDER_PLUGINS_CONFIG = "Configuration";
const char* SALAMANDER_PLUGINS_LOADSAVE = "Persistent";
const char* SALAMANDER_PLUGINS_VIEWER = "File Viewer";
const char* SALAMANDER_PLUGINS_FS = "File System";

// Plugins Configuration
const char* SALAMANDER_PLUGINSCONFIG = "Plugins Configuration";

// Plugins Order
const char* SALAMANDER_PLUGINSORDER = "Plugins Order";
const char* SALAMANDER_PLUGINSORDER_SHOW = "ShowInBar";

// Packers & Unpackers
const char* SALAMANDER_PACKANDUNPACK = "Packers & Unpackers";
const char* SALAMANDER_CUSTOMPACKERS = "Custom Packers";
const char* SALAMANDER_CUSTOMUNPACKERS = "Custom Unpackers";
const char* SALAMANDER_PREDPACKERS = "Predefined Packers";
const char* SALAMANDER_ARCHIVEASSOC = "Archive Association";
// pro SALAMANDER_CUSTOMPACKERS i SALAMANDER_CUSTOMUNPACKERS
const char* SALAMANDER_ANOTHERPANEL = "Use Another Panel";
const char* SALAMANDER_PREFFERED = "Preffered";
const char* SALAMANDER_NAMEBYARCHIVE = "Use Subdir Name By Archive";
const char* SALAMANDER_SIMPLEICONSINARCHIVES = "Simple Icons In Archives";

const char* SALAMANDER_PWDMNGR_REG = "Password Manager";

//****************************************************************************
//
// GetUpgradeInfo
//
// Tries to find "AutoImportConfig" in the configuration key of this version of Salamander.
// If it is not found or if the key stored in AutoImportConfig does not exist
// (points to the key of this version, which makes no sense)
// or if it contains a corrupted (incomplete save) or empty configuration, it returns
// FALSE in 'autoImportConfig'. Otherwise it returns TRUE in 'autoImportConfig' and
// in 'autoImportConfigFromKey' returns the path of the key from which to import the configuration.
// Handles the case when AutoImportConfig points to a key that itself contains AutoImportConfig
// for another key. We simply follow the "target" key and leave intermediate keys untouched-
// if the import succeeds, the target key will be removed anyway. Returns FALSE only if the application should exit.
//
// If the configuration key of this version contains, besides AutoImportConfig, also the "Configuration"
// key (expected to be a saved configuration),
// we ask the user whether to:
//   - Use the current configuration and ignore the old one (we do not delete it so the user does not lose data,
//     and it does not require that much space anyway). In this case delete AutoImportConfig immediately.
//     This is done silently, if AutoImportConfig points to this version of Salamander`s key
//     (DEFAULT OFFER because it does not cause data loss and users may dismiss the message box without reading).
//   - Delete the current configuration and import the old version. In this case remove everything except AutoImportConfig.
//   - Exit the application - simply return FALSE.

BOOL GetUpgradeInfo(BOOL* autoImportConfig, char* autoImportConfigFromKey, int autoImportConfigFromKeySize)
{
    HKEY rootKey;
    DWORD saveInProgress; // dummy
    BOOL doNotExit = TRUE;
    if (autoImportConfigFromKeySize > 0)
        *autoImportConfigFromKey = 0;
    LoadSaveToRegistryMutex.Enter();
    int rounds = 0; // prevent infinite loops
    *autoImportConfig = FALSE;
    if (HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, SalamanderConfigurationRoots[0], 0,
                               KEY_READ, &rootKey)) == ERROR_SUCCESS)
    {
        HKEY oldCfgKey;
        char oldKeyName[200];

        if (GetValue(rootKey, SALAMANDER_AUTO_IMPORT_CONFIG, REG_SZ, oldKeyName, 200))
        { // we found "AutoImportConfig"
        OPEN_AUTO_IMPORT_CONFIG_KEY:
            lstrcpyn(autoImportConfigFromKey, SalamanderConfigurationRoots[0], autoImportConfigFromKeySize);
            if (CutDirectory(autoImportConfigFromKey) &&
                SalPathAppend(autoImportConfigFromKey, oldKeyName, autoImportConfigFromKeySize) &&
                !IsTheSamePath(autoImportConfigFromKey, SalamanderConfigurationRoots[0]) &&     // the key stored in AutoImportConfig does not point to this version's key
                HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, autoImportConfigFromKey, 0, KEY_READ, // the key stored in AutoImportConfig can be opened (otherwise it doesn't exist?)
                                       &oldCfgKey)) == ERROR_SUCCESS)
            {
                // if the current "target" key also contains AutoImportConfig, follow it...
                if (GetValue(oldCfgKey, SALAMANDER_AUTO_IMPORT_CONFIG, REG_SZ, oldKeyName, 200) && ++rounds <= 50)
                {
                    HANDLES(RegCloseKey(oldCfgKey));
                    goto OPEN_AUTO_IMPORT_CONFIG_KEY;
                }
                HKEY cfgKey;
                if (rounds <= 50 &&
                    !GetValue(oldCfgKey, SALAMANDER_SAVE_IN_PROGRESS, REG_DWORD, &saveInProgress, sizeof(DWORD)) &&
                    HANDLES_Q(RegOpenKeyEx(oldCfgKey, SALAMANDER_CONFIG_REG, 0, KEY_READ, &cfgKey)) == ERROR_SUCCESS)
                {
                    HANDLES(RegCloseKey(cfgKey));
                    *autoImportConfig = TRUE; // configuration is valid and not empty
                }
                HANDLES(RegCloseKey(oldCfgKey));
            }
        }
        if (*autoImportConfig) // check whether this version's key also contains configuration (besides "AutoImportConfig")
        {
            HKEY cfgKey;
            lstrcpyn(oldKeyName, SalamanderConfigurationRoots[0], 200);
            if (SalPathAppend(oldKeyName, SALAMANDER_CONFIG_REG, 200) &&
                HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, oldKeyName, 0, KEY_READ, &cfgKey)) == ERROR_SUCCESS)
            {
                HANDLES(RegCloseKey(cfgKey));
                BOOL clearCfg = FALSE;
                if (!GetValue(rootKey, SALAMANDER_SAVE_IN_PROGRESS, REG_DWORD, &saveInProgress, sizeof(DWORD)))
                { // this key contains a valid configuration; ask the user what to do
                    HANDLES(RegCloseKey(rootKey));
                    rootKey = NULL;
                    LoadSaveToRegistryMutex.Leave();

                    MSGBOXEX_PARAMS params;
                    memset(&params, 0, sizeof(params));
                    params.HParent = NULL;
                    params.Flags = MB_ABORTRETRYIGNORE | MB_ICONQUESTION | MB_SETFOREGROUND;
                    params.Caption = SALAMANDER_TEXT_VERSION;
                    lstrcpyn(oldKeyName, autoImportConfigFromKey, 200);
                    char* keyName;
                    if (!CutDirectory(oldKeyName, &keyName))
                        keyName = oldKeyName; // theoretically cannot happen
                    char buf[1000];
                    sprintf(buf, "You have upgraded from %s (old version) to %s (new version). The configuration of the old "
                                 "version should be imported to the new version now, but there is already existing "
                                 "configuration for the new version. You can use this existing configuration (the configuration of "
                                 "the old version remains in registry, so you can import it later). Or you can overwrite "
                                 "this existing configuration (it would be lost) with the configuration of the old version. "
                                 "Or you can exit Open Salamander and solve this problem later.",
                            keyName, SALAMANDER_TEXT_VERSION);
                    params.Text = buf;
                    char aliasBtnNames[200];
                    sprintf(aliasBtnNames, "%d\t%s\t%d\t%s\t%d\t%s",
                            DIALOG_ABORT, "&Use Existing Configuration",
                            DIALOG_RETRY, "&Overwrite Existing Configuration",
                            DIALOG_IGNORE, "&Exit");
                    params.AliasBtnNames = aliasBtnNames;
                    int res = SalMessageBoxEx(&params);
                    switch (res)
                    {
                    case DIALOG_ABORT:
                        *autoImportConfig = FALSE;
                        break;
                    case DIALOG_RETRY:
                        clearCfg = TRUE;
                        break;

                    // case DIALOG_IGNORE:
                    default:
                        doNotExit = FALSE;
                        break;
                    }

                    LoadSaveToRegistryMutex.Enter();
                }
                else
                    clearCfg = TRUE; // configuration is corrupted, delete it
                if (clearCfg &&
                    HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, SalamanderConfigurationRoots[0], 0,
                                           KEY_READ | KEY_WRITE, &cfgKey)) == ERROR_SUCCESS)
                { // delete the configuration and leave only "AutoImportConfig" (recreate it)
                    ClearKey(cfgKey);
                    lstrcpyn(oldKeyName, autoImportConfigFromKey, 200);
                    char* keyName;
                    if (!CutDirectory(oldKeyName, &keyName))
                        keyName = oldKeyName; // theoretically cannot happen
                    SetValue(cfgKey, SALAMANDER_AUTO_IMPORT_CONFIG, REG_SZ, keyName, -1);
                    HANDLES(RegCloseKey(cfgKey));
                }
            }
        }
        if (rootKey != NULL)
            HANDLES(RegCloseKey(rootKey));
    }
    if (!*autoImportConfig && // this version's key lacks "AutoImportConfig" or does not point to a valid old configuration
        HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, SalamanderConfigurationRoots[0], 0,
                               KEY_READ | KEY_WRITE, &rootKey)) == ERROR_SUCCESS)
    { // remove "AutoImportConfig" from this version's key (if it exists it makes no sense here)
        RegDeleteValue(rootKey, SALAMANDER_AUTO_IMPORT_CONFIG);
        HANDLES(RegCloseKey(rootKey));
    }
    LoadSaveToRegistryMutex.Leave();
    return doNotExit;
}

//****************************************************************************
//
// FindLanguageFromPrevVerOfSal
//
// Retrieves the language (the .slg module used) from an older version of Salamander.
// The oldest version from which we obtain this information is 2.53 beta 2 (the first version shipped with multiple languages: CZ+DE+EN).
// If a configuration for the current version exists or such a language is not found, returns FALSE.
// Otherwise returns the language in 'slgName' (MAX_PATH buffer).

BOOL FindLanguageFromPrevVerOfSal(char* slgName)
{
    HKEY hCfgKey;
    HKEY hRootKey;
    int rootIndex = 0;
    const char* root;
    DWORD saveInProgress; // dummy

    slgName[0] = 0;
    LoadSaveToRegistryMutex.Enter();
    do
    {
        // check if the key exists and if a configuration is stored under it
        root = SalamanderConfigurationRoots[rootIndex];
        BOOL rootFound = HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, root, 0, KEY_READ, &hRootKey)) == ERROR_SUCCESS;
        BOOL cfgFound = rootFound && HANDLES_Q(RegOpenKeyEx(hRootKey, SALAMANDER_CONFIG_REG, 0,
                                                            KEY_READ, &hCfgKey)) == ERROR_SUCCESS;
        if (cfgFound && GetValue(hRootKey, SALAMANDER_SAVE_IN_PROGRESS, REG_DWORD, &saveInProgress, sizeof(DWORD)))
        { // the configuration is corrupted
            cfgFound = FALSE;
            HANDLES(RegCloseKey(hCfgKey));
        }
        DWORD configVersion = 1; // this is configuration from 1.52 or older
        if (cfgFound)
        {
            HKEY actKey;
            if (HANDLES_Q(RegOpenKeyEx(hRootKey, SALAMANDER_VERSION_REG, 0, KEY_READ, &actKey) == ERROR_SUCCESS))
            {
                configVersion = 2; // configuration from 1.6b1
                GetValue(actKey, SALAMANDER_VERSIONREG_REG, REG_DWORD, &configVersion, sizeof(DWORD));
                HANDLES(RegCloseKey(actKey));
            }
        }
        if (rootFound)
            HANDLES(RegCloseKey(hRootKey));
        if (cfgFound)
        {
            BOOL found = FALSE;
            if (rootIndex != 0 &&                      // only for one of the older keys
                configVersion >= 59 /* 2.53 beta 2 */) // before 2.53 beta 2 there was only English, so reading makes no sense; offer system default language or manual selection of the language
            {
                GetValue(hCfgKey, CONFIG_LANGUAGE_REG, REG_SZ, slgName, MAX_PATH);
                found = slgName[0] != 0;
            }
            HANDLES(RegCloseKey(hCfgKey));
            LoadSaveToRegistryMutex.Leave();
            return found;
        }
        rootIndex++;
    } while (rootIndex < SALCFG_ROOTS_COUNT);

    LoadSaveToRegistryMutex.Leave();
    return FALSE;
}

// obtains a number from a string (unsigned decimal format); returns TRUE, if a number was found
// ignores white spaces before and after the number
BOOL GetNumFromStr(const char* s, DWORD* retNum)
{
    DWORD n = 0;
    while (*s != 0 && *s <= ' ')
        s++;
    BOOL mayBeOK = *s >= '0' && *s <= '9';
    while (*s >= '0' && *s <= '9')
        n = 10 * n + (*s++ - '0');
    while (*s != 0 && *s <= ' ')
        s++;
    *retNum = n;
    return mayBeOK && *s == 0;
}

void CheckShutdownParams()
{
    // HKEY_CURRENT_USER\Control Panel\Desktop\WaitToKillAppTimeout=20000,REG_SZ ... warn if less than 20000
    // HKEY_CURRENT_USER\Control Panel\Desktop\AutoEndTasks=0,REG_SZ ... warn if not 0
    // W2K and XP have it; I could not find it on Vista but supposedly it is there (info from the internet)

    BOOL showWarning = FALSE;
    HKEY key;
    if (OpenKeyAux(NULL, HKEY_CURRENT_USER, "Control Panel\\Desktop", key))
    {
        char num[100];
        DWORD value;
        if (GetValueAux(NULL, key, "WaitToKillAppTimeout", REG_SZ, num, 100) &&
            GetNumFromStr(num, &value) && value < 20000)
        {
            TRACE_E("CheckShutdownParams(): WaitToKillAppTimeout is '" << num << "' (" << value << ")");
            showWarning = TRUE;
        }
        if (GetValueAux(NULL, key, "AutoEndTasks", REG_SZ, num, 100) &&
            GetNumFromStr(num, &value) && value != 0)
        {
            TRACE_E("CheckShutdownParams(): AutoEndTasks is '" << num << "' (" << value << ")");
            showWarning = TRUE;
        }
        CloseKeyAux(key);
    }

    if (showWarning)
        SalMessageBox(NULL, LoadStr(IDS_CHANGEDSHUTDOWNPARS), SALAMANDER_TEXT_VERSION, MB_OK | MB_ICONWARNING);
}

BOOL MyRegRenameKey(HKEY key, const char* name, const char* newName)
{
    BOOL ret = FALSE;
    // There is also NtRenameKey but I could not get it working (requires UNICODE_STRING
    // and probably the key opened via NtOpenKey with the key passed via OBJECT_ATTRIBUTES initialized through
    // InitializeObjectAttributes). It's overly complicated and not frequently used code,
    // so we'll do it the slow but simple way... copy the key to a new one and then delete the original
    HKEY newKey;
    if (!OpenKeyAux(NULL, key, newName, newKey)) // verify if the target key does not already exist
    {
        if (CreateKeyAux(NULL, key, newName, newKey)) // create the target key
        {
            // I also tried RegCopyTree (didn't work without KEY_ALL_ACCESS) and the speed was the same as SHCopyKey
            if (SHCopyKey(key, name, newKey, 0) == ERROR_SUCCESS) // copy into the target key
                ret = TRUE;
            CloseKeyAux(newKey);
            if (ret)
                SHDeleteKey(key, name);
        }
        else
            TRACE_E("MyRegRenameKey(): unable to create target key: " << newName);
    }
    else
    {
        CloseKeyAux(newKey);
        TRACE_E("MyRegRenameKey(): target key already exists: " << newName);
    }
    return ret;
}

//****************************************************************************
//
// FindLatestConfiguration
//
// Tries to find a configuration that matches our program version.
// If it succeeds, 'loadConfiguration' variable is set and the function returns TRUE.
// If a configuration for this version does not exist yet, the function scans
// older configurations from the 'SalamanderConfigurationRoots' array (from newest to oldest).
// When it finds one of the configurations, a dialog is shown offering to convert it into the current configuration
// and delete it from the registry. After the last dialog, it returns TRUE and fills
// 'deleteConfigurations' and 'loadConfiguration' according to the user's choice.
// If the user chooses to exit the application, the function returns FALSE.
//

// Helper: detect product name from registry path
static const char* DetectProductName(const char* root)
{
    if (StrIStr(root, "Open Salamander") != NULL)
        return LoadStr(IDS_MCD_OPEN_SALAMANDER);
    if (StrIStr(root, "Altap Salamander") != NULL)
        return LoadStr(IDS_MCD_ALTAP_SALAMANDER);
    return LoadStr(IDS_MCD_SERVANT_SALAMANDER);
}

static void MCDTrimTrailingSpaces(char* text)
{
    if (text == NULL)
        return;
    char* end = text + strlen(text);
    while (end > text && end[-1] == ' ')
        *--end = 0;
}

static BOOL MCDIsGeneratedConfigDisplayName(const char* root, const char* version, const char* displayName)
{
    if (root == NULL || version == NULL || displayName == NULL || displayName[0] == 0)
        return FALSE;

    char actual[256];
    strncpy_s(actual, displayName, _TRUNCATE);
    MCDTrimTrailingSpaces(actual);

    char expected[256];
    _snprintf_s(expected, _TRUNCATE, "%s", DetectProductName(root));
    MCDTrimTrailingSpaces(expected);
    if (_stricmp(actual, expected) == 0)
        return TRUE;

    const char* platforms[] = {"x64", "x86"};
    for (int i = 0; i < SizeOf(platforms); i++)
    {
        char expectedWithPlatform[256];
        _snprintf_s(expectedWithPlatform, _TRUNCATE, "%s (%s)", expected, platforms[i]);
        if (_stricmp(actual, expectedWithPlatform) == 0)
            return TRUE;
    }

    return FALSE;
}

// Helper: read language from a registry config
static BOOL ReadConfigLanguage(HKEY hRootKey, char* language, int languageSize)
{
    language[0] = 0;

    // Nacist Language z <root>\Configuration\Language
    HKEY hCfgKey;
    if (RegOpenKeyEx(hRootKey, "Configuration", 0, KEY_READ, &hCfgKey) == ERROR_SUCCESS)
    {
        DWORD dwSize = languageSize;
        if (RegQueryValueEx(hCfgKey, "Language", NULL, NULL, (LPBYTE)language, &dwSize) == ERROR_SUCCESS)
        {
            // Odstranit priponu ".slg" (napr. "english.slg" -> "english")
            char* dot = strrchr(language, '.');
            if (dot != NULL && _stricmp(dot, ".slg") == 0)
                *dot = 0;
        }
        RegCloseKey(hCfgKey);
    }
    return language[0] != 0;
}

// Helper: read config version from registry
static DWORD ReadConfigVersion(HKEY hRootKey)
{
    DWORD configVersion = 1;
    HKEY hVerKey;
    if (HANDLES_Q(RegOpenKeyEx(hRootKey, SALAMANDER_VERSION_REG, 0, KEY_READ, &hVerKey)) == ERROR_SUCCESS)
    {
        configVersion = 2;
        GetValueAux(NULL, hVerKey, SALAMANDER_VERSIONREG_REG, REG_DWORD, &configVersion, sizeof(DWORD));
        HANDLES(RegCloseKey(hVerKey));
    }
    return configVersion;
}

BOOL FindLatestConfiguration(BOOL* deleteConfigurations, const char*& loadConfiguration, BOOL forceWelcomeDialog, char* selectedRegFilePath, int selectedRegFilePathSize)
{
    HKEY hRootKey;
    loadConfiguration = NULL;
    if (selectedRegFilePath != NULL && selectedRegFilePathSize > 0)
        selectedRegFilePath[0] = 0;
    DWORD saveInProgress;
    HKEY hCfgKey;

    CManageConfigsDialog dlg;
    dlg.DeleteConfigurations = deleteConfigurations;
    dlg.IndexOfConfigToLoad = -1;
    dlg.StorageType = Configuration.StorageType;
    dlg.CanSaveBootstrap = ConfigurationStorage.CanSaveStorageTypeBootstrap();

    LoadSaveToRegistryMutex.Enter();

    // Backup check (stejna logika jako drive)
    char backup[200];
    sprintf_s(backup, "%s.backup.63A7CD13", SalamanderConfigurationRoots[0]);
    HKEY backupKey;
    BOOL backupFound = OpenKeyAux(NULL, HKEY_CURRENT_USER, backup, backupKey);
    if (backupFound)
    {
        DWORD copyIsOK;
        if (GetValueAux(NULL, backupKey, SALAMANDER_COPY_IS_OK, REG_DWORD, &copyIsOK, sizeof(DWORD)))
            copyIsOK = 1;
        else
            copyIsOK = 0;
        HANDLES(RegCloseKey(backupKey));
        if (!copyIsOK)
        {
            TRACE_I("Configuration backup is incomplete, removing... " << backup);
            SHDeleteKey(HKEY_CURRENT_USER, backup);
            backupFound = FALSE;
        }
        else
            TRACE_I("Configuration backup is OK: " << backup);
    }

    // Scan all registry configurations
    int configCount = 0;
    for (int rootIndex = 0; rootIndex < SALCFG_ROOTS_COUNT && configCount < MCD_MAX_CONFIGS; rootIndex++)
    {
        const char* root = SalamanderConfigurationRoots[rootIndex];
        BOOL rootFound = OpenKeyAux(NULL, HKEY_CURRENT_USER, root, hRootKey);

        // Corruption check
        if (rootFound &&
            GetValueAux(NULL, hRootKey, SALAMANDER_SAVE_IN_PROGRESS, REG_DWORD, &saveInProgress, sizeof(DWORD)))
        {
            TRACE_E("Configuration is corrupted!");
            rootFound = FALSE;
            CloseKeyAux(hRootKey);

            if (rootIndex == 0 && backupFound)
            {
                char corrupted[200];
                sprintf_s(corrupted, "%s.corrupted.63A7CD13", root);
                SHDeleteKey(HKEY_CURRENT_USER, corrupted);
                if (MyRegRenameKey(HKEY_CURRENT_USER, root, corrupted) &&
                    MyRegRenameKey(HKEY_CURRENT_USER, backup, root))
                {
                    backupFound = FALSE;
                    if (CreateKeyAux(NULL, HKEY_CURRENT_USER, root, hRootKey))
                    {
                        DeleteValueAux(hRootKey, SALAMANDER_COPY_IS_OK);
                        CloseKeyAux(hRootKey);
                    }
                    TRACE_I("Corrupted configuration was moved to: " << corrupted);
                    TRACE_I("Using configuration backup instead ...");
                    rootFound = OpenKeyAux(NULL, HKEY_CURRENT_USER, root, hRootKey);
                }
                else
                    TRACE_E("Unable to move corrupted configuration or configuration backup.");
            }

            if (rootIndex == 0 && rootFound == FALSE)
            {
                char buf[1500];
                _snprintf_s(buf, _TRUNCATE, LoadStr(IDS_CORRUPTEDCONFIGFOUND), root);
                LoadSaveToRegistryMutex.Leave();

                MSGBOXEX_PARAMS params;
                memset(&params, 0, sizeof(params));
                params.HParent = NULL;
                params.Flags = MB_OKCANCEL | MB_ICONERROR | MB_DEFBUTTON2;
                params.Caption = SALAMANDER_TEXT_VERSION;
                params.Text = buf;
                char aliasBtnNames[200];
                sprintf(aliasBtnNames, "%d\t%s\t%d\t%s", DIALOG_OK, LoadStr(IDS_CORRUPTEDCONFIGREMOVEBTN),
                        DIALOG_CANCEL, LoadStr(IDS_SELLANGEXITBUTTON));
                params.AliasBtnNames = aliasBtnNames;
                if (SalMessageBoxEx(&params) == IDCANCEL)
                {
                    CheckShutdownParams();
                    return FALSE;
                }

                CheckShutdownParams();
                LoadSaveToRegistryMutex.Enter();
                if (HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, root, 0, KEY_READ | KEY_WRITE, &hRootKey)) == ERROR_SUCCESS)
                {
                    TRACE_I("Deleting corrupted configuration on user demand: " << root);
                    ClearKeyAux(hRootKey);
                    CloseKeyAux(hRootKey);
                    DeleteKeyAux(HKEY_CURRENT_USER, root);
                }
                continue;
            }
        }

        BOOL cfgFound = rootFound && OpenKeyAux(NULL, hRootKey, SALAMANDER_CONFIG_REG, hCfgKey);

        if (rootIndex == 0 && backupFound)
        {
            TRACE_I("Removing unnecessary configuration backup: " << backup);
            SHDeleteKey(HKEY_CURRENT_USER, backup);
            backupFound = FALSE;
        }

        if (cfgFound)
        {
            CFoundConfig& cfg = dlg.Configs[configCount];
            cfg.Exists = TRUE;
            cfg.IsCurrentVersion = (rootIndex == 0);
            cfg.IsPortable = FALSE;
            cfg.IsCorrupted = FALSE;
            cfg.RootIndex = rootIndex;
            _snprintf_s(cfg.Location, _TRUNCATE, "reg:\\HKEY_CURRENT_USER\\%s", root);

            // Nazev - nejdriv zkusit vlastni nazev z registru, pak automaticky generovany
            char customName[256];
            customName[0] = 0;
            if (GetValueAux(NULL, hCfgKey, "ConfigDisplayName", REG_SZ, customName, sizeof(customName)) && customName[0] != 0)
            {
                strncpy_s(cfg.DisplayName, customName, _TRUNCATE);
            }
            else
            {
                const char* name = DetectProductName(root);
                sprintf_s(cfg.DisplayName, name, SalamanderConfigurationVersions[rootIndex]);
            }
            cfg.IsGeneratedName = MCDIsGeneratedConfigDisplayName(root, SalamanderConfigurationVersions[rootIndex], cfg.DisplayName);

            // Zkontrolovat WelcomeProcessed v tomto klici
            DWORD wpVal = 0;
            GetValueAux(NULL, hCfgKey, "WelcomeProcessed", REG_DWORD, &wpVal, sizeof(wpVal));
            // WelcomeProcessed suppresses the first-run Welcome dialog only for the current
            // target registry root.  Older discovered configurations remain import sources;
            // they must never become an implicit active configuration just because they
            // contain stale metadata from a previous version/import.
            if (rootIndex == 0 && wpVal == 1)
            {
                char processedInstancePath[MAX_PATH];
                processedInstancePath[0] = 0;
                GetValueAux(NULL, hCfgKey, "WelcomeProcessedInstancePath", REG_SZ,
                            processedInstancePath, sizeof(processedInstancePath));
                char currentInstancePath[MAX_PATH];
                currentInstancePath[0] = 0;
                MCDGetCurrentInstancePath(currentInstancePath, SizeOf(currentInstancePath));
                if ((processedInstancePath[0] != 0 && currentInstancePath[0] != 0 &&
                     IsTheSamePath(processedInstancePath, currentInstancePath)) ||
                    (processedInstancePath[0] == 0 && !dlg.CanSaveBootstrap))
                {
                    strncpy_s(dlg.WelcomeProcessedLocation, cfg.Location, _TRUNCATE);
                }
            }

            CloseKeyAux(hCfgKey);

            // Verze: pro Samandarin pouzit format s pomlckami
            strncpy_s(cfg.Version, SalamanderConfigurationVersions[rootIndex], _TRUNCATE);
            if (StrIStr(cfg.Version, "Samandarin") != NULL)
            {
                for (char* p = cfg.Version; *p; p++)
                    if (*p == ' ') *p = '-';
            }

            // Storage
            strncpy_s(cfg.StorageTypeStr, LoadStr(IDS_MCD_STORAGE_REGISTRY), _TRUNCATE);

            // Language
            ReadConfigLanguage(hRootKey, cfg.Language, SizeOf(cfg.Language));

            // Location
            _snprintf_s(cfg.Location, _TRUNCATE, "reg:\\HKEY_CURRENT_USER\\%s", root);

            // Last update
            RegQueryInfoKey(hRootKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cfg.LastUpdate);

            CloseKeyAux(hRootKey);

            configCount++;
        }
        else if (rootFound)
        {
            CloseKeyAux(hRootKey);
        }
    }

    // Check for portable config.reg
    static char portableConfigPath[SAL_MAX_PATH];
    ConfigurationStorage.GetPortableConfigFilePath(portableConfigPath, SizeOf(portableConfigPath));
    if (GetFileAttributes(portableConfigPath) != INVALID_FILE_ATTRIBUTES && configCount < MCD_MAX_CONFIGS)
    {
        CFoundConfig& cfg = dlg.Configs[configCount];
        MCDReadFileConfigurationInfo(portableConfigPath, cfg, FALSE);
        configCount++;
    }

    // Check for known file storage paths (z configstorage.ini)
    static char knownPaths[20][SAL_MAX_PATH];
    int knownCount = 0;
    ConfigurationStorage.LoadKnownFileStoragePaths(knownPaths, &knownCount, 20);
    for (int k = 0; k < knownCount && configCount < MCD_MAX_CONFIGS; k++)
    {
        // Preskocit pokud je to stejna cesta jako portable config.reg
        if (portableConfigPath[0] != 0 && _stricmp(knownPaths[k], portableConfigPath) == 0)
            continue;

        if (GetFileAttributes(knownPaths[k]) != INVALID_FILE_ATTRIBUTES)
        {
            CFoundConfig& cfg = dlg.Configs[configCount];
            MCDReadFileConfigurationInfo(knownPaths[k], cfg, FALSE);
            configCount++;
        }
    }

    // Add "Empty Configuration" as first item
    if (configCount < MCD_MAX_CONFIGS)
    {
        // Shift all configs down
        for (int i = configCount; i > 0; i--)
            dlg.Configs[i] = dlg.Configs[i - 1];

        CFoundConfig& cfg = dlg.Configs[0];
        memset(&cfg, 0, sizeof(cfg));
        cfg.Exists = TRUE;
        cfg.IsCurrentVersion = FALSE;
        cfg.IsPortable = FALSE;
        cfg.IsCorrupted = FALSE;
        cfg.RootIndex = -1;
        strncpy_s(cfg.DisplayName, LoadStr(IDS_MCD_CLEANCONFIG), _TRUNCATE);
        strncpy_s(cfg.Version, SalamanderConfigurationVersions[0], _TRUNCATE);
        if (StrIStr(cfg.Version, "Samandarin") != NULL)
            for (char* p = cfg.Version; *p; p++) if (*p == ' ') *p = '-';
        strncpy_s(cfg.StorageTypeStr, "-", _TRUNCATE);
        const char* welcomeLanguage = Configuration.LoadedSLGName[0] != 0 ? Configuration.LoadedSLGName : Configuration.SLGName;
        strncpy_s(cfg.Language, welcomeLanguage != NULL && welcomeLanguage[0] != 0 ? welcomeLanguage : "english", _TRUNCATE);
        {
            char* dot = strrchr(cfg.Language, '.');
            if (dot != NULL && _stricmp(dot, ".slg") == 0)
                *dot = 0;
        }
        strncpy_s(cfg.Location, "-", _TRUNCATE);
        cfg.LastUpdate.dwLowDateTime = 0;
        cfg.LastUpdate.dwHighDateTime = 0;

        configCount++;
    }

    dlg.ConfigsCount = configCount;

    LoadSaveToRegistryMutex.Leave();

    // Check if we should skip the dialog
    // Skip if: WelcomeProcessed is set in any of the scanned configs
    BOOL welcomeProcessed = !forceWelcomeDialog && (dlg.WelcomeProcessedLocation[0] != 0);

    if (welcomeProcessed)
    {
        // Config exists and was already processed -> load it directly without showing the dialog

        // Pokud je WelcomeProcessed nastaven a mame prava pro zapis, automaticky vytvorime
        // configstorage.ini s registry storage type (mohlo by chybet po upgrade z no-write uctu)
        if (dlg.CanSaveBootstrap)
        {
            CConfigurationStorageType bootstrapType;
            char bootstrapPath[MAX_PATH];
            bootstrapPath[0] = 0;
            if (!ConfigurationStorage.LoadStorageTypeBootstrap(bootstrapType, bootstrapPath, SizeOf(bootstrapPath)))
            {
                ConfigurationStorage.SaveStorageTypeBootstrap(cstRegistry, NULL);
            }
        }

        // Nastavit loadConfiguration z presne cesty v WelcomeProcessedLocation
        // Location je "reg:\HKEY_CURRENT_USER\<subkey>" - extrahujeme subkey
        const char* loc = dlg.WelcomeProcessedLocation;
        const char* subkey = strchr(loc, '\\');
        if (subkey) subkey++;
        if (subkey) subkey = strchr(subkey, '\\');
        if (subkey) subkey++;
        if (subkey && subkey[0] != 0)
        {
            // Najit odpovidajici root v SalamanderConfigurationRoots
            for (int i = 0; i < SALCFG_ROOTS_COUNT; i++)
            {
                if (_stricmp(SalamanderConfigurationRoots[i], subkey) == 0)
                {
                    loadConfiguration = SalamanderConfigurationRoots[i];
                    break;
                }
            }
        }
        return TRUE;
    }

    // Show the dialog
    HWND hSplash = GetSplashScreenHandle();
    if (hSplash != NULL)
        ShowWindow(hSplash, SW_HIDE);

    int dlgRet = (int)dlg.Execute();

    if (hSplash != NULL)
    {
        ShowWindow(hSplash, SW_SHOW);
        UpdateWindow(hSplash);
    }

    if (dlgRet == IDCANCEL)
    {
        return FALSE;
    }

    Configuration.StorageType = dlg.StorageType;

    const char* selectedLoadConfiguration = NULL;
    if (!MCDApplyConfigurationSelection(NULL, dlg, TRUE, selectedLoadConfiguration))
        return FALSE;
    if (selectedLoadConfiguration != NULL)
        loadConfiguration = selectedLoadConfiguration;
    if (dlg.CustomLanguage[0] != 0)
        strncpy_s(Configuration.SLGName, dlg.CustomLanguage, _TRUNCATE);
    if (dlg.StorageType == cstRegFile && selectedRegFilePath != NULL && selectedRegFilePathSize > 0)
        strncpy_s(selectedRegFilePath, selectedRegFilePathSize, dlg.RegFilePath, _TRUNCATE);

    BOOL bootstrapSaved = ConfigurationStorage.SaveStorageTypeBootstrap((CConfigurationStorageType)Configuration.StorageType,
                                                                        dlg.StorageType == cstRegFile ? dlg.RegFilePath : NULL);
    if (!bootstrapSaved && (dlg.CanSaveBootstrap || dlg.StorageType == cstRegFile))
    {
        SalMessageBox(NULL, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                      MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    // Pridat file storage path do seznamu known paths
    if (dlg.StorageType == cstRegFile && dlg.RegFilePath[0] != 0)
    {
        ConfigurationStorage.AddKnownFileStoragePath(dlg.RegFilePath);
    }

    if (dlg.DeleteSourceAfterMigration)
        dlg.DeleteConfigByIndex(dlg.SelectedSourceIndex);

    if (dlg.CustomLanguage[0] != 0 && _stricmp(dlg.CustomLanguage, Configuration.LoadedSLGName) != 0)
    {
        // The first-run Welcome dialog is already localized before the target
        // configuration exists.  Restart the same way Manage Configurations does
        // so the selected target language is loaded immediately from the saved
        // target configuration instead of being overwritten by this process.
        if (MCDRestartSalamanderAfterWelcome(NULL))
            return FALSE;
        SalMessageBox(NULL, LoadStr(IDS_MCD_RESTARTMSG), LoadStr(IDS_INFOTITLE), MB_OK | MB_ICONINFORMATION);
    }

    if (loadConfiguration == NULL && DarkModeShouldUseDarkColors())
    {
        Configuration.UseWindowsDarkMode = TRUE;
        WindowsDarkModeBuildPalette(UserColors, ViewerColors);
        CurrentColors = UserColors;
    }

    return TRUE;
}

// deletes keys according to the array returned by FindLatestConfiguration

void CMainWindow::DeleteOldConfigurations(BOOL* deleteConfigurations, BOOL autoImportConfig,
                                          const char* autoImportConfigFromKey,
                                          BOOL doNotDeleteImportedCfg)
{
    // anything to delete?
    BOOL dirty = FALSE;
    if (autoImportConfig)
        dirty = TRUE;
    else
    {
        int rootIndex;
        for (rootIndex = 0; rootIndex < SALCFG_ROOTS_COUNT; rootIndex++)
        {
            if (deleteConfigurations[rootIndex])
            {
                dirty = TRUE;
                break;
            }
        }
    }
    if (dirty)
    {
        // remove old configurations
        HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        CWaitWindow analysing(HWindow, IDS_DELETINGCONFIGURATION, FALSE, ooStatic);
        analysing.Create();
        EnableWindow(HWindow, FALSE);
        LoadSaveToRegistryMutex.Enter();
        int rootIndex;
        for (rootIndex = 0; rootIndex < SALCFG_ROOTS_COUNT; rootIndex++)
        {
            if (deleteConfigurations[rootIndex])
            {
                HKEY hKey;
                const char* key = SalamanderConfigurationRoots[rootIndex];
                if (CreateKeyAux(NULL, HKEY_CURRENT_USER, key, hKey))
                {
                    ClearKeyAux(hKey);
                    CloseKeyAux(hKey);
                    DeleteKeyAux(HKEY_CURRENT_USER, key);
                }
            }
        }
        if (autoImportConfig) // clean old configuration (already stored in the new key) and remove "AutoImportConfig" from the new key
        {
            BOOL ok = FALSE;
            HKEY cfgKey;
            if (HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, SalamanderConfigurationRoots[0], 0,
                                       KEY_READ | KEY_WRITE, &cfgKey)) == ERROR_SUCCESS)
            { // remove "AutoImportConfig" value from the new key
                if (RegDeleteValue(cfgKey, SALAMANDER_AUTO_IMPORT_CONFIG) == ERROR_SUCCESS)
                    ok = TRUE;
                HANDLES(RegCloseKey(cfgKey));
            }
            if (!ok) // if this happens it's probably fine because we likely didn't
                     // write Salamander's configuration either (it goes to the
                     // same key) and the whole upgrade will need to be run again
            {
                TRACE_E("CMainWindow::DeleteOldConfigurations(): unable to delete " << SALAMANDER_AUTO_IMPORT_CONFIG << " value from HKCU\\" << SalamanderConfigurationRoots[0]);
            }
            else // clean the old configuration (already saved to the new key)
            {
                if (!doNotDeleteImportedCfg)
                {
                    if (HANDLES_Q(RegOpenKeyEx(HKEY_CURRENT_USER, autoImportConfigFromKey, 0,
                                               KEY_READ | KEY_WRITE, &cfgKey)) == ERROR_SUCCESS)
                    {
                        ClearKeyAux(cfgKey);
                        HANDLES(RegCloseKey(cfgKey));
                        DeleteKeyAux(HKEY_CURRENT_USER, autoImportConfigFromKey);
                    }
                }
            }
        }
        LoadSaveToRegistryMutex.Leave();
        EnableWindow(HWindow, TRUE);
        DestroyWindow(analysing.HWindow);
        SetCursor(hOldCursor);
    }
}

//
// ****************************************************************************
// CMainWindow
//

static BOOL IsDiskOrUNCPath(const char* path)
{
    if (path == NULL || *path == 0)
        return FALSE;
    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':')
        return TRUE;
    if ((path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
        return TRUE;
    return FALSE;
}

static void SavePanelSettingsToKey(CFilesWindow* panel, HKEY key, BOOL useGeneralPath)
{
    if (panel == NULL || key == NULL)
        return;

    DWORD value = panel->HeaderLineVisible;
    SetValue(key, PANEL_HEADER_REG, REG_DWORD, &value, sizeof(DWORD));

    char path[2 * MAX_PATH];
    if (useGeneralPath)
    {
        if (!panel->GetGeneralPath(path, _countof(path), TRUE))
            path[0] = 0;
    }
    else
        lstrcpyn(path, panel->GetPath(), _countof(path));
    SetValue(key, PANEL_PATH_REG, REG_SZ, path, -1);

    value = panel->GetViewTemplateIndex();
    SetValue(key, PANEL_VIEW_REG, REG_DWORD, &value, sizeof(DWORD));
    value = panel->SortType;
    SetValue(key, PANEL_SORT_REG, REG_DWORD, &value, sizeof(DWORD));
    value = panel->SortCustomData;
    SetValue(key, PANEL_SORT_CUSTOM_DATA_REG, REG_DWORD, &value, sizeof(DWORD));
    value = panel->ReverseSort;
    SetValue(key, PANEL_REVERSE_REG, REG_DWORD, &value, sizeof(DWORD));
    value = (panel->DirectoryLine->HWindow != NULL);
    SetValue(key, PANEL_DIRLINE_REG, REG_DWORD, &value, sizeof(DWORD));
    value = (panel->StatusLine->HWindow != NULL);
    SetValue(key, PANEL_STATUS_REG, REG_DWORD, &value, sizeof(DWORD));
    DWORD filterEnabled = panel->FilterEnabled;
    SetValue(key, PANEL_FILTER_ENABLE, REG_DWORD, &filterEnabled, sizeof(DWORD));
    SetValue(key, PANEL_FILTER, REG_SZ, panel->Filter.GetMasksString(), -1);

    if (panel->HasCustomTabColor())
    {
        DWORD color = panel->GetCustomTabColor();
        SetValue(key, PANEL_TABCOLOR_REG, REG_DWORD, &color, sizeof(DWORD));
    }
    else
        DeleteValue(key, PANEL_TABCOLOR_REG);

    if (panel->HasCustomTabPrefix())
    {
        const std::wstring& prefix = panel->GetCustomTabPrefix();
        int needed = WideCharToMultiByte(CP_ACP, 0, prefix.c_str(), -1, NULL, 0, NULL, NULL);
        if (needed > 0)
        {
            std::vector<char> buffer(needed);
            if (WideCharToMultiByte(CP_ACP, 0, prefix.c_str(), -1, buffer.data(), needed, NULL, NULL) > 0)
                SetValue(key, PANEL_TABPREFIX_REG, REG_SZ, buffer.data(), -1);
            else
                DeleteValue(key, PANEL_TABPREFIX_REG);
        }
        else
            DeleteValue(key, PANEL_TABPREFIX_REG);
    }
    else
        DeleteValue(key, PANEL_TABPREFIX_REG);

    if (panel->IsTabLocked())
    {
        DWORD locked = 1;
        SetValue(key, PANEL_TABLOCKED_REG, REG_DWORD, &locked, sizeof(DWORD));
    }
    else
        DeleteValue(key, PANEL_TABLOCKED_REG);
}

static void LoadPanelSettingsFromKey(CFilesWindow* panel, HKEY key, char* pathBuffer, int pathBufferSize)
{
    if (panel == NULL || key == NULL)
        return;

    if (pathBuffer != NULL && pathBufferSize > 0)
        pathBuffer[0] = 0;

    char path[2 * MAX_PATH];
    if (GetValue(key, PANEL_PATH_REG, REG_SZ, path, _countof(path)))
    {
        if (pathBuffer != NULL && pathBufferSize > 0)
            lstrcpyn(pathBuffer, path, pathBufferSize);

        DWORD value;
        if (GetValue(key, PANEL_HEADER_REG, REG_DWORD, &value, sizeof(DWORD)))
            panel->HeaderLineVisible = value;
        if (GetValue(key, PANEL_VIEW_REG, REG_DWORD, &value, sizeof(DWORD)))
        {
            if (Configuration.ConfigVersion < 13 && !value)
                value = 2;
            panel->SelectViewTemplate(value, FALSE, FALSE, VALID_DATA_ALL, FALSE, TRUE);
        }
        if (GetValue(key, PANEL_REVERSE_REG, REG_DWORD, &value, sizeof(DWORD)))
            panel->ReverseSort = value;
        DWORD sortCustomData = 0;
        BOOL hasSortCustomData = GetValue(key, PANEL_SORT_CUSTOM_DATA_REG, REG_DWORD, &sortCustomData, sizeof(DWORD));
        if (GetValue(key, PANEL_SORT_REG, REG_DWORD, &value, sizeof(DWORD)))
        {
            if (value > stCustom || (value == stCustom && !hasSortCustomData))
                value = stName;
            panel->SortType = (CSortType)value;
            panel->SortCustomData = value == stCustom ? sortCustomData : 0;
        }
        if (GetValue(key, PANEL_DIRLINE_REG, REG_DWORD, &value, sizeof(DWORD)))
            if ((BOOL)value != (panel->DirectoryLine->HWindow != NULL))
                panel->ToggleDirectoryLine();
        if (GetValue(key, PANEL_STATUS_REG, REG_DWORD, &value, sizeof(DWORD)))
            if ((BOOL)value != (panel->StatusLine->HWindow != NULL))
                panel->ToggleStatusLine();
        GetValue(key, PANEL_FILTER_ENABLE, REG_DWORD, &panel->FilterEnabled, sizeof(DWORD));

        char filter[MAX_PATH];
        if (!GetValue(key, PANEL_FILTER, REG_SZ, filter, MAX_PATH))
        {
            filter[0] = 0;
            if (Configuration.ConfigVersion < 22)
            {
                char* filterHistory[1];
                filterHistory[0] = NULL;
                LoadHistory(key, PANEL_FILTERHISTORY_REG, filterHistory, 1);
                if (filterHistory[0] != NULL)
                {
                    DWORD filterInverse = FALSE;
                    if (panel->FilterEnabled && Configuration.ConfigVersion < 14)
                        GetValue(key, PANEL_FILTER_INVERSE, REG_DWORD, &filterInverse, sizeof(DWORD));
                    if (filterInverse)
                        strcpy(filter, "|");
                    else
                        filter[0] = 0;
                    strcat(filter, filterHistory[0]);
                    free(filterHistory[0]);
                }
            }
            else
                panel->FilterEnabled = FALSE;
        }
        if (filter[0] != 0)
            panel->Filter.SetMasksString(filter);

        panel->UpdateFilterSymbol();
        int errPos;
        if (!panel->Filter.PrepareMasks(errPos))
        {
            panel->Filter.SetMasksString("*.*");
            panel->Filter.PrepareMasks(errPos);
        }
    }

    DWORD colorValue;
    if (GetValue(key, PANEL_TABCOLOR_REG, REG_DWORD, &colorValue, sizeof(DWORD)))
        panel->SetCustomTabColor(colorValue);
    else
        panel->ClearCustomTabColor();

    DWORD prefixSize = 0;
    if (GetSize(key, PANEL_TABPREFIX_REG, REG_SZ, prefixSize) && prefixSize > 1)
    {
        std::vector<char> buffer(prefixSize);
        if (GetValue(key, PANEL_TABPREFIX_REG, REG_SZ, buffer.data(), prefixSize))
        {
            int needed = MultiByteToWideChar(CP_ACP, 0, buffer.data(), -1, NULL, 0);
            if (needed > 0)
            {
                std::wstring prefix(needed - 1, L'\0');
                int written = MultiByteToWideChar(CP_ACP, 0, buffer.data(), -1, &prefix[0], needed);
                if (written > 0)
                {
                    prefix.resize(written - 1);
                    if (!prefix.empty())
                        panel->SetCustomTabPrefix(prefix.c_str());
                    else
                        panel->ClearCustomTabPrefix();
                }
                else
                    panel->ClearCustomTabPrefix();
            }
            else
                panel->ClearCustomTabPrefix();
        }
        else
            panel->ClearCustomTabPrefix();
    }
    else
        panel->ClearCustomTabPrefix();

    DWORD lockedValue = 0;
    if (GetValue(key, PANEL_TABLOCKED_REG, REG_DWORD, &lockedValue, sizeof(DWORD)))
        panel->SetTabLocked(lockedValue != 0);
    else
        panel->SetTabLocked(false);
}

static BOOL RestorePanelPathFromConfig(CMainWindow* mainWnd, CFilesWindow* panel, const char* path)
{
    if (panel == NULL)
        return FALSE;
    if (path == NULL || path[0] == 0)
    {
        if (mainWnd != NULL)
            mainWnd->UpdatePanelTabTitle(panel);
        return FALSE;
    }

    if (panel->ChangeDirLite(path))
    {
        if (mainWnd != NULL)
            mainWnd->UpdatePanelTabTitle(panel);
        return TRUE;
    }

    if (IsDiskOrUNCPath(path))
    {
        char tmp[2 * MAX_PATH];
        lstrcpyn(tmp, path, _countof(tmp));
        BOOL tryNet = TRUE;
        DWORD err, lastErr;
        BOOL pathInvalid, cut;
        if (SalCheckAndRestorePathWithCut(panel->HWindow, tmp, tryNet, err, lastErr, pathInvalid, cut, TRUE))
        {
            if (panel->ChangePathToDisk(panel->HWindow, tmp))
            {
                if (mainWnd != NULL)
                    mainWnd->UpdatePanelTabTitle(panel);
                return TRUE;
            }
        }
        panel->ChangeToRescuePathOrFixedDrive(panel->HWindow);
        if (mainWnd != NULL)
            mainWnd->UpdatePanelTabTitle(panel);
        return FALSE;
    }

    if (mainWnd != NULL)
        mainWnd->UpdatePanelTabTitle(panel);
    return FALSE;
}

void CMainWindow::SavePanelConfig(CPanelSide side, HKEY hSalamander, const char* reg)
{
    HKEY actKey;
    if (!CreateKey(hSalamander, reg, actKey))
        return;

    ClearKeyAux(actKey);

    CFilesWindow* activePanel = (side == cpsLeft) ? LeftPanel : RightPanel;
    if (activePanel != NULL)
        SavePanelSettingsToKey(activePanel, actKey, FALSE);

    TIndirectArray<CFilesWindow>& tabs = GetPanelTabs(side);
    DWORD tabCount = tabs.Count;
    SetValue(actKey, PANEL_TABCOUNT_REG, REG_DWORD, &tabCount, sizeof(DWORD));

    int activeIndex = GetPanelTabIndex(side, activePanel);
    if (activeIndex < 0)
        activeIndex = 0;
    DWORD activeValue = (DWORD)activeIndex;
    SetValue(actKey, PANEL_ACTIVETAB_REG, REG_DWORD, &activeValue, sizeof(DWORD));

    for (int i = 0; i < tabs.Count; i++)
    {
        char tabKeyName[16];
        wsprintf(tabKeyName, "Tab%d", i + 1);
        HKEY tabKey;
        if (CreateKey(actKey, tabKeyName, tabKey))
        {
            SavePanelSettingsToKey(tabs[i], tabKey, TRUE);
            CPathHistory* history = tabs[i]->GetWorkDirHistory();
            BOOL onlyClear = !Configuration.SaveWorkDirs;
            if (history != NULL)
            {
                history->SaveToRegistry(tabKey, CONFIG_WORKDIRSHISTORY_REG, onlyClear);
            }
            else
            {
                HKEY historyKey;
                if (CreateKey(tabKey, CONFIG_WORKDIRSHISTORY_REG, historyKey))
                {
                    ClearKey(historyKey);
                    CloseKey(historyKey);
                }
            }
            CloseKey(tabKey);
        }
    }

    CloseKey(actKey);
}

void CMainWindow::SaveConfig(HWND parent, BOOL showConfigFileSaveError)
{
    CALL_STACK_MESSAGE1("CMainWindow::SaveConfig()");

    if (parent == NULL)
        parent = HWindow;

    if (SALAMANDER_ROOT_REG == NULL)
    {
        TRACE_E("SALAMANDER_ROOT_REG == NULL"); // not necessarily an error: during UPGRADE we may exit Salamander without saving the configuration (if not all plug-ins are installed and the user chooses Exit)
        return;
    }

    HCURSOR hOldCursor = NULL;
    if (GlobalSaveWaitWindow == NULL)
        hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    CWaitWindow analysing(parent, IDS_SAVINGCONFIGURATION, FALSE, ooStatic, TRUE);
    int savingProgress = 0;
    HWND oldPluginMsgBoxParent = PluginMsgBoxParent;
    if (GlobalSaveWaitWindow == NULL)
    {
        //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);
        analysing.SetProgressMax(7 /* MUST BE SYNCHRONIZED with CMainWindow::WindowProc::WM_USER_CLOSE_MAINWND !!! */); // one less so they can enjoy looking at 100%
        analysing.Create();
        EnableWindow(parent, FALSE);

        // SaveConfiguration plug-ins will be invoked as well -> set the parent for their message boxes
        PluginMsgBoxParent = analysing.HWindow;
    }

    LoadSaveToRegistryMutex.Enter();

    HKEY salamander;
    if (CreateKey(HKEY_CURRENT_USER, SALAMANDER_ROOT_REG, salamander))
    {
        HKEY actKey;

        BOOL cfgIsOK = TRUE;
        BOOL deleteSALAMANDER_SAVE_IN_PROGRESS = !IsSetSALAMANDER_SAVE_IN_PROGRESS;
        if (deleteSALAMANDER_SAVE_IN_PROGRESS)
        {
            DWORD saveInProgress = 1;
            if (GetValueAux(NULL, salamander, SALAMANDER_SAVE_IN_PROGRESS, REG_DWORD, &saveInProgress, sizeof(DWORD)))
            {                    // use GetValueAux so we don't show the "Load Configuration" message
                cfgIsOK = FALSE; // the configuration is corrupted; saving won't fix it (it wasn't stored completely)
                TRACE_E("CMainWindow::SaveConfig(): unable to save configuration, configuration key in registry is corrupted");
            }
            else
            {
                saveInProgress = 1;
                SetValue(salamander, SALAMANDER_SAVE_IN_PROGRESS, REG_DWORD, &saveInProgress, sizeof(DWORD));
                IsSetSALAMANDER_SAVE_IN_PROGRESS = TRUE;
            }
        }

        if (cfgIsOK)
        {
            //--- version
            if (CreateKey(salamander, SALAMANDER_VERSION_REG, actKey))
            {
                DWORD newConfigVersion = THIS_CONFIG_VERSION;
                SetValue(actKey, SALAMANDER_VERSIONREG_REG, REG_DWORD,
                         &newConfigVersion, sizeof(DWORD));
                CloseKey(actKey);
            }

            //---  window

            if (CreateKey(salamander, SALAMANDER_WINDOW_REG, actKey))
            {
                WINDOWPLACEMENT place;
                place.length = sizeof(WINDOWPLACEMENT);
                GetWindowPlacement(HWindow, &place);
                SetValue(actKey, WINDOW_LEFT_REG, REG_DWORD,
                         &(place.rcNormalPosition.left), sizeof(DWORD));
                SetValue(actKey, WINDOW_RIGHT_REG, REG_DWORD,
                         &(place.rcNormalPosition.right), sizeof(DWORD));
                SetValue(actKey, WINDOW_TOP_REG, REG_DWORD,
                         &(place.rcNormalPosition.top), sizeof(DWORD));
                SetValue(actKey, WINDOW_BOTTOM_REG, REG_DWORD,
                         &(place.rcNormalPosition.bottom), sizeof(DWORD));
                SetValue(actKey, WINDOW_SHOW_REG, REG_DWORD,
                         &(place.showCmd), sizeof(DWORD));
                char buf[20];
                sprintf(buf, "%.1lf", SplitPosition * 100);
                SetValue(actKey, WINDOW_SPLIT_REG, REG_SZ, buf, -1);
                sprintf(buf, "%.1lf", BeforeZoomSplitPosition * 100);
                SetValue(actKey, WINDOW_BEFOREZOOMSPLIT_REG, REG_SZ, buf, -1);

                if (DetachedPanels && HRightDetachedWindow != NULL)
                {
                    Configuration.DetachedWindowPlacement.length = sizeof(WINDOWPLACEMENT);
                    GetWindowPlacement(HRightDetachedWindow, &Configuration.DetachedWindowPlacement);
                    Configuration.DetachedPanels = TRUE;
                }
                DWORD detachedPanels = Configuration.DetachedPanels ? 1 : 0;
                SetValue(actKey, WINDOW_DETACHED_PANELS_REG, REG_DWORD, &detachedPanels, sizeof(DWORD));
                if (Configuration.DetachedWindowPlacement.length != 0)
                {
                    SetValue(actKey, WINDOW_DETACHED_LEFT_REG, REG_DWORD,
                             &(Configuration.DetachedWindowPlacement.rcNormalPosition.left), sizeof(DWORD));
                    SetValue(actKey, WINDOW_DETACHED_RIGHT_REG, REG_DWORD,
                             &(Configuration.DetachedWindowPlacement.rcNormalPosition.right), sizeof(DWORD));
                    SetValue(actKey, WINDOW_DETACHED_TOP_REG, REG_DWORD,
                             &(Configuration.DetachedWindowPlacement.rcNormalPosition.top), sizeof(DWORD));
                    SetValue(actKey, WINDOW_DETACHED_BOTTOM_REG, REG_DWORD,
                             &(Configuration.DetachedWindowPlacement.rcNormalPosition.bottom), sizeof(DWORD));
                    SetValue(actKey, WINDOW_DETACHED_SHOW_REG, REG_DWORD,
                             &(Configuration.DetachedWindowPlacement.showCmd), sizeof(DWORD));
                }

                CloseKey(actKey);
            }

            if (Configuration.FindDialogWindowPlacement.length != 0)
            {
                if (CreateKey(salamander, FINDDIALOG_WINDOW_REG, actKey))
                {
                    SetValue(actKey, WINDOW_LEFT_REG, REG_DWORD,
                             &(Configuration.FindDialogWindowPlacement.rcNormalPosition.left), sizeof(DWORD));
                    SetValue(actKey, WINDOW_RIGHT_REG, REG_DWORD,
                             &(Configuration.FindDialogWindowPlacement.rcNormalPosition.right), sizeof(DWORD));
                    SetValue(actKey, WINDOW_TOP_REG, REG_DWORD,
                             &(Configuration.FindDialogWindowPlacement.rcNormalPosition.top), sizeof(DWORD));
                    SetValue(actKey, WINDOW_BOTTOM_REG, REG_DWORD,
                             &(Configuration.FindDialogWindowPlacement.rcNormalPosition.bottom), sizeof(DWORD));
                    SetValue(actKey, WINDOW_SHOW_REG, REG_DWORD,
                             &(Configuration.FindDialogWindowPlacement.showCmd), sizeof(DWORD));

                    SetValue(actKey, FINDDIALOG_NAMEWIDTH_REG, REG_DWORD,
                             &(Configuration.FindColNameWidth), sizeof(DWORD));
                    CloseKey(actKey);
                }
            }

            //---  left and right panel

            SavePanelConfig(cpsLeft, salamander, SALAMANDER_LEFTP_REG);
            SavePanelConfig(cpsRight, salamander, SALAMANDER_RIGHTP_REG);

            //---  default directories

            if (CreateKey(salamander, SALAMANDER_DEFDIRS_REG, actKey))
            {
                char name[2];
                name[1] = 0;
                char d;
                for (d = 'A'; d <= 'Z'; d++)
                {
                    name[0] = d;
                    char* path = DefaultDir[d - 'A'];
                    if (path[1] == ':' && path[2] == '\\' && path[3] != 0) // not "C:\"
                        SetValue(actKey, name, REG_SZ, path, -1);
                    else
                        DeleteValue(actKey, name);
                }
                CloseKey(actKey);
            }

            //---  password manager

            if (CreateKey(salamander, SALAMANDER_PWDMNGR_REG, actKey))
            {
                PasswordManager.Save(actKey);
                CloseKey(actKey);
            }

            //---  hot paths

            if (CreateKey(salamander, SALAMANDER_HOTPATHS_REG, actKey))
            {
                HotPaths.Save(actKey);
                CloseKey(actKey);
            }

            //--- view templates

            if (CreateKey(salamander, SALAMANDER_VIEWTEMPLATES_REG, actKey))
            {
                ViewTemplates.Save(actKey);
                CloseKey(actKey);
            }

            //---  Plugins
            HKEY configKey;
            HKEY orderKey;
            if (CreateKey(salamander, SALAMANDER_PLUGINS, actKey) &&
                CreateKey(salamander, SALAMANDER_PLUGINSCONFIG, configKey) &&
                CreateKey(salamander, SALAMANDER_PLUGINSORDER, orderKey))
            {
                Plugins.Save(parent, actKey, configKey, orderKey);
                CloseKey(orderKey);
                CloseKey(actKey);
                CloseKey(configKey);
            }

            if (GlobalSaveWaitWindow == NULL)
                analysing.SetProgressPos(++savingProgress); // 1
            else
                GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 1
            //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

            //---  Packers & Unpackers
            if (CreateKey(salamander, SALAMANDER_PACKANDUNPACK, actKey))
            {
                SetValue(actKey, SALAMANDER_SIMPLEICONSINARCHIVES, REG_DWORD,
                         &(Configuration.UseSimpleIconsInArchives), sizeof(DWORD));

                //---  Custom Packers
                HKEY actSubKey;
                if (CreateKey(actKey, SALAMANDER_CUSTOMPACKERS, actSubKey))
                {
                    ClearKey(actSubKey);
                    HKEY itemKey;
                    char buf[30];
                    int i;
                    for (i = 0; i < PackerConfig.GetPackersCount(); i++)
                    {
                        itoa(i + 1, buf, 10);
                        if (CreateKey(actSubKey, buf, itemKey))
                        {
                            PackerConfig.Save(i, itemKey);
                            CloseKey(itemKey);
                        }
                        else
                            break;
                    }
                    SetValue(actSubKey, SALAMANDER_ANOTHERPANEL, REG_DWORD,
                             &(Configuration.UseAnotherPanelForPack), sizeof(DWORD));
                    int pp = PackerConfig.GetPreferedPacker();
                    SetValue(actSubKey, SALAMANDER_PREFFERED, REG_DWORD, &pp, sizeof(DWORD));
                    CloseKey(actSubKey);
                }

                if (GlobalSaveWaitWindow == NULL)
                    analysing.SetProgressPos(++savingProgress); // 2
                else
                    GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 2
                //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

                //---  Custom Unpackers
                if (CreateKey(actKey, SALAMANDER_CUSTOMUNPACKERS, actSubKey))
                {
                    ClearKey(actSubKey);
                    HKEY itemKey;
                    char buf[30];
                    int i;
                    for (i = 0; i < UnpackerConfig.GetUnpackersCount(); i++)
                    {
                        itoa(i + 1, buf, 10);
                        if (CreateKey(actSubKey, buf, itemKey))
                        {
                            UnpackerConfig.Save(i, itemKey);
                            CloseKey(itemKey);
                        }
                        else
                            break;
                    }
                    SetValue(actSubKey, SALAMANDER_ANOTHERPANEL, REG_DWORD,
                             &(Configuration.UseAnotherPanelForUnpack), sizeof(DWORD));
                    SetValue(actSubKey, SALAMANDER_NAMEBYARCHIVE, REG_DWORD,
                             &(Configuration.UseSubdirNameByArchiveForUnpack), sizeof(DWORD));
                    int pp = UnpackerConfig.GetPreferedUnpacker();
                    SetValue(actSubKey, SALAMANDER_PREFFERED, REG_DWORD, &pp, sizeof(DWORD));
                    CloseKey(actSubKey);
                }

                if (GlobalSaveWaitWindow == NULL)
                    analysing.SetProgressPos(++savingProgress); // 3
                else
                    GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 3
                //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

                //---  Predefined Packers
                if (CreateKey(actKey, SALAMANDER_PREDPACKERS, actSubKey))
                {
                    ClearKey(actSubKey);
                    HKEY itemKey;
                    char buf[30];
                    int i;
                    for (i = 0; i < ArchiverConfig.GetArchiversCount(); i++)
                    {
                        itoa(i + 1, buf, 10);
                        if (CreateKey(actSubKey, buf, itemKey))
                        {
                            ArchiverConfig.Save(i, itemKey);
                            CloseKey(itemKey);
                        }
                        else
                            break;
                    }
                    if (PackGetAutoconfigDrives() != NULL)
                        SetValue(actSubKey, SALAMANDER_AUTOCONFIGDRIVES_REG, REG_MULTI_SZ,
                                 PackGetAutoconfigDrives(), PackGetAutoconfigDrivesSize());
                    CloseKey(actSubKey);
                }

                //---  Archive Association
                if (CreateKey(actKey, SALAMANDER_ARCHIVEASSOC, actSubKey))
                {
                    ClearKey(actSubKey);
                    HKEY itemKey;
                    char buf[30];
                    int i;
                    for (i = 0; i < PackerFormatConfig.GetFormatsCount(); i++)
                    {
                        itoa(i + 1, buf, 10);
                        if (CreateKey(actSubKey, buf, itemKey))
                        {
                            PackerFormatConfig.Save(i, itemKey);
                            CloseKey(itemKey);
                        }
                        else
                            break;
                    }
                    CloseKey(actSubKey);
                }

                CloseKey(actKey);
            }

            //---  configuration

            if (CreateKey(salamander, SALAMANDER_CONFIG_REG, actKey))
            {
                //---  top rebar begin
                SetValue(actKey, CONFIG_MENUINDEX_REG, REG_DWORD,
                         &Configuration.MenuIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_MENUBREAK_REG, REG_DWORD,
                         &Configuration.MenuBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_MENUWIDTH_REG, REG_DWORD,
                         &Configuration.MenuWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_TOOLBARINDEX_REG, REG_DWORD,
                         &Configuration.TopToolbarIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_TOOLBARBREAK_REG, REG_DWORD,
                         &Configuration.TopToolbarBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_TOOLBARWIDTH_REG, REG_DWORD,
                         &Configuration.TopToolbarWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_PLUGINSBARINDEX_REG, REG_DWORD,
                         &Configuration.PluginsBarIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_PLUGINSBARBREAK_REG, REG_DWORD,
                         &Configuration.PluginsBarBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_PLUGINSBARWIDTH_REG, REG_DWORD,
                         &Configuration.PluginsBarWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_EXTENSIONBARINDEX_REG, REG_DWORD,
                         &Configuration.ExtensionBarIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_EXTENSIONBARBREAK_REG, REG_DWORD,
                         &Configuration.ExtensionBarBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_EXTENSIONBARWIDTH_REG, REG_DWORD,
                         &Configuration.ExtensionBarWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_USERMENUINDEX_REG, REG_DWORD,
                         &Configuration.UserMenuToolbarIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_USERMENUBREAK_REG, REG_DWORD,
                         &Configuration.UserMenuToolbarBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_USERMENUWIDTH_REG, REG_DWORD,
                         &Configuration.UserMenuToolbarWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_USERMENULABELS_REG, REG_DWORD,
                         &Configuration.UserMenuToolbarLabels, sizeof(DWORD));
                SetValue(actKey, CONFIG_HOTPATHSINDEX_REG, REG_DWORD,
                         &Configuration.HotPathsBarIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_HOTPATHSBREAK_REG, REG_DWORD,
                         &Configuration.HotPathsBarBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_HOTPATHSWIDTH_REG, REG_DWORD,
                         &Configuration.HotPathsBarWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_DRIVEBARINDEX_REG, REG_DWORD,
                         &Configuration.DriveBarIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_DRIVEBARBREAK_REG, REG_DWORD,
                         &Configuration.DriveBarBreak, sizeof(DWORD));
                SetValue(actKey, CONFIG_DRIVEBARWIDTH_REG, REG_DWORD,
                         &Configuration.DriveBarWidth, sizeof(DWORD));
                if (LeftPanel != NULL)
                {
                    Configuration.TreeViewWidth = LeftPanel->TreeViewWidth;
                    Configuration.TreeViewAutoHide = LeftPanel->TreeViewAutoHide;
                }
                if (RightPanel != NULL && DetachedPanels)
                {
                    Configuration.DetachedTreeViewWidth = RightPanel->TreeViewWidth;
                    Configuration.DetachedTreeViewAutoHide = RightPanel->TreeViewAutoHide;
                }
                SetValue(actKey, CONFIG_TREEVIEWWIDTH_REG, REG_DWORD,
                         &Configuration.TreeViewWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_TREEVIEWAUTOHIDE_REG, REG_DWORD,
                         &Configuration.TreeViewAutoHide, sizeof(DWORD));
                SetValue(actKey, CONFIG_DETACHEDTREEVIEWWIDTH_REG, REG_DWORD,
                         &Configuration.DetachedTreeViewWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_DETACHEDTREEVIEWAUTOHIDE_REG, REG_DWORD,
                         &Configuration.DetachedTreeViewAutoHide, sizeof(DWORD));
                SetValue(actKey, CONFIG_GRIPSVISIBLE_REG, REG_DWORD,
                         &Configuration.GripsVisible, sizeof(DWORD));

                //---  top rebar end
                SetValue(actKey, CONFIG_FILENAMEFORMAT_REG, REG_DWORD,
                         &Configuration.FileNameFormat, sizeof(DWORD));
                SetValue(actKey, CONFIG_SIZEFORMAT_REG, REG_DWORD,
                         &Configuration.SizeFormat, sizeof(DWORD));
                SetValue(actKey, CONFIG_SELECTION_REG, REG_DWORD,
                         &Configuration.IncludeDirs, sizeof(DWORD));
                SetValue(actKey, CONFIG_COPYFINDTEXT_REG, REG_DWORD,
                         &Configuration.CopyFindText, sizeof(DWORD));
                SetValue(actKey, CONFIG_CLEARREADONLY_REG, REG_DWORD,
                         &Configuration.ClearReadOnly, sizeof(DWORD));
                SetValue(actKey, CONFIG_PRIMARYCONTEXTMENU_REG, REG_DWORD,
                         &Configuration.PrimaryContextMenu, sizeof(DWORD));
                SetValue(actKey, CONFIG_NOTHIDDENSYSTEM_REG, REG_DWORD,
                         &Configuration.NotHiddenSystemFiles, sizeof(DWORD));
                SetValue(actKey, CONFIG_RECYCLEBIN_REG, REG_DWORD,
                         &Configuration.UseRecycleBin, sizeof(DWORD));
                SetValue(actKey, CONFIG_RECYCLEMASKS_REG, REG_SZ,
                         Configuration.RecycleMasks.GetMasksString(), -1);
                SetValue(actKey, CONFIG_SAVEONEXIT_REG, REG_DWORD,
                         &Configuration.AutoSave, sizeof(DWORD));
                SetValue(actKey, CONFIG_SHOWGREPERRORS_REG, REG_DWORD,
                         &Configuration.ShowGrepErrors, sizeof(DWORD));
                SetValue(actKey, CONFIG_FINDFULLROW_REG, REG_DWORD,
                         &Configuration.FindFullRowSelect, sizeof(DWORD));
                SetValue(actKey, CONFIG_MINBEEPWHENDONE_REG, REG_DWORD,
                         &Configuration.MinBeepWhenDone, sizeof(DWORD));
                SetValue(actKey, CONFIG_CLOSESHELL_REG, REG_DWORD,
                         &Configuration.CloseShell, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMMANDLINEAPP_REG, REG_SZ,
                         Configuration.CommandLineApplication, -1);
                SetValue(actKey, CONFIG_COMMANDLINEARGS_REG, REG_SZ,
                         Configuration.CommandLineArguments, -1);
                DWORD rightPanelFocused = (GetActivePanel() == RightPanel);
                SetValue(actKey, CONFIG_RIGHT_FOCUS_REG, REG_DWORD,
                         &rightPanelFocused, sizeof(DWORD));
                SetValue(actKey, CONFIG_ALWAYSONTOP_REG, REG_DWORD,
                         &Configuration.AlwaysOnTop, sizeof(DWORD));
                //      SetValue(actKey, CONFIG_FASTDIRMOVE_REG, REG_DWORD,
                //               &Configuration.FastDirectoryMove, sizeof(DWORD));
                SetValue(actKey, CONFIG_SORTUSESLOCALE_REG, REG_DWORD,
                         &Configuration.SortUsesLocale, sizeof(DWORD));
                SetValue(actKey, CONFIG_SORTDETECTNUMBERS_REG, REG_DWORD,
                         &Configuration.SortDetectNumbers, sizeof(DWORD));
                SetValue(actKey, CONFIG_SORTNEWERONTOP_REG, REG_DWORD,
                         &Configuration.SortNewerOnTop, sizeof(DWORD));
                SetValue(actKey, CONFIG_SORTDIRSBYNAME_REG, REG_DWORD,
                         &Configuration.SortDirsByName, sizeof(DWORD));
                SetValue(actKey, CONFIG_SORTDIRSBYEXT_REG, REG_DWORD,
                         &Configuration.SortDirsByExt, sizeof(DWORD));
                SetValue(actKey, CONFIG_SAVEHISTORY_REG, REG_DWORD,
                         &Configuration.SaveHistory, sizeof(DWORD));
                SetValue(actKey, CONFIG_SAVEWORKDIRS_REG, REG_DWORD,
                         &Configuration.SaveWorkDirs, sizeof(DWORD));
                SetValue(actKey, CONFIG_WORKDIRS_HISTORY_SCOPE_REG, REG_DWORD,
                         &Configuration.WorkDirsHistoryScope, sizeof(DWORD));
                SetValue(actKey, CONFIG_ENABLECMDLINEHISTORY_REG, REG_DWORD,
                         &Configuration.EnableCmdLineHistory, sizeof(DWORD));
                SetValue(actKey, CONFIG_SAVECMDLINEHISTORY_REG, REG_DWORD,
                         &Configuration.SaveCmdLineHistory, sizeof(DWORD));
                SetValue(actKey, CONFIG_BACKSPACEACTION_REG, REG_DWORD,
                         &Configuration.BackspaceAction, sizeof(DWORD));
                //      SetValue(actKey, CONFIG_LANTASTICCHECK_REG, REG_DWORD,
                //               &Configuration.LantasticCheck, sizeof(DWORD));
                SetValue(actKey, CONFIG_ONLYONEINSTANCE_REG, REG_DWORD,
                         &Configuration.OnlyOneInstance, sizeof(DWORD));
                SetValue(actKey, CONFIG_STATUSAREA_REG, REG_DWORD,
                         &Configuration.StatusArea, sizeof(DWORD));
                SetValue(actKey, CONFIG_FULLROWSELECT_REG, REG_DWORD,
                         &Configuration.FullRowSelect, sizeof(DWORD));
                SetValue(actKey, CONFIG_FULLROWHIGHLIGHT_REG, REG_DWORD,
                         &Configuration.FullRowHighlight, sizeof(DWORD));
                SetValue(actKey, CONFIG_USEICONTINCTURE_REG, REG_DWORD,
                         &Configuration.UseIconTincture, sizeof(DWORD));
                SetValue(actKey, CONFIG_PANELS_USETABS_REG, REG_DWORD,
                         &Configuration.UsePanelTabs, sizeof(DWORD));
                SetValue(actKey, CONFIG_SHOWPANELCAPTION_REG, REG_DWORD,
                         &Configuration.ShowPanelCaption, sizeof(DWORD));
                SetValue(actKey, CONFIG_SHOWPANELZOOM_REG, REG_DWORD,
                         &Configuration.ShowPanelZoom, sizeof(DWORD));
                SetValue(actKey, CONFIG_SINGLECLICK_REG, REG_DWORD,
                         &Configuration.SingleClick, sizeof(DWORD));
                //      SetValue(actKey, CONFIG_SHOWTIPOFTHEDAY_REG, REG_DWORD,
                //               &Configuration.ShowTipOfTheDay, sizeof(DWORD));
                //      SetValue(actKey, CONFIG_LASTTIPOFTHEDAY_REG, REG_DWORD,
                //               &Configuration.LastTipOfTheDay, sizeof(DWORD));
                SetValue(actKey, CONFIG_INFOLINECONTENT_REG, REG_SZ,
                         Configuration.InfoLineContent, -1);
                SetValue(actKey, CONFIG_IFPATHISINACCESSIBLEGOTOISMYDOCS_REG, REG_DWORD,
                         &Configuration.IfPathIsInaccessibleGoToIsMyDocs, sizeof(DWORD));
                SetValue(actKey, CONFIG_IFPATHISINACCESSIBLEGOTO_REG, REG_SZ,
                         Configuration.IfPathIsInaccessibleGoTo, -1);
                SetValue(actKey, CONFIG_HOTPATH_AUTOCONFIG, REG_DWORD,
                         &Configuration.HotPathAutoConfig, sizeof(DWORD));
                SetValue(actKey, CONFIG_LASTUSEDSPEEDLIM_REG, REG_DWORD,
                         &Configuration.LastUsedSpeedLimit, sizeof(DWORD));
                SetValue(actKey, CONFIG_QUICKSEARCHENTER_REG, REG_DWORD,
                         &Configuration.QuickSearchEnterAlt, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWMOUNTFOLDERS, REG_DWORD,
                         &Configuration.ChangeDriveShowMountFolders, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_MOUNTFOLDERS_MODE, REG_DWORD,
                         &Configuration.ChangeDriveMountFoldersMode, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_MOUNTFOLDERS_NAME, REG_DWORD,
                         &Configuration.ChangeDriveMountFoldersName, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_MOUNTFOLDERS_DRIVEBAR, REG_DWORD,
                         &Configuration.ChangeDriveMountFoldersDriveBar, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWMYDOC, REG_DWORD,
                         &Configuration.ChangeDriveShowMyDoc, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOW3DOBJECTS, REG_DWORD,
                         &Configuration.ChangeDriveShow3DObjects, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWDESKTOP, REG_DWORD,
                         &Configuration.ChangeDriveShowDesktop, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWDOWNLOADS, REG_DWORD,
                         &Configuration.ChangeDriveShowDownloads, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWMUSIC, REG_DWORD,
                         &Configuration.ChangeDriveShowMusic, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWPICTURES, REG_DWORD,
                         &Configuration.ChangeDriveShowPictures, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWVIDEOS, REG_DWORD,
                         &Configuration.ChangeDriveShowVideos, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWCLOUDSTOR, REG_DWORD,
                         &Configuration.ChangeDriveCloudStorage, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWANOTHER, REG_DWORD,
                         &Configuration.ChangeDriveShowAnother, sizeof(DWORD));
                SetValue(actKey, CONFIG_CHD_SHOWNET, REG_DWORD,
                         &Configuration.ChangeDriveShowNet, sizeof(DWORD));
                SetValue(actKey, CONFIG_SEARCHFILECONTENT, REG_DWORD,
                         &Configuration.SearchFileContent, sizeof(DWORD));
                SetValue(actKey, CONFIG_LASTPLUGINVER, REG_DWORD,
                         &Configuration.LastPluginVer, sizeof(DWORD));
                SetValue(actKey, CONFIG_LASTPLUGINVER_OP, REG_DWORD,
                         &Configuration.LastPluginVerOP, sizeof(DWORD));
                SetValue(actKey, CONFIG_USESALOPEN_REG, REG_DWORD,
                         &Configuration.UseSalOpen, sizeof(DWORD));
                SetValue(actKey, CONFIG_NETWAREFASTDIRMOVE_REG, REG_DWORD,
                         &Configuration.NetwareFastDirMove, sizeof(DWORD));
                if (Windows7AndLater)
                    SetValue(actKey, CONFIG_ASYNCCOPYALG_REG, REG_DWORD,
                             &Configuration.UseAsyncCopyAlg, sizeof(DWORD));
                SetValue(actKey, CONFIG_RELOAD_ENV_VARS_REG, REG_DWORD,
                         &Configuration.ReloadEnvVariables, sizeof(DWORD));
                SetValue(actKey, CONFIG_PATH_AUTOCOMPLETE_REG, REG_DWORD,
                         &Configuration.PathAutoComplete, sizeof(DWORD));
                SetValue(actKey, CONFIG_CREATEDIR_AUTOCOMPLETE_REG, REG_DWORD,
                         &Configuration.CreateDirAutoComplete, sizeof(DWORD));
                SetValue(actKey, CONFIG_QUICKRENAME_SELALL_REG, REG_DWORD,
                         &Configuration.QuickRenameSelectAll, sizeof(DWORD));
                SetValue(actKey, CONFIG_EDITNEW_SELALL_REG, REG_DWORD,
                         &Configuration.EditNewSelectAll, sizeof(DWORD));
                SetValue(actKey, CONFIG_SHIFTFORHOTPATHS_REG, REG_DWORD,
                         &Configuration.ShiftForHotPaths, sizeof(DWORD));
                SetValue(actKey, CONFIG_LANGUAGE_REG, REG_SZ,
                         Configuration.SLGName, -1);
                SetValue(actKey, CONFIG_USEALTLANGFORPLUGINS_REG, REG_DWORD,
                         &Configuration.UseAsAltSLGInOtherPlugins, sizeof(DWORD));
                SetValue(actKey, CONFIG_ALTLANGFORPLUGINS_REG, REG_SZ,
                         Configuration.AltPluginSLGName, -1);
                DWORD langChanged = (StrICmp(Configuration.SLGName, Configuration.LoadedSLGName) != 0); // TRUE if user changed Salamander language
                SetValue(actKey, CONFIG_LANGUAGECHANGED_REG, REG_DWORD, &langChanged, sizeof(DWORD));
                SetValue(actKey, CONFIG_SHOWSPLASHSCREEN_REG, REG_DWORD,
                         &Configuration.ShowSplashScreen, sizeof(DWORD));
                SetValue(actKey, CONFIG_CONVERSIONTABLE_REG, REG_SZ,
                         &Configuration.ConversionTable, -1);
                SetValue(actKey, CONFIG_SKILLLEVEL_REG, REG_DWORD,
                         &Configuration.SkillLevel, sizeof(DWORD));
                SetValue(actKey, CONFIG_TITLEBARSHOWPATH_REG, REG_DWORD,
                         &Configuration.TitleBarShowPath, sizeof(DWORD));
                SetValue(actKey, CONFIG_TITLEBARMODE_REG, REG_DWORD,
                         &Configuration.TitleBarMode, sizeof(DWORD));
                SetValue(actKey, CONFIG_TABCAPTIONMODE_REG, REG_DWORD,
                         &Configuration.TabCaptionMode, sizeof(DWORD));
                SetValue(actKey, CONFIG_TABCAPTIONALIGNMENT_REG, REG_DWORD,
                         &Configuration.TabCaptionAlignment, sizeof(DWORD));
                DWORD tabMinWidth = (Configuration.TabButtonMinWidth > 0)
                                          ? (DWORD)Configuration.TabButtonMinWidth
                                          : 0;
                DWORD tabMaxWidth = (Configuration.TabButtonMaxWidth > 0)
                                          ? (DWORD)Configuration.TabButtonMaxWidth
                                          : 0;
                SetValue(actKey, CONFIG_TABMINWIDTH_REG, REG_DWORD, &tabMinWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_TABMAXWIDTH_REG, REG_DWORD, &tabMaxWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_TABACTIVEBORDER_REG, REG_DWORD,
                         &Configuration.TabActiveBorder, sizeof(DWORD));
                DWORD tabActiveBorderColor = (DWORD)Configuration.TabActiveBorderColor;
                SetValue(actKey, CONFIG_TABACTIVEBORDERCOLOR_REG, REG_DWORD,
                         &tabActiveBorderColor, sizeof(DWORD));
                SetValue(actKey, CONFIG_TABCLOSEBUTTONACTIVE_REG, REG_DWORD,
                         &Configuration.TabCloseButtonActive, sizeof(DWORD));
                SetValue(actKey, CONFIG_TABCLOSEBUTTONALL_REG, REG_DWORD,
                         &Configuration.TabCloseButtonAll, sizeof(DWORD));
                SetValue(actKey, CONFIG_TITLEBARPREFIX_REG, REG_DWORD,
                         &Configuration.UseTitleBarPrefix, sizeof(DWORD));
                SetValue(actKey, CONFIG_TITLEBARPREFIXTEXT_REG, REG_SZ,
                         &Configuration.TitleBarPrefix, -1);
                SetValue(actKey, CONFIG_MAINWINDOWICONINDEX_REG, REG_DWORD,
                         &Configuration.MainWindowIconIndex, sizeof(DWORD));
                SetValue(actKey, CONFIG_CLICKQUICKRENAME_REG, REG_DWORD,
                         &Configuration.ClickQuickRename, sizeof(DWORD));
                SetValue(actKey, CONFIG_VISIBLEDRIVES_REG, REG_DWORD,
                         &Configuration.VisibleDrives, sizeof(DWORD));
                SetValue(actKey, CONFIG_SEPARATEDDRIVES_REG, REG_DWORD,
                         &Configuration.SeparatedDrives, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREBYTIME_REG, REG_DWORD,
                         &Configuration.CompareByTime, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREBYSIZE_REG, REG_DWORD,
                         &Configuration.CompareBySize, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREBYCONTENT_REG, REG_DWORD,
                         &Configuration.CompareByContent, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREBYATTR_REG, REG_DWORD,
                         &Configuration.CompareByAttr, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREBYSUBDIRS_REG, REG_DWORD,
                         &Configuration.CompareSubdirs, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREBYSUBDIRSATTR_REG, REG_DWORD,
                         &Configuration.CompareSubdirsAttr, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREONEPANELDIRS_REG, REG_DWORD,
                         &Configuration.CompareOnePanelDirs, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREMOREOPTIONS_REG, REG_DWORD,
                         &Configuration.CompareMoreOptions, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREIGNOREFILES_REG, REG_DWORD,
                         &Configuration.CompareIgnoreFiles, sizeof(DWORD));
                SetValue(actKey, CONFIG_COMPAREIGNOREDIRS_REG, REG_DWORD,
                         &Configuration.CompareIgnoreDirs, sizeof(DWORD));
                SetValue(actKey, CONFIG_CONFIGTIGNOREFILESMASKS_REG, REG_SZ,
                         Configuration.CompareIgnoreFilesMasks.GetMasksString(), -1);
                SetValue(actKey, CONFIG_CONFIGTIGNOREDIRSMASKS_REG, REG_SZ,
                         Configuration.CompareIgnoreDirsMasks.GetMasksString(), -1);

                SetValue(actKey, CONFIG_THUMBNAILSIZE_REG, REG_DWORD,
                         &Configuration.ThumbnailSize, sizeof(DWORD));
                SetValue(actKey, CONFIG_KEEPPLUGINSSORTED_REG, REG_DWORD,
                         &Configuration.KeepPluginsSorted, sizeof(DWORD));
                SetValue(actKey, CONFIG_SHOWSLGINCOMPLETE_REG, REG_DWORD,
                         &Configuration.ShowSLGIncomplete, sizeof(DWORD));

                // WARNING: when an icon overlay handler crashes, these values are written directly into the registry
                //         (prevents Salamander from becoming "unstartable"), see InformAboutIconOvrlsHanCrash()
                SetValue(actKey, CONFIG_ENABLECUSTICOVRLS_REG, REG_DWORD,
                         &Configuration.EnableCustomIconOverlays, sizeof(DWORD));
                SetValue(actKey, CONFIG_DISABLEDCUSTICOVRLS_REG, REG_SZ,
                         Configuration.DisabledCustomIconOverlays != NULL ? Configuration.DisabledCustomIconOverlays : "", -1);

                SetValue(actKey, CONFIG_EDITNEWFILE_USEDEFAULT_REG, REG_DWORD,
                         &Configuration.UseEditNewFileDefault, sizeof(DWORD));
                SetValue(actKey, CONFIG_EDITNEWFILE_DEFAULT_REG, REG_SZ,
                         Configuration.EditNewFileDefault, -1);

#ifndef _WIN64 // FIXME_X64_WINSCP
                SetValue(actKey, "Add x86-Only Plugins", REG_DWORD,
                         &Configuration.AddX86OnlyPlugins, sizeof(DWORD));
#endif // _WIN64

                HKEY actSubKey;
                if (CreateKey(actKey, SALAMANDER_CONFIRMATION_REG, actSubKey))
                {
                    SetValue(actSubKey, CONFIG_CNFRM_FILEDIRDEL, REG_DWORD,
                             &Configuration.CnfrmFileDirDel, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_NEDIRDEL, REG_DWORD,
                             &Configuration.CnfrmNEDirDel, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_FILEOVER, REG_DWORD,
                             &Configuration.CnfrmFileOver, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_DIROVER, REG_DWORD,
                             &Configuration.CnfrmDirOver, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_SHFILEDEL, REG_DWORD,
                             &Configuration.CnfrmSHFileDel, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_SHDIRDEL, REG_DWORD,
                             &Configuration.CnfrmSHDirDel, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_SHFILEOVER, REG_DWORD,
                             &Configuration.CnfrmSHFileOver, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_NTFSPRESS, REG_DWORD,
                             &Configuration.CnfrmNTFSPress, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_NTFSCRYPT, REG_DWORD,
                             &Configuration.CnfrmNTFSCrypt, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_DAD, REG_DWORD,
                             &Configuration.CnfrmDragDrop, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CLOSEARCHIVE, REG_DWORD,
                             &Configuration.CnfrmCloseArchive, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CLOSEFIND, REG_DWORD,
                             &Configuration.CnfrmCloseFind, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_STOPFIND, REG_DWORD,
                             &Configuration.CnfrmStopFind, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CREATETARGETPATH, REG_DWORD,
                             &Configuration.CnfrmCreatePath, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_ALWAYSONTOP, REG_DWORD,
                             &Configuration.CnfrmAlwaysOnTop, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_ONSALCLOSE, REG_DWORD,
                             &Configuration.CnfrmOnSalClose, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_DETACHCLOSE, REG_DWORD,
                             &Configuration.CnfrmDetachClose, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_SENDEMAIL, REG_DWORD,
                             &Configuration.CnfrmSendEmail, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_ADDTOARCHIVE, REG_DWORD,
                             &Configuration.CnfrmAddToArchive, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CREATEDIR, REG_DWORD,
                             &Configuration.CnfrmCreateDir, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CHANGEDIRTC, REG_DWORD,
                             &Configuration.CnfrmChangeDirTC, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_SHOWNAMETOCOMP, REG_DWORD,
                             &Configuration.CnfrmShowNamesToCompare, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_DSTSHIFTSIGNORED, REG_DWORD,
                             &Configuration.CnfrmDSTShiftsIgnored, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_DSTSHIFTSOCCURED, REG_DWORD,
                             &Configuration.CnfrmDSTShiftsOccured, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_COPYMOVEOPTIONSNS, REG_DWORD,
                             &Configuration.CnfrmCopyMoveOptionsNS, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CHANGEDIRHISTORYERR, REG_DWORD,
                             &Configuration.CnfrmChangeDirHistoryErr, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_CNFRM_CONFIRMDELETEEXTINFO, REG_DWORD,
                             &Configuration.CnfrmConfirmDeleteExtInfo, sizeof(DWORD));

                    CloseKey(actSubKey);
                }

                if (CreateKey(actKey, SALAMANDER_DRVSPEC_REG, actSubKey))
                {
                    SetValue(actSubKey, CONFIG_DRVSPEC_FLOPPY_MON, REG_DWORD,
                             &Configuration.DrvSpecFloppyMon, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_FLOPPY_SIMPLE, REG_DWORD,
                             &Configuration.DrvSpecFloppySimple, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_REMOVABLE_MON, REG_DWORD,
                             &Configuration.DrvSpecRemovableMon, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_REMOVABLE_SIMPLE, REG_DWORD,
                             &Configuration.DrvSpecRemovableSimple, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_FIXED_MON, REG_DWORD,
                             &Configuration.DrvSpecFixedMon, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_FIXED_SIMPLE, REG_DWORD,
                             &Configuration.DrvSpecFixedSimple, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_REMOTE_MON, REG_DWORD,
                             &Configuration.DrvSpecRemoteMon, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_REMOTE_SIMPLE, REG_DWORD,
                             &Configuration.DrvSpecRemoteSimple, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_REMOTE_ACT, REG_DWORD,
                             &Configuration.DrvSpecRemoteDoNotRefreshOnAct, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_CDROM_MON, REG_DWORD,
                             &Configuration.DrvSpecCDROMMon, sizeof(DWORD));
                    SetValue(actSubKey, CONFIG_DRVSPEC_CDROM_SIMPLE, REG_DWORD,
                             &Configuration.DrvSpecCDROMSimple, sizeof(DWORD));
                    CloseKey(actSubKey);
                }

                SetValue(actKey, CONFIG_TOPTOOLBAR_REG, REG_SZ, Configuration.TopToolBar, -1);
                SetValue(actKey, CONFIG_MIDDLETOOLBAR_REG, REG_SZ, Configuration.MiddleToolBar, -1);

                SetValue(actKey, CONFIG_LEFTTOOLBAR_REG, REG_SZ, Configuration.LeftToolBar, -1);
                SetValue(actKey, CONFIG_RIGHTTOOLBAR_REG, REG_SZ, Configuration.RightToolBar, -1);

                SetValue(actKey, CONFIG_TOPTOOLBARVISIBLE_REG, REG_DWORD,
                         &Configuration.TopToolBarVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_PLGTOOLBARVISIBLE_REG, REG_DWORD,
                         &Configuration.PluginsBarVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_EXTENSIONBARVISIBLE_REG, REG_DWORD,
                         &Configuration.ExtensionBarVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_MIDDLETOOLBARVISIBLE_REG, REG_DWORD,
                         &Configuration.MiddleToolBarVisible, sizeof(DWORD));

                SetValue(actKey, CONFIG_USERMENUTOOLBARVISIBLE_REG, REG_DWORD,
                         &Configuration.UserMenuToolBarVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_HOTPATHSBARVISIBLE_REG, REG_DWORD,
                         &Configuration.HotPathsBarVisible, sizeof(DWORD));

                SetValue(actKey, CONFIG_DRIVEBARVISIBLE_REG, REG_DWORD,
                         &Configuration.DriveBarVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_DRIVEBAR2VISIBLE_REG, REG_DWORD,
                         &Configuration.DriveBar2Visible, sizeof(DWORD));
                SetValue(actKey, CONFIG_TREEVIEWVISIBLE_REG, REG_DWORD,
                         &Configuration.TreeViewVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_PANELTOOLTIPS_REG, REG_DWORD,
                         &Configuration.PanelTooltips, sizeof(DWORD));

                SetValue(actKey, CONFIG_BOTTOMTOOLBARVISIBLE_REG, REG_DWORD,
                         &Configuration.BottomToolBarVisible, sizeof(DWORD));

                //      SetValue(actKey, CONFIG_SPACESELCALCSPACE, REG_DWORD,
                //               &Configuration.SpaceSelCalcSpace, sizeof(DWORD));
                SetValue(actKey, CONFIG_COUNTSIZESTAYONFILESYSTEM, REG_DWORD,
                         &Configuration.CountSizeStayOnFileSystem, sizeof(DWORD));
                SetValue(actKey, CONFIG_USETIMERESOLUTION, REG_DWORD,
                         &Configuration.UseTimeResolution, sizeof(DWORD));
                SetValue(actKey, CONFIG_TIMERESOLUTION, REG_DWORD,
                         &Configuration.TimeResolution, sizeof(DWORD));
                SetValue(actKey, CONFIG_IGNOREDSTSHIFTS, REG_DWORD,
                         &Configuration.IgnoreDSTShifts, sizeof(DWORD));
                SetValue(actKey, CONFIG_USEDRAGDROPMINTIME, REG_DWORD,
                         &Configuration.UseDragDropMinTime, sizeof(DWORD));
                SetValue(actKey, CONFIG_DRAGDROPMINTIME, REG_DWORD,
                         &Configuration.DragDropMinTime, sizeof(DWORD));

                SetValue(actKey, CONFIG_LASTFOCUSEDPAGE, REG_DWORD,
                         &Configuration.LastFocusedPage, sizeof(DWORD));
                SetValue(actKey, CONFIG_CONFIGURATION_HEIGHT, REG_DWORD,
                         &Configuration.ConfigurationHeight, sizeof(DWORD));
                SetValue(actKey, CONFIG_CONFIGURATION_WIDTH, REG_DWORD,
                         &Configuration.ConfigurationWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_CONFIGURATION_TREE_WIDTH, REG_DWORD,
                         &Configuration.ConfigurationTreeWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_CONFIGURATION_VIEWS_RIGHT_WIDTH, REG_DWORD,
                         &Configuration.ConfigurationViewsRightWidth, sizeof(DWORD));
                SetValue(actKey, CONFIG_VIEWANDEDITEXPAND, REG_DWORD,
                         &Configuration.ViewersAndEditorsExpanded, sizeof(DWORD));
                SetValue(actKey, CONFIG_PACKEPAND, REG_DWORD,
                         &Configuration.PackersAndUnpackersExpanded, sizeof(DWORD));

                SetValue(actKey, CONFIG_CMDLINE_REG, REG_DWORD, &EditPermanentVisible, sizeof(DWORD));
                SetValue(actKey, CONFIG_CMDLFOCUS_REG, REG_DWORD, &EditMode, sizeof(DWORD));

                SetValue(actKey, CONFIG_USECUSTOMPANELFONT_REG, REG_DWORD, &UseCustomPanelFont, sizeof(DWORD));
                SaveLogFont(actKey, CONFIG_PANELFONT_REG, &LogFont);

                if (GlobalSaveWaitWindow == NULL)
                    analysing.SetProgressPos(++savingProgress); // 4
                else
                    GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 4
                //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

                SaveHistory(actKey, CONFIG_NAMEDHISTORY_REG, FindNamedHistory,
                            FIND_NAMED_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_LOOKINHISTORY_REG, FindLookInHistory,
                            FIND_LOOKIN_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_GREPHISTORY_REG, FindGrepHistory,
                            FIND_GREP_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_SELECTHISTORY_REG, Configuration.SelectHistory,
                            SELECT_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_COPYHISTORY_REG, Configuration.CopyHistory,
                            COPY_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_CHANGEDIRHISTORY_REG, Configuration.ChangeDirHistory,
                            CHANGEDIR_HISTORY_SIZE, !Configuration.SaveHistory);

                if (GlobalSaveWaitWindow == NULL)
                    analysing.SetProgressPos(++savingProgress); // 5
                else
                    GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 5
                //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

                SaveHistory(actKey, CONFIG_VIEWERHISTORY_REG, ViewerHistory,
                            VIEWER_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_COMMANDHISTORY_REG, Configuration.EditHistory,
                            EDIT_HISTORY_SIZE, !(Configuration.SaveHistory && Configuration.EnableCmdLineHistory && Configuration.SaveCmdLineHistory));
                SaveHistory(actKey, CONFIG_FILELISTHISTORY_REG, Configuration.FileListHistory,
                            FILELIST_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_CREATEDIRHISTORY_REG, Configuration.CreateDirHistory,
                            CREATEDIR_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_QUICKRENAMEHISTORY_REG, Configuration.QuickRenameHistory,
                            QUICKRENAME_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_EDITNEWHISTORY_REG, Configuration.EditNewHistory,
                            EDITNEW_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_CONVERTHISTORY_REG, Configuration.ConvertHistory,
                            CONVERT_HISTORY_SIZE, !Configuration.SaveHistory);
                SaveHistory(actKey, CONFIG_FILTERHISTORY_REG, Configuration.FilterHistory,
                            FILTER_HISTORY_SIZE, !Configuration.SaveHistory);

                if (DirHistory != NULL)
                {
                    if (UsingSharedWorkDirHistory())
                        DirHistory->SaveToRegistry(actKey, CONFIG_WORKDIRSHISTORY_REG, !Configuration.SaveWorkDirs);
                    else
                        DirHistory->SaveToRegistry(actKey, CONFIG_WORKDIRSHISTORY_REG, TRUE);
                }

                if (GlobalSaveWaitWindow == NULL)
                    analysing.SetProgressPos(++savingProgress); // 6
                else
                    GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 6
                //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

                if (CreateKey(actKey, CONFIG_COPYMOVEOPTIONS_REG, actSubKey))
                {
                    CopyMoveOptions.Save(actSubKey);
                    CloseKey(actSubKey);
                }

                if (CreateKey(actKey, CONFIG_FINDOPTIONS_REG, actSubKey))
                {
                    FindOptions.Save(actSubKey);
                    CloseKey(actSubKey);
                }

                if (CreateKey(actKey, CONFIG_FINDIGNORE_REG, actSubKey))
                {
                    FindIgnore.Save(actSubKey);
                    CloseKey(actSubKey);
                }

                SetValue(actKey, CONFIG_FILELISTNAME_REG, REG_SZ, Configuration.FileListName, -1);
                SetValue(actKey, CONFIG_FILELISTAPPEND_REG, REG_DWORD, &Configuration.FileListAppend, sizeof(DWORD));
                SetValue(actKey, CONFIG_FILELISTDESTINATION_REG, REG_DWORD, &Configuration.FileListDestination, sizeof(DWORD));

                CloseKey(actKey);
            }

            //---  viewer

            if (CreateKey(salamander, SALAMANDER_VIEWER_REG, actKey))
            {
                SetValue(actKey, VIEWER_FINDFORWARD_REG, REG_DWORD,
                         &GlobalFindDialog.Forward, sizeof(DWORD));
                SetValue(actKey, VIEWER_FINDWHOLEWORDS_REG, REG_DWORD,
                         &GlobalFindDialog.WholeWords, sizeof(DWORD));
                SetValue(actKey, VIEWER_FINDCASESENSITIVE_REG, REG_DWORD,
                         &GlobalFindDialog.CaseSensitive, sizeof(DWORD));
                SetValue(actKey, VIEWER_FINDREGEXP_REG, REG_DWORD,
                         &GlobalFindDialog.Regular, sizeof(DWORD));
                SetValue(actKey, VIEWER_FINDTEXT_REG, REG_SZ, GlobalFindDialog.Text, -1);
                SetValue(actKey, VIEWER_FINDHEXMODE_REG, REG_DWORD,
                         &GlobalFindDialog.HexMode, sizeof(DWORD));

                SetValue(actKey, VIEWER_CONFIGCRLF_REG, REG_DWORD,
                         &Configuration.EOL_CRLF, sizeof(DWORD));
                SetValue(actKey, VIEWER_CONFIGCR_REG, REG_DWORD,
                         &Configuration.EOL_CR, sizeof(DWORD));
                SetValue(actKey, VIEWER_CONFIGLF_REG, REG_DWORD,
                         &Configuration.EOL_LF, sizeof(DWORD));
                SetValue(actKey, VIEWER_CONFIGNULL_REG, REG_DWORD,
                         &Configuration.EOL_NULL, sizeof(DWORD));
                SetValue(actKey, VIEWER_CONFIGTABSIZE_REG, REG_DWORD,
                         &Configuration.TabSize, sizeof(DWORD));
                SetValue(actKey, VIEWER_CONFIGDEFMODE_REG, REG_DWORD,
                         &Configuration.DefViewMode, sizeof(DWORD));
                SetValue(actKey, VIEWER_CONFIGTEXTMASK_REG, REG_SZ,
                         Configuration.TextModeMasks.GetMasksString(), -1);
                SetValue(actKey, VIEWER_CONFIGHEXMASK_REG, REG_SZ,
                         Configuration.HexModeMasks.GetMasksString(), -1);
                SetValue(actKey, VIEWER_CONFIGUSECUSTOMFONT_REG, REG_DWORD,
                         &UseCustomViewerFont, sizeof(DWORD));
                SaveLogFont(actKey, VIEWER_CONFIGFONT_REG, &ViewerLogFont);
                SetValue(actKey, VIEWER_WRAPTEXT_REG, REG_DWORD,
                         &Configuration.WrapText, sizeof(DWORD));
                SetValue(actKey, VIEWER_SHOWNUMBERS_REG, REG_DWORD,
                         &Configuration.ViewerShowLineNumbers, sizeof(DWORD));
                SetValue(actKey, VIEWER_SHOWSTATUS_REG, REG_DWORD,
                         &Configuration.ViewerShowStatusBar, sizeof(DWORD));
                SetValue(actKey, VIEWER_ZOOMPERCENT_REG, REG_DWORD,
                         &Configuration.ViewerZoomPercent, sizeof(DWORD));
                SetValue(actKey, VIEWER_CPAUTOSELECT_REG, REG_DWORD,
                         &Configuration.CodePageAutoSelect, sizeof(DWORD));
                SetValue(actKey, VIEWER_DEFAULTCONVERT_REG, REG_SZ, Configuration.DefaultConvert, -1);
                SetValue(actKey, VIEWER_AUTOCOPYSELECTION_REG, REG_DWORD,
                         &Configuration.AutoCopySelection, sizeof(DWORD));
                SetValue(actKey, VIEWER_GOTOOFFSETISHEX_REG, REG_DWORD,
                         &Configuration.GoToOffsetIsHex, sizeof(DWORD));

                SetValue(actKey, VIEWER_CONFIGSAVEWINPOS_REG, REG_DWORD,
                         &Configuration.SavePosition, sizeof(DWORD));
                if (Configuration.WindowPlacement.length != 0)
                {
                    SetValue(actKey, VIEWER_CONFIGWNDLEFT_REG, REG_DWORD,
                             &Configuration.WindowPlacement.rcNormalPosition.left, sizeof(DWORD));
                    SetValue(actKey, VIEWER_CONFIGWNDRIGHT_REG, REG_DWORD,
                             &Configuration.WindowPlacement.rcNormalPosition.right, sizeof(DWORD));
                    SetValue(actKey, VIEWER_CONFIGWNDTOP_REG, REG_DWORD,
                             &Configuration.WindowPlacement.rcNormalPosition.top, sizeof(DWORD));
                    SetValue(actKey, VIEWER_CONFIGWNDBOTTOM_REG, REG_DWORD,
                             &Configuration.WindowPlacement.rcNormalPosition.bottom, sizeof(DWORD));
                    SetValue(actKey, VIEWER_CONFIGWNDSHOW_REG, REG_DWORD,
                             &Configuration.WindowPlacement.showCmd, sizeof(DWORD));
                }

                CloseKey(actKey);
            }

            //---  user menu

            if (CreateKey(salamander, SALAMANDER_USERMENU_REG, actKey))
            {
                ClearKey(actKey);

                HKEY subKey;
                char buf[30];
                int i;
                for (i = 0; i < UserMenuItems->Count; i++)
                {
                    itoa(i + 1, buf, 10);
                    if (CreateKey(actKey, buf, subKey))
                    {
                        SetValue(subKey, USERMENU_ITEMNAME_REG, REG_SZ, UserMenuItems->At(i)->ItemName, -1);
                        SetValue(subKey, USERMENU_COMMAND_REG, REG_SZ, UserMenuItems->At(i)->UMCommand, -1);
                        SetValue(subKey, USERMENU_ARGUMENTS_REG, REG_SZ, UserMenuItems->At(i)->Arguments, -1);
                        SetValue(subKey, USERMENU_INITDIR_REG, REG_SZ, UserMenuItems->At(i)->InitDir, -1);
                        SetValue(subKey, USERMENU_SHELL_REG, REG_DWORD,
                                 &UserMenuItems->At(i)->ThroughShell, sizeof(DWORD));
                        SetValue(subKey, USERMENU_CLOSE_REG, REG_DWORD,
                                 &UserMenuItems->At(i)->CloseShell, sizeof(DWORD));
                        SetValue(subKey, USERMENU_USEWINDOW_REG, REG_DWORD,
                                 &UserMenuItems->At(i)->UseWindow, sizeof(DWORD));

                        SetValue(subKey, USERMENU_ICON_REG, REG_SZ, UserMenuItems->At(i)->Icon, -1);
                        SetValue(subKey, USERMENU_TYPE_REG, REG_DWORD,
                                 &UserMenuItems->At(i)->Type, sizeof(DWORD));
                        SetValue(subKey, USERMENU_SHOWINTOOLBAR_REG, REG_DWORD,
                                 &UserMenuItems->At(i)->ShowInToolbar, sizeof(DWORD));

                        CloseKey(subKey);
                    }
                    else
                        break;
                }
                CloseKey(actKey);
            }

            //---  internal ZIP packer

            if (Configuration.ConfigVersion < 6 && // only for old configurations, otherwise we neither create nor clear the key
                CreateKey(salamander, SALAMANDER_IZIP_REG, actKey))
            {
                ClearKey(actKey);

                CloseKey(actKey);

                DeleteKey(salamander, SALAMANDER_IZIP_REG);
            }

            //---  viewers

            SaveViewers(salamander, SALAMANDER_VIEWERS_REG, ViewerMasks);
            SaveViewers(salamander, SALAMANDER_ALTVIEWERS_REG, AltViewerMasks);

            //---  editors

            SaveEditors(salamander, SALAMANDER_EDITORS_REG, EditorMasks);

            if (GlobalSaveWaitWindow == NULL)
                analysing.SetProgressPos(++savingProgress); // 7
            else
                GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 7
            //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

            //---  colors
            if (CreateKey(salamander, SALAMANDER_CUSTOMCOLORS_REG, actKey))
            {
                char buff[10];
                int i;
                for (i = 0; i < NUMBER_OF_CUSTOMCOLORS; i++)
                {
                    itoa(i + 1, buff, 10);
                    SaveRGB(actKey, buff, CustomColors[i]);
                }

                CloseKey(actKey);
            }

            if (CreateKey(salamander, SALAMANDER_COLORS_REG, actKey))
            {
                DWORD scheme = 4; // custom
                if (Configuration.UseWindowsDarkMode)
                    scheme = 5;
                else if (CurrentColors == SalamanderColors)
                    scheme = 0;
                else if (CurrentColors == ExplorerColors)
                    scheme = 1;
                else if (CurrentColors == NortonColors)
                    scheme = 2;
                else if (CurrentColors == NavigatorColors)
                    scheme = 3;
                SetValue(actKey, SALAMANDER_CLRSCHEME_REG, REG_DWORD, &scheme, sizeof(DWORD));

                DWORD useWinDark = Configuration.UseWindowsDarkMode ? 1U : 0U;
                SetValue(actKey, SALAMANDER_CLR_USE_WIN_DARK_REG, REG_DWORD, &useWinDark, sizeof(DWORD));

                ConfigurationStorage.SaveUseWindowsDarkMode(Configuration.UseWindowsDarkMode);
                SaveInstalledPluginVersionsToBootstrap();

                SaveRGBF(actKey, SALAMANDER_CLR_FOCUS_ACTIVE_NORMAL_REG, UserColors[FOCUS_ACTIVE_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_FOCUS_ACTIVE_SELECTED_REG, UserColors[FOCUS_ACTIVE_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_FOCUS_INACTIVE_NORMAL_REG, UserColors[FOCUS_FG_INACTIVE_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_FOCUS_INACTIVE_SELECTED_REG, UserColors[FOCUS_FG_INACTIVE_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_FOCUS_BK_INACTIVE_NORMAL_REG, UserColors[FOCUS_BK_INACTIVE_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_FOCUS_BK_INACTIVE_SELECTED_REG, UserColors[FOCUS_BK_INACTIVE_SELECTED]);

                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_FG_NORMAL_REG, UserColors[ITEM_FG_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_FG_SELECTED_REG, UserColors[ITEM_FG_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_FG_FOCUSED_REG, UserColors[ITEM_FG_FOCUSED]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_FG_FOCSEL_REG, UserColors[ITEM_FG_FOCSEL]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_FG_HIGHLIGHT_REG, UserColors[ITEM_FG_HIGHLIGHT]);

                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_BK_NORMAL_REG, UserColors[ITEM_BK_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_BK_SELECTED_REG, UserColors[ITEM_BK_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_BK_FOCUSED_REG, UserColors[ITEM_BK_FOCUSED]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_BK_FOCSEL_REG, UserColors[ITEM_BK_FOCSEL]);
                SaveRGBF(actKey, SALAMANDER_CLR_ITEM_BK_HIGHLIGHT_REG, UserColors[ITEM_BK_HIGHLIGHT]);

                SaveRGBF(actKey, SALAMANDER_CLR_ICON_BLEND_SELECTED_REG, UserColors[ICON_BLEND_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_ICON_BLEND_FOCUSED_REG, UserColors[ICON_BLEND_FOCUSED]);
                SaveRGBF(actKey, SALAMANDER_CLR_ICON_BLEND_FOCSEL_REG, UserColors[ICON_BLEND_FOCSEL]);

                SaveRGBF(actKey, SALAMANDER_CLR_PROGRESS_FG_NORMAL_REG, UserColors[PROGRESS_FG_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_PROGRESS_FG_SELECTED_REG, UserColors[PROGRESS_FG_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_PROGRESS_BK_NORMAL_REG, UserColors[PROGRESS_BK_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_PROGRESS_BK_SELECTED_REG, UserColors[PROGRESS_BK_SELECTED]);

                SaveRGBF(actKey, SALAMANDER_CLR_HOT_PANEL_REG, UserColors[HOT_PANEL]);
                SaveRGBF(actKey, SALAMANDER_CLR_HOT_ACTIVE_REG, UserColors[HOT_ACTIVE]);
                SaveRGBF(actKey, SALAMANDER_CLR_HOT_INACTIVE_REG, UserColors[HOT_INACTIVE]);

                SaveRGBF(actKey, SALAMANDER_CLR_ACTIVE_CAPTION_FG_REG, UserColors[ACTIVE_CAPTION_FG]);
                SaveRGBF(actKey, SALAMANDER_CLR_ACTIVE_CAPTION_BK_REG, UserColors[ACTIVE_CAPTION_BK]);
                SaveRGBF(actKey, SALAMANDER_CLR_INACTIVE_CAPTION_FG_REG, UserColors[INACTIVE_CAPTION_FG]);
                SaveRGBF(actKey, SALAMANDER_CLR_INACTIVE_CAPTION_BK_REG, UserColors[INACTIVE_CAPTION_BK]);

                SaveRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_NORMAL_REG, UserColors[THUMBNAIL_FRAME_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_SELECTED_REG, UserColors[THUMBNAIL_FRAME_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_FOCUSED_REG, UserColors[THUMBNAIL_FRAME_FOCUSED]);
                SaveRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_FOCSEL_REG, UserColors[THUMBNAIL_FRAME_FOCSEL]);

                SaveRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_PATH_FG_REG, UserColors[AUTOCOMPLETE_PATH_FG]);
                SaveRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_PATH_BK_REG, UserColors[AUTOCOMPLETE_PATH_BK]);
                SaveRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_LIST_FG_REG, UserColors[AUTOCOMPLETE_LIST_FG]);
                SaveRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_LIST_BK_REG, UserColors[AUTOCOMPLETE_LIST_BK]);

                SaveRGBF(actKey, SALAMANDER_CLR_VIEWER_FG_NORMAL_REG, ViewerColors[VIEWER_FG_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_VIEWER_BK_NORMAL_REG, ViewerColors[VIEWER_BK_NORMAL]);
                SaveRGBF(actKey, SALAMANDER_CLR_VIEWER_FG_SELECTED_REG, ViewerColors[VIEWER_FG_SELECTED]);
                SaveRGBF(actKey, SALAMANDER_CLR_VIEWER_BK_SELECTED_REG, ViewerColors[VIEWER_BK_SELECTED]);

                // save colors for file highlighting
                HKEY hHltKey;
                if (CreateKey(actKey, SALAMANDER_HLT, hHltKey))
                {
                    ClearKey(hHltKey);
                    HKEY hSubKey;
                    char buf[30];
                    int i;
                    for (i = 0; i < HighlightMasks->Count; i++)
                    {
                        itoa(i + 1, buf, 10);
                        if (CreateKey(hHltKey, buf, hSubKey))
                        {
                            CHighlightMasksItem* item = HighlightMasks->At(i);
                            SetValue(hSubKey, SALAMANDER_HLT_ITEM_MASKS, REG_SZ, item->Masks->GetMasksString(), -1);
                            SetValue(hSubKey, SALAMANDER_HLT_ITEM_ATTR, REG_DWORD, &item->Attr, sizeof(DWORD));
                            SetValue(hSubKey, SALAMANDER_HLT_ITEM_VALIDATTR, REG_DWORD, &item->ValidAttr, sizeof(DWORD));

                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_NORMAL_REG, item->NormalFg);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_SELECTED_REG, item->SelectedFg);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_FOCUSED_REG, item->FocusedFg);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_FOCSEL_REG, item->FocSelFg);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_HIGHLIGHT_REG, item->HighlightFg);

                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_NORMAL_REG, item->NormalBk);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_SELECTED_REG, item->SelectedBk);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_FOCUSED_REG, item->FocusedBk);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_FOCSEL_REG, item->FocSelBk);
                            SaveRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_HIGHLIGHT_REG, item->HighlightBk);
                            CloseKey(hSubKey);
                        }
                    }
                    CloseKey(hHltKey);
                }
                CloseKey(actKey);
            }

            if (GlobalSaveWaitWindow == NULL)
                analysing.SetProgressPos(++savingProgress); // 8
            else
                GlobalSaveWaitWindow->SetProgressPos(++GlobalSaveWaitWindowProgress); // 8
            //TRACE_I("analysing.SetProgressPos() savingProgress="<<savingProgress);

            if (deleteSALAMANDER_SAVE_IN_PROGRESS)
            {
                DeleteValue(salamander, SALAMANDER_SAVE_IN_PROGRESS);
                IsSetSALAMANDER_SAVE_IN_PROGRESS = FALSE;
            }
        }
        CloseKey(salamander);
    }

    ConfigurationStorage.Flush(showConfigFileSaveError);

    LoadSaveToRegistryMutex.Leave();

    if (GlobalSaveWaitWindow == NULL)
    {
        EnableWindow(parent, TRUE);
        PluginMsgBoxParent = oldPluginMsgBoxParent;
        DestroyWindow(analysing.HWindow);
        SetCursor(hOldCursor);
    }
}

void CMainWindow::LoadPanelConfig(char* panelPath, CPanelSide side, HKEY hSalamander, const char* reg)
{
    if (panelPath != NULL)
        panelPath[0] = 0;

    HKEY actKey;
    if (!OpenKey(hSalamander, reg, actKey))
        return;

    if (side == cpsLeft)
        PanelConfigPathsRestoredLeft = FALSE;
    else
        PanelConfigPathsRestoredRight = FALSE;

    TIndirectArray<CFilesWindow>& tabs = GetPanelTabs(side);

    DWORD tabCountValue = 0;
    if (!GetValue(actKey, PANEL_TABCOUNT_REG, REG_DWORD, &tabCountValue, sizeof(DWORD)) || tabCountValue == 0)
    {
        CFilesWindow* panel = (side == cpsLeft) ? LeftPanel : RightPanel;
        if (panel != NULL)
        {
            LoadPanelSettingsFromKey(panel, actKey, panelPath, panelPath != NULL ? MAX_PATH : 0);
            UpdatePanelTabColor(panel);
        }
        CloseKey(actKey);
        return;
    }

    int tabCount = (int)tabCountValue;
    if (tabCount <= 0)
        tabCount = 1;
    if (!Configuration.UsePanelTabs)
        tabCount = 1;

    if (tabs.Count == 0)
    {
        CFilesWindow* panel = AddPanelTab(side, 0);
        if (panel != NULL)
        {
            DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
            if (!panel->Create(CWINDOW_CLASSNAME2, "",
                               style,
                               0, 0, 0, 0,
                               HWindow,
                               NULL,
                               HInstance,
                               panel))
            {
                TRACE_E("Unable to create panel window while loading configuration");
                int idx = GetPanelTabIndex(side, panel);
                if (idx >= 0)
                {
                    CTabWindow* tabWnd = GetPanelTabWindow(side);
                    if (tabWnd != NULL && tabWnd->HWindow != NULL)
                        tabWnd->RemoveTab(idx);
                    tabs.Delete(idx);
                }
                delete panel;
            }
        }
    }

    while (tabs.Count > tabCount)
    {
        CFilesWindow* toClose = tabs[tabs.Count - 1];
        ClosePanelTab(toClose, false);
    }

    while (tabs.Count < tabCount)
    {
        CFilesWindow* previous = (side == cpsLeft) ? LeftPanel : RightPanel;
        CFilesWindow* panel = AddPanelTab(side, tabs.Count);
        if (panel == NULL)
            break;

        DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        if (!panel->Create(CWINDOW_CLASSNAME2, "",
                           style,
                           0, 0, 0, 0,
                           HWindow,
                           NULL,
                           HInstance,
                           panel))
        {
            TRACE_E("Unable to create panel window while loading configuration");
            int idx = GetPanelTabIndex(side, panel);
            TIndirectArray<CFilesWindow>& localTabs = GetPanelTabs(side);
            if (idx >= 0)
            {
                CTabWindow* tabWnd = GetPanelTabWindow(side);
                if (tabWnd != NULL && tabWnd->HWindow != NULL)
                    tabWnd->RemoveTab(idx);
                localTabs.Delete(idx);
            }
            delete panel;
            if (previous != NULL)
                SwitchPanelTab(previous);
            else
                UpdatePanelTabVisibility(side);
            break;
        }
    }

    TIndirectArray<CFilesWindow>& localTabs = GetPanelTabs(side);

    DWORD activeValue = 0;
    if (!GetValue(actKey, PANEL_ACTIVETAB_REG, REG_DWORD, &activeValue, sizeof(DWORD)) ||
        activeValue >= (DWORD)localTabs.Count)
    {
        activeValue = 0;
    }
    int activeIndex = (int)activeValue;

    BOOL activeRestored = FALSE;
    for (int i = 0; i < localTabs.Count; i++)
    {
        CFilesWindow* panel = localTabs[i];
        char tabKeyName[16];
        wsprintf(tabKeyName, "Tab%d", i + 1);

        char path[2 * MAX_PATH];
        path[0] = 0;

        HKEY tabKey;
        if (OpenKey(actKey, tabKeyName, tabKey))
        {
            LoadPanelSettingsFromKey(panel, tabKey, path, _countof(path));
            if (Configuration.SaveWorkDirs)
            {
                CPathHistory* history = panel->EnsureWorkDirHistory();
                if (history != NULL)
                    history->LoadFromRegistry(tabKey, CONFIG_WORKDIRSHISTORY_REG);
            }
            else
                panel->ClearWorkDirHistory();
            CloseKey(tabKey);
        }
        else if (i == 0)
        {
            LoadPanelSettingsFromKey(panel, actKey, path, _countof(path));
            if (Configuration.SaveWorkDirs)
            {
                CPathHistory* history = panel->EnsureWorkDirHistory();
                if (history != NULL)
                    history->LoadFromRegistry(actKey, CONFIG_WORKDIRSHISTORY_REG);
            }
            else
                panel->ClearWorkDirHistory();
        }
        else
            panel->ClearWorkDirHistory();

        UpdatePanelTabColor(panel);
        UpdatePanelTabTitle(panel);
        if (Configuration.WorkDirsHistoryScope == wdhsPerTab)
            UpdateDirectoryLineHistoryState(panel);

        BOOL restored;
        if (i != activeIndex && IsDiskOrUNCPath(path))
        {
            // Hidden tabs do not need a directory listing during startup. Remember their disk path
            // cheaply and let SwitchPanelTab load the listing when the user first activates them.
            panel->SetPanelType(ptDisk);
            panel->SetPath(path);
            panel->NeedsRefreshOnActivation = TRUE;
            UpdatePanelTabTitle(panel);
            restored = TRUE;
        }
        else
            restored = RestorePanelPathFromConfig(this, panel, path);

        if (i == activeIndex)
        {
            activeRestored = restored;
            if (panelPath != NULL && IsDiskOrUNCPath(path))
                lstrcpyn(panelPath, path, MAX_PATH);
        }
    }

    CFilesWindow* activePanel = NULL;
    if (activeIndex >= 0 && activeIndex < localTabs.Count)
        activePanel = localTabs[activeIndex];
    else if (localTabs.Count > 0)
        activePanel = localTabs[0];

    if (activePanel != NULL)
    {
        SwitchPanelTab(activePanel);
        int sel = GetPanelTabIndex(side, activePanel);
        CTabWindow* tabWnd = GetPanelTabWindow(side);
        if (tabWnd != NULL && tabWnd->HWindow != NULL && sel >= 0)
            tabWnd->SetCurSel(sel);
    }

    UpdatePanelTabVisibility(side);

    if (side == cpsLeft)
        PanelConfigPathsRestoredLeft = activeRestored;
    else
        PanelConfigPathsRestoredRight = activeRestored;

    CloseKey(actKey);
}

void LoadIconOvrlsInfo(const char* root)
{
    HKEY hSalamander;
    if (OpenKey(HKEY_CURRENT_USER, root, hSalamander))
    {
        HKEY actKey;
        DWORD configVersion = 1; // this configuration is from version 1.52 or older
        if (OpenKey(hSalamander, SALAMANDER_VERSION_REG, actKey))
        {
            configVersion = 2; // this configuration is from version 1.6b1
            GetValue(actKey, SALAMANDER_VERSIONREG_REG, REG_DWORD,
                     &configVersion, sizeof(DWORD));
            CloseKey(actKey);
        }
        if (OpenKey(hSalamander, SALAMANDER_CONFIG_REG, actKey))
        {
            ClearListOfDisabledCustomIconOverlays();
            DWORD disabledCustomIconOverlaysBufSize;
            if (GetValue(actKey, CONFIG_ENABLECUSTICOVRLS_REG, REG_DWORD,
                         &Configuration.EnableCustomIconOverlays, sizeof(DWORD)) &&
                GetSize(actKey, CONFIG_DISABLEDCUSTICOVRLS_REG, REG_SZ, disabledCustomIconOverlaysBufSize))
            {
                if (disabledCustomIconOverlaysBufSize > 1) // <= 1 means an empty string, NULL is enough in that case
                {
                    Configuration.DisabledCustomIconOverlays = (char*)malloc(disabledCustomIconOverlaysBufSize);
                    if (Configuration.DisabledCustomIconOverlays == NULL)
                    {
                        TRACE_E(LOW_MEMORY);
                        Configuration.EnableCustomIconOverlays = FALSE; // for safety reasons (icon overlay handlers crash often)
                    }
                    else
                    {
                        if (!GetValue(actKey, CONFIG_DISABLEDCUSTICOVRLS_REG, REG_SZ,
                                      Configuration.DisabledCustomIconOverlays, disabledCustomIconOverlaysBufSize))
                        {
                            free(Configuration.DisabledCustomIconOverlays);
                            Configuration.DisabledCustomIconOverlays = NULL;
                            Configuration.EnableCustomIconOverlays = FALSE; // for safety reasons (icon overlay handlers crash often)
                        }
                    }
                }
            }
            else
            {
                if (configVersion >= 41) // if this value is missing in newer configurations, disable overlays (older versions didn't have these variables, so it's not an error-leave overlays enabled)
                    Configuration.EnableCustomIconOverlays = FALSE;
            }

            CloseKey(actKey);
        }
        CloseKey(hSalamander);
    }
}

BOOL CMainWindow::LoadConfig(BOOL importingOldConfig, const CCommandLineParams* cmdLineParams)
{
    CALL_STACK_MESSAGE2("CMainWindow::LoadConfig(%d)", importingOldConfig);
    if (SALAMANDER_ROOT_REG == NULL)
        return FALSE;

    LoadSaveToRegistryMutex.Enter();

    HKEY salamander;
    if (OpenKey(HKEY_CURRENT_USER, SALAMANDER_ROOT_REG, salamander))
    {
        HKEY actKey;
        BOOL ret = TRUE;

        IfExistSetSplashScreenText(LoadStr(IDS_STARTUP_CONFIG));

        Configuration.ConfigVersion = 1; // this configuration is from version 1.52 or older
                                         //--- version
        if (OpenKey(salamander, SALAMANDER_VERSION_REG, actKey))
        {
            Configuration.ConfigVersion = 2; // this configuration is from version 1.6b1
            GetValue(actKey, SALAMANDER_VERSIONREG_REG, REG_DWORD,
                     &Configuration.ConfigVersion, sizeof(DWORD));
            CloseKey(actKey);
        }

        //---  viewers

        EnterViewerMasksCS();
        LoadViewers(salamander, SALAMANDER_VIEWERS_REG, ViewerMasks);
        LeaveViewerMasksCS();
        LoadViewers(salamander, SALAMANDER_ALTVIEWERS_REG, AltViewerMasks);

        //---  editors

        LoadEditors(salamander, SALAMANDER_EDITORS_REG, EditorMasks);

        //---  colors
        if (OpenKey(salamander, SALAMANDER_CUSTOMCOLORS_REG, actKey))
        {
            char buff[10];
            int i;
            for (i = 0; i < NUMBER_OF_CUSTOMCOLORS; i++)
            {
                itoa(i + 1, buff, 10);
                LoadRGB(actKey, buff, CustomColors[i]);
            }

            CloseKey(actKey);
        }

        if (OpenKey(salamander, SALAMANDER_COLORS_REG, actKey))
        {
            DWORD scheme = 4; // custom
            BOOL colorSchemeLoaded = FALSE;
            BOOL restoreWindowsDarkPalette = FALSE;
            BOOL migrateSamandarin01To06Scheme = MCDShouldMigrateSamandarin01To06Scheme(SALAMANDER_ROOT_REG, NULL);
            CurrentColors = UserColors;
            DWORD useWinDark = Configuration.UseWindowsDarkMode ? 1U : 0U;
            BOOL useWinDarkLoaded = GetValue(actKey, SALAMANDER_CLR_USE_WIN_DARK_REG, REG_DWORD, &useWinDark, sizeof(DWORD));
            if (useWinDarkLoaded)
                Configuration.UseWindowsDarkMode = useWinDark != 0;
            if (GetValue(actKey, SALAMANDER_CLRSCHEME_REG, REG_DWORD, &scheme, sizeof(DWORD)))
            {
                // we added a new scheme (DOS Navigator) at position 3
                if (Configuration.ConfigVersion < 28 && scheme == 3)
                    scheme = 4;

                if (migrateSamandarin01To06Scheme && scheme == 4 && Configuration.UseWindowsDarkMode)
                    scheme = 5; // samandarin 0.1-0.6 stored Windows Dark Mode as 4
                else if (migrateSamandarin01To06Scheme && scheme == 5 && !Configuration.UseWindowsDarkMode)
                    scheme = 4; // samandarin 0.1-0.6 stored Custom as 5

                if (scheme == 0)
                {
                    CurrentColors = SalamanderColors;
                    Configuration.UseWindowsDarkMode = FALSE;
                }
                else if (scheme == 1)
                {
                    CurrentColors = ExplorerColors;
                    Configuration.UseWindowsDarkMode = FALSE;
                }
                else if (scheme == 2)
                {
                    CurrentColors = NortonColors;
                    Configuration.UseWindowsDarkMode = FALSE;
                }
                else if (scheme == 3)
                {
                    CurrentColors = NavigatorColors;
                    Configuration.UseWindowsDarkMode = FALSE;
                }
                else if (scheme == 4)
                {
                    CurrentColors = UserColors;
                    Configuration.UseWindowsDarkMode = FALSE;
                }
                else if (scheme == 5)
                {
                    CurrentColors = UserColors;
                    Configuration.UseWindowsDarkMode = TRUE;
                }
                else
                {
                    CurrentColors = UserColors;
                    Configuration.UseWindowsDarkMode = FALSE;
                }
                colorSchemeLoaded = TRUE;
            }

            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_FG_NORMAL_REG, UserColors[ITEM_FG_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_FG_SELECTED_REG, UserColors[ITEM_FG_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_FG_FOCUSED_REG, UserColors[ITEM_FG_FOCUSED]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_FG_FOCSEL_REG, UserColors[ITEM_FG_FOCSEL]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_FG_HIGHLIGHT_REG, UserColors[ITEM_FG_HIGHLIGHT]);

            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_BK_NORMAL_REG, UserColors[ITEM_BK_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_BK_SELECTED_REG, UserColors[ITEM_BK_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_BK_FOCUSED_REG, UserColors[ITEM_BK_FOCUSED]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_BK_FOCSEL_REG, UserColors[ITEM_BK_FOCSEL]);
            LoadRGBF(actKey, SALAMANDER_CLR_ITEM_BK_HIGHLIGHT_REG, UserColors[ITEM_BK_HIGHLIGHT]);

            LoadRGBF(actKey, SALAMANDER_CLR_FOCUS_ACTIVE_NORMAL_REG, UserColors[FOCUS_ACTIVE_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_FOCUS_ACTIVE_SELECTED_REG, UserColors[FOCUS_ACTIVE_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_FOCUS_INACTIVE_NORMAL_REG, UserColors[FOCUS_FG_INACTIVE_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_FOCUS_INACTIVE_SELECTED_REG, UserColors[FOCUS_FG_INACTIVE_SELECTED]);
            if (!LoadRGBF(actKey, SALAMANDER_CLR_FOCUS_BK_INACTIVE_NORMAL_REG, UserColors[FOCUS_BK_INACTIVE_NORMAL]))
                UserColors[FOCUS_BK_INACTIVE_NORMAL] = UserColors[ITEM_BK_NORMAL]; // conversion of older configurations
            if (!LoadRGBF(actKey, SALAMANDER_CLR_FOCUS_BK_INACTIVE_SELECTED_REG, UserColors[FOCUS_BK_INACTIVE_SELECTED]))
                UserColors[FOCUS_BK_INACTIVE_SELECTED] = UserColors[ITEM_BK_NORMAL]; // conversion of older configurations

            LoadRGBF(actKey, SALAMANDER_CLR_ICON_BLEND_SELECTED_REG, UserColors[ICON_BLEND_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_ICON_BLEND_FOCUSED_REG, UserColors[ICON_BLEND_FOCUSED]);
            LoadRGBF(actKey, SALAMANDER_CLR_ICON_BLEND_FOCSEL_REG, UserColors[ICON_BLEND_FOCSEL]);

            LoadRGBF(actKey, SALAMANDER_CLR_PROGRESS_FG_NORMAL_REG, UserColors[PROGRESS_FG_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_PROGRESS_FG_SELECTED_REG, UserColors[PROGRESS_FG_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_PROGRESS_BK_NORMAL_REG, UserColors[PROGRESS_BK_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_PROGRESS_BK_SELECTED_REG, UserColors[PROGRESS_BK_SELECTED]);

            LoadRGBF(actKey, SALAMANDER_CLR_HOT_PANEL_REG, UserColors[HOT_PANEL]);
            LoadRGBF(actKey, SALAMANDER_CLR_HOT_ACTIVE_REG, UserColors[HOT_ACTIVE]);
            LoadRGBF(actKey, SALAMANDER_CLR_HOT_INACTIVE_REG, UserColors[HOT_INACTIVE]);

            LoadRGBF(actKey, SALAMANDER_CLR_ACTIVE_CAPTION_FG_REG, UserColors[ACTIVE_CAPTION_FG]);
            LoadRGBF(actKey, SALAMANDER_CLR_ACTIVE_CAPTION_BK_REG, UserColors[ACTIVE_CAPTION_BK]);
            LoadRGBF(actKey, SALAMANDER_CLR_INACTIVE_CAPTION_FG_REG, UserColors[INACTIVE_CAPTION_FG]);
            LoadRGBF(actKey, SALAMANDER_CLR_INACTIVE_CAPTION_BK_REG, UserColors[INACTIVE_CAPTION_BK]);

            LoadRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_NORMAL_REG, UserColors[THUMBNAIL_FRAME_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_SELECTED_REG, UserColors[THUMBNAIL_FRAME_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_FOCUSED_REG, UserColors[THUMBNAIL_FRAME_FOCUSED]);
            LoadRGBF(actKey, SALAMANDER_CLR_THUMBNAIL_FRAME_FOCSEL_REG, UserColors[THUMBNAIL_FRAME_FOCSEL]);

            if (!LoadRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_PATH_FG_REG, UserColors[AUTOCOMPLETE_PATH_FG]))
                UserColors[AUTOCOMPLETE_PATH_FG] = RGBF(255, 255, 255, 0);
            if (!LoadRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_PATH_BK_REG, UserColors[AUTOCOMPLETE_PATH_BK]))
                UserColors[AUTOCOMPLETE_PATH_BK] = RGBF(0, 0, 128, SCF_DEFAULT);
            if (!LoadRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_LIST_FG_REG, UserColors[AUTOCOMPLETE_LIST_FG]))
                UserColors[AUTOCOMPLETE_LIST_FG] = RGBF(255, 255, 255, 0);
            if (!LoadRGBF(actKey, SALAMANDER_CLR_AUTOCOMPLETE_LIST_BK_REG, UserColors[AUTOCOMPLETE_LIST_BK]))
                UserColors[AUTOCOMPLETE_LIST_BK] = RGBF(0, 0, 128, SCF_DEFAULT);

            LoadRGBF(actKey, SALAMANDER_CLR_VIEWER_FG_NORMAL_REG, ViewerColors[VIEWER_FG_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_VIEWER_BK_NORMAL_REG, ViewerColors[VIEWER_BK_NORMAL]);
            LoadRGBF(actKey, SALAMANDER_CLR_VIEWER_FG_SELECTED_REG, ViewerColors[VIEWER_FG_SELECTED]);
            LoadRGBF(actKey, SALAMANDER_CLR_VIEWER_BK_SELECTED_REG, ViewerColors[VIEWER_BK_SELECTED]);

            if (colorSchemeLoaded && scheme == 5 && (!useWinDarkLoaded || Configuration.UseWindowsDarkMode))
            {
                SALCOLOR windowsDarkColors[NUMBER_OF_COLORS];
                SALCOLOR windowsDarkViewerColors[NUMBER_OF_VIEWERCOLORS];
                memset(windowsDarkColors, 0, sizeof(windowsDarkColors));
                memset(windowsDarkViewerColors, 0, sizeof(windowsDarkViewerColors));
                WindowsDarkModeBuildPalette(windowsDarkColors, windowsDarkViewerColors);

                // A file-storage configuration can contain the Windows dark scheme and dark-mode flag
                // while the serialized palette falls back to the light defaults. In that state the
                // non-client/menu areas are dark, but the panels stay white. Treat scheme 5 as the
                // authoritative Windows dark preset and repair the panel/viewer palette before applying it.
                if (!useWinDarkLoaded ||
                    UserColors[ITEM_FG_NORMAL] != windowsDarkColors[ITEM_FG_NORMAL] ||
                    UserColors[ITEM_BK_NORMAL] != windowsDarkColors[ITEM_BK_NORMAL] ||
                    ViewerColors[VIEWER_FG_NORMAL] != windowsDarkViewerColors[VIEWER_FG_NORMAL] ||
                    ViewerColors[VIEWER_BK_NORMAL] != windowsDarkViewerColors[VIEWER_BK_NORMAL])
                {
                    memcpy(UserColors, windowsDarkColors, sizeof(UserColors));
                    memcpy(ViewerColors, windowsDarkViewerColors, sizeof(ViewerColors));
                    restoreWindowsDarkPalette = TRUE;
                }
                Configuration.UseWindowsDarkMode = TRUE;
            }

            // load colors for file highlighting
            HKEY hHltKey;
            if (OpenKey(actKey, SALAMANDER_HLT, hHltKey))
            {
                HKEY hSubKey;
                char buf[30];
                strcpy(buf, "1");
                int i = 1;
                HighlightMasks->DestroyMembers();
                while (OpenKey(hHltKey, buf, hSubKey))
                {
                    char masks[MAX_PATH];
                    if (GetValue(hSubKey, SALAMANDER_HLT_ITEM_MASKS, REG_SZ, masks, MAX_PATH))
                    {
                        CHighlightMasksItem* item = new CHighlightMasksItem();
                        if (item == NULL || !item->Set(masks))
                        {
                            TRACE_E(LOW_MEMORY);
                            if (item != NULL)
                                delete item;
                            continue;
                        }
                        int errPos;
                        item->Masks->PrepareMasks(errPos);

                        GetValue(hSubKey, SALAMANDER_HLT_ITEM_ATTR, REG_DWORD, &item->Attr, sizeof(DWORD));
                        GetValue(hSubKey, SALAMANDER_HLT_ITEM_VALIDATTR, REG_DWORD, &item->ValidAttr, sizeof(DWORD));

                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_NORMAL_REG, item->NormalFg);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_SELECTED_REG, item->SelectedFg);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_FOCUSED_REG, item->FocusedFg);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_FOCSEL_REG, item->FocSelFg);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_FG_HIGHLIGHT_REG, item->HighlightFg);

                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_NORMAL_REG, item->NormalBk);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_SELECTED_REG, item->SelectedBk);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_FOCUSED_REG, item->FocusedBk);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_FOCSEL_REG, item->FocSelBk);
                        LoadRGBF(hSubKey, SALAMANDER_HLT_ITEM_BK_HIGHLIGHT_REG, item->HighlightBk);
                        HighlightMasks->Add(item);
                        if (!HighlightMasks->IsGood())
                        {
                            HighlightMasks->ResetState();
                            delete item;
                        }
                        itoa(++i, buf, 10);
                        CloseKey(hSubKey);
                    }
                }
                if (Configuration.ConfigVersion < 16) // add highlighting for encrypted files/directories
                {
                    CHighlightMasksItem* hItem = new CHighlightMasksItem();
                    if (hItem != NULL)
                    {
                        HighlightMasks->Add(hItem);
                        hItem->Set("*.*");
                        int errPos;
                        hItem->Masks->PrepareMasks(errPos);
                        hItem->NormalFg = RGBF(19, 143, 13, 0); // color taken from Windows XP
                        hItem->FocusedFg = RGBF(19, 143, 13, 0);
                        hItem->ValidAttr = FILE_ATTRIBUTE_ENCRYPTED;
                        hItem->Attr = FILE_ATTRIBUTE_ENCRYPTED;
                    }
                }
                CloseKey(hHltKey);
            }
            if (restoreWindowsDarkPalette)
                WindowsDarkModeBuildHighlightMasks(HighlightMasks);

            ColorsChanged(FALSE, !Configuration.UseWindowsDarkMode, TRUE); // Windows Dark Mode changes toolbar SVGs/backgrounds, so reload icons

            CloseKey(actKey);
        }

        //---  window

        WINDOWPLACEMENT place;
        BOOL useWinPlacement = FALSE;
        if (OpenKey(salamander, SALAMANDER_WINDOW_REG, actKey))
        {
            place.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(HWindow, &place);
            if (GetValue(actKey, WINDOW_LEFT_REG, REG_DWORD,
                         &(place.rcNormalPosition.left), sizeof(DWORD)) &&
                GetValue(actKey, WINDOW_RIGHT_REG, REG_DWORD,
                         &(place.rcNormalPosition.right), sizeof(DWORD)) &&
                GetValue(actKey, WINDOW_TOP_REG, REG_DWORD,
                         &(place.rcNormalPosition.top), sizeof(DWORD)) &&
                GetValue(actKey, WINDOW_BOTTOM_REG, REG_DWORD,
                         &(place.rcNormalPosition.bottom), sizeof(DWORD)) &&
                GetValue(actKey, WINDOW_SHOW_REG, REG_DWORD,
                         &(place.showCmd), sizeof(DWORD)))
            {
                char buf[20];
                if (GetValue(actKey, WINDOW_SPLIT_REG, REG_SZ, buf, 20))
                {
                    sscanf(buf, "%lf", &SplitPosition);
                    SplitPosition /= 100;
                    if (SplitPosition < 0)
                        SplitPosition = 0;
                    if (SplitPosition > 1)
                        SplitPosition = 1;
                }
                if (GetValue(actKey, WINDOW_BEFOREZOOMSPLIT_REG, REG_SZ, buf, 20))
                {
                    sscanf(buf, "%lf", &BeforeZoomSplitPosition);
                    BeforeZoomSplitPosition /= 100;
                    if (BeforeZoomSplitPosition < 0)
                        BeforeZoomSplitPosition = 0;
                    if (BeforeZoomSplitPosition > 1)
                        BeforeZoomSplitPosition = 1;
                    BeforeZoomVisibleLeftRatio = BeforeZoomSplitPosition;
                }
                DWORD detachedPanels = 0;
                if (GetValue(actKey, WINDOW_DETACHED_PANELS_REG, REG_DWORD, &detachedPanels, sizeof(DWORD)))
                    Configuration.DetachedPanels = detachedPanels != 0;
                BOOL detachedPlacementExists = TRUE;
                Configuration.DetachedWindowPlacement.length = 0;
                detachedPlacementExists &= GetValue(actKey, WINDOW_DETACHED_LEFT_REG, REG_DWORD,
                                                    &Configuration.DetachedWindowPlacement.rcNormalPosition.left, sizeof(DWORD));
                detachedPlacementExists &= GetValue(actKey, WINDOW_DETACHED_RIGHT_REG, REG_DWORD,
                                                    &Configuration.DetachedWindowPlacement.rcNormalPosition.right, sizeof(DWORD));
                detachedPlacementExists &= GetValue(actKey, WINDOW_DETACHED_TOP_REG, REG_DWORD,
                                                    &Configuration.DetachedWindowPlacement.rcNormalPosition.top, sizeof(DWORD));
                detachedPlacementExists &= GetValue(actKey, WINDOW_DETACHED_BOTTOM_REG, REG_DWORD,
                                                    &Configuration.DetachedWindowPlacement.rcNormalPosition.bottom, sizeof(DWORD));
                detachedPlacementExists &= GetValue(actKey, WINDOW_DETACHED_SHOW_REG, REG_DWORD,
                                                    &Configuration.DetachedWindowPlacement.showCmd, sizeof(DWORD));
                if (detachedPlacementExists)
                    Configuration.DetachedWindowPlacement.length = sizeof(WINDOWPLACEMENT);
                useWinPlacement = TRUE;
            }
            else
                ret = FALSE;

            CloseKey(actKey);
        }
        else
            ret = FALSE;

        if (useWinPlacement)
        {
            RECT startupRect = place.rcNormalPosition;
            if (startupRect.right > startupRect.left && startupRect.bottom > startupRect.top &&
                MonitorFromRect(&startupRect, MONITOR_DEFAULTTONULL) != NULL)
            {
                // Move the hidden main window to its saved monitor before panels
                // and icon caches are loaded.  Otherwise a process started on a
                // high-DPI primary monitor can initialize 24px shell icons and
                // only later move to the remembered 100% monitor.
                SetWindowPos(HWindow, NULL, startupRect.left, startupRect.top,
                             startupRect.right - startupRect.left, startupRect.bottom - startupRect.top,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW);
                UpdateSystemDPIForWindow(HWindow);
                ColorsChanged(FALSE, FALSE, TRUE);
            }
        }

        if (OpenKey(salamander, FINDDIALOG_WINDOW_REG, actKey))
        {
            Configuration.FindDialogWindowPlacement.length = sizeof(WINDOWPLACEMENT);

            GetValue(actKey, WINDOW_LEFT_REG, REG_DWORD,
                     &(Configuration.FindDialogWindowPlacement.rcNormalPosition.left), sizeof(DWORD));
            GetValue(actKey, WINDOW_RIGHT_REG, REG_DWORD,
                     &(Configuration.FindDialogWindowPlacement.rcNormalPosition.right), sizeof(DWORD));
            GetValue(actKey, WINDOW_TOP_REG, REG_DWORD,
                     &(Configuration.FindDialogWindowPlacement.rcNormalPosition.top), sizeof(DWORD));
            GetValue(actKey, WINDOW_BOTTOM_REG, REG_DWORD,
                     &(Configuration.FindDialogWindowPlacement.rcNormalPosition.bottom), sizeof(DWORD));
            GetValue(actKey, WINDOW_SHOW_REG, REG_DWORD,
                     &(Configuration.FindDialogWindowPlacement.showCmd), sizeof(DWORD));

            GetValue(actKey, FINDDIALOG_NAMEWIDTH_REG, REG_DWORD,
                     &(Configuration.FindColNameWidth), sizeof(DWORD));
            CloseKey(actKey);
        }

        //---  default directories

        if (OpenKey(salamander, SALAMANDER_DEFDIRS_REG, actKey))
        {
            BOOL useActiveRegistry = FALSE;
            CSalamanderRegistryExAbstract* registry = ConfigurationStorage.GetRegistry();
            if (registry != NULL && ConfigurationStorage.UseActiveRegistryForKey(actKey))
                useActiveRegistry = TRUE;

            DWORD values = 0;
            DWORD res = ERROR_SUCCESS;
            if (!useActiveRegistry)
                res = RegQueryInfoKey(actKey, NULL, 0, 0, NULL, NULL, NULL, &values, NULL,
                                      NULL, NULL, NULL);
            if (useActiveRegistry || res == ERROR_SUCCESS)
            {
                char dir[4] = " :\\"; // reset DefaultDir
                char d;
                for (d = 'A'; d <= 'Z'; d++)
                {
                    dir[0] = d;
                    lstrcpyn(DefaultDir[d - 'A'], dir, _countof(DefaultDir[d - 'A']));
                }

                char name[2];
                BYTE path[32768];
                DWORD nameLen, dataLen, type;

                int i;
                for (i = 0; useActiveRegistry || i < (int)values; i++)
                {
                    nameLen = 2;
                    dataLen = sizeof(path);
                    if (useActiveRegistry)
                    {
                        name[0] = 0;
                        if (!registry->EnumValue(actKey, i, name, SizeOf(name), &type, path, &dataLen))
                            break;
                        res = ERROR_SUCCESS;
                    }
                    else
                        res = RegEnumValue(actKey, i, name, &nameLen, 0, &type, path, &dataLen);
                    if (res == ERROR_SUCCESS)
                        if (type == REG_SZ)
                        {
                            char d2 = LowerCase[name[0]];
                            if (d2 >= 'a' && d2 <= 'z')
                            {
                                if (dataLen > 2 && dataLen <= sizeof(DefaultDir[d2 - 'a']) &&
                                    LowerCase[path[0]] == d2 &&
                                    path[1] == ':' && path[2] == '\\')
                                {
                                    memmove(DefaultDir[d2 - 'a'], path, dataLen);
                                    DefaultDir[d2 - 'a'][_countof(DefaultDir[d2 - 'a']) - 1] = 0;
                                }
                                else
                                    SalMessageBox(HWindow, LoadStr(IDS_UNEXPECTEDVALUE),
                                                  LoadStr(IDS_ERRORLOADCONFIG), MB_OK | MB_ICONEXCLAMATION);
                            }
                            else
                                SalMessageBox(HWindow, LoadStr(IDS_UNEXPECTEDVALUE),
                                              LoadStr(IDS_ERRORLOADCONFIG), MB_OK | MB_ICONEXCLAMATION);
                        }
                        else
                            SalMessageBox(HWindow, LoadStr(IDS_UNEXPECTEDVALUETYPE),
                                          LoadStr(IDS_ERRORLOADCONFIG), MB_OK | MB_ICONEXCLAMATION);
                    else
                        SalMessageBox(HWindow, GetErrorText(res),
                                      LoadStr(IDS_ERRORLOADCONFIG), MB_OK | MB_ICONEXCLAMATION);
                }
            }
            else if (res != ERROR_FILE_NOT_FOUND)
                SalMessageBox(HWindow, GetErrorText(res),
                              LoadStr(IDS_ERRORLOADCONFIG), MB_OK | MB_ICONEXCLAMATION);
            CloseKey(actKey);
        }

        //---  password manager

        if (OpenKey(salamander, SALAMANDER_PWDMNGR_REG, actKey))
        {
            PasswordManager.Load(actKey);
            CloseKey(actKey);
        }

        //---  hot paths

        if (OpenKey(salamander, SALAMANDER_HOTPATHS_REG, actKey))
        {
            if (Configuration.ConfigVersion == 1) // HotPaths need conversion
                HotPaths.Load1_52(actKey);
            else
                HotPaths.Load(actKey);

            CloseKey(actKey);
        }

        //--- view templates

        if (OpenKey(salamander, SALAMANDER_VIEWTEMPLATES_REG, actKey))
        {
            ViewTemplates.Load(actKey);
            CloseKey(actKey);
        }

        //---  Plugins Order
        if (OpenKey(salamander, SALAMANDER_PLUGINSORDER, actKey))
        {
            Plugins.LoadOrder(HWindow, actKey);
            CloseKey(actKey);
        }

        //---  Plugins
        if (OpenKey(salamander, SALAMANDER_PLUGINS, actKey)) // otherwise default values
        {
            Plugins.Load(HWindow, actKey);
            CloseKey(actKey);
        }
        else
        {
            if (Configuration.ConfigVersion >= 6)
                Plugins.Clear(); // does not even want default archivers ...
        }

        //---  Packers & Unpackers
        if (OpenKey(salamander, SALAMANDER_PACKANDUNPACK, actKey))
        {
            GetValue(actKey, SALAMANDER_SIMPLEICONSINARCHIVES, REG_DWORD,
                     &(Configuration.UseSimpleIconsInArchives), sizeof(DWORD));
            //---  Custom Packers
            HKEY actSubKey;
            if (OpenKey(actKey, SALAMANDER_CUSTOMPACKERS, actSubKey))
            {
                PackerConfig.DeleteAllPackers();
                HKEY itemKey;
                char buf[30];
                int i = 1;
                strcpy(buf, "1");
                while (OpenKey(actSubKey, buf, itemKey))
                {
                    PackerConfig.Load(itemKey);
                    CloseKey(itemKey);
                    itoa(++i, buf, 10);
                }
                GetValue(actSubKey, SALAMANDER_ANOTHERPANEL, REG_DWORD,
                         &(Configuration.UseAnotherPanelForPack), sizeof(DWORD));
                int pp;
                if (GetValue(actSubKey, SALAMANDER_PREFFERED, REG_DWORD, &pp, sizeof(DWORD)))
                {
                    PackerConfig.SetPreferedPacker(pp);
                }
                CloseKey(actSubKey);
                // add new items introduced since the previous version :-)
                PackerConfig.AddDefault(Configuration.ConfigVersion);
            }
            //---  Custom Unpackers
            if (OpenKey(actKey, SALAMANDER_CUSTOMUNPACKERS, actSubKey))
            {
                UnpackerConfig.DeleteAllUnpackers();
                HKEY itemKey;
                char buf[30];
                int i = 1;
                strcpy(buf, "1");
                while (OpenKey(actSubKey, buf, itemKey))
                {
                    UnpackerConfig.Load(itemKey);
                    CloseKey(itemKey);
                    itoa(++i, buf, 10);
                }
                GetValue(actSubKey, SALAMANDER_ANOTHERPANEL, REG_DWORD,
                         &(Configuration.UseAnotherPanelForUnpack), sizeof(DWORD));
                GetValue(actSubKey, SALAMANDER_NAMEBYARCHIVE, REG_DWORD,
                         &(Configuration.UseSubdirNameByArchiveForUnpack), sizeof(DWORD));
                int pp;
                if (GetValue(actSubKey, SALAMANDER_PREFFERED, REG_DWORD, &pp, sizeof(DWORD)))
                {
                    UnpackerConfig.SetPreferedUnpacker(pp);
                }
                CloseKey(actSubKey);
                // add new items introduced since the previous version
                UnpackerConfig.AddDefault(Configuration.ConfigVersion);
            }
            //---  Predefined Packers
            if (OpenKey(actKey, SALAMANDER_PREDPACKERS, actSubKey))
            {
                // j.r.
                // External Archivers Locations: default values are no longer deleted during configuration load;
                // they are only updated. If the registry contains an incomplete or unknown entry,
                // it is ignored. Only when the Title matches one of the default values are its paths used.
                // ArchiverConfig.DeleteAllArchivers();
                ArchiverConfig.EnsureDefaultValues();
                HKEY itemKey;
                char buf[30];
                int i = 1;
                strcpy(buf, "1");
                while (OpenKey(actSubKey, buf, itemKey))
                {
                    ArchiverConfig.Load(itemKey);
                    CloseKey(itemKey);
                    itoa(++i, buf, 10);
                }
                char autoconfigDrives[SAL_MAX_PATH];
                if (GetValue(actSubKey, SALAMANDER_AUTOCONFIGDRIVES_REG, REG_MULTI_SZ,
                             autoconfigDrives, sizeof(autoconfigDrives)))
                    PackSetAutoconfigDrives(autoconfigDrives);
                CloseKey(actSubKey);
                // add new items introduced since the previous version
                // ArchiverConfig.AddDefault(Configuration.ConfigVersion); // j.r. no longer needed
            }
            ArchiverConfig.EnsureDefaultValues();
            //---  Archive Association
            if (OpenKey(actKey, SALAMANDER_ARCHIVEASSOC, actSubKey))
            {
                PackerFormatConfig.DeleteAllFormats();
                HKEY itemKey;
                char buf[30];
                int i = 1;
                strcpy(buf, "1");
                while (OpenKey(actSubKey, buf, itemKey))
                {
                    PackerFormatConfig.Load(itemKey);
                    CloseKey(itemKey);
                    itoa(++i, buf, 10);
                }
                CloseKey(actSubKey);
                // add new items introduced since the previous version
                PackerFormatConfig.AddDefault(Configuration.ConfigVersion);
                PackerFormatConfig.BuildArray();
            }
            CloseKey(actKey);
        }

        Plugins.CheckData(); // adjust loaded data

        //---  user menu

        IfExistSetSplashScreenText(LoadStr(IDS_STARTUP_USERMENU));

        if (OpenKey(salamander, SALAMANDER_USERMENU_REG, actKey))
        {
            HKEY subKey;
            char buf[30];
            strcpy(buf, "1");
            char name[MAX_PATH];
            char command[MAX_PATH];
            char arguments[USRMNUARGS_MAXLEN];
            char initDir[MAX_PATH];
            int throughShell, closeShell, useWindow;
            int showInToolbar, separator;
            CUserMenuItemType type;
            char icon[MAX_PATH];
            int i = 1;
            UserMenuItems->DestroyMembers();

            CUserMenuIconDataArr* bkgndReaderData = new CUserMenuIconDataArr();

            while (OpenKey(actKey, buf, subKey))
            {
                if (GetValue(subKey, USERMENU_ITEMNAME_REG, REG_SZ, name, MAX_PATH) &&
                    GetValue(subKey, USERMENU_COMMAND_REG, REG_SZ, command, MAX_PATH) &&
                    GetValue(subKey, USERMENU_SHELL_REG, REG_DWORD,
                             &throughShell, sizeof(DWORD)) &&
                    GetValue(subKey, USERMENU_CLOSE_REG, REG_DWORD,
                             &closeShell, sizeof(DWORD)))
                {
                    if (Configuration.ConfigVersion == 1 ||
                        !GetValue(subKey, USERMENU_ARGUMENTS_REG, REG_SZ, arguments, USRMNUARGS_MAXLEN))
                    {
                        // convert from user-menu version 1.52 to the current version
                        char* s = command;
                        while (*s != 0)
                        {
                            if (*s == '%' && *++s != '%')
                                break;
                            s++;
                        }
                        if (*s == 0)
                            *arguments = 0; // no parameters
                        else
                        {
                            s--;
                            while (--s >= command && *s != ' ')
                                ;
                            if (s < command)
                                *arguments = 0; // syntax error
                            else
                            {
                                *s++ = 0; // terminate the command, set s to the first argument character
                                char* st = arguments;
                                char* stEnd = arguments + sizeof(arguments) - 1;
                                while (*s != 0 && st < stEnd)
                                {
                                    if (*s == '%')
                                    {
                                        const char* add = "";
                                        switch (LowerCase[*++s])
                                        {
                                        case '%':
                                            add = "%";
                                            break;
                                        case 'd':
                                            add = "$(Drive)";
                                            break;
                                        case 'p':
                                            add = "$(Path)";
                                            break;
                                        case 'h':
                                            add = "$(DOSPath)";
                                            break;
                                        case 'f':
                                            add = "$(Name)";
                                            break;
                                        case 's':
                                            add = "$(DOSName)";
                                            break;
                                        }
                                        if (st + strlen(add) > stEnd)
                                            break;
                                        else
                                        {
                                            strcpy(st, add);
                                            st += strlen(add);
                                        }
                                    }
                                    else
                                        *st++ = *s;
                                    s++;
                                }
                                *st = 0;
                            }
                        }
                    }
                    if (Configuration.ConfigVersion == 1 ||
                        !GetValue(subKey, USERMENU_INITDIR_REG, REG_SZ, initDir, MAX_PATH))
                    {
                        strcpy(initDir, "$(Drive)$(Path)");
                    }
                    if (Configuration.ConfigVersion == 1 ||
                        !GetValue(subKey, USERMENU_USEWINDOW_REG, REG_DWORD, &useWindow, sizeof(DWORD)))
                    {
                        useWindow = TRUE;
                    }

                    if (Configuration.ConfigVersion == 1 ||
                        !GetValue(subKey, USERMENU_ICON_REG, REG_SZ, icon, MAX_PATH))
                    {
                        icon[0] = 0;
                    }

                    if (Configuration.ConfigVersion == 1 ||
                        !GetValue(subKey, USERMENU_SEPARATOR_REG, REG_DWORD, &separator, sizeof(DWORD)))
                    {
                        separator = FALSE;
                    }

                    if (!GetValue(subKey, USERMENU_TYPE_REG, REG_DWORD, &type, sizeof(DWORD)))
                    {
                        type = separator ? umitSeparator : umitItem;
                    }

                    if (Configuration.ConfigVersion == 1 ||
                        !GetValue(subKey, USERMENU_SHOWINTOOLBAR_REG, REG_DWORD, &showInToolbar, sizeof(DWORD)))
                    {
                        showInToolbar = TRUE;
                    }

                    CUserMenuItem* item = new CUserMenuItem(name, command, arguments, initDir, icon,
                                                            throughShell, closeShell, useWindow,
                                                            showInToolbar, type, bkgndReaderData);
                    if (item != NULL && item->IsGood())
                    {
                        UserMenuItems->Add(item);
                        if (!UserMenuItems->IsGood())
                        {
                            delete item;
                            UserMenuItems->ResetState();
                            break;
                        }
                    }
                    else
                    {
                        if (item != NULL)
                            delete item;
                        TRACE_E(LOW_MEMORY);
                        break;
                    }
                }
                else
                    break;
                itoa(++i, buf, 10);
                CloseKey(subKey);
            }

            UserMenuIconBkgndReader.StartBkgndReadingIcons(bkgndReaderData); // CAUTION: frees 'bkgndReaderData'

            CloseKey(actKey);
        }

        IfExistSetSplashScreenText(LoadStr(IDS_STARTUP_CONFIG));

        //---  configuration

        DWORD cmdLine = 0, cmdLineFocus = 0;
        DWORD rightPanelFocused = FALSE;
        if (OpenKey(salamander, SALAMANDER_CONFIG_REG, actKey))
        {
            if (importingOldConfig)
            {
                GetValue(actKey, CONFIG_ONLYONEINSTANCE_REG, REG_DWORD,
                         &Configuration.OnlyOneInstance, sizeof(DWORD));
            }
            //---  top rebar begin
            GetValue(actKey, CONFIG_MENUINDEX_REG, REG_DWORD,
                     &Configuration.MenuIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_MENUBREAK_REG, REG_DWORD,
                     &Configuration.MenuBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_MENUWIDTH_REG, REG_DWORD,
                     &Configuration.MenuWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_TOOLBARINDEX_REG, REG_DWORD,
                     &Configuration.TopToolbarIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_TOOLBARBREAK_REG, REG_DWORD,
                     &Configuration.TopToolbarBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_TOOLBARWIDTH_REG, REG_DWORD,
                     &Configuration.TopToolbarWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_PLUGINSBARINDEX_REG, REG_DWORD,
                     &Configuration.PluginsBarIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_PLUGINSBARBREAK_REG, REG_DWORD,
                     &Configuration.PluginsBarBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_PLUGINSBARWIDTH_REG, REG_DWORD,
                     &Configuration.PluginsBarWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_EXTENSIONBARINDEX_REG, REG_DWORD,
                     &Configuration.ExtensionBarIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_EXTENSIONBARBREAK_REG, REG_DWORD,
                     &Configuration.ExtensionBarBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_EXTENSIONBARWIDTH_REG, REG_DWORD,
                     &Configuration.ExtensionBarWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_USERMENUINDEX_REG, REG_DWORD,
                     &Configuration.UserMenuToolbarIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_USERMENUBREAK_REG, REG_DWORD,
                     &Configuration.UserMenuToolbarBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_USERMENUWIDTH_REG, REG_DWORD,
                     &Configuration.UserMenuToolbarWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_USERMENULABELS_REG, REG_DWORD,
                     &Configuration.UserMenuToolbarLabels, sizeof(DWORD));
            GetValue(actKey, CONFIG_HOTPATHSINDEX_REG, REG_DWORD,
                     &Configuration.HotPathsBarIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_HOTPATHSBREAK_REG, REG_DWORD,
                     &Configuration.HotPathsBarBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_HOTPATHSWIDTH_REG, REG_DWORD,
                     &Configuration.HotPathsBarWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_DRIVEBARINDEX_REG, REG_DWORD,
                     &Configuration.DriveBarIndex, sizeof(DWORD));
            GetValue(actKey, CONFIG_DRIVEBARBREAK_REG, REG_DWORD,
                     &Configuration.DriveBarBreak, sizeof(DWORD));
            GetValue(actKey, CONFIG_DRIVEBARWIDTH_REG, REG_DWORD,
                     &Configuration.DriveBarWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_TREEVIEWWIDTH_REG, REG_DWORD,
                     &Configuration.TreeViewWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_TREEVIEWAUTOHIDE_REG, REG_DWORD,
                     &Configuration.TreeViewAutoHide, sizeof(DWORD));
            Configuration.DetachedTreeViewWidth = Configuration.TreeViewWidth;
            Configuration.DetachedTreeViewAutoHide = Configuration.TreeViewAutoHide;
            GetValue(actKey, CONFIG_DETACHEDTREEVIEWWIDTH_REG, REG_DWORD,
                     &Configuration.DetachedTreeViewWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_DETACHEDTREEVIEWAUTOHIDE_REG, REG_DWORD,
                     &Configuration.DetachedTreeViewAutoHide, sizeof(DWORD));
            if (LeftPanel != NULL)
            {
                LeftPanel->TreeViewWidth = Configuration.TreeViewWidth;
                LeftPanel->TreeViewAutoHide = Configuration.TreeViewAutoHide;
                LeftPanel->TreeViewAutoHideExpanded = FALSE;
                LeftPanel->TreeViewAutoHideCollapseStart = 0;
            }
            if (RightPanel != NULL)
            {
                RightPanel->TreeViewWidth = Configuration.DetachedPanels ? Configuration.DetachedTreeViewWidth : Configuration.TreeViewWidth;
                RightPanel->TreeViewAutoHide = Configuration.DetachedPanels ? Configuration.DetachedTreeViewAutoHide : Configuration.TreeViewAutoHide;
                RightPanel->TreeViewAutoHideExpanded = FALSE;
                RightPanel->TreeViewAutoHideCollapseStart = 0;
            }
            GetValue(actKey, CONFIG_GRIPSVISIBLE_REG, REG_DWORD,
                     &Configuration.GripsVisible, sizeof(DWORD));
            //---  top rebar end
            GetValue(actKey, CONFIG_FILENAMEFORMAT_REG, REG_DWORD,
                     &Configuration.FileNameFormat, sizeof(DWORD));
            GetValue(actKey, CONFIG_SIZEFORMAT_REG, REG_DWORD,
                     &Configuration.SizeFormat, sizeof(DWORD));
            // automatic conversion from "mixed-case" to "partially-mixed-case"
            if (Configuration.FileNameFormat == 1)
                Configuration.FileNameFormat = 7;

            GetValue(actKey, CONFIG_SELECTION_REG, REG_DWORD,
                     &Configuration.IncludeDirs, sizeof(DWORD));
            GetValue(actKey, CONFIG_COPYFINDTEXT_REG, REG_DWORD,
                     &Configuration.CopyFindText, sizeof(DWORD));
            GetValue(actKey, CONFIG_CLEARREADONLY_REG, REG_DWORD,
                     &Configuration.ClearReadOnly, sizeof(DWORD));
            GetValue(actKey, CONFIG_PRIMARYCONTEXTMENU_REG, REG_DWORD,
                     &Configuration.PrimaryContextMenu, sizeof(DWORD));
            GetValue(actKey, CONFIG_NOTHIDDENSYSTEM_REG, REG_DWORD,
                     &Configuration.NotHiddenSystemFiles, sizeof(DWORD));
            GetValue(actKey, CONFIG_RECYCLEBIN_REG, REG_DWORD,
                     &Configuration.UseRecycleBin, sizeof(DWORD));
            // a bit ugly: we provide MasksString, but the range is checked so it's fine
            GetValue(actKey, CONFIG_RECYCLEMASKS_REG, REG_SZ,
                     Configuration.RecycleMasks.GetWritableMasksString(), MAX_PATH);
            GetValue(actKey, CONFIG_SAVEONEXIT_REG, REG_DWORD,
                     &Configuration.AutoSave, sizeof(DWORD));
            GetValue(actKey, CONFIG_SHOWGREPERRORS_REG, REG_DWORD,
                     &Configuration.ShowGrepErrors, sizeof(DWORD));
            GetValue(actKey, CONFIG_FINDFULLROW_REG, REG_DWORD,
                     &Configuration.FindFullRowSelect, sizeof(DWORD));
            if (Configuration.ConfigVersion <= 6)
                Configuration.ShowGrepErrors = FALSE; // force FALSE so we don't annoy users unnecessarily (others do it this way too)
            GetValue(actKey, CONFIG_MINBEEPWHENDONE_REG, REG_DWORD,
                     &Configuration.MinBeepWhenDone, sizeof(DWORD));
            GetValue(actKey, CONFIG_CLOSESHELL_REG, REG_DWORD,
                     &Configuration.CloseShell, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMMANDLINEAPP_REG, REG_SZ,
                     Configuration.CommandLineApplication, SAL_MAX_PATH);
            GetValue(actKey, CONFIG_COMMANDLINEARGS_REG, REG_SZ,
                     Configuration.CommandLineArguments, CONFIG_COMMANDLINEARGS_MAXLEN);
            GetValue(actKey, CONFIG_RIGHT_FOCUS_REG, REG_DWORD,
                     &rightPanelFocused, sizeof(DWORD));
            GetValue(actKey, CONFIG_ALWAYSONTOP_REG, REG_DWORD,
                     &Configuration.AlwaysOnTop, sizeof(DWORD));
            //      GetValue(actKey, CONFIG_FASTDIRMOVE_REG, REG_DWORD,
            //               &Configuration.FastDirectoryMove, sizeof(DWORD));
            GetValue(actKey, CONFIG_SORTUSESLOCALE_REG, REG_DWORD,
                     &Configuration.SortUsesLocale, sizeof(DWORD));
            GetValue(actKey, CONFIG_SORTDETECTNUMBERS_REG, REG_DWORD,
                     &Configuration.SortDetectNumbers, sizeof(DWORD));
            GetValue(actKey, CONFIG_SORTNEWERONTOP_REG, REG_DWORD,
                     &Configuration.SortNewerOnTop, sizeof(DWORD));
            GetValue(actKey, CONFIG_SORTDIRSBYNAME_REG, REG_DWORD,
                     &Configuration.SortDirsByName, sizeof(DWORD));
            GetValue(actKey, CONFIG_SORTDIRSBYEXT_REG, REG_DWORD,
                     &Configuration.SortDirsByExt, sizeof(DWORD));
            GetValue(actKey, CONFIG_SAVEHISTORY_REG, REG_DWORD,
                     &Configuration.SaveHistory, sizeof(DWORD));
            GetValue(actKey, CONFIG_SAVEWORKDIRS_REG, REG_DWORD,
                     &Configuration.SaveWorkDirs, sizeof(DWORD));
            if (GetValue(actKey, CONFIG_WORKDIRS_HISTORY_SCOPE_REG, REG_DWORD,
                         &Configuration.WorkDirsHistoryScope, sizeof(DWORD)))
            {
                if (Configuration.WorkDirsHistoryScope != wdhsShared &&
                    Configuration.WorkDirsHistoryScope != wdhsPerTab)
                    Configuration.WorkDirsHistoryScope = wdhsShared;
            }
            else
                Configuration.WorkDirsHistoryScope = wdhsShared;
            GetValue(actKey, CONFIG_ENABLECMDLINEHISTORY_REG, REG_DWORD,
                     &Configuration.EnableCmdLineHistory, sizeof(DWORD));
            GetValue(actKey, CONFIG_SAVECMDLINEHISTORY_REG, REG_DWORD,
                     &Configuration.SaveCmdLineHistory, sizeof(DWORD));
            if (GetValue(actKey, CONFIG_BACKSPACEACTION_REG, REG_DWORD,
                         &Configuration.BackspaceAction, sizeof(DWORD)))
            {
                if (Configuration.BackspaceAction != 0 && Configuration.BackspaceAction != 1)
                    Configuration.BackspaceAction = 0;
            }
            //      GetValue(actKey, CONFIG_LANTASTICCHECK_REG, REG_DWORD,
            //               &Configuration.LantasticCheck, sizeof(DWORD));
            GetValue(actKey, CONFIG_STATUSAREA_REG, REG_DWORD,
                     &Configuration.StatusArea, sizeof(DWORD));
            if (!GetValue(actKey, CONFIG_FULLROWSELECT_REG, REG_DWORD,
                          &Configuration.FullRowSelect, sizeof(DWORD)))
            {
                // we don't want conversion - force TRUE
                //        if (GetValue(actKey, CONFIG_EXPLORERLOOK_REG, REG_DWORD,
                //                     &Configuration.FullRowSelect, sizeof(DWORD)))
                //        {
                DeleteValue(actKey, CONFIG_EXPLORERLOOK_REG);
                //          Configuration.FullRowSelect = !Configuration.FullRowSelect;
                //        }
            }
            GetValue(actKey, CONFIG_FULLROWHIGHLIGHT_REG, REG_DWORD,
                     &Configuration.FullRowHighlight, sizeof(DWORD));
            GetValue(actKey, CONFIG_USEICONTINCTURE_REG, REG_DWORD,
                     &Configuration.UseIconTincture, sizeof(DWORD));
            GetValue(actKey, CONFIG_PANELS_USETABS_REG, REG_DWORD,
                     &Configuration.UsePanelTabs, sizeof(DWORD));
            Configuration.UsePanelTabs = Configuration.UsePanelTabs ? TRUE : FALSE;
            GetValue(actKey, CONFIG_SHOWPANELCAPTION_REG, REG_DWORD,
                     &Configuration.ShowPanelCaption, sizeof(DWORD));
            GetValue(actKey, CONFIG_SHOWPANELZOOM_REG, REG_DWORD,
                     &Configuration.ShowPanelZoom, sizeof(DWORD));
            GetValue(actKey, CONFIG_SINGLECLICK_REG, REG_DWORD,
                     &Configuration.SingleClick, sizeof(DWORD));
            //      GetValue(actKey, CONFIG_SHOWTIPOFTHEDAY_REG, REG_DWORD,
            //               &Configuration.ShowTipOfTheDay, sizeof(DWORD));
            //      GetValue(actKey, CONFIG_LASTTIPOFTHEDAY_REG, REG_DWORD,
            //               &Configuration.LastTipOfTheDay, sizeof(DWORD));
            GetValue(actKey, CONFIG_INFOLINECONTENT_REG, REG_SZ,
                     Configuration.InfoLineContent, 200);
            GetValue(actKey, CONFIG_IFPATHISINACCESSIBLEGOTO_REG, REG_SZ,
                     Configuration.IfPathIsInaccessibleGoTo, SAL_MAX_PATH);
            if (!GetValue(actKey, CONFIG_IFPATHISINACCESSIBLEGOTOISMYDOCS_REG, REG_DWORD,
                          &Configuration.IfPathIsInaccessibleGoToIsMyDocs, sizeof(DWORD)))
            {
                char path[SAL_MAX_PATH];
                GetIfPathIsInaccessibleGoTo(path, TRUE);
                if (IsTheSamePath(path, Configuration.IfPathIsInaccessibleGoTo)) // user wants to go to My Documents
                {
                    Configuration.IfPathIsInaccessibleGoToIsMyDocs = TRUE;
                    Configuration.IfPathIsInaccessibleGoTo[0] = 0;
                }
                else
                    Configuration.IfPathIsInaccessibleGoToIsMyDocs = FALSE;
            }
            GetValue(actKey, CONFIG_HOTPATH_AUTOCONFIG, REG_DWORD,
                     &Configuration.HotPathAutoConfig, sizeof(DWORD));
            GetValue(actKey, CONFIG_LASTUSEDSPEEDLIM_REG, REG_DWORD,
                     &Configuration.LastUsedSpeedLimit, sizeof(DWORD));
            GetValue(actKey, CONFIG_QUICKSEARCHENTER_REG, REG_DWORD,
                     &Configuration.QuickSearchEnterAlt, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWMOUNTFOLDERS, REG_DWORD,
                     &Configuration.ChangeDriveShowMountFolders, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_MOUNTFOLDERS_MODE, REG_DWORD,
                     &Configuration.ChangeDriveMountFoldersMode, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_MOUNTFOLDERS_NAME, REG_DWORD,
                     &Configuration.ChangeDriveMountFoldersName, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_MOUNTFOLDERS_DRIVEBAR, REG_DWORD,
                     &Configuration.ChangeDriveMountFoldersDriveBar, sizeof(DWORD));
            if (Configuration.ChangeDriveMountFoldersMode < TITLE_BAR_MODE_DIRECTORY ||
                Configuration.ChangeDriveMountFoldersMode > TITLE_BAR_MODE_FULLPATH)
                Configuration.ChangeDriveMountFoldersMode = TITLE_BAR_MODE_DIRECTORY;
            GetValue(actKey, CONFIG_CHD_SHOWMYDOC, REG_DWORD,
                     &Configuration.ChangeDriveShowMyDoc, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOW3DOBJECTS, REG_DWORD,
                     &Configuration.ChangeDriveShow3DObjects, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWDESKTOP, REG_DWORD,
                     &Configuration.ChangeDriveShowDesktop, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWDOWNLOADS, REG_DWORD,
                     &Configuration.ChangeDriveShowDownloads, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWMUSIC, REG_DWORD,
                     &Configuration.ChangeDriveShowMusic, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWPICTURES, REG_DWORD,
                     &Configuration.ChangeDriveShowPictures, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWVIDEOS, REG_DWORD,
                     &Configuration.ChangeDriveShowVideos, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWCLOUDSTOR, REG_DWORD,
                     &Configuration.ChangeDriveCloudStorage, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWANOTHER, REG_DWORD,
                     &Configuration.ChangeDriveShowAnother, sizeof(DWORD));
            GetValue(actKey, CONFIG_CHD_SHOWNET, REG_DWORD,
                     &Configuration.ChangeDriveShowNet, sizeof(DWORD));
            GetValue(actKey, CONFIG_SEARCHFILECONTENT, REG_DWORD,
                     &Configuration.SearchFileContent, sizeof(DWORD));
            GetValue(actKey, CONFIG_LASTPLUGINVER, REG_DWORD,
                     &Configuration.LastPluginVer, sizeof(DWORD));
            GetValue(actKey, CONFIG_LASTPLUGINVER_OP, REG_DWORD,
                     &Configuration.LastPluginVerOP, sizeof(DWORD));
            GetValue(actKey, CONFIG_QUICKRENAME_SELALL_REG, REG_DWORD,
                     &Configuration.QuickRenameSelectAll, sizeof(DWORD));
            GetValue(actKey, CONFIG_EDITNEW_SELALL_REG, REG_DWORD,
                     &Configuration.EditNewSelectAll, sizeof(DWORD));
            if (!GetValue(actKey, CONFIG_USESALOPEN_REG, REG_DWORD,
                          &Configuration.UseSalOpen, sizeof(DWORD)))
            {
                Configuration.UseSalOpen = FALSE; // default is not to use it
            }
            else
            {
                if (Configuration.ConfigVersion == 11) // in 1.6 beta 7 it was enabled ... turn it off
                {
                    Configuration.UseSalOpen = FALSE; // default is not to use it
                }
            }
            GetValue(actKey, CONFIG_NETWAREFASTDIRMOVE_REG, REG_DWORD,
                     &Configuration.NetwareFastDirMove, sizeof(DWORD));
            if (Windows7AndLater)
                GetValue(actKey, CONFIG_ASYNCCOPYALG_REG, REG_DWORD,
                         &Configuration.UseAsyncCopyAlg, sizeof(DWORD));
            GetValue(actKey, CONFIG_RELOAD_ENV_VARS_REG, REG_DWORD,
                     &Configuration.ReloadEnvVariables, sizeof(DWORD));
            GetValue(actKey, CONFIG_PATH_AUTOCOMPLETE_REG, REG_DWORD,
                     &Configuration.PathAutoComplete, sizeof(DWORD));
            GetValue(actKey, CONFIG_CREATEDIR_AUTOCOMPLETE_REG, REG_DWORD,
                     &Configuration.CreateDirAutoComplete, sizeof(DWORD));
            GetValue(actKey, CONFIG_SHIFTFORHOTPATHS_REG, REG_DWORD,
                     &Configuration.ShiftForHotPaths, sizeof(DWORD));
            //      GetValue(actKey, CONFIG_LANGUAGE_REG, REG_SZ,
            //               Configuration.SLGName, MAX_PATH);
            //      GetValue(actKey, CONFIG_USEALTLANGFORPLUGINS_REG, REG_DWORD,
            //               &Configuration.UseAsAltSLGInOtherPlugins, sizeof(DWORD));
            //      GetValue(actKey, CONFIG_ALTLANGFORPLUGINS_REG, REG_SZ,
            //               Configuration.AltPluginSLGName, MAX_PATH);
            GetValue(actKey, CONFIG_CONVERSIONTABLE_REG, REG_SZ,
                     &Configuration.ConversionTable, MAX_PATH);
            GetValue(actKey, CONFIG_SKILLLEVEL_REG, REG_DWORD,
                     &Configuration.SkillLevel, sizeof(DWORD));
            GetValue(actKey, CONFIG_TITLEBARSHOWPATH_REG, REG_DWORD,
                     &Configuration.TitleBarShowPath, sizeof(DWORD));
            GetValue(actKey, CONFIG_TITLEBARMODE_REG, REG_DWORD,
                     &Configuration.TitleBarMode, sizeof(DWORD));
            BOOL hasTabCaptionMode = GetValue(actKey, CONFIG_TABCAPTIONMODE_REG, REG_DWORD,
                                              &Configuration.TabCaptionMode, sizeof(DWORD));
            if (!hasTabCaptionMode)
                Configuration.TabCaptionMode = TITLE_BAR_MODE_DIRECTORY;
            if (Configuration.TabCaptionMode < TITLE_BAR_MODE_DIRECTORY ||
                Configuration.TabCaptionMode > TITLE_BAR_MODE_FULLPATH)
                Configuration.TabCaptionMode = TITLE_BAR_MODE_DIRECTORY;
            if (!GetValue(actKey, CONFIG_TABCAPTIONALIGNMENT_REG, REG_DWORD,
                          &Configuration.TabCaptionAlignment, sizeof(DWORD)))
            {
                Configuration.TabCaptionAlignment = TAB_CAPTION_ALIGN_CENTER;
            }
            if (Configuration.TabCaptionAlignment != TAB_CAPTION_ALIGN_LEFT &&
                Configuration.TabCaptionAlignment != TAB_CAPTION_ALIGN_CENTER)
            {
                Configuration.TabCaptionAlignment = TAB_CAPTION_ALIGN_CENTER;
            }
            DWORD storedTabMinWidth = 0;
            if (GetValue(actKey, CONFIG_TABMINWIDTH_REG, REG_DWORD, &storedTabMinWidth, sizeof(DWORD)))
            {
                Configuration.TabButtonMinWidth = (int)storedTabMinWidth;
            }
            else
                Configuration.TabButtonMinWidth = 0;
            DWORD storedTabMaxWidth = 0;
            if (GetValue(actKey, CONFIG_TABMAXWIDTH_REG, REG_DWORD, &storedTabMaxWidth, sizeof(DWORD)))
            {
                Configuration.TabButtonMaxWidth = (int)storedTabMaxWidth;
            }
            else
                Configuration.TabButtonMaxWidth = 0;
            if (Configuration.TabButtonMinWidth < 0)
                Configuration.TabButtonMinWidth = 0;
            if (Configuration.TabButtonMaxWidth < 0)
                Configuration.TabButtonMaxWidth = 0;
            if (Configuration.TabButtonMinWidth > 0 && Configuration.TabButtonMaxWidth > 0 &&
                Configuration.TabButtonMinWidth > Configuration.TabButtonMaxWidth)
                Configuration.TabButtonMinWidth = Configuration.TabButtonMaxWidth;
            if (!GetValue(actKey, CONFIG_TABACTIVEBORDER_REG, REG_DWORD,
                          &Configuration.TabActiveBorder, sizeof(DWORD)))
            {
                Configuration.TabActiveBorder = TRUE;
            }
            {
                DWORD tabActiveBorderColor = (DWORD)CLR_INVALID;
                if (!GetValue(actKey, CONFIG_TABACTIVEBORDERCOLOR_REG, REG_DWORD,
                              &tabActiveBorderColor, sizeof(DWORD)))
                {
                    tabActiveBorderColor = (DWORD)CLR_INVALID;
                }
                Configuration.TabActiveBorderColor = (COLORREF)tabActiveBorderColor;
            }
            if (!GetValue(actKey, CONFIG_TABCLOSEBUTTONACTIVE_REG, REG_DWORD,
                          &Configuration.TabCloseButtonActive, sizeof(DWORD)))
            {
                Configuration.TabCloseButtonActive = FALSE;
            }
            if (!GetValue(actKey, CONFIG_TABCLOSEBUTTONALL_REG, REG_DWORD,
                          &Configuration.TabCloseButtonAll, sizeof(DWORD)))
            {
                Configuration.TabCloseButtonAll = FALSE;
            }
            GetValue(actKey, CONFIG_TITLEBARPREFIX_REG, REG_DWORD,
                     &Configuration.UseTitleBarPrefix, sizeof(DWORD));
            GetValue(actKey, CONFIG_TITLEBARPREFIXTEXT_REG, REG_SZ,
                     &Configuration.TitleBarPrefix, TITLE_PREFIX_MAX);
            GetValue(actKey, CONFIG_MAINWINDOWICONINDEX_REG, REG_DWORD,
                     &Configuration.MainWindowIconIndex, sizeof(DWORD));
            if (Configuration.MainWindowIconIndex < 0 || Configuration.MainWindowIconIndex >= MAINWINDOWICONS_COUNT)
                Configuration.MainWindowIconIndex = MAINWINDOWICON_DEFAULT_INDEX;
            GetValue(actKey, CONFIG_CLICKQUICKRENAME_REG, REG_DWORD,
                     &Configuration.ClickQuickRename, sizeof(DWORD));
            GetValue(actKey, CONFIG_VISIBLEDRIVES_REG, REG_DWORD,
                     &Configuration.VisibleDrives, sizeof(DWORD));
            GetValue(actKey, CONFIG_SEPARATEDDRIVES_REG, REG_DWORD,
                     &Configuration.SeparatedDrives, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREBYTIME_REG, REG_DWORD,
                     &Configuration.CompareByTime, sizeof(DWORD));
            if (!GetValue(actKey, CONFIG_COMPAREBYSIZE_REG, REG_DWORD,
                          &Configuration.CompareBySize, sizeof(DWORD)))
            { // conversion from older configuration - BySize used to be part of ByTime, so copy that setting
                Configuration.CompareBySize = Configuration.CompareByTime;
            }
            GetValue(actKey, CONFIG_COMPAREBYCONTENT_REG, REG_DWORD,
                     &Configuration.CompareByContent, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREBYATTR_REG, REG_DWORD,
                     &Configuration.CompareByAttr, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREBYSUBDIRS_REG, REG_DWORD,
                     &Configuration.CompareSubdirs, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREBYSUBDIRSATTR_REG, REG_DWORD,
                     &Configuration.CompareSubdirsAttr, sizeof(DWORD));

            GetValue(actKey, CONFIG_COMPAREONEPANELDIRS_REG, REG_DWORD,
                     &Configuration.CompareOnePanelDirs, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREMOREOPTIONS_REG, REG_DWORD,
                     &Configuration.CompareMoreOptions, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREIGNOREFILES_REG, REG_DWORD,
                     &Configuration.CompareIgnoreFiles, sizeof(DWORD));
            GetValue(actKey, CONFIG_COMPAREIGNOREDIRS_REG, REG_DWORD,
                     &Configuration.CompareIgnoreDirs, sizeof(DWORD));
            // a bit ugly: we provide MasksString, but the range is checked so it's fine
            GetValue(actKey, CONFIG_CONFIGTIGNOREFILESMASKS_REG, REG_SZ,
                     Configuration.CompareIgnoreFilesMasks.GetWritableMasksString(), MAX_PATH);
            GetValue(actKey, CONFIG_CONFIGTIGNOREDIRSMASKS_REG, REG_SZ,
                     Configuration.CompareIgnoreDirsMasks.GetWritableMasksString(), MAX_PATH);
            int errPos;
            Configuration.CompareIgnoreFilesMasks.PrepareMasks(errPos);
            Configuration.CompareIgnoreDirsMasks.PrepareMasks(errPos);

            GetValue(actKey, CONFIG_THUMBNAILSIZE_REG, REG_DWORD,
                     &Configuration.ThumbnailSize, sizeof(DWORD));
            LeftPanel->SetThumbnailSize(Configuration.ThumbnailSize);
            RightPanel->SetThumbnailSize(Configuration.ThumbnailSize);

            GetValue(actKey, CONFIG_KEEPPLUGINSSORTED_REG, REG_DWORD,
                     &Configuration.KeepPluginsSorted, sizeof(DWORD));

            Configuration.ShowSLGIncomplete = TRUE;
            if (Configuration.ConfigVersion == THIS_CONFIG_VERSION)
            {
                GetValue(actKey, CONFIG_SHOWSLGINCOMPLETE_REG, REG_DWORD,
                         &Configuration.ShowSLGIncomplete, sizeof(DWORD));
            }

            GetValue(actKey, CONFIG_EDITNEWFILE_USEDEFAULT_REG, REG_DWORD,
                     &Configuration.UseEditNewFileDefault, sizeof(DWORD));
            GetValue(actKey, CONFIG_EDITNEWFILE_DEFAULT_REG, REG_SZ,
                     Configuration.EditNewFileDefault, MAX_PATH);

#ifndef _WIN64 // FIXME_X64_WINSCP
            if (!GetValue(actKey, "Add x86-Only Plugins", REG_DWORD,
                          &Configuration.AddX86OnlyPlugins, sizeof(DWORD)))
            {
                Configuration.AddX86OnlyPlugins = TRUE;
            }
#endif // _WIN64

            HKEY actSubKey;
            if (OpenKey(actKey, SALAMANDER_CONFIRMATION_REG, actSubKey))
            {
                GetValue(actSubKey, CONFIG_CNFRM_FILEDIRDEL, REG_DWORD,
                         &Configuration.CnfrmFileDirDel, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_NEDIRDEL, REG_DWORD,
                         &Configuration.CnfrmNEDirDel, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_FILEOVER, REG_DWORD,
                         &Configuration.CnfrmFileOver, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_DIROVER, REG_DWORD,
                         &Configuration.CnfrmDirOver, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_SHFILEDEL, REG_DWORD,
                         &Configuration.CnfrmSHFileDel, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_SHDIRDEL, REG_DWORD,
                         &Configuration.CnfrmSHDirDel, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_SHFILEOVER, REG_DWORD,
                         &Configuration.CnfrmSHFileOver, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_NTFSPRESS, REG_DWORD,
                         &Configuration.CnfrmNTFSPress, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_NTFSCRYPT, REG_DWORD,
                         &Configuration.CnfrmNTFSCrypt, sizeof(DWORD));
                if (Configuration.ConfigVersion != 1)
                    GetValue(actSubKey, CONFIG_CNFRM_DAD, REG_DWORD,
                             &Configuration.CnfrmDragDrop, sizeof(DWORD));
                else // for old configs we read it one level up
                    GetValue(actKey, "Confirm Drop Operations", REG_DWORD,
                             &Configuration.CnfrmDragDrop, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CLOSEARCHIVE, REG_DWORD,
                         &Configuration.CnfrmCloseArchive, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CLOSEFIND, REG_DWORD,
                         &Configuration.CnfrmCloseFind, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_STOPFIND, REG_DWORD,
                         &Configuration.CnfrmStopFind, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CREATETARGETPATH, REG_DWORD,
                         &Configuration.CnfrmCreatePath, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_ALWAYSONTOP, REG_DWORD,
                         &Configuration.CnfrmAlwaysOnTop, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_ONSALCLOSE, REG_DWORD,
                         &Configuration.CnfrmOnSalClose, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_DETACHCLOSE, REG_DWORD,
                         &Configuration.CnfrmDetachClose, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_SENDEMAIL, REG_DWORD,
                         &Configuration.CnfrmSendEmail, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_ADDTOARCHIVE, REG_DWORD,
                         &Configuration.CnfrmAddToArchive, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CREATEDIR, REG_DWORD,
                         &Configuration.CnfrmCreateDir, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CHANGEDIRTC, REG_DWORD,
                         &Configuration.CnfrmChangeDirTC, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_SHOWNAMETOCOMP, REG_DWORD,
                         &Configuration.CnfrmShowNamesToCompare, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_DSTSHIFTSIGNORED, REG_DWORD,
                         &Configuration.CnfrmDSTShiftsIgnored, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_DSTSHIFTSOCCURED, REG_DWORD,
                         &Configuration.CnfrmDSTShiftsOccured, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_COPYMOVEOPTIONSNS, REG_DWORD,
                         &Configuration.CnfrmCopyMoveOptionsNS, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CHANGEDIRHISTORYERR, REG_DWORD,
                         &Configuration.CnfrmChangeDirHistoryErr, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_CNFRM_CONFIRMDELETEEXTINFO, REG_DWORD,
                         &Configuration.CnfrmConfirmDeleteExtInfo, sizeof(DWORD));

                CloseKey(actSubKey);
            }

            if (OpenKey(actKey, SALAMANDER_DRVSPEC_REG, actSubKey))
            {
                GetValue(actSubKey, CONFIG_DRVSPEC_FLOPPY_MON, REG_DWORD,
                         &Configuration.DrvSpecFloppyMon, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_FLOPPY_SIMPLE, REG_DWORD,
                         &Configuration.DrvSpecFloppySimple, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_REMOVABLE_MON, REG_DWORD,
                         &Configuration.DrvSpecRemovableMon, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_REMOVABLE_SIMPLE, REG_DWORD,
                         &Configuration.DrvSpecRemovableSimple, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_FIXED_MON, REG_DWORD,
                         &Configuration.DrvSpecFixedMon, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_FIXED_SIMPLE, REG_DWORD,
                         &Configuration.DrvSpecFixedSimple, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_REMOTE_MON, REG_DWORD,
                         &Configuration.DrvSpecRemoteMon, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_REMOTE_SIMPLE, REG_DWORD,
                         &Configuration.DrvSpecRemoteSimple, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_REMOTE_ACT, REG_DWORD,
                         &Configuration.DrvSpecRemoteDoNotRefreshOnAct, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_CDROM_MON, REG_DWORD,
                         &Configuration.DrvSpecCDROMMon, sizeof(DWORD));
                GetValue(actSubKey, CONFIG_DRVSPEC_CDROM_SIMPLE, REG_DWORD,
                         &Configuration.DrvSpecCDROMSimple, sizeof(DWORD));

                // for old versions we force icon reading on removable drives because we introduced the floppy category
                if (Configuration.ConfigVersion < 31)
                    Configuration.DrvSpecRemovableSimple = FALSE;

                CloseKey(actSubKey);
            }

            if (Configuration.ConfigVersion >= 8) // force the new toolbar for old versions
                GetValue(actKey, CONFIG_TOPTOOLBAR_REG, REG_SZ,
                         Configuration.TopToolBar, 400);
            GetValue(actKey, CONFIG_MIDDLETOOLBAR_REG, REG_SZ,
                     Configuration.MiddleToolBar, 400);
            GetValue(actKey, CONFIG_LEFTTOOLBAR_REG, REG_SZ,
                     Configuration.LeftToolBar, 100);
            GetValue(actKey, CONFIG_RIGHTTOOLBAR_REG, REG_SZ,
                     Configuration.RightToolBar, 100);
            // there used to be only one change drive button - now we introduce two buttons
            // and merge all bitmaps into one

            if (Configuration.ConfigVersion <= 3 && Configuration.RightToolBar[0] != 0)
            {
                char tmp[5000];
                lstrcpyn(tmp, Configuration.RightToolBar, 5000);
                char num[50];

                Configuration.RightToolBar[0] = 0;

                BOOL first = TRUE;
                char* p = strtok(tmp, ",");
                while (p != NULL)
                {
                    int i = atoi(p);

                    // replace the old tbbeChangeDrive with tbbeChangeDriveR
                    //#define TBBE_CHANGE_DRIVE_R     51
                    if (i == 36)
                        i = 51;

                    if (!first)
                        strcat(Configuration.RightToolBar, ",");
                    sprintf(num, "%d", i);
                    strcat(Configuration.RightToolBar, num);
                    first = FALSE;
                    p = strtok(NULL, ",");
                }
            }

            if (TopToolBar != NULL)
                TopToolBar->Load(Configuration.TopToolBar);
            if (MiddleToolBar != NULL)
                MiddleToolBar->Load(Configuration.MiddleToolBar);
            ReloadPanelToolBars(cpsLeft);
            ReloadPanelToolBars(cpsRight);

            GetValue(actKey, CONFIG_TOPTOOLBARVISIBLE_REG, REG_DWORD,
                     &Configuration.TopToolBarVisible, sizeof(DWORD));
            GetValue(actKey, CONFIG_PLGTOOLBARVISIBLE_REG, REG_DWORD,
                     &Configuration.PluginsBarVisible, sizeof(DWORD));
            GetValue(actKey, CONFIG_EXTENSIONBARVISIBLE_REG, REG_DWORD,
                     &Configuration.ExtensionBarVisible, sizeof(DWORD));
            GetValue(actKey, CONFIG_MIDDLETOOLBARVISIBLE_REG, REG_DWORD,
                     &Configuration.MiddleToolBarVisible, sizeof(DWORD));

            GetValue(actKey, CONFIG_USERMENUTOOLBARVISIBLE_REG, REG_DWORD,
                     &Configuration.UserMenuToolBarVisible, sizeof(DWORD));
            GetValue(actKey, CONFIG_HOTPATHSBARVISIBLE_REG, REG_DWORD,
                     &Configuration.HotPathsBarVisible, sizeof(DWORD));

            // if this is an old version of configuration and the user menu contains items,
            // show the UserMenuBar
            if (Configuration.ConfigVersion <= 3 && UserMenuItems->Count > 0)
                Configuration.UserMenuToolBarVisible = TRUE;

            GetValue(actKey, CONFIG_DRIVEBARVISIBLE_REG, REG_DWORD,
                     &Configuration.DriveBarVisible, sizeof(DWORD));
            GetValue(actKey, CONFIG_DRIVEBAR2VISIBLE_REG, REG_DWORD,
                     &Configuration.DriveBar2Visible, sizeof(DWORD));
            GetValue(actKey, CONFIG_TREEVIEWVISIBLE_REG, REG_DWORD,
                     &Configuration.TreeViewVisible, sizeof(DWORD));
            GetValue(actKey, CONFIG_PANELTOOLTIPS_REG, REG_DWORD,
                     &Configuration.PanelTooltips, sizeof(DWORD));

            if (ret) // if we return FALSE, everything will be inserted later
            {
                // bands must be inserted in the correct order according to their index
                BOOL menuInserted = FALSE; // the menu is important, insert it at all costs
                // disable saving positions while adding bands, otherwise their order would be overwritten
                int idx;
                for (idx = 0; idx < 10; idx++) // we can safely try more indices than there are bands
                {
                    if (idx == Configuration.MenuIndex)
                    {
                        InsertMenuBand();
                        menuInserted = TRUE;
                    }
                    if (idx == Configuration.TopToolbarIndex && Configuration.TopToolBarVisible)
                        ToggleTopToolBar(FALSE);
                    if (idx == Configuration.PluginsBarIndex && Configuration.PluginsBarVisible)
                        TogglePluginsBar(FALSE);
                    if (idx == Configuration.ExtensionBarIndex && Configuration.ExtensionBarVisible)
                        ToggleExtensionBar(FALSE);
                    if (idx == Configuration.UserMenuToolbarIndex && Configuration.UserMenuToolBarVisible)
                        ToggleUserMenuToolBar(FALSE);
                    if (idx == Configuration.HotPathsBarIndex && Configuration.HotPathsBarVisible)
                        ToggleHotPathsBar(FALSE);
                    if (idx == Configuration.DriveBarIndex && Configuration.DriveBarVisible)
                        ToggleDriveBar(Configuration.DriveBar2Visible, FALSE);
                }
                if (!menuInserted)
                {
                    TRACE_E("Inserting MenuBar. Configuration seems to be corrupted.");
                    Configuration.MenuIndex = 0;
                    InsertMenuBand();
                }
                if (Configuration.MiddleToolBarVisible)
                    ToggleMiddleToolBar();
                CreateAndInsertWorkerBand(); // insert the worker band at the end
            }

            GetValue(actKey, CONFIG_BOTTOMTOOLBARVISIBLE_REG, REG_DWORD,
                     &Configuration.BottomToolBarVisible, sizeof(DWORD));
            // If mandatory layout data is missing, LoadConfig returns FALSE and the caller
            // builds the default UI. Do not create the bottom toolbar here too, otherwise
            // the caller's default pass toggles it back off.
            if (ret && Configuration.BottomToolBarVisible)
                ToggleBottomToolBar();

            //      GetValue(actKey, CONFIG_SPACESELCALCSPACE, REG_DWORD,
            //               &Configuration.SpaceSelCalcSpace, sizeof(DWORD));
            GetValue(actKey, CONFIG_COUNTSIZESTAYONFILESYSTEM, REG_DWORD,
                     &Configuration.CountSizeStayOnFileSystem, sizeof(DWORD));
            GetValue(actKey, CONFIG_USETIMERESOLUTION, REG_DWORD,
                     &Configuration.UseTimeResolution, sizeof(DWORD));
            GetValue(actKey, CONFIG_TIMERESOLUTION, REG_DWORD,
                     &Configuration.TimeResolution, sizeof(DWORD));
            GetValue(actKey, CONFIG_IGNOREDSTSHIFTS, REG_DWORD,
                     &Configuration.IgnoreDSTShifts, sizeof(DWORD));
            GetValue(actKey, CONFIG_USEDRAGDROPMINTIME, REG_DWORD,
                     &Configuration.UseDragDropMinTime, sizeof(DWORD));
            GetValue(actKey, CONFIG_DRAGDROPMINTIME, REG_DWORD,
                     &Configuration.DragDropMinTime, sizeof(DWORD));

            GetValue(actKey, CONFIG_LASTFOCUSEDPAGE, REG_DWORD,
                     &Configuration.LastFocusedPage, sizeof(DWORD));
            if (!hasTabCaptionMode && Configuration.LastFocusedPage >= 2)
                Configuration.LastFocusedPage++;
            GetValue(actKey, CONFIG_CONFIGURATION_HEIGHT, REG_DWORD,
                     &Configuration.ConfigurationHeight, sizeof(DWORD));
            GetValue(actKey, CONFIG_CONFIGURATION_WIDTH, REG_DWORD,
                     &Configuration.ConfigurationWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_CONFIGURATION_TREE_WIDTH, REG_DWORD,
                     &Configuration.ConfigurationTreeWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_CONFIGURATION_VIEWS_RIGHT_WIDTH, REG_DWORD,
                     &Configuration.ConfigurationViewsRightWidth, sizeof(DWORD));
            GetValue(actKey, CONFIG_VIEWANDEDITEXPAND, REG_DWORD,
                     &Configuration.ViewersAndEditorsExpanded, sizeof(DWORD));
            GetValue(actKey, CONFIG_PACKEPAND, REG_DWORD,
                     &Configuration.PackersAndUnpackersExpanded, sizeof(DWORD));

            GetValue(actKey, CONFIG_CMDLINE_REG, REG_DWORD, &cmdLine, sizeof(DWORD));
            GetValue(actKey, CONFIG_CMDLFOCUS_REG, REG_DWORD, &cmdLineFocus, sizeof(DWORD));

            GetValue(actKey, CONFIG_USECUSTOMPANELFONT_REG, REG_DWORD, &UseCustomPanelFont, sizeof(DWORD));
            if (LoadLogFont(actKey, CONFIG_PANELFONT_REG, &LogFont) && UseCustomPanelFont)
            {
                // if the user uses a custom font, propagate it now
                SetFont();
            }

            LoadHistory(actKey, CONFIG_NAMEDHISTORY_REG, FindNamedHistory, FIND_NAMED_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_LOOKINHISTORY_REG, FindLookInHistory, FIND_LOOKIN_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_GREPHISTORY_REG, FindGrepHistory, FIND_GREP_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_SELECTHISTORY_REG, Configuration.SelectHistory, SELECT_HISTORY_SIZE);
            //      Guys (Honza Patera, Tomas Jelinek) didn't like this because when they
            //      launch a new instance, they don't remember the previous mask. They hit (Un)Select
            //      and the last mask is still there. FAR, VC, NC start with *.* when launched,
            //      we will behave the same way.
            //      if (Configuration.SelectHistory[0] != NULL)  // load the initial state of +/- selection as well
            //        strcpy(SelectionMask, Configuration.SelectHistory[0]);
            LoadHistory(actKey, CONFIG_COPYHISTORY_REG, Configuration.CopyHistory, COPY_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_CHANGEDIRHISTORY_REG, Configuration.ChangeDirHistory, CHANGEDIR_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_VIEWERHISTORY_REG, ViewerHistory, VIEWER_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_COMMANDHISTORY_REG, Configuration.EditHistory, EDIT_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_FILELISTHISTORY_REG, Configuration.FileListHistory, FILELIST_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_CREATEDIRHISTORY_REG, Configuration.CreateDirHistory, CREATEDIR_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_QUICKRENAMEHISTORY_REG, Configuration.QuickRenameHistory, QUICKRENAME_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_EDITNEWHISTORY_REG, Configuration.EditNewHistory, EDITNEW_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_CONVERTHISTORY_REG, Configuration.ConvertHistory, CONVERT_HISTORY_SIZE);
            LoadHistory(actKey, CONFIG_FILTERHISTORY_REG, Configuration.FilterHistory, FILTER_HISTORY_SIZE);
            if (DirHistory != NULL)
            {
                if (UsingSharedWorkDirHistory())
                    DirHistory->LoadFromRegistry(actKey, CONFIG_WORKDIRSHISTORY_REG);
                else
                    DirHistory->ClearHistory();
            }
            UpdateAllDirectoryLineHistoryStates();

            if (OpenKey(actKey, CONFIG_COPYMOVEOPTIONS_REG, actSubKey))
            {
                CopyMoveOptions.Load(actSubKey);
                CloseKey(actSubKey);
            }

            if (OpenKey(actKey, CONFIG_FINDOPTIONS_REG, actSubKey))
            {
                FindOptions.Load(actSubKey, Configuration.ConfigVersion);
                CloseKey(actSubKey);
            }

            if (OpenKey(actKey, CONFIG_FINDIGNORE_REG, actSubKey))
            {
                FindIgnore.Load(actSubKey, Configuration.ConfigVersion);
                CloseKey(actSubKey);
            }

            GetValue(actKey, CONFIG_FILELISTNAME_REG, REG_SZ, Configuration.FileListName, MAX_PATH);
            GetValue(actKey, CONFIG_FILELISTAPPEND_REG, REG_DWORD, &Configuration.FileListAppend, sizeof(DWORD));
            GetValue(actKey, CONFIG_FILELISTDESTINATION_REG, REG_DWORD, &Configuration.FileListDestination, sizeof(DWORD));

            CloseKey(actKey);
        }

        //---  viewer

        if (OpenKey(salamander, SALAMANDER_VIEWER_REG, actKey))
        {
            GetValue(actKey, VIEWER_FINDFORWARD_REG, REG_DWORD,
                     &GlobalFindDialog.Forward, sizeof(DWORD));
            GetValue(actKey, VIEWER_FINDWHOLEWORDS_REG, REG_DWORD,
                     &GlobalFindDialog.WholeWords, sizeof(DWORD));
            GetValue(actKey, VIEWER_FINDCASESENSITIVE_REG, REG_DWORD,
                     &GlobalFindDialog.CaseSensitive, sizeof(DWORD));
            GetValue(actKey, VIEWER_FINDREGEXP_REG, REG_DWORD,
                     &GlobalFindDialog.Regular, sizeof(DWORD));
            GetValue(actKey, VIEWER_FINDTEXT_REG, REG_SZ,
                     GlobalFindDialog.Text, FIND_TEXT_LEN);
            GetValue(actKey, VIEWER_FINDHEXMODE_REG, REG_DWORD,
                     &GlobalFindDialog.HexMode, sizeof(DWORD));

            GetValue(actKey, VIEWER_CONFIGCRLF_REG, REG_DWORD,
                     &Configuration.EOL_CRLF, sizeof(DWORD));
            GetValue(actKey, VIEWER_CONFIGCR_REG, REG_DWORD,
                     &Configuration.EOL_CR, sizeof(DWORD));
            GetValue(actKey, VIEWER_CONFIGLF_REG, REG_DWORD,
                     &Configuration.EOL_LF, sizeof(DWORD));
            GetValue(actKey, VIEWER_CONFIGNULL_REG, REG_DWORD,
                     &Configuration.EOL_NULL, sizeof(DWORD));
            GetValue(actKey, VIEWER_CONFIGTABSIZE_REG, REG_DWORD,
                     &Configuration.TabSize, sizeof(DWORD));
            GetValue(actKey, VIEWER_CONFIGDEFMODE_REG, REG_DWORD,
                     &Configuration.DefViewMode, sizeof(DWORD));
            // a bit ugly: we provide MasksString, but the range is checked so it's fine
            GetValue(actKey, VIEWER_CONFIGTEXTMASK_REG, REG_SZ,
                     Configuration.TextModeMasks.GetWritableMasksString(), MAX_PATH);
            if (Configuration.ConfigVersion < 17 &&
                strcmp(Configuration.TextModeMasks.GetWritableMasksString(), "*.txt;*.602") == 0)
            {
                strcpy(Configuration.TextModeMasks.GetWritableMasksString(), "*.txt;*.602;*.xml");
            }
            int errPos;
            Configuration.TextModeMasks.PrepareMasks(errPos);
            // a bit ugly: we provide MasksString, but the range is checked so it's fine
            GetValue(actKey, VIEWER_CONFIGHEXMASK_REG, REG_SZ,
                     Configuration.HexModeMasks.GetWritableMasksString(), MAX_PATH);
            Configuration.HexModeMasks.PrepareMasks(errPos);

            GetValue(actKey, VIEWER_CONFIGUSECUSTOMFONT_REG, REG_DWORD,
                     &UseCustomViewerFont, sizeof(DWORD));
            LoadLogFont(actKey, VIEWER_CONFIGFONT_REG, &ViewerLogFont); // no viewer can be open yet, so no need to call SetViewerFont()
            GetValue(actKey, VIEWER_WRAPTEXT_REG, REG_DWORD,
                     &Configuration.WrapText, sizeof(DWORD));
            GetValue(actKey, VIEWER_SHOWNUMBERS_REG, REG_DWORD,
                     &Configuration.ViewerShowLineNumbers, sizeof(DWORD));
            GetValue(actKey, VIEWER_SHOWSTATUS_REG, REG_DWORD,
                     &Configuration.ViewerShowStatusBar, sizeof(DWORD));
            GetValue(actKey, VIEWER_ZOOMPERCENT_REG, REG_DWORD,
                     &Configuration.ViewerZoomPercent, sizeof(DWORD));
            Configuration.ViewerZoomPercent = max(25, min(500, Configuration.ViewerZoomPercent));
            GetValue(actKey, VIEWER_CPAUTOSELECT_REG, REG_DWORD,
                     &Configuration.CodePageAutoSelect, sizeof(DWORD));
            GetValue(actKey, VIEWER_DEFAULTCONVERT_REG, REG_SZ, Configuration.DefaultConvert, 200);
            GetValue(actKey, VIEWER_AUTOCOPYSELECTION_REG, REG_DWORD,
                     &Configuration.AutoCopySelection, sizeof(DWORD));
            GetValue(actKey, VIEWER_GOTOOFFSETISHEX_REG, REG_DWORD,
                     &Configuration.GoToOffsetIsHex, sizeof(DWORD));

            GetValue(actKey, VIEWER_CONFIGSAVEWINPOS_REG, REG_DWORD,
                     &Configuration.SavePosition, sizeof(DWORD));
            BOOL plcmntExist = TRUE;
            plcmntExist &= GetValue(actKey, VIEWER_CONFIGWNDLEFT_REG, REG_DWORD,
                                    &Configuration.WindowPlacement.rcNormalPosition.left, sizeof(DWORD));
            plcmntExist &= GetValue(actKey, VIEWER_CONFIGWNDRIGHT_REG, REG_DWORD,
                                    &Configuration.WindowPlacement.rcNormalPosition.right, sizeof(DWORD));
            plcmntExist &= GetValue(actKey, VIEWER_CONFIGWNDTOP_REG, REG_DWORD,
                                    &Configuration.WindowPlacement.rcNormalPosition.top, sizeof(DWORD));
            plcmntExist &= GetValue(actKey, VIEWER_CONFIGWNDBOTTOM_REG, REG_DWORD,
                                    &Configuration.WindowPlacement.rcNormalPosition.bottom, sizeof(DWORD));
            plcmntExist &= GetValue(actKey, VIEWER_CONFIGWNDSHOW_REG, REG_DWORD,
                                    &Configuration.WindowPlacement.showCmd, sizeof(DWORD));
            if (plcmntExist)
                Configuration.WindowPlacement.length = sizeof(Configuration.WindowPlacement);

            CloseKey(actKey);
        }

        //---  left and right panel

        char leftPanelPath[MAX_PATH];
        char rightPanelPath[MAX_PATH];
        GetSystemDirectory(leftPanelPath, MAX_PATH);
        strcpy(rightPanelPath, leftPanelPath);
        char sysDefDir[32768];
        lstrcpyn(sysDefDir, DefaultDir[LowerCase[leftPanelPath[0]] - 'a'], _countof(sysDefDir));
        // Restoring each saved panel/tab path updates its monitoring state. Defer Tree View rebuilding
        // until the final active panel is focused below; rebuilding it for every restored path makes
        // startup perform the same synchronous directory enumeration many times.
        RestoringPanelPaths = TRUE;
        LoadPanelConfig(leftPanelPath, cpsLeft, salamander, SALAMANDER_LEFTP_REG);
        LoadPanelConfig(rightPanelPath, cpsRight, salamander, SALAMANDER_RIGHTP_REG);
        RestoringPanelPaths = FALSE;
        if (Configuration.WorkDirsHistoryScope == wdhsPerTab)
            RebuildSharedDirHistoryFromPanels();

        CloseKey(salamander);
        salamander = NULL;

        LoadSaveToRegistryMutex.Leave();

        //---  END OF LOADING CONFIGURATION

        if (cmdLine && !SystemPolicies.GetNoRun())
            PostMessage(HWindow, WM_COMMAND, CM_TOGGLEEDITLINE, TRUE);

        // Finish messages queued while the hidden main window and its controls were being
        // initialized. In particular, toolbar/rebar layout and panel paints must settle before
        // SetWindowPlacement reveals the window below; otherwise their intermediate state flashes
        // on screen and panel contents can visibly change immediately after startup.
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // set the active panel according to command line parameters
        if (ret && cmdLineParams != NULL)
        {
            if (cmdLineParams->ActivatePanel == 1 && rightPanelFocused ||
                cmdLineParams->ActivatePanel == 2 && !rightPanelFocused)
            {
                rightPanelFocused = !rightPanelFocused;
            }
        }

        FocusPanel(rightPanelFocused ? RightPanel : LeftPanel);
        (rightPanelFocused ? RightPanel : LeftPanel)->SetCaretIndex(0, FALSE);
        if (cmdLineFocus)
            SendMessage(HWindow, WM_COMMAND, CM_EDITLINE, 0);

        // this caused trouble:
        // when a panel pointed to an unavailable UNC path,
        // it would wait here for several seconds
        //    LeftPanel->UpdateDriveIcon(TRUE);
        //    RightPanel->UpdateDriveIcon(TRUE);
        //    RefreshMenuAndTB(TRUE);

        HMENU h = GetSystemMenu(HWindow, FALSE);
        if (h != NULL)
        {
            CheckMenuItem(h, CM_ALWAYSONTOP, MF_BYCOMMAND | (Configuration.AlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));

            char buff[200];
            MENUITEMINFO mii;
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_TYPE;
            mii.dwTypeData = buff;
            mii.cch = 199;

            GetMenuItemInfo(h, SC_MINIMIZE, FALSE, &mii);
            wsprintf(buff + strlen(buff), "\t%s+%s", LoadStr(IDS_SHIFT), LoadStr(IDS_ESCAPE));
            SetMenuItemInfo(h, SC_MINIMIZE, FALSE, &mii);

            mii.cch = 199;
            GetMenuItemInfo(h, SC_MAXIMIZE, FALSE, &mii);
            wsprintf(buff + strlen(buff), "\t%s+%s+%s", LoadStr(IDS_CTRL), LoadStr(IDS_SHIFT), LoadStr(IDS_F11));
            SetMenuItemInfo(h, SC_MAXIMIZE, FALSE, &mii);
        }

        SplashScreenCloseIfExist();
        if (Configuration.StatusArea)
            AddTrayIcon();

        SetWindowIcon();
        SetWindowTitle();

        SetWindowPos(HWindow,
                     Configuration.AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // show the window in full
        if (useWinPlacement)
        {
            // from MSDN:
            // ShowCmd: 0 = SW_SHOWNORMAL
            //          3 = SW_SHOWMAXIMIZED
            //          7 = SW_SHOWMINNOACTIVE

            // we don't want the application minimized on startup unless the user
            // defined it in a shortcut
            if (!Configuration.StatusArea)
            {
                switch (CmdShow)
                {
                case SW_SHOWNORMAL:
                {
                    // if the configuration specifies a minimized window, open it restored
                    if (place.showCmd == SW_MINIMIZE)
                        place.showCmd = SW_RESTORE;
                    if (place.showCmd == SW_SHOWMINIMIZED)
                        place.showCmd = SW_SHOWNORMAL;
                    break;
                }

                // settings in the shortcut take priority over the configuration
                case SW_SHOWMINNOACTIVE:
                case SW_SHOWMAXIMIZED:
                {
                    place.showCmd = CmdShow;
                    break;
                }
                }
            }
            else
            {
                switch (CmdShow)
                {
                case SW_SHOWNORMAL:
                {
                    // if the configuration specifies a minimized window, open it restored
                    if (place.showCmd == SW_MINIMIZE)
                        place.showCmd = SW_RESTORE;
                    if (place.showCmd == SW_SHOWMINIMIZED)
                        place.showCmd = SW_SHOWNORMAL;
                    break;
                }

                // settings in the shortcut take priority over the configuration
                case SW_SHOWMINNOACTIVE:
                {
                    place.showCmd = SW_HIDE;
                    PostMessage(HWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    break;
                }

                // settings in the shortcut take priority over the configuration
                case SW_SHOWMAXIMIZED:
                {
                    place.showCmd = CmdShow;
                    PostMessage(HWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
                    break;
                }
                }
            }
            SetWindowPlacement(HWindow, &place);
        }
        if (Configuration.DetachedPanels)
        {
            if (SetPanelsDetached(TRUE) &&
                Configuration.DetachedWindowPlacement.length != 0 &&
                HRightDetachedWindow != NULL)
            {
                WINDOWPLACEMENT detachedPlace = Configuration.DetachedWindowPlacement;
                detachedPlace.length = sizeof(WINDOWPLACEMENT);
                if (detachedPlace.showCmd == SW_MINIMIZE || detachedPlace.showCmd == SW_SHOWMINIMIZED)
                    detachedPlace.showCmd = SW_SHOWNORMAL;
                SetWindowPlacement(HRightDetachedWindow, &detachedPlace);
                LayoutDetachedPanels();
            }
        }
        LeftPanel->SetupListBoxScrollBars();
        RightPanel->SetupListBoxScrollBars();

        UpdateWindow(HWindow);

        // set panel paths according to command line parameters (all path types, including archives and FS)
        BOOL leftPanelPathSet = FALSE;
        BOOL rightPanelPathSet = FALSE;
        if (ret && cmdLineParams != NULL)
        {
            if (cmdLineParams->LeftPath[0] == 0 && cmdLineParams->RightPath[0] == 0 && cmdLineParams->ActivePath[0] != 0)
            {
                if (GetActivePanel()->ChangeDirLite(cmdLineParams->ActivePath)) // no point in combining this with left/right panel settings
                {
                    if (rightPanelFocused)
                        rightPanelPathSet = TRUE;
                    else
                    {
                        leftPanelPathSet = TRUE;
                        LeftPanel->RefreshVisibleItemsArray(); // see "RefreshVisibleItemsArray" comment below
                    }
                }
            }
            else
            {
                if (cmdLineParams->LeftPath[0] != 0)
                {
                    if (LeftPanel->ChangeDirLite(cmdLineParams->LeftPath))
                    {
                        leftPanelPathSet = TRUE;
                        LeftPanel->RefreshVisibleItemsArray(); // see "RefreshVisibleItemsArray" comment below
                    }
                }
                if (cmdLineParams->RightPath[0] != 0)
                {
                    if (RightPanel->ChangeDirLite(cmdLineParams->RightPath))
                        rightPanelPathSet = TRUE;
                }
            }
        }

        // save the array of visible items; normally this is done in idle time, but if it
        // should be ready so that icon reading for user menu entries has priority over icons
        // outside the visible part of the panel, we must handle it manually (icon loading
        // is already running, but sooner is better than later, this minimal delay should not hurt)
        if (rightPanelPathSet)
            RightPanel->RefreshVisibleItemsArray();

        // leftPanelPath and rightPanelPath are only disk paths; we don't store archives or FS paths
        DWORD err, lastErr;
        BOOL pathInvalid, cut;
        BOOL tryNet = TRUE;
        if (!leftPanelPathSet && !PanelConfigPathsRestoredLeft)
        {
            if (SalCheckAndRestorePathWithCut(LeftPanel->HWindow, leftPanelPath, tryNet,
                                              err, lastErr, pathInvalid, cut, TRUE))
            {
                LeftPanel->ChangePathToDisk(LeftPanel->HWindow, leftPanelPath);
            }
            else
                LeftPanel->ChangeToRescuePathOrFixedDrive(LeftPanel->HWindow);
            LeftPanel->RefreshVisibleItemsArray(); // see comment "RefreshVisibleItemsArray" above
        }
        UpdateWindow(LeftPanel->HWindow); // ensures dir/info line is drawn immediately after the panel content

        tryNet = TRUE;
        if (!rightPanelPathSet && !PanelConfigPathsRestoredRight)
        {
            if (SalCheckAndRestorePathWithCut(RightPanel->HWindow, rightPanelPath, tryNet,
                                              err, lastErr, pathInvalid, cut, TRUE))
            {
                RightPanel->ChangePathToDisk(RightPanel->HWindow, rightPanelPath);
            }
            else
                RightPanel->ChangeToRescuePathOrFixedDrive(RightPanel->HWindow);
            RightPanel->RefreshVisibleItemsArray(); // see comment "RefreshVisibleItemsArray" above
        }
        UpdateWindow(RightPanel->HWindow); // ensures dir/info line is drawn immediately after the panel content

        // restore default-dir on the system drive (damaged - system root was in both panels)
        lstrcpyn(DefaultDir[LowerCase[sysDefDir[0]] - 'a'], sysDefDir,
                 _countof(DefaultDir[LowerCase[sysDefDir[0]] - 'a']));
        // restore DefaultDir
        MainWindow->UpdateDefaultDir(TRUE);

        return ret;
    }

    LoadSaveToRegistryMutex.Leave();

    return FALSE;
}
