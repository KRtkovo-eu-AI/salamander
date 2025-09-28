// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#include "precomp.h"

// ****************************************************************************
// SEKCE MENU
// ****************************************************************************

namespace
{
bool ShowTreeViewBrowser(HWND parent, int panel)
{
    char initialPath[2 * MAX_PATH] = {0};
    int pathType = 0;
    if (!SalamanderGeneral->GetPanelPath(panel, initialPath, sizeof(initialPath), &pathType, NULL) ||
        pathType != PATH_TYPE_WINDOWS)
    {
        initialPath[0] = '\0';
    }

    if (!ManagedBridge_ShowBrowser(parent, initialPath[0] != '\0' ? initialPath : NULL))
    {
        SalamanderGeneral->ShowMessageBox("Unable to open the Tree View browser window.",
                                         LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        return false;
    }

    return true;
}
} // namespace

BOOL WINAPI
CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander,
                                            HWND parent, int id, DWORD eventMask)
{
    switch (id)
    {
    case MENUCMD_SHOWBROWSER:
        ShowTreeViewBrowser(parent, PANEL_SOURCE);
        break;

    case MENUCMD_SHOWLEFTPANEL:
        ShowTreeViewBrowser(parent, PANEL_LEFT);
        break;

    case MENUCMD_SHOWRIGHTPANEL:
        ShowTreeViewBrowser(parent, PANEL_RIGHT);
        break;

    default:
        SalamanderGeneral->ShowMessageBox("Unknown command.", LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        break;
    }
    return FALSE; // neodznacovat polozky v panelu
}

BOOL WINAPI
CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
    return FALSE;
}

void WINAPI
CPluginInterfaceForMenuExt::BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander)
{
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_OPEN_BROWSER), 0,
                            MENUCMD_SHOWBROWSER, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_SHOW_LEFTPANEL), 0,
                            MENUCMD_SHOWLEFTPANEL, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_SHOW_RIGHTPANEL), 0,
                            MENUCMD_SHOWRIGHTPANEL, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
}
