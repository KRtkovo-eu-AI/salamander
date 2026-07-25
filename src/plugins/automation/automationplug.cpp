// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	automationplug.cpp
	Automation plugin main object and menu extension.
*/

#include "precomp.h"
#include "automationplug.h"
#include "scriptlist.h"
#include "automation.rh2"
#include "lang\lang.rh"
#include "engassoc.h"
#include "cfgdlg.h"
#include "abortpalette.h"
#include "versinfo.rh2"
#include "persistence.h"
#include "abortmodal.h"

#include <vector>

#pragma comment(lib, "UxTheme.lib")

CAutomationMenuExtInterface g_oMenuExtInterface;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;
extern HINSTANCE g_hInstance;
extern HINSTANCE g_hLangInst;
extern CAutomationPluginInterface g_oAutomationPlugin;
CWindowQueue AbortPaletteWindowQueue("Automation Abort Palette Window");

static std::string EscapeAssistantContext(const char* value)
{
    std::string escaped;
    if (value == NULL)
        return escaped;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value);
         *p != '\0'; ++p)
    {
        if (*p == '\\') escaped.append("\\\\");
        else if (*p == '"') escaped.append("\\\"");
        else if (*p == '\n') escaped.append("\\n");
        else if (*p == '\r') escaped.append("\\r");
        else escaped.push_back(static_cast<char>(*p));
    }
    return escaped;
}

static std::string BuildAssistantPanelContext(
    Salamatrix::Sides::ISidesService* sides)
{
    if (sides == NULL)
        return "{}";
    const Salamatrix::Sides::SideReference side =
        Salamatrix::Sides::SideReferenceSource;
    std::vector<char> path(SALAMATRIX_SIDE_ITEM_PATH_CAPACITY);
    int pathType = 0;
    sides->GetPath(side, &path[0], static_cast<int>(path.size()), &pathType);
    std::string result =
        std::string("{\"source\":{\"path\":\"") +
        EscapeAssistantContext(&path[0]) + "\",\"pathType\":" +
        std::to_string(pathType) + ",\"selectedItems\":[";
    int selectedCount = sides->GetSelectedItemCount(side);
    int emitted = selectedCount < 32 ? selectedCount : 32;
    std::vector<Salamatrix::Sides::ItemInfo> itemBuffer(1);
    for (int index = 0; index < emitted; ++index)
    {
        Salamatrix::Sides::ItemInfo& item = itemBuffer[0];
        if (!sides->GetSelectedItem(side, index, &item))
            continue;
        if (index != 0)
            result.append(",");
        result += std::string("{\"name\":\"") +
                  EscapeAssistantContext(item.Name) + "\",\"path\":\"" +
                  EscapeAssistantContext(item.Path) + "\",\"isDirectory\":" +
                  (item.IsDirectory ? "true" : "false") + "}";
    }
    result += "]}}";
    return result;
}

static BOOL SaveAssistantScript(
    HWND parent,
    const char* script,
    TCHAR* savedPath,
    int savedPathCapacity)
{
    if (script == NULL || savedPath == NULL || savedPathCapacity <= 0)
        return FALSE;
    StringCchCopy(savedPath, savedPathCapacity, _T("salamatrix-script"));
    static const TCHAR filter[] =
        _T("Script files\0*.js;*.py;*.ps1;*.php\0All files\0*.*\0");
    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parent;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = savedPath;
    ofn.nMaxFile = savedPathCapacity;
    ofn.lpstrDefExt = _T("js");
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!SalamanderGeneral->SafeGetSaveFileName(&ofn))
        return FALSE;
    std::wstring widePath;
#ifdef UNICODE
    widePath.assign(savedPath);
#else
    int wideLength = MultiByteToWideChar(
        CP_ACP, 0, savedPath, -1, NULL, 0);
    if (wideLength <= 0)
        return FALSE;
    std::vector<wchar_t> converted(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(
            CP_ACP, 0, savedPath, -1, &converted[0], wideLength) <= 0)
        return FALSE;
    widePath.assign(&converted[0]);
#endif
    if (widePath.size() >= MAX_PATH &&
        widePath.compare(0, 4, L"\\\\?\\") != 0)
    {
        if (widePath.size() >= 2 && widePath[0] == L'\\' &&
            widePath[1] == L'\\')
            widePath = L"\\\\?\\UNC\\" + widePath.substr(2);
        else
            widePath = L"\\\\?\\" + widePath;
    }
    HANDLE file = CreateFileW(
        widePath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD length = static_cast<DWORD>(strlen(script));
    DWORD written = 0;
    BOOL result = WriteFile(file, script, length, &written, NULL) &&
                  written == length;
    CloseHandle(file);
    return result;
}

static const TCHAR CONFIG_VERSION[] = TEXT("Version");
static const UINT CURRENT_CONFIG_VERSION = 1;
static const TCHAR CONFIG_ENABLEDEBUGGER[] = TEXT("EnableDebugger");
static const TCHAR CONFIG_DIRECTORIES[] = TEXT("Directories");
static const TCHAR CONFIG_SCRIPTS[] = TEXT("Scripts");
static const TCHAR CONFIG_PERSISTENCE[] = TEXT("Persistent");

CScriptLookup g_oScriptLookup;
CPersistentValueStorage g_oPersistentStorage;

CAutomationMenuExtInterface::CAutomationMenuExtInterface()
{
    m_bDeferredPopup = false;
}

BOOL WINAPI CAutomationMenuExtInterface::ExecuteMenuItem(
    CSalamanderForOperationsAbstract* salamander,
    HWND parent,
    int id,
    DWORD eventMask)
{
    CScriptInfo::EXECUTION_INFO info;
    bool bExecuted = false;
    bool bRunScript = false;

    g_oAutomationPlugin.RefreshSalamatrixServices();

    info.pOperation = salamander;
    info.bEnableDebugger = g_oAutomationPlugin.IsDebuggerEnabled();

    if (id == CmdRunFocusedScript)
    {
        TCHAR szFullName[MAX_PATH];
        const CFileData* pFocusedFile;

        SalamanderGeneral->GetPanelPath(PANEL_SOURCE, szFullName, _countof(szFullName), NULL, NULL);
        pFocusedFile = SalamanderGeneral->GetPanelFocusedItem(PANEL_SOURCE, NULL);
        SalamanderGeneral->SalPathAppend(szFullName, pFocusedFile->Name, _countof(szFullName));

        CScriptInfo scriptInfo(szFullName, NULL);
        bExecuted = scriptInfo.Execute(info);
    }
    else if (id == CmdScriptPopupMenu)
    {
        m_bDeferredPopup = true;
        SalamanderGeneral->PostPluginMenuChanged();
    }
    else if (id == CmdOpenPopupMenu)
    {
        id = ExecuteScriptMenu();
        return ExecuteMenuItem(salamander, parent, id, eventMask);
    }
    else if (id == CmdAskAssistant)
    {
        CAutomationSalamatrixBridge* bridge =
            g_oAutomationPlugin.GetSalamatrixBridge();
        Salamatrix::AI::IAssistantService* assistant =
            bridge != NULL ? bridge->GetAssistantService() : NULL;
        Salamatrix::UI::IUIService* ui =
            bridge != NULL ? bridge->GetUIService() : NULL;
        char prompt[4096];
        if (assistant == NULL || ui == NULL ||
            !ShowRuntimeInputBox(
                parent,
                "Salamatrix AI",
                "What should the generated script do?",
                "",
                prompt,
                _countof(prompt)))
            return FALSE;
        std::string context = BuildAssistantPanelContext(
            bridge->GetSidesService());
        Salamatrix::AI::AssistantRequest request;
        request.Prompt = prompt;
        request.ContextJson = context.c_str();
        Salamatrix::AI::AssistantResponse response;
        BOOL generated = assistant->Generate(NULL, &request, &response);
        if (!generated)
        {
            ui->ShowMessageBox(
                parent,
                "The local assistant did not return a valid script.",
                "Salamatrix AI",
                MB_OK | MB_ICONWARNING);
            return FALSE;
        }
        std::string summary =
            std::string(response.Summary.Title) + "\n\n" +
            response.Summary.Description +
            "\n\nThe generated script is ready for review.\n";
        if (Salamatrix::AI::IsSafeToRun(response.Summary))
            summary += "Declared effects are read-only; no script was run automatically.";
        else
            summary += "The declared effects require explicit review; no script was run.";
        ui->CopyTextToClipboard(response.Summary.Script, TRUE, parent);
        ui->ShowMessageBox(
            parent,
            summary.c_str(),
            "Salamatrix AI preview (script copied to clipboard)",
            MB_OK | MB_ICONINFORMATION);
        int saveChoice = ui->ShowMessageBox(
            parent,
            "Save the generated script to a file now?",
            "Salamatrix AI",
            MB_YESNO | MB_ICONQUESTION);
        if (saveChoice == IDYES)
        {
            std::vector<TCHAR> savedPath(SAL_MAX_PATH);
            if (SaveAssistantScript(
                    parent,
                    response.Summary.Script,
                    &savedPath[0],
                    static_cast<int>(savedPath.size())))
            {
                ui->ShowMessageBox(
                    parent,
                    "The generated script was saved.",
                    "Salamatrix AI",
                    MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                ui->ShowMessageBox(
                    parent,
                    "The generated script could not be saved.",
                    "Salamatrix AI",
                    MB_OK | MB_ICONWARNING);
            }
        }
        return FALSE;
    }
    else
    {
        bRunScript = true;
    }

    if (bRunScript)
    {
        CScriptInfo* pScript = g_oScriptLookup.LookupScript(id);
        if (pScript == NULL)
            pScript = g_oScriptLookup.LookupRuntimeCommand(id);
        if (pScript)
        {
            bExecuted = pScript->Execute(info);
        }
    }

    return bExecuted ? info.bDeselect : FALSE;
}

DWORD WINAPI CAutomationMenuExtInterface::GetMenuItemState(
    int id,
    DWORD eventMask)
{
    if (id == CmdRunFocusedScript)
    {
        if ((eventMask & (MENU_EVENT_DISK | MENU_EVENT_FILE_FOCUSED)) != (MENU_EVENT_DISK | MENU_EVENT_FILE_FOCUSED))
        {
            // no file-on-disk focused
            return 0;
        }

        return CanExecuteFocusedItem() ? MENU_ITEM_STATE_ENABLED : 0;
    }
    else if (id == CmdScriptPopupMenu)
    {
        return MENU_ITEM_STATE_ENABLED;
    }
    else if (id == CmdAskAssistant)
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        const CAutomationSalamatrixBridge* bridge =
            g_oAutomationPlugin.GetSalamatrixBridge();
        return bridge != NULL && bridge->HasAssistant() && bridge->HasUI()
                   ? MENU_ITEM_STATE_ENABLED
                   : 0;
    }
    else
    {
        // preloaded script item
        CScriptInfo* pScript = g_oScriptLookup.LookupScript(id);
        if (pScript == NULL)
            pScript = g_oScriptLookup.LookupRuntimeCommand(id);
        if (pScript == NULL)
            return 0;

        int runtimeIndex = pScript->GetRuntimeCommandIndexByMenuId(id);
        if (runtimeIndex >= 0)
        {
            const CScriptInfo::RUNTIME_COMMAND_INFO* command =
                pScript->GetRuntimeCommand(runtimeIndex);
            if (command == NULL ||
                (eventMask & command->MenuEventAndMask) !=
                    command->MenuEventAndMask ||
                (eventMask & command->MenuEventOrMask) == 0)
                return 0;
            return MENU_ITEM_STATE_ENABLED;
        }

        if ((eventMask & pScript->GetMenuEventAndMask()) != pScript->GetMenuEventAndMask())
            return 0;

        if ((eventMask & pScript->GetMenuEventOrMask()) == 0)
            return 0;

        return MENU_ITEM_STATE_ENABLED;
    }
}

int CAutomationMenuExtInterface::ExecuteScriptMenu()
{
    POINT pt;
    int nCmd;
    CGUIMenuPopupAbstract* pMenu;
    MENU_ITEM_INFO mii;
    TCHAR szText[100];
    TCHAR szHotKeyText[64];

    pMenu = SalamanderGUI->CreateMenuPopup();

    SalamanderGeneral->GetFocusedItemMenuPos(&pt);

    /* used for the export_mnu.py script, which generates salmenu.mnu for Translator
   keep synchronized with the InsertItem() call below...
MENU_TEMPLATE_ITEM ExecuteScriptMenu[] =
{
        {MNTT_PB, 0
        {MNTT_IT, IDS_RUNFOCUSED
        {MNTT_PE, 0
};
*/
    // run focused script menu item
    memset(&mii, 0, sizeof(MENU_ITEM_INFO));
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_STATE | MENU_MASK_IMAGEINDEX;
    mii.Type = MENU_TYPE_STRING;
    mii.ID = CmdRunFocusedScript;
    mii.State = CanExecuteFocusedItem() ? 0 : MENU_STATE_GRAYED;
    LoadString(g_hLangInst, IDS_RUNFOCUSED, szText, _countof(szText));
    if (SalamanderGeneral->GetMenuItemHotKey(mii.ID, NULL, szHotKeyText, _countof(szHotKeyText)))
    {
        StringCchCat(szText, _countof(szText), TEXT("\t"));
        StringCchCat(szText, _countof(szText), szHotKeyText);
    }
    mii.String = szText;
    mii.ImageIndex = PluginIconRun;
    pMenu->InsertItem(0, TRUE, &mii);

    if (g_oScriptLookup.GetCount() > 0)
    {
        // separator
        memset(&mii, 0, sizeof(MENU_ITEM_INFO));
        mii.Mask = MENU_MASK_TYPE;
        mii.Type = MENU_TYPE_SEPARATOR;
        pMenu->InsertItem(1, TRUE, &mii);

        const CScriptContainer* pRootContainer = g_oScriptLookup.GetRootContainer();
        _ASSERTE(pRootContainer != NULL);
        AddScriptContainerToPopup(pRootContainer, pMenu, 0);
    }

    // The image lists are applied on existing submenus only,
    // so setup the image list after the menu structure is
    // built completely.
    pMenu->SetImageList(g_oAutomationPlugin.GetImageList(false), TRUE);
    pMenu->SetHotImageList(g_oAutomationPlugin.GetImageList(true), TRUE);

    nCmd = pMenu->Track(MENU_TRACK_RETURNCMD | MENU_TRACK_NONOTIFY,
                        pt.x, pt.y, SalamanderGeneral->GetMainWindowHWND(), NULL);

    SalamanderGUI->DestroyMenuPopup(pMenu);

    return nCmd;
}

void CAutomationMenuExtInterface::AddScriptContainerToPopup(
    const CScriptContainer* pContainer,
    CGUIMenuPopupAbstract* pMenu,
    int nLevel)
{
    MENU_ITEM_INFO mii = {
        0,
    };
    int i = 0;
    CGUIMenuPopupAbstract* pSubMenu = NULL;
    TCHAR szDisplayName[256];

    const CScriptContainer* pSubContainer;
    const CScriptInfo* pScript;

    if (nLevel > 1)
    {
        pSubMenu = SalamanderGUI->CreateMenuPopup();

        mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_SUBMENU;
        mii.Type = MENU_TYPE_STRING;
        mii.ID = 0;
        mii.SubMenu = pSubMenu;

        StringCchCopy(szDisplayName, _countof(szDisplayName), pContainer->GetName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));
        mii.String = szDisplayName;

        pMenu->InsertItem(INT_MAX, TRUE, &mii);
    }

    pSubContainer = pContainer->FirstChild();
    while (pSubContainer)
    {
        AddScriptContainerToPopup(pSubContainer, pSubMenu ? pSubMenu : pMenu, nLevel + 1);
        pSubContainer = pSubContainer->NextSibling();
    }

    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_IMAGEINDEX;
    mii.Type = MENU_TYPE_STRING;
    mii.ImageIndex = PluginIconScript;
    for (pScript = pContainer->FirstScript(); pScript; pScript = pScript->Next(), i++)
    {
        if (pScript->GetRuntimeCommandCount() > 0)
        {
            for (int commandIndex = 0;
                 commandIndex < pScript->GetRuntimeCommandCount();
                 ++commandIndex)
            {
                const CScriptInfo::RUNTIME_COMMAND_INFO* command =
                    pScript->GetRuntimeCommand(commandIndex);
                if (command == NULL || !command->PluginMenu)
                    continue;
                mii.ID = command->MenuId;
                StringCchCopy(
                    szDisplayName,
                    _countof(szDisplayName),
                    command->Title);
                SalamanderGeneral->DuplicateAmpersands(
                    szDisplayName, _countof(szDisplayName));
                mii.String = szDisplayName;
                if (pSubMenu != NULL)
                    pSubMenu->InsertItem(INT_MAX, TRUE, &mii);
                else
                    pMenu->InsertItem(INT_MAX, TRUE, &mii);
            }
            continue;
        }
        if (!pScript->ShowInPluginMenu())
            continue;

        mii.ID = pScript->GetId();

        StringCchCopy(szDisplayName, _countof(szDisplayName), pScript->GetDisplayName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));
        TCHAR szHotKeyText[100];
        if (SalamanderGeneral->GetMenuItemHotKey(pScript->GetId(), NULL, szHotKeyText, 100))
        {
            StringCchCat(szDisplayName, _countof(szDisplayName), "\t");
            StringCchCat(szDisplayName, _countof(szDisplayName), szHotKeyText);
        }
        mii.String = szDisplayName;

        if (pSubMenu != NULL)
        {
            pSubMenu->InsertItem(INT_MAX, TRUE, &mii);
        }
        else
        {
            pMenu->InsertItem(INT_MAX, TRUE, &mii);
        }
    }
}

void WINAPI CAutomationMenuExtInterface::BuildMenu(
    HWND parent,
    CSalamanderBuildMenuAbstract* salamander)
{
    // refresh script list
    g_oScriptLookup.Refresh();

    /* used for the export_mnu.py script, which generates salmenu.mnu for Translator
   keep synchronized with the salamander->AddMenuItem() call below...
MENU_TEMPLATE_ITEM PluginMenu[] =
{
        {MNTT_PB, 0
        {MNTT_IT, IDS_RUNFOCUSED
        {MNTT_IT, IDS_SCRIPTPOPUPMENU
	{MNTT_PE, 0
};
*/

    // run focused script menu item
    salamander->AddMenuItem(
        PluginIconRun,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_RUNFOCUSED),
        0,
        CAutomationMenuExtInterface::CmdRunFocusedScript,
        TRUE, // callGetState
        0,    // or-mask (ignored id callGetState == TRUE)
        0,    // and-mask (ignored id callGetState == TRUE)
        MENU_SKILLLEVEL_ALL);

    // open script menu
    salamander->AddMenuItem(
        -1,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_SCRIPTPOPUPMENU),
        SALHOTKEY('A', HOTKEYF_CONTROL | HOTKEYF_SHIFT),
        CAutomationMenuExtInterface::CmdScriptPopupMenu,
        TRUE,
        0,
        0,
        MENU_SKILLLEVEL_ALL);

    salamander->AddMenuItem(
        PluginIconRun,
        "Ask Salamatrix AI...",
        0,
        CAutomationMenuExtInterface::CmdAskAssistant,
        TRUE,
        0,
        0,
        MENU_SKILLLEVEL_ALL);

    // for our menu items we set callGetState to TRUE, so the
    // GetMenuItemState will be always called for the items which
    // forces our plugin to be loaded before first menu popup
    // and the items get refreshed

    if (g_oScriptLookup.GetCount() > 0)
    {
        // separator
        salamander->AddMenuItem(
            -1, // icon index
            NULL,
            0,     // hotkey
            0,     // id
            FALSE, // callGetState
            0,     // or-mask
            0,     // and-mask
            MENU_SKILLLEVEL_ALL);

        const CScriptContainer* pRootContainer = g_oScriptLookup.GetRootContainer();
        _ASSERTE(pRootContainer != NULL);
        AddScriptContainerToMenu(pRootContainer, salamander, 0);
    }

    // Salamander manages the icon list itself, so we must everytime
    // create a copy of that list that we pass to menu builder.
    CGUIIconListAbstract* pListCopy = SalamanderGUI->CreateIconList();
    if (pListCopy)
    {
        if (pListCopy->CreateAsCopy(g_oAutomationPlugin.GetIconList(), FALSE))
        {
            salamander->SetIconListForMenu(pListCopy);
        }
        else
        {
            _ASSERTE(0);
            SalamanderGUI->DestroyIconList(pListCopy);
        }
    }
    else
    {
        _ASSERTE(0);
    }

    if (m_bDeferredPopup)
    {
        m_bDeferredPopup = false;
        SalamanderGeneral->PostMenuExtCommand(CmdOpenPopupMenu, TRUE);
    }
}

void CAutomationMenuExtInterface::AddScriptContainerToMenu(
    const CScriptContainer* pContainer,
    CSalamanderBuildMenuAbstract* pMenuBuilder,
    int nLevel)
{
    const CScriptContainer* pSubContainer;
    const CScriptInfo* pScript;
    TCHAR szDisplayName[256];

    if (nLevel > 1)
    {
        StringCchCopy(szDisplayName, _countof(szDisplayName), pContainer->GetName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));

        pMenuBuilder->AddSubmenuStart(
            -1, // iconIndex
            szDisplayName,
            0,
            FALSE,
            MENU_EVENT_TRUE,
            MENU_EVENT_TRUE,
            MENU_SKILLLEVEL_ALL);
    }

    pSubContainer = pContainer->FirstChild();
    while (pSubContainer)
    {
        AddScriptContainerToMenu(pSubContainer, pMenuBuilder, nLevel + 1);
        pSubContainer = pSubContainer->NextSibling();
    }

    pScript = pContainer->FirstScript();
    while (pScript)
    {
        if (pScript->GetRuntimeCommandCount() > 0)
        {
            for (int commandIndex = 0;
                 commandIndex < pScript->GetRuntimeCommandCount();
                 ++commandIndex)
            {
                const CScriptInfo::RUNTIME_COMMAND_INFO* command =
                    pScript->GetRuntimeCommand(commandIndex);
                if (command == NULL ||
                    (!command->PluginMenu && !command->ContextMenu))
                    continue;
                StringCchCopy(
                    szDisplayName,
                    _countof(szDisplayName),
                    command->Title);
                SalamanderGeneral->DuplicateAmpersands(
                    szDisplayName, _countof(szDisplayName));
                pMenuBuilder->AddMenuItem(
                    PluginIconScript,
                    szDisplayName,
                    0,
                    command->MenuId,
                    TRUE,
                    command->MenuEventOrMask,
                    command->MenuEventAndMask,
                    MENU_SKILLLEVEL_ALL);
            }
            pScript = pScript->Next();
            continue;
        }
        if (!pScript->ShowInPluginMenu() && !pScript->ShowInContextMenu())
        {
            pScript = pScript->Next();
            continue;
        }

        StringCchCopy(szDisplayName, _countof(szDisplayName), pScript->GetDisplayName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));

        pMenuBuilder->AddMenuItem(
            PluginIconScript, // icon index
            szDisplayName,
            0,                // hotkey
            pScript->GetId(), // id
            TRUE,                         // callGetState
            pScript->GetMenuEventOrMask(),  // or-mask
            pScript->GetMenuEventAndMask(), // and-mask
            MENU_SKILLLEVEL_ALL);

        pScript = pScript->Next();
    }

    if (nLevel > 1)
    {
        pMenuBuilder->AddSubmenuEnd();
    }
}

bool CAutomationMenuExtInterface::CanExecuteFocusedItem()
{
    // check if we have script engine for the file extension
    const CFileData* pFocusedFile = SalamanderGeneral->GetPanelFocusedItem(PANEL_SOURCE, NULL);
    // _ASSERTE(pFocusedFile);  // Petr: if the panel is empty (e.g. when we are in the root of an empty disk)
    if (pFocusedFile == NULL || pFocusedFile->Ext == NULL || *pFocusedFile->Ext == _T('\0'))
    {
        return false;
    }

    if (!g_oScriptAssociations.FindEngineByExt(pFocusedFile->Ext - 1))
    {
        // there is no script engine registered for this extension
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

CAutomationPluginInterface::CAutomationPluginInterface() : m_aDirectories(4, 1)
{
    m_pIcons = NULL;
    m_himlHot = NULL;
    m_himlCold = NULL;
    m_bEnableDebugger = false;

    GetModuleFileName(NULL, m_szSalDir, _countof(m_szSalDir));
    PTSTR pszNamePart = PathFindFileName(m_szSalDir);
    if (pszNamePart)
    {
        *pszNamePart = _T('\0');
    }
}

CAutomationPluginInterface::~CAutomationPluginInterface()
{
}

void CAutomationPluginInterface::Connect(
    HWND parent,
    CSalamanderConnectAbstract* salamander)
{
    CGUIIconListAbstract* pIcons;
    BOOL bLoaded;

    RefreshSalamatrixServices();

    pIcons = SalamanderGUI->CreateIconList();
    _ASSERTE(pIcons);

    bLoaded = pIcons->CreateFromPNG(g_hInstance, MAKEINTRESOURCE(IDB_ICONSTRIP), 16);
    _ASSERTE(bLoaded);
    if (bLoaded)
    {
        salamander->SetIconListForGUI(pIcons);
        salamander->SetPluginIcon(PluginIconMain);
        salamander->SetPluginMenuAndToolbarIcon(PluginIconMain);

        m_pIcons = SalamanderGUI->CreateIconList();
        if (!m_pIcons->CreateAsCopy(pIcons, FALSE))
        {
            SalamanderGUI->DestroyIconList(m_pIcons);
            m_pIcons = NULL;
        }

        m_himlHot = pIcons->GetImageList();
        _ASSERTE(m_himlHot != NULL);

        m_himlCold = NULL;
        CGUIIconListAbstract* pColdIcons = SalamanderGUI->CreateIconList();
        _ASSERTE(pColdIcons);
        if (pColdIcons->CreateAsCopy(pIcons, TRUE))
        {
            m_himlCold = pColdIcons->GetImageList();
            _ASSERTE(m_himlCold);
        }
        else
        {
            _ASSERTE(0);
        }
        SalamanderGUI->DestroyIconList(pColdIcons);
    }
    else
    {
        SalamanderGUI->DestroyIconList(pIcons);
        pIcons = NULL;
    }
}

void CAutomationPluginInterface::RefreshSalamatrixServices()
{
    m_oSalamatrix.Refresh(SalamanderGeneral);
}

void CAutomationPluginInterface::About(HWND parent)
{
    TCHAR szMessage[512];
    TCHAR szSalamatrixStatus[256];

    RefreshSalamatrixServices();
    m_oSalamatrix.GetStatusText(szSalamatrixStatus, _countof(szSalamatrixStatus));

    StringCchPrintf(szMessage, _countof(szMessage),
                    TEXT("%s ") TEXT(VERSINFO_VERSION) TEXT("\n\n")
                        TEXT(VERSINFO_COPYRIGHT) TEXT("\n\n")
                            TEXT("%s\n\n")
                                TEXT("Salamatrix Framework: %s"),
                    SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
                    SalamanderGeneral->LoadStr(g_hLangInst, IDS_DESCRIPTION),
                    szSalamatrixStatus);

    SalamanderGeneral->SalMessageBox(
        parent,
        szMessage,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_ABOUT),
        MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI CAutomationPluginInterface::Release(HWND parent, BOOL force)
{
    g_oScriptLookup.UnpublishSalamatrixExtensions();
    m_oSalamatrix.Reset();
    ReleaseWinLib(g_hInstance);
    UninitializeAbortableModalDialogWrapper();

    ImageList_Destroy(m_himlHot);
    m_himlHot = NULL;

    ImageList_Destroy(m_himlCold);
    m_himlCold = NULL;

    SalamanderGUI->DestroyIconList(m_pIcons);
    m_pIcons = NULL;

    return TRUE;
}

void WINAPI CAutomationPluginInterface::LoadConfiguration(
    HWND parent,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    DWORD dwVersion;
    DWORD dwArbitrary;
    DIRECTORY_INFO dir;

    CALL_STACK_MESSAGE1("CAutomationPluginInterface::LoadConfiguration(, ,)");

    if (!hKey)
    {
        dwVersion = CURRENT_CONFIG_VERSION;
    }
    else if (!registry->GetValue(hKey, CONFIG_VERSION, REG_DWORD, &dwVersion, sizeof(DWORD)))
    {
        dwVersion = 0;
    }

    if (hKey && registry->GetValue(hKey, CONFIG_ENABLEDEBUGGER, REG_DWORD, &dwArbitrary, sizeof(DWORD)))
    {
        m_bEnableDebugger = !!dwArbitrary;
    }
    else
    {
        m_bEnableDebugger = false;
    }

    bool bLoadDefaultDirs = true;

    if (hKey)
    {
        HKEY hkDirs;

        if (registry->OpenKey(hKey, CONFIG_DIRECTORIES, hkDirs))
        {
            TCHAR szValName[16];
            int iVal;

            bLoadDefaultDirs = false;

            for (iVal = 1;; iVal++)
            {
                _itot_s(iVal, szValName, _countof(szValName), 10);
                if (!registry->GetValue(hkDirs, szValName, REG_SZ, dir.szDirectory, _countof(dir.szDirectory)))
                {
                    break;
                }

                m_aDirectories.Add(dir);
            }

            registry->CloseKey(hkDirs);
        }
    }

    if (bLoadDefaultDirs)
    {
        dir.Set(_T("$[AppData]\\Open Salamander\\Automation\\scripts"));
        m_aDirectories.Add(dir);

        dir.Set(_T("$[AllUsersProfile]\\Open Salamander\\Automation\\scripts"));
        m_aDirectories.Add(dir);

        dir.Set(_T("$(SalDir)\\plugins\\automation\\scripts"));
        m_aDirectories.Add(dir);
    }

    bool bLookupLoaded = false;
    if (hKey)
    {
        HKEY hkScripts;

        if (registry->OpenKey(hKey, CONFIG_SCRIPTS, hkScripts))
        {
            bLookupLoaded = g_oScriptLookup.Load(hkScripts, registry);
            registry->CloseKey(hkScripts);
        }
    }

    if (!bLookupLoaded)
    {
        g_oScriptLookup.Load(NULL, registry);
    }

    g_oScriptLookup.PublishSalamatrixExtensions();

    if (hKey)
    {
        HKEY hkPersist;

        if (registry->OpenKey(hKey, CONFIG_PERSISTENCE, hkPersist))
        {
            g_oPersistentStorage.Load(hkPersist, registry);
            registry->CloseKey(hkPersist);
        }
    }
}

void WINAPI CAutomationPluginInterface::SaveConfiguration(
    HWND parent,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    CALL_STACK_MESSAGE1("CAutomationPluginInterface::SaveConfiguration(, ,)");

    if (hKey != NULL)
    {
        DWORD dwArbitrary;

        dwArbitrary = CURRENT_CONFIG_VERSION;
        registry->SetValue(hKey, CONFIG_VERSION, REG_DWORD, &dwArbitrary, sizeof(DWORD));

        dwArbitrary = m_bEnableDebugger;
        registry->SetValue(hKey, CONFIG_ENABLEDEBUGGER, REG_DWORD, &dwArbitrary, sizeof(DWORD));

        HKEY hkDirs;
        if (registry->CreateKey(hKey, CONFIG_DIRECTORIES, hkDirs))
        {
            TCHAR szValName[16];
            int iVal;

            registry->ClearKey(hkDirs);

            for (iVal = 0; iVal < m_aDirectories.Count; iVal++)
            {
                _itot_s(iVal + 1, szValName, _countof(szValName), 10);
                if (!registry->SetValue(hkDirs, szValName, REG_SZ,
                                        m_aDirectories.At(iVal).szDirectory, -1))
                {
                    break;
                }
            }

            registry->CloseKey(hkDirs);
        }

        if (g_oScriptLookup.IsModified())
        {
            HKEY hkScripts;

            if (registry->CreateKey(hKey, CONFIG_SCRIPTS, hkScripts))
            {
                g_oScriptLookup.Save(hkScripts, registry);
                registry->CloseKey(hkScripts);
            }
        }

        if (g_oPersistentStorage.IsModified())
        {
            HKEY hkPersist;

            if (registry->CreateKey(hKey, CONFIG_PERSISTENCE, hkPersist))
            {
                g_oPersistentStorage.Save(hkPersist, registry);
                registry->CloseKey(hkPersist);
            }
        }
    }
}

void WINAPI CAutomationPluginInterface::Configuration(HWND hwndParent)
{
    CALL_STACK_MESSAGE1("CAutomationPluginInterface::Configuration()");

    CAutomationConfigDialog(hwndParent).Execute();
}

bool CAutomationPluginInterface::ExpandPath(
    __in PCTSTR pszPath,
    __out_ecount(cchMax) PTSTR pszExpanded,
    __in int cchMax)
{
    static const CSalamanderVarStrEntry variables[] =
        {
            _T("SalDir"),
            ExpandSalDir,
            NULL,
            NULL,
        };

    if (!SalamanderGeneral->ExpandVarString(
            NULL, // hwndParent
            pszPath,
            pszExpanded,
            cchMax,
            variables,
            this))
    {
        return false;
    }

    return true;
}

/*static*/ PCTSTR CALLBACK CAutomationPluginInterface::ExpandSalDir(
    HWND hwndParent,
    void* pContext)
{
    CAutomationPluginInterface* that = reinterpret_cast<CAutomationPluginInterface*>(pContext);
    _ASSERTE(that);

    return that->m_szSalDir;
}

void CAutomationPluginInterface::Event(int event, DWORD param)
{
    switch (event)
    {
    case PLUGINEVENT_SETTINGCHANGE:
    {
        AbortPaletteWindowQueue.BroadcastMessage(CScriptAbortPaletteWindow::WM_USER_SETTINGCHANGE, 0, 0);
        break;
    }
    }
}
