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

        enum RuntimeExecutionFlags
        {
            RuntimeExecutionFlagNone = 0x00000000,
            RuntimeExecutionFlagPersistentWorker = 0x00000001,
            /// Ask a process adapter to start its common Salamatrix worker
            /// bootstrap before loading EntryPoint. Raw persistent sessions
            /// may omit this bit for transport-level tests.
            RuntimeExecutionFlagUseWorkerBootstrap = 0x00000002,
            /// The worker exits after the entry point returns instead of
            /// waiting for long-lived event callbacks.
            RuntimeExecutionFlagOneShotWorker = 0x00000004
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
            typedef BOOL(WINAPI* RuntimeHostDispatchProc)(
                void* context,
                Protocol::MessageType type,
                ULONGLONG requestId,
                const char* payloadJson,
                char* resultJson,
                DWORD resultCapacity,
                DWORD* resultLength);
            RuntimeHostDispatchProc HostDispatch;
            void* HostDispatchContext;
            /// Optional manifest handler associated with CommandId. This is
            /// appended to preserve the existing request field layout.
            const char* CommandHandler;

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
                  CompatibilityContext(NULL),
                  HostDispatch(NULL),
                  HostDispatchContext(NULL),
                  CommandHandler(NULL)
            {
            }
        };

        /// Bidirectional stdio session used by persistent runtime workers.
        /// Frames are encoded by Protocol::LineCodec and include their trailing
        /// newline. The interface intentionally exposes bytes, not C++ strings,
        /// so Python/PowerShell/PHP bindings can share the same wire contract.
        class IRuntimeSession
        {
        public:
            virtual BOOL WINAPI IsAlive() const = 0;
            virtual BOOL WINAPI SendFrame(
                const char* bytes,
                DWORD count) = 0;
            virtual BOOL WINAPI ReceiveFrame(
                char* bytes,
                DWORD capacity,
                DWORD timeoutMs,
                DWORD* received) = 0;
            virtual BOOL WINAPI Pump(DWORD timeoutMs) = 0;
            virtual void WINAPI Stop() = 0;
            virtual void WINAPI Release() = 0;

            /// Optional exit-code query for bounded one-shot workers.
            /// Older session providers may keep the default FALSE result.
            virtual BOOL WINAPI GetExitCode(DWORD* exitCode) const
            {
                if (exitCode != NULL)
                    *exitCode = 0;
                return FALSE;
            }

            /// Queue an asynchronous event frame without making the caller
            /// wait for a potentially back-pressured worker pipe. Providers
            /// that do not need a queue may keep the compatibility default.
            virtual BOOL WINAPI QueueFrame(const char* bytes, DWORD count)
            {
                return SendFrame(bytes, count);
            }

        protected:
            virtual ~IRuntimeSession() {}
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

            /// Starts a long-lived worker when the adapter supports it. The
            /// default keeps existing one-shot/compatibility adapters source
            /// compatible while modern process adapters opt in explicitly.
            virtual BOOL WINAPI StartPersistent(
                const RuntimeExecutionRequest* request,
                IRuntimeSession** session)
            {
                if (session != NULL)
                    *session = NULL;
                (void)request;
                return FALSE;
            }

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

        /// Small header-only lifecycle helper for standalone runtime-provider
        /// plugins. A provider owns its adapter object; this helper only keeps
        /// the exact broker/adapter pair so Release can unregister the same
        /// pointer without depending on Automation.
        class RuntimeProviderRegistration
        {
        private:
            IRuntimeService* m_pService;
            IRuntimeAdapter* m_pAdapter;
            BOOL m_bRegistered;

        public:
            RuntimeProviderRegistration()
                : m_pService(NULL),
                  m_pAdapter(NULL),
                  m_bRegistered(FALSE)
            {
            }

            BOOL Register(IRuntimeService* service, IRuntimeAdapter* adapter)
            {
                if (m_bRegistered || service == NULL || adapter == NULL)
                    return FALSE;
                if (!service->RegisterAdapter(adapter))
                    return FALSE;
                m_pService = service;
                m_pAdapter = adapter;
                m_bRegistered = TRUE;
                return TRUE;
            }

            void Unregister()
            {
                if (m_bRegistered && m_pService != NULL && m_pAdapter != NULL)
                    m_pService->UnregisterAdapter(m_pAdapter);
                m_pService = NULL;
                m_pAdapter = NULL;
                m_bRegistered = FALSE;
            }

            BOOL IsRegistered() const { return m_bRegistered; }
            IRuntimeService* GetService() const { return m_pService; }
        };

    } // namespace Runtime
} // namespace Salamatrix
