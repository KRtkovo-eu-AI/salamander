// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include <string>
#include <shlwapi.h>
#undef PathIsPrefix

#include "tooltip.h"
#include "stswnd.h"
#include "plugins.h"
#include "fileswnd.h"
#include "filesbox.h"
#include "mainwnd.h"
#include "tabwnd.h"
#include "editwnd.h"
#include "cfgdlg.h"
#include "dialogs.h"
#include "usermenu.h"
#include "toolbar.h"
#include "shellib.h"
#include "menu.h"
#include "pack.h"
#include "gui.h"
#include "execute.h"
#include "jumplist.h"
#include "darkmode.h"
#include "titlebar_builder.h"
#include "common/widepath.h"
#include "common/winlibdpi.h"

#include "versinfo.rh2"

const char* SALAMANDER_TEXT_VERSION = "Open Salamander 5.0 Samandarin " VERSINFO_SAMANDARIN_VERSION " (" SAL_VER_PLATFORM ") ";

static void Utf8SafeCopyWindowTitle(char* target, int targetSize, const char* source);

//****************************************************************************
//
// ToolTip's calls redirection
//

void SetCurrentToolTip(HWND hNotifyWindow, DWORD id, int showDelay)
{
    if (MainWindow != NULL && MainWindow->ToolTip != NULL)
        MainWindow->ToolTip->SetCurrentToolTip(hNotifyWindow, id, showDelay);
}

void SetCurrentPanelToolTip(HWND hNotifyWindow, DWORD id, int showDelay)
{
    if (MainWindow != NULL && MainWindow->ToolTip != NULL)
        MainWindow->ToolTip->SetCurrentPanelToolTip(hNotifyWindow, id, showDelay);
}

void SuppressToolTipOnCurrentMousePos()
{
    if (MainWindow != NULL && MainWindow->ToolTip != NULL)
        MainWindow->ToolTip->SuppressToolTipOnCurrentMousePos();
}

void RefreshToolTip()
{
    if (MainWindow != NULL && MainWindow->ToolTip != NULL)
        PostMessage(MainWindow->ToolTip->HWindow, WM_USER_REFRESHTOOLTIP, 0, 0); // ask the window to load new text and redraw
}

//****************************************************************************
//
// CHotPathItems
//

const char* SALAMANDER_HOTPATHS_NAME = "Name";
const char* SALAMANDER_HOTPATHS_PATH = "Path";
const char* SALAMANDER_HOTPATHS_VISIBLE = "Visible";

BOOL CHotPathItems::SwapItems(int index1, int index2)
{
    // CHotPathItem has no destructor, so we can assign it directly to a local variable
    CHotPathItem item = Items[index1];
    Items[index1] = Items[index2];
    Items[index2] = item;
    return TRUE;
}

void CHotPathItems::FillHotPathsMenu(CMenuPopup* menu, int minCommand, BOOL emptyItems, BOOL emptyEcho,
                                     BOOL customize, BOOL topSeparator, BOOL forAssign)
{
    CALL_STACK_MESSAGE1("CHotPathItems::FillMenu()");
    char root[MAX_PATH + 100];
    BOOL menuIsEmpty = TRUE;
    int firstIndex = menu->GetItemCount();
    MENU_ITEM_INFO mii;
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STATE | MENU_MASK_STRING | MENU_MASK_ICON;
    mii.Type = MENU_TYPE_STRING;
    mii.State = 0;
    char name[MAX_PATH];
    int i;
    for (i = 0; i < HOT_PATHS_COUNT; i++)
    {
        BOOL assigned = GetPathLen(i) > 0 && GetNameLen(i) > 0;
        if (i >= 10)
        {
            if (emptyItems)
                emptyItems = FALSE; // starting with the tenth hot path we display the trimmed array
        }
        if (emptyItems || assigned)
        {
            menuIsEmpty = FALSE;
            GetName(i, name, MAX_PATH);
            DuplicateAmpersands(name, MAX_PATH);
            if (i < 10)
            {
                int key = (i == 9 ? 0 : i + 1);
                if (emptyItems)
                    sprintf(root, "%s\t%s+%s+%d", name, LoadStr(IDS_CTRL), LoadStr(IDS_SHIFT), key);
                else
                    sprintf(root, "%s\t%s+%d", name, LoadStr(IDS_CTRL), key);
            }
            else
                sprintf(root, "%s", name);
            mii.ID = minCommand + i;
            mii.String = root;
            mii.HIcon = assigned ? HFavoritIcon : NULL;
            menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
        }
    }
    int unassignedIndex = GetUnassignedHotPathIndex();
    if (forAssign && unassignedIndex != -1)
    {
        mii.ID = minCommand + unassignedIndex;
        mii.String = LoadStr(IDS_NEWHOTPATH);
        mii.HIcon = NULL;
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
    }

    if (!menuIsEmpty && topSeparator)
    {
        mii.Mask = MENU_MASK_TYPE;
        mii.Type = MENU_TYPE_SEPARATOR;
        menu->InsertItem(firstIndex, TRUE, &mii);
    }

    if (emptyEcho && !emptyItems && menuIsEmpty)
    {
        mii.Mask = MENU_MASK_TYPE | MENU_MASK_STATE | MENU_MASK_STRING;
        mii.Type = MENU_TYPE_STRING;
        mii.State = MENU_STATE_GRAYED;
        mii.String = LoadStr(IDS_EMPTYHOTPATHS);
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
    }

    if (customize)
    {
        // add a separator and the configuration option
        mii.Mask = MENU_MASK_TYPE;
        mii.Type = MENU_TYPE_SEPARATOR;
        mii.HIcon = NULL;
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);

        mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STATE | MENU_MASK_STRING;
        mii.Type = MENU_TYPE_STRING;
        mii.State = 0;
        mii.ID = CM_CUSTOMIZE_HOTPATHS;
        mii.String = LoadStr(IDS_CUSTOMIZE_HOTPATHS);
        mii.HIcon = NULL;
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
    }
}

int CHotPathItems::GetUnassignedHotPathIndex()
{
    for (int i = 10; i < HOT_PATHS_COUNT; i++)
    {
        BOOL assigned = GetPathLen(i) > 0 && GetNameLen(i) > 0;
        if (!assigned)
            return i;
    }
    return -1;
}

BOOL CHotPathItems::CleanName(char* name)
{
    char* start = name;
    char* end = name + strlen(name) - 1;
    while (*start != 0 && *start == ' ')
        start++;
    while (end >= name && *end == ' ')
        end--;
    end++;
    *end = 0;
    if (start > name && start < end)
        memmove(name, start, end - start + 1);
    return strlen(name) > 0;
}

BOOL CHotPathItems::Save(HKEY hKey)
{
    char keyName[5];
    int i;
    for (i = 0; i < HOT_PATHS_COUNT; i++)
    {
        itoa(i + 1, keyName, 10);
        HKEY actKey;
        if (CreateKey(hKey, keyName, actKey))
        {
            const char* name = "";
            if (GetNameLen(i) > 0)
                name = Items[i].Name;
            const char* path = "";
            if (GetPathLen(i) > 0)
                path = Items[i].Path;
            DWORD visible = Items[i].Visible;

            if (*name == 0 && *path == 0 && visible == TRUE)
            {
                // optimization: don't clutter the registry unless needed
                // not ready for configuration merging, but neither is the rest of our configuration
                ClearKey(actKey);
                CloseKey(actKey);
                DeleteKey(hKey, keyName);
            }
            else
            {
                SetValue(actKey, SALAMANDER_HOTPATHS_NAME, REG_SZ, name, -1);
                SetValue(actKey, SALAMANDER_HOTPATHS_PATH, REG_SZ, path, -1);
                SetValue(actKey, SALAMANDER_HOTPATHS_VISIBLE, REG_DWORD, &visible, sizeof(DWORD));
                CloseKey(actKey);
            }
        }
    }
    return TRUE;
}

BOOL CHotPathItems::Load(HKEY hKey)
{
    char keyName[5];
    int i;
    for (i = 0; i <= HOT_PATHS_COUNT; i++)
    {
        itoa(i, keyName, 10);
        HKEY actKey;
        if (OpenKey(hKey, keyName, actKey))
        {
            // when there were only 10 hot paths, the tenth entry was stored under key '0', so we attempt to load it here
            int index = (i == 0) ? 9 : i - 1;
            char name[MAX_PATH];
            char path[HOTPATHITEM_MAXPATH];
            DWORD visible;
            name[0] = 0;
            path[0] = 0;
            visible = TRUE;
            GetValue(actKey, SALAMANDER_HOTPATHS_NAME, REG_SZ, name, MAX_PATH);
            CleanName(name);
            if (GetValue(actKey, SALAMANDER_HOTPATHS_PATH, REG_SZ, path, HOTPATHITEM_MAXPATH))
            {
                if (Configuration.ConfigVersion < 47)            // the old path limit was MAX_PATH, so it fits with expansion
                    DuplicateDollars(path, HOTPATHITEM_MAXPATH); // if the path is long and contains '$', the end might be truncated; we ignore it
            }
            GetValue(actKey, SALAMANDER_HOTPATHS_VISIBLE, REG_DWORD, &visible, sizeof(DWORD));

            Set(index, name, path, visible);
            CloseKey(actKey);
        }
    }
    return TRUE;
}

BOOL CHotPathItems::Load1_52(HKEY hKey)
{
    // convert configuration from version 1.52 to 1.6
    char keyName[5];
    int i;
    for (i = 0; i < HOT_PATHS_COUNT; i++)
    {
        char name[MAX_PATH];
        char path[MAX_PATH];
        DWORD visible;
        name[0] = 0;
        path[0] = 0;
        visible = FALSE; // do not display converted paths because they are long

        itoa(i, keyName, 10);
        if (GetValue(hKey, keyName, REG_SZ, path, MAX_PATH))
        {
            DuplicateDollars(path, MAX_PATH); // if the path is long and contains '$', the end might be truncated; ignore it
            strcpy(name, path);
        }

        Set(i == 0 ? 9 : i - 1, name, path, visible);
    }
    return TRUE;
}

//
// ****************************************************************************
// CMainWindow
//

class CDetachedTabsLock
{
private:
    CRITICAL_SECTION* Lock;

public:
    explicit CDetachedTabsLock(CRITICAL_SECTION* lock) : Lock(lock)
    {
        HANDLES(EnterCriticalSection(Lock));
    }
    ~CDetachedTabsLock()
    {
        HANDLES(LeaveCriticalSection(Lock));
    }
};

CMainWindow::CMainWindow()
    : ChangeNotifArray(3, 5), LeftPanelTabs(1, 1, dtNoDelete), RightPanelTabs(1, 1, dtNoDelete)
{
    HANDLES(InitializeCriticalSection(&DispachChangeNotifCS));
    HANDLES(InitializeCriticalSection(&DetachedTabsCS));
    LastDispachChangeNotifTime = 0;
    NeedToResentDispachChangeNotif = FALSE;
    DoNotLoadAnyPlugins = FALSE;

    ActivateSuspMode = 0;
    FirstActivateApp = TRUE;
    UserMenuItems = new CUserMenuItems(10, 5);
    ViewerMasks = new CViewerMasks(10, 5);
    HANDLES(InitializeCriticalSection(&ViewerMasksCS));
    AltViewerMasks = new CViewerMasks(10, 5);
    EditorMasks = new CEditorMasks(10, 5);
    HighlightMasks = new CHighlightMasks(10, 5);
    DetachedFSList = new CDetachedFSList;
    DirHistory = new CPathHistory(TRUE);
    CanAddToDirHistory = FALSE;
    FileHistory = new CFileHistory;
    LeftPanel = RightPanel = NULL;
    SetActivePanel(NULL);
    EditWindow = NULL;
    EditMode = FALSE;
    HelpMode = HELP_INACTIVE;
    EditPermanentVisible = FALSE;
    TopToolBar = NULL;
    PluginsBar = NULL;
    ExtensionBar = NULL;
    MiddleToolBar = NULL;
    UMToolBar = NULL;
    HPToolBar = NULL;
    DriveBar = NULL;
    DriveBar2 = NULL;
    BottomToolBar = NULL;
    LeftTabWindow = NULL;
    RightTabWindow = NULL;
    PanelTabCrossDragActive = false;
    PanelTabCrossDragSourceSide = cpsLeft;
    PanelTabCrossDragSourceIndex = -1;
    PanelTabCrossDragHasTarget = false;
    PanelTabCrossDragDisplayedInsertIndex = -1;
    PanelTabCrossDragDisplayedMarkItem = -1;
    PanelTabCrossDragDisplayedMarkFlags = 0;
    PanelTabCrossDragStoredInsertIndex = -1;
    PanelTabCrossDragStoredMarkItem = -1;
    PanelTabCrossDragStoredMarkFlags = 0;
    PendingPanelTabContextCommand = 0;
    PendingPanelTabContextTabId = 0;
    PendingPanelTabContextSide = cpsLeft;
    HPanelTabDetachPreview = NULL;
    PanelTabMouseWheelAccumulator = 0;
    PanelTabMouseWheelSwitchTime = 0;
    //AnimateBar = NULL;
    //  TipOfTheDayDialog = NULL;
    HTopRebar = NULL;
    MenuBar = NULL;
    WindowWidth = WindowHeight = EditHeight = 0;
    SplitPosition = 0.5; // split is in the middle
    BeforeZoomSplitPosition = 0.5;
    BeforeZoomVisibleLeftRatio = 0.5;
    KeepSplitPositionCenteredOnVisiblePanes = FALSE;
    PanelZoomedState = 0;
    DragMode = FALSE;
    ContextMenuNew = new CMenuNew;
    ContextMenuChngDrv = NULL;
    TaskbarRestartMsg = 0;
    CanClose = FALSE;
    CanCloseButInEndSuspendMode = FALSE;
    SaveCfgInEndSession = FALSE;
    WaitInEndSession = FALSE;
    DisableIdleProcessing = FALSE;
    strcpy(SelectionMask, "*.*");
    Created = FALSE;
    RestoringPanelPaths = FALSE;
    StartupWindowCloaked = FALSE;
    DetachedPanels = FALSE;
    CreatingDetachedChrome = FALSE;
    DetachedPanelsSwapFixNeeded = FALSE;
    WindowPosSizeUpdatePending = FALSE;
    LayoutWindowsInProgress = FALSE;
    MainWindowSizeInProgress = FALSE;
    DetachedDPIRefreshInProgress = FALSE;
    DetachedDPIRefreshPosted = FALSE;
    DetachedTabDPIRefreshPosted = FALSE;
    //  DrivesControlHWnd = NULL;
    HDisabledKeyboard = NULL;
    CmdShow = SW_SHOWNORMAL;
    IdleStatesChanged = TRUE;
    PluginsStatesChanged = FALSE;
    CaptionIsActive = FALSE;
    SHChangeNotifyRegisterID = 0;
    IgnoreWM_SETTINGCHANGE = FALSE;
    LockedUI = FALSE;
    LockedUIToolWnd = NULL;
    LockedUIReason = NULL;
    HLeftDetachedWindow = NULL;
    HRightDetachedWindow = NULL;
    HDetachedTopRebar = NULL;
    DetachedMenuBar = NULL;
    DetachedTopToolBar = NULL;
    DetachedPluginsBar = NULL;
    DetachedExtensionBar = NULL;
    DetachedUMToolBar = NULL;
    DetachedHPToolBar = NULL;
    DetachedDriveBar = NULL;
    DetachedDriveBar2 = NULL;
    DetachedBottomToolBar = NULL;
    DetachedEditWindow = NULL;
    HDetachedGrayToolBarImageList = NULL;
    HDetachedHotToolBarImageList = NULL;
    HDetachedBottomTBImageList = NULL;
    HDetachedHotBottomTBImageList = NULL;
    DetachedWindowDPI = 0;
    PreserveDetachedPanelsOnShutdown = FALSE;
    DetachedTabPanel = NULL;
    DetachedTabOriginalSide = cpsLeft;
    DetachedTabOriginalIndex = -1;
    HDetachedTabWindow = NULL;
    HDetachedTabGrayToolBarImageList = NULL;
    HDetachedTabHotToolBarImageList = NULL;
    DetachedTabWindowDPI = 0;
    MainWindowTitlePanel = NULL;

    PanelConfigPathsRestoredLeft = FALSE;
    PanelConfigPathsRestoredRight = FALSE;

    ToolTip = new CToolTip(ooStatic);

    // viewers
    CViewerMasksItem* item;

    item = new CViewerMasksItem();
    if (ViewerMasks != NULL && item != NULL)
    {
        ViewerMasks->Add(item); // no critical section needed, we're in the constructor
        item->Set("*.htm;*.html;*.xml;*.mht", "", "", "");
        item->ViewerType = -4; // IE viewer (4th plugin in the default configuration)
    }

    item = new CViewerMasksItem();
    if (ViewerMasks != NULL && item != NULL)
    {
        ViewerMasks->Add(item); // no critical section needed, we're in the constructor
        item->Set("*.rpm", "", "", "");
        item->ViewerType = -2; // TAR (2nd plugin in the default configuration)
    }

    item = new CViewerMasksItem();
    if (ViewerMasks != NULL && item != NULL)
    {
        ViewerMasks->Add(item); // no critical section needed, we're in the constructor
        item->Set("*.*", "", "", "");
        item->ViewerType = VIEWER_INTERNAL; // internal viewer
    }

    item = new CViewerMasksItem();
    if (AltViewerMasks != NULL && item != NULL)
    {
        AltViewerMasks->Add(item);
        item->Set("*.*", "", "", "");
        item->ViewerType = VIEWER_INTERNAL; // internal viewer
    }

    CEditorMasksItem* eItem;
    eItem = new CEditorMasksItem();
    if (EditorMasks != NULL && eItem != NULL)
    {
        EditorMasks->Add(eItem);
        eItem->Set("*.*", "notepad.exe", "\"$(Name)\"", "$(FullPath)");
    }

    if (HighlightMasks != NULL)
    {
        CHighlightMasksItem* hItem = new CHighlightMasksItem();
        if (hItem != NULL)
        {
            HighlightMasks->Add(hItem);
            hItem->Set("*.*");
            int errPos;
            hItem->Masks->PrepareMasks(errPos);
            hItem->NormalFg = RGBF(0, 0, 255, 0);
            hItem->FocusedFg = RGBF(0, 0, 255, 0);
            hItem->ValidAttr = FILE_ATTRIBUTE_COMPRESSED;
            hItem->Attr = FILE_ATTRIBUTE_COMPRESSED;
        }

        hItem = new CHighlightMasksItem();
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
}

BOOL CMainWindow::IsGood()
{
    return LeftPanel != NULL && LeftPanel->IsGood() &&
           RightPanel != NULL && RightPanel->IsGood() &&
           EditWindow != NULL && EditWindow->IsGood() &&
           UserMenuItems != NULL && ViewerMasks != NULL &&
           AltViewerMasks != NULL && EditorMasks != NULL &&
           HighlightMasks != NULL && ContextMenuNew != NULL &&
           ToolTip != NULL && DetachedFSList != NULL &&
           FileHistory != NULL && DirHistory != NULL;
}

CMainWindow::~CMainWindow()
{
    HANDLES(DeleteCriticalSection(&DispachChangeNotifCS));
    if (HPanelTabDetachPreview != NULL)
    {
        DestroyWindow(HPanelTabDetachPreview);
        HPanelTabDetachPreview = NULL;
    }
    DestroyDetachedChrome();
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        if (DetachedTabs[i].HWindow != NULL)
            DestroyWindow(DetachedTabs[i].HWindow);
        if (DetachedTabs[i].HHotToolBarImageList != NULL)
            ImageList_Destroy(DetachedTabs[i].HHotToolBarImageList);
        if (DetachedTabs[i].HGrayToolBarImageList != NULL)
            ImageList_Destroy(DetachedTabs[i].HGrayToolBarImageList);
    }
    DetachedTabs.clear();
    HANDLES(DeleteCriticalSection(&DetachedTabsCS));
    if (HLeftDetachedWindow != NULL)
        DestroyWindow(HLeftDetachedWindow);
    if (HRightDetachedWindow != NULL)
        DestroyWindow(HRightDetachedWindow);
    if (FileHistory != NULL)
        delete FileHistory;
    if (DetachedFSList != NULL)
        delete DetachedFSList;
    if (DirHistory != NULL)
        delete DirHistory;
    if (UserMenuItems != NULL)
        delete UserMenuItems;
    if (ViewerMasks != NULL)
        delete ViewerMasks;
    HANDLES(DeleteCriticalSection(&ViewerMasksCS));
    if (AltViewerMasks != NULL)
        delete AltViewerMasks;
    if (EditorMasks != NULL)
        delete EditorMasks;
    if (HighlightMasks != NULL)
        delete HighlightMasks;
    if (ContextMenuNew != NULL)
        delete ContextMenuNew;
    if (ToolTip != NULL)
        delete ToolTip;
    if (LeftTabWindow != NULL)
        delete LeftTabWindow;
    if (RightTabWindow != NULL)
        delete RightTabWindow;
    for (int side = 0; side < 2; ++side)
    {
        ClosedPanelTabs[side].clear();
    }
    MainWindow = NULL;
}

CFilesWindow* CMainWindow::GetOtherPanel(CFilesWindow* panel)
{
    if (panel == NULL)
        return NULL;
    return panel->IsLeftPanel() ? RightPanel : LeftPanel;
}

void CMainWindow::ClearHistory()
{
    if (FileHistory != NULL)
        FileHistory->ClearHistory();

    if (DirHistory != NULL)
        DirHistory->ClearHistory();

    TIndirectArray<CFilesWindow>& leftTabs = GetPanelTabs(cpsLeft);
    for (int i = 0; i < leftTabs.Count; i++)
        leftTabs[i]->ClearWorkDirHistory();
    TIndirectArray<CFilesWindow>& rightTabs = GetPanelTabs(cpsRight);
    for (int i = 0; i < rightTabs.Count; i++)
        rightTabs[i]->ClearWorkDirHistory();
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
        if (DetachedTabs[i].Panel != NULL)
            DetachedTabs[i].Panel->ClearWorkDirHistory();

    UpdateAllDirectoryLineHistoryStates();
    RefreshCommandStates();
}

void CMainWindow::UpdateDefaultDir(BOOL activePrefered)
{
    CFilesWindow *active, *nonactive;
    if (activePrefered)
    {
        nonactive = GetActivePanel();
        active = GetNonActivePanel();
    }
    else
    {
        active = GetActivePanel();
        nonactive = GetNonActivePanel();
    }
    const char* pathActive = active->GetPath();
    if (!active->Is(ptPluginFS) && pathActive[0] != '\\')
    {
        lstrcpyn(DefaultDir[LowerCase[pathActive[0]] - 'a'], pathActive,
                 _countof(DefaultDir[LowerCase[pathActive[0]] - 'a']));
    }
    const char* pathPasive = nonactive->GetPath();
    if (!nonactive->Is(ptPluginFS) && pathPasive[0] != '\\')
    {
        lstrcpyn(DefaultDir[LowerCase[pathPasive[0]] - 'a'], pathPasive,
                 _countof(DefaultDir[LowerCase[pathPasive[0]] - 'a']));
    }
}

BOOL CMainWindow::ToggleTopToolBar(BOOL storePos)
{
    CALL_STACK_MESSAGE2("CMainWindow::ToggleTopToolBar(%d)", storePos);
    if (TopToolBar == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);

    if (TopToolBar->HWindow != NULL)
    {
        TopToolBar->Save(Configuration.TopToolBar);
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_TOPTOOLBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        //    InvalidateRect(HTopRebar, NULL, TRUE);
        //    UpdateWindow(HTopRebar);
        DestroyWindow(TopToolBar->HWindow);
        Configuration.TopToolBarVisible = FALSE;
        if (storePos)
            StoreBandsPos();
    }
    else
    {
        if (!TopToolBar->CreateWnd(HTopRebar))
            return FALSE;
        TopToolBar->Load(Configuration.TopToolBar);
        IdleForceRefresh = TRUE;  // force an update
        IdleRefreshStates = TRUE; // on next Idle, enforce a check on status variables
        InsertTopToolbarBand();
        ShowWindow(TopToolBar->HWindow, SW_SHOW);
        Configuration.TopToolBarVisible = TRUE;
        if (storePos)
            StoreBandsPos();
    }

    if (lockWindowUpdate)
        LockWindowUpdate(NULL);

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }

    return TRUE;
}

void CMainWindow::RefreshExtensionToolbars()
{
    if (ExtensionBar != NULL && ExtensionBar->HWindow != NULL)
        ExtensionBar->CreateExtensionButtons(HGrayToolBarImageList,
                                             HHotToolBarImageList);
    if (DetachedExtensionBar != NULL &&
        DetachedExtensionBar->HWindow != NULL)
        DetachedExtensionBar->CreateExtensionButtons(
            HDetachedGrayToolBarImageList, HDetachedHotToolBarImageList);

    LayoutWindows();
}

BOOL CMainWindow::ToggleExtensionBar(BOOL storePos)
{
    CALL_STACK_MESSAGE2("CMainWindow::ToggleExtensionBar(%d)", storePos);
    if (ExtensionBar == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);
    if (ExtensionBar->HWindow != NULL)
    {
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX,
                                     BANDID_EXTENSIONBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        DestroyWindow(ExtensionBar->HWindow);
        Configuration.ExtensionBarVisible = FALSE;
        if (storePos)
            StoreBandsPos();
    }
    else
    {
        if (!ExtensionBar->CreateWnd(HTopRebar))
            return FALSE;
        ExtensionBar->CreateExtensionButtons(HGrayToolBarImageList,
                                             HHotToolBarImageList);
        InsertExtensionBarBand();
        ShowWindow(ExtensionBar->HWindow, SW_SHOW);
        Configuration.ExtensionBarVisible = TRUE;
        if (storePos)
            StoreBandsPos();
    }
    if (lockWindowUpdate)
        LockWindowUpdate(NULL);

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }
    return TRUE;
}

BOOL CMainWindow::TogglePluginsBar(BOOL storePos)
{
    CALL_STACK_MESSAGE2("CMainWindow::TogglePluginsBar(%d)", storePos);
    if (PluginsBar == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);

    if (PluginsBar->HWindow != NULL)
    {
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_PLUGINSBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        DestroyWindow(PluginsBar->HWindow);
        Configuration.PluginsBarVisible = FALSE;
        if (storePos)
            StoreBandsPos();
    }
    else
    {
        if (!PluginsBar->CreateWnd(HTopRebar))
            return FALSE;
        //    IdleForceRefresh = TRUE;   // force an update
        //    IdleRefreshStates = TRUE;  // on next Idle, enforce a check on status variables
        PluginsBar->CreatePluginButtons();
        InsertPluginsBarBand();
        ShowWindow(PluginsBar->HWindow, SW_SHOW);
        Configuration.PluginsBarVisible = TRUE;
        if (storePos)
            StoreBandsPos();
    }

    if (lockWindowUpdate)
        LockWindowUpdate(NULL);

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }

    return TRUE;
}

BOOL CMainWindow::ToggleMiddleToolBar()
{
    CALL_STACK_MESSAGE1("CMainWindow::ToggleMiddleToolBar()");
    if (MiddleToolBar == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);

    if (MiddleToolBar->HWindow != NULL)
    {
        MiddleToolBar->Save(Configuration.MiddleToolBar);
        DestroyWindow(MiddleToolBar->HWindow);
        Configuration.MiddleToolBarVisible = FALSE;
    }
    else
    {
        if (!MiddleToolBar->CreateWnd(HWindow))
            return FALSE;
        MiddleToolBar->Load(Configuration.MiddleToolBar);
        IdleForceRefresh = TRUE;  // force an update
        IdleRefreshStates = TRUE; // on next Idle, enforce a check on status variables
        ShowWindow(MiddleToolBar->HWindow, SW_SHOW);
        Configuration.MiddleToolBarVisible = TRUE;
    }

    if (lockWindowUpdate)
        LockWindowUpdate(NULL);

    return TRUE;
}

BOOL CMainWindow::ToggleUserMenuToolBar(BOOL storePos)
{
    CALL_STACK_MESSAGE2("CMainWindow::ToggleUserMenuToolBar(%d)", storePos);
    if (UMToolBar == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);

    if (UMToolBar->HWindow != NULL)
    {
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_UMTOOLBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        //    InvalidateRect(HTopRebar, NULL, TRUE);
        //    UpdateWindow(HTopRebar);
        DestroyWindow(UMToolBar->HWindow);
        Configuration.UserMenuToolBarVisible = FALSE;
        if (storePos)
            StoreBandsPos();
    }
    else
    {
        if (!UMToolBar->CreateWnd(HTopRebar))
            return FALSE;
        UMToolBar->CreateButtons();
        UMToolBar->SetFont();
        InsertUMToolbarBand();
        ShowWindow(UMToolBar->HWindow, SW_SHOW);
        Configuration.UserMenuToolBarVisible = TRUE;
        if (storePos)
            StoreBandsPos();
    }

    if (lockWindowUpdate)
        LockWindowUpdate(NULL);

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }

    return TRUE;
}

BOOL CMainWindow::ToggleHotPathsBar(BOOL storePos)
{
    CALL_STACK_MESSAGE2("CMainWindow::ToggleHotPathsBar(%d)", storePos);
    if (HPToolBar == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);

    if (HPToolBar->HWindow != NULL)
    {
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_HPTOOLBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        DestroyWindow(HPToolBar->HWindow);
        Configuration.HotPathsBarVisible = FALSE;
        if (storePos)
            StoreBandsPos();
    }
    else
    {
        if (!HPToolBar->CreateWnd(HTopRebar))
            return FALSE;
        HPToolBar->CreateButtons();
        HPToolBar->SetFont();
        InsertHPToolbarBand();
        ShowWindow(HPToolBar->HWindow, SW_SHOW);
        Configuration.HotPathsBarVisible = TRUE;
        if (storePos)
            StoreBandsPos();
    }

    if (lockWindowUpdate)
        LockWindowUpdate(NULL);

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }

    return TRUE;
}

BOOL CMainWindow::ToggleDriveBar(BOOL twoDriveBars, BOOL storePos)
{
    CALL_STACK_MESSAGE3("CMainWindow::ToggleDriveBar(%d, %d)", twoDriveBars, storePos);
    if (DriveBar == NULL || DriveBar2 == NULL)
        return FALSE;

    const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && !StartupWindowCloaked;
    if (lockWindowUpdate)
        LockWindowUpdate(HWindow);

    BOOL forceShow = TRUE;
    BOOL twoBarsVisible = (DriveBar2->HWindow != NULL);

    if (DriveBar->HWindow != NULL)
    {
        forceShow = FALSE;
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_DRIVEBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        DestroyWindow(DriveBar->HWindow);
        Configuration.DriveBarVisible = FALSE;
    }
    if (DriveBar2->HWindow != NULL)
    {
        forceShow = FALSE;
        int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_DRIVEBAR2, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        DestroyWindow(DriveBar2->HWindow);
        Configuration.DriveBar2Visible = FALSE;
    }

    if (forceShow || twoBarsVisible != twoDriveBars)
    {
        if (!DriveBar->CreateWnd(HTopRebar))
            return FALSE;
        if (twoDriveBars && !DriveBar2->CreateWnd(HTopRebar))
            return FALSE;

        DriveBar->CreateDriveButtons(NULL);
        DriveBar->SetFont();

        if (twoDriveBars)
        {
            DriveBar2->CreateDriveButtons(DriveBar);
            DriveBar2->SetFont();
        }

        InsertDriveBarBand(twoDriveBars);

        ShowWindow(DriveBar->HWindow, SW_SHOW);
        Configuration.DriveBarVisible = TRUE;

        if (twoDriveBars)
        {
            ShowWindow(DriveBar2->HWindow, SW_SHOW);
            Configuration.DriveBar2Visible = TRUE;
        }
        else
            Configuration.DriveBar2Visible = FALSE;
        if (storePos)
            StoreBandsPos();
    }

    if (lockWindowUpdate)
        LockWindowUpdate(NULL);
    //  InvalidateRect(HTopRebar, NULL, TRUE);
    //  UpdateWindow(HTopRebar);
    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }

    return TRUE;
}

BOOL CMainWindow::ToggleBottomToolBar()
{
    CALL_STACK_MESSAGE1("CMainWindow::ToggleBottomToolBar()");
    if (BottomToolBar == NULL)
        return FALSE;
    if (BottomToolBar->HWindow != NULL)
    {
        DestroyWindow(BottomToolBar->HWindow);
        BottomToolBar->SetState(btbsCount); // on the next display, load some valid state
        Configuration.BottomToolBarVisible = FALSE;
    }
    else
    {
        if (!CBottomToolBar::InitDataFromResources())
            return FALSE;
        if (!BottomToolBar->CreateWnd(HWindow))
            return FALSE;
        BottomToolBar->SetFont();
        UpdateBottomToolBar();
        ShowWindow(BottomToolBar->HWindow, SW_SHOW);
        Configuration.BottomToolBarVisible = TRUE;
    }

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }

    return TRUE;
}


BOOL CMainWindow::ToggleTreeView()
{
    CALL_STACK_MESSAGE1("CMainWindow::ToggleTreeView()");

    double visibleLeftRatio = GetVisibleLeftPanelRatio();

    Configuration.TreeViewVisible = !Configuration.TreeViewVisible;

    LeftPanel->UpdateTreeView(TRUE);
    RightPanel->UpdateTreeView(DetachedPanels);

    if (KeepSplitPositionCenteredOnVisiblePanes)
        UpdateCenteredSplitPosition();
    else
        SplitPosition = GetSplitPositionForVisibleLeftPanelRatio(visibleLeftRatio);

    return TRUE;
}

void CMainWindow::ToggleToolBarGrips()
{
    CALL_STACK_MESSAGE1("CMainWindow::ToggleToolBarGrips()");

    Configuration.GripsVisible = !Configuration.GripsVisible;

    LockWindowUpdate(HWindow);

    // menu
    int index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_MENU, 0);
    SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
    InsertMenuBand();

    // top toolbar
    index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_TOPTOOLBAR, 0);
    if (index != -1)
    {
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        InsertTopToolbarBand();
    }

    // plugin bar
    index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_PLUGINSBAR, 0);
    if (index != -1)
    {
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        InsertPluginsBarBand();
    }

    // extension bar
    index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_EXTENSIONBAR, 0);
    if (index != -1)
    {
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        InsertExtensionBarBand();
    }

    // user menu bar
    index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_UMTOOLBAR, 0);
    if (index != -1)
    {
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        InsertUMToolbarBand();
    }

    // hot path bar
    index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_HPTOOLBAR, 0);
    if (index != -1)
    {
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        InsertHPToolbarBand();
    }

    // drive bar
    // only if there is one bar; otherwise there are no grips
    if (DriveBar->HWindow != NULL && DriveBar2->HWindow == NULL)
    {
        index = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_DRIVEBAR, 0);
        SendMessage(HTopRebar, RB_DELETEBAND, index, 0);
        InsertDriveBarBand(FALSE);
    }

    LockWindowUpdate(NULL);

    if (DetachedPanels)
    {
        DestroyDetachedChrome();
        EnsureDetachedChrome();
        LayoutDetachedPanels();
    }
}

void CMainWindow::StoreBandsPos()
{
    CALL_STACK_MESSAGE1("CMainWindow::StoreBandsPos()");
    // save the layout in the rebar
    REBARBANDINFO rbbi;

    rbbi.cbSize = sizeof(rbbi);
    rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
    Configuration.MenuIndex = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_MENU, 0);
    SendMessage(HTopRebar, RB_GETBANDINFO,
                Configuration.MenuIndex, (LPARAM)&rbbi);
    Configuration.MenuBreak = (rbbi.fStyle & RBBS_BREAK) != 0;
    Configuration.MenuWidth = rbbi.cx;

    if (TopToolBar->HWindow != NULL)
    {
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
        Configuration.TopToolbarIndex = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_TOPTOOLBAR, 0);
        SendMessage(HTopRebar, RB_GETBANDINFO,
                    Configuration.TopToolbarIndex, (LPARAM)&rbbi);
        Configuration.TopToolbarBreak = (rbbi.fStyle & RBBS_BREAK) != 0;
        Configuration.TopToolbarWidth = rbbi.cx;
    }
    if (PluginsBar->HWindow != NULL)
    {
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
        Configuration.PluginsBarIndex = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_PLUGINSBAR, 0);
        SendMessage(HTopRebar, RB_GETBANDINFO,
                    Configuration.PluginsBarIndex, (LPARAM)&rbbi);
        Configuration.PluginsBarBreak = (rbbi.fStyle & RBBS_BREAK) != 0;
        Configuration.PluginsBarWidth = rbbi.cx;
    }
    if (ExtensionBar->HWindow != NULL)
    {
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
        Configuration.ExtensionBarIndex = (int)SendMessage(
            HTopRebar, RB_IDTOINDEX, BANDID_EXTENSIONBAR, 0);
        SendMessage(HTopRebar, RB_GETBANDINFO,
                    Configuration.ExtensionBarIndex, (LPARAM)&rbbi);
        Configuration.ExtensionBarBreak =
            (rbbi.fStyle & RBBS_BREAK) != 0;
        Configuration.ExtensionBarWidth = rbbi.cx;
    }
    if (UMToolBar->HWindow != NULL)
    {
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
        Configuration.UserMenuToolbarIndex = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_UMTOOLBAR, 0);
        SendMessage(HTopRebar, RB_GETBANDINFO,
                    Configuration.UserMenuToolbarIndex, (LPARAM)&rbbi);
        Configuration.UserMenuToolbarBreak = (rbbi.fStyle & RBBS_BREAK) != 0;
        Configuration.UserMenuToolbarWidth = rbbi.cx;
    }
    if (HPToolBar->HWindow != NULL)
    {
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
        Configuration.HotPathsBarIndex = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_HPTOOLBAR, 0);
        SendMessage(HTopRebar, RB_GETBANDINFO,
                    Configuration.HotPathsBarIndex, (LPARAM)&rbbi);
        Configuration.HotPathsBarBreak = (rbbi.fStyle & RBBS_BREAK) != 0;
        Configuration.HotPathsBarWidth = rbbi.cx;
    }
    if (DriveBar->HWindow != NULL && DriveBar2->HWindow == NULL)
    {
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE;
        Configuration.DriveBarIndex = (int)SendMessage(HTopRebar, RB_IDTOINDEX, BANDID_DRIVEBAR, 0);
        SendMessage(HTopRebar, RB_GETBANDINFO,
                    Configuration.DriveBarIndex, (LPARAM)&rbbi);
        Configuration.DriveBarBreak = (rbbi.fStyle & RBBS_BREAK) != 0;
        Configuration.DriveBarWidth = rbbi.cx;
    }
}

BOOL CMainWindow::InsertMenuBand()
{
    CALL_STACK_MESSAGE1("CMainWindow::InsertMenuBand()");

    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE |
                 RBBIM_ID;
    rbbi.cxMinChild = 10;
    rbbi.cyMinChild = MenuBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.MenuWidth;
    if (Configuration.MenuBreak)
        rbbi.fStyle |= RBBS_BREAK;
    if (Configuration.GripsVisible)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        // so we are not close to the edge
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }
    rbbi.hwndChild = MenuBar->HWindow;
    rbbi.wID = BANDID_MENU;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);
    if (count >= 2 && DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        count -= 2;
    if (Configuration.MenuIndex > count)
        Configuration.MenuIndex = count;

    SendMessage(HTopRebar, RB_INSERTBAND,
                (WPARAM)Configuration.MenuIndex, (LPARAM)&rbbi);

    return TRUE;
}

BOOL CMainWindow::CreateAndInsertWorkerBand()
{
    /*
  CALL_STACK_MESSAGE1("CMainWindow::CreateAndInsertWorkerBand()");

  REBARBANDINFO  rbbi;
  ZeroMemory(&rbbi, sizeof(rbbi));

  rbbi.cbSize       = sizeof(REBARBANDINFO);
  rbbi.fMask        = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_ID;
  SIZE sz;
  AnimateBar->GetFrameSize(&sz);
  rbbi.cxMinChild   = sz.cx + 10;
  rbbi.cyMinChild   = sz.cy;
  rbbi.fStyle = RBBS_FIXEDSIZE | RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;

  AnimateBar->Create(CWINDOW_CLASSNAME2, "",
                     WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                     0, 0, 0, 0,
                     HTopRebar,
                     NULL,
                     HInstance,
                     AnimateBar);

  rbbi.hwndChild    = AnimateBar->HWindow;
  rbbi.wID          = BANDID_WORKER;

//  SendMessage(HTopRebar, RB_INSERTBAND, (WPARAM)1, (LPARAM)&rbbi);
*/
    return TRUE;
}

BOOL CMainWindow::InsertTopToolbarBand()
{
    CALL_STACK_MESSAGE1("CMainWindow::InsertTopToolbarBand()");
    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE |
                 RBBIM_ID;
    rbbi.cxMinChild = 10;
    rbbi.cyMinChild = TopToolBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.TopToolbarWidth;
    if (Configuration.TopToolbarBreak)
        rbbi.fStyle |= RBBS_BREAK;
    if (Configuration.GripsVisible)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        // so we are not close to the edge
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }
    rbbi.hwndChild = TopToolBar->HWindow;
    rbbi.wID = BANDID_TOPTOOLBAR;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);
    if (count >= 2 && DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        count -= 2;
    if (Configuration.TopToolbarIndex > count)
        Configuration.TopToolbarIndex = count;
    SendMessage(HTopRebar, RB_INSERTBAND,
                (WPARAM)Configuration.TopToolbarIndex, (LPARAM)&rbbi);
    return TRUE;
}

BOOL CMainWindow::InsertPluginsBarBand()
{
    CALL_STACK_MESSAGE1("CMainWindow::InsertPluginsBarBand()");
    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE |
                 RBBIM_ID;
    rbbi.cxMinChild = 10;
    rbbi.cyMinChild = PluginsBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.PluginsBarWidth;
    if (Configuration.PluginsBarBreak)
        rbbi.fStyle |= RBBS_BREAK;
    if (Configuration.GripsVisible)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        // so we are not close to the edge
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }
    rbbi.hwndChild = PluginsBar->HWindow;
    rbbi.wID = BANDID_PLUGINSBAR;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);
    if (count >= 2 && DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        count -= 2;
    if (Configuration.PluginsBarIndex > count)
        Configuration.PluginsBarIndex = count;
    SendMessage(HTopRebar, RB_INSERTBAND,
                (WPARAM)Configuration.PluginsBarIndex, (LPARAM)&rbbi);
    return TRUE;
}

BOOL CMainWindow::InsertExtensionBarBand()
{
    CALL_STACK_MESSAGE1("CMainWindow::InsertExtensionBarBand()");
    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE |
                 RBBIM_STYLE | RBBIM_ID;
    rbbi.cxMinChild = 10;
    rbbi.cyMinChild = ExtensionBar->GetNeededHeight() +
                      (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.ExtensionBarWidth;
    if (Configuration.ExtensionBarBreak)
        rbbi.fStyle |= RBBS_BREAK;
    if (Configuration.GripsVisible)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }
    rbbi.hwndChild = ExtensionBar->HWindow;
    rbbi.wID = BANDID_EXTENSIONBAR;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);
    if (count >= 2 && DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        count -= 2;
    if (Configuration.ExtensionBarIndex > count)
        Configuration.ExtensionBarIndex = count;
    SendMessage(HTopRebar, RB_INSERTBAND,
                (WPARAM)Configuration.ExtensionBarIndex, (LPARAM)&rbbi);
    return TRUE;
}

BOOL CMainWindow::InsertUMToolbarBand()
{
    CALL_STACK_MESSAGE1("CMainWindow::InsertUMToolbarBand()");
    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE |
                 RBBIM_ID;
    rbbi.cxMinChild = 10;
    rbbi.cyMinChild = UMToolBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.UserMenuToolbarWidth;
    if (Configuration.UserMenuToolbarBreak)
        rbbi.fStyle |= RBBS_BREAK;
    if (Configuration.GripsVisible)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        // so we are not close to the edge
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }
    rbbi.hwndChild = UMToolBar->HWindow;
    rbbi.wID = BANDID_UMTOOLBAR;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);
    if (count >= 2 && DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        count -= 2;
    if (Configuration.UserMenuToolbarIndex > count)
        Configuration.UserMenuToolbarIndex = count;

    SendMessage(HTopRebar, RB_INSERTBAND,
                (WPARAM)Configuration.UserMenuToolbarIndex, (LPARAM)&rbbi);
    return TRUE;
}

BOOL CMainWindow::InsertHPToolbarBand()
{
    CALL_STACK_MESSAGE1("CMainWindow::InsertHPToolbarBand()");
    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE |
                 RBBIM_ID;
    rbbi.cxMinChild = 10;
    rbbi.cyMinChild = HPToolBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.HotPathsBarWidth;
    if (Configuration.HotPathsBarBreak)
        rbbi.fStyle |= RBBS_BREAK;
    if (Configuration.GripsVisible)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        // so we are not close to the edge
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }
    rbbi.hwndChild = HPToolBar->HWindow;
    rbbi.wID = BANDID_HPTOOLBAR;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);
    if (count >= 2 && DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        count -= 2;
    if (Configuration.HotPathsBarIndex > count)
        Configuration.HotPathsBarIndex = count;

    SendMessage(HTopRebar, RB_INSERTBAND,
                (WPARAM)Configuration.HotPathsBarIndex, (LPARAM)&rbbi);
    return TRUE;
}

BOOL CMainWindow::InsertDriveBarBand(BOOL twoDriveBars)
{
    CALL_STACK_MESSAGE2("CMainWindow::InsertDriveBarBand(%d)", twoDriveBars);
    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE |
                 RBBIM_ID;
    rbbi.cyMinChild = DriveBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = Configuration.DriveBarWidth;
    if (twoDriveBars)
    {
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.fStyle = RBBS_NOGRIPPER | RBBS_BREAK;
        rbbi.cxHeader = 0; // note: this value is also set elsewhere
        rbbi.cxMinChild = 0;
    }
    else
    {
        if (Configuration.DriveBarBreak)
            rbbi.fStyle |= RBBS_BREAK;
        if (Configuration.GripsVisible)
            rbbi.fStyle |= RBBS_GRIPPERALWAYS;
        else
            rbbi.fStyle |= RBBS_NOGRIPPER;
        rbbi.cxMinChild = 10;
    }
    rbbi.hwndChild = DriveBar->HWindow;
    rbbi.wID = BANDID_DRIVEBAR;

    int count = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0);

    if (twoDriveBars)
    {
        SendMessage(HTopRebar, RB_INSERTBAND, (WPARAM)count, (LPARAM)&rbbi);
        rbbi.wID = BANDID_DRIVEBAR2;
        rbbi.fStyle = RBBS_NOGRIPPER;
        rbbi.hwndChild = DriveBar2->HWindow;
        SendMessage(HTopRebar, RB_INSERTBAND, (WPARAM)count + 1, (LPARAM)&rbbi);
    }
    else
    {
        if (Configuration.DriveBarIndex > count)
            Configuration.DriveBarIndex = count;
        SendMessage(HTopRebar, RB_INSERTBAND,
                    (WPARAM)Configuration.DriveBarIndex, (LPARAM)&rbbi);
    }
    return TRUE;
}

void CMainWindow::FocusLeftPanel()
{
    FocusPanel(LeftPanel);
    LeftPanel->SetCaretIndex(0, FALSE);
}

BOOL CMainWindow::EditWindowKnowHWND(HWND hwnd)
{
    return EditWindow != NULL && EditWindow->KnowHWND(hwnd);
}

void CMainWindow::EditWindowSetDirectory()
{
    SetWindowTitle(); // current directory into the title bar
    CFilesWindow* panel = DetachedPanels ? LeftPanel : GetActivePanel();
    if (panel != NULL &&
        (panel->Is(ptDisk) ||
         panel->Is(ptPluginFS) && panel->GetPluginFS()->NotEmpty() &&
             panel->GetPluginFS()->IsServiceSupported(FS_SERVICE_COMMANDLINE)))
    {
        CPathBuffer dir(2 * SAL_MAX_PATH);
        panel->GetGeneralPath(dir.Data(), dir.Capacity());
        EditWindow->Enable(TRUE); // cached in EditWindow
        EditWindow->SetDirectory(dir.Data());
    }
    else // disable/hide edit-line
    {
        if (EditMode && panel != NULL) // release focus from command line before disabling it
            FocusPanel(panel, TRUE);
        EditWindow->Enable(FALSE); // cached in EditWindow
        EditWindow->SetDirectory("");
    }
}

HWND CMainWindow::GetEditLineHWND(BOOL disableSkip)
{
    if (EditWindow == NULL || EditWindow->EditLine == NULL)
        return NULL;
    if (disableSkip)
        EditWindow->EditLine->SkipCharacter = FALSE;
    return EditWindow->EditLine->HWindow;
}

HWND CMainWindow::GetActivePanelHWND()
{
    return GetActivePanel()->HWindow;
}

int CMainWindow::GetDirectoryLineHeight()
{
    if (GetActivePanel()->DirectoryLine != NULL &&
        GetActivePanel()->DirectoryLine->HWindow != NULL)
    {
        RECT r;
        GetClientRect(GetActivePanel()->DirectoryLine->HWindow, &r);
        return r.bottom - r.top;
    }
    return 0;
}

void CMainWindow::RefreshDiskFreeSpace()
{
    LeftPanel->RefreshDiskFreeSpace(TRUE, TRUE);
    RightPanel->RefreshDiskFreeSpace(TRUE, TRUE);
}

void CMainWindow::RefreshDirs()
{
    LeftPanel->ChangePathToDisk(LeftPanel->HWindow, LeftPanel->GetPath());
    RightPanel->ChangePathToDisk(RightPanel->HWindow, RightPanel->GetPath());
}

// for passing the path to the configuration dialog
char HotPathSetBufferName[MAX_PATH];
char HotPathSetBufferPath[HOTPATHITEM_MAXPATH];

void CMainWindow::SetUnescapedHotPath(int index, const char* path)
{
    if (Configuration.HotPathAutoConfig)
    {
        // switch to the buffer so that Cancel works
        lstrcpyn(HotPathSetBufferName, path, MAX_PATH);
        lstrcpyn(HotPathSetBufferPath, path, HOTPATHITEM_MAXPATH);
        DuplicateDollars(HotPathSetBufferPath, HOTPATHITEM_MAXPATH);
        // open the HotPaths page and edit item index
        PostMessage(HWindow, WM_USER_CONFIGURATION, 1, index);
    }
    else
    {
        // push the value directly
        char buff[HOTPATHITEM_MAXPATH];
        lstrcpyn(buff, path, HOTPATHITEM_MAXPATH);
        char nameBuff[MAX_PATH];
        lstrcpyn(nameBuff, path, MAX_PATH);
        DuplicateDollars(buff, HOTPATHITEM_MAXPATH);
        HotPaths.Set(index, nameBuff, buff);
        // a change occurred, rebuild the Hot Path Bar
        if (HPToolBar != NULL && HPToolBar->HWindow != NULL)
            HPToolBar->CreateButtons();
        if (Windows7AndLater)
            CreateJumpList();
    }
}

BOOL CMainWindow::GetExpandedHotPath(HWND hParent, int index, char* buffer, int bufferSize)
{
    // The expanded path can be up to the Win32 long-path limit.
    if (bufferSize != SAL_MAX_PATH)
        TRACE_E("CMainWindow::GetExpandedHotPath: invalid buffer size!");

    // if the path is not defined, we can exit immediately
    int pathLen = HotPaths.GetPathLen(index);
    if (pathLen == 0)
        return FALSE;

    // extract the path for us
    char* path = (char*)malloc(pathLen + 1);
    HotPaths.GetPath(index, path, pathLen + 1);

    // perform validation
    int errorPos1, errorPos2;
    if (!ValidateHotPath(hParent, path, errorPos1, errorPos2))
    {
        free(path);
        return FALSE;
    }

    // finally perform the expansion
    BOOL ret = ExpandHotPath(hParent, path, buffer, bufferSize, FALSE);
    free(path);
    return ret;
}

int CMainWindow::GetUnassignedHotPathIndex()
{
    return HotPaths.GetUnassignedHotPathIndex();
}


static void ScaleSmallLogFontForCurrentDPI(LOGFONT* lf)
{
    int dpi = GetSystemDPI();
    if (lf == NULL || lf->lfHeight == 0)
        return;

    int height = abs(lf->lfHeight);
    int expected = MulDiv(12, dpi, 96);
    if (height < expected - 1 || height > expected + 2)
        lf->lfHeight = lf->lfHeight < 0 ? -expected : expected;
}

static BOOL SystemParametersInfoForCurrentDPI(UINT action, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    typedef BOOL(WINAPI * FSystemParametersInfoForDpi)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
    static FSystemParametersInfoForDpi systemParametersInfoForDpi = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandle("user32.dll");
        if (user32 != NULL)
            systemParametersInfoForDpi = (FSystemParametersInfoForDpi)GetProcAddress(user32, "SystemParametersInfoForDpi");
        loaded = TRUE;
    }
    if (systemParametersInfoForDpi != NULL &&
        systemParametersInfoForDpi(action, uiParam, pvParam, fWinIni, GetSystemDPI()))
        return TRUE;
    return SystemParametersInfo(action, uiParam, pvParam, fWinIni);
}

// font for our GUI (the panel font can be defined in the configuration)
BOOL GetSystemGUIFont(LOGFONT* lf)
{
    if (!SystemParametersInfoForCurrentDPI(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), lf, 0))
    {
        // if SystemParametersInfo fails unexpectedly, use a fallback
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForCurrentDPI(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
        *lf = ncm.lfMessageFont;
        lf->lfWeight = FW_NORMAL;
    }
    ScaleSmallLogFontForCurrentDPI(lf);
    return TRUE;
}

// tooltip font
BOOL GetSystemTooltipFont(LOGFONT* lf)
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoForCurrentDPI(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    *lf = ncm.lfStatusFont;
    ScaleSmallLogFontForCurrentDPI(lf);
    return TRUE;
}

BOOL CreatePanelFont()
{
    CALL_STACK_MESSAGE1("CreatePanelFont()");

    if (Font != NULL)
        HANDLES(DeleteObject(Font));

    LOGFONT lf;
    if (UseCustomPanelFont)
        lf = LogFont; // the user set a custom font
    else
        GetSystemGUIFont(&lf); // get the font from the system

    Font = HANDLES(CreateFontIndirect(&lf));
    if (Font == NULL)
    {
        TRACE_E("Unable to create panel font.");
        return FALSE;
    }

    // create an underlined variant
    BYTE oldUnderline = lf.lfUnderline;
    lf.lfUnderline = TRUE;
    if (FontUL != NULL)
        HANDLES(DeleteObject(FontUL));
    FontUL = HANDLES(CreateFontIndirect(&lf));
    lf.lfUnderline = oldUnderline;
    if (FontUL == NULL)
    {
        TRACE_E("Unable to create underlined panel font.");
        return FALSE;
    }

    HDC dc = HANDLES(GetDC(NULL));
    HFONT oldFont = (HFONT)SelectObject(dc, Font);
    TEXTMETRIC tm;
    GetTextMetrics(dc, &tm);
    FontCharHeight = tm.tmHeight;
    SIZE sz;
    GetTextExtentPoint32(dc, "...", 3, &sz);
    TextEllipsisWidth = sz.cx;
    SelectObject(dc, oldFont);
    HANDLES(ReleaseDC(NULL, dc));
    return TRUE;
}

BOOL CreateEnvFonts()
{
    LOGFONT lf;
    GetSystemGUIFont(&lf);

    if (EnvFont != NULL)
        HANDLES(DeleteObject(EnvFont));
    EnvFont = HANDLES(CreateFontIndirect(&lf));
    if (EnvFont == NULL)
    {
        TRACE_E("Unable to create font.");
        return FALSE;
    }

    LOGFONT boldLF = lf;
    boldLF.lfWeight = FW_BOLD;
    if (EnvFontBold != NULL)
        HANDLES(DeleteObject(EnvFontBold));
    EnvFontBold = HANDLES(CreateFontIndirect(&boldLF));
    if (EnvFontBold == NULL)
    {
        TRACE_E("Unable to create bold font.");
        return FALSE;
    }

    // create an underlined variant
    lf.lfUnderline = TRUE;
    if (EnvFontUL != NULL)
        HANDLES(DeleteObject(EnvFontUL));
    EnvFontUL = HANDLES(CreateFontIndirect(&lf));
    if (EnvFontUL == NULL)
    {
        TRACE_E("Unable to create font.");
        return FALSE;
    }

    HDC dc = HANDLES(GetDC(NULL));
    HFONT oldFont = (HFONT)SelectObject(dc, EnvFont);
    TEXTMETRIC tm;
    GetTextMetrics(dc, &tm);
    EnvFontCharHeight = tm.tmHeight;
    SIZE sz;
    GetTextExtentPoint32(dc, "...", 3, &sz);
    TextEllipsisWidthEnv = sz.cx;
    SelectObject(dc, oldFont);
    HANDLES(ReleaseDC(NULL, dc));

    LOGFONT toolLF;
    GetSystemTooltipFont(&toolLF);
    if (TooltipFont != NULL)
        HANDLES(DeleteObject(TooltipFont));
    TooltipFont = HANDLES(CreateFontIndirect(&toolLF));
    if (TooltipFont == NULL)
    {
        TRACE_E("Unable to create font.");
        return FALSE;
    }

    return TRUE;
}

void CMainWindow::SetFont()
{
    CALL_STACK_MESSAGE1("CMainWindow::SetFont()");

    CreatePanelFont();

    // Keep attached tab strips in the main window's complete DPI refresh
    // instead of relying only on the ordering of PMv2 child notifications. A
    // right tab strip hosted by a detached top-level window has its own DPI
    // lifecycle.
    if (LeftTabWindow != NULL && LeftTabWindow->HWindow != NULL &&
        GetAncestor(LeftTabWindow->HWindow, GA_ROOT) == HWindow)
    {
        LeftTabWindow->RefreshDPIResources();
    }
    if (RightTabWindow != NULL && RightTabWindow->HWindow != NULL &&
        GetAncestor(RightTabWindow->HWindow, GA_ROOT) == HWindow)
    {
        RightTabWindow->RefreshDPIResources();
    }

    // Each panel can live under a different PMv2 top-level window. Keep its
    // drawing fonts and icon metrics tied to that window instead of copying
    // the main window's process-global DPI.
    for (int i = 0; i < LeftPanelTabs.Count; ++i)
    {
        CFilesWindow* panel = LeftPanelTabs[i];
        if (panel != NULL)
        {
            panel->RefreshDPIResources(TRUE);
            panel->SetFont();
            panel->RefreshTreeViewDPI();
        }
    }
    for (int i = 0; i < RightPanelTabs.Count; ++i)
    {
        CFilesWindow* panel = RightPanelTabs[i];
        if (panel != NULL)
        {
            panel->RefreshDPIResources(TRUE);
            panel->SetFont();
            panel->RefreshTreeViewDPI();
        }
    }
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        CFilesWindow* panel = DetachedTabs[i].Panel;
        if (panel != NULL)
        {
            panel->RefreshDPIResources(TRUE);
            panel->SetFont();
            panel->RefreshTreeViewDPI();
        }
    }

    if (IsWindowVisible(HWindow))
    {
        RECT r;
        GetClientRect(HWindow, &r);
        PostMessage(HWindow, WM_SIZE, SIZE_RESTORED,
                    MAKELONG(r.right - r.left, r.bottom - r.top));
        HANDLES(EnterCriticalSection(&TimeCounterSection));
        int t1 = MyTimeCounter++;
        int t2 = MyTimeCounter++;
        HANDLES(LeaveCriticalSection(&TimeCounterSection));
        if (LeftPanel != NULL)
            PostMessage(LeftPanel->HWindow, WM_USER_REFRESH_DIR, 0, t1);
        if (RightPanel != NULL)
            PostMessage(RightPanel->HWindow, WM_USER_REFRESH_DIR, 0, t2);
        InvalidateRect(HWindow, NULL, FALSE);
    }
}

void CMainWindow::SetEnvFont()
{
    CALL_STACK_MESSAGE1("CMainWindow::SetEnvFont()");

    CreateEnvFonts();

    if (MenuBar != NULL)
        MenuBar->SetFont();
    if (EditWindow != NULL)
        EditWindow->SetFont();
    if (UMToolBar != NULL)
        UMToolBar->SetFont();
    if (HPToolBar != NULL)
        HPToolBar->SetFont();
    if (DriveBar != NULL)
        DriveBar->SetFont();
    if (DriveBar2 != NULL)
        DriveBar2->SetFont();
    if (BottomToolBar != NULL)
        BottomToolBar->SetFont();

    if (HTopRebar != NULL)
    {
        REBARBANDINFO rbbi;
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_CHILDSIZE;
        rbbi.cxMinChild = 10;
        rbbi.cxMinChild = 10;
        if (MenuBar != NULL)
        {
            rbbi.cyMinChild = MenuBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
            SendMessage(HTopRebar, RB_SETBANDINFO, (WPARAM)Configuration.MenuIndex, (LPARAM)&rbbi);
        }
        if (DriveBar != NULL && DriveBar->HWindow != NULL)
        {
            rbbi.cyMinChild = DriveBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
            int index;
            if (DriveBar2 != NULL && DriveBar2->HWindow != NULL)
                index = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0) - 2;
            else
                index = Configuration.DriveBarIndex;
            SendMessage(HTopRebar, RB_SETBANDINFO, (WPARAM)index, (LPARAM)&rbbi);
        }
        if (DriveBar2 != NULL && DriveBar2->HWindow != NULL)
        {
            rbbi.cyMinChild = DriveBar2->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
            int index = (int)SendMessage(HTopRebar, RB_GETBANDCOUNT, 0, 0) - 1;
            SendMessage(HTopRebar, RB_SETBANDINFO, (WPARAM)index, (LPARAM)&rbbi);
        }
        if (UMToolBar != NULL && UMToolBar->HWindow != NULL)
        {
            rbbi.cyMinChild = UMToolBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
            SendMessage(HTopRebar, RB_SETBANDINFO, (WPARAM)Configuration.UserMenuToolbarIndex, (LPARAM)&rbbi);
        }
        if (HPToolBar != NULL && HPToolBar->HWindow != NULL)
        {
            rbbi.cyMinChild = HPToolBar->GetNeededHeight() + (DarkModeShouldUseDarkColors() ? 2 : 0);
            SendMessage(HTopRebar, RB_SETBANDINFO, (WPARAM)Configuration.HotPathsBarIndex, (LPARAM)&rbbi);
        }
    }

    if (LeftPanel != NULL)
        LeftPanel->SetFont();
    if (RightPanel != NULL)
        RightPanel->SetFont();

    if (IsWindowVisible(HWindow))
    {
        RECT r;
        GetClientRect(HWindow, &r);
        PostMessage(HWindow, WM_SIZE, SIZE_RESTORED,
                    MAKELONG(r.right - r.left, r.bottom - r.top));
        HANDLES(EnterCriticalSection(&TimeCounterSection));
        int t1 = MyTimeCounter++;
        int t2 = MyTimeCounter++;
        HANDLES(LeaveCriticalSection(&TimeCounterSection));
        if (LeftPanel != NULL)
        {
            LeftPanel->SleepIconCacheThread();
            LeftPanel->IconCache->Release();
            LeftPanel->EndOfIconReadingTime = GetTickCount() - 10000;
            LeftPanel->UseThumbnails = FALSE;
            PostMessage(LeftPanel->HWindow, WM_USER_REFRESH_DIR, 0, t1);
        }
        if (RightPanel != NULL)
        {
            RightPanel->SleepIconCacheThread();
            RightPanel->IconCache->Release();
            RightPanel->EndOfIconReadingTime = GetTickCount() - 10000;
            RightPanel->UseThumbnails = FALSE;
            PostMessage(RightPanel->HWindow, WM_USER_REFRESH_DIR, 0, t2);
        }
        InvalidateRect(HWindow, NULL, FALSE);
    }
}

void CMainWindow::FillUserMenu2(CMenuPopup* menu, int* iterator, int max)
{
    MENU_ITEM_INFO mii;
    int added = 0;
    for (; *iterator < max; (*iterator)++)
    {
        switch (UserMenuItems->At(*iterator)->Type)
        {
        case umitSeparator:
        {
            mii.Mask = MENU_MASK_TYPE;
            mii.Type = MENU_TYPE_SEPARATOR;
            menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
            break;
        }

        case umitItem:
        {
            mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STATE | MENU_MASK_STRING | MENU_MASK_ICON;
            mii.State = 0;
            mii.Type = MENU_TYPE_STRING;
            mii.ID = CM_USERMENU_MIN + *iterator;
            mii.String = UserMenuItems->At(*iterator)->ItemName;
            mii.HIcon = UserMenuItems->At(*iterator)->UMIcon;
            menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
            added++;
            break;
        }

        case umitSubmenuBegin:
        {
            CMenuPopup* popup = new CMenuPopup();
            if (popup == NULL)
            {
                TRACE_E(LOW_MEMORY);
                return;
            }
            mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ICON | MENU_MASK_SUBMENU;
            mii.Type = MENU_TYPE_STRING;
            mii.SubMenu = popup;
            mii.String = UserMenuItems->At(*iterator)->ItemName;
            mii.HIcon = UserMenuItems->At(*iterator)->UMIcon;
            menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
            // recursion
            (*iterator)++;
            FillUserMenu2(popup, iterator, max);
            added++;
            break;
        }

        case umitSubmenuEnd:
        {
            goto ESCAPE;
        }
        }
    }
ESCAPE:
    if (added == 0)
    {
        mii.Mask = MENU_MASK_TYPE | MENU_MASK_STATE | MENU_MASK_STRING;
        mii.Type = MENU_TYPE_STRING;
        mii.State = MENU_STATE_GRAYED;
        mii.String = LoadStr(IDS_EMPTYUSERMENU);
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
    }
}

void CMainWindow::FillUserMenu(CMenuPopup* menu, BOOL customize)
{
    int max = min(CM_USERMENU_MAX - CM_USERMENU_MIN, UserMenuItems->Count);

    int iterator = 0;
    FillUserMenu2(menu, &iterator, max);

    if (customize)
    {
        // add a separator and the configuration option
        MENU_ITEM_INFO mii;
        mii.Mask = MENU_MASK_TYPE;
        mii.Type = MENU_TYPE_SEPARATOR;
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
        mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STATE | MENU_MASK_STRING | MENU_MASK_ICON;
        mii.Type = MENU_TYPE_STRING;
        mii.State = 0;
        mii.ID = CM_CUSTOMIZE_USERMENU;
        mii.String = LoadStr(IDS_CUSTOMIZE_HOTPATHS);
        mii.HIcon = NULL;
        menu->InsertItem(0xFFFFFFFF, TRUE, &mii);
    }
}

/*
void
CMainWindow::ReleaseMenuNew()
{
  CALL_STACK_MESSAGE1("CMainWindow::ReleaseMenuNew()");
  if (ContextMenuNew != NULL) ContextMenuNew->Release();

  MENUITEMINFO mi;
  mi.cbSize = sizeof(mi);
  mi.fMask = MIIM_STATE | MIIM_SUBMENU;
  mi.fState = MFS_GRAYED;
  mi.hSubMenu = NULL;
  SetMenuItemInfo(GetSubMenu(Menu->HMenu, FILES_MENU_INDEX), CM_NEWMENU, FALSE, &mi);
}
*/

void CMainWindow::LayoutMainWindow()
{
    RECT r;
    GetClientRect(HWindow, &r);
    SendMessage(HWindow, WM_SIZE, SIZE_RESTORED,
                MAKELONG(r.right - r.left, r.bottom - r.top));
}

void CMainWindow::LayoutWindows()
{
    // Moving/resizing a rebar can synchronously send RBN_AUTOSIZE back to the
    // main window. That notification calls LayoutWindows() as well. During a
    // DPI transition this used to recurse through WM_SIZE and CEditWindow's
    // child layout until the process exhausted its stack.
    if (LayoutWindowsInProgress)
        return;

    LayoutWindowsInProgress = TRUE;
    LayoutMainWindow();
    LayoutDetachedPanels();
    LayoutWindowsInProgress = FALSE;
}

HWND CMainWindow::GetDetachedPanelWindow(CPanelSide side)
{
    return side == cpsLeft ? HLeftDetachedWindow : HRightDetachedWindow;
}


static BOOL InsertDetachedBand(HWND rebar, HWND child, int bandID, int index, int width, int height,
                               BOOL breakBand, BOOL fixedSize = FALSE)
{
    if (rebar == NULL || child == NULL)
        return FALSE;

    REBARBANDINFO rbbi;
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_ID;
    rbbi.cxMinChild = fixedSize ? width : 10;
    rbbi.cyMinChild = height + (DarkModeShouldUseDarkColors() ? 2 : 0);
    rbbi.cx = width;
    rbbi.hwndChild = child;
    rbbi.wID = bandID;
    if (breakBand)
        rbbi.fStyle |= RBBS_BREAK;
    if (fixedSize)
        rbbi.fStyle |= RBBS_FIXEDSIZE;
    if (Configuration.GripsVisible && !fixedSize)
        rbbi.fStyle |= RBBS_GRIPPERALWAYS;
    else
    {
        rbbi.fStyle |= RBBS_NOGRIPPER;
        rbbi.fMask |= RBBIM_HEADERSIZE;
        rbbi.cxHeader = 2;
    }

    int count = (int)SendMessage(rebar, RB_GETBANDCOUNT, 0, 0);
    if (index < 0 || index > count)
        index = count;
    return SendMessage(rebar, RB_INSERTBAND, (WPARAM)index, (LPARAM)&rbbi) != 0;
}

BOOL CMainWindow::RebuildDetachedToolbarImageLists(int dpi)
{
    if (dpi <= 0)
        dpi = USER_DEFAULT_SCREEN_DPI;
    if (DetachedWindowDPI == dpi &&
        HDetachedGrayToolBarImageList != NULL && HDetachedHotToolBarImageList != NULL &&
        HDetachedBottomTBImageList != NULL && HDetachedHotBottomTBImageList != NULL)
    {
        return TRUE;
    }

    COLORREF toolbarFace = Configuration.UseWindowsDarkMode && DarkModeShouldUseDarkColors()
                               ? RGB(32, 32, 32)
                               : GetSysColor(COLOR_BTNFACE);
    HBITMAP mask = NULL;
    HBITMAP gray = NULL;
    HBITMAP color = NULL;
    CSVGIcon* svgIcons = NULL;
    int svgIconsCount = 0;
    GetSVGIconsMainToolbar(&svgIcons, &svgIconsCount);
    if (!CreateToolbarBitmaps(HInstance,
                              Use256ColorsBitmap() ? IDB_TOOLBAR_256 : IDB_TOOLBAR_16,
                              RGB(255, 0, 255), toolbarFace,
                              mask, gray, color, TRUE, svgIcons, svgIconsCount, dpi))
    {
        return FALSE;
    }

    int iconSize = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
    HIMAGELIST newHot = ImageList_Create(iconSize, iconSize, ILC_MASK | ILC_COLORDDB,
                                         IDX_TB_COUNT + 1, 1);
    HIMAGELIST newGray = ImageList_Create(iconSize, iconSize, ILC_MASK | ILC_COLORDDB,
                                          IDX_TB_COUNT + 1, 1);
    if (newHot != NULL)
        ImageList_Add(newHot, color, mask);
    if (newGray != NULL)
        ImageList_Add(newGray, gray, mask);
    HANDLES(DeleteObject(mask));
    HANDLES(DeleteObject(gray));
    HANDLES(DeleteObject(color));

    if (newHot == NULL || newGray == NULL)
    {
        if (newHot != NULL)
            ImageList_Destroy(newHot);
        if (newGray != NULL)
            ImageList_Destroy(newGray);
        return FALSE;
    }

    HIMAGELIST newBottom = NULL;
    HIMAGELIST newHotBottom = NULL;
    if (!CreateBottomToolbarImageLists(
            dpi, &newBottom, &newHotBottom))
    {
        ImageList_Destroy(newHot);
        ImageList_Destroy(newGray);
        return FALSE;
    }

    ImageList_SetBkColor(newHot, toolbarFace);
    ImageList_SetBkColor(newGray, toolbarFace);

    // Keep the process-global lock index valid in the private list as well.
    if (ToolBarLockImageIndex >= 0 && LockFrames != NULL)
    {
        HICON lock = LockFrames->GetIcon(0);
        if (lock != NULL)
        {
            while (ImageList_GetImageCount(newHot) <= ToolBarLockImageIndex)
                ImageList_SetImageCount(newHot, ImageList_GetImageCount(newHot) + 1);
            while (ImageList_GetImageCount(newGray) <= ToolBarLockImageIndex)
                ImageList_SetImageCount(newGray, ImageList_GetImageCount(newGray) + 1);
            ImageList_ReplaceIcon(newHot, ToolBarLockImageIndex, lock);
            ImageList_ReplaceIcon(newGray, ToolBarLockImageIndex, lock);
            DestroyIcon(lock);
        }
    }

    if (HDetachedHotToolBarImageList != NULL)
        ImageList_Destroy(HDetachedHotToolBarImageList);
    if (HDetachedGrayToolBarImageList != NULL)
        ImageList_Destroy(HDetachedGrayToolBarImageList);
    if (HDetachedBottomTBImageList != NULL)
        ImageList_Destroy(HDetachedBottomTBImageList);
    if (HDetachedHotBottomTBImageList != NULL)
        ImageList_Destroy(HDetachedHotBottomTBImageList);
    HDetachedHotToolBarImageList = newHot;
    HDetachedGrayToolBarImageList = newGray;
    HDetachedBottomTBImageList = newBottom;
    HDetachedHotBottomTBImageList = newHotBottom;
    DetachedWindowDPI = dpi;
    return TRUE;
}

BOOL CMainWindow::RebuildDetachedTabToolbarImageLists(int dpi, HWND hWnd)
{
    CDetachedTabInfo* info = FindDetachedTab(hWnd != NULL ? hWnd : HDetachedTabWindow);
    if (info == NULL)
        return FALSE;
    if (dpi <= 0)
        dpi = USER_DEFAULT_SCREEN_DPI;
    if (info->WindowDPI == dpi &&
        info->HGrayToolBarImageList != NULL && info->HHotToolBarImageList != NULL)
    {
        return TRUE;
    }

    COLORREF toolbarFace = Configuration.UseWindowsDarkMode && DarkModeShouldUseDarkColors()
                               ? RGB(32, 32, 32)
                               : GetSysColor(COLOR_BTNFACE);
    HBITMAP mask = NULL;
    HBITMAP gray = NULL;
    HBITMAP color = NULL;
    CSVGIcon* svgIcons = NULL;
    int svgIconsCount = 0;
    GetSVGIconsMainToolbar(&svgIcons, &svgIconsCount);
    if (!CreateToolbarBitmaps(HInstance,
                              Use256ColorsBitmap() ? IDB_TOOLBAR_256 : IDB_TOOLBAR_16,
                              RGB(255, 0, 255), toolbarFace,
                              mask, gray, color, TRUE, svgIcons, svgIconsCount, dpi))
    {
        return FALSE;
    }

    int iconSize = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);
    HIMAGELIST newHot = ImageList_Create(iconSize, iconSize, ILC_MASK | ILC_COLORDDB,
                                         IDX_TB_COUNT + 1, 1);
    HIMAGELIST newGray = ImageList_Create(iconSize, iconSize, ILC_MASK | ILC_COLORDDB,
                                          IDX_TB_COUNT + 1, 1);
    if (newHot != NULL)
        ImageList_Add(newHot, color, mask);
    if (newGray != NULL)
        ImageList_Add(newGray, gray, mask);
    HANDLES(DeleteObject(mask));
    HANDLES(DeleteObject(gray));
    HANDLES(DeleteObject(color));

    if (newHot == NULL || newGray == NULL)
    {
        if (newHot != NULL)
            ImageList_Destroy(newHot);
        if (newGray != NULL)
            ImageList_Destroy(newGray);
        return FALSE;
    }

    ImageList_SetBkColor(newHot, toolbarFace);
    ImageList_SetBkColor(newGray, toolbarFace);
    if (ToolBarLockImageIndex >= 0 && LockFrames != NULL)
    {
        HICON lock = LockFrames->GetIcon(0);
        if (lock != NULL)
        {
            while (ImageList_GetImageCount(newHot) <= ToolBarLockImageIndex)
                ImageList_SetImageCount(newHot, ImageList_GetImageCount(newHot) + 1);
            while (ImageList_GetImageCount(newGray) <= ToolBarLockImageIndex)
                ImageList_SetImageCount(newGray, ImageList_GetImageCount(newGray) + 1);
            ImageList_ReplaceIcon(newHot, ToolBarLockImageIndex, lock);
            ImageList_ReplaceIcon(newGray, ToolBarLockImageIndex, lock);
            DestroyIcon(lock);
        }
    }

    if (info->HHotToolBarImageList != NULL)
        ImageList_Destroy(info->HHotToolBarImageList);
    if (info->HGrayToolBarImageList != NULL)
        ImageList_Destroy(info->HGrayToolBarImageList);
    info->HHotToolBarImageList = newHot;
    info->HGrayToolBarImageList = newGray;
    info->WindowDPI = dpi;
    if (info->HWindow == HDetachedTabWindow)
    {
        HDetachedTabHotToolBarImageList = newHot;
        HDetachedTabGrayToolBarImageList = newGray;
        DetachedTabWindowDPI = dpi;
    }
    return TRUE;
}

HIMAGELIST CMainWindow::GetToolbarImageListForWindow(HWND child, BOOL hot) const
{
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        const CDetachedTabInfo& info = DetachedTabs[i];
        if (info.HWindow != NULL &&
            (child == info.HWindow || (child != NULL && IsChild(info.HWindow, child))))
        {
            HIMAGELIST imageList = hot ? info.HHotToolBarImageList : info.HGrayToolBarImageList;
            if (imageList != NULL)
                return imageList;
            break;
        }
    }
    BOOL detached = HRightDetachedWindow != NULL &&
                    (child == HRightDetachedWindow ||
                     (child != NULL && IsChild(HRightDetachedWindow, child)));
    if (detached)
    {
        HIMAGELIST imageList = hot ? HDetachedHotToolBarImageList
                                   : HDetachedGrayToolBarImageList;
        if (imageList != NULL)
            return imageList;
    }
    return hot ? HHotToolBarImageList : HGrayToolBarImageList;
}

BOOL CMainWindow::EnsureDetachedChrome()
{
    CALL_STACK_MESSAGE1("CMainWindow::EnsureDetachedChrome()");
    if (HRightDetachedWindow == NULL)
        return FALSE;

#define DETACHED_CHROME_FAIL() \
    do                         \
    {                          \
        CreatingDetachedChrome = FALSE; \
        return FALSE;          \
    } while (0)

    CreatingDetachedChrome = TRUE;

    int detachedDPI = (int)WinLibDPIGetWindowDPI(HRightDetachedWindow);
    RebuildDetachedToolbarImageLists(detachedDPI);

    if (HDetachedTopRebar == NULL)
    {
        DWORD rebarStyle = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                           RBS_VARHEIGHT | CCS_NODIVIDER | CCS_NOPARENTALIGN | RBS_AUTOSIZE;
        if (!DarkModeShouldUseDarkColors())
            rebarStyle |= WS_BORDER | RBS_BANDBORDERS;
        HDetachedTopRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, "",
                                           rebarStyle, 0, 0, 0, 0,
                                           HRightDetachedWindow, (HMENU)0, HInstance, NULL);
        if (HDetachedTopRebar == NULL)
            DETACHED_CHROME_FAIL();
        DarkModeApplyWindow(HDetachedTopRebar);
        DarkModeApplyRebarSeparators(HDetachedTopRebar);
    }

    if (DetachedMenuBar == NULL)
        DetachedMenuBar = new CMenuBar(&MainMenu, HWindow);
    if (DetachedMenuBar == NULL)
        DETACHED_CHROME_FAIL();
    if (DetachedMenuBar->HWindow == NULL && !DetachedMenuBar->CreateWnd(HDetachedTopRebar))
        DETACHED_CHROME_FAIL();
    DetachedMenuBar->SetFont();
    if ((int)SendMessage(HDetachedTopRebar, RB_IDTOINDEX, BANDID_MENU, 0) == -1)
        InsertDetachedBand(HDetachedTopRebar, DetachedMenuBar->HWindow, BANDID_MENU,
                           Configuration.MenuIndex, Configuration.MenuWidth,
                           DetachedMenuBar->GetNeededHeight(), Configuration.MenuBreak);

    if (TopToolBar != NULL && TopToolBar->HWindow != NULL)
    {
        if (DetachedTopToolBar == NULL)
        {
            DetachedTopToolBar = new CMainToolBar(HWindow, mtbtTop, ooStatic);
            if (DetachedTopToolBar != NULL)
            {
                DetachedTopToolBar->SetImageList(HDetachedGrayToolBarImageList != NULL
                                                     ? HDetachedGrayToolBarImageList
                                                     : HGrayToolBarImageList);
                DetachedTopToolBar->SetHotImageList(HDetachedHotToolBarImageList != NULL
                                                        ? HDetachedHotToolBarImageList
                                                        : HHotToolBarImageList);
                DetachedTopToolBar->SetStyle(TLB_STYLE_IMAGE | TLB_STYLE_ADJUSTABLE);
                TOOLBAR_PADDING padding;
                DetachedTopToolBar->GetPadding(&padding);
                padding.ToolBarVertical = 1;
                padding.IconLeft = 2;
                padding.IconRight = 3;
                DetachedTopToolBar->SetPadding(&padding);
            }
        }
        if (DetachedTopToolBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedTopToolBar->HWindow == NULL)
        {
            if (!DetachedTopToolBar->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedTopToolBar->Load(Configuration.TopToolBar);
            InsertDetachedBand(HDetachedTopRebar, DetachedTopToolBar->HWindow, BANDID_TOPTOOLBAR,
                               Configuration.TopToolbarIndex, Configuration.TopToolbarWidth,
                               DetachedTopToolBar->GetNeededHeight(), Configuration.TopToolbarBreak);
            ShowWindow(DetachedTopToolBar->HWindow, SW_SHOW);
        }
    }

    if (PluginsBar != NULL && PluginsBar->HWindow != NULL)
    {
        if (DetachedPluginsBar == NULL)
            DetachedPluginsBar = new CPluginsBar(HWindow, ooStatic);
        if (DetachedPluginsBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedPluginsBar->HWindow == NULL)
        {
            if (!DetachedPluginsBar->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedPluginsBar->CreatePluginButtons();
            InsertDetachedBand(HDetachedTopRebar, DetachedPluginsBar->HWindow, BANDID_PLUGINSBAR,
                               Configuration.PluginsBarIndex, Configuration.PluginsBarWidth,
                               DetachedPluginsBar->GetNeededHeight(), Configuration.PluginsBarBreak);
            ShowWindow(DetachedPluginsBar->HWindow, SW_SHOW);
        }
    }

    if (ExtensionBar != NULL && ExtensionBar->HWindow != NULL)
    {
        if (DetachedExtensionBar == NULL)
            DetachedExtensionBar = new CExtensionBar(HWindow, ooStatic);
        if (DetachedExtensionBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedExtensionBar->HWindow == NULL)
        {
            if (!DetachedExtensionBar->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedExtensionBar->CreateExtensionButtons(
                HDetachedGrayToolBarImageList, HDetachedHotToolBarImageList);
            InsertDetachedBand(
                HDetachedTopRebar, DetachedExtensionBar->HWindow,
                BANDID_EXTENSIONBAR, Configuration.ExtensionBarIndex,
                Configuration.ExtensionBarWidth,
                DetachedExtensionBar->GetNeededHeight(),
                Configuration.ExtensionBarBreak);
            ShowWindow(DetachedExtensionBar->HWindow, SW_SHOW);
        }
    }

    if (UMToolBar != NULL && UMToolBar->HWindow != NULL)
    {
        if (DetachedUMToolBar == NULL)
            DetachedUMToolBar = new CUserMenuBar(HWindow, ooStatic);
        if (DetachedUMToolBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedUMToolBar->HWindow == NULL)
        {
            if (!DetachedUMToolBar->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedUMToolBar->CreateButtons();
            InsertDetachedBand(HDetachedTopRebar, DetachedUMToolBar->HWindow, BANDID_UMTOOLBAR,
                               Configuration.UserMenuToolbarIndex, Configuration.UserMenuToolbarWidth,
                               DetachedUMToolBar->GetNeededHeight(), Configuration.UserMenuToolbarBreak);
            ShowWindow(DetachedUMToolBar->HWindow, SW_SHOW);
        }
    }

    if (HPToolBar != NULL && HPToolBar->HWindow != NULL)
    {
        if (DetachedHPToolBar == NULL)
            DetachedHPToolBar = new CHotPathsBar(HWindow, ooStatic);
        if (DetachedHPToolBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedHPToolBar->HWindow == NULL)
        {
            if (!DetachedHPToolBar->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedHPToolBar->CreateButtons();
            InsertDetachedBand(HDetachedTopRebar, DetachedHPToolBar->HWindow, BANDID_HPTOOLBAR,
                               Configuration.HotPathsBarIndex, Configuration.HotPathsBarWidth,
                               DetachedHPToolBar->GetNeededHeight(), Configuration.HotPathsBarBreak);
            ShowWindow(DetachedHPToolBar->HWindow, SW_SHOW);
        }
    }

    if (DriveBar != NULL && DriveBar->HWindow != NULL)
    {
        if (DetachedDriveBar == NULL)
            DetachedDriveBar = new CDriveBar(HWindow, ooStatic);
        if (DetachedDriveBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedDriveBar->HWindow == NULL)
        {
            if (!DetachedDriveBar->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedDriveBar->CreateDriveButtons(NULL);
            InsertDetachedBand(HDetachedTopRebar, DetachedDriveBar->HWindow, BANDID_DRIVEBAR,
                               Configuration.DriveBarIndex, Configuration.DriveBarWidth,
                               DetachedDriveBar->GetNeededHeight(), Configuration.DriveBarBreak);
            ShowWindow(DetachedDriveBar->HWindow, SW_SHOW);
        }
    }

    if (DriveBar2 != NULL && DriveBar2->HWindow != NULL)
    {
        if (DetachedDriveBar2 == NULL)
            DetachedDriveBar2 = new CDriveBar(HWindow, ooStatic);
        if (DetachedDriveBar2 == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedDriveBar2->HWindow == NULL)
        {
            if (!DetachedDriveBar2->CreateWnd(HDetachedTopRebar))
                DETACHED_CHROME_FAIL();
            DetachedDriveBar2->CreateDriveButtons(DetachedDriveBar);
            InsertDetachedBand(HDetachedTopRebar, DetachedDriveBar2->HWindow, BANDID_DRIVEBAR2,
                               -1, Configuration.DriveBarWidth,
                               DetachedDriveBar2->GetNeededHeight(), FALSE);
            ShowWindow(DetachedDriveBar2->HWindow, SW_SHOW);
        }
    }

    if (BottomToolBar != NULL && BottomToolBar->HWindow != NULL)
    {
        if (DetachedBottomToolBar == NULL)
        {
            DetachedBottomToolBar = new CBottomToolBar(HWindow, ooStatic);
            if (DetachedBottomToolBar != NULL)
            {
                DetachedBottomToolBar->SetImageList(
                    HDetachedBottomTBImageList != NULL
                        ? HDetachedBottomTBImageList
                        : HBottomTBImageList);
                DetachedBottomToolBar->SetHotImageList(
                    HDetachedHotBottomTBImageList != NULL
                        ? HDetachedHotBottomTBImageList
                        : HHotBottomTBImageList);
            }
        }
        if (DetachedBottomToolBar == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedBottomToolBar->HWindow == NULL)
        {
            if (!CBottomToolBar::InitDataFromResources())
                DETACHED_CHROME_FAIL();
            if (!DetachedBottomToolBar->CreateWnd(HRightDetachedWindow))
                DETACHED_CHROME_FAIL();
            DetachedBottomToolBar->SetFont();
            ShowWindow(DetachedBottomToolBar->HWindow, SW_SHOW);
            UpdateBottomToolBar();
        }
    }

    if (EditWindow != NULL && EditWindow->HWindow != NULL)
    {
        if (DetachedEditWindow == NULL)
            DetachedEditWindow = new CEditWindow();
        if (DetachedEditWindow == NULL)
            DETACHED_CHROME_FAIL();
        if (DetachedEditWindow->HWindow == NULL)
        {
            if (!DetachedEditWindow->Create(HRightDetachedWindow, IDC_EDITWINDOW))
                DETACHED_CHROME_FAIL();
            DetachedEditWindow->SetFont();
            ShowWindow(DetachedEditWindow->HWindow, SW_SHOW);
        }
        UpdateDetachedCommandLine();
    }

    CreatingDetachedChrome = FALSE;
#undef DETACHED_CHROME_FAIL
    return TRUE;
}
void CMainWindow::DestroyDetachedChrome()
{
    if (DetachedEditWindow != NULL && DetachedEditWindow->HWindow != NULL)
        DestroyWindow(DetachedEditWindow->HWindow);
    if (DetachedBottomToolBar != NULL && DetachedBottomToolBar->HWindow != NULL)
        DestroyWindow(DetachedBottomToolBar->HWindow);
    if (DetachedDriveBar2 != NULL && DetachedDriveBar2->HWindow != NULL)
        DestroyWindow(DetachedDriveBar2->HWindow);
    if (DetachedDriveBar != NULL && DetachedDriveBar->HWindow != NULL)
        DestroyWindow(DetachedDriveBar->HWindow);
    if (DetachedHPToolBar != NULL && DetachedHPToolBar->HWindow != NULL)
        DestroyWindow(DetachedHPToolBar->HWindow);
    if (DetachedUMToolBar != NULL && DetachedUMToolBar->HWindow != NULL)
        DestroyWindow(DetachedUMToolBar->HWindow);
    if (DetachedPluginsBar != NULL && DetachedPluginsBar->HWindow != NULL)
        DestroyWindow(DetachedPluginsBar->HWindow);
    if (DetachedExtensionBar != NULL && DetachedExtensionBar->HWindow != NULL)
        DestroyWindow(DetachedExtensionBar->HWindow);
    if (DetachedTopToolBar != NULL && DetachedTopToolBar->HWindow != NULL)
        DestroyWindow(DetachedTopToolBar->HWindow);
    if (DetachedMenuBar != NULL && DetachedMenuBar->HWindow != NULL)
        DestroyWindow(DetachedMenuBar->HWindow);

    if (DetachedEditWindow != NULL)
    {
        delete DetachedEditWindow;
        DetachedEditWindow = NULL;
    }
    if (DetachedBottomToolBar != NULL)
    {
        delete DetachedBottomToolBar;
        DetachedBottomToolBar = NULL;
    }
    if (DetachedDriveBar2 != NULL)
    {
        delete DetachedDriveBar2;
        DetachedDriveBar2 = NULL;
    }
    if (DetachedDriveBar != NULL)
    {
        delete DetachedDriveBar;
        DetachedDriveBar = NULL;
    }
    if (DetachedHPToolBar != NULL)
    {
        delete DetachedHPToolBar;
        DetachedHPToolBar = NULL;
    }
    if (DetachedUMToolBar != NULL)
    {
        delete DetachedUMToolBar;
        DetachedUMToolBar = NULL;
    }
    if (DetachedPluginsBar != NULL)
    {
        delete DetachedPluginsBar;
        DetachedPluginsBar = NULL;
    }
    if (DetachedExtensionBar != NULL)
    {
        delete DetachedExtensionBar;
        DetachedExtensionBar = NULL;
    }
    if (DetachedTopToolBar != NULL)
    {
        delete DetachedTopToolBar;
        DetachedTopToolBar = NULL;
    }
    if (DetachedMenuBar != NULL)
    {
        delete DetachedMenuBar;
        DetachedMenuBar = NULL;
    }
    if (HDetachedTopRebar != NULL)
    {
        DestroyWindow(HDetachedTopRebar);
        HDetachedTopRebar = NULL;
    }
    if (HDetachedHotToolBarImageList != NULL)
    {
        ImageList_Destroy(HDetachedHotToolBarImageList);
        HDetachedHotToolBarImageList = NULL;
    }
    if (HDetachedGrayToolBarImageList != NULL)
    {
        ImageList_Destroy(HDetachedGrayToolBarImageList);
        HDetachedGrayToolBarImageList = NULL;
    }
    if (HDetachedHotBottomTBImageList != NULL)
    {
        ImageList_Destroy(HDetachedHotBottomTBImageList);
        HDetachedHotBottomTBImageList = NULL;
    }
    if (HDetachedBottomTBImageList != NULL)
    {
        ImageList_Destroy(HDetachedBottomTBImageList);
        HDetachedBottomTBImageList = NULL;
    }
    DetachedWindowDPI = 0;
}

void CMainWindow::UpdateDetachedCommandLine()
{
    if (DetachedEditWindow == NULL || DetachedEditWindow->HWindow == NULL || RightPanel == NULL)
        return;

    if (RightPanel->Is(ptDisk) ||
        RightPanel->Is(ptPluginFS) && RightPanel->GetPluginFS()->NotEmpty() &&
            RightPanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_COMMANDLINE))
    {
        CPathBuffer dir(2 * SAL_MAX_PATH);
        RightPanel->GetGeneralPath(dir.Data(), dir.Capacity());
        DetachedEditWindow->Enable(TRUE);
        DetachedEditWindow->SetDirectory(dir.Data());
    }
    else
    {
        DetachedEditWindow->Enable(FALSE);
        DetachedEditWindow->SetDirectory("");
    }
}

void CMainWindow::LayoutDetachedPanelWindow(CPanelSide side, int width, int height)
{
    CFilesWindow* panel = side == cpsLeft ? LeftPanel : RightPanel;
    CTabWindow* tabWindow = side == cpsLeft ? LeftTabWindow : RightTabWindow;
    TIndirectArray<CFilesWindow>& tabs = side == cpsLeft ? LeftPanelTabs : RightPanelTabs;

    if (panel == NULL)
        return;

    int detachedTopRebarHeight = 0;
    int detachedBottomToolBarHeight = 0;
    int detachedEditHeight = 0;
    if (HDetachedTopRebar != NULL)
    {
        RECT rebRect;
        GetWindowRect(HDetachedTopRebar, &rebRect);
        detachedTopRebarHeight = rebRect.bottom - rebRect.top;
    }
    if (DetachedBottomToolBar != NULL && DetachedBottomToolBar->HWindow != NULL)
        detachedBottomToolBarHeight = DetachedBottomToolBar->GetNeededHeight();
    if (DetachedEditWindow != NULL && DetachedEditWindow->HWindow != NULL)
    {
        UpdateDetachedCommandLine();
        detachedEditHeight = DetachedEditWindow->GetNeededHeight() + 1;
    }

    int contentHeight = height - detachedTopRebarHeight - detachedBottomToolBarHeight - detachedEditHeight;
    if (contentHeight < 0)
        contentHeight = 0;

    int tabHeight = 0;
    BOOL tabsVisible = tabWindow != NULL && tabWindow->HWindow != NULL &&
                       Configuration.UsePanelTabs && tabs.Count > 0;
    if (tabsVisible)
        tabHeight = tabWindow->GetNeededHeight();

    int treeWidth = 0;
    int treeDisplayWidth = 0;
    int treeSplitWidth = 0;
    int treeHeaderHeight = tabHeight;
    BOOL treeAutoHideExpanded = FALSE;
    if (panel->HTreeView != NULL && panel->TreeViewActive)
    {
        if (treeHeaderHeight == 0)
            treeHeaderHeight = panel->GetTreeViewHeaderHeight();
        if (panel->TreeViewAutoHide)
            treeDisplayWidth = panel->GetTreeViewWidth(width);
        else
            treeDisplayWidth = panel->GetTreeViewWidth(width);
        treeAutoHideExpanded = panel->TreeViewAutoHide && panel->TreeViewAutoHideExpanded;
        if (panel->TreeViewAutoHide)
            treeWidth = treeHeaderHeight;
        else
        {
            treeWidth = treeDisplayWidth;
            treeSplitWidth = 4;
        }
    }
    int panelX = treeWidth + treeSplitWidth;
    int panelWidth = width - panelX;
    if (panelWidth < 0)
        panelWidth = 0;

    int windowsCount = 0;
    if (HDetachedTopRebar != NULL)
        windowsCount++;
    if (tabWindow != NULL && tabWindow->HWindow != NULL)
        windowsCount++;
    if (panel->HTreeView != NULL && panel->TreeViewActive)
        windowsCount += 3;
    for (int i = 0; i < tabs.Count; ++i)
        if (tabs[i] != NULL && tabs[i]->HWindow != NULL)
            windowsCount++;
    if (DetachedBottomToolBar != NULL && DetachedBottomToolBar->HWindow != NULL)
        windowsCount++;
    if (DetachedEditWindow != NULL && DetachedEditWindow->HWindow != NULL)
        windowsCount++;

    int panelHeight = contentHeight - tabHeight;
    if (panelHeight < 0)
        panelHeight = 0;

    HDWP hdwp = HANDLES(BeginDeferWindowPos(windowsCount));
    if (hdwp != NULL)
    {
        if (HDetachedTopRebar != NULL)
            hdwp = HANDLES(DeferWindowPos(hdwp, HDetachedTopRebar, NULL,
                                          0, 0, width, detachedTopRebarHeight,
                                          SWP_NOACTIVATE | SWP_NOZORDER));

        if (tabWindow != NULL && tabWindow->HWindow != NULL)
        {
            hdwp = HANDLES(DeferWindowPos(hdwp, tabWindow->HWindow, NULL,
                                          panelX, detachedTopRebarHeight, panelWidth, tabHeight,
                                          SWP_NOACTIVATE | SWP_NOZORDER |
                                              (tabsVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)));
        }
        if (panel->HTreeHeader != NULL && panel->TreeViewActive)
        {
            BOOL collapsed = panel->TreeViewAutoHide && !treeAutoHideExpanded;
            int headerWidth = collapsed ? treeWidth + 1 : treeDisplayWidth + (panel->TreeViewAutoHide ? 1 : 0);
            int headerHeight = collapsed ? contentHeight : treeHeaderHeight;
            hdwp = HANDLES(DeferWindowPos(hdwp, panel->HTreeHeader, HWND_TOP,
                                          0, detachedTopRebarHeight, headerWidth, headerHeight,
                                          SWP_NOACTIVATE | SWP_SHOWWINDOW));
        }
        if (panel->HTreeView != NULL && panel->TreeViewActive)
        {
            int treeViewHeight = contentHeight - treeHeaderHeight;
            if (treeViewHeight < 0)
                treeViewHeight = 0;
            BOOL show = !panel->TreeViewAutoHide || treeAutoHideExpanded;
            hdwp = HANDLES(DeferWindowPos(hdwp, panel->HTreeView, HWND_TOP,
                                          0, detachedTopRebarHeight + treeHeaderHeight, treeDisplayWidth + (panel->TreeViewAutoHide ? 1 : 0), treeViewHeight,
                                          SWP_NOACTIVATE | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)));
        }
        if (panel->HTreeSplit != NULL && panel->TreeViewActive)
        {
            BOOL show = !panel->TreeViewAutoHide || treeAutoHideExpanded;
            int displaySplitWidth = show ? 4 : 0;
            hdwp = HANDLES(DeferWindowPos(hdwp, panel->HTreeSplit, HWND_TOP,
                                          treeDisplayWidth, detachedTopRebarHeight, displaySplitWidth, contentHeight,
                                          SWP_NOACTIVATE | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)));
        }
        for (int i = 0; i < tabs.Count; ++i)
        {
            CFilesWindow* tabPanel = tabs[i];
            if (tabPanel == NULL || tabPanel->HWindow == NULL)
                continue;
            UINT flags = SWP_NOACTIVATE | SWP_NOZORDER;
            if (tabPanel == panel)
                flags |= SWP_SHOWWINDOW;
            hdwp = HANDLES(DeferWindowPos(hdwp, tabPanel->HWindow, NULL,
                                          panelX, detachedTopRebarHeight + tabHeight, panelWidth, panelHeight,
                                          flags));
        }
        if (DetachedEditWindow != NULL && DetachedEditWindow->HWindow != NULL)
            hdwp = HANDLES(DeferWindowPos(hdwp, DetachedEditWindow->HWindow, HWND_BOTTOM,
                                          0, detachedTopRebarHeight + contentHeight + 2, width, detachedEditHeight + 150,
                                          SWP_NOACTIVATE));
        if (DetachedBottomToolBar != NULL && DetachedBottomToolBar->HWindow != NULL)
            hdwp = HANDLES(DeferWindowPos(hdwp, DetachedBottomToolBar->HWindow, NULL,
                                          1, detachedTopRebarHeight + contentHeight + detachedEditHeight + 1, max(0, width - 2), detachedBottomToolBarHeight,
                                          SWP_NOACTIVATE | SWP_NOZORDER));
        HANDLES(EndDeferWindowPos(hdwp));
    }

    for (int i = 0; i < tabs.Count; ++i)
    {
        CFilesWindow* tabPanel = tabs[i];
        if (tabPanel != NULL && tabPanel->HWindow != NULL)
        {
            MoveWindow(tabPanel->HWindow, panelX, detachedTopRebarHeight + tabHeight, panelWidth, panelHeight, FALSE);
            ::SendMessage(tabPanel->HWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(panelWidth, panelHeight));
            tabPanel->LayoutListBoxChilds();
            if (tabPanel == panel)
                RedrawWindow(tabPanel->HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }
}

void CMainWindow::LayoutDetachedPanels()
{
    if (!DetachedPanels)
        return;

    RECT r;
    if (HRightDetachedWindow != NULL)
    {
        GetClientRect(HRightDetachedWindow, &r);
        LayoutDetachedPanelWindow(cpsRight, r.right - r.left, r.bottom - r.top);
    }
}

void CMainWindow::LayoutMainWindowDetachedPanel(int width, int height)
{
    if (LeftPanel == NULL)
        return;

    TopRebarHeight = 0;
    BottomToolBarHeight = 0;
    EditHeight = 0;
    PanelsHeight = height - 1;

    RECT rebRect;
    if (HTopRebar != NULL)
    {
        GetWindowRect(HTopRebar, &rebRect);
        TopRebarHeight = rebRect.bottom - rebRect.top;
    }

    if (BottomToolBar != NULL && BottomToolBar->HWindow != NULL)
        BottomToolBarHeight = BottomToolBar->GetNeededHeight();
    if (EditWindow != NULL && EditWindow->HWindow != NULL)
        EditHeight = EditWindow->GetNeededHeight() + 1;

    PanelsHeight -= TopRebarHeight + BottomToolBarHeight + EditHeight;
    if (PanelsHeight < 0)
        PanelsHeight = 0;
    int treeWidth = 0;
    int treeDisplayWidth = 0;
    int treeSplitWidth = 0;
    int treeHeaderHeight = 0;
    BOOL treeAutoHideExpanded = FALSE;
    if (LeftPanel->HTreeView != NULL && LeftPanel->TreeViewActive)
    {
        treeHeaderHeight = LeftPanel->GetTreeViewHeaderHeight();
        if (LeftPanel->TreeViewAutoHide)
            treeDisplayWidth = LeftPanel->GetTreeViewWidth(width);
        else
            treeDisplayWidth = LeftPanel->GetTreeViewWidth(width);
        treeAutoHideExpanded = LeftPanel->TreeViewAutoHide && LeftPanel->TreeViewAutoHideExpanded;
        if (LeftPanel->TreeViewAutoHide)
            treeWidth = treeHeaderHeight;
        else
        {
            treeWidth = treeDisplayWidth;
            treeSplitWidth = 4;
        }
    }

    int panelX = 1 + treeWidth + treeSplitWidth;
    int panelWidth = width - panelX - 1;
    if (panelWidth < 0)
        panelWidth = 0;

    int leftTabHeight = 0;
    BOOL leftTabsVisible = LeftTabWindow != NULL && LeftTabWindow->HWindow != NULL &&
                           Configuration.UsePanelTabs && LeftPanelTabs.Count > 0;
    if (leftTabsVisible)
        leftTabHeight = LeftTabWindow->GetNeededHeight();

    int windowsCount = 1;
    if (LeftTabWindow != NULL && LeftTabWindow->HWindow != NULL)
        windowsCount++;
    for (int i = 0; i < LeftPanelTabs.Count; ++i)
        if (LeftPanelTabs[i] != NULL && LeftPanelTabs[i]->HWindow != NULL)
            windowsCount++;
    if (LeftPanel->HTreeView != NULL && LeftPanel->TreeViewActive)
        windowsCount += 3;
    if (EditWindow != NULL && EditWindow->HWindow != NULL)
        windowsCount++;
    if (BottomToolBar != NULL && BottomToolBar->HWindow != NULL)
        windowsCount++;

    int leftPanelHeight = PanelsHeight - leftTabHeight;
    if (leftPanelHeight < 0)
        leftPanelHeight = 0;

    HDWP hdwp = HANDLES(BeginDeferWindowPos(windowsCount));
    if (hdwp != NULL)
    {
        if (HTopRebar != NULL)
            hdwp = HANDLES(DeferWindowPos(hdwp, HTopRebar, NULL,
                                          0, 0, width, TopRebarHeight,
                                          SWP_NOACTIVATE | SWP_NOZORDER));

        if (LeftPanel->HTreeHeader != NULL && LeftPanel->TreeViewActive)
        {
            BOOL collapsed = LeftPanel->TreeViewAutoHide && !treeAutoHideExpanded;
            int headerWidth = collapsed ? treeWidth + 1 : treeDisplayWidth + (LeftPanel->TreeViewAutoHide ? 1 : 0);
            int headerHeight = collapsed ? PanelsHeight : treeHeaderHeight;
            hdwp = HANDLES(DeferWindowPos(hdwp, LeftPanel->HTreeHeader, HWND_TOP,
                                          0, TopRebarHeight, headerWidth, headerHeight,
                                          SWP_NOACTIVATE | SWP_SHOWWINDOW));
        }
        if (LeftPanel->HTreeView != NULL && LeftPanel->TreeViewActive)
        {
            int treeViewHeight = PanelsHeight - treeHeaderHeight;
            if (treeViewHeight < 0)
                treeViewHeight = 0;
            BOOL show = !LeftPanel->TreeViewAutoHide || treeAutoHideExpanded;
            hdwp = HANDLES(DeferWindowPos(hdwp, LeftPanel->HTreeView, HWND_TOP,
                                          0, TopRebarHeight + treeHeaderHeight, treeDisplayWidth + (LeftPanel->TreeViewAutoHide ? 1 : 0), treeViewHeight,
                                          SWP_NOACTIVATE | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)));
        }
        if (LeftPanel->HTreeSplit != NULL && LeftPanel->TreeViewActive)
        {
            BOOL show = !LeftPanel->TreeViewAutoHide || treeAutoHideExpanded;
            int displaySplitWidth = show ? 4 : 0;
            hdwp = HANDLES(DeferWindowPos(hdwp, LeftPanel->HTreeSplit, HWND_TOP,
                                          treeDisplayWidth, TopRebarHeight, displaySplitWidth, PanelsHeight,
                                          SWP_NOACTIVATE | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)));
        }

        if (LeftTabWindow != NULL && LeftTabWindow->HWindow != NULL)
        {
            hdwp = HANDLES(DeferWindowPos(hdwp, LeftTabWindow->HWindow, NULL,
                                          panelX, TopRebarHeight, panelWidth, leftTabHeight,
                                          SWP_NOACTIVATE | SWP_NOZORDER |
                                              (leftTabsVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)));
        }

        for (int i = 0; i < LeftPanelTabs.Count; ++i)
        {
            CFilesWindow* tabPanel = LeftPanelTabs[i];
            if (tabPanel == NULL || tabPanel->HWindow == NULL)
                continue;
            UINT flags = SWP_NOACTIVATE | SWP_NOZORDER;
            if (tabPanel == LeftPanel)
                flags |= SWP_SHOWWINDOW;
            hdwp = HANDLES(DeferWindowPos(hdwp, tabPanel->HWindow, NULL,
                                          panelX, TopRebarHeight + leftTabHeight, panelWidth, leftPanelHeight,
                                          flags));
        }

        if (EditWindow != NULL && EditWindow->HWindow != NULL)
            hdwp = HANDLES(DeferWindowPos(hdwp, EditWindow->HWindow, HWND_BOTTOM,
                                          0, TopRebarHeight + PanelsHeight + 2, width, EditHeight + 150,
                                          SWP_NOACTIVATE));

        if (BottomToolBar != NULL && BottomToolBar->HWindow != NULL)
            hdwp = HANDLES(DeferWindowPos(hdwp, BottomToolBar->HWindow, NULL,
                                          1, TopRebarHeight + PanelsHeight + EditHeight + 1, panelWidth, BottomToolBarHeight,
                                          SWP_NOACTIVATE | SWP_NOZORDER));

        HANDLES(EndDeferWindowPos(hdwp));
    }

    for (int i = 0; i < LeftPanelTabs.Count; ++i)
    {
        CFilesWindow* tabPanel = LeftPanelTabs[i];
        if (tabPanel != NULL && tabPanel->HWindow != NULL)
        {
            MoveWindow(tabPanel->HWindow, panelX, TopRebarHeight + leftTabHeight, panelWidth, leftPanelHeight, FALSE);
            ::SendMessage(tabPanel->HWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(panelWidth, leftPanelHeight));
            tabPanel->LayoutListBoxChilds();
            if (tabPanel == LeftPanel)
                RedrawWindow(tabPanel->HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }
}

static const char* DETACHED_PANEL_CLASSNAME = "SalamanderDetachedPanelWindow";

static void RegisterDetachedPanelWindowClass()
{
    static BOOL registered = FALSE;
    if (registered)
        return;

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CMainWindow::DetachedPanelWindowProc;
    wc.hInstance = HInstance;
    wc.hIcon = SalLoadIcon(HInstance, IDI_SALAMANDER, IconSizes[ICONSIZE_32]);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = DETACHED_PANEL_CLASSNAME;
    RegisterClass(&wc);
    registered = TRUE;
}

static HWND CreateDetachedPanelWindow(CMainWindow* mainWindow, CPanelSide side)
{
    RegisterDetachedPanelWindowClass();

    char title[4096];
    GetWindowText(mainWindow->HWindow, title, _countof(title));
    if (title[0] == 0)
        lstrcpyn(title, MAINWINDOW_NAME, _countof(title));
    lstrcat(title, " - ");
    lstrcat(title, LoadStr(IDS_DETACHED_WINDOW_TITLE));

    RECT mainRect;
    GetWindowRect(mainWindow->HWindow, &mainRect);
    int width = max(320, (mainRect.right - mainRect.left) / 2);
    int height = max(240, mainRect.bottom - mainRect.top);
    int x = side == cpsLeft ? mainRect.left : mainRect.left + width + 16;
    int y = mainRect.top;

    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW,
                                DETACHED_PANEL_CLASSNAME,
                                title,
                                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                x, y, width, height,
                                NULL, NULL, HInstance, mainWindow);
    if (hWnd != NULL)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((DWORD_PTR)mainWindow | (side == cpsRight ? 1 : 0)));
        DarkModeApplyWindow(hWnd);
        DarkModeRefreshTitleBar(hWnd);
        DarkModeAllowDarkScrollbars(hWnd);
    }
    return hWnd;
}

static const char* DETACHED_TAB_CLASSNAME = "SalamanderDetachedTabWindow";
static const char* DETACHED_TAB_PREVIEW_CLASSNAME = "SalamanderDetachedTabPreview";

static LRESULT CALLBACK DetachedTabPreviewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps);
        FillRect(dc, &ps.rcPaint, GetSysColorBrush(COLOR_HIGHLIGHT));
        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static void RegisterDetachedTabPreviewWindowClass()
{
    static BOOL registered = FALSE;
    if (registered)
        return;

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = DetachedTabPreviewWindowProc;
    wc.hInstance = HInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = DETACHED_TAB_PREVIEW_CLASSNAME;
    RegisterClass(&wc);
    registered = TRUE;
}

static void RegisterDetachedTabWindowClass()
{
    static BOOL registered = FALSE;
    if (registered)
        return;

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CMainWindow::DetachedTabWindowProc;
    wc.hInstance = HInstance;
    wc.hIcon = SalLoadIcon(HInstance, IDI_SALAMANDER, IconSizes[ICONSIZE_32]);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = DETACHED_TAB_CLASSNAME;
    RegisterClass(&wc);
    registered = TRUE;
}

void CMainWindow::GetDetachedTabWindowRect(const POINT* dropPoint, CFilesWindow* sourcePanel, RECT* windowRect) const
{
    RECT mainRect;
    GetWindowRect(HWindow, &mainRect);
    int width = max(320, (mainRect.right - mainRect.left) / 2);
    CFilesWindow* geometryPanel = sourcePanel;
    if (sourcePanel != NULL)
    {
        CFilesWindow* visibleSidePanel = sourcePanel->GetPanelSide() == cpsLeft ? LeftPanel : RightPanel;
        if (visibleSidePanel != NULL && visibleSidePanel->HWindow != NULL &&
            visibleSidePanel != sourcePanel)
        {
            // Hidden tabs keep the rectangle from the last time they were
            // active.  Use the currently laid-out panel on the same side so a
            // context-menu detach gets the same client size as an active-tab
            // or drag detach.
            geometryPanel = visibleSidePanel;
        }
    }
    if (geometryPanel != NULL && geometryPanel->HWindow != NULL)
    {
        RECT panelRect;
        RECT hostRect;
        RECT hostClient;
        HWND sourceHost = GetAncestor(geometryPanel->HWindow, GA_ROOT);
        if (GetWindowRect(geometryPanel->HWindow, &panelRect) && sourceHost != NULL &&
            GetWindowRect(sourceHost, &hostRect) && GetClientRect(sourceHost, &hostClient))
        {
            // The detached top-level window has the same non-client frame as
            // the source host. Include it so its client area (and therefore
            // the panel) keeps exactly the width it had before the drag.
            int nonClientWidth = (hostRect.right - hostRect.left) -
                                 (hostClient.right - hostClient.left);
            width = max(320, panelRect.right - panelRect.left + nonClientWidth);
        }
    }
    int height = max(240, mainRect.bottom - mainRect.top);
    int x = mainRect.right + 16;
    int y = mainRect.top;
    if (dropPoint != NULL)
    {
        x = dropPoint->x - width / 2;
        y = dropPoint->y - 24;
        HMONITOR monitor = MonitorFromPoint(*dropPoint, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(monitor, &mi))
        {
            if (x < mi.rcWork.left)
                x = mi.rcWork.left;
            if (y < mi.rcWork.top)
                y = mi.rcWork.top;
            if (x + width > mi.rcWork.right)
                x = max(mi.rcWork.left, mi.rcWork.right - width);
            if (y + height > mi.rcWork.bottom)
                y = max(mi.rcWork.top, mi.rcWork.bottom - height);
        }
    }
    SetRect(windowRect, x, y, x + width, y + height);
}

void CMainWindow::ShowPanelTabDetachPreview(POINT screenPt)
{
    RECT previewRect;
    CFilesWindow* sourcePanel = GetPanelTabAt(PanelTabCrossDragSourceSide,
                                              PanelTabCrossDragSourceIndex);
    GetDetachedTabWindowRect(&screenPt, sourcePanel, &previewRect);
    int width = previewRect.right - previewRect.left;
    int height = previewRect.bottom - previewRect.top;

    if (HPanelTabDetachPreview == NULL)
    {
        RegisterDetachedTabPreviewWindowClass();
        HPanelTabDetachPreview = CreateWindowEx(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
            DETACHED_TAB_PREVIEW_CLASSNAME, "", WS_POPUP,
            previewRect.left, previewRect.top, width, height,
            HWindow, NULL, HInstance, NULL);
        if (HPanelTabDetachPreview == NULL)
            return;
        DarkModeApplyWindow(HPanelTabDetachPreview);
    }

    int border = max(2, MulDiv(3, (int)WinLibDPIGetWindowDPI(HPanelTabDetachPreview), USER_DEFAULT_SCREEN_DPI));
    HRGN outline = CreateRectRgn(0, 0, width, height);
    HRGN interior = CreateRectRgn(border, border, max(border, width - border), max(border, height - border));
    if (outline != NULL && interior != NULL)
    {
        CombineRgn(outline, outline, interior, RGN_DIFF);
        if (SetWindowRgn(HPanelTabDetachPreview, outline, FALSE) == 0)
            DeleteObject(outline);
        outline = NULL; // SetWindowRgn owns it after success
    }
    if (outline != NULL)
        DeleteObject(outline);
    if (interior != NULL)
        DeleteObject(interior);

    SetWindowPos(HPanelTabDetachPreview, HWND_TOPMOST,
                 previewRect.left, previewRect.top, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    RedrawWindow(HPanelTabDetachPreview, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CMainWindow::HidePanelTabDetachPreview()
{
    if (HPanelTabDetachPreview != NULL)
        ShowWindow(HPanelTabDetachPreview, SW_HIDE);
}

static HWND CreateDetachedTabWindow(CMainWindow* mainWindow, CFilesWindow* sourcePanel, const POINT* dropPoint)
{
    RegisterDetachedTabWindowClass();

    RECT windowRect;
    mainWindow->GetDetachedTabWindowRect(dropPoint, sourcePanel, &windowRect);

    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW,
                                DETACHED_TAB_CLASSNAME,
                                LoadStr(IDS_DETACHED_TAB_WINDOW_TITLE),
                                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                windowRect.left, windowRect.top,
                                windowRect.right - windowRect.left,
                                windowRect.bottom - windowRect.top,
                                NULL, NULL, HInstance, mainWindow);
    if (hWnd != NULL)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)mainWindow);
        DarkModeApplyWindow(hWnd);
        DarkModeRefreshTitleBar(hWnd);
        DarkModeAllowDarkScrollbars(hWnd);
    }
    return hWnd;
}

struct CDetachedOperationTarget
{
    CFilesWindow* Panel;
    ULONGLONG TabId;
    CPanelSide Side;
    BOOL Detached;
    std::wstring DisplayPath;
};

static std::wstring DetachedTargetTextToWide(const char* text)
{
    if (text == NULL || text[0] == 0)
        return std::wstring();
    int len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (len <= 1)
        return std::wstring();
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, -1, &result[0], len);
    result.resize(len - 1);
    return result;
}

class CDetachedOperationTargetDialog : public CCommonDialog
{
private:
    const std::vector<CDetachedOperationTarget>& Targets;
    ULONGLONG PreferredTabId;
    ULONGLONG SelectedTabId;
    BOOL RememberTarget;

public:
    CDetachedOperationTargetDialog(HWND parent,
                                   const std::vector<CDetachedOperationTarget>& targets,
                                   ULONGLONG preferredTabId, BOOL rememberTarget)
        : CCommonDialog(HLanguage, IDD_DETACHED_TARGET, parent), Targets(targets),
          PreferredTabId(preferredTabId), SelectedTabId(0), RememberTarget(rememberTarget)
    {
    }

    ULONGLONG GetSelectedTabId() const { return SelectedTabId; }
    BOOL GetRememberTarget() const { return RememberTarget; }

protected:
    virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_INITDIALOG:
        {
            INT_PTR ret = CCommonDialog::DialogProc(uMsg, wParam, lParam);
            SetWindowText(HWindow, LoadStr(IDS_DETACHED_TARGET_TITLE));
            SetDlgItemText(HWindow, IDT_DETACHED_TARGET_PROMPT, LoadStr(IDS_DETACHED_TARGET_PROMPT));
            SetDlgItemText(HWindow, IDC_DETACHED_TARGET_REMEMBER,
                           LoadStr(IDS_DETACHED_TARGET_REMEMBER));
            SetDlgItemText(HWindow, IDOK, LoadStr(IDS_BUTTON_OK));
            SetDlgItemText(HWindow, IDCANCEL, LoadStr(IDS_BUTTON_CANCEL));
            CheckDlgButton(HWindow, IDC_DETACHED_TARGET_REMEMBER,
                           RememberTarget ? BST_CHECKED : BST_UNCHECKED);

            HWND list = GetDlgItem(HWindow, IDC_DETACHED_TARGET_LIST);
            SendMessage(list, LVM_SETUNICODEFORMAT, TRUE, 0);
            ListView_SetExtendedListViewStyle(list,
                                              ListView_GetExtendedListViewStyle(list) |
                                                  LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

            RECT listRect;
            GetClientRect(list, &listRect);
            int listWidth = listRect.right - listRect.left;
            const int widths[] = {listWidth / 5, listWidth * 3 / 5, listWidth / 5};
            const int headings[] = {IDS_DETACHED_TARGET_SIDE, IDS_DETACHED_TARGET_PATH,
                                    IDS_DETACHED_TARGET_STATE};
            for (int column = 0; column < 3; ++column)
            {
                std::wstring heading = DetachedTargetTextToWide(LoadStr(headings[column]));
                LVCOLUMNW lvc;
                memset(&lvc, 0, sizeof(lvc));
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
                lvc.pszText = const_cast<wchar_t*>(heading.c_str());
                lvc.cx = widths[column];
                lvc.fmt = LVCFMT_LEFT;
                SendMessageW(list, LVM_INSERTCOLUMNW, column, (LPARAM)&lvc);
            }

            int selectedIndex = 0;
            for (size_t i = 0; i < Targets.size(); ++i)
            {
                const CDetachedOperationTarget& target = Targets[i];
                std::wstring side = DetachedTargetTextToWide(
                    LoadStr(target.Side == cpsLeft ? IDS_DETACHED_TARGET_LEFT : IDS_DETACHED_TARGET_RIGHT));
                std::wstring state = DetachedTargetTextToWide(
                    LoadStr(target.Detached ? IDS_DETACHED_TARGET_DETACHED : IDS_DETACHED_TARGET_ATTACHED));

                LVITEMW item;
                memset(&item, 0, sizeof(item));
                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = (int)i;
                item.pszText = const_cast<wchar_t*>(side.c_str());
                item.lParam = (LPARAM)i;
                int row = (int)SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
                if (row >= 0)
                {
                    item.mask = LVIF_TEXT;
                    item.iItem = row;
                    item.iSubItem = 1;
                    item.pszText = const_cast<wchar_t*>(target.DisplayPath.c_str());
                    SendMessageW(list, LVM_SETITEMTEXTW, row, (LPARAM)&item);
                    item.iSubItem = 2;
                    item.pszText = const_cast<wchar_t*>(state.c_str());
                    SendMessageW(list, LVM_SETITEMTEXTW, row, (LPARAM)&item);
                }
                if (target.TabId == PreferredTabId)
                    selectedIndex = (int)i;
            }
            ListView_SetItemState(list, selectedIndex, LVIS_SELECTED | LVIS_FOCUSED,
                                  LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(list, selectedIndex, FALSE);
            SetFocus(list);
            return FALSE;
        }

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->idFrom == IDC_DETACHED_TARGET_LIST &&
                ((LPNMHDR)lParam)->code == NM_DBLCLK)
            {
                PostMessage(HWindow, WM_COMMAND, IDOK, 0);
                return TRUE;
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                HWND list = GetDlgItem(HWindow, IDC_DETACHED_TARGET_LIST);
                int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
                if (row < 0)
                    return TRUE;
                LVITEMW item;
                memset(&item, 0, sizeof(item));
                item.mask = LVIF_PARAM;
                item.iItem = row;
                if (SendMessageW(list, LVM_GETITEMW, 0, (LPARAM)&item) &&
                    item.lParam >= 0 && (size_t)item.lParam < Targets.size())
                {
                    SelectedTabId = Targets[(size_t)item.lParam].TabId;
                }
                if (SelectedTabId == 0)
                    return TRUE;
                RememberTarget = IsDlgButtonChecked(HWindow, IDC_DETACHED_TARGET_REMEMBER) == BST_CHECKED;
            }
            break;
        }
        return CCommonDialog::DialogProc(uMsg, wParam, lParam);
    }
};

static BOOL HasSelectedTargetDirectory(CFilesWindow* panel)
{
    if (panel == NULL || !panel->Is(ptDisk))
        return FALSE;
    for (int i = 0; i < panel->Dirs->Count; ++i)
    {
        const CFileData& dir = panel->Dirs->At(i);
        if (dir.Selected && strcmp(dir.Name, "..") != 0)
            return TRUE;
    }
    return FALSE;
}

static BOOL IsDetachedOperationTargetSuitable(CFilesWindow* source, CFilesWindow* target,
                                              UINT command)
{
    if (source == NULL || target == NULL || source == target)
        return FALSE;

    if (command == CM_COPYTOSELECTEDDIRS)
        return source->Is(ptDisk) && HasSelectedTargetDirectory(target);
    if (command == CM_PACK || command == CM_UNPACK)
        return target->Is(ptDisk);
    if (command != CM_COPYFILES && command != CM_MOVEFILES)
        return FALSE;

    if (target->Is(ptDisk))
        return TRUE;
    if (target->Is(ptZIPArchive))
    {
        int format = PackerFormatConfig.PackIsArchive(target->GetZIPArchive());
        return format != 0 && PackerFormatConfig.GetUsePacker(format - 1);
    }
    if (!target->Is(ptPluginFS) || !target->GetPluginFS()->NotEmpty())
        return FALSE;

    if (source->Is(ptDisk))
    {
        return target->GetPluginFS()->IsServiceSupported(
            command == CM_COPYFILES ? FS_SERVICE_COPYFROMDISKTOFS : FS_SERVICE_MOVEFROMDISKTOFS);
    }
    return target->GetPluginFS()->IsServiceSupported(FS_SERVICE_COPYFROMDISKTOFS);
}

static std::wstring GetDetachedTargetDisplayPath(CFilesWindow* panel)
{
    std::wstring result;
    if (panel->HasCustomTabPrefix())
    {
        result = panel->GetCustomTabPrefix();
        result.append(L" — ");
    }
    const wchar_t* path = panel->GetPathW();
    if (path != NULL)
        result.append(path);
    return result;
}

static CFilesWindow* FindDetachedOperationTargetById(CMainWindow* mainWindow,
                                                     CFilesWindow* sourcePanel,
                                                     ULONGLONG tabId, UINT command)
{
    if (tabId == 0)
        return NULL;
    for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
    {
        CPanelSide side = sideIndex == 0 ? cpsLeft : cpsRight;
        for (int i = 0; i < mainWindow->GetPanelTabCount(side); ++i)
        {
            CFilesWindow* panel = mainWindow->GetPanelTabAt(side, i);
            if (panel != NULL && panel->GetPanelTabId() == tabId &&
                IsDetachedOperationTargetSuitable(sourcePanel, panel, command))
                return panel;
        }
    }
    for (int i = 0; i < mainWindow->GetDetachedTabCount(); ++i)
    {
        CFilesWindow* panel = mainWindow->GetDetachedTabAt(i);
        if (panel != NULL && panel->GetPanelTabId() == tabId &&
            IsDetachedOperationTargetSuitable(sourcePanel, panel, command))
            return panel;
    }
    return NULL;
}

CFilesWindow* CMainWindow::SelectDetachedOperationTarget(CFilesWindow* sourcePanel, UINT command,
                                                          BOOL forceDialog)
{
    CDetachedTabInfo* sourceInfo = FindDetachedTab(sourcePanel);
    if (sourceInfo == NULL)
        return GetOtherPanel(sourcePanel);

    if (!forceDialog && sourceInfo->RememberOperationTarget)
    {
        CFilesWindow* remembered = FindDetachedOperationTargetById(
            this, sourcePanel, sourceInfo->LastOperationTargetTabId, command);
        if (remembered != NULL)
            return remembered;
    }

    std::vector<CDetachedOperationTarget> targets;
    for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
    {
        CPanelSide side = sideIndex == 0 ? cpsLeft : cpsRight;
        for (int i = 0; i < GetPanelTabCount(side); ++i)
        {
            CFilesWindow* panel = GetPanelTabAt(side, i);
            if (IsDetachedOperationTargetSuitable(sourcePanel, panel, command))
            {
                CDetachedOperationTarget target = {panel, panel->GetPanelTabId(), side, FALSE,
                                                   GetDetachedTargetDisplayPath(panel)};
                targets.push_back(target);
            }
        }
    }
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        CFilesWindow* panel = DetachedTabs[i].Panel;
        if (IsDetachedOperationTargetSuitable(sourcePanel, panel, command))
        {
            CDetachedOperationTarget target = {panel, panel->GetPanelTabId(),
                                               DetachedTabs[i].OriginalSide, TRUE,
                                               GetDetachedTargetDisplayPath(panel)};
            targets.push_back(target);
        }
    }

    HWND parent = sourceInfo->HWindow != NULL ? sourceInfo->HWindow : HWindow;
    if (targets.empty())
    {
        SalMessageBox(parent, LoadStr(IDS_DETACHED_TARGET_NONE), LoadStr(IDS_INFOTITLE),
                      MB_OK | MB_ICONINFORMATION);
        return NULL;
    }

    CDetachedOperationTargetDialog dialog(parent, targets, sourceInfo->LastOperationTargetTabId,
                                           sourceInfo->RememberOperationTarget);
    if (dialog.Execute() != IDOK)
        return NULL;
    CFilesWindow* selected = FindDetachedOperationTargetById(this, sourcePanel,
                                                            dialog.GetSelectedTabId(), command);
    sourceInfo = FindDetachedTab(sourcePanel);
    if (sourceInfo == NULL)
        return NULL;
    if (selected == NULL)
        return SelectDetachedOperationTarget(sourcePanel, command, TRUE);
    sourceInfo->RememberOperationTarget = dialog.GetRememberTarget();
    sourceInfo->LastOperationTargetTabId = sourceInfo->RememberOperationTarget
                                               ? selected->GetPanelTabId()
                                               : 0;
    return selected;
}

CDetachedTabInfo* CMainWindow::FindDetachedTab(CFilesWindow* panel)
{
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
        if (DetachedTabs[i].Panel == panel)
            return &DetachedTabs[i];
    return NULL;
}

const CDetachedTabInfo* CMainWindow::FindDetachedTab(CFilesWindow* panel) const
{
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
        if (DetachedTabs[i].Panel == panel)
            return &DetachedTabs[i];
    return NULL;
}

CDetachedTabInfo* CMainWindow::FindDetachedTab(HWND hWnd)
{
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
        if (DetachedTabs[i].HWindow == hWnd)
            return &DetachedTabs[i];
    return NULL;
}

const CDetachedTabInfo* CMainWindow::FindDetachedTab(HWND hWnd) const
{
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
        if (DetachedTabs[i].HWindow == hWnd)
            return &DetachedTabs[i];
    return NULL;
}

BOOL CMainWindow::SelectDetachedTab(HWND hWnd)
{
    CDetachedTabInfo* info = FindDetachedTab(hWnd);
    if (info == NULL)
        return FALSE;
    DetachedTabPanel = info->Panel;
    DetachedTabOriginalSide = info->OriginalSide;
    DetachedTabOriginalIndex = info->OriginalIndex;
    HDetachedTabWindow = info->HWindow;
    HDetachedTabGrayToolBarImageList = info->HGrayToolBarImageList;
    HDetachedTabHotToolBarImageList = info->HHotToolBarImageList;
    DetachedTabWindowDPI = info->WindowDPI;
    DetachedTabDPIRefreshPosted = info->DPIRefreshPosted;
    return TRUE;
}

CPanelSide CMainWindow::GetDetachedTabOriginalSide(CFilesWindow* panel) const
{
    const CDetachedTabInfo* info = FindDetachedTab(panel);
    return info != NULL ? info->OriginalSide : cpsLeft;
}

CFilesWindow* CMainWindow::FindDetachedTabPanelByPluginFS(CPluginFSInterfaceAbstract* pluginFS)
{
    if (pluginFS == NULL)
        return NULL;
    CFilesWindow* result = NULL;
    CDetachedTabsLock lock(&DetachedTabsCS);
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        CFilesWindow* panel = DetachedTabs[i].Panel;
        if (panel != NULL && panel->Is(ptPluginFS) &&
            panel->GetPluginFS()->Contains(pluginFS))
        {
            result = panel;
            break;
        }
    }
    return result;
}

BOOL CMainWindow::DetachPanelTab(CFilesWindow* panel, const POINT* dropPoint, BOOL showWindow)
{
    if (!Configuration.UsePanelTabs || panel == NULL || FindDetachedTab(panel) != NULL)
        return FALSE;

    CPanelSide side = panel->GetPanelSide();
    int index = GetPanelTabIndex(side, panel);
    if (index <= 0 || panel->IsTabLocked())
        return FALSE;

    HWND hDetachedWindow = CreateDetachedTabWindow(this, panel, dropPoint);
    if (hDetachedWindow == NULL)
        return FALSE;

    CancelPanelsUI();
    TIndirectArray<CFilesWindow>& tabs = GetPanelTabs(side);
    CTabWindow* tabWnd = GetPanelTabWindow(side);
    BOOL wasSideActive = (side == cpsLeft ? LeftPanel : RightPanel) == panel;
    BOOL wasGloballyActive = GetActivePanel() == panel;

    if (tabWnd != NULL && tabWnd->HWindow != NULL)
        tabWnd->RemoveTab(index);
    tabs.Detach(index);

    CFilesWindow* replacement = NULL;
    if (tabs.Count > 0)
        replacement = tabs[min(index, tabs.Count - 1)];
    if (wasSideActive && replacement != NULL)
        // The detached window takes foreground below.  Populate the newly
        // exposed source tab synchronously while it is still active, otherwise
        // its posted refresh can be deferred until the next click.
        SwitchPanelTab(replacement, false);
    else
        UpdatePanelTabVisibility(side);

    panel->DestroyTreeView();
    CDetachedTabInfo info;
    info.Panel = panel;
    info.OriginalSide = side;
    info.OriginalIndex = index;
    info.HWindow = hDetachedWindow;
    {
        CDetachedTabsLock lock(&DetachedTabsCS);
        DetachedTabs.push_back(info);
    }
    SelectDetachedTab(hDetachedWindow);
    panel->SetPanelSide(side);
    SetParent(panel->HWindow, hDetachedWindow);
    DarkModeApplyTree(hDetachedWindow);
    RebuildDetachedTabToolbarImageLists((int)WinLibDPIGetWindowDPI(hDetachedWindow), hDetachedWindow);
    SendMessage(panel->HWindow, WM_DPICHANGED_AFTERPARENT, 0, 0);
    // WM_DPICHANGED_AFTERPARENT refreshes children only when the DPI changed.
    // A same-DPI detach still changes the owning top-level window and therefore
    // its image lists.  Rebind the directory toolbar unconditionally so hidden
    // tabs also restore Change Drive and the other dynamic button images.
    if (panel->DirectoryLine != NULL)
        panel->DirectoryLine->SetFont();
    // Reparenting a live panel invalidates its icon cache. During configuration
    // restore the saved path is opened only after the panel reaches this final
    // host, so an additional refresh here would enumerate an extension FS
    // twice and visibly clear/repopulate it during startup.
    if (!RestoringPanelPaths)
        EnsurePanelRefreshAndRequest(panel, false, true);
    LayoutDetachedTabWindow(hDetachedWindow);
    ShowWindow(panel->HWindow, SW_SHOW);
    Configuration.DetachedTab = TRUE;
    SetWindowTitle();

    if (showWindow)
    {
        ShowWindow(hDetachedWindow, SW_SHOW);
        SetForegroundWindow(hDetachedWindow);
        SetFocus(panel->GetListBoxHWND());
        SetActivePanel(panel);
        panel->SetPanelSide(side);
        RefreshCommandStates();
    }
    else if (wasGloballyActive && replacement != NULL)
        SetActivePanel(replacement);

    Plugins.Event(PLUGINEVENT_TABCHANGED, side == cpsRight ? PANEL_RIGHT : PANEL_LEFT);
    LayoutWindows();
    return TRUE;
}

BOOL CMainWindow::ReattachDetachedTab(CPanelSide targetSide, BOOL activate)
{
    return ReattachDetachedTab(DetachedTabPanel, targetSide, activate);
}

BOOL CMainWindow::ReattachDetachedTab(CFilesWindow* panel, CPanelSide targetSide, BOOL activate)
{
    CDetachedTabInfo* info = FindDetachedTab(panel);
    if (info == NULL || (targetSide != cpsLeft && targetSide != cpsRight))
        return FALSE;

    CPanelSide originalSide = info->OriginalSide;
    int insertIndex = targetSide == originalSide ? info->OriginalIndex : GetPanelTabs(targetSide).Count;
    BOOL wasGloballyActive = GetActivePanel() == panel;
    HWND detachedWindow = info->HWindow;

    const BOOL suppressDetachedRedraw = detachedWindow != NULL && IsWindowVisible(detachedWindow);
    if (detachedWindow != NULL)
    {
        info->Placement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(detachedWindow, &info->Placement);
        if (suppressDetachedRedraw)
            SendMessage(detachedWindow, WM_SETREDRAW, FALSE, 0);
    }

    HWND targetHost = DetachedPanels && targetSide == cpsRight && HRightDetachedWindow != NULL
                          ? HRightDetachedWindow
                          : HWindow;
    const BOOL suppressTargetRedraw = IsWindowVisible(targetHost);
    if (suppressTargetRedraw)
        SendMessage(targetHost, WM_SETREDRAW, FALSE, 0);
    SetParent(panel->HWindow, targetHost);
    if (!InsertPanelTabInstance(targetSide, insertIndex, panel, true))
    {
        SetParent(panel->HWindow, detachedWindow);
        if (suppressTargetRedraw)
        {
            SendMessage(targetHost, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(targetHost, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        if (suppressDetachedRedraw)
        {
            SendMessage(detachedWindow, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(detachedWindow, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        return FALSE;
    }

    SendMessage(panel->HWindow, WM_DPICHANGED_AFTERPARENT, 0, 0);
    EnsurePanelRefreshAndRequest(panel, false, true);
    size_t detachedIndex = (size_t)(info - &DetachedTabs[0]);
    if (info->HHotToolBarImageList != NULL)
        ImageList_Destroy(info->HHotToolBarImageList);
    if (info->HGrayToolBarImageList != NULL)
        ImageList_Destroy(info->HGrayToolBarImageList);
    {
        CDetachedTabsLock lock(&DetachedTabsCS);
        DetachedTabs.erase(DetachedTabs.begin() + detachedIndex);
    }
    Configuration.DetachedTab = !DetachedTabs.empty();
    if (!DetachedTabs.empty())
        SelectDetachedTab(DetachedTabs.back().HWindow);
    else
    {
        DetachedTabPanel = NULL;
        DetachedTabOriginalIndex = -1;
        HDetachedTabWindow = NULL;
        HDetachedTabGrayToolBarImageList = NULL;
        HDetachedTabHotToolBarImageList = NULL;
        DetachedTabWindowDPI = 0;
        DetachedTabDPIRefreshPosted = FALSE;
    }
    if (activate)
        SwitchPanelTab(panel);
    else
    {
        ShowWindow(panel->HWindow, SW_HIDE);
        if (wasGloballyActive)
        {
            if (!DetachedTabs.empty())
            {
                CDetachedTabInfo& remaining = DetachedTabs.back();
                SelectDetachedTab(remaining.HWindow);
                SetActivePanel(remaining.Panel);
                remaining.Panel->SetPanelSide(remaining.OriginalSide);
            }
            else
                SetActivePanel(targetSide == cpsLeft ? LeftPanel : RightPanel);
        }
        UpdatePanelTabVisibility(targetSide);
    }
    SetWindowTitle();

    if (Configuration.TreeViewVisible && activate)
        panel->UpdateTreeView(targetSide == cpsLeft || (DetachedPanels && targetSide == cpsRight));
    LayoutWindows();
    RefreshCommandStates();
    Plugins.Event(PLUGINEVENT_TABCHANGED, targetSide == cpsRight ? PANEL_RIGHT : PANEL_LEFT);
    if (suppressTargetRedraw)
    {
        SendMessage(targetHost, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(targetHost, NULL, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    // Keep the old detached surface visible and frozen until the complete
    // target-host frame exists behind it. Destroying it earlier exposes a hole
    // followed by the target panel being assembled in several visible steps.
    if (detachedWindow != NULL)
    {
        if (suppressDetachedRedraw)
            DarkModeSetWindowCloaked(detachedWindow, true);
        DestroyWindow(detachedWindow);
    }
    return TRUE;
}

void CMainWindow::CloseDetachedTab()
{
    CloseDetachedTab(DetachedTabPanel);
}

void CMainWindow::CloseDetachedTab(CFilesWindow* panel)
{
    CDetachedTabInfo* info = FindDetachedTab(panel);
    if (info == NULL)
        return;
    CPanelSide side = info->OriginalSide;
    if (ReattachDetachedTab(panel, side, FALSE))
    {
        ClosePanelTab(panel);
        // Do not leave an explicitly closed detached tab dependent on a later
        // full application shutdown.  If shutdown is interrupted, the stale
        // portable configuration must not resurrect this window next time.
        SaveDetachedTabConfigNow();
    }
}

void CMainWindow::LayoutDetachedTabWindow(HWND hWnd)
{
    if (hWnd == NULL)
        hWnd = HDetachedTabWindow;
    CDetachedTabInfo* info = FindDetachedTab(hWnd);
    if (info == NULL || info->Panel == NULL || info->Panel->HWindow == NULL)
        return;
    RECT r;
    GetClientRect(hWnd, &r);
    int width = max(0, r.right - r.left);
    int height = max(0, r.bottom - r.top);
    MoveWindow(info->Panel->HWindow, 0, 0, width, height, TRUE);
    SendMessage(info->Panel->HWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(width, height));
    info->Panel->LayoutListBoxChilds();
}

void CMainWindow::ShowDetachedTabWindowFromConfig()
{
    ShowDetachedTabWindowsFromConfig();
}

void CMainWindow::ShowDetachedTabWindowsFromConfig()
{
    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        CDetachedTabInfo& info = DetachedTabs[i];
        if (info.Placement.length != 0)
        {
            WINDOWPLACEMENT place = info.Placement;
            place.length = sizeof(WINDOWPLACEMENT);
            if (place.showCmd == SW_MINIMIZE || place.showCmd == SW_SHOWMINIMIZED)
                place.showCmd = SW_SHOWNORMAL;
            SetWindowPlacement(info.HWindow, &place);
        }
        LayoutDetachedTabWindow(info.HWindow);
        ShowWindow(info.HWindow, SW_SHOW);
    }
}

BOOL CMainWindow::SetPanelsDetached(BOOL detached)
{
    CALL_STACK_MESSAGE2("CMainWindow::SetPanelsDetached(%d)", detached);

    if (DetachedPanels == detached)
        return TRUE;
    if (LeftPanel == NULL || RightPanel == NULL)
        return FALSE;

    CancelPanelsUI();

    if (detached)
    {
        RECT mainRect;
        RECT mainClientRect;
        GetWindowRect(HWindow, &mainRect);
        GetClientRect(HWindow, &mainClientRect);
        int mainOuterWidth = mainRect.right - mainRect.left;
        int mainOuterHeight = mainRect.bottom - mainRect.top;
        int mainClientWidth = mainClientRect.right - mainClientRect.left;
        int nonClientWidth = mainOuterWidth - mainClientWidth;
        int leftClientWidth = SplitPositionPix > 0 ? SplitPositionPix : mainClientWidth / 2;
        if (leftClientWidth < MIN_WIN_WIDTH)
            leftClientWidth = MIN_WIN_WIDTH;
        int leftOuterWidth = leftClientWidth + nonClientWidth;
        if (leftOuterWidth < 320)
            leftOuterWidth = 320;
        int splitWidth = GetSplitBarWidth();
        int rightClientWidth = mainClientWidth - leftClientWidth - splitWidth;
        if (rightClientWidth < MIN_WIN_WIDTH)
            rightClientWidth = MIN_WIN_WIDTH;
        int rightOuterWidth = rightClientWidth + nonClientWidth;
        if (rightOuterWidth < 320)
            rightOuterWidth = 320;

        if (HRightDetachedWindow == NULL)
            HRightDetachedWindow = CreateDetachedPanelWindow(this, cpsRight);
        if (HRightDetachedWindow == NULL)
            return FALSE;

        // Build the detached host as one compositor transaction. Reparenting
        // the right panel first used to expose an empty half of the main window
        // while its duplicate chrome and DPI resources were still being made.
        const BOOL suppressMainRedraw = IsWindowVisible(HWindow);
        if (suppressMainRedraw)
            SendMessage(HWindow, WM_SETREDRAW, FALSE, 0);
        SendMessage(HRightDetachedWindow, WM_SETREDRAW, FALSE, 0);
        const BOOL detachedWindowCloaked = DarkModeSetWindowCloaked(HRightDetachedWindow, true);

        if (RightTabWindow != NULL && RightTabWindow->HWindow != NULL)
        {
            SetParent(RightTabWindow->HWindow, HRightDetachedWindow);
            RightTabWindow->RefreshDPIResources();
        }
        for (int i = 0; i < RightPanelTabs.Count; ++i)
        {
            CFilesWindow* tabPanel = RightPanelTabs[i];
            if (tabPanel != NULL && tabPanel->HWindow != NULL)
            {
                SetParent(tabPanel->HWindow, HRightDetachedWindow);
                // Rebind per-host image lists even if both hosts happen to use
                // the same numeric DPI.
                SendMessage(tabPanel->HWindow, WM_DPICHANGED_AFTERPARENT, 0, 0);
            }
        }
        DarkModeApplyTree(HRightDetachedWindow);

        DetachedPanels = TRUE;
        Configuration.DetachedPanels = TRUE;
        DetachedPanelsSwapFixNeeded = FALSE;
        UpdateDetachedMenuLabels();
        if (!EnsureDetachedChrome())
        {
            SendMessage(HRightDetachedWindow, WM_SETREDRAW, TRUE, 0);
            if (detachedWindowCloaked)
                DarkModeSetWindowCloaked(HRightDetachedWindow, false);
            if (suppressMainRedraw)
            {
                SendMessage(HWindow, WM_SETREDRAW, TRUE, 0);
                RedrawWindow(HWindow, NULL, NULL,
                             RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                                 RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
            return FALSE;
        }
        CreatingDetachedChrome = TRUE;
        RightPanel->TreeViewWidth = Configuration.DetachedTreeViewWidth;
        RightPanel->TreeViewAutoHide = Configuration.DetachedTreeViewAutoHide;
        RightPanel->TreeViewAutoHideExpanded = FALSE;
        RightPanel->TreeViewAutoHideCollapseStart = 0;
        RightPanel->UpdateTreeView(TRUE);
        SetWindowPos(HRightDetachedWindow, NULL, mainRect.left + leftOuterWidth + 16, mainRect.top,
                     rightOuterWidth, mainOuterHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(HRightDetachedWindow, SW_SHOW);
        SetWindowPos(HWindow, NULL, mainRect.left, mainRect.top, leftOuterWidth, mainOuterHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        LayoutWindows();
        LayoutDetachedPanels();
        SetWindowTitle();
        SetActivePanel(RightPanel);
        Plugins.Event(PLUGINEVENT_TABCHANGED, PANEL_RIGHT);
        CreatingDetachedChrome = FALSE;

        SendMessage(HRightDetachedWindow, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(HRightDetachedWindow, NULL, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                         RDW_ALLCHILDREN | RDW_UPDATENOW);
        if (suppressMainRedraw)
        {
            SendMessage(HWindow, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(HWindow, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                             RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        if (detachedWindowCloaked)
            DarkModeSetWindowCloaked(HRightDetachedWindow, false);
        return TRUE;
    }
    else
    {
        // Reparenting the right-side tab strip and every panel, rebuilding the
        // shared chrome, resizing the main window, and replaying swap state are
        // one visual transaction. Keep both currently visible hosts frozen so
        // none of those intermediate rectangles is presented.
        const BOOL suppressMainRedraw = IsWindowVisible(HWindow);
        const BOOL suppressDetachedRedraw = HRightDetachedWindow != NULL &&
                                             IsWindowVisible(HRightDetachedWindow);
        if (suppressMainRedraw)
            SendMessage(HWindow, WM_SETREDRAW, FALSE, 0);
        if (suppressDetachedRedraw)
            SendMessage(HRightDetachedWindow, WM_SETREDRAW, FALSE, 0);

        RECT mainRectBeforeReattach;
        RECT mainClientBeforeReattach;
        RECT detachedRectBeforeReattach;
        RECT detachedClientBeforeReattach;
        GetWindowRect(HWindow, &mainRectBeforeReattach);
        GetClientRect(HWindow, &mainClientBeforeReattach);
        SetRectEmpty(&detachedRectBeforeReattach);
        SetRectEmpty(&detachedClientBeforeReattach);
        if (HRightDetachedWindow != NULL)
        {
            GetWindowRect(HRightDetachedWindow, &detachedRectBeforeReattach);
            GetClientRect(HRightDetachedWindow, &detachedClientBeforeReattach);
        }

        if (RightTabWindow != NULL && RightTabWindow->HWindow != NULL)
        {
            SetParent(RightTabWindow->HWindow, HWindow);
            RightTabWindow->RefreshDPIResources();
        }
        for (int i = 0; i < RightPanelTabs.Count; ++i)
        {
            CFilesWindow* tabPanel = RightPanelTabs[i];
            if (tabPanel != NULL && tabPanel->HWindow != NULL)
            {
                SetParent(tabPanel->HWindow, HWindow);
                // The directory/status toolbars must stop referencing private
                // detached image lists before DestroyDetachedChrome destroys
                // them, even on a same-DPI reattach.
                SendMessage(tabPanel->HWindow, WM_DPICHANGED_AFTERPARENT, 0, 0);
            }
        }

        if (HRightDetachedWindow != NULL)
        {
            Configuration.DetachedWindowPlacement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(HRightDetachedWindow, &Configuration.DetachedWindowPlacement);
        }
        DetachedPanels = FALSE;
        Configuration.DetachedPanels = FALSE;
        UpdateDetachedMenuLabels();
        PanelZoomedState = 0;
        KeepSplitPositionCenteredOnVisiblePanes = FALSE;
        SplitPosition = 0.5;
        CreatingDetachedChrome = TRUE;
        for (int i = 0; i < RightPanelTabs.Count; ++i)
        {
            CFilesWindow* tabPanel = RightPanelTabs[i];
            if (tabPanel != NULL)
                tabPanel->UpdateTreeView(FALSE);
        }
        CreatingDetachedChrome = FALSE;
        DestroyDetachedChrome();
        // The detached menu used a different DPI and shared menu item metrics.
        // Re-measure the main menu before the rebar negotiates its bands again;
        // otherwise the old cxMinChild clips item captions on their right edge.
        if (MenuBar != NULL)
            MenuBar->SetFont();
        UpdateRebarVisuals();
        RebuildPanelTabs(cpsLeft);
        RebuildPanelTabs(cpsRight);
        RefreshPanelTabLayout();
        int mainClientWidthBeforeReattach = mainClientBeforeReattach.right - mainClientBeforeReattach.left;
        int detachedClientWidthBeforeReattach = detachedClientBeforeReattach.right - detachedClientBeforeReattach.left;
        if (detachedClientWidthBeforeReattach > 0)
        {
            int mainOuterWidthBeforeReattach = mainRectBeforeReattach.right - mainRectBeforeReattach.left;
            int mainOuterHeightBeforeReattach = mainRectBeforeReattach.bottom - mainRectBeforeReattach.top;
            int mainClientHeightBeforeReattach = mainClientBeforeReattach.bottom - mainClientBeforeReattach.top;
            int detachedOuterHeightBeforeReattach = detachedRectBeforeReattach.bottom - detachedRectBeforeReattach.top;
            int detachedClientHeightBeforeReattach = detachedClientBeforeReattach.bottom - detachedClientBeforeReattach.top;
            int nonClientWidth = mainOuterWidthBeforeReattach - mainClientWidthBeforeReattach;
            int nonClientHeight = mainOuterHeightBeforeReattach - mainClientHeightBeforeReattach;
            int combinedClientWidth = mainClientWidthBeforeReattach + detachedClientWidthBeforeReattach + GetSplitBarWidth();
            int combinedClientHeight = max(mainClientHeightBeforeReattach, detachedClientHeightBeforeReattach);
            int combinedOuterWidth = combinedClientWidth + nonClientWidth;
            int combinedOuterHeight = max(combinedClientHeight + nonClientHeight, detachedOuterHeightBeforeReattach);
            SetWindowPos(HWindow, NULL, mainRectBeforeReattach.left, mainRectBeforeReattach.top,
                         combinedOuterWidth, combinedOuterHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        RECT mainClientRect;
        GetClientRect(HWindow, &mainClientRect);
        WindowWidth = mainClientRect.right - mainClientRect.left;
        WindowHeight = mainClientRect.bottom - mainClientRect.top;
        LayoutWindows();
        SetWindowTitle();

        // WM_SIZE normally restores the two-panel layout.  After Swap Sides while detached,
        // however, the tabs being reattached have just moved between two different top-level
        // hosts and may still carry the single-host/maximized rectangle.  Normalize every tab
        // window to the freshly computed main-window split so the reattached side becomes the
        // right pane immediately instead of overlapping the left pane.
        int splitWidth = GetSplitBarWidth();
        int totalPanelsWidth = WindowWidth - 2 - splitWidth;
        if (totalPanelsWidth < 0)
            totalPanelsWidth = 0;

        int treeWidth = 0;
        int treeSplitWidth = 0;
        if (LeftPanel != NULL && LeftPanel->HTreeView != NULL && LeftPanel->TreeViewActive)
        {
            int treeHeaderHeight = LeftPanel->GetTreeViewHeaderHeight();
            if (LeftTabWindow != NULL)
                treeHeaderHeight = LeftTabWindow->GetNeededHeight();
            if (LeftPanel->TreeViewAutoHide)
                treeWidth = treeHeaderHeight;
            else
            {
                treeWidth = LeftPanel->GetTreeViewWidth(totalPanelsWidth);
                treeSplitWidth = 4;
            }
        }

        int leftX = 1 + treeWidth + treeSplitWidth;
        int leftWidth = SplitPositionPix - leftX;
        if (leftWidth < 0)
            leftWidth = 0;
        int rightX = SplitPositionPix + splitWidth;
        int rightWidth = WindowWidth - 2 - rightX;
        if (rightWidth < 0)
            rightWidth = 0;

        int leftTabHeight = LeftTabWindow != NULL && LeftTabWindow->HWindow != NULL &&
                                    Configuration.UsePanelTabs && LeftPanelTabs.Count > 0 ?
                                LeftTabWindow->GetNeededHeight() :
                                0;
        int rightTabHeight = RightTabWindow != NULL && RightTabWindow->HWindow != NULL &&
                                     Configuration.UsePanelTabs && RightPanelTabs.Count > 0 ?
                                 RightTabWindow->GetNeededHeight() :
                                 0;
        int leftPanelHeight = PanelsHeight - leftTabHeight;
        int rightPanelHeight = PanelsHeight - rightTabHeight;
        if (leftPanelHeight < 0)
            leftPanelHeight = 0;
        if (rightPanelHeight < 0)
            rightPanelHeight = 0;

        if (LeftTabWindow != NULL && LeftTabWindow->HWindow != NULL)
            MoveWindow(LeftTabWindow->HWindow, leftX, TopRebarHeight, leftWidth, leftTabHeight, TRUE);
        if (RightTabWindow != NULL && RightTabWindow->HWindow != NULL)
            MoveWindow(RightTabWindow->HWindow, rightX, TopRebarHeight, rightWidth, rightTabHeight, TRUE);

        for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
        {
            BOOL leftSide = sideIndex == 0;
            TIndirectArray<CFilesWindow>& tabs = leftSide ? LeftPanelTabs : RightPanelTabs;
            int panelX = leftSide ? leftX : rightX;
            int panelY = TopRebarHeight + (leftSide ? leftTabHeight : rightTabHeight);
            int panelWidth = leftSide ? leftWidth : rightWidth;
            int panelHeight = leftSide ? leftPanelHeight : rightPanelHeight;

            for (int i = 0; i < tabs.Count; ++i)
            {
                CFilesWindow* tabPanel = tabs[i];
                if (tabPanel == NULL || tabPanel->HWindow == NULL)
                    continue;
                if (GetParent(tabPanel->HWindow) != HWindow)
                    SetParent(tabPanel->HWindow, HWindow);
                MoveWindow(tabPanel->HWindow, panelX, panelY, panelWidth, panelHeight, FALSE);
                ::SendMessage(tabPanel->HWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(panelWidth, panelHeight));
                tabPanel->LayoutListBoxChilds();
            }
        }

        UpdatePanelTabVisibility(cpsLeft);
        UpdatePanelTabVisibility(cpsRight);
        if (DetachedPanelsSwapFixNeeded)
        {
            DetachedPanelsSwapFixNeeded = FALSE;
            // A single Swap Sides while the right side is detached can leave stale side/host
            // layout state that is refreshed by the user's workaround of doing Swap Sides
            // twice after reattach.  Replay that no-op double swap here so the final side
            // contents stay unchanged but all swap-side layout bookkeeping is rebuilt.
            SendMessage(HWindow, WM_COMMAND, CM_SWAPPANELS, 0);
            SendMessage(HWindow, WM_COMMAND, CM_SWAPPANELS, 0);
        }

        FocusPanel(GetActivePanel());
        Plugins.Event(PLUGINEVENT_TABCHANGED, PANEL_RIGHT);

        if (suppressMainRedraw)
        {
            SendMessage(HWindow, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(HWindow, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        // The detached window is the visual cover over the area by which the
        // main window grows. Leave its last complete surface in place until
        // the expanded main surface has been fully drawn behind it.
        const BOOL detachedWindowCloaked = suppressDetachedRedraw &&
                                           DarkModeSetWindowCloaked(HRightDetachedWindow, true);
        if (suppressDetachedRedraw)
            SendMessage(HRightDetachedWindow, WM_SETREDRAW, TRUE, 0);
        if (HRightDetachedWindow != NULL)
            ShowWindow(HRightDetachedWindow, SW_HIDE);
        if (detachedWindowCloaked)
            DarkModeSetWindowCloaked(HRightDetachedWindow, false);
        return TRUE;
    }
}

BOOL CMainWindow::TogglePanelsDetached()
{
    return SetPanelsDetached(!DetachedPanels);
}

int CMainWindow::ConfirmDetachedTabWindowClose(HWND hWndDetached)
{
    const CDetachedTabInfo* info = FindDetachedTab(hWndDetached);
    if (!Configuration.CnfrmDetachTabClose)
        return info != NULL && info->OriginalSide == cpsRight ? 3 : 2;

    MSGBOXEX_PARAMS params;
    memset(&params, 0, sizeof(params));
    params.HParent = hWndDetached;
    params.Flags = MSGBOXEX_YESNOOKCANCEL | MSGBOXEX_ESCAPEENABLED | MSGBOXEX_ICONQUESTION |
                   MSGBOXEX_SILENT | MSGBOXEX_HINT;
    params.Caption = LoadStr(IDS_DETACHED_TAB_CLOSE_CAPTION);
    params.Text = LoadStr(IDS_DETACHED_TAB_CLOSE_TEXT);
    params.CheckBoxText = LoadStr(IDS_DETACHED_TAB_CLOSE_CHECKBOX);
    BOOL dontShow = !Configuration.CnfrmDetachTabClose;
    params.CheckBoxValue = &dontShow;
    char aliasBtnNames[500];
    sprintf(aliasBtnNames, "%d\t%s\t%d\t%s\t%d\t%s\t%d\t%s",
            DIALOG_YES, LoadStr(IDS_DETACHED_TAB_BTN_CLOSE),
            DIALOG_NO, LoadStr(IDS_DETACHED_TAB_BTN_LEFT),
            DIALOG_OK, LoadStr(IDS_DETACHED_TAB_BTN_RIGHT),
            DIALOG_CANCEL, LoadStr(IDS_BUTTON_CANCEL));
    params.AliasBtnNames = aliasBtnNames;
    int ret = SalMessageBoxEx(&params);
    Configuration.CnfrmDetachTabClose = !dontShow;
    if (ret == DIALOG_YES)
        return 1;
    if (ret == DIALOG_NO)
        return 2;
    if (ret == DIALOG_OK)
        return 3;
    return 0;
}

LRESULT CALLBACK CMainWindow::DetachedTabWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define WM_USER_REFRESH_DETACHED_TAB_DPI (WM_APP + 0x3C3)
    // CreateWindow can erase the client area before GWLP_USERDATA is assigned.
    // Paint that first frame here as well, otherwise a restored extension tab
    // briefly exposes the system's white window background in the dark scheme.
    if (uMsg == WM_ERASEBKGND)
    {
        RECT r;
        GetClientRect(hWnd, &r);
        HBRUSH background = HDialogBrush != NULL ? HDialogBrush : GetSysColorBrush(COLOR_BTNFACE);
        if (DarkModeIsWindowsDarkSchemeSelected())
        {
            HDC hDC = (HDC)wParam;
            COLORREF oldColor = SetDCBrushColor(hDC, DarkModeGetColors().background);
            FillRect(hDC, &r, (HBRUSH)GetStockObject(DC_BRUSH));
            SetDCBrushColor(hDC, oldColor);
        }
        else
            FillRect((HDC)wParam, &r, background);
        return 1;
    }

    CMainWindow* mainWindow = (CMainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (uMsg == WM_CREATE)
        return 0;
    if (mainWindow != NULL)
    {
        CDetachedTabInfo* info = mainWindow->FindDetachedTab(hWnd);
        switch (uMsg)
        {
        case WM_DPICHANGED:
            if (lParam != 0)
            {
                const RECT* suggested = (const RECT*)lParam;
                SetWindowPos(hWnd, NULL, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOACTIVATE | SWP_NOZORDER);
            }
            if (info != NULL && !info->DPIRefreshPosted)
            {
                info->DPIRefreshPosted = TRUE;
                PostMessage(hWnd, WM_USER_REFRESH_DETACHED_TAB_DPI, 0, 0);
            }
            return 0;

        case WM_USER_REFRESH_DETACHED_TAB_DPI:
            if (info != NULL)
            {
                info->DPIRefreshPosted = FALSE;
                mainWindow->RebuildDetachedTabToolbarImageLists((int)WinLibDPIGetWindowDPI(hWnd), hWnd);
                if (info->Panel != NULL && info->Panel->HWindow != NULL)
                {
                    SendMessage(info->Panel->HWindow, WM_DPICHANGED_AFTERPARENT, 0, 0);
                    mainWindow->EnsurePanelRefreshAndRequest(info->Panel, false, true);
                }
            }
            mainWindow->LayoutDetachedTabWindow(hWnd);
            RedrawWindow(hWnd, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
            return 0;

        case WM_SIZE:
            mainWindow->LayoutDetachedTabWindow(hWnd);
            return 0;

        case WM_SETFOCUS:
            if (info != NULL && info->Panel != NULL)
                SetFocus(info->Panel->GetListBoxHWND());
            return 0;

        case WM_MOUSEACTIVATE:
        case WM_ACTIVATE:
            if ((uMsg == WM_MOUSEACTIVATE || LOWORD(wParam) != WA_INACTIVE) &&
                info != NULL && info->Panel != NULL)
            {
                CFilesWindow* oldPanel = mainWindow->GetActivePanel();
                mainWindow->SelectDetachedTab(hWnd);
                mainWindow->SetActivePanel(info->Panel);
                info->Panel->SetPanelSide(info->OriginalSide);
                mainWindow->InvalidateDirectoryLine(oldPanel, FALSE);
                mainWindow->InvalidateDirectoryLine(info->Panel, TRUE);
                mainWindow->RefreshCommandStates();
            }
            return uMsg == WM_MOUSEACTIVATE ? MA_ACTIVATE : 0;

        case WM_COMMAND:
            if (info != NULL && info->Panel != NULL)
            {
                mainWindow->SelectDetachedTab(hWnd);
                mainWindow->SetActivePanel(info->Panel);
                info->Panel->SetPanelSide(info->OriginalSide);
            }
            return SendMessage(mainWindow->HWindow, WM_COMMAND, wParam, lParam);

        case WM_NOTIFY:
            if (info != NULL && info->Panel != NULL)
            {
                mainWindow->SelectDetachedTab(hWnd);
                mainWindow->SetActivePanel(info->Panel);
                info->Panel->SetPanelSide(info->OriginalSide);
            }
            return SendMessage(mainWindow->HWindow, WM_NOTIFY, wParam, lParam);

        case WM_CONTEXTMENU:
            if (info != NULL && info->Panel != NULL)
            {
                mainWindow->SelectDetachedTab(hWnd);
                mainWindow->SetActivePanel(info->Panel);
                info->Panel->SetPanelSide(info->OriginalSide);
            }
            return SendMessage(mainWindow->HWindow, WM_CONTEXTMENU, wParam, lParam);

        case WM_APPCOMMAND:
        {
            DWORD cmd = GET_APPCOMMAND_LPARAM(lParam);
            if (cmd == APPCOMMAND_BROWSER_BACKWARD || cmd == APPCOMMAND_BROWSER_FORWARD)
            {
                if (info != NULL && info->Panel != NULL)
                {
                    mainWindow->SelectDetachedTab(hWnd);
                    mainWindow->SetActivePanel(info->Panel);
                }
                return SendMessage(mainWindow->HWindow, WM_COMMAND,
                                   cmd == APPCOMMAND_BROWSER_BACKWARD ? CM_ACTIVEBACK : CM_ACTIVEFORWARD, 0);
            }
            break;
        }

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) != SC_CLOSE)
                break;
            // fall through to the shared close decision
        case WM_CLOSE:
        {
            CFilesWindow* panel = info != NULL ? info->Panel : NULL;
            int action = mainWindow->ConfirmDetachedTabWindowClose(hWnd);
            if (action == 1)
                mainWindow->CloseDetachedTab(panel);
            else if (action == 2)
            {
                if (mainWindow->ReattachDetachedTab(panel, cpsLeft))
                    mainWindow->SaveDetachedTabConfigNow();
            }
            else if (action == 3)
            {
                if (mainWindow->ReattachDetachedTab(panel, cpsRight))
                    mainWindow->SaveDetachedTabConfigNow();
            }
            return 0;
        }
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
#undef WM_USER_REFRESH_DETACHED_TAB_DPI
}

BOOL CMainWindow::ConfirmDetachedWindowClose(HWND hWndDetached, BOOL* closeSalamander)
{
    if (!Configuration.CnfrmDetachClose)
    {
        *closeSalamander = FALSE;
        return TRUE;
    }
    MSGBOXEX_PARAMS params;
    memset(&params, 0, sizeof(params));
    params.HParent = hWndDetached;
    params.Flags = MSGBOXEX_YESNOCANCEL | MSGBOXEX_ESCAPEENABLED | MSGBOXEX_ICONQUESTION |
                   MSGBOXEX_SILENT | MSGBOXEX_HINT;
    params.Caption = LoadStr(IDS_DETACHED_CLOSE_CAPTION);
    params.Text = LoadStr(IDS_DETACHED_CLOSE_TEXT);
    params.CheckBoxText = LoadStr(IDS_DETACHED_CLOSE_CHECKBOX);
    BOOL dontShow = !Configuration.CnfrmDetachClose;
    params.CheckBoxValue = &dontShow;
    char aliasBtnNames[200];
    sprintf(aliasBtnNames, "%d\t%s\t%d\t%s\t%d\t%s",
            DIALOG_YES, LoadStr(IDS_DETACHED_BTN_CLOSE),
            DIALOG_NO, LoadStr(IDS_DETACHED_BTN_REATTACH),
            DIALOG_CANCEL, LoadStr(IDS_BUTTON_CANCEL));
    params.AliasBtnNames = aliasBtnNames;
    int ret = SalMessageBoxEx(&params);
    Configuration.CnfrmDetachClose = !dontShow;
    if (ret == IDYES)
    {
        *closeSalamander = TRUE;
        return TRUE;
    }
    if (ret == IDNO)
    {
        *closeSalamander = FALSE;
        return TRUE;
    }
    return FALSE; // cancel / escape
}

LRESULT CALLBACK CMainWindow::DetachedPanelWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define WM_USER_REFRESH_DETACHED_DPI (WM_APP + 0x3C2)
    if (uMsg == WM_ERASEBKGND)
    {
        RECT r;
        GetClientRect(hWnd, &r);
        HDC hDC = (HDC)wParam;
        if (DarkModeIsWindowsDarkSchemeSelected())
        {
            COLORREF oldColor = SetDCBrushColor(hDC, DarkModeGetColors().background);
            FillRect(hDC, &r, (HBRUSH)GetStockObject(DC_BRUSH));
            SetDCBrushColor(hDC, oldColor);
        }
        else
            FillRect(hDC, &r, HDialogBrush != NULL ? HDialogBrush : GetSysColorBrush(COLOR_BTNFACE));
        return 1;
    }

    LONG_PTR data = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    CMainWindow* mainWindow = (CMainWindow*)(data & ~(LONG_PTR)1);
    CPanelSide side = (data & 1) ? cpsRight : cpsLeft;

    if (uMsg == WM_CREATE)
        return 0;

    if (mainWindow != NULL)
    {
        switch (uMsg)
        {
        case WM_DPICHANGED:
        {
            if (lParam != 0)
            {
                const RECT* suggested = (const RECT*)lParam;
                SetWindowPos(hWnd, NULL, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOACTIVATE | SWP_NOZORDER);
            }

            // Do not destroy controls/image lists while PMv2 is walking the
            // child hierarchy. Rebuild once, after all AFTERPARENT messages.
            if (side == cpsRight && !mainWindow->DetachedDPIRefreshPosted)
            {
                mainWindow->DetachedDPIRefreshPosted = TRUE;
                PostMessage(hWnd, WM_USER_REFRESH_DETACHED_DPI, 0, 0);
            }
            return 0;
        }

        case WM_USER_REFRESH_DETACHED_DPI:
        {
            mainWindow->DetachedDPIRefreshPosted = FALSE;
            if (side != cpsRight || mainWindow->DetachedDPIRefreshInProgress)
                return 0;

            mainWindow->DetachedDPIRefreshInProgress = TRUE;
            int dpi = (int)WinLibDPIGetWindowDPI(hWnd);

            // Chrome owns native pixel resources (toolbar strips, fonts and
            // rebar band heights). Recreate only this top-level's copies.
            mainWindow->DestroyDetachedChrome();
            mainWindow->RebuildDetachedToolbarImageLists(dpi);
            mainWindow->EnsureDetachedChrome();

            if (mainWindow->RightTabWindow != NULL &&
                mainWindow->RightTabWindow->HWindow != NULL)
            {
                mainWindow->RightTabWindow->RefreshDPIResources();
            }
            for (int i = 0; i < mainWindow->RightPanelTabs.Count; ++i)
            {
                CFilesWindow* panel = mainWindow->RightPanelTabs[i];
                if (panel != NULL && panel->HWindow != NULL)
                    SendMessage(panel->HWindow, WM_DPICHANGED_AFTERPARENT, 0, 0);
            }

            mainWindow->LayoutDetachedPanels();
            RedrawWindow(hWnd, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                             RDW_ALLCHILDREN | RDW_UPDATENOW);
            mainWindow->DetachedDPIRefreshInProgress = FALSE;
            return 0;
        }

        case WM_SIZE:
            mainWindow->LayoutDetachedPanelWindow(side, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_SETFOCUS:
            if (!mainWindow->CreatingDetachedChrome)
            {
                CFilesWindow* oldPanel = mainWindow->GetActivePanel();
                CFilesWindow* newPanel = side == cpsLeft ? mainWindow->LeftPanel : mainWindow->RightPanel;
                mainWindow->SetActivePanel(newPanel);
                mainWindow->InvalidateDirectoryLine(oldPanel, FALSE);
                mainWindow->InvalidateDirectoryLine(newPanel, TRUE);
                mainWindow->RefreshCommandStates();
            }
            return 0;

        case WM_MOUSEACTIVATE:
            if (!mainWindow->CreatingDetachedChrome)
            {
                CFilesWindow* oldPanel = mainWindow->GetActivePanel();
                CFilesWindow* newPanel = side == cpsLeft ? mainWindow->LeftPanel : mainWindow->RightPanel;
                mainWindow->SetActivePanel(newPanel);
                mainWindow->InvalidateDirectoryLine(oldPanel, FALSE);
                mainWindow->InvalidateDirectoryLine(newPanel, TRUE);
                mainWindow->RefreshCommandStates();
            }
            return MA_ACTIVATE;

        case WM_ACTIVATE:
            mainWindow->CaptionIsActive = LOWORD(wParam) != WA_INACTIVE;
            if (mainWindow->CaptionIsActive && !mainWindow->CreatingDetachedChrome)
            {
                CFilesWindow* oldPanel = mainWindow->GetActivePanel();
                CFilesWindow* newPanel = side == cpsLeft ? mainWindow->LeftPanel : mainWindow->RightPanel;
                mainWindow->SetActivePanel(newPanel);
                mainWindow->InvalidateDirectoryLine(oldPanel, FALSE);
                mainWindow->InvalidateDirectoryLine(newPanel, TRUE);
                mainWindow->RefreshCommandStates();
            }
            return 0;

        case WM_COMMAND:
        {
            if (mainWindow->CreatingDetachedChrome)
                return 0;
            WORD command = LOWORD(wParam);
            if (side == cpsRight)
            {
                if (command == CM_ACTIVEBACK)
                    command = CM_RBACK;
                else if (command == CM_ACTIVEFORWARD)
                    command = CM_RFORWARD;
            }
            if (command == LOWORD(wParam))
                mainWindow->SetActivePanel(side == cpsLeft ? mainWindow->LeftPanel : mainWindow->RightPanel);
            else
                wParam = MAKEWPARAM(command, HIWORD(wParam));
            return SendMessage(mainWindow->HWindow, WM_COMMAND, wParam, lParam);
        }

        case WM_NOTIFY:
            if (mainWindow->CreatingDetachedChrome)
                return 0;
            if (side == cpsRight && mainWindow->RightPanel != NULL)
            {
                LPNMHDR hdr = (LPNMHDR)lParam;
                if (hdr != NULL &&
                    !mainWindow->RightPanel->TreeViewDisableNotify &&
                    (hdr->hwndFrom == mainWindow->RightPanel->HTreeView ||
                     hdr->hwndFrom == mainWindow->RightPanel->HTreeHeader ||
                     hdr->hwndFrom == mainWindow->RightPanel->HTreeSplit))
                {
                    mainWindow->SetActivePanel(mainWindow->RightPanel);
                }
            }
            return SendMessage(mainWindow->HWindow, WM_NOTIFY, wParam, lParam);

        case WM_CONTEXTMENU:
            if (mainWindow->CreatingDetachedChrome)
                return 0;
            mainWindow->SetActivePanel(side == cpsLeft ? mainWindow->LeftPanel : mainWindow->RightPanel);
            return SendMessage(mainWindow->HWindow, WM_CONTEXTMENU, wParam, lParam);

        case WM_MOUSEWHEEL:
        {
            POINT screenPt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (mainWindow->TrySwitchPanelTabByMouseWheel(screenPt, wParam))
                return 0;
            break;
        }

        case WM_APPCOMMAND:
        {
            DWORD cmd = GET_APPCOMMAND_LPARAM(lParam);
            if (cmd == APPCOMMAND_BROWSER_BACKWARD || cmd == APPCOMMAND_BROWSER_FORWARD)
            {
                return SendMessage(mainWindow->HWindow, WM_COMMAND,
                                   cmd == APPCOMMAND_BROWSER_BACKWARD ? CM_RBACK : CM_RFORWARD, 0);
            }
            break;
        }

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_CLOSE)
            {
                BOOL closeSalamander = FALSE;
                if (mainWindow->ConfirmDetachedWindowClose(hWnd, &closeSalamander))
                {
                    if (closeSalamander)
                        PostMessage(mainWindow->HWindow, WM_USER_CLOSE_MAINWND, 1, 0);
                    else
                        mainWindow->SetPanelsDetached(FALSE);
                }
                return 0;
            }
            break;

        case WM_CLOSE:
        {
            BOOL closeSalamander = FALSE;
            if (mainWindow->ConfirmDetachedWindowClose(hWnd, &closeSalamander))
            {
                if (closeSalamander)
                    PostMessage(mainWindow->HWindow, WM_USER_CLOSE_MAINWND, 1, 0);
                else
                    mainWindow->SetPanelsDetached(FALSE);
            }
            return 0;
        }
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
#undef WM_USER_REFRESH_DETACHED_DPI
}

void CMainWindow::AddTrayIcon(BOOL updateIcon)
{
    CALL_STACK_MESSAGE1("CMainWindow::AddTrayIcon()");

    NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = HWindow;
    tnid.uID = TASKBAR_ICON_ID;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_USER_ICON_NOTIFY;
    int resID = MainWindowIcons[Configuration.GetMainWindowIconIndex()].IconResID;
    tnid.hIcon = SalLoadIcon(HInstance, resID, IconSizes[ICONSIZE_16]);
    lstrcpyn(tnid.szTip, MAINWINDOW_NAME, sizeof(tnid.szTip));
    Shell_NotifyIcon(updateIcon ? NIM_MODIFY : NIM_ADD, &tnid);
    HANDLES(DestroyIcon(tnid.hIcon));
}

void CMainWindow::RemoveTrayIcon()
{
    CALL_STACK_MESSAGE1("CMainWindow::RemoveTrayIcon()");
    NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = HWindow;
    tnid.uID = TASKBAR_ICON_ID;
    tnid.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &tnid);
}

void CMainWindow::SetTrayIconText(const char* text)
{
    CALL_STACK_MESSAGE1("CMainWindow::SetTrayIconText()");
    if (!Configuration.StatusArea)
    {
        TRACE_E("CMainWindow::SetTrayIconText(): !Configuration.StatusArea");
        return;
    }
    NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = HWindow;
    tnid.uID = TASKBAR_ICON_ID;
    tnid.uFlags = NIF_TIP;
    lstrcpyn(tnid.szTip, text, sizeof(tnid.szTip));
    Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

void CMainWindow::FormatPanelPathForDisplay(CFilesWindow* panel, int mode, char* buffer, int bufferSize)
{
    if (buffer == NULL || bufferSize <= 0)
        return;

    buffer[0] = 0;

    if (panel == NULL)
        return;

    if (mode < TITLE_BAR_MODE_DIRECTORY || mode > TITLE_BAR_MODE_FULLPATH)
        mode = TITLE_BAR_MODE_DIRECTORY;

    CPluginFSInterfaceEncapsulation* pluginFS = NULL;
    BOOL pluginTitleService = FALSE;
    if (panel->Is(ptPluginFS))
    {
        pluginFS = panel->GetPluginFS();
        if (pluginFS != NULL)
            pluginTitleService = pluginFS->IsServiceSupported(FS_SERVICE_GETPATHFORMAINWNDTITLE);

        if (!pluginTitleService && mode != TITLE_BAR_MODE_FULLPATH)
            mode = TITLE_BAR_MODE_FULLPATH;
    }

    static char generalPath[SAL_MAX_PATH];
    generalPath[0] = 0;
    panel->GetGeneralPath(generalPath, _countof(generalPath));

    switch (mode)
    {
    case TITLE_BAR_MODE_COMPOSITE:
    {
        if (pluginTitleService &&
            pluginFS->GetPathForMainWindowTitle(pluginFS->GetPluginFSName(), 2, buffer, bufferSize) && buffer[0] != 0)
        {
            return;
        }

        Utf8SafeCopyWindowTitle(buffer, bufferSize, generalPath);
        if (buffer[0] != 0)
        {
            const char backslash = 0x5C;  // '\\'
            char* trimStart = NULL;
            char* trimEnd = NULL;
            if (panel->Is(ptDisk) || panel->Is(ptZIPArchive))
            {
                static char rootPath[SAL_MAX_PATH];
                GetRootPath(rootPath, buffer);
                int chars = (int)strlen(rootPath);
                trimStart = buffer + chars;
                while (buffer[chars] != 0)
                {
                    if (buffer[chars] == backslash && buffer[chars + 1] != 0)
                        trimEnd = buffer + chars;
                    chars++;
                }
            }
            else if (panel->Is(ptPluginFS) && pluginFS != NULL)
            {
                int chars = 0;
                int pathLen = (int)strlen(buffer);
                if (pluginFS->GetNextDirectoryLineHotPath(buffer, pathLen, chars) && chars < pathLen)
                {
                    trimStart = buffer + chars;
                    int lastChars = chars;
                    while (pluginFS->GetNextDirectoryLineHotPath(buffer, pathLen, chars))
                    {
                        if (chars < pathLen)
                            lastChars = chars;
                        else
                            break;
                    }
                    trimEnd = buffer + lastChars;
                }
            }

            if (trimStart != NULL && trimEnd != NULL && trimEnd > trimStart)
            {
                memmove(trimStart + 3, trimEnd, strlen(trimEnd) + 1);
                memcpy(trimStart, "...", 3);
            }
        }
        break;
    }

    case TITLE_BAR_MODE_DIRECTORY:
    {
        if (pluginTitleService &&
            pluginFS->GetPathForMainWindowTitle(pluginFS->GetPluginFSName(), 1, buffer, bufferSize) && buffer[0] != 0)
        {
            return;
        }

        Utf8SafeCopyWindowTitle(buffer, bufferSize, generalPath);
        if (buffer[0] != 0)
        {
            const char backslash = 0x5C;      // '\\'
            const char forwardSlash = 0x2F;   // '/'
            if (panel->Is(ptDisk) || panel->Is(ptZIPArchive))
            {
                static char rootPath[SAL_MAX_PATH];
                GetRootPath(rootPath, buffer);
                int chars = (int)strlen(rootPath);
                char* p = buffer + strlen(buffer);
                if (*p == backslash)
                    p--;
                while (p > buffer && *p != backslash)
                    p--;
                if (*(p + 1) != 0 && p + 1 >= buffer + chars)
                    memmove(buffer, p + 1, strlen(p + 1) + 1);
            }
            else if (panel->Is(ptPluginFS) && pluginFS != NULL)
            {
                int chars = 0;
                int pathLen = (int)strlen(buffer);
                int lastChars = 0;
                while (pluginFS->GetNextDirectoryLineHotPath(buffer, pathLen, chars))
                {
                    if (chars < pathLen)
                        lastChars = chars;
                    else
                        break;
                }
                if (lastChars > 0)
                {
                    char* p = buffer + lastChars;
                    if (*p == forwardSlash || *p == backslash)
                        p++;
                    memmove(buffer, p, strlen(p) + 1);
                }
            }
        }
        break;
    }

    case TITLE_BAR_MODE_FULLPATH:
    default:
        Utf8SafeCopyWindowTitle(buffer, bufferSize, generalPath);
        break;
    }

    if (buffer[0] == 0)
        Utf8SafeCopyWindowTitle(buffer, bufferSize, generalPath);
}

static void AppendToWindowTitle(char* title, int titleSize, const char* text)
{
    if (title == NULL || titleSize <= 0 || text == NULL)
        return;

    int len = (int)strlen(title);
    if (len >= titleSize - 1)
        return;

    lstrcpyn(title + len, text, titleSize - len);
}

static void AppendUtf8ToWindowTitle(char* title, int titleSize, const char* text)
{
    if (title == NULL || titleSize <= 0 || text == NULL)
        return;

    int len = (int)strlen(title);
    if (len >= titleSize - 1)
        return;

    int copy = min((int)strlen(text), titleSize - len - 1);
    while (copy > 0 && ((unsigned char)text[copy] & 0xC0) == 0x80)
        copy--; // do not cut in the middle of a UTF-8 character
    if (copy <= 0)
        return;

    memcpy(title + len, text, copy);
    title[len + copy] = 0;
}

static int GetUtf8WindowTitlePrevChar(const char* text, int pos)
{
    if (text == NULL || pos <= 0)
        return 0;

    pos--;
    while (pos > 0 && ((unsigned char)text[pos] & 0xC0) == 0x80)
        pos--;
    return pos;
}

static BOOL IsUtf8WindowTitleCombiningMark(const char* text)
{
    if (text == NULL)
        return FALSE;

    const unsigned char* s = (const unsigned char*)text;
    return s[0] == 0xCC || (s[0] == 0xCD && s[1] <= 0xAF);
}

static int GetUtf8WindowTitleSafeCut(const char* text, int maxLen)
{
    if (text == NULL || maxLen <= 0)
        return 0;

    int len = (int)strlen(text);
    if (len <= maxLen)
        return len;

    int cut = maxLen;
    while (cut > 0 && ((unsigned char)text[cut] & 0xC0) == 0x80)
        cut--; // do not cut in the middle of a UTF-8 character
    while (cut > 0 && IsUtf8WindowTitleCombiningMark(text + cut))
        cut = GetUtf8WindowTitlePrevChar(text, cut);
    return cut;
}

static void Utf8SafeCopyWindowTitle(char* target, int targetSize, const char* source)
{
    if (target == NULL || targetSize <= 0)
        return;

    target[0] = 0;
    if (source == NULL)
        return;

    int copy = GetUtf8WindowTitleSafeCut(source, targetSize - 1);
    if (copy <= 0)
        return;

    memcpy(target, source, copy);
    target[copy] = 0;
}

static void TruncateUtf8WindowTitle(char* title, int maxLen)
{
    if (title == NULL || maxLen < 0)
        return;

    int len = (int)strlen(title);
    if (len <= maxLen)
        return;

    title[GetUtf8WindowTitleSafeCut(title, maxLen)] = 0;
}

static void TrimTrailingWindowTitleSpaces(char* title)
{
    if (title == NULL)
        return;

    char* end = title + strlen(title);
    while (end > title && *(end - 1) == ' ')
        *(--end) = 0;
}

static std::wstring MultiByteToWindowTitleWide(const char* text)
{
    if (text == NULL)
        return std::wstring();

    int textLen = (int)strlen(text);
    if (textLen == 0)
        return std::wstring();

    int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, textLen, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (wideLen <= 0)
    {
        codePage = CP_ACP;
        flags = 0;
        wideLen = MultiByteToWideChar(codePage, flags, text, textLen, NULL, 0);
    }
    if (wideLen <= 0)
        return std::wstring();

    std::wstring result(wideLen, L'\0');
    MultiByteToWideChar(codePage, flags, text, textLen, &result[0], wideLen);
    return result;
}

static int GetRootPathLenW(const wchar_t* path)
{
    if (path == NULL)
        return 0;
    if (path[0] == L'\\' && path[1] == L'\\')
    {
        int pos = 2;
        while (path[pos] != 0 && path[pos] != L'\\')
            pos++;
        if (path[pos] != 0)
            pos++;
        while (path[pos] != 0 && path[pos] != L'\\')
            pos++;
        if (path[pos] != 0)
            pos++;
        return pos;
    }
    else
    {
        if (path[0] != 0 && path[1] == L':')
            return 3;
    }
    return 0;
}

static std::wstring FormatPanelPathForDisplayW(CFilesWindow* panel, int mode)
{
    if (panel == NULL)
        return std::wstring();

    if (mode < TITLE_BAR_MODE_DIRECTORY || mode > TITLE_BAR_MODE_FULLPATH)
        mode = TITLE_BAR_MODE_DIRECTORY;

    CPluginFSInterfaceEncapsulation* pluginFS = NULL;
    BOOL pluginTitleService = FALSE;
    if (panel->Is(ptPluginFS))
    {
        pluginFS = panel->GetPluginFS();
        if (pluginFS != NULL)
            pluginTitleService = pluginFS->IsServiceSupported(FS_SERVICE_GETPATHFORMAINWNDTITLE);
        if (!pluginTitleService && mode != TITLE_BAR_MODE_FULLPATH)
            mode = TITLE_BAR_MODE_FULLPATH;
    }

    if (pluginTitleService && (mode == TITLE_BAR_MODE_COMPOSITE || mode == TITLE_BAR_MODE_DIRECTORY))
    {
        static char buf[SAL_MAX_PATH];
        buf[0] = 0;
        int modeForPlugin = (mode == TITLE_BAR_MODE_COMPOSITE) ? 2 : 1;
        if (pluginFS->GetPathForMainWindowTitle(pluginFS->GetPluginFSName(), modeForPlugin, buf, _countof(buf)) && buf[0] != 0)
        {
            std::wstring result = MultiByteToWindowTitleWide(buf);
            if (!result.empty())
                return result;
        }
    }

    if (panel->Is(ptPluginFS))
    {
        static char buffer[4 * SAL_MAX_PATH];
        buffer[0] = 0;
        CMainWindow::FormatPanelPathForDisplay(panel, mode, buffer, _countof(buffer));
        if (buffer[0] != 0)
            return MultiByteToWindowTitleWide(buffer);
        return std::wstring();
    }

    std::wstring pathW;
    if (panel->Is(ptDisk))
    {
        const wchar_t* pw = panel->GetPathW();
        if (pw != NULL && pw[0] != 0)
            pathW = pw;
    }
    else
    {
        static char generalPath[SAL_MAX_PATH];
        generalPath[0] = 0;
        panel->GetGeneralPath(generalPath, _countof(generalPath));
        if (generalPath[0] != 0)
            pathW = MultiByteToWindowTitleWide(generalPath);
    }

    if (pathW.empty())
        return std::wstring();

    switch (mode)
    {
    case TITLE_BAR_MODE_COMPOSITE:
    {
        int rootLen = GetRootPathLenW(pathW.c_str());
        if (rootLen > 0)
        {
            int lastBS = -1;
            for (int i = (int)pathW.length() - 1; i >= rootLen; i--)
            {
                if (pathW[i] == L'\\')
                {
                    lastBS = i;
                    break;
                }
            }
            if (lastBS > rootLen)
                return pathW.substr(0, rootLen) + L"..." + pathW.substr(lastBS);
        }
        return pathW;
    }

    case TITLE_BAR_MODE_DIRECTORY:
    {
        int len = (int)pathW.length();
        if (len > 0 && pathW[len - 1] == L'\\')
            len--;
        for (int i = len - 1; i >= 0; i--)
        {
            if (pathW[i] == L'\\')
            {
                int rootLen = GetRootPathLenW(pathW.c_str());
                if (i + 1 >= rootLen)
                    return pathW.substr(i + 1);
                break;
            }
        }
        return pathW;
    }

    case TITLE_BAR_MODE_FULLPATH:
    default:
        return pathW;
    }
}

static std::wstring BuildWindowTitleForPanel(CFilesWindow* panel, const std::wstring& prefix,
                                             const std::wstring& suffix)
{
    std::wstring path;
    if (Configuration.TitleBarShowPath && panel != NULL)
        path = FormatPanelPathForDisplayW(panel, Configuration.TitleBarMode);
    return BuildMainWindowTitleText(prefix, path, suffix);
}

void CMainWindow::GetFormatedPathForTitle(char* path, int textSize)
{
    if (path == NULL || textSize <= 0)
        return;
    path[0] = 0;
    CFilesWindow* panel = GetActivePanel();
    if (panel == NULL)
    {
        path[0] = 0;
        return;
    }
    FormatPanelPathForDisplay(panel, Configuration.TitleBarMode, path, textSize);
}
void CMainWindow::SetWindowTitle(const char* text)
{
    CALL_STACK_MESSAGE2("CMainWindow::SetWindowTitle(%s)", text != NULL ? text : "");

    std::wstring wideText;
    std::wstring wideAppSuffix;
    std::wstring explicitDetachedText;
    std::wstring prefix;
    CFilesWindow* mainTitlePanel = LeftPanel;
    if (!DetachedPanels)
    {
        CFilesWindow* activePanel = GetActivePanel();
        BOOL activePanelIsInMainWindow = FALSE;
        for (int i = 0; i < LeftPanelTabs.Count && !activePanelIsInMainWindow; ++i)
            activePanelIsInMainWindow = LeftPanelTabs[i] == activePanel;
        for (int i = 0; i < RightPanelTabs.Count && !activePanelIsInMainWindow; ++i)
            activePanelIsInMainWindow = RightPanelTabs[i] == activePanel;
        if (activePanelIsInMainWindow)
            MainWindowTitlePanel = activePanel;

        BOOL rememberedPanelIsInMainWindow = FALSE;
        for (int i = 0; i < LeftPanelTabs.Count && !rememberedPanelIsInMainWindow; ++i)
            rememberedPanelIsInMainWindow = LeftPanelTabs[i] == MainWindowTitlePanel;
        for (int i = 0; i < RightPanelTabs.Count && !rememberedPanelIsInMainWindow; ++i)
            rememberedPanelIsInMainWindow = RightPanelTabs[i] == MainWindowTitlePanel;
        if (rememberedPanelIsInMainWindow)
            mainTitlePanel = MainWindowTitlePanel;
        else
            MainWindowTitlePanel = mainTitlePanel;
    }
    if (text == NULL)
    {
        std::wstring suffix = MultiByteToWindowTitleWide(SALAMANDER_TEXT_VERSION);

        if (RunningAsAdmin)
        {
            suffix += L" (";
            suffix += MultiByteToWindowTitleWide(LoadStr(IDS_AS_ADMIN_TITLE));
            suffix += L")";
        }

#ifdef X64_STRESS_TEST
        suffix += L" ST";
#endif //X64_STRESS_TEST

#ifdef USE_BETA_EXPIRATION_DATE
        suffix += L" - Expires on ";
        char expire[100];
        if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &BETA_EXPIRATION_DATE, NULL, expire, 100) == 0)
            sprintf(expire, "%u.%u.%u", BETA_EXPIRATION_DATE.wDay, BETA_EXPIRATION_DATE.wMonth, BETA_EXPIRATION_DATE.wYear);
        suffix += MultiByteToWindowTitleWide(expire);
#endif // USE_BETA_EXPIRATION_DATE

        while (!suffix.empty() && suffix.back() == L' ')
            suffix.pop_back();

        wideAppSuffix = suffix;

        if (Configuration.UseTitleBarPrefixForced)
        {
            std::wstring wPrefix = MultiByteToWindowTitleWide(Configuration.TitleBarPrefixForced);
            if (!wPrefix.empty())
            {
                prefix = wPrefix;
            }
        }
        else if (Configuration.UseTitleBarPrefix)
        {
            std::wstring wPrefix = MultiByteToWindowTitleWide(Configuration.TitleBarPrefix);
            if (!wPrefix.empty())
            {
                prefix = wPrefix;
            }
        }

        std::wstring mainSuffix = suffix;
        if (DetachedPanels)
        {
            mainSuffix += L" - ";
            mainSuffix += MultiByteToWindowTitleWide(LoadStr(IDS_MAIN_WINDOW_TITLE));
        }
        wideText = BuildWindowTitleForPanel(mainTitlePanel, prefix, mainSuffix);

    }
    else
    {
        wideText = MultiByteToWindowTitleWide(text);
        explicitDetachedText = wideText;
        if (DetachedPanels)
        {
            wideText += L" - ";
            wideText += MultiByteToWindowTitleWide(LoadStr(IDS_MAIN_WINDOW_TITLE));
        }
    }

    std::wstring detachedText;
    if (HRightDetachedWindow != NULL)
    {
        if (text == NULL)
        {
            detachedText = BuildWindowTitleForPanel(DetachedPanels ? RightPanel : mainTitlePanel, prefix, wideAppSuffix);
        }
        else
        {
            detachedText = explicitDetachedText;
        }
        detachedText += L" - ";
        detachedText += MultiByteToWindowTitleWide(LoadStr(IDS_DETACHED_WINDOW_TITLE));
    }

    int curLen = GetWindowTextLengthW(HWindow);
    std::wstring curTitle;
    if (curLen > 0)
    {
        curTitle.resize(curLen + 1);
        int copied = GetWindowTextW(HWindow, &curTitle[0], curLen + 1);
        if (copied >= 0)
            curTitle.resize(copied);
    }

    if (wideText != curTitle)
    {
        ::SetWindowTextW(HWindow, wideText.c_str());
        if (Configuration.StatusArea)
        {
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), (int)wideText.length(), NULL, 0, NULL, NULL);
            if (utf8Len > 0)
            {
                char utf8Buf[4096];
                if (utf8Len < (int)sizeof(utf8Buf))
                {
                    WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), (int)wideText.length(), utf8Buf, utf8Len, NULL, NULL);
                    utf8Buf[utf8Len] = 0;
                    SetTrayIconText(utf8Buf);
                }
            }
        }
    }

    if (HRightDetachedWindow != NULL)
    {
        int detachedCurLen = GetWindowTextLengthW(HRightDetachedWindow);
        std::wstring detachedCurTitle;
        if (detachedCurLen > 0)
        {
            detachedCurTitle.resize(detachedCurLen + 1);
            int copied = GetWindowTextW(HRightDetachedWindow, &detachedCurTitle[0], detachedCurLen + 1);
            if (copied >= 0)
                detachedCurTitle.resize(copied);
        }
        if (detachedText != detachedCurTitle)
            ::SetWindowTextW(HRightDetachedWindow, detachedText.c_str());
    }

    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        CDetachedTabInfo& info = DetachedTabs[i];
        if (info.HWindow == NULL || info.Panel == NULL)
            continue;
        std::wstring detachedTabText;
        if (text == NULL)
            detachedTabText = BuildWindowTitleForPanel(info.Panel, prefix, wideAppSuffix);
        else
            detachedTabText = explicitDetachedText;

        int detachedTabCurLen = GetWindowTextLengthW(info.HWindow);
        std::wstring detachedTabCurTitle;
        if (detachedTabCurLen > 0)
        {
            detachedTabCurTitle.resize(detachedTabCurLen + 1);
            int copied = GetWindowTextW(info.HWindow, &detachedTabCurTitle[0], detachedTabCurLen + 1);
            if (copied >= 0)
                detachedTabCurTitle.resize(copied);
        }
        if (detachedTabText != detachedTabCurTitle)
            ::SetWindowTextW(info.HWindow, detachedTabText.c_str());
    }
}

void CMainWindow::UpdateDetachedMenuLabels()
{
    MENU_ITEM_INFO mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.Mask = MENU_MASK_STRING;

    mii.String = LoadStr(DetachedPanels ? IDS_MENU_MAIN_WINDOW : IDS_MENU_LEFT);
    MainMenu.SetItemInfo(CML_LEFT, FALSE, &mii);

    mii.String = LoadStr(DetachedPanels ? IDS_MENU_DETACHED_WINDOW : IDS_MENU_RIGHT);
    MainMenu.SetItemInfo(CML_RIGHT, FALSE, &mii);

    CMenuBar* bars[] = {MenuBar, DetachedMenuBar};
    for (int i = 0; i < _countof(bars); ++i)
    {
        if (bars[i] != NULL && bars[i]->HWindow != NULL)
        {
            bars[i]->SetFont();
            InvalidateRect(bars[i]->HWindow, NULL, TRUE);
        }
    }
}

void CMainWindow::SetWindowIcon()
{
    int resID = MainWindowIcons[Configuration.GetMainWindowIconIndex()].IconResID;
    // assign the icon to the window
    SendMessage(HWindow, WM_SETICON, ICON_BIG,
                (LPARAM)HANDLES(LoadIcon(HInstance, MAKEINTRESOURCE(resID))));

    if (Configuration.StatusArea)
        AddTrayIcon(TRUE);
}

// btbsCount == empty toolbar
CBottomTBStateEnum VirtKeyStateTable[2][2][2] =
    {
        {
            {btbsNormal, btbsCtrl},
            {btbsAlt, btbsCount},
        },
        {
            {btbsShift, btbsCtrlShift},
            {btbsAltShift, btbsCount},
        }};

void CMainWindow::UpdateBottomToolBar()
{
    if ((BottomToolBar == NULL || BottomToolBar->HWindow == NULL) &&
        (DetachedBottomToolBar == NULL || DetachedBottomToolBar->HWindow == NULL))
        return;

    DWORD shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? 1 : 0;
    DWORD altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0 ? 1 : 0;
    DWORD ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0 ? 1 : 0;

    BOOL updateDetached = DetachedPanels && GetActivePanel() == RightPanel;
    CMenuBar* activeMenuBar = updateDetached ? DetachedMenuBar : MenuBar;

    CBottomTBStateEnum newState = btbsCount;
    if (CaptionIsActive)
    {
        if (activeMenuBar != NULL && activeMenuBar->IsInMenuLoop())
            newState = btbsMenu;
        else
            newState = VirtKeyStateTable[shiftPressed][altPressed][ctrlPressed];
    }
    else
        newState = btbsNormal;

    if (!updateDetached && BottomToolBar != NULL && BottomToolBar->HWindow != NULL)
    {
        BottomToolBar->SetState(newState);
        BottomToolBar->UpdateItemsState();
    }
    if (updateDetached && DetachedBottomToolBar != NULL && DetachedBottomToolBar->HWindow != NULL)
    {
        DetachedBottomToolBar->SetState(newState);
        DetachedBottomToolBar->UpdateItemsState();
    }
}

CMainWindowsHitTestEnum
CMainWindow::HitTest(int xPos, int yPos) // screen coordinates
{
    POINT p;
    p.x = xPos;
    p.y = yPos;
    auto pointInWindow = [](HWND hwnd, POINT pt) -> BOOL {
        if (hwnd == NULL || !IsWindowVisible(hwnd))
            return FALSE;
        RECT wr;
        return (GetWindowRect(hwnd, &wr) && PtInRect(&wr, pt)) ? TRUE : FALSE;
    };

    auto hitRebarBand = [](HWND hRebar, POINT screenPt) {
        POINT rebarPt = screenPt;
        ScreenToClient(hRebar, &rebarPt);
        RBHITTESTINFO hti;
        hti.pt = rebarPt;
        if (SendMessage(hRebar, RB_HITTEST, 0, (LPARAM)&hti) == -1 ||
            hti.flags == RBHT_NOWHERE || hti.iBand == -1)
            return mwhteTopRebar;

        REBARBANDINFO rbi;
        rbi.cbSize = sizeof(rbi);
        rbi.fMask = RBBIM_ID;
        SendMessage(hRebar, RB_GETBANDINFO, hti.iBand, (LPARAM)&rbi);
        switch (rbi.wID)
        {
        case BANDID_MENU:
            return mwhteMenu;
        case BANDID_TOPTOOLBAR:
            return mwhteTopToolbar;
        case BANDID_PLUGINSBAR:
            return mwhtePluginsBar;
        case BANDID_EXTENSIONBAR:
            return mwhteExtensionBar;
        case BANDID_UMTOOLBAR:
            return mwhteUMToolbar;
        case BANDID_HPTOOLBAR:
            return mwhteHPToolbar;
        case BANDID_DRIVEBAR:
        case BANDID_DRIVEBAR2:
            return mwhteDriveBar;
        case BANDID_WORKER:
            return mwhteWorker;
        default:
            TRACE_E("Unknown band in rebar id = " << rbi.wID);
            return mwhteTopRebar;
        }
    };

    if (DetachedPanels && pointInWindow(HRightDetachedWindow, p))
    {
        if (pointInWindow(HDetachedTopRebar, p))
            return hitRebarBand(HDetachedTopRebar, p);
        if (DetachedBottomToolBar != NULL && pointInWindow(DetachedBottomToolBar->HWindow, p))
            return mwhteBottomToolbar;
        if (RightPanel != NULL && pointInWindow(RightPanel->HWindow, p))
        {
            if (RightPanel->DirectoryLine != NULL && pointInWindow(RightPanel->DirectoryLine->HWindow, p))
                return mwhteRightDirLine;
            if (pointInWindow(RightPanel->GetHeaderLineHWND(), p))
                return mwhteRightHeaderLine;
            if (RightPanel->StatusLine != NULL && pointInWindow(RightPanel->StatusLine->HWindow, p))
                return mwhteRightStatusLine;
            return mwhteRightWorkingArea;
        }
        return mwhteNone;
    }

    RECT clientRect;
    RECT r;
    GetClientRect(HWindow, &clientRect);
    r = clientRect;
    MapWindowPoints(HWindow, NULL, (POINT*)&r, 2);

    // if the point is outside the client area, we don't care
    if (!PtInRect(&r, p))
        return mwhteNone;

    CMainWindowsHitTestEnum hit = mwhteNone;

    // find which window owns the point
    ScreenToClient(HWindow, &p);

    // rebar?
    if (PtInChild(HTopRebar, p))
    {
        RBHITTESTINFO hti;
        hti.pt = p;
        if (SendMessage(HTopRebar, RB_HITTEST, 0, (LPARAM)&hti) == -1 ||
            hti.flags == RBHT_NOWHERE || hti.iBand == -1) // perhaps too many tests, but we want to be sure...
        {
            hit = mwhteTopRebar;
        }
        else
        {
            REBARBANDINFO rbi;
            rbi.cbSize = sizeof(rbi);
            rbi.fMask = RBBIM_ID;
            SendMessage(HTopRebar, RB_GETBANDINFO, hti.iBand, (LPARAM)&rbi);
            switch (rbi.wID)
            {
            case BANDID_MENU:
                hit = mwhteMenu;
                break;
            case BANDID_TOPTOOLBAR:
                hit = mwhteTopToolbar;
                break;
            case BANDID_PLUGINSBAR:
                hit = mwhtePluginsBar;
                break;
            case BANDID_EXTENSIONBAR:
                hit = mwhteExtensionBar;
                break;
            case BANDID_UMTOOLBAR:
                hit = mwhteUMToolbar;
                break;
            case BANDID_HPTOOLBAR:
                hit = mwhteHPToolbar;
                break;
            case BANDID_DRIVEBAR:
                hit = mwhteDriveBar;
                break;
            case BANDID_DRIVEBAR2:
                hit = mwhteDriveBar;
                break;
            case BANDID_WORKER:
                hit = mwhteWorker;
                break;
            default:
            {
                TRACE_E("Unknown band in rebar id = " << rbi.wID);
                hit = mwhteTopRebar;
            }
            }
        }
    }

    // middle toolbar?
    if (hit == mwhteNone)
    {
        if (PtInChild(MiddleToolBar->HWindow, p))
            hit = mwhteMiddleToolbar;
    }

    // command line?
    if (hit == mwhteNone)
    {
        if (PtInChild(EditWindow->HWindow, p))
            hit = mwhteCmdLine;
    }

    // bottom toolbar?
    if (hit == mwhteNone)
    {
        if (PtInChild(((CWindow*)BottomToolBar)->HWindow, p))
            hit = mwhteBottomToolbar;
    }

    // split line?
    if (hit == mwhteNone)
    {
        GetSplitRect(r);
        if (PtInRect(&r, p))
            hit = mwhteSplitLine;
    }

    // left panel?
    if (hit == mwhteNone)
    {
        if (PtInChild(LeftPanel->HWindow, p))
        {
            if (PtInChild(LeftPanel->DirectoryLine->HWindow, p))
                hit = mwhteLeftDirLine;
            else if (PtInChild(LeftPanel->GetHeaderLineHWND(), p))
                hit = mwhteLeftHeaderLine;
            else if (PtInChild(LeftPanel->StatusLine->HWindow, p))
                hit = mwhteLeftStatusLine;
            else
                hit = mwhteLeftWorkingArea;
        }
    }

    // right panel?
    if (hit == mwhteNone)
    {
        if (PtInChild(RightPanel->HWindow, p))
        {
            if (PtInChild(RightPanel->DirectoryLine->HWindow, p))
                hit = mwhteRightDirLine;
            else if (PtInChild(RightPanel->GetHeaderLineHWND(), p))
                hit = mwhteRightHeaderLine;
            else if (PtInChild(RightPanel->StatusLine->HWindow, p))
                hit = mwhteRightStatusLine;
            else
                hit = mwhteRightWorkingArea;
        }
    }

    return hit;
}

void CMainWindow::OnWmContextMenu(HWND hWnd, int xPos, int yPos)
{
    CALL_STACK_MESSAGE3("CMainWindow::OnWmContextMenu(, %d, %d)", xPos, yPos);

    if (xPos != -1 || yPos != -1)
    {
        POINT screenPt = {xPos, yPos};
        if (ShouldSuppressPanelTabMouseWheelContextMenu(screenPt))
            return;

        auto pointInWindow = [](HWND hwnd, POINT pt) {
            if (hwnd == NULL || !IsWindowVisible(hwnd))
                return false;
            RECT rect;
            return GetWindowRect(hwnd, &rect) && PtInRect(&rect, pt) != FALSE;
        };

        CTabWindow* tabWnd = NULL;
        CPanelSide tabSide = cpsLeft;
        if (LeftTabWindow != NULL && pointInWindow(LeftTabWindow->HWindow, screenPt))
        {
            tabWnd = LeftTabWindow;
            tabSide = cpsLeft;
        }
        else if (RightTabWindow != NULL && pointInWindow(RightTabWindow->HWindow, screenPt))
        {
            tabWnd = RightTabWindow;
            tabSide = cpsRight;
        }

        if (tabWnd != NULL)
        {
            POINT client = screenPt;
            ScreenToClient(tabWnd->HWindow, &client);
            int tabHit = tabWnd->HitTest(client);
            if (tabHit < 0)
            {
                OnPanelTabNewTabAreaContextMenu(tabSide, screenPt);
                return;
            }
            return;
        }
    }

    CMainWindowsHitTestEnum hit = HitTest(xPos, yPos);

    if (hit == mwhteNone)
        return;

    BOOL mainClass = (hit == mwhteTopRebar || hit == mwhteMenu || hit == mwhteTopToolbar ||
                      hit == mwhteUMToolbar || hit == mwhteDriveBar || hit == mwhteCmdLine ||
                      hit == mwhteBottomToolbar || hit == mwhteMiddleToolbar ||
                      hit == mwhteHPToolbar || hit == mwhtePluginsBar ||
                      hit == mwhteExtensionBar);
    BOOL leftPanel = (hit == mwhteLeftDirLine || hit == mwhteLeftHeaderLine ||
                      hit == mwhteLeftStatusLine);
    BOOL panelClass = (leftPanel || hit == mwhteRightDirLine || hit == mwhteRightHeaderLine ||
                       hit == mwhteRightStatusLine);

    // create the menu
    CMenuPopup menu;

    menu.SetImageList(HGrayToolBarImageList, TRUE);
    menu.SetHotImageList(HHotToolBarImageList, TRUE);

    // prepare a separator item
    MENU_ITEM_INFO miiSep;
    miiSep.Mask = MENU_MASK_TYPE;
    miiSep.Type = MENU_TYPE_SEPARATOR;

    MENU_ITEM_INFO mii;
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STATE | MENU_MASK_STRING | MENU_MASK_SUBMENU | MENU_MASK_IMAGEINDEX;
    mii.Type = MENU_TYPE_STRING;
    mii.State = 0;
    mii.ImageIndex = -1;
    mii.SubMenu = NULL;

    // fill it
    if (hit == mwhteSplitLine)
    {
        char buff[20];
        int i;
        for (i = 2; i < 9; i++)
        {
            sprintf(buff, "&%d0 / %d0", i, 10 - i);
            mii.ID = i;
            mii.State = i == 5 ? MENU_STATE_DEFAULT : 0;
            mii.String = buff;
            menu.InsertItem(0xffffffff, TRUE, &mii);
        }
    }

    if (mainClass)
    {
        /* used by the export_mnu.py script that generates salmenu.mnu for Translator;
           keep synchronized with the InsertItem() call below...
MENU_TEMPLATE_ITEM ToolbarsCtxMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_TOPTOOLBAR
  {MNTT_IT, IDS_PLUGINSBAR
  {MNTT_IT, IDS_EXTENSIONBAR
  {MNTT_IT, IDS_UMTOOLBAR
  {MNTT_IT, IDS_HPTOOLBAR
  {MNTT_IT, IDS_DRIVEBAR
  {MNTT_IT, IDS_DRIVEBAR2
  {MNTT_IT, IDS_TABBEDPANELS
  {MNTT_IT, IDS_MIDDLETOOLBAR
  {MNTT_IT, IDS_COMMANDLINE
  {MNTT_IT, IDS_MENU_OPT_VSB_TREE
  {MNTT_IT, IDS_BOTTOMTOOLBAR
  {MNTT_IT, IDS_GRIPSINTOOLBAR
  {MNTT_IT, IDS_SHOWLABELS
  {MNTT_IT, IDS_CUSTOMIZE
  {MNTT_PE, 0
};
*/

        mii.String = LoadStr(IDS_TOPTOOLBAR);
        mii.ID = 1;
        mii.State = TopToolBar->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_PLUGINSBAR);
        mii.ID = 12;
        mii.State = PluginsBar->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_EXTENSIONBAR);
        mii.ID = 15;
        mii.State = ExtensionBar->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_UMTOOLBAR);
        mii.ID = 2;
        mii.State = UMToolBar->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_HPTOOLBAR);
        mii.ID = 11;
        mii.State = HPToolBar->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_DRIVEBAR);
        mii.ID = 3;
        mii.State = (DriveBar->HWindow != NULL && DriveBar2->HWindow == NULL) ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_DRIVEBAR2);
        mii.ID = 4;
        mii.State = (DriveBar->HWindow != NULL && DriveBar2->HWindow != NULL) ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_TABBEDPANELS);
        mii.ID = 14;
        mii.State = Configuration.UsePanelTabs ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_MIDDLETOOLBAR);
        mii.ID = 10;
        mii.State = (MiddleToolBar->HWindow != NULL) ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_COMMANDLINE);
        mii.ID = 5;
        mii.State = EditPermanentVisible ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_MENU_OPT_VSB_TREE);
        mii.ID = 13;
        mii.State = Configuration.TreeViewVisible ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_BOTTOMTOOLBAR);
        mii.ID = 6;
        mii.State = ((CWindow*)BottomToolBar)->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        menu.InsertItem(0xffffffff, TRUE, &miiSep);

        mii.String = LoadStr(IDS_GRIPSINTOOLBAR);
        mii.ID = 9;
        mii.State = Configuration.GripsVisible ? 0 : MENU_STATE_CHECKED;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_SHOWLABELS);
        mii.ID = 8;
        mii.State = Configuration.UserMenuToolbarLabels ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        if (hit == mwhteTopToolbar || hit == mwhteUMToolbar || hit == mwhteHPToolbar ||
            hit == mwhteMiddleToolbar || hit == mwhtePluginsBar ||
            hit == mwhteExtensionBar)
        {
            menu.InsertItem(0xffffffff, TRUE, &miiSep);

            mii.String = LoadStr(IDS_CUSTOMIZE);
            mii.ID = 7;
            mii.State = 0;
            menu.InsertItem(0xffffffff, TRUE, &mii);
        }
    }

    char HotText[2 * MAX_PATH];
    int HeaderLineItem = -1; // will be filled with the item index if the user clicked on one

    if (panelClass)
    {
        CFilesWindow* panel = leftPanel ? LeftPanel : RightPanel;

        /* used by the export_mnu.py script that generates salmenu.mnu for Translator;
           keep synchronized with the InsertItem() call below...
MENU_TEMPLATE_ITEM DirLineHeaderLineMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_HDR_ELASTIC
  {MNTT_IT, IDS_HDR_SMARTMODE
  {MNTT_IT, IDS_MENU_LEFT_VIEW
  {MNTT_IT, IDS_DIRECTORYLINE
  {MNTT_IT, IDS_HEADERLINE
  {MNTT_IT, IDS_INFORMATIONLINE
  {MNTT_IT, IDS_CUSTOMIZEPANEL
  {MNTT_PE, 0
};
MENU_TEMPLATE_ITEM DirLinePathMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_CHANGEDIRECTORY
  {MNTT_IT, IDS_SETHOTPATH
  {MNTT_IT, IDS_COPYTOCLIPBOARD
  {MNTT_IT, IDS_DIRECTORYLINE
  {MNTT_IT, IDS_HEADERLINE
  {MNTT_IT, IDS_INFORMATIONLINE
  {MNTT_IT, IDS_CUSTOMIZEPANEL
  {MNTT_PE, 0
};
MENU_TEMPLATE_ITEM InfoLineMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_COPYTOCLIPBOARD
  {MNTT_IT, IDS_DIRECTORYLINE
  {MNTT_IT, IDS_HEADERLINE
  {MNTT_IT, IDS_INFORMATIONLINE
  {MNTT_IT, IDS_CUSTOMIZEPANEL
  {MNTT_PE, 0
};
*/

        if (hit == mwhteLeftHeaderLine || hit == mwhteRightHeaderLine)
        {
            CHeaderLine* hdrLine = panel->GetHeaderLine();
            if (hdrLine != NULL)
            {
                // find out over which item of the header line the point is located
                POINT hdrP;
                hdrP.x = xPos;
                hdrP.y = yPos;
                ScreenToClient(hdrLine->HWindow, &hdrP);
                int index;
                BOOL extInName;
                CHeaderHitTestEnum ht = hdrLine->HitTest(hdrP.x, hdrP.y, index, extInName);
                if (index >= 0 && (ht == hhtItem || ht == hhtDivider))
                {
                    HeaderLineItem = index;
                    CColumn* column = &panel->Columns[index];

                    mii.String = LoadStr(IDS_HDR_ELASTIC);
                    mii.ID = 1;
                    mii.State = column->FixedWidth ? 0 : MENU_STATE_CHECKED;
                    menu.InsertItem(0xffffffff, TRUE, &mii);

                    if (index == 0 /* Name column */)
                    {
                        mii.String = LoadStr(IDS_HDR_SMARTMODE);
                        mii.ID = 17;
                        mii.State = GetSmartColumnMode(panel) ? MENU_STATE_CHECKED : 0;
                        menu.InsertItem(0xffffffff, TRUE, &mii);
                    }
                }
            }

            mii.String = LoadStr(IDS_MENU_LEFT_VIEW);
            mii.ID = 2;
            mii.State = 0;
            menu.InsertItem(0xffffffff, TRUE, &mii);

            menu.InsertItem(0xffffffff, TRUE, &miiSep);

            /*
      char modiBuff[200];
      strcpy(modiBuff, LoadStr(IDS_HDR_ELASTIC));
      mii.String = modiBuff;
      mii.ID = 1;
      mii.State = Configuration.ElasticMode ? MENU_STATE_CHECKED : 0;
      menu.InsertItem(0xffffffff, TRUE, &mii);
      menu.InsertItem(0xffffffff, TRUE, &miiSep);
      strcpy(modiBuff, LoadStr(IDS_HDR_EXTENSION));
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_STRING | 
                 (Configuration.ShowExtension ? MF_CHECKED : 0),
                 2, modiBuff);
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_STRING | 
                 (Configuration.ShowDosName ? MF_CHECKED : 0),
                 3, LoadStr(IDS_HDR_DOS));
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_STRING | 
                 (Configuration.ShowSize ? MF_CHECKED : 0),
                 4, LoadStr(IDS_HDR_SIZE));
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_STRING | 
                 (Configuration.ShowDate ? MF_CHECKED : 0),
                 5, LoadStr(IDS_HDR_DATE));
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_STRING | 
                 (Configuration.ShowTime ? MF_CHECKED : 0),
                 6, LoadStr(IDS_HDR_TIME));
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_STRING | 
                 (Configuration.ShowAttr ? MF_CHECKED : 0),
                 7, LoadStr(IDS_HDR_ATTR));
      InsertMenu(hMenu, 0xffffffff, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
*/
        }

        // handle the hot path
        if (hit == mwhteLeftDirLine || hit == mwhteRightDirLine)
        {
            mii.String = LoadStr(IDS_CHANGEDIRECTORY);
            mii.ID = 16;
            mii.State = 0;
            mii.ImageIndex = IDX_TB_CHANGE_DIR;
            menu.InsertItem(0xffffffff, TRUE, &mii);
            mii.ImageIndex = -1;

            menu.InsertItem(0xffffffff, TRUE, &miiSep);

            panel->DirectoryLine->GetHotText(HotText, _countof(HotText));
            if (strlen(HotText) > 0)
            {
                CMenuPopup* popup = new CMenuPopup();
                if (popup != NULL)
                {
                    HotPaths.FillHotPathsMenu(popup, 20, TRUE, FALSE, FALSE, FALSE, TRUE);
                    mii.SubMenu = popup;
                    mii.String = LoadStr(IDS_SETHOTPATH);
                    mii.ID = 0;
                    mii.State = 0;
                    menu.InsertItem(0xffffffff, TRUE, &mii);
                    mii.SubMenu = NULL;

                    mii.String = LoadStr(IDS_COPYTOCLIPBOARD);
                    mii.ID = 8;
                    mii.State = 0;
                    menu.InsertItem(0xffffffff, TRUE, &mii);

                    menu.InsertItem(0xffffffff, TRUE, &miiSep);
                }
            }
        }

        // handle hot text in the info line
        if (hit == mwhteLeftStatusLine || hit == mwhteRightStatusLine)
        {
            panel->StatusLine->GetHotText(HotText, _countof(HotText));
            if (strlen(HotText) > 0)
            {
                mii.String = LoadStr(IDS_COPYTOCLIPBOARD);
                mii.ID = 9;
                mii.State = 0;
                menu.InsertItem(0xffffffff, TRUE, &mii);

                menu.InsertItem(0xffffffff, TRUE, &miiSep);
            }
        }

        mii.String = LoadStr(IDS_DIRECTORYLINE);
        mii.ID = 11;
        mii.State = panel->DirectoryLine->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_HEADERLINE);
        mii.ID = 12;
        mii.State = (panel->GetViewMode() == vmDetailed ? 0 : (MENU_STATE_GRAYED)) |
                    (panel->GetViewMode() == vmDetailed && panel->HeaderLineVisible ? MENU_STATE_CHECKED : 0);
        menu.InsertItem(0xffffffff, TRUE, &mii);

        mii.String = LoadStr(IDS_INFORMATIONLINE);
        mii.ID = 13;
        mii.State = panel->StatusLine->HWindow != NULL ? MENU_STATE_CHECKED : 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);

        menu.InsertItem(0xffffffff, TRUE, &miiSep);

        mii.String = LoadStr(IDS_CUSTOMIZEPANEL);
        mii.ID = 15;
        mii.State = 0;
        menu.InsertItem(0xffffffff, TRUE, &mii);
    }

    // open the menu
    HWND hTrackWnd = HWindow;
    if (DetachedPanels && HRightDetachedWindow != NULL)
    {
        POINT pt = {xPos, yPos};
        RECT wr;
        if (GetWindowRect(HRightDetachedWindow, &wr) && PtInRect(&wr, pt))
            hTrackWnd = HRightDetachedWindow;
    }
    int cmd = menu.Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON,
                         xPos, yPos, hTrackWnd, NULL);
    if (cmd == 0)
        return;

    // evaluate the result
    if (hit == mwhteSplitLine)
    {
        double visibleLeftRatio = (double)cmd / 10;
        KeepSplitPositionCenteredOnVisiblePanes = cmd == 5;
        SplitPosition = GetSplitPositionForVisibleLeftPanelRatio(visibleLeftRatio);
        LayoutWindows();
        return;
    }

    if (mainClass)
    {
        int cm = 0;
        switch (cmd)
        {
        case 1:
            cm = CM_TOGGLETOPTOOLBAR;
            break;
        case 2:
            cm = CM_TOGGLEUSERMENUTOOLBAR;
            break;
        case 3:
            cm = CM_TOGGLEDRIVEBAR;
            break;
        case 4:
            cm = CM_TOGGLEDRIVEBAR2;
            break;
        case 5:
            cm = CM_TOGGLEEDITLINE;
            break;
        case 6:
            cm = CM_TOGGLEBOTTOMTOOLBAR;
            break;
        case 8:
            cm = CM_TOGGLE_UMLABELS;
            break;
        case 9:
            cm = CM_TOGGLE_GRIPS;
            break;
        case 10:
            cm = CM_TOGGLEMIDDLETOOLBAR;
            break;
        case 11:
            cm = CM_TOGGLEHOTPATHSBAR;
            break;
        case 12:
            cm = CM_TOGGLEPLUGINSBAR;
            break;
        case 13:
            cm = CM_TOGGLETREEVIEW;
            break;
        case 14:
            cm = CM_TOGGLEPANELTABS;
            break;
        case 15:
            cm = CM_TOGGLEEXTENSIONBAR;
            break;
        }
        if (cm != 0)
        {
            PostMessage(HWindow, WM_COMMAND, MAKEWPARAM(cm, 0), 0);
            return;
        }
        if (cmd == 7)
        {
            switch (hit)
            {
            case mwhteTopToolbar:
            {
                PostMessage(MainWindow->HWindow, WM_COMMAND, CM_CUSTOMIZETOP, 0);
                break;
            }

            case mwhtePluginsBar:
            {
                PostMessage(MainWindow->HWindow, WM_COMMAND, CM_CUSTOMIZEPLUGINS, 0);
                break;
            }

            case mwhteExtensionBar:
            {
                PostMessage(MainWindow->HWindow, WM_COMMAND, CM_PLUGINS, 0);
                break;
            }

            case mwhteMiddleToolbar:
            {
                PostMessage(MainWindow->HWindow, WM_COMMAND, CM_CUSTOMIZEMIDDLE, 0);
                break;
            }

            case mwhteUMToolbar:
            {
                // open the UserMenu page and edit the given index
                PostMessage(HWindow, WM_USER_CONFIGURATION, 2, 0);
                break;
            }

            case mwhteHPToolbar:
            {
                // open the HotPaths page
                PostMessage(HWindow, WM_USER_CONFIGURATION, 1, -1);
                break;
            }
            }
            return;
        }
    }

    if (panelClass)
    {
        CFilesWindow* panel = leftPanel ? LeftPanel : RightPanel;
        int cm = 0;
        switch (cmd)
        {
        case 1:
        {
            CColumn* column = &panel->Columns[HeaderLineItem];
            if (column->ID != COLUMN_ID_CUSTOM)
            {
                CColumnConfig* colCfg = &panel->ViewTemplate->Columns[column->ID - 1];
                if (leftPanel)
                    colCfg->LeftFixedWidth = column->FixedWidth ? 0 : 1;
                else
                    colCfg->RightFixedWidth = column->FixedWidth ? 0 : 1;
                if (column->ID == COLUMN_ID_NAME)
                {
                    if (leftPanel)
                    {
                        if (colCfg->LeftFixedWidth)
                            colCfg->LeftWidth = panel->GetResidualColumnWidth();
                        else
                            panel->ViewTemplate->LeftSmartMode = FALSE;
                    }
                    else
                    {
                        if (colCfg->RightFixedWidth)
                            colCfg->RightWidth = panel->GetResidualColumnWidth();
                        else
                            panel->ViewTemplate->RightSmartMode = FALSE;
                    }
                }
                else
                {
                    if (leftPanel)
                        colCfg->LeftWidth = column->Width;
                    else
                        colCfg->RightWidth = column->Width;
                }
            }
            else if (column->GetText == InternalGetExplorerColumn &&
                     column->CustomData < EXPLORER_COLUMNS_COUNT)
            {
                CColumnConfig* colCfg = &panel->ViewTemplate->ExplorerColumns[column->CustomData];
                if (leftPanel)
                {
                    colCfg->LeftFixedWidth = column->FixedWidth ? 0 : 1;
                    colCfg->LeftWidth = column->Width;
                }
                else
                {
                    colCfg->RightFixedWidth = column->FixedWidth ? 0 : 1;
                    colCfg->RightWidth = column->Width;
                }
            }
            else
            {
                if (panel->PluginData.NotEmpty()) // "always true"
                    panel->PluginData.ColumnFixedWidthShouldChange(leftPanel, column, column->FixedWidth ? 0 : 1);
            }
            // user changed something in the view configuration - let’s rebuild the columns
            if (leftPanel)
                LeftPanel->SelectViewTemplate(LeftPanel->GetViewTemplateIndex(), TRUE, FALSE);
            else
                RightPanel->SelectViewTemplate(RightPanel->GetViewTemplateIndex(), TRUE, FALSE);
            break;
        }

        case 2:
        {
            PostMessage(HWindow, WM_USER_CONFIGURATION, 4,
                        (leftPanel ? LeftPanel : RightPanel)->GetViewTemplateIndex());
            break;
        }

            /*
      case 2: Configuration.ShowExtension = !Configuration.ShowExtension; break;
      case 3: Configuration.ShowDosName = !Configuration.ShowDosName; break;
      case 4: Configuration.ShowSize = !Configuration.ShowSize; break;
      case 5: Configuration.ShowDate = !Configuration.ShowDate; break;
      case 6: Configuration.ShowTime = !Configuration.ShowTime; break;
      case 7: Configuration.ShowAttr = !Configuration.ShowAttr; break;
*/
        case 8:
        {
            CopyTextToClipboard(HotText);
            panel->DirectoryLine->FlashText(TRUE);
        }
        break;
        case 9:
        {
            CopyTextToClipboard(HotText);
            panel->StatusLine->FlashText(TRUE);
        }
        break;
        case 11:
            cm = leftPanel ? CM_LEFTDIRLINE : CM_RIGHTDIRLINE;
            break;
        case 12:
            cm = leftPanel ? CM_LEFTHEADER : CM_RIGHTHEADER;
            break;
        case 13:
            cm = leftPanel ? CM_LEFTSTATUS : CM_RIGHTSTATUS;
            break;
        case 15:
            cm = leftPanel ? CM_CUSTOMIZELEFT : CM_CUSTOMIZERIGHT;
            break;
        case 16:
            cm = leftPanel ? CM_LEFT_CHANGEDIR : CM_RIGHT_CHANGEDIR;
            break;
        case 17:
            cm = leftPanel ? CM_LEFT_SMARTMODE : CM_RIGHT_SMARTMODE;
            break;
        }

        // catch hot paths
        if (cmd >= 20 && cmd < 50)
        {
            SetUnescapedHotPath(cmd - 20, HotText);
            if (!Configuration.HotPathAutoConfig)
                panel->DirectoryLine->FlashText(TRUE);
        }

        if (cm != 0)
        {
            PostMessage(HWindow, WM_COMMAND, MAKEWPARAM(cm, 0), 0);
            return;
        }
    }
}

static DWORD CheckerViewMode = 0xFFFFFFFF;
static DWORD CheckerLeftViewMode = 0xFFFFFFFF;
static DWORD CheckerRightViewMode = 0xFFFFFFFF;
static DWORD CheckerSortType = 0xFFFFFFFF;
static DWORD CheckerLeftSortType = 0xFFFFFFFF;
static DWORD CheckerRightSortType = 0xFFFFFFFF;
static BOOL CheckerHelpMode = 0xFFFFFFFF;
static BOOL CheckerSmartMode = 0xFFFFFFFF;
static BOOL CheckerLeftSmartMode = 0xFFFFFFFF;
static BOOL CheckerRightSmartMode = 0xFFFFFFFF;

void CMainWindow_RefreshCommandStates(CMainWindow* obj)
{
    IdleRefreshStates = FALSE; // clear the control variable

    //---  obtain state values for enabling
    BOOL file = FALSE;                               // cursor on a file
    BOOL subDir = FALSE;                             // cursor on a subdirectory
    BOOL files = FALSE;                              // cursor on a file or directory or a selection
    BOOL linkOnDisk = FALSE;                         // disks only: cursor on a link (file or directory with FILE_ATTRIBUTE_REPARSE_POINT attribute)
    BOOL containsFile = FALSE;                       // cursor (or selection) contains files
    BOOL containsDir = FALSE;                        // cursor (or selection) only on directories
    BOOL compress = FALSE;                           // supports file-based compression?
    BOOL encrypt = FALSE;                            // supports file-based encryption?
    BOOL acls = FALSE;                               // supports ACLs? (NTFS disks)
    BOOL archive = FALSE;                            // is the panel an archive?
    BOOL targetArchive = FALSE;                      // is the other panel an archive?
    BOOL archiveEdit = FALSE;                        // is the panel an editable archive?
    BOOL upDir = FALSE;                              // presence of ".."
    BOOL leftUpDir = FALSE;                          // presence of ".."
    BOOL rightUpDir = FALSE;                         // presence of ".."
    BOOL rootDir = FALSE;                            // TRUE = we are not yet at root
    BOOL leftRootDir = FALSE;                        // TRUE = we are not yet at root
    BOOL rightRootDir = FALSE;                       // TRUE = we are not yet at root
    BOOL hasForward = FALSE;                         // forward possible (path history)
    BOOL hasBackward = FALSE;                        // backward possible (path history)
    BOOL leftHasForward = FALSE;                     // forward possible in left panel (path history)
    BOOL leftHasBackward = FALSE;                    // backward possible in left panel (path history)
    BOOL rightHasForward = FALSE;                    // forward possible in right panel (path history)
    BOOL rightHasBackward = FALSE;                   // backward possible in right panel (path history)
    BOOL pasteFiles = EnablerPasteFiles;             // is Edit/Paste allowed? (cut or copied files)
    BOOL pastePath = EnablerPastePath;               // is Edit/Paste allowed? (path text)
    BOOL pasteLinks = EnablerPasteLinks;             // is Edit/Paste Shortcuts allowed? (copied files)
    BOOL pasteSimpleFiles = EnablerPasteSimpleFiles; // are clipboard files/directories from a single path? (allows Paste into archive or FS)
    DWORD pasteDefEffect = EnablerPasteDefEffect;    // what is the default paste effect, can be a combination of DROPEFFECT_COPY+DROPEFFECT_MOVE (Copy or Cut?)
    BOOL pasteFilesToArcOrFS = FALSE;                // can we paste files into archive/FS in the active panel?
    BOOL onDisk = FALSE;                             // is the panel on a disk?
    BOOL customizeLeftView = FALSE;                  // can columns be configured for the left panel?
    BOOL customizeRightView = FALSE;                 // can columns be configured for the right panel?
    BOOL validPluginFS = FALSE;                      // is the panel a FS with an initialized PluginFS interface?
    DWORD viewMode = 0;                              // panel display mode (tree/brief/detailed/...)
    DWORD leftViewMode = 0;
    DWORD rightViewMode = 0;
    DWORD sortType = 0;
    DWORD leftSortType = 0;
    DWORD rightSortType = 0;
    BOOL existPrevSel = FALSE;
    BOOL existNextSel = FALSE;

    BOOL dirHistory = FALSE; // is there a directory available in directory history?
    BOOL smartMode = FALSE;
    BOOL leftSmartMode = FALSE;
    BOOL rightSmartMode = FALSE;

    BOOL newTab = FALSE;
    BOOL closeTab = FALSE;
    BOOL nextTab = FALSE;
    BOOL prevTab = FALSE;
    BOOL duplicateTab = FALSE;
    BOOL leftNewTab = FALSE;
    BOOL leftCloseTab = FALSE;
    BOOL leftNextTab = FALSE;
    BOOL leftPrevTab = FALSE;
    BOOL leftDuplicateTabSame = FALSE;
    BOOL rightNewTab = FALSE;
    BOOL rightCloseTab = FALSE;
    BOOL rightNextTab = FALSE;
    BOOL rightPrevTab = FALSE;
    BOOL rightDuplicateTabSame = FALSE;
    BOOL leftCloseAllButDefault = FALSE;
    BOOL leftCloseAllExceptThisAndDefault = FALSE;
    BOOL rightCloseAllButDefault = FALSE;
    BOOL rightCloseAllExceptThisAndDefault = FALSE;
    BOOL leftDuplicateTab = FALSE;
    BOOL leftMoveTab = FALSE;
    BOOL rightDuplicateTab = FALSE;
    BOOL rightMoveTab = FALSE;
    BOOL reopenTab = FALSE;
    BOOL lockTab = FALSE;
    BOOL unlockTab = FALSE;
    BOOL leftReopenTab = FALSE;
    BOOL leftLockTab = FALSE;
    BOOL leftUnlockTab = FALSE;
    BOOL rightReopenTab = FALSE;
    BOOL rightLockTab = FALSE;
    BOOL rightUnlockTab = FALSE;

    int selCount = 0;
    int unselCount = 0;

    CFilesWindow* activePanel = obj->GetActivePanel();
    CFilesWindow* nonActivePanel = obj->GetNonActivePanel();
    if (activePanel != NULL && nonActivePanel != NULL && obj->LeftPanel != NULL && obj->RightPanel != NULL)
    {
        hasForward = activePanel->PathHistory->HasForward();
        hasBackward = activePanel->PathHistory->HasBackward();
        leftHasForward = obj->LeftPanel->PathHistory->HasForward();
        leftHasBackward = obj->LeftPanel->PathHistory->HasBackward();
        rightHasForward = obj->RightPanel->PathHistory->HasForward();
        rightHasBackward = obj->RightPanel->PathHistory->HasBackward();
        compress = activePanel->FileBasedCompression;
        acls = activePanel->SupportACLS;
        encrypt = activePanel->FileBasedEncryption;
        archive = activePanel->Is(ptZIPArchive);
        targetArchive = nonActivePanel->Is(ptZIPArchive);
        selCount = activePanel->GetSelCount();
        onDisk = activePanel->Is(ptDisk);
        dirHistory = obj->HasDirHistory(activePanel);
        sortType = activePanel->SortType;
        leftSortType = obj->LeftPanel->SortType;
        rightSortType = obj->RightPanel->SortType;
        validPluginFS = activePanel->Is(ptPluginFS) && activePanel->GetPluginFS()->NotEmpty();
        smartMode = obj->GetSmartColumnMode(activePanel);
        leftSmartMode = obj->GetSmartColumnMode(obj->LeftPanel);
        rightSmartMode = obj->GetSmartColumnMode(obj->RightPanel);

        CPanelSide activeSide = activePanel->GetPanelSide();
        int activeCount = obj->GetPanelTabCount(activeSide);
        int activeIndex = obj->GetPanelTabIndex(activeSide, activePanel);
        bool activeLocked = activePanel->IsTabLocked();
        newTab = TRUE;
        closeTab = (activeIndex > 0 && !activeLocked);
        nextTab = (activeCount > 1);
        prevTab = (activeCount > 1);
        duplicateTab = (activeIndex >= 0);
        reopenTab = obj->HasClosedTab(activeSide);
        lockTab = (activeIndex > 0 && !activeLocked);
        unlockTab = (activeIndex > 0 && activeLocked);

        int leftCount = obj->GetPanelTabCount(cpsLeft);
        leftNewTab = (leftCount > 0);
        CFilesWindow* leftPanel = obj->LeftPanel;
        int leftIndex = obj->GetPanelTabIndex(cpsLeft, leftPanel);
        bool leftLocked = (leftPanel != NULL && leftPanel->IsTabLocked());
        leftCloseTab = (leftIndex > 0 && !leftLocked);
        leftNextTab = (leftCount > 1);
        leftPrevTab = (leftCount > 1);
        leftDuplicateTabSame = (leftIndex >= 0);
        bool leftHasClosable = FALSE;
        bool leftHasClosableExcludingCurrent = FALSE;
        for (int i = 1; i < leftCount; ++i)
        {
            CFilesWindow* tabPanel = obj->GetPanelTabAt(cpsLeft, i);
            if (tabPanel != NULL && !tabPanel->IsTabLocked())
            {
                leftHasClosable = TRUE;
                if (i != leftIndex)
                    leftHasClosableExcludingCurrent = TRUE;
            }
        }
        leftCloseAllButDefault = leftHasClosable;
        leftCloseAllExceptThisAndDefault = (leftIndex <= 0) ? leftHasClosable : leftHasClosableExcludingCurrent;
        leftDuplicateTab = (leftIndex >= 0);
        leftMoveTab = (leftIndex > 0 && !leftLocked);
        leftReopenTab = obj->HasClosedTab(cpsLeft);
        leftLockTab = (leftIndex > 0 && !leftLocked);
        leftUnlockTab = (leftIndex > 0 && leftLocked);

        int rightCount = obj->GetPanelTabCount(cpsRight);
        rightNewTab = (rightCount > 0);
        CFilesWindow* rightPanel = obj->RightPanel;
        int rightIndex = obj->GetPanelTabIndex(cpsRight, rightPanel);
        bool rightLocked = (rightPanel != NULL && rightPanel->IsTabLocked());
        rightCloseTab = (rightIndex > 0 && !rightLocked);
        rightNextTab = (rightCount > 1);
        rightPrevTab = (rightCount > 1);
        rightDuplicateTabSame = (rightIndex >= 0);
        bool rightHasClosable = FALSE;
        bool rightHasClosableExcludingCurrent = FALSE;
        for (int i = 1; i < rightCount; ++i)
        {
            CFilesWindow* tabPanel = obj->GetPanelTabAt(cpsRight, i);
            if (tabPanel != NULL && !tabPanel->IsTabLocked())
            {
                rightHasClosable = TRUE;
                if (i != rightIndex)
                    rightHasClosableExcludingCurrent = TRUE;
            }
        }
        rightCloseAllButDefault = rightHasClosable;
        rightCloseAllExceptThisAndDefault = (rightIndex <= 0) ? rightHasClosable : rightHasClosableExcludingCurrent;
        rightDuplicateTab = (rightIndex >= 0);
        rightMoveTab = (rightIndex > 0 && !rightLocked);
        rightReopenTab = obj->HasClosedTab(cpsRight);
        rightLockTab = (rightIndex > 0 && !rightLocked);
        rightUnlockTab = (rightIndex > 0 && rightLocked);

        if (!Configuration.UsePanelTabs)
        {
            newTab = FALSE;
            closeTab = FALSE;
            nextTab = FALSE;
            prevTab = FALSE;
            duplicateTab = FALSE;
            leftNewTab = FALSE;
            leftCloseTab = FALSE;
            leftNextTab = FALSE;
            leftPrevTab = FALSE;
            leftDuplicateTabSame = FALSE;
            leftCloseAllButDefault = FALSE;
            leftCloseAllExceptThisAndDefault = FALSE;
            rightNewTab = FALSE;
            rightCloseTab = FALSE;
            rightNextTab = FALSE;
            rightPrevTab = FALSE;
            rightDuplicateTabSame = FALSE;
            rightCloseAllButDefault = FALSE;
            rightCloseAllExceptThisAndDefault = FALSE;
            leftDuplicateTab = FALSE;
            leftMoveTab = FALSE;
            rightDuplicateTab = FALSE;
            rightMoveTab = FALSE;
            reopenTab = FALSE;
            lockTab = FALSE;
            unlockTab = FALSE;
            leftReopenTab = FALSE;
            leftLockTab = FALSE;
            leftUnlockTab = FALSE;
            rightReopenTab = FALSE;
            rightLockTab = FALSE;
            rightUnlockTab = FALSE;
        }

        if (archive)
        {
            int format = PackerFormatConfig.PackIsArchive(activePanel->GetZIPArchive());
            if (format != 0) // we found a supported archive
            {
                archiveEdit = PackerFormatConfig.GetUsePacker(format - 1); // does it have an edit?
            }
        }

        upDir = (activePanel->Dirs->Count != 0 &&
                 strcmp(activePanel->Dirs->At(0).Name, "..") == 0);
        leftUpDir = (obj->LeftPanel->Dirs->Count != 0 &&
                     strcmp(obj->LeftPanel->Dirs->At(0).Name, "..") == 0);
        rightUpDir = (obj->RightPanel->Dirs->Count != 0 &&
                      strcmp(obj->RightPanel->Dirs->At(0).Name, "..") == 0);
        if (!leftUpDir)
            leftRootDir = FALSE; // we are already at root (no up-dir exists)
        else
            leftRootDir = TRUE; //!obj->LeftPanel->Is(ptDisk) || !IsUNCRootPath(obj->LeftPanel->GetPath());
        if (!rightUpDir)
            rightRootDir = FALSE; // we are already at root (no up-dir exists)
        else
            rightRootDir = TRUE; //!obj->RightPanel->Is(ptDisk) || !IsUNCRootPath(obj->RightPanel->GetPath());
        rootDir = activePanel == obj->LeftPanel ? leftRootDir : rightRootDir;

        unselCount = activePanel->Dirs->Count + activePanel->Files->Count - selCount;
        if (upDir)
            unselCount--; // do not count up-dir as an unselected directory

        viewMode = activePanel->GetViewTemplateIndex();
        leftViewMode = obj->LeftPanel->GetViewTemplateIndex();
        rightViewMode = obj->RightPanel->GetViewTemplateIndex();

        customizeLeftView = leftViewMode > 1;
        customizeRightView = rightViewMode > 1;

        int caret = activePanel->GetCaretIndex();
        if (caret >= 0)
        {
            if (caret == 0)
            {
                if (!upDir)
                    files = (activePanel->Dirs->Count + activePanel->Files->Count > 0);
                else
                {
                    int count = activePanel->GetSelCount();
                    if (count == 1)
                    {
                        files = (activePanel->GetSel(0) == FALSE);
                    }
                    else
                        files = (count > 0);
                }
            }
            else
                files = TRUE;
            file = files && caret >= activePanel->Dirs->Count;
            subDir = files && caret < activePanel->Dirs->Count && (caret != 0 || !upDir);

            containsFile = activePanel->SelectionContainsFile();
            containsDir = activePanel->SelectionContainsDirectory();

            int dummyIndex;
            existPrevSel = activePanel->SelectFindNext(caret, FALSE, TRUE, dummyIndex);
            existNextSel = activePanel->SelectFindNext(caret, TRUE, TRUE, dummyIndex);

            if (onDisk && (file || subDir))
            {
                if (caret < activePanel->Dirs->Count)
                    linkOnDisk = (activePanel->Dirs->At(caret).Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
                else
                    linkOnDisk = (activePanel->Files->At(caret - activePanel->Dirs->Count).Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            }
        }

        if (IdleCheckClipboard)
        {
            IdleCheckClipboard = FALSE; // clear the control variable
            pasteFiles = activePanel->ClipboardPaste(FALSE, TRUE);
            pastePath = activePanel->IsTextOnClipboard();
            pasteLinks = activePanel->ClipboardPasteLinks(TRUE);
            pasteSimpleFiles = activePanel->ClipboardPasteToArcOrFS(TRUE, &pasteDefEffect);
        }

        // compute the value of pasteFilesToArcOrFS
        if (pasteSimpleFiles)
        {
            if (archiveEdit)
                pasteFilesToArcOrFS = (pasteDefEffect & (DROPEFFECT_COPY | DROPEFFECT_MOVE)) != 0;
            else
            {
                if (validPluginFS)
                {
                    if ((pasteDefEffect & DROPEFFECT_COPY) &&
                            activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_COPYFROMDISKTOFS) ||
                        (pasteDefEffect & DROPEFFECT_MOVE) &&
                            activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_MOVEFROMDISKTOFS))
                    {
                        pasteFilesToArcOrFS = TRUE;
                    }
                }
            }
        }
    }

    // transfer results to global variables
    // if any of them changes, obj->IdleStatesChanged variable will be set
    obj->CheckAndSet(&EnablerUpDir, upDir);
    obj->CheckAndSet(&EnablerRootDir, rootDir);
    obj->CheckAndSet(&EnablerForward, hasForward);
    obj->CheckAndSet(&EnablerBackward, hasBackward);
    obj->CheckAndSet(&EnablerFiles, files);
    obj->CheckAndSet(&EnablerFileOnDisk, file && onDisk);
    obj->CheckAndSet(&EnablerFileOrDirLinkOnDisk, (file || linkOnDisk) && onDisk);
    obj->CheckAndSet(&EnablerFileOnDiskOrArchive, file && (onDisk || archive));
    obj->CheckAndSet(&EnablerFilesOnDisk, onDisk && files);
    obj->CheckAndSet(&EnablerFilesOnDiskOrArchive, (onDisk || archive) && files);
    obj->CheckAndSet(&EnablerOccupiedSpace, (onDisk || archive && (activePanel->ValidFileData & VALID_DATA_SIZE)) && files);
    obj->CheckAndSet(&EnablerFilesOnDiskCompress, onDisk && compress && files);
    obj->CheckAndSet(&EnablerFilesOnDiskEncrypt, onDisk && encrypt && files);
    obj->CheckAndSet(&EnablerFileDir, (file || subDir));
    obj->CheckAndSet(&EnablerFileDirANDSelected, (file || subDir) && selCount > 0);
    obj->CheckAndSet(&EnablerOnDisk, onDisk);
    obj->CheckAndSet(&EnablerCalcDirSizes, (onDisk || archive && (activePanel->ValidFileData & VALID_DATA_SIZE)));
    obj->CheckAndSet(&EnablerPasteFiles, pasteFiles);                   // store clipboard state for the next call to RefreshCommandStates()
    obj->CheckAndSet(&EnablerPastePath, pastePath);                     // store clipboard state for the next call to RefreshCommandStates()
    obj->CheckAndSet(&EnablerPasteLinks, pasteLinks);                   // store clipboard state for the next call to RefreshCommandStates()
    obj->CheckAndSet(&EnablerPasteSimpleFiles, pasteSimpleFiles);       // store clipboard state for the next call to RefreshCommandStates()
    obj->CheckAndSet(&EnablerPasteDefEffect, pasteDefEffect);           // store clipboard state for the next call to RefreshCommandStates()
    obj->CheckAndSet(&EnablerPasteFilesToArcOrFS, pasteFilesToArcOrFS); // store state distinguishing "Paste" and "Paste (Change Directory)"
    obj->CheckAndSet(&EnablerPaste, (onDisk && pasteFiles || pasteFilesToArcOrFS || pastePath));
    obj->CheckAndSet(&EnablerPasteLinksOnDisk, onDisk && pasteLinks);
    obj->CheckAndSet(&EnablerSelected, selCount > 0);
    obj->CheckAndSet(&EnablerUnselected, unselCount > 0);
    obj->CheckAndSet(&EnablerHiddenNames, activePanel->HiddenNames.GetCount() > 0);
    obj->CheckAndSet(&EnablerSelectionStored, activePanel->OldSelection.GetCount() > 0);
    obj->CheckAndSet(&EnablerGlobalSelStored, (GlobalSelection.GetCount() > 0 || pastePath));
    obj->CheckAndSet(&EnablerSelGotoPrev, existPrevSel);
    obj->CheckAndSet(&EnablerSelGotoNext, existNextSel);
    obj->CheckAndSet(&EnablerLeftUpDir, leftUpDir);
    obj->CheckAndSet(&EnablerRightUpDir, rightUpDir);
    obj->CheckAndSet(&EnablerLeftRootDir, leftRootDir);
    obj->CheckAndSet(&EnablerRightRootDir, rightRootDir);
    obj->CheckAndSet(&EnablerLeftForward, leftHasForward);
    obj->CheckAndSet(&EnablerRightForward, rightHasForward);
    obj->CheckAndSet(&EnablerLeftBackward, leftHasBackward);
    obj->CheckAndSet(&EnablerRightBackward, rightHasBackward);
    obj->CheckAndSet(&EnablerFileHistory, obj->FileHistory->HasItem());
    obj->CheckAndSet(&EnablerDirHistory, dirHistory);
    obj->CheckAndSet(&EnablerCustomizeLeftView, customizeLeftView);
    obj->CheckAndSet(&EnablerCustomizeRightView, customizeRightView);
    obj->CheckAndSet(&EnablerDriveInfo, (onDisk || archive ||
                                         validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_SHOWINFO)));
    obj->CheckAndSet(&EnablerQuickRename, (file || subDir) &&
                                              (onDisk ||
                                               validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_QUICKRENAME)));
    obj->CheckAndSet(&EnablerCreateDir, (onDisk ||
                                         validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_CREATEDIR)));
    obj->CheckAndSet(&EnablerViewFile, file &&
                                           (onDisk || archive ||
                                            validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_VIEWFILE)));
    obj->CheckAndSet(&EnablerFilesDelete, files &&
                                              (onDisk || archiveEdit ||
                                               validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_DELETE)));
    obj->CheckAndSet(&EnablerFilesCopy, files &&
                                            (onDisk || archive ||
                                             validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_COPYFROMFS)));
    obj->CheckAndSet(&EnablerFilesMove, files &&
                                            (onDisk ||
                                             validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_MOVEFROMFS)));
    obj->CheckAndSet(&EnablerChangeAttrs, files &&
                                              (onDisk ||
                                               validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_CHANGEATTRS)));
    obj->CheckAndSet(&EnablerShowProperties, files &&
                                                 (onDisk ||
                                                  validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_SHOWPROPERTIES)));
    obj->CheckAndSet(&EnablerItemsContextMenu, files &&
                                                   (onDisk ||
                                                    validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_CONTEXTMENU)));
    obj->CheckAndSet(&EnablerOpenActiveFolder, (onDisk ||
                                                validPluginFS && activePanel->GetPluginFS()->IsServiceSupported(FS_SERVICE_OPENACTIVEFOLDER)));
    obj->CheckAndSet(&EnablerPermissions, onDisk && files && acls &&
                                              ((containsFile && !containsDir) || (!containsFile && containsDir)));
    obj->CheckAndSet(&EnablerNewTab, newTab);
    obj->CheckAndSet(&EnablerCloseTab, closeTab);
    obj->CheckAndSet(&EnablerNextTab, nextTab);
    obj->CheckAndSet(&EnablerPrevTab, prevTab);
    obj->CheckAndSet(&EnablerDuplicateTab, duplicateTab);
    obj->CheckAndSet(&EnablerReopenTab, reopenTab);
    obj->CheckAndSet(&EnablerLockTab, lockTab);
    obj->CheckAndSet(&EnablerUnlockTab, unlockTab);
    obj->CheckAndSet(&EnablerLeftNewTab, leftNewTab);
    obj->CheckAndSet(&EnablerLeftCloseTab, leftCloseTab);
    obj->CheckAndSet(&EnablerLeftNextTab, leftNextTab);
    obj->CheckAndSet(&EnablerLeftPrevTab, leftPrevTab);
    obj->CheckAndSet(&EnablerLeftDuplicateTab, leftDuplicateTabSame);
    obj->CheckAndSet(&EnablerLeftCloseAllButDefault, leftCloseAllButDefault);
    obj->CheckAndSet(&EnablerLeftCloseAllExceptThisAndDefault, leftCloseAllExceptThisAndDefault);
    obj->CheckAndSet(&EnablerLeftDuplicateTabToRight, leftDuplicateTab);
    obj->CheckAndSet(&EnablerLeftMoveTabToRight, leftMoveTab);
    obj->CheckAndSet(&EnablerLeftReopenTab, leftReopenTab);
    obj->CheckAndSet(&EnablerLeftLockTab, leftLockTab);
    obj->CheckAndSet(&EnablerLeftUnlockTab, leftUnlockTab);
    obj->CheckAndSet(&EnablerRightNewTab, rightNewTab);
    obj->CheckAndSet(&EnablerRightCloseTab, rightCloseTab);
    obj->CheckAndSet(&EnablerRightNextTab, rightNextTab);
    obj->CheckAndSet(&EnablerRightPrevTab, rightPrevTab);
    obj->CheckAndSet(&EnablerRightDuplicateTab, rightDuplicateTabSame);
    obj->CheckAndSet(&EnablerRightCloseAllButDefault, rightCloseAllButDefault);
    obj->CheckAndSet(&EnablerRightCloseAllExceptThisAndDefault, rightCloseAllExceptThisAndDefault);
    obj->CheckAndSet(&EnablerRightDuplicateTabToLeft, rightDuplicateTab);
    obj->CheckAndSet(&EnablerRightMoveTabToLeft, rightMoveTab);
    obj->CheckAndSet(&EnablerRightReopenTab, rightReopenTab);
    obj->CheckAndSet(&EnablerRightLockTab, rightLockTab);
    obj->CheckAndSet(&EnablerRightUnlockTab, rightUnlockTab);

    if (obj->IdleStatesChanged || IdleForceRefresh)
    {
        // if any variables changed or the cache was cleared
        // via IdleForceRefresh, let visible toolbars fetch new data
        if (obj->TopToolBar != NULL && obj->TopToolBar->HWindow != NULL)
            obj->TopToolBar->UpdateItemsState();
        if (obj->MiddleToolBar != NULL && obj->MiddleToolBar->HWindow != NULL)
            obj->MiddleToolBar->UpdateItemsState();
        if (obj->LeftPanel->DirectoryLine->ToolBar != NULL &&
            obj->LeftPanel->DirectoryLine->ToolBar->HWindow != NULL)
            obj->LeftPanel->DirectoryLine->ToolBar->UpdateItemsState();
        if (obj->RightPanel->DirectoryLine->ToolBar != NULL &&
            obj->RightPanel->DirectoryLine->ToolBar->HWindow != NULL)
            obj->RightPanel->DirectoryLine->ToolBar->UpdateItemsState();
        BOOL updateDetachedBottom = obj->DetachedPanels && obj->GetActivePanel() == obj->RightPanel;
        if (!updateDetachedBottom && obj->BottomToolBar != NULL && obj->BottomToolBar->HWindow != NULL)
            obj->BottomToolBar->UpdateItemsState();
        if (obj->UMToolBar != NULL && obj->UMToolBar->HWindow != NULL)
            obj->UMToolBar->UpdateItemsState();
        if (obj->DetachedTopToolBar != NULL && obj->DetachedTopToolBar->HWindow != NULL)
            obj->DetachedTopToolBar->UpdateItemsState();
        if (updateDetachedBottom && obj->DetachedBottomToolBar != NULL && obj->DetachedBottomToolBar->HWindow != NULL)
            obj->DetachedBottomToolBar->UpdateItemsState();
        if (obj->DetachedUMToolBar != NULL && obj->DetachedUMToolBar->HWindow != NULL)
            obj->DetachedUMToolBar->UpdateItemsState();
    }

    CToolBar* topToolbar = obj->TopToolBar;
    CToolBar* midToolbar = obj->MiddleToolBar;
    if (obj->HelpMode != CheckerHelpMode || IdleForceRefresh)
    {
        CheckerHelpMode = obj->HelpMode;
        if (topToolbar != NULL && topToolbar->HWindow != NULL)
            topToolbar->CheckItem(CM_HELP_CONTEXT, FALSE, obj->HelpMode == HELP_ACTIVE);
        if (midToolbar != NULL && midToolbar->HWindow != NULL)
            midToolbar->CheckItem(CM_HELP_CONTEXT, FALSE, obj->HelpMode == HELP_ACTIVE);
    }
    if (viewMode != CheckerViewMode || IdleForceRefresh)
    {
        CheckerViewMode = viewMode;
        if (topToolbar != NULL && topToolbar->HWindow != NULL)
        {
            topToolbar->CheckItem(CM_ACTIVEMODE_2, FALSE, CheckerViewMode == 1);
            topToolbar->CheckItem(CM_ACTIVEMODE_3, FALSE, CheckerViewMode == 2);
        }
        if (midToolbar != NULL && midToolbar->HWindow != NULL)
        {
            midToolbar->CheckItem(CM_ACTIVEMODE_2, FALSE, CheckerViewMode == 1);
            midToolbar->CheckItem(CM_ACTIVEMODE_3, FALSE, CheckerViewMode == 2);
        }
    }
    if (sortType != CheckerSortType || IdleForceRefresh)
    {
        CheckerSortType = sortType;
        if (topToolbar != NULL && topToolbar->HWindow != NULL)
        {
            topToolbar->CheckItem(CM_ACTIVENAME, FALSE, CheckerSortType == stName);
            topToolbar->CheckItem(CM_ACTIVEEXT, FALSE, CheckerSortType == stExtension);
            topToolbar->CheckItem(CM_ACTIVETIME, FALSE, CheckerSortType == stTime);
            topToolbar->CheckItem(CM_ACTIVESIZE, FALSE, CheckerSortType == stSize);
        }
        if (midToolbar != NULL && midToolbar->HWindow != NULL)
        {
            midToolbar->CheckItem(CM_ACTIVENAME, FALSE, CheckerSortType == stName);
            midToolbar->CheckItem(CM_ACTIVEEXT, FALSE, CheckerSortType == stExtension);
            midToolbar->CheckItem(CM_ACTIVETIME, FALSE, CheckerSortType == stTime);
            midToolbar->CheckItem(CM_ACTIVESIZE, FALSE, CheckerSortType == stSize);
        }
    }
    if (smartMode != CheckerSmartMode || IdleForceRefresh)
    {
        CheckerSmartMode = smartMode;
        if (topToolbar != NULL && topToolbar->HWindow != NULL)
        {
            topToolbar->CheckItem(CM_ACTIVE_SMARTMODE, FALSE, CheckerSmartMode == TRUE);
        }
        if (midToolbar != NULL && midToolbar->HWindow != NULL)
        {
            midToolbar->CheckItem(CM_ACTIVE_SMARTMODE, FALSE, CheckerSmartMode == TRUE);
        }
    }

    CToolBar* toolbar = obj->LeftPanel->DirectoryLine->ToolBar;
    if (toolbar != NULL && toolbar->HWindow != NULL)
    {
        if (leftViewMode != CheckerLeftViewMode || IdleForceRefresh)
        {
            CheckerLeftViewMode = leftViewMode;
            toolbar->CheckItem(CM_LEFTMODE_2, FALSE, CheckerLeftViewMode == 1);
            toolbar->CheckItem(CM_LEFTMODE_3, FALSE, CheckerLeftViewMode == 2);
        }

        if (leftSortType != CheckerLeftSortType || IdleForceRefresh)
        {
            CheckerLeftSortType = leftSortType;
            toolbar->CheckItem(CM_LEFTNAME, FALSE, CheckerLeftSortType == stName);
            toolbar->CheckItem(CM_LEFTEXT, FALSE, CheckerLeftSortType == stExtension);
            toolbar->CheckItem(CM_LEFTTIME, FALSE, CheckerLeftSortType == stTime);
            toolbar->CheckItem(CM_LEFTSIZE, FALSE, CheckerLeftSortType == stSize);
        }

        if (leftSmartMode != CheckerLeftSmartMode || IdleForceRefresh)
        {
            CheckerLeftSmartMode = leftSmartMode;
            toolbar->CheckItem(CM_LEFT_SMARTMODE, FALSE, CheckerLeftSmartMode == TRUE);
        }
    }

    toolbar = obj->RightPanel->DirectoryLine->ToolBar;
    if (toolbar != NULL && toolbar->HWindow != NULL)
    {
        if (rightViewMode != CheckerRightViewMode || IdleForceRefresh)
        {
            CheckerRightViewMode = rightViewMode;
            toolbar->CheckItem(CM_RIGHTMODE_2, FALSE, CheckerRightViewMode == 1);
            toolbar->CheckItem(CM_RIGHTMODE_3, FALSE, CheckerRightViewMode == 2);
        }

        if (rightSortType != CheckerRightSortType || IdleForceRefresh)
        {
            CheckerRightSortType = rightSortType;
            toolbar->CheckItem(CM_RIGHTNAME, FALSE, CheckerRightSortType == stName);
            toolbar->CheckItem(CM_RIGHTEXT, FALSE, CheckerRightSortType == stExtension);
            toolbar->CheckItem(CM_RIGHTTIME, FALSE, CheckerRightSortType == stTime);
            toolbar->CheckItem(CM_RIGHTSIZE, FALSE, CheckerRightSortType == stSize);
        }

        if (rightSmartMode != CheckerRightSmartMode || IdleForceRefresh)
        {
            CheckerRightSmartMode = rightSmartMode;
            toolbar->CheckItem(CM_RIGHT_SMARTMODE, FALSE, CheckerRightSmartMode == TRUE);
        }
    }
    obj->IdleStatesChanged = FALSE;
    IdleForceRefresh = FALSE;
}

void CMainWindow::RefreshCommandStates()
{
    CMainWindow_RefreshCommandStates(this); // this hack exists because we can't obtain the object's method address (as a plain function we can)
}

void CMainWindow::OnEnterIdle()
{
    CALL_STACK_MESSAGE3("CMainWindow::OnEnterIdle(R:%d, C:%d)", IdleRefreshStates, IdleCheckClipboard);

    if (DisableIdleProcessing)
        return;

    // the main window is surely fully started now, otherwise idle wouldn't occur
    FirstActivateApp = FALSE;

    // check if someone requested a state update
    if (IdleRefreshStates)
        RefreshCommandStates();

    // update plugin bar and drives bar if needed
    if (!SalamanderBusy && PluginsStatesChanged)
    {
        if (PluginsBar != NULL && PluginsBar->HWindow != NULL)
            PluginsBar->CreatePluginButtons();

        CDriveBar* copyDrivesListFrom = NULL;
        if (DriveBar != NULL && DriveBar->HWindow != NULL)
        {
            DriveBar->RebuildDrives(DriveBar); // no need for slow drive enumeration
            copyDrivesListFrom = DriveBar;
        }
        if (DriveBar2 != NULL && DriveBar2->HWindow != NULL)
            DriveBar2->RebuildDrives(copyDrivesListFrom);

        PluginsStatesChanged = FALSE;
    }

    // make sure the bottom toolbar reflects the current state (if it doesn't already)
    UpdateBottomToolBar();

    // if panels scrolled or resized, store the new array of visible items
    if (LeftPanel != NULL)
        LeftPanel->RefreshVisibleItemsArray();
    if (RightPanel != NULL)
        RightPanel->RefreshVisibleItemsArray();
}

void CMainWindow::OnColorsChanged(BOOL reloadUMIcons)
{
    DarkModeApplyTree(HWindow);
    DarkModeRefreshTitleBar(HWindow);

    // screen colors or color depth changed; new imagelists have been created
    // for the toolbars and must be attached to the controls that use them

    // top toolbar
    if (TopToolBar != NULL)
    {
        TopToolBar->SetImageList(HGrayToolBarImageList);
        TopToolBar->SetHotImageList(HHotToolBarImageList);
        TopToolBar->OnColorsChanged();
    }

    // middle toolbar
    if (MiddleToolBar != NULL)
    {
        MiddleToolBar->SetImageList(HGrayToolBarImageList);
        MiddleToolBar->SetHotImageList(HHotToolBarImageList);
        MiddleToolBar->OnColorsChanged();
    }

    // plugin bar
    if (PluginsBar != NULL)
    {
        PluginsBar->OnColorsChanged();
    }
    if (ExtensionBar != NULL)
    {
        if (ExtensionBar->HWindow != NULL)
            ExtensionBar->CreateExtensionButtons(HGrayToolBarImageList,
                                                 HHotToolBarImageList);
        ExtensionBar->OnColorsChanged();
    }

    // user menu toolbar
    if (UMToolBar != NULL)
    {
        UMToolBar->OnColorsChanged(); // new CacheBitmap
        if (reloadUMIcons)
        {
            CUserMenuIconDataArr* bkgndReaderData = new CUserMenuIconDataArr();
            // Theoretically, the user menu might be open in Find and WM_COLORCHANGE could arrive,
            // which would trash the icons under its feet. In practice, this should not happen, so we'll ignore it.
            // If solved later: also handle the case where the configuration dialog is open and colors change;
            // after closing the dialog with OK (new user menu version) the icons must be reloaded
            for (int i = 0; i < UserMenuItems->Count; i++)
                UserMenuItems->At(i)->GetIconHandle(bkgndReaderData, FALSE);
            UserMenuIconBkgndReader.StartBkgndReadingIcons(bkgndReaderData); // NOTE: releases 'bkgndReaderData'
        }
        UMToolBar->CreateButtons(); // group icon handle changed
    }

    // hot path bar
    if (HPToolBar != NULL)
    {
        HPToolBar->OnColorsChanged(); // new CacheBitmap
        HPToolBar->CreateButtons();   // HFavoritIcon handle has changed
    }

    // drive bars
    CDriveBar* copyDrivesListFrom = NULL;
    if (DriveBar != NULL)
    {
        DriveBar->OnColorsChanged(); // new CacheBitmap
        DriveBar->RebuildDrives();   // load new icons
        copyDrivesListFrom = DriveBar;
    }
    if (DriveBar2 != NULL)
    {
        DriveBar2->OnColorsChanged();                 // new CacheBitmap
        DriveBar2->RebuildDrives(copyDrivesListFrom); // load new icons
    }

    // bottom toolbar
    if (BottomToolBar != NULL)
    {
        BottomToolBar->SetImageList(HBottomTBImageList);
        BottomToolBar->SetHotImageList(HHotBottomTBImageList);
        BottomToolBar->OnColorsChanged();
    }

    UpdateRebarVisuals();
    if (HRightDetachedWindow != NULL)
    {
        DarkModeApplyTree(HRightDetachedWindow);
        DarkModeRefreshTitleBar(HRightDetachedWindow);
    }
    if (HDetachedTopRebar != NULL)
    {
        DarkModeApplyWindow(HDetachedTopRebar);
        DarkModeApplyRebarSeparators(HDetachedTopRebar);
    }
    LayoutWindows();

    // main menu
    MainMenu.SetImageList(HGrayToolBarImageList, TRUE);
    MainMenu.SetHotImageList(HHotToolBarImageList, TRUE);
    if (MenuBar != NULL && MenuBar->HWindow != NULL)
    {
        InvalidateRect(MenuBar->HWindow, NULL, TRUE);
        UpdateWindow(MenuBar->HWindow);
    }

    // archive menu
    ArchiveMenu.SetImageList(HGrayToolBarImageList, TRUE);
    ArchiveMenu.SetHotImageList(HHotToolBarImageList, TRUE);
    ArchivePanelMenu.SetImageList(HGrayToolBarImageList, TRUE);
    ArchivePanelMenu.SetHotImageList(HHotToolBarImageList, TRUE);

    if (DetachedPanels)
    {
        // Recreate the detached image lists before panel toolbars are rebound
        // below. Rebinding first would leave every detached tab referencing
        // handles that DestroyDetachedChrome() destroys a few lines later.
        DestroyDetachedChrome();
        EnsureDetachedChrome();
    }

    // Every tab can own DPI-specific icon copies for a different top-level
    // window. Global icon sources have just been recreated, so invalidate
    // all tab copies rather than only the two currently active panels.
    for (int i = 0; i < LeftPanelTabs.Count; ++i)
    {
        if (LeftPanelTabs[i] != NULL)
            LeftPanelTabs[i]->OnColorsChanged();
    }

    for (int i = 0; i < RightPanelTabs.Count; ++i)
    {
        if (RightPanelTabs[i] != NULL)
            RightPanelTabs[i]->OnColorsChanged();
    }

    for (size_t i = 0; i < DetachedTabs.size(); ++i)
    {
        CDetachedTabInfo& info = DetachedTabs[i];
        info.WindowDPI = 0; // color scheme changes the rasterized toolbar background
        RebuildDetachedTabToolbarImageLists((int)WinLibDPIGetWindowDPI(info.HWindow), info.HWindow);
        if (info.Panel != NULL)
            info.Panel->OnColorsChanged();
        if (info.HWindow != NULL)
        {
            DarkModeApplyTree(info.HWindow);
            DarkModeRefreshTitleBar(info.HWindow);
            RedrawWindow(info.HWindow, NULL, NULL,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
    }

    if (EditWindow != NULL && EditWindow->HWindow != NULL)
    {
        // The command-line combo has custom painting and native combo child
        // windows. Recreate it on a color-scheme switch; this follows the same
        // path as startup, which avoids stale themed combo background pieces.
        const BOOL focusCommandLine = EditWindow->KnowHWND(GetFocus());
        HideCommandLine(TRUE, FALSE);
        ShowCommandLine(focusCommandLine);
    }

    if (DetachedPanels)
    {
        LayoutDetachedPanels();
        RedrawWindow(HRightDetachedWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }

    RedrawWindow(HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CMainWindow::StartAnimate()
{
    //AnimateBar->Start();
}

void CMainWindow::StopAnimate()
{
    //AnimateBar->Stop();
}
