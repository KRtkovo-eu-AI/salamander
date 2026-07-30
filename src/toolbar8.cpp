// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "mainwnd.h"
#include "plugins.h"
#include "toolbar.h"
#include "common/winlibdpi.h"

//*****************************************************************************
//
// CPluginsBar
//

CPluginsBar::CPluginsBar(HWND hNotifyWindow, CObjectOrigin origin)
    : CToolBar(hNotifyWindow, origin)
{
    CALL_STACK_MESSAGE_NONE
    HPluginsIcons = NULL;
    HPluginsIconsGray = NULL;
}

CPluginsBar::~CPluginsBar()
{
    DestroyImageLists();
}

void CPluginsBar::DestroyImageLists()
{
    if (HPluginsIcons != NULL)
    {
        ImageList_Destroy(HPluginsIcons);
        HPluginsIcons = NULL;
    }
    if (HPluginsIconsGray != NULL)
    {
        ImageList_Destroy(HPluginsIconsGray);
        HPluginsIconsGray = NULL;
    }
}

BOOL CPluginsBar::CreatePluginButtons()
{
    CALL_STACK_MESSAGE1("CPluginsBar::CreateButtons()");
    if (HWindow == NULL)
        return FALSE;

    RemoveAllItems();

    SetStyle(TLB_STYLE_IMAGE /*| TLB_STYLE_TEXT*/);

    DestroyImageLists();

    HPluginsIcons = Plugins.CreateIconsList(FALSE, HWindow);
    HPluginsIconsGray = Plugins.CreateIconsList(TRUE, HWindow);

    SetImageList(HPluginsIconsGray);
    SetHotImageList(HPluginsIcons);

    Plugins.InitPluginsBar(this);
    /*
  TLBI_ITEM_INFO2 tii;
  int i;
  for (i = 0; i < Order.GetCount(); i++)
  {
    CPluginData *plugin = Plugins.Get(i);
    if (plugin == NULL || plugin->MenuItems.Count == 0) 
      continue;

    tii.Mask = TLBI_MASK_STYLE | TLBI_MASK_IMAGEINDEX | TLBI_MASK_ID;
    tii.Style = TLBI_STYLE_WHOLEDROPDOWN | TLBI_STYLE_DROPDOWN;
    tii.ImageIndex = i;
    tii.ID = CM_PLUGINCMD_MIN + i; // do mainwnd3 prijde jako WM_USER_TBDROPDOWN
    InsertItem2(0xFFFFFFFF, TRUE, &tii);
  }
  */

    return TRUE;
}

int CPluginsBar::GetNeededHeight()
{
    CALL_STACK_MESSAGE_NONE
    // i v pripade, ze nedrzime zadnou ikonu budeem vracet spravnou vysku
    int height = CToolBar::GetNeededHeight();
    int iconSize = MulDiv(16, (int)WinLibDPIGetWindowDPI(HWindow),
                          USER_DEFAULT_SCREEN_DPI);
    int minH = 3 + iconSize + 3;
    if (height < minH)
        height = minH;
    return height;
}

void CPluginsBar::Customize()
{
    CALL_STACK_MESSAGE_NONE
    // zobrazim okno Plugins
    PostMessage(MainWindow->HWindow, WM_COMMAND, CM_CUSTOMIZEPLUGINS, 0);
}

void CPluginsBar::OnGetToolTip(LPARAM lParam)
{
    CALL_STACK_MESSAGE2("CPluginsBar::OnGetToolTip(0x%IX)", lParam);
    TOOLBAR_TOOLTIP* tt = (TOOLBAR_TOOLTIP*)lParam;

    int index = tt->ID - CM_PLUGINCMD_MIN;
    tt->Buffer[0] = 0;
    CPluginData* plugin = Plugins.Get(index);
    if (plugin != NULL)
        lstrcpy(tt->Buffer, plugin->Name);
}

//*****************************************************************************
//
// CExtensionBar
//

CExtensionBar::CExtensionBar(HWND hNotifyWindow, CObjectOrigin origin)
    : CToolBar(hNotifyWindow, origin)
{
}

BOOL CExtensionBar::CreateExtensionButtons(HIMAGELIST imageList,
                                           HIMAGELIST hotImageList)
{
    CALL_STACK_MESSAGE1("CExtensionBar::CreateExtensionButtons()");
    if (HWindow == NULL)
        return FALSE;

    RemoveAllItems();
    SetStyle(TLB_STYLE_IMAGE);
    SetImageList(imageList);
    SetHotImageList(hotImageList);
    Plugins.EnsureToolbarButtonImages(hotImageList, imageList);

    for (int index = 0; index < Plugins.GetToolbarButtonCount(); ++index)
    {
        DWORD toolbarId;
        const char* title;
        int imageIndex;
        if (!Plugins.GetToolbarButtonInfo(index, &toolbarId, &title, &imageIndex) ||
            !Plugins.GetExtensionBarVisible(index))
            continue;

        TLBI_ITEM_INFO2 tii;
        tii.Mask = TLBI_MASK_STYLE | TLBI_MASK_ID | TLBI_MASK_CUSTOMDATA |
                   TLBI_MASK_TEXT | TLBI_MASK_TEXTLEN;
        tii.Style = TLBI_STYLE_NOPREFIX | TLBI_STYLE_DARK_DISABLED_IMAGE_TEXT;
        if (imageIndex >= 0)
        {
            tii.Mask |= TLBI_MASK_IMAGEINDEX;
            tii.ImageIndex = imageIndex;
        }
        else
            tii.Style |= TLBI_STYLE_SHOWTEXT;
        tii.ID = toolbarId;
        tii.CustomData = index;
        tii.Text = const_cast<char*>(title);
        tii.TextLen = lstrlen(title);
        if (!InsertItem2(0xFFFFFFFF, TRUE, &tii))
            return FALSE;
    }
    return TRUE;
}

int CExtensionBar::GetNeededHeight()
{
    int height = CToolBar::GetNeededHeight();
    int iconSize = MulDiv(16, (int)WinLibDPIGetWindowDPI(HWindow),
                          USER_DEFAULT_SCREEN_DPI);
    return max(height, 3 + iconSize + 3);
}

void CExtensionBar::Customize()
{
    PostMessage(MainWindow->HWindow, WM_COMMAND, CM_PLUGINS, 0);
}

void CExtensionBar::OnGetToolTip(LPARAM lParam)
{
    TOOLBAR_TOOLTIP* tt = (TOOLBAR_TOOLTIP*)lParam;
    DWORD toolbarId;
    const char* title;
    int imageIndex;
    tt->Buffer[0] = 0;
    if (Plugins.GetToolbarButtonInfo(static_cast<int>(tt->CustomData),
                                     &toolbarId, &title, &imageIndex))
        lstrcpyn(tt->Buffer, title, TOOLTIP_TEXT_MAX);
}
