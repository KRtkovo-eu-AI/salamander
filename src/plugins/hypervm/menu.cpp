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
    case MENUCMD_CREATE_VHD:
        if (!ManagedBridge_RunMenuCommand(parent, "CreateVhd"))
            SalamanderGeneral->ShowMessageBox("Unable to execute the managed command.", LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        break;

    case MENUCMD_ATTACH_VHD:
        if (!ManagedBridge_RunMenuCommand(parent, "AttachVhd"))
            SalamanderGeneral->ShowMessageBox("Unable to execute the managed command.", LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
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
    int helpID = 0;
    switch (id)
    {
    }
    if (helpID != 0)
        SalamanderGeneral->OpenHtmlHelp(parent, HHCDisplayContext, helpID, FALSE);
    return helpID != 0;
}


void WINAPI
CPluginInterfaceForMenuExt::BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander)
{
    (void)parent;
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_CREATE_VHD), 0,
                            MENUCMD_CREATE_VHD, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_ATTACH_VHD), 0,
                            MENUCMD_ATTACH_VHD, FALSE, MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
}
