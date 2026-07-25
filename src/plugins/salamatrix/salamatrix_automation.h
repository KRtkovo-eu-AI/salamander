// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Runtime for Open Salamander

    salamatrix_automation.h
    Script/Automation adapter contracts over the native Salamatrix MVP services.
*/

#pragma once

#include "salamatrix_commands.h"
#include "salamatrix_ui.h"

namespace Salamatrix
{
namespace Automation
{

#define SALAMATRIX_SERVICE_AUTOMATION_ADAPTER "Salamatrix.Automation"
#define SALAMATRIX_AUTOMATION_VERSION_1_0 0x00010000

class ScriptProgressDialog
{
private:
    UI::IUIService* UIService;
    UI::IProgressDialog* Progress;

    ScriptProgressDialog(const ScriptProgressDialog&);
    ScriptProgressDialog& operator=(const ScriptProgressDialog&);

public:
    ScriptProgressDialog(UI::IUIService* uiService,
                         CSalamanderForOperationsAbstract* operations,
                         const char* title)
        : UIService(uiService),
          Progress(NULL)
    {
        if (UIService == NULL)
            return;

        Progress = UIService->CreateProgressDialog(operations);
        if (Progress != NULL)
        {
            UI::ProgressDialogOptions options;
            options.Title = title;
            Progress->Open(options);
        }
    }

    ~ScriptProgressDialog()
    {
        Close();
        if (UIService != NULL && Progress != NULL)
        {
            UIService->DestroyProgressDialog(Progress);
            Progress = NULL;
        }
    }

    BOOL WINAPI IsAvailable() const
    {
        return Progress != NULL;
    }

    void WINAPI Close()
    {
        if (Progress != NULL)
            Progress->Close();
    }

    void WINAPI SetTotal(const CQuadWord& total)
    {
        if (Progress != NULL)
            Progress->SetTotal(total);
    }

    void WINAPI AddText(const char* text)
    {
        if (Progress != NULL)
            Progress->AddText(text, FALSE);
    }

    BOOL WINAPI Step(int amount)
    {
        if (Progress == NULL)
            return FALSE;
        return Progress->Step(amount, FALSE);
    }

    BOOL WINAPI IsCancelled()
    {
        if (Progress == NULL)
            return TRUE;
        return Progress->IsCancelled();
    }

    void WINAPI SetCancelEnabled(BOOL enabled)
    {
        if (Progress != NULL)
            Progress->SetCancelEnabled(enabled);
    }
};

class ScriptUIAdapter
{
private:
    UI::IUIService* UIService;

public:
    explicit ScriptUIAdapter(UI::IUIService* uiService)
        : UIService(uiService)
    {
    }

    ScriptProgressDialog* WINAPI Progress(CSalamanderForOperationsAbstract* operations, const char* title)
    {
        if (UIService == NULL)
            return NULL;
        return new ScriptProgressDialog(UIService, operations, title);
    }

    void WINAPI DestroyProgress(ScriptProgressDialog* progress)
    {
        delete progress;
    }

    class Dialog
    {
    private:
        UI::IUIService* UIService;
        UI::IDialog* NativeDialog;

        Dialog(const Dialog&);
        Dialog& operator=(const Dialog&);

    public:
        Dialog(UI::IUIService* uiService, UI::IDialog* dialog)
            : UIService(uiService),
              NativeDialog(dialog)
        {
        }

        ~Dialog()
        {
            Close();
        }

        UI::IControl* WINAPI AddLabel(const char* id, const char* text)
        {
            return Add(UI::ControlKindLabel, id, text, FALSE, FALSE, 0);
        }

        UI::IControl* WINAPI AddTextBox(
            const char* id,
            const char* text,
            BOOL readOnly)
        {
            return Add(UI::ControlKindTextBox, id, text, readOnly, FALSE, 0);
        }

        UI::IControl* WINAPI AddCheckBox(
            const char* id,
            const char* text,
            BOOL checked)
        {
            return Add(UI::ControlKindCheckBox, id, text, FALSE, checked, 0);
        }

        UI::IControl* WINAPI AddRadioButton(
            const char* id,
            const char* text,
            BOOL checked)
        {
            return Add(UI::ControlKindRadioButton, id, text, FALSE, checked, 0);
        }

        UI::IControl* WINAPI AddComboBox(const char* id, const char* text)
        {
            return Add(UI::ControlKindComboBox, id, text, FALSE, FALSE, 0);
        }

        UI::IControl* WINAPI AddListView(const char* id)
        {
            return Add(UI::ControlKindListView, id, NULL, FALSE, FALSE, 0);
        }

        UI::IControl* WINAPI AddTreeView(const char* id)
        {
            return Add(UI::ControlKindTreeView, id, NULL, FALSE, FALSE, 0);
        }

        UI::IControl* WINAPI AddTabControl(const char* id)
        {
            return Add(UI::ControlKindTabControl, id, NULL, FALSE, FALSE, 0);
        }

        UI::IControl* WINAPI AddControl(
            UI::ControlKind kind,
            const UI::ControlOptions& options,
            const UI::ControlLayout& layout)
        {
            return NativeDialog != NULL
                       ? NativeDialog->AddControlEx(kind, options, layout)
                       : NULL;
        }

        BOOL WINAPI SetEventCallback(
            UI::DialogEventCallback callback,
            void* context)
        {
            return NativeDialog != NULL
                       ? NativeDialog->SetEventCallback(callback, context)
                       : FALSE;
        }

        BOOL WINAPI AddItem(
            const char* controlId,
            const char* text,
            int parentIndex = -1)
        {
            UI::IControl* control = FindControl(controlId);
            return control != NULL ? control->AddItem(text, parentIndex) : FALSE;
        }

        BOOL WINAPI AddColumn(
            const char* controlId,
            const char* title,
            int width = 180)
        {
            UI::IControl* control = FindControl(controlId);
            return control != NULL ? control->AddColumn(title, width) : FALSE;
        }

        BOOL WINAPI SetSelectedIndex(const char* controlId, int index)
        {
            UI::IControl* control = FindControl(controlId);
            return control != NULL ? control->SetSelectedIndex(index) : FALSE;
        }

        UI::IControl* WINAPI AddButton(
            const char* id,
            const char* text,
            int dialogResult)
        {
            return Add(UI::ControlKindButton, id, text, FALSE, FALSE, dialogResult);
        }

        int WINAPI ShowModal()
        {
            return NativeDialog != NULL ? NativeDialog->ShowModal() : 0;
        }

        UI::IControl* WINAPI FindControl(const char* id)
        {
            return NativeDialog != NULL ? NativeDialog->FindControl(id) : NULL;
        }

        void WINAPI Close()
        {
            if (UIService != NULL && NativeDialog != NULL)
            {
                UIService->DestroyDialog(NativeDialog);
                NativeDialog = NULL;
            }
        }

    private:
        UI::IControl* Add(
            UI::ControlKind kind,
            const char* id,
            const char* text,
            BOOL readOnly,
            BOOL checked,
            int dialogResult)
        {
            if (NativeDialog == NULL)
                return NULL;
            UI::ControlOptions options;
            options.Id = id;
            options.Text = text;
            options.ReadOnly = readOnly;
            options.Checked = checked;
            options.DialogResult = dialogResult;
            return NativeDialog->AddControl(kind, options);
        }
    };

    Dialog* WINAPI DialogBox(const char* title, HWND parent)
    {
        if (UIService == NULL)
            return NULL;
        UI::DialogOptions options;
        options.Title = title;
        options.Parent = parent;
        UI::IDialog* dialog = UIService->CreateDialog(options);
        return dialog != NULL ? new Dialog(UIService, dialog) : NULL;
    }

    void WINAPI DestroyDialog(Dialog* dialog)
    {
        delete dialog;
    }
};

class ScriptCommandsAdapter
{
private:
    Commands::ICommandService* CommandService;

public:
    explicit ScriptCommandsAdapter(Commands::ICommandService* commandService)
        : CommandService(commandService)
    {
    }

    Runtime::OperationResult WINAPI Execute(const char* commandId)
    {
        if (CommandService == NULL)
            return Runtime::OperationResultError;

        Commands::ExecuteOptions options;
        return CommandService->Execute(commandId, options);
    }
};

class ScriptFileOperationsAdapter
{
private:
    FileOperations::IFileOperationsService* FileOperationsService;

public:
    explicit ScriptFileOperationsAdapter(FileOperations::IFileOperationsService* fileOperationsService)
        : FileOperationsService(fileOperationsService)
    {
    }

    Runtime::OperationResult WINAPI RenameInteractive()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;

        FileOperations::InteractiveOptions options;
        return FileOperationsService->RenameInteractive(options);
    }

    Runtime::OperationResult WINAPI CopyInteractive()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;

        FileOperations::InteractiveOptions options;
        return FileOperationsService->CopyInteractive(options);
    }

    Runtime::OperationResult WINAPI MoveInteractive()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;

        FileOperations::InteractiveOptions options;
        return FileOperationsService->MoveInteractive(options);
    }

    Runtime::OperationResult WINAPI DeleteInteractive()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;
        FileOperations::InteractiveOptions options;
        return FileOperationsService->DeleteInteractive(options);
    }

    Runtime::OperationResult WINAPI CreateDirectoryInteractive()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;
        FileOperations::InteractiveOptions options;
        return FileOperationsService->CreateDirectoryInteractive(options);
    }

    Runtime::OperationResult WINAPI Refresh()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;
        FileOperations::InteractiveOptions options;
        return FileOperationsService->Refresh(options);
    }

    Runtime::OperationResult WINAPI ShowProperties()
    {
        if (FileOperationsService == NULL)
            return Runtime::OperationResultError;
        FileOperations::InteractiveOptions options;
        return FileOperationsService->ShowProperties(options);
    }
};

class ScriptRootAdapter
{
private:
    ScriptUIAdapter UIAdapter;
    ScriptCommandsAdapter CommandsAdapter;
    ScriptFileOperationsAdapter FileOperationsAdapter;

public:
    ScriptRootAdapter(UI::IUIService* uiService,
                      Commands::ICommandService* commandService,
                      FileOperations::IFileOperationsService* fileOperationsService)
        : UIAdapter(uiService),
          CommandsAdapter(commandService),
          FileOperationsAdapter(fileOperationsService)
    {
    }

    ScriptUIAdapter* WINAPI UI()
    {
        return &UIAdapter;
    }

    ScriptCommandsAdapter* WINAPI Commands()
    {
        return &CommandsAdapter;
    }

    ScriptFileOperationsAdapter* WINAPI FileOperations()
    {
        return &FileOperationsAdapter;
    }
};

enum ControlKind
{
    ControlKindDialog = 0,
    ControlKindContainer = 1,
    ControlKindLabel = 2,
    ControlKindTextBox = 3,
    ControlKindCheckBox = 4,
    ControlKindComboBox = 5,
    ControlKindButton = 6,
    ControlKindListView = 7
};

class IControlAdapter
{
public:
    virtual ControlKind WINAPI GetKind() const = 0;
    virtual void* WINAPI GetNativeControl() = 0;

protected:
    virtual ~IControlAdapter() {}
};

class IContainerAdapter : public IControlAdapter
{
public:
    virtual Runtime::OperationResult WINAPI AddChild(IControlAdapter* child) = 0;
};

class IDialogAdapter : public IContainerAdapter
{
public:
    virtual Runtime::OperationResult WINAPI ShowModal() = 0;
};

} // namespace Automation
} // namespace Salamatrix
