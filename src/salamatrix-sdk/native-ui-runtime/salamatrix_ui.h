// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Runtime for Open Salamander

    salamatrix_ui.h
    First public C++ shape for Salamatrix.UI.
*/

#pragma once

#ifdef CreateDialog
#pragma push_macro("CreateDialog")
#undef CreateDialog
#define SALAMATRIX_RESTORE_CREATE_DIALOG 1
#endif

#include <string>

#include "../../plugins/shared/spl_com.h"

namespace Salamatrix
{
namespace UI
{

class INativeDialogHost;

#define SALAMATRIX_SERVICE_UI "Salamatrix.UI"
#define SALAMATRIX_UI_VERSION_1_0 0x00010000
#define SALAMATRIX_UI_VERSION_1_1 0x00010001
#define SALAMATRIX_UI_VERSION_1_2 0x00010002
#define SALAMATRIX_UI_VERSION_1_3 0x00010003
#define SALAMATRIX_UI_VERSION_1_4 0x00010004

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
    ControlKindTreeView = 7,
    ControlKindTabControl = 8,
    // A native folder chooser embedded in a dialog. Its text value is the
    // selected path and is available through the normal IControl accessors.
    ControlKindFolderPicker = 9,
    // An editable UTF-8 file path with an adjacent native browse button.
    // Appended so existing control-kind values remain stable.
    ControlKindFilePicker = 10,
    // A draggable horizontal separator. Movement is reported through the
    // normal dialog event callback with the parent-client Y coordinate in Text.
    ControlKindSplitter = 11,
    // SDK-owned controls appended in 1.4. Salamatrix.SPL and standalone tools
    // compile the same implementations for identical layout and behavior.
    ControlKindGroupBox = 12,
    ControlKindStaticText = 13,
    ControlKindHyperLink = 14,
    ControlKindProgressBar = 15,
    ControlKindArrowButton = 16,
    ControlKindTextArrowButton = 17,
    ControlKindColorArrowButton = 18,
    ControlKindToolbarHeader = 19
};

enum StaticTextStyle
{
    StaticTextCachedPaint = 0x00000001,
    StaticTextBold = 0x00000002,
    StaticTextUnderline = 0x00000004,
    StaticTextDotUnderline = 0x00000008,
    StaticTextHyperLinkColor = 0x00000010,
    StaticTextEndEllipsis = 0x00000020,
    StaticTextPathEllipsis = 0x00000040,
    StaticTextHandlePrefix = 0x00000080,
    StaticTextAlignCenter = 0x00010000,
    StaticTextAlignRight = 0x00020000,
    StaticTextNotify = 0x00040000
};

enum TextArrowButtonStyle
{
    TextArrowButtonRightArrow = 0x00000008,
    TextArrowButtonDropDown = 0x00000002,
    TextArrowButtonMore = 0x00000010
};

enum ButtonStyle
{
    ButtonDefault = 0x00100000
};

enum ListViewStyle
{
    ListViewNoDefaultColumn = 0x00200000,
    ListViewShowSelectionAlways = 0x00400000,
    ListViewEditLabels = 0x00800000,
    ListViewNoSortHeader = 0x01000000
};

enum ToolbarHeaderButtons
{
    ToolbarHeaderModify = 0x00000001,
    ToolbarHeaderNew = 0x00000002,
    ToolbarHeaderDelete = 0x00000004,
    ToolbarHeaderSort = 0x00000008,
    ToolbarHeaderUp = 0x00000010,
    ToolbarHeaderDown = 0x00000020,
    ToolbarHeaderTop = 0x00000040,
    ToolbarHeaderFilter = 0x00000080,
    ToolbarHeaderSearch = 0x00000100,
    ToolbarHeaderBottom = 0x00000200
};

struct DialogOptions
{
    const char* Title;
    HWND Parent;
    short Width;
    short Height;
    BOOL Modeless;
    BOOL Resizable;
    BOOL Taskbar;
    HICON SmallIcon;
    HICON LargeIcon;

    DialogOptions()
        : Title("Salamander"),
          Parent(NULL),
          Width(320),
          Height(180),
          Modeless(FALSE),
          Resizable(FALSE),
          Taskbar(FALSE),
          SmallIcon(NULL),
          LargeIcon(NULL)
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
    /// Keep a button dialog open after dispatching its event callback.
    /// Appended for ABI compatibility with the original control contract.
    BOOL KeepOpen;
    /// Use a multiline edit control with vertical scrolling.
    /// Appended for ABI compatibility with the original control contract.
    BOOL Multiline;
    /// File filter in Salamatrix pipe syntax, for file-picker controls only.
    /// Appended for ABI compatibility with the original control contract.
    const char* FileFilter;
    /// File picker should use save-mode (GetSaveFileNameW) when TRUE.
    /// Appended for ABI compatibility with the original control contract.
    BOOL FileSave;
    /// Accessible name exposed by the native control.
    /// Appended for ABI compatibility with the original control contract.
    const char* AccessibleName;
    /// Accessible description exposed by the native control.
    /// Appended for ABI compatibility with the original control contract.
    const char* AccessibleDescription;

    ControlOptions()
        : Id(NULL),
          Text(NULL),
          ReadOnly(FALSE),
          Checked(FALSE),
          DialogResult(0),
          KeepOpen(FALSE),
          Multiline(FALSE),
          FileFilter(NULL),
          FileSave(FALSE),
          AccessibleName(NULL),
          AccessibleDescription(NULL)
    {
    }
};

struct ControlLayout
{
    BOOL HasBounds;
    int X;
    int Y;
    int Width;
    int Height;

    ControlLayout()
        : HasBounds(FALSE),
          X(0),
          Y(0),
          Width(0),
          Height(0)
    {
    }
};

enum DialogEventKind
{
    DialogEventControlChanged = 1
};

struct DialogEvent
{
    DWORD StructSize;
    DialogEventKind Kind;
    ControlKind Control;
    char ControlId[128];
    char Text[4096];
    BOOL Checked;
    int SelectedIndex;

    DialogEvent()
        : StructSize(sizeof(DialogEvent)),
          Kind(DialogEventControlChanged),
          Control(ControlKindLabel),
          Checked(FALSE),
          SelectedIndex(-1)
    {
        ControlId[0] = '\0';
        Text[0] = '\0';
    }
};

typedef BOOL(WINAPI* DialogEventCallback)(
    void* context,
    const DialogEvent* event);

class IDialog;
typedef void(WINAPI* DialogResizeCallback)(
    void* context, IDialog* dialog, int width, int height);
typedef void(WINAPI* DialogCloseCallback)(void* context, IDialog* dialog);

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

    /// Adds one item to a ComboBox, ListView, or TreeView. For TreeView,
    /// parentIndex is the zero-based index of the parent item, or -1 for root.
    /// Optional so older UI providers can keep the original control surface.
    virtual BOOL WINAPI AddItem(
        const char* text,
        int parentIndex = -1)
    {
        (void)text;
        (void)parentIndex;
        return FALSE;
    }

    virtual BOOL WINAPI ClearItems()
    {
        return FALSE;
    }

    virtual int WINAPI GetItemCount() const
    {
        return 0;
    }

    /// Optional validation state appended after the original control surface.
    virtual BOOL WINAPI SetRequired(BOOL required)
    {
        (void)required;
        return FALSE;
    }

    virtual BOOL WINAPI IsRequired() const
    {
        return FALSE;
    }

    virtual BOOL WINAPI SetValidationMessage(const char* message)
    {
        (void)message;
        return FALSE;
    }

    virtual BOOL WINAPI GetValidationMessage(
        char* buffer,
        DWORD capacity) const
    {
        if (buffer != NULL && capacity != 0)
            buffer[0] = '\0';
        return FALSE;
    }

    /// Optional ListView column and selection surface appended to the
    /// control contract. Other control kinds return FALSE/-1.
    virtual BOOL WINAPI AddColumn(const char* title, int width)
    {
        (void)title;
        (void)width;
        return FALSE;
    }

    virtual int WINAPI GetSelectedIndex() const
    {
        return -1;
    }

    virtual BOOL WINAPI SetSelectedIndex(int index)
    {
        (void)index;
        return FALSE;
    }

    /// Optional bounded accessibility metadata appended to the control
    /// contract. Older providers receive empty strings by default.
    virtual const char* WINAPI GetAccessibleName() const
    {
        return "";
    }

    virtual const char* WINAPI GetAccessibleDescription() const
    {
        return "";
    }

    virtual BOOL WINAPI SetBounds(int x, int y, int width, int height)
    {
        (void)x; (void)y; (void)width; (void)height;
        return FALSE;
    }

    /// Optional enabled state appended after the original control surface.
    /// Composite controls, such as a file picker, apply the state to all of
    /// their child windows.
    virtual BOOL WINAPI SetEnabled(BOOL enabled)
    {
        (void)enabled;
        return FALSE;
    }

    virtual BOOL WINAPI IsEnabled() const
    {
        return TRUE;
    }

    /// Host-control configuration appended in UI 1.4. Unsupported control
    /// kinds return FALSE and older providers retain their default behaviour.
    virtual BOOL WINAPI SetStyleFlags(DWORD flags)
    {
        (void)flags;
        return FALSE;
    }

    virtual BOOL WINAPI SetPathSeparator(char separator)
    {
        (void)separator;
        return FALSE;
    }

    virtual BOOL WINAPI SetToolTipText(const char* text)
    {
        (void)text;
        return FALSE;
    }

    virtual BOOL WINAPI SetActionOpen(const char* target)
    {
        (void)target;
        return FALSE;
    }

    virtual BOOL WINAPI SetActionPostCommand(WORD command)
    {
        (void)command;
        return FALSE;
    }

    virtual BOOL WINAPI SetActionShowHint(const char* text)
    {
        (void)text;
        return FALSE;
    }

    virtual BOOL WINAPI SetProgress(int progress, const char* text = NULL)
    {
        (void)progress;
        (void)text;
        return FALSE;
    }

    virtual BOOL WINAPI SetProgressValues(
        ULONGLONG current,
        ULONGLONG total,
        const char* text = NULL)
    {
        (void)current;
        (void)total;
        (void)text;
        return FALSE;
    }

    virtual BOOL WINAPI SetIndeterminateTiming(DWORD duration, DWORD interval)
    {
        (void)duration;
        (void)interval;
        return FALSE;
    }

    virtual BOOL WINAPI SetColor(COLORREF textColor, COLORREF backgroundColor)
    {
        (void)textColor;
        (void)backgroundColor;
        return FALSE;
    }

    virtual BOOL WINAPI SetToolbarHeader(
        const char* alignControlId,
        DWORD buttonMask)
    {
        (void)alignControlId;
        (void)buttonMask;
        return FALSE;
    }

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

    /// ABI-safe extension for explicit control bounds. Older providers use
    /// the original AddControl implementation and simply ignore the layout.
    virtual IControl* WINAPI AddControlEx(
        ControlKind kind,
        const ControlOptions& options,
        const ControlLayout& layout)
    {
        (void)layout;
        return AddControl(kind, options);
    }

    /// Appended callback surface for control changes while a modal dialog is
    /// running. The event payload is valid only for the duration of callback.
    virtual BOOL WINAPI SetEventCallback(
        DialogEventCallback callback,
        void* context)
    {
        (void)callback;
        (void)context;
        return FALSE;
    }

    virtual BOOL WINAPI SetResizeCallback(
        DialogResizeCallback callback,
        void* context)
    {
        (void)callback;
        (void)context;
        return FALSE;
    }

    virtual BOOL WINAPI SetCloseCallback(
        DialogCloseCallback callback,
        void* context)
    {
        (void)callback;
        (void)context;
        return FALSE;
    }

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
    virtual IControl* WINAPI AddControlEx(
        ControlKind kind,
        const ControlOptions& options,
        const ControlLayout& layout);
    virtual BOOL WINAPI SetEventCallback(
        DialogEventCallback callback,
        void* context);
    virtual BOOL WINAPI SetResizeCallback(
        DialogResizeCallback callback,
        void* context);
    virtual BOOL WINAPI SetCloseCallback(
        DialogCloseCallback callback,
        void* context);
};

// Closes every HWND created by NativeDialog before the UI provider DLL is
// unloaded. This is a provider-internal lifecycle hook, not part of the
// plug-in-facing IUIService ABI.
void WINAPI CloseAllNativeDialogs();

// Implemented by the Salamatrix native UI provider and used by its local
// service implementation. It is intentionally a free function so the
// IUIService vtable can keep its append-only ABI contract.
BOOL WINAPI ShowNativeNotification(
    HWND parent,
    const char* title,
    const char* message,
    DWORD timeoutMs);

// Framework-owned showcase of the controls exposed by Salamatrix.UI.
// Native plug-ins and runtime adapters reach the same implementation through
// IUIService::ShowControlsShowcase.
BOOL WINAPI ShowNativeControlsShowcase(HWND parent);

class IUIService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual IProgressDialog* WINAPI CreateProgressDialog(CSalamanderForOperationsAbstract* operations) = 0;
    virtual void WINAPI DestroyProgressDialog(IProgressDialog* dialog) = 0;

    /// Optional in the original 1.0 contract; providers that do not expose
    /// native dialogs can keep the default NULL implementation.
    virtual IDialog* WINAPI CreateSalamatrixDialog(const DialogOptions& options)
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

    /// Clipboard is part of the shared application UI surface for scripts
    /// that need to publish generated text without reimplementing Win32.
    virtual BOOL WINAPI CopyTextToClipboard(
        const char* text,
        BOOL showEcho,
        HWND echoParent)
    {
        (void)text;
        (void)showEcho;
        (void)echoParent;
        return FALSE;
    }

    /// Opens a native file picker. `filter` is UTF-8 text in the form
    /// "Description|pattern|Description|pattern"; an empty filter means
    /// all files. The selected path is returned as UTF-8.
    virtual BOOL WINAPI PickFile(
        HWND parent,
        BOOL save,
        const char* title,
        const char* filter,
        const char* initialPath,
        char* result,
        DWORD resultCapacity)
    {
        (void)parent;
        (void)save;
        (void)title;
        (void)filter;
        (void)initialPath;
        (void)result;
        (void)resultCapacity;
        return FALSE;
    }

    /// Opens a native folder picker. The selected directory is returned as
    /// UTF-8. This method is appended after the original picker contract so
    /// existing UI providers keep their vtable layout.
    virtual BOOL WINAPI PickFolder(
        HWND parent,
        const char* title,
        const char* initialPath,
        char* result,
        DWORD resultCapacity)
    {
        (void)parent;
        (void)title;
        (void)initialPath;
        (void)result;
        (void)resultCapacity;
        return FALSE;
    }

    /// Non-modal, auto-dismissing native notification appended after the
    /// original UI contract. Providers without a notification surface may
    /// keep the default FALSE implementation.
    virtual BOOL WINAPI ShowNotification(
        HWND parent,
        const char* title,
        const char* message,
        DWORD timeoutMs)
    {
        (void)parent;
        (void)title;
        (void)message;
        (void)timeoutMs;
        return FALSE;
    }

    /// Shows the framework-owned native controls showcase. Appended in UI
    /// 1.3 so existing providers keep their vtable layout.
    virtual BOOL WINAPI ShowControlsShowcase(HWND parent)
    {
        (void)parent;
        return FALSE;
    }

protected:
    virtual ~IUIService() {}
};

} // namespace UI
} // namespace Salamatrix

#ifdef SALAMATRIX_RESTORE_CREATE_DIALOG
#pragma pop_macro("CreateDialog")
#undef SALAMATRIX_RESTORE_CREATE_DIALOG
#endif
