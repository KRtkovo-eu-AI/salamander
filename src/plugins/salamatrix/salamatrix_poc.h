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
inline Runtime::RuntimeServices* CreatePocRuntimeServices(
    CSalamanderGeneralAbstract* general)
{
#ifdef new
#undef new
#define RESTORE_SALAMATRIX_POC_DEBUG_NEW_MACRO
#endif
    Runtime::RuntimeServices* services =
        new (std::nothrow) Runtime::RuntimeServices(general, FALSE);
#ifdef RESTORE_SALAMATRIX_POC_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_SALAMATRIX_POC_DEBUG_NEW_MACRO
#endif
    return services;
}

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
    case Runtime::OperationResultNotAvailable:
        return "not available";
    default:
        return "unknown";
    }
}

inline Commands::ICommandService* WINAPI QueryHostCommands(CSalamanderGeneralAbstract* general)
{
    if (general == NULL)
        return NULL;

    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    query.ServiceId = SALAMATRIX_SERVICE_COMMANDS;
    query.MinimumVersion = SALAMATRIX_COMMANDS_VERSION_1_0;
    if (!general->QueryService(&query, &result))
        return NULL;
    return static_cast<Commands::ICommandService*>(result.Interface);
}

inline FileOperations::IFileOperationsService* WINAPI QueryHostFileOperations(CSalamanderGeneralAbstract* general)
{
    if (general == NULL)
        return NULL;

    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    query.ServiceId = SALAMATRIX_SERVICE_FILEOPERATIONS;
    query.MinimumVersion = SALAMATRIX_FILEOPERATIONS_VERSION_1_0;
    if (!general->QueryService(&query, &result))
        return NULL;
    return static_cast<FileOperations::IFileOperationsService*>(result.Interface);
}

inline BOOL WINAPI QueryHostUI(CSalamanderGeneralAbstract* general)
{
    if (general == NULL)
        return FALSE;

    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    query.ServiceId = SALAMATRIX_SERVICE_UI;
    query.MinimumVersion = SALAMATRIX_UI_VERSION_1_0;
    return general->QueryService(&query, &result);
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
    Commands::ExecuteOptions options;
    options.RequireEnabled = FALSE;

    Commands::ICommandService* hostCommands = QueryHostCommands(general);
    if (hostCommands != NULL)
        return hostCommands->Execute("QuickRename", options);

    Runtime::RuntimeServices* services = CreatePocRuntimeServices(general);
    if (services == NULL)
        return Runtime::OperationResultError;
    const Runtime::OperationResult result =
        services->Commands()->Execute("QuickRename", options);
    delete services;
    return result;
}

inline Runtime::OperationResult WINAPI CopyInteractivePoc(CSalamanderGeneralAbstract* general)
{
    FileOperations::InteractiveOptions options;
    options.RequireEnabled = FALSE;

    FileOperations::IFileOperationsService* hostFileOperations = QueryHostFileOperations(general);
    if (hostFileOperations != NULL)
        return hostFileOperations->CopyInteractive(options);

    Runtime::RuntimeServices* services = CreatePocRuntimeServices(general);
    if (services == NULL)
        return Runtime::OperationResultError;
    const Runtime::OperationResult result =
        services->FileOperations()->CopyInteractive(options);
    delete services;
    return result;
}

struct RunAllResult
{
    BOOL ServicesRegistered;
    BOOL HostServicesRegistered;
    int ServiceCount;
    Runtime::OperationResult NativeProgress;
    Runtime::OperationResult ScriptProgress;
    Runtime::OperationResult QuickRename;
    Runtime::OperationResult CopyInteractive;

    RunAllResult()
        : ServicesRegistered(FALSE),
          HostServicesRegistered(FALSE),
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
    Runtime::RuntimeServices* services = CreatePocRuntimeServices(general);
    if (services == NULL)
        return result;
    result.ServicesRegistered = services->IsRegistered();
    result.HostServicesRegistered = QueryHostUI(general);
    result.ServiceCount = services->Services()->GetCount();

    result.NativeProgress = RunProgressDialogPoc(operations);
    result.ScriptProgress = RunAutomationProgressPoc(operations);
    Commands::ExecuteOptions commandOptions;
    commandOptions.RequireEnabled = FALSE;
    Commands::ICommandService* hostCommands = QueryHostCommands(general);
    result.QuickRename = hostCommands != NULL ? hostCommands->Execute("QuickRename", commandOptions) : services->Commands()->Execute("QuickRename", commandOptions);

    FileOperations::InteractiveOptions fileOptions;
    fileOptions.RequireEnabled = FALSE;
    FileOperations::IFileOperationsService* hostFileOperations = QueryHostFileOperations(general);
    result.CopyInteractive = hostFileOperations != NULL ? hostFileOperations->CopyInteractive(fileOptions) : services->FileOperations()->CopyInteractive(fileOptions);
    delete services;
    return result;
}

} // namespace Poc
} // namespace Salamatrix
