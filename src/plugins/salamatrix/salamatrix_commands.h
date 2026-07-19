// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Runtime for Open Salamander

    salamatrix_commands.h
    MVP command and interactive file-operation API shapes.
*/

#pragma once

#include <string.h>

#include "../shared/spl_gen.h"

namespace Salamatrix
{

#define SALAMATRIX_SERVICE_COMMANDS "Salamatrix.Commands"
#define SALAMATRIX_SERVICE_FILEOPERATIONS "Salamatrix.FileOperations"
#define SALAMATRIX_COMMANDS_VERSION_1_0 0x00010000
#define SALAMATRIX_FILEOPERATIONS_VERSION_1_0 0x00010000

namespace Runtime
{

enum OperationResult
{
    OperationResultOk = 0,
    OperationResultCancel = 1,
    OperationResultError = 2
};

} // namespace Runtime

namespace Commands
{

struct ExecuteOptions
{
    HWND Parent;
    BOOL RequireEnabled;

    ExecuteOptions()
        : Parent(NULL),
          RequireEnabled(TRUE)
    {
    }
};

class ICommandService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual Runtime::OperationResult WINAPI Execute(int salamanderCommandId, const ExecuteOptions& options) = 0;
    virtual Runtime::OperationResult WINAPI Execute(const char* commandId, const ExecuteOptions& options) = 0;

protected:
    virtual ~ICommandService() {}
};

class CommandService : public ICommandService
{
private:
    CSalamanderGeneralAbstract* General;

    static int ResolveCommandId(const char* commandId)
    {
        if (commandId == NULL)
            return -1;
        if (strcmp(commandId, "QuickRename") == 0 || strcmp(commandId, "quick_rename") == 0)
            return SALCMD_QUICKRENAME;
        if (strcmp(commandId, "Copy") == 0 || strcmp(commandId, "copy") == 0)
            return SALCMD_COPY;
        if (strcmp(commandId, "Move") == 0 || strcmp(commandId, "move") == 0 || strcmp(commandId, "MoveRename") == 0)
            return SALCMD_MOVE;
        return -1;
    }

public:
    explicit CommandService(CSalamanderGeneralAbstract* general)
        : General(general)
    {
    }

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_COMMANDS_VERSION_1_0;
    }

    virtual Runtime::OperationResult WINAPI Execute(int salamanderCommandId, const ExecuteOptions& options)
    {
        if (General == NULL || salamanderCommandId < 0)
            return Runtime::OperationResultError;

        if (options.RequireEnabled)
        {
            BOOL enabled = TRUE;
            if (!General->GetSalamanderCommand(salamanderCommandId, NULL, 0, &enabled, NULL) || !enabled)
                return Runtime::OperationResultError;
        }

        General->PostSalamanderCommand(salamanderCommandId);
        return Runtime::OperationResultOk;
    }

    virtual Runtime::OperationResult WINAPI Execute(const char* commandId, const ExecuteOptions& options)
    {
        return Execute(ResolveCommandId(commandId), options);
    }
};

} // namespace Commands

namespace FileOperations
{

struct InteractiveOptions
{
    HWND Parent;
    const char* TargetHint;
    BOOL UseExistingDialog;

    InteractiveOptions()
        : Parent(NULL),
          TargetHint(NULL),
          UseExistingDialog(TRUE)
    {
    }
};

class IFileOperationsService
{
public:
    virtual DWORD WINAPI GetVersion() const = 0;
    virtual Runtime::OperationResult WINAPI RenameInteractive(const InteractiveOptions& options) = 0;
    virtual Runtime::OperationResult WINAPI CopyInteractive(const InteractiveOptions& options) = 0;
    virtual Runtime::OperationResult WINAPI MoveInteractive(const InteractiveOptions& options) = 0;

protected:
    virtual ~IFileOperationsService() {}
};

class FileOperationsService : public IFileOperationsService
{
private:
    Commands::ICommandService* CommandServicePtr;

    Runtime::OperationResult ExecuteExistingCommand(int salamanderCommandId, const InteractiveOptions& options)
    {
        if (CommandServicePtr == NULL || !options.UseExistingDialog)
            return Runtime::OperationResultError;

        Commands::ExecuteOptions executeOptions;
        executeOptions.Parent = options.Parent;
        executeOptions.RequireEnabled = TRUE;
        return CommandServicePtr->Execute(salamanderCommandId, executeOptions);
    }

public:
    explicit FileOperationsService(Commands::ICommandService* commands)
        : CommandServicePtr(commands)
    {
    }

    virtual DWORD WINAPI GetVersion() const
    {
        return SALAMATRIX_FILEOPERATIONS_VERSION_1_0;
    }

    virtual Runtime::OperationResult WINAPI RenameInteractive(const InteractiveOptions& options)
    {
        return ExecuteExistingCommand(SALCMD_QUICKRENAME, options);
    }

    virtual Runtime::OperationResult WINAPI CopyInteractive(const InteractiveOptions& options)
    {
        return ExecuteExistingCommand(SALCMD_COPY, options);
    }

    virtual Runtime::OperationResult WINAPI MoveInteractive(const InteractiveOptions& options)
    {
        return ExecuteExistingCommand(SALCMD_MOVE, options);
    }
};

} // namespace FileOperations
} // namespace Salamatrix
