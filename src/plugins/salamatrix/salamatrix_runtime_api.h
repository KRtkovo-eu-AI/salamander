// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_runtime_api.h
    ABI-oriented runtime adapter registry shared by the framework provider and
    language runtime plugins.
*/

#pragma once

#include <windows.h>

#include "salamatrix_runtime_protocol.h"

namespace Salamatrix
{
    namespace Runtime
    {

#define SALAMATRIX_SERVICE_RUNTIME "Salamatrix.Runtime"
#define SALAMATRIX_RUNTIME_VERSION_1_0 0x00010000

        enum RuntimeAdapterFlags
        {
            RuntimeAdapterFlagNone = 0x00000000,
            RuntimeAdapterFlagInProcess = 0x00000001,
            RuntimeAdapterFlagOutOfProcess = 0x00000002,
            RuntimeAdapterFlagBundled = 0x00000004,
            RuntimeAdapterFlagCompatibility = 0x00000008,
            RuntimeAdapterFlagPersistentExtensions = 0x00000010
        };

        struct RuntimeAdapterDescriptor
        {
            DWORD StructSize;
            const char* RuntimeId;
            const char* DisplayName;
            const char* LanguageId;
            const char* FileExtensions;
            DWORD RuntimeVersion;
            DWORD Flags;

            RuntimeAdapterDescriptor()
                : StructSize(sizeof(RuntimeAdapterDescriptor)),
                  RuntimeId(NULL),
                  DisplayName(NULL),
                  LanguageId(NULL),
                  FileExtensions(NULL),
                  RuntimeVersion(0),
                  Flags(RuntimeAdapterFlagNone)
            {
            }
        };

        enum RuntimeExecutionStatus
        {
            RuntimeExecutionStatusNotStarted = 0,
            RuntimeExecutionStatusSucceeded = 1,
            RuntimeExecutionStatusFailed = 2,
            RuntimeExecutionStatusCancelled = 3
        };

        struct RuntimeExecutionResult
        {
            DWORD StructSize;
            RuntimeExecutionStatus Status;
            HRESULT ErrorCode;
            DWORD ExitCode;
            DWORD ProcessId;
            DWORD OutputLength;
            wchar_t Message[512];
            wchar_t Output[4096];

            RuntimeExecutionResult()
                : StructSize(sizeof(RuntimeExecutionResult)),
                  Status(RuntimeExecutionStatusNotStarted),
                  ErrorCode(S_OK),
                  ExitCode(0),
                  ProcessId(0),
                  OutputLength(0)
            {
                Message[0] = L'\0';
                Output[0] = L'\0';
            }
        };

        /// Transitional callback used by compatibility adapters whose actual
        /// engine host still lives in the caller. Modern runtime adapters execute
        /// EntryPoint directly and leave this callback unused.
        typedef BOOL(WINAPI* RuntimeCompatibilityExecuteProc)(
            void* context,
            RuntimeExecutionResult* result);

        struct RuntimeExecutionRequest
        {
            DWORD StructSize;
            const char* ExtensionId;
            const char* CommandId;
            const wchar_t* EntryPoint;
            const wchar_t* WorkingDirectory;
            HWND ParentWindow;
            DWORD TimeoutMs;
            DWORD Flags;
            RuntimeCompatibilityExecuteProc CompatibilityExecute;
            void* CompatibilityContext;

            RuntimeExecutionRequest()
                : StructSize(sizeof(RuntimeExecutionRequest)),
                  ExtensionId(NULL),
                  CommandId(NULL),
                  EntryPoint(NULL),
                  WorkingDirectory(NULL),
                  ParentWindow(NULL),
                  TimeoutMs(120000),
                  Flags(0),
                  CompatibilityExecute(NULL),
                  CompatibilityContext(NULL)
            {
            }
        };

        class IRuntimeAdapter
        {
        public:
            virtual const RuntimeAdapterDescriptor* WINAPI GetDescriptor() const = 0;
            virtual BOOL WINAPI IsAvailable() const = 0;
            virtual BOOL WINAPI SupportsEntryPoint(const char* entryPoint) const = 0;
            virtual BOOL WINAPI Execute(
                const RuntimeExecutionRequest* request,
                RuntimeExecutionResult* result) = 0;

        protected:
            virtual ~IRuntimeAdapter() {}
        };

        class IRuntimeService
        {
        public:
            virtual DWORD WINAPI GetVersion() const = 0;
            virtual BOOL WINAPI RegisterAdapter(IRuntimeAdapter* adapter) = 0;
            virtual BOOL WINAPI UnregisterAdapter(IRuntimeAdapter* adapter) = 0;
            virtual int WINAPI GetAdapterCount() const = 0;
            virtual IRuntimeAdapter* WINAPI GetAdapter(int index) const = 0;
            virtual IRuntimeAdapter* WINAPI FindAdapter(const char* runtimeId, DWORD minimumRuntimeVersion) const = 0;
            virtual IRuntimeAdapter* WINAPI FindAdapterForEntryPoint(const char* entryPoint) const = 0;

        protected:
            virtual ~IRuntimeService() {}
        };

    } // namespace Runtime
} // namespace Salamatrix
