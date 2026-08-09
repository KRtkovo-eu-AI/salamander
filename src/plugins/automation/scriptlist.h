// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	scriptlist.h
	List of the scripts stored in the repositories.
*/

#pragma once

#include <string>
#include <vector>

#include "../salamatrix/salamatrix_manifest.h"
#include "../salamatrix/salamatrix_runtime_api.h"
#include "../salamatrix/salamatrix_ui.h"
#include "../salamatrix/salamatrix_extensions.h"
#include "../salamatrix/salamatrix_events.h"
#include "../salamatrix/salamatrix_storage.h"
#include "../salamatrix/salamatrix_script_runner.h"

BOOL ShowRuntimeInputBox(
    HWND parent,
    const std::string& title,
    const std::string& prompt,
    const std::string& initial,
    char* output,
    DWORD outputCapacity);

class CScriptInfo
{
public:
    struct RUNTIME_COMMAND_INFO
    {
        int MenuId;
        char Id[128];
        char Handler[128];
        TCHAR Title[256];
        bool PluginMenu;
        bool ContextMenu;
        bool Toolbar;
        DWORD HotKey;
        DWORD MenuEventOrMask;
        DWORD MenuEventAndMask;
        LONG Enabled;
        LONG Visible;

        RUNTIME_COMMAND_INFO()
            : MenuId(0),
              PluginMenu(false),
              ContextMenu(false),
              Toolbar(false),
              HotKey(0),
              MenuEventOrMask(MENU_EVENT_TRUE),
              MenuEventAndMask(MENU_EVENT_TRUE),
              Enabled(TRUE),
              Visible(TRUE)
        {
            Id[0] = '\0';
            Handler[0] = '\0';
            Title[0] = _T('\0');
        }
    };

    struct DEBUG_INFO
    {
        IProcessDebugManager* pProcDbgMgr;
        IDebugDocumentHelper* pDbgDocHelper;
        IDebugApplication* pDbgApp;
#ifdef _WIN64
        DWORDLONG dwSourceContext;
#else
        DWORD dwSourceContext;
#endif
        DWORD dwAppCookie;
    };

    struct EXECUTION_INFO
    {
        /// Deselect selection.
        bool bDeselect;

        /// Interface for displaying progress dialogs.
        CSalamanderForOperationsAbstract* pOperation;

        /// Enable script debugger.
        bool bEnableDebugger;

        // Internal members, do not touch.
        void* pInstance;
        bool bAsyncResult;
        // The native menu command that started this invocation. Persistent
        // extensions may register more than one command; this keeps the
        // selected command distinct from the manifest's default command.
        char SalamatrixCommandId[128];
        DEBUG_INFO dbgInfo;

        class CScriptAbortPalette* pAbortPalette;

        /// Constructor.
        EXECUTION_INFO()
        {
            memset(this, 0, sizeof(*this));
        }
    };

private:
    TCHAR m_szFileName[SAL_MAX_PATH];
    TCHAR m_szDisplayName[256];
    TCHAR m_szSalamatrixCommandId[128];
    char m_szRuntimeCommandId[128];
    char m_szSalamatrixExtensionId[128];
    char m_szSalamatrixRuntimeId[128];
    DWORD m_dwSalamatrixMinimumRuntimeVersion;
    std::vector<std::string> m_salamatrixCapabilities;
    std::vector<std::string> m_salamatrixDependencies;
    std::vector<CExtensionManifestSetting> m_salamatrixSettings;
    unsigned int m_salamatrixSettingsVersion;
    std::vector<CExtensionManifestSettingMigration> m_salamatrixSettingsMigrations;
    bool m_bSalamatrixEventsDeclared;
    std::vector<std::string> m_salamatrixEvents;
    struct SALAMATRIX_MANIFEST_COMMAND
    {
        std::string Id;
        std::string Handler;
        std::string Title;
        std::string Menu;
        std::string Requires;
        std::string IconPath;
        std::string IconDarkPath;
        bool ContextMenu;
        bool Toolbar;
        bool Enabled;
        bool Visible;
        DWORD MenuEventOrMask;
        DWORD MenuEventAndMask;

        SALAMATRIX_MANIFEST_COMMAND()
            : ContextMenu(false),
              Toolbar(false),
              Enabled(true),
              Visible(true),
              MenuEventOrMask(MENU_EVENT_TRUE),
              MenuEventAndMask(MENU_EVENT_TRUE)
        {
        }
    };
    std::vector<SALAMATRIX_MANIFEST_COMMAND> m_salamatrixManifestCommands;
    bool m_bSalamatrixManifestCommandsPublished;
    std::string m_salamatrixIconPath;
    std::string m_salamatrixIconDarkPath;
    bool m_bShowInPluginMenu;
    bool m_bShowInContextMenu;
    bool m_bManifestToolbar;
    bool m_bRuntimeCommandOwned;
    RUNTIME_COMMAND_INFO m_runtimeCommands[16];
    int m_nRuntimeCommands;
    DWORD m_dwMenuEventOrMask;
    DWORD m_dwMenuEventAndMask;
    CLSID m_clsidEngine;
    IActiveScript* m_pScript;
    class CScriptSite* m_pSite;
    EXECUTION_INFO* m_pExecInfo;
    bool m_bAbortPending; ///< AbortScript() was called.
    CScriptInfo* m_pNext;
    CScriptInfo* m_pNextHash;
    int m_nId;
    bool m_bDirty;
    class CScriptContainer* m_pContainer;
    IActiveScriptError* m_pHardError;
    bool m_bSiteErrorDisplayed; ///< Message box shown in site's OnScriptError.
    class CScriptEngineShim* m_pShim;
    HANDLE m_hAbortEvent; ///< Manually reset event signaled when the user requested abort.
    HWND m_hwndAbortTarget;
    Salamatrix::Runtime::IRuntimeSession* m_pRuntimeSession;
    volatile LONG m_lRuntimeStopping;
    HANDLE m_hRuntimePumpThread;
    ULONGLONG m_runtimeEventSubscriptions[8];
    int m_nRuntimeEventSubscriptions;
    struct RUNTIME_DIALOG
    {
        CScriptInfo* Owner;
        ULONGLONG Id;
        Salamatrix::UI::IDialog* Dialog;
        BOOL EventsEnabled;
        char EventName[128];
    };
    RUNTIME_DIALOG m_runtimeDialogs[8];
    int m_nRuntimeDialogs;
    ULONGLONG m_nextRuntimeDialogId;
    struct RUNTIME_PROGRESS
    {
        CScriptInfo* Owner;
        ULONGLONG Id;
        Salamatrix::UI::IProgressDialog* Dialog;
    };
    RUNTIME_PROGRESS m_runtimeProgress;
    ULONGLONG m_nextRuntimeProgressId;

    // statistics stuff
    LONG m_cExecuted;

    bool EnsureEngineAssociation();
    bool ExecuteThroughRuntime(__inout EXECUTION_INFO& info);
    bool ExecuteLegacy(__inout EXECUTION_INFO& info);
    static BOOL WINAPI ExecuteCompatibilityRuntime(
        void* context,
        Salamatrix::Runtime::RuntimeExecutionResult* result);

    bool CreateEngine(EXECUTION_INFO* info);

    HRESULT LoadScript(IActiveScriptParse* pParse, EXECUTION_INFO* info);

    static void FreeOleString(LPOLESTR s)
    {
        free(s);
    }

    static HRESULT LoadOleStringFromFile(PCTSTR pszFileName, __out LPOLESTR& s, __out_opt ULONG* cch);

    bool ExecuteWorker(__inout EXECUTION_INFO* info);
    bool ExecuteInSeparateThread(__inout EXECUTION_INFO* info);
    static DWORD WINAPI ExecuteEntryProc(void* arg);

    void InitializeDebugger(DEBUG_INFO* dbgInfo);
    void UninitializeDebugger(DEBUG_INFO* dbgInfo);
    void LoadSalamatrixMetadata();
    void LoadSalamatrixManifestMetadata();
    void InitializeSalamatrixSettings(
        Salamatrix::Storage::IStorageService* storage);
    void ApplySalamatrixMetadataLine(PCTSTR pszLine);
    void ApplySalamatrixManifestValue(const char* key, const char* value);
    void ApplySalamatrixPlacement(PCTSTR pszValue);
    void ApplySalamatrixRequires(PCTSTR pszValue);
    void ApplySalamatrixContextMenu(bool value);
    bool PublishSalamatrixManifestCommands();
    const char* FindRuntimeCommandHandler(const char* commandId) const;

    void ScriptEnter();
    void ScriptLeave();
    void ReleaseRuntimeSession();
    void ReleaseRuntimeProgress();
    static DWORD WINAPI RuntimePumpProc(void* arg);
    static BOOL WINAPI RuntimeEventCallback(
        void* context,
        const Salamatrix::Events::EventPayload* payload);
    static BOOL WINAPI RuntimeDialogEventCallback(
        void* context,
        const Salamatrix::UI::DialogEvent* event);
    void ReleaseRuntimeEventSubscriptions();
    void ReleaseRuntimeDialogs();
    void ReleaseRuntimeCommand();
    bool RegisterRuntimeCommand(
        const char* commandId,
        const char* title,
        const char* handler,
        bool pluginMenu,
        bool contextMenu,
        bool toolbar,
        DWORD hotKey,
        DWORD menuEventOrMask,
        DWORD menuEventAndMask,
        const char* iconPath = NULL,
        const char* iconDarkPath = NULL,
        bool enabled = true,
        bool visible = true);
    bool UnregisterRuntimeCommand(const char* commandId);
    bool SetRuntimeCommandState(
        const char* commandId,
        bool hasEnabled,
        bool enabled,
        bool hasVisible,
        bool visible);
    void ReleaseRuntimeCommands();
    int FindRuntimeCommandIndexByMenuId(int menuId) const;
    static BOOL WINAPI RuntimeLifecycleCallback(
        void* context,
        Salamatrix::Extensions::ExtensionAction action,
        const Salamatrix::Extensions::ExtensionInfo* info);
    static BOOL WINAPI RuntimeHostDispatch(
        void* context,
        Salamatrix::Runtime::Protocol::MessageType type,
        ULONGLONG requestId,
        const char* payloadJson,
        char* resultJson,
        DWORD resultCapacity,
        DWORD* resultLength);

    friend class CScriptLookup;
    friend class CScriptEngineShim;

public:
    // Public bridge entry points used by the Salamatrix AI runner. They keep
    // generated scripts on the same host-dispatch path as regular extensions.
    static BOOL WINAPI DispatchRuntimeHostCall(
        void* context,
        Salamatrix::Runtime::Protocol::MessageType type,
        ULONGLONG requestId,
        const char* payloadJson,
        char* resultJson,
        DWORD resultCapacity,
        DWORD* resultLength)
    {
        return RuntimeHostDispatch(
            context, type, requestId, payloadJson, resultJson,
            resultCapacity, resultLength);
    }

    static BOOL WINAPI DispatchCompatibilityRuntime(
        void* context,
        Salamatrix::Runtime::RuntimeExecutionResult* result)
    {
        return ExecuteCompatibilityRuntime(context, result);
    }

    static BOOL WINAPI DispatchCompatibilityRuntimeForScript(
        void* context,
        Salamatrix::Runtime::RuntimeExecutionResult* result)
    {
        CScriptInfo* script = static_cast<CScriptInfo*>(context);
        if (script == NULL || result == NULL)
            return FALSE;
        EXECUTION_INFO info;
        bool succeeded = script->ExecuteLegacy(info);
        result->Status = succeeded
                             ? Salamatrix::Runtime::RuntimeExecutionStatusSucceeded
                             : Salamatrix::Runtime::RuntimeExecutionStatusFailed;
        result->ErrorCode = succeeded ? S_OK : E_FAIL;
        return succeeded ? TRUE : FALSE;
    }

    /// Configures a temporary generated-script instance to use the modern
    /// runtime worker and the regular capability-aware host dispatcher.
    BOOL ConfigureGeneratedRuntime(const char* runtimeId,
                                   const char* extensionId);

    CScriptInfo(
        PCTSTR pszFileName,
        CScriptContainer* pContainer);

    ~CScriptInfo();

    PCTSTR GetFileName() const
    {
        return m_szFileName;
    }

    PCTSTR GetFileExt() const;

    PCTSTR GetDisplayName() const
    {
        return m_szDisplayName;
    }

    bool IsSalamatrixCommand() const
    {
        return m_szSalamatrixCommandId[0] != _T('\0');
    }

    PCTSTR GetSalamatrixCommandId() const
    {
        return m_szSalamatrixCommandId;
    }

    const char* GetSalamatrixRuntimeId() const
    {
        return m_szSalamatrixRuntimeId;
    }

    DWORD GetSalamatrixMinimumRuntimeVersion() const
    {
        return m_dwSalamatrixMinimumRuntimeVersion;
    }

    const char* GetSalamatrixExtensionId() const
    {
        return m_szSalamatrixExtensionId;
    }

    // Legacy Automation can execute a script after the package discovery
    // surface has been refreshed. Ensure Storage sees the manifest identity
    // even when the CScriptInfo instance predates that refresh.
    void EnsureSalamatrixManifestMetadata()
    {
        if (m_szSalamatrixExtensionId[0] == '\0')
            LoadSalamatrixManifestMetadata();
    }

    const char* GetSalamatrixIconPath() const
    {
        return m_salamatrixIconPath.c_str();
    }

    const char* GetSalamatrixIconDarkPath() const
    {
        return m_salamatrixIconDarkPath.c_str();
    }

    bool HasDeclaredSalamatrixCapabilities() const
    {
        return !m_salamatrixCapabilities.empty();
    }

    bool AllowsSalamatrixEvent(const char* eventName) const
    {
        if (eventName == NULL || eventName[0] == '\0')
            return FALSE;
        if (!m_bSalamatrixEventsDeclared)
            return TRUE;
        for (size_t index = 0; index < m_salamatrixEvents.size(); ++index)
        {
            if (_stricmp(m_salamatrixEvents[index].c_str(), eventName) == 0)
                return TRUE;
        }
        return FALSE;
    }

    bool HasSalamatrixCapability(const char* capability) const
    {
        if (capability == NULL || capability[0] == '\0')
            return FALSE;
        for (size_t index = 0; index < m_salamatrixCapabilities.size(); ++index)
        {
            const std::string& declared = m_salamatrixCapabilities[index];
            if (declared == "*" || _stricmp(declared.c_str(), "all") == 0 ||
                _stricmp(declared.c_str(), capability) == 0)
                return TRUE;
        }
        return FALSE;
    }

    bool ShowInPluginMenu() const
    {
        return m_bShowInPluginMenu;
    }

    bool ShowInContextMenu() const
    {
        return m_bShowInContextMenu;
    }

    DWORD GetMenuEventOrMask() const
    {
        return m_dwMenuEventOrMask;
    }

    DWORD GetMenuEventAndMask() const
    {
        return m_dwMenuEventAndMask;
    }

    REFCLSID GetEngineCLSID() const
    {
        _ASSERTE(m_clsidEngine != CLSID_NULL);
        return m_clsidEngine;
    }

    class CScriptEngineShim* GetShim()
    {
        return m_pShim;
    }

    HRESULT GetScriptEngine(__out IActiveScript** pScript)
    {
        _ASSERTE(pScript != NULL);
        _ASSERTE(m_pScript != NULL);
        *pScript = m_pScript;
        m_pScript->AddRef();
        return S_OK;
    }

    bool Execute(__inout EXECUTION_INFO& info);

    DEBUG_INFO* GetDebugInfo()
    {
        _ASSERTE(m_pExecInfo);
        return &m_pExecInfo->dbgInfo;
    }

    /// Aborts script execution.
    HRESULT AbortScript();

    int GetId() const
    {
        return m_nId;
    }

    int GetRuntimeCommandCount() const
    {
        return m_nRuntimeCommands;
    }

    const RUNTIME_COMMAND_INFO* GetRuntimeCommand(int index) const
    {
        return index >= 0 && index < m_nRuntimeCommands
                   ? &m_runtimeCommands[index]
                   : NULL;
    }

    bool IsRuntimeCommandEnabled(int index) const
    {
        return index >= 0 && index < m_nRuntimeCommands &&
               InterlockedCompareExchange(
                   const_cast<LONG*>(&m_runtimeCommands[index].Enabled),
                   0, 0) != 0;
    }

    bool IsRuntimeCommandVisible(int index) const
    {
        return index >= 0 && index < m_nRuntimeCommands &&
               InterlockedCompareExchange(
                   const_cast<LONG*>(&m_runtimeCommands[index].Visible),
                   0, 0) != 0;
    }

    int GetRuntimeCommandIndexByMenuId(int menuId) const
    {
        return FindRuntimeCommandIndexByMenuId(menuId);
    }

    const CScriptInfo* Next() const
    {
        return m_pNext;
    }

    void SetDirty()
    {
        m_bDirty = true;
    }

    void ResetDirty()
    {
        m_bDirty = false;
    }

    bool IsDirty() const
    {
        return m_bDirty;
    }

    void SetHardError(IActiveScriptError* pError)
    {
        _ASSERTE(pError != NULL);
        if (m_pHardError != NULL)
        {
            m_pHardError->Release();
            m_pHardError = NULL;
        }
        m_pHardError = pError;
        m_pHardError->AddRef();
    }

    void SetSiteError(IActiveScriptError* pError)
    {
        _ASSERTE(pError != NULL);
        UNREFERENCED_PARAMETER(pError);
        m_bSiteErrorDisplayed = true;
    }

    HANDLE GetAbortEvent() const
    {
        _ASSERTE(m_hAbortEvent != NULL);
        return m_hAbortEvent;
    }

    bool IsAbortPending() const
    {
        return m_bAbortPending;
    }

    HWND SetAbortTargetHwnd(HWND hwndAbortTarget)
    {
        return (HWND)InterlockedExchangePointer((void**)&m_hwndAbortTarget, (void*)hwndAbortTarget);
    }
};

class CScriptContainer
{
private:
    CScriptContainer* m_pSibling;
    CScriptContainer* m_pChild;
    CScriptContainer* m_pParent;
    CScriptInfo* m_pScripts;
    TCHAR m_szPath[SAL_MAX_PATH];
    PTSTR m_pszName;

    friend class CScriptLookup;

public:
    CScriptContainer();
    CScriptContainer(CScriptContainer* pParent, PCTSTR pszPath, bool bFullPath);
    ~CScriptContainer();

    void Clear();
    bool Fill(__in_z PCTSTR pszPath);

    PCTSTR GetPath() const
    {
        return m_szPath;
    }

    PCTSTR GetName() const
    {
        return m_pszName;
    }

    const CScriptContainer* FirstChild() const
    {
        return m_pChild;
    }

    const CScriptContainer* NextSibling() const
    {
        return m_pSibling;
    }

    const CScriptInfo* FirstScript() const
    {
        return m_pScripts;
    }

    CScriptContainer* FirstChild(PCTSTR pszPath, bool bFullPath);
};

class CScriptLookup
{
private:
    CScriptContainer* m_pRootContainer;
    CScriptInfo* m_apHashBins[37];
    int m_cScriptsTotal;
    bool m_bModified;
    DWORD m_dwLastRefreshTime;

    enum
    {
        HASH_SHIFT = 8,
        HASH_MASK = 0x7FFFFF00,
        UNIQUIER_MASK = 0xFF,
        UNIQUIER_MAX = 255,
    };

    UINT HashPath(__in_z PCTSTR pszPath);

    int FillContainer(
        CScriptContainer* pContainer,
        HKEY hKey,
        CSalamanderRegistryAbstract* registry);

    /// Inserts the subcontainer to the container.
    void LinkContainer(CScriptContainer* pContainer, CScriptContainer* pParent);

    void UnlinkContainer(CScriptContainer* pContainer);

    /// Inserts the script to the container.
    void LinkScript(CScriptInfo* pScript);

    /// Removed the script from the container.
    void UnlinkScript(CScriptInfo* pScript);

    /// Adds the script into the hash map.
    void LinkScriptHash(CScriptInfo* pScript);

    CScriptInfo* AddScriptFromFile(
        CScriptContainer* pContainer,
        PCTSTR pszFullPath,
        HKEY hKey,
        CSalamanderRegistryAbstract* registry);

    int GetUniquier(
        UINT nHash,
        __in_z PCTSTR pszPath,
        HKEY hKey,
        CSalamanderRegistryAbstract* registry);

    int MakeId(UINT nHash, int nUniquier)
    {
        _ASSERTE((nHash & ~HASH_MASK) == 0);
        _ASSERTE((nUniquier & ~UNIQUIER_MASK) == 0);
        return nHash + nUniquier;
    }

    UINT HashFromId(int nId)
    {
        return (((UINT)nId) & HASH_MASK) >> HASH_SHIFT;
    }

    int UniquierFromId(int nId)
    {
        return (nId & UNIQUIER_MASK);
    }

    bool SaveBin(
        CScriptInfo* pFirst,
        HKEY hKey,
        CSalamanderRegistryAbstract* registry);

    void CascadeDeleteContainer(CScriptContainer* pContainer);

    void MarkAllScriptsDirty();
    void RemoveDirtyScripts();
    void RemoveEmptyContainers(CScriptContainer* pContainer);

    CScriptInfo* LookupScriptByPath(UINT nHash, PCTSTR pszFullPath);

    /// This data structure holds bitmap of free uniquiers for
    /// single hash bucket.
    class CUniquierBitmap
    {
    private:
        enum
        {
            BITS_PER_ULONG = sizeof(ULONG) * 8,
        };

        /// This is the actual bitmap. If the bit is set, the
        /// uniquier is free.
        ULONG m_bitmap[(UNIQUIER_MAX + 1) / BITS_PER_ULONG];

    public:
        CUniquierBitmap()
        {
            memset(m_bitmap, 0xFF, sizeof(m_bitmap));
        }

        /// Marks the uniquier as allocated.
        void MarkBusy(int nUniquier)
        {
            _ASSERTE(nUniquier >= 0 || nUniquier <= UNIQUIER_MAX);
            m_bitmap[nUniquier / BITS_PER_ULONG] &= ~(1 << (nUniquier % BITS_PER_ULONG));
        }

        /// Finds first free uniquier.
        int Alloc()
        {
            ULONG ibit;
            for (int iword = 0; iword < _countof(m_bitmap); iword++)
            {
                if (BitScanForward(&ibit, m_bitmap[iword]))
                {
                    return (iword * BITS_PER_ULONG) + ibit;
                }
            }

            // uniquiers exhausted
            return -1;
        }
    };

public:
    CScriptLookup();
    ~CScriptLookup();

    bool Load(HKEY hKey, CSalamanderRegistryAbstract* registry);
    bool Save(HKEY hKey, CSalamanderRegistryAbstract* registry);

    bool Refresh(bool bForce = false);

    /// Publishes manifest-backed scripts into Salamatrix.Extensions.
    void PublishSalamatrixExtensions();

    /// Removes manifest-backed scripts from Salamatrix.Extensions.
    void UnpublishSalamatrixExtensions();

    CScriptInfo* LookupScript(int nId);
    CScriptInfo* LookupRuntimeCommand(int menuId);

    int GetCount() const
    {
        return m_cScriptsTotal;
    }

    const CScriptContainer* GetRootContainer() const
    {
        return m_pRootContainer;
    }

    bool IsModified() const
    {
        return m_bModified;
    }
};
