// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Runtime for Open Salamander

    salamatrix_poc.h
    Proof-of-concept scenarios for the MVP services.
*/

#pragma once

#include "salamatrix_runtime.h"

namespace Salamatrix
{
namespace Poc
{

inline const char* WINAPI ResultToText(Runtime::OperationResult result)
{
    switch (result)
    {
    case Runtime::OperationResultOk:
        return "ok";
    case Runtime::OperationResultCancel:
        return "cancel";
    case Runtime::OperationResultError:
        return "error";
    default:
        return "unknown";
    }
}

inline Runtime::OperationResult WINAPI RunProgressDialogPoc(CSalamanderForOperationsAbstract* operations)
{
    Runtime::LocalUIService uiService;
    UI::IProgressDialog* progress = uiService.CreateProgressDialog(operations);
    if (progress == NULL)
        return Runtime::OperationResultError;

    UI::ProgressDialogOptions options;
    options.Title = "Salamatrix.UI ProgressDialog PoC";
    options.CancelEnabled = TRUE;
    progress->Open(options);
    progress->SetTotal(CQuadWord(6, 0));
    progress->AddText("Preparing Salamatrix progress PoC...", FALSE);

    Runtime::OperationResult result = Runtime::OperationResultOk;
    for (int i = 0; i < 6; ++i)
    {
        Sleep(250);
        if (!progress->Step(1, FALSE))
        {
            progress->AddText("Canceling operation, please wait...", FALSE);
            progress->SetCancelEnabled(FALSE);
            result = Runtime::OperationResultCancel;
            break;
        }
    }

    Sleep(250);
    progress->Close();
    uiService.DestroyProgressDialog(progress);
    return result;
}

inline Runtime::OperationResult WINAPI RunAutomationProgressPoc(CSalamanderForOperationsAbstract* operations)
{
    Runtime::LocalUIService uiService;
    Automation::ScriptUIAdapter scriptUI(&uiService);
    Automation::ScriptProgressDialog* progress = scriptUI.Progress(operations, "Salamander.UI.progress PoC");
    if (progress == NULL || !progress->IsAvailable())
    {
        scriptUI.DestroyProgress(progress);
        return Runtime::OperationResultError;
    }

    progress->SetTotal(CQuadWord(3, 0));
    progress->AddText("Running script-facing Salamatrix progress PoC...");

    Runtime::OperationResult result = Runtime::OperationResultOk;
    for (int i = 0; i < 3; ++i)
    {
        Sleep(250);
        if (!progress->Step(1))
        {
            result = Runtime::OperationResultCancel;
            break;
        }
    }

    if (result == Runtime::OperationResultOk && progress->IsCancelled())
        result = Runtime::OperationResultCancel;

    if (result == Runtime::OperationResultCancel)
        progress->SetCancelEnabled(FALSE);

    scriptUI.DestroyProgress(progress);
    return result;
}

inline Runtime::OperationResult WINAPI ExecuteQuickRenamePoc(CSalamanderGeneralAbstract* general)
{
    Runtime::RuntimeServices services(general);
    Commands::ExecuteOptions options;
    options.RequireEnabled = FALSE;
    return services.Commands()->Execute("QuickRename", options);
}

inline Runtime::OperationResult WINAPI CopyInteractivePoc(CSalamanderGeneralAbstract* general)
{
    Runtime::RuntimeServices services(general);
    FileOperations::InteractiveOptions options;
    options.RequireEnabled = FALSE;
    return services.FileOperations()->CopyInteractive(options);
}

struct RunAllResult
{
    BOOL ServicesRegistered;
    int ServiceCount;
    Runtime::OperationResult NativeProgress;
    Runtime::OperationResult ScriptProgress;
    Runtime::OperationResult QuickRename;
    Runtime::OperationResult CopyInteractive;

    RunAllResult()
        : ServicesRegistered(FALSE),
          ServiceCount(0),
          NativeProgress(Runtime::OperationResultError),
          ScriptProgress(Runtime::OperationResultError),
          QuickRename(Runtime::OperationResultError),
          CopyInteractive(Runtime::OperationResultError)
    {
    }
};

inline RunAllResult WINAPI RunAllPoc(CSalamanderGeneralAbstract* general, CSalamanderForOperationsAbstract* operations)
{
    RunAllResult result;
    Runtime::RuntimeServices services(general);
    result.ServicesRegistered = services.IsRegistered();
    result.ServiceCount = services.Services()->GetCount();

    result.NativeProgress = RunProgressDialogPoc(operations);
    result.ScriptProgress = RunAutomationProgressPoc(operations);
    Commands::ExecuteOptions commandOptions;
    commandOptions.RequireEnabled = FALSE;
    result.QuickRename = services.Commands()->Execute("QuickRename", commandOptions);

    FileOperations::InteractiveOptions fileOptions;
    fileOptions.RequireEnabled = FALSE;
    result.CopyInteractive = services.FileOperations()->CopyInteractive(fileOptions);
    return result;
}

} // namespace Poc
} // namespace Salamatrix
