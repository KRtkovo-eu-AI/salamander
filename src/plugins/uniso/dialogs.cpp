﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "uniso.h"
#include "dialogs.h"
#include "uniso.rh"
#include "uniso.rh2"
#include "lang\lang.rh"

//****************************************************************************
//
// CCommonDialog
//

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin)
    : CDialog(hInstance, resID, hParent, origin)
{
}

INT_PTR
CCommonDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // horizontal and vertical centering of the dialog relative to the parent
        if (Parent != NULL)
            SalamanderGeneral->MultiMonCenterWindow(HWindow, Parent, TRUE);
        break; // let DefDlgProc handle setting the focus
    }
    } // switch
    return CDialog::DialogProc(uMsg, wParam, lParam);
}

void CCommonDialog::NotifDlgJustCreated()
{
    SalamanderGUI->ArrangeHorizontalLines(HWindow);
}

//****************************************************************************
//
// CConfigurationDialog
//

CConfigurationDialog::CConfigurationDialog(HWND hParent)
    : CCommonDialog(HLanguage, IDD_CONFIGURATION, hParent)
{
}

void CConfigurationDialog::Transfer(CTransferInfo& ti)
{
    ti.CheckBox(IDC_CFG_READONLY, Options.ClearReadOnly);
    ti.CheckBox(IDC_CFG_SESSIONASDIR, Options.SessionAsDirectory);
    ti.CheckBox(IDC_CFG_BOOTIMAGEASFILE, Options.BootImageAsFile);
}

INT_PTR
CConfigurationDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // set (enable/disable) 'boot image as file' according to 'sessions as dirs'
        EnableWindow(GetDlgItem(HWindow, IDC_CFG_BOOTIMAGEASFILE), Options.SessionAsDirectory);
        break; // let DefDlgProc handle setting the focus
    }

    case WM_COMMAND:
    {
        // toggle the 'show boot disk as file' option
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CFG_SESSIONASDIR)
        {
            BOOL enable = IsDlgButtonChecked(HWindow, IDC_CFG_SESSIONASDIR) == BST_CHECKED;
            EnableWindow(GetDlgItem(HWindow, IDC_CFG_BOOTIMAGEASFILE), enable);
            if (!enable)
                CheckDlgButton(HWindow, IDC_CFG_BOOTIMAGEASFILE, BST_UNCHECKED);
        }
        break;
    }

    } // switch
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}
