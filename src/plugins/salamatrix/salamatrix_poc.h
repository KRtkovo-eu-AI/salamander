// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Runtime for Open Salamander

    salamatrix_poc.h
    In-process proof-of-concept wiring for the MVP services.
*/

#pragma once

#include "salamatrix_automation.h"

namespace Salamatrix
{
namespace Poc
{

class LocalUIService : public UI::IUIService
{
public:
    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_UI_VERSION_1_0;
    }

    virtual UI::IProgressDialog* WINAPI CreateProgressDialog(CSalamanderForOperationsAbstract* operations)
    {
        if (operations == NULL)
            return NULL;
        return new UI::ProgressDialog(operations);
    }

    virtual void WINAPI DestroyProgressDialog(UI::IProgressDialog* dialog)
    {
        delete static_cast<UI::ProgressDialog*>(dialog);
    }
};

class RuntimeServices
{
private:
    LocalUIService UIService;
    Commands::CommandService CommandService;
    FileOperations::FileOperationsService FileOperationsService;
    Automation::ScriptRootAdapter ScriptRoot;

    RuntimeServices(const RuntimeServices&);
    RuntimeServices& operator=(const RuntimeServices&);

public:
    explicit RuntimeServices(CSalamanderGeneralAbstract* general)
        : UIService(),
          CommandService(general),
          FileOperationsService(&CommandService),
          ScriptRoot(&UIService, &CommandService, &FileOperationsService)
    {
    }

    UI::IUIService* WINAPI UI()
    {
        return &UIService;
    }

    Commands::ICommandService* WINAPI Commands()
    {
        return &CommandService;
    }

    FileOperations::IFileOperationsService* WINAPI FileOperations()
    {
        return &FileOperationsService;
    }

    Automation::ScriptRootAdapter* WINAPI Script()
    {
        return &ScriptRoot;
    }
};

inline Runtime::OperationResult WINAPI RunProgressDialogPoc(CSalamanderForOperationsAbstract* operations)
{
    LocalUIService uiService;
    UI::IProgressDialog* progress = uiService.CreateProgressDialog(operations);
    if (progress == NULL)
        return Runtime::OperationResultError;

    UI::ProgressDialogOptions options;
    options.Title = "Salamatrix.UI ProgressDialog PoC";
    options.CancelEnabled = TRUE;
    progress->Open(options);
    progress->SetTotal(CQuadWord(3, 0));
    progress->AddText("Preparing Salamatrix progress PoC...", FALSE);

    Runtime::OperationResult result = Runtime::OperationResultOk;
    for (int i = 0; i < 3; ++i)
    {
        if (!progress->Step(1, FALSE))
        {
            progress->AddText("Canceling operation, please wait...", FALSE);
            progress->SetCancelEnabled(FALSE);
            result = Runtime::OperationResultCancel;
            break;
        }
    }

    progress->Close();
    uiService.DestroyProgressDialog(progress);
    return result;
}

inline Runtime::OperationResult WINAPI RunAutomationProgressPoc(CSalamanderForOperationsAbstract* operations)
{
    LocalUIService uiService;
    Automation::ScriptUIAdapter scriptUI(&uiService);
    Automation::ScriptProgressDialog* progress = scriptUI.Progress(operations, "Salamander.UI.progress PoC");
    if (progress == NULL || !progress->IsAvailable())
    {
        scriptUI.DestroyProgress(progress);
        return Runtime::OperationResultError;
    }

    progress->SetTotal(CQuadWord(2, 0));
    progress->AddText("Running script-facing Salamatrix progress PoC...");
    Runtime::OperationResult result = progress->Step(1) ? Runtime::OperationResultOk : Runtime::OperationResultCancel;
    if (result == Runtime::OperationResultOk && progress->IsCancelled())
        result = Runtime::OperationResultCancel;

    if (result == Runtime::OperationResultCancel)
        progress->SetCancelEnabled(FALSE);

    scriptUI.DestroyProgress(progress);
    return result;
}

inline Runtime::OperationResult WINAPI ExecuteQuickRenamePoc(CSalamanderGeneralAbstract* general)
{
    Commands::CommandService commands(general);
    Commands::ExecuteOptions options;
    return commands.Execute("QuickRename", options);
}

inline Runtime::OperationResult WINAPI CopyInteractivePoc(CSalamanderGeneralAbstract* general)
{
    Commands::CommandService commands(general);
    FileOperations::FileOperationsService fileOperations(&commands);
    FileOperations::InteractiveOptions options;
    return fileOperations.CopyInteractive(options);
}

} // namespace Poc
} // namespace Salamatrix
