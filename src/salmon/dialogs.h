// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

enum CDialogTaskEnum
{
    dteCompress,
    dteMinidump,
    dteDialog
};

class CMainDialog : public CDialog
{
protected:
    HFONT HBoldFont;
    BOOL Compressing;
    BOOL Minidumping;
    CCompressParams CompressParams;
    CMinidumpParams MinidumpParams;
    char CurrentProgressText[200];
    BOOL MinidumpOnOpen; // should minidump generation start after opening the window?

public:
    CMainDialog(HINSTANCE modul, int resID, BOOL minidumpOnOpen);
    ~CMainDialog();

    virtual void Validate(CTransferInfo& ti);
    virtual void Transfer(CTransferInfo& ti);

protected:
    virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void ShowChilds(CDialogTaskEnum task, BOOL enable);
    void CenterControl(int resID);
};
