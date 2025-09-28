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

BOOL WINAPI
CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander,
                                            HWND parent, int id, DWORD eventMask)
{
    switch (id)
    {
    case MENUCMD_SHOWBROWSER:
    {
        char initialPath[2 * MAX_PATH] = {0};
        int pathType = 0;
        if (!SalamanderGeneral->GetPanelPath(PANEL_SOURCE, initialPath, sizeof(initialPath), &pathType, NULL) ||
            pathType != PATH_TYPE_WINDOWS)
        {
            initialPath[0] = '\0';
        }

        if (!ManagedBridge_ShowBrowser(parent, initialPath[0] != '\0' ? initialPath : NULL))
        {
            SalamanderGeneral->ShowMessageBox("Unable to open the Tree View browser window.",
                                             LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        }
        break;
    }

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
