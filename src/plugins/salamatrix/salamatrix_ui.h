// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Runtime for Open Salamander

    salamatrix_ui.h
    First public C++ shape for Salamatrix.UI.
*/

#pragma once

#include <string>

#include "../shared/spl_com.h"

namespace Salamatrix
{
namespace UI
{

#define SALAMATRIX_SERVICE_UI "Salamatrix.UI"
#define SALAMATRIX_UI_VERSION_1_0 0x00010000

struct ProgressDialogOptions
{
    const char* Title;
    HWND Parent;
    BOOL TwoProgressBars;
    BOOL FileProgress;
    BOOL CancelEnabled;

    ProgressDialogOptions()
        : Title(NULL),
          Parent(NULL),
          TwoProgressBars(FALSE),
          FileProgress(FALSE),
          CancelEnabled(TRUE)
    {
    }
};

class IProgressDialog
{
public:
    virtual void WINAPI SetTitle(const char* title) = 0;
    virtual void WINAPI Open() = 0;
    virtual void WINAPI Open(const ProgressDialogOptions& options) = 0;
    virtual void WINAPI Close() = 0;
    virtual BOOL WINAPI IsOpen() const = 0;

    virtual void WINAPI SetTotal(const CQuadWord& total) = 0;
    virtual void WINAPI SetTotals(const CQuadWord& firstTotal, const CQuadWord& secondTotal) = 0;
    virtual BOOL WINAPI SetPosition(const CQuadWord& position, BOOL delayedPaint) = 0;
    virtual BOOL WINAPI SetPositions(const CQuadWord& firstPosition, const CQuadWord& secondPosition, BOOL delayedPaint) = 0;
    virtual BOOL WINAPI Step(int amount, BOOL delayedPaint) = 0;
    virtual BOOL WINAPI IsCancelled() = 0;

    virtual void WINAPI AddText(const char* text, BOOL delayedPaint) = 0;
    virtual void WINAPI SetCancelEnabled(BOOL enabled) = 0;
    virtual HWND WINAPI GetHWND() = 0;

protected:
    virtual ~IProgressDialog() {}
};

class ProgressDialog : public IProgressDialog
{
private:
    CSalamanderForOperationsAbstract* Operations;
    BOOL Opened;
    BOOL TwoProgressBars;
    ProgressDialogOptions Options;

    static const CQuadWord& InvalidSize()
    {
        static const CQuadWord Invalid(-1, -1);
        return Invalid;
    }

public:
    explicit ProgressDialog(CSalamanderForOperationsAbstract* operations)
        : Operations(operations),
          Opened(FALSE),
          TwoProgressBars(FALSE)
    {
    }

private:
    ProgressDialog(const ProgressDialog&);
    ProgressDialog& operator=(const ProgressDialog&);

public:
    ~ProgressDialog()
    {
        Close();
    }

    virtual void WINAPI SetTitle(const char* title)
    {
        if (Opened)
            return;

        Options.Title = title;
    }

    virtual void WINAPI Open()
    {
        Open(Options);
    }

    virtual void WINAPI Open(const ProgressDialogOptions& options)
    {
        if (Opened || Operations == NULL)
            return;

        Operations->OpenProgressDialog(options.Title != NULL ? options.Title : "Operation Progress",
                                       options.TwoProgressBars, options.Parent, options.FileProgress);
        Opened = TRUE;
        TwoProgressBars = options.TwoProgressBars;
        Operations->ProgressEnableCancel(options.CancelEnabled);
    }

    virtual void WINAPI Close()
    {
        if (!Opened || Operations == NULL)
            return;

        Operations->CloseProgressDialog();
        Opened = FALSE;
        TwoProgressBars = FALSE;
    }

    virtual BOOL WINAPI IsOpen() const
    {
        return Opened;
    }

    virtual void WINAPI SetTotal(const CQuadWord& total)
    {
        if (!Opened || Operations == NULL)
            return;

        Operations->ProgressSetTotalSize(total, InvalidSize());
    }

    virtual void WINAPI SetTotals(const CQuadWord& firstTotal, const CQuadWord& secondTotal)
    {
        if (!Opened || Operations == NULL)
            return;

        Operations->ProgressSetTotalSize(firstTotal, TwoProgressBars ? secondTotal : InvalidSize());
    }

    virtual BOOL WINAPI SetPosition(const CQuadWord& position, BOOL delayedPaint)
    {
        if (!Opened || Operations == NULL)
            return TRUE;

        return Operations->ProgressSetSize(position, InvalidSize(), delayedPaint);
    }

    virtual BOOL WINAPI SetPositions(const CQuadWord& firstPosition, const CQuadWord& secondPosition, BOOL delayedPaint)
    {
        if (!Opened || Operations == NULL)
            return TRUE;

        return Operations->ProgressSetSize(firstPosition, TwoProgressBars ? secondPosition : InvalidSize(), delayedPaint);
    }

    virtual BOOL WINAPI Step(int amount, BOOL delayedPaint)
    {
        if (!Opened || Operations == NULL)
            return TRUE;

        return Operations->ProgressAddSize(amount, delayedPaint);
    }

    virtual BOOL WINAPI IsCancelled()
    {
        if (!Opened || Operations == NULL)
            return FALSE;

        return Operations->ProgressAddSize(0, TRUE) ? FALSE : TRUE;
    }

    virtual void WINAPI AddText(const char* text, BOOL delayedPaint)
    {
        if (!Opened || Operations == NULL || text == NULL)
            return;

        Operations->ProgressDialogAddText(text, delayedPaint);
    }

    virtual void WINAPI SetCancelEnabled(BOOL enabled)
    {
        if (!Opened || Operations == NULL)
            return;

        Operations->ProgressEnableCancel(enabled);
    }

    virtual HWND WINAPI GetHWND()
    {
        if (!Opened || Operations == NULL)
            return NULL;

        return Operations->ProgressGetHWND();
    }
};

enum ControlKind
{
    ControlKindLabel = 0,
    ControlKindTextBox = 1,
    ControlKindCheckBox = 2,
    ControlKindComboBox = 3,
    ControlKindRadioButton = 4,
    ControlKindButton = 5,
    ControlKindListView = 6,
    ControlKindTreeView = 7
};

struct DialogOptions
{
    const char* Title;
    HWND Parent;
    short Width;
    short Height;

    DialogOptions()
        : Title("Salamander"),
          Parent(NULL),
          Width(320),
          Height(180)
    {
    }
};

struct ControlOptions
{
    const char* Id;
    const char* Text;
    BOOL ReadOnly;
    BOOL Checked;
    int DialogResult;

    ControlOptions()
        : Id(NULL),
          Text(NULL),
          ReadOnly(FALSE),
          Checked(FALSE),
          DialogResult(0)
    {
    }
};

class IControl
{
public:
    virtual ControlKind WINAPI GetKind() const = 0;
    virtual const char* WINAPI GetId() const = 0;
    virtual BOOL WINAPI GetText(char* buffer, DWORD capacity) const = 0;
    virtual BOOL WINAPI SetText(const char* value) = 0;
    virtual BOOL WINAPI GetChecked() const = 0;
    virtual BOOL WINAPI SetChecked(BOOL checked) = 0;
    virtual int WINAPI GetDialogResult() const = 0;

protected:
    virtual ~IControl() {}
};

class IDialog
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual IControl* WINAPI AddControl(
        ControlKind kind,
        const ControlOptions& options) = 0;
    virtual IControl* WINAPI FindControl(const char* id) = 0;
    virtual int WINAPI ShowModal() = 0;
    virtual void WINAPI Close() = 0;
    virtual void WINAPI Release() = 0;

protected:
    virtual ~IDialog() {}
};

/// Native dialog implementation shared by the runtime plugin and all
/// adapters. The implementation lives in salamatrix_ui.cpp; keeping the
/// interface here lets native plugins use the same controls as workers.
class NativeDialog : public IDialog
{
private:
    struct Impl;
    Impl* m_pImpl;
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    NativeDialog(const NativeDialog&);
    NativeDialog& operator=(const NativeDialog&);

public:
    explicit NativeDialog(const DialogOptions& options);
    virtual ~NativeDialog();

    virtual DWORD WINAPI GetVersion() const;
    virtual IControl* WINAPI AddControl(
        ControlKind kind,
        const ControlOptions& options);
    virtual IControl* WINAPI FindControl(const char* id);
    virtual int WINAPI ShowModal();
    virtual void WINAPI Close();
    virtual void WINAPI Release();
};

class IUIService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual IProgressDialog* WINAPI CreateProgressDialog(CSalamanderForOperationsAbstract* operations) = 0;
    virtual void WINAPI DestroyProgressDialog(IProgressDialog* dialog) = 0;

    /// Optional in the original 1.0 contract; providers that do not expose
    /// native dialogs can keep the default NULL implementation.
    virtual IDialog* WINAPI CreateDialog(const DialogOptions& options)
    {
        (void)options;
        return NULL;
    }

    virtual void WINAPI DestroyDialog(IDialog* dialog)
    {
        if (dialog != NULL)
            dialog->Release();
    }

    /// Optional message-box entry point appended after the original 1.0
    /// methods so older providers keep their existing vtable order.
    virtual int WINAPI ShowMessageBox(
        HWND parent,
        const char* message,
        const char* title,
        UINT flags)
    {
        (void)parent;
        (void)message;
        (void)title;
        (void)flags;
        return 0;
    }

protected:
    virtual ~IUIService() {}
};

} // namespace UI
} // namespace Salamatrix
