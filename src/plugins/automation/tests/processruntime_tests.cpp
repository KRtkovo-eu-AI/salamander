// SPDX-License-Identifier: GPL-2.0-or-later

#include "../precomp.h"
#include <strsafe.h>
#include <cstdio>
#include <vector>

#include "../salamatrixbridge.h"

namespace
{
int Failures = 0;

struct BootstrapDispatchState
{
    int FilePickerOptionsPayloadCalls;
    int FilePickerOptionsPayloadFilterCalls;
    int FilePickerOptionsPayloadSaveTrueCalls;
    int CommandCalls;
    int StorageCalls;
    int StorageKeysCalls;
    int FileSystemCalls;
    int SubscribeCalls;
    int FileOperationCalls;
    int DialogCalls;
    int FolderPickerControlCalls;
    int FilePickerControlCalls;
    int SideContextCalls;
    int ClipboardCalls;
    int PickerCalls;
    int FolderPickerCalls;
    int RuntimeListCalls;
    int LanguageCalls;
    int AppearanceCalls;
    int MessageBoxCalls;
    int FilePropertiesCalls;
    int CommandRegistrationCalls;
    int CommandStateCalls;
    int SchemaCalls;
    int CreateTabCalls;
    int CloseTabCalls;
    int ReorderTabCalls;
    int MoveTabCalls;
    int SetDetachedCalls;
    bool HandlerRegistrationSeen;
    bool BooleanStorageSeen;
    bool IntegerStorageSeen;
    bool MessageBoxOptionsSeen;
    int NotificationCalls;
    int ControlsCalls;

    BootstrapDispatchState()
        : FilePickerOptionsPayloadCalls(0),
          FilePickerOptionsPayloadFilterCalls(0),
          FilePickerOptionsPayloadSaveTrueCalls(0),
          CommandCalls(0),
          StorageCalls(0),
          StorageKeysCalls(0),
          FileSystemCalls(0),
          SubscribeCalls(0),
          FileOperationCalls(0),
          DialogCalls(0),
          FolderPickerControlCalls(0),
          FilePickerControlCalls(0),
          SideContextCalls(0),
          ClipboardCalls(0),
          PickerCalls(0),
          FolderPickerCalls(0),
          RuntimeListCalls(0),
          LanguageCalls(0),
          AppearanceCalls(0),
          MessageBoxCalls(0),
          FilePropertiesCalls(0),
          CommandRegistrationCalls(0),
          CommandStateCalls(0),
          SchemaCalls(0),
          CreateTabCalls(0),
          CloseTabCalls(0),
          ReorderTabCalls(0),
          MoveTabCalls(0),
          SetDetachedCalls(0),
          HandlerRegistrationSeen(false),
          BooleanStorageSeen(false),
          IntegerStorageSeen(false),
          MessageBoxOptionsSeen(false),
          NotificationCalls(0),
          ControlsCalls(0)
    {
    }
};

void PumpBootstrapUntilComplete(
    Salamatrix::Runtime::IRuntimeSession* session,
    BootstrapDispatchState* state)
{
    const ULONGLONG deadline = GetTickCount64() + 30000;
    while (state->CommandCalls < 2 && session->IsAlive())
    {
        ULONGLONG now = GetTickCount64();
        if (now >= deadline)
            break;
        ULONGLONG remaining = deadline - now;
        DWORD timeout = remaining < 250 ? static_cast<DWORD>(remaining) : 250;
        (void)session->Pump(timeout);
    }
}

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}

BOOL WINAPI WorkerHostDispatch(
    void* context,
    Salamatrix::Runtime::Protocol::MessageType type,
    ULONGLONG requestId,
    const char* payloadJson,
    char* resultJson,
    DWORD resultCapacity,
    DWORD* resultLength)
{
    (void)requestId;
    if (payloadJson == NULL || resultJson == NULL || resultLength == NULL)
        return FALSE;
    BootstrapDispatchState* state =
        static_cast<BootstrapDispatchState*>(context);
    const char* response = "{\"ok\":true,\"method\":\"host.call\"}";
    if (type == Salamatrix::Runtime::Protocol::MessageHello)
    {
        response = "{\"ok\":true,\"protocol\":1}";
    }
    else if (type != Salamatrix::Runtime::Protocol::MessageCall)
    {
        return FALSE;
    }
    else if (strstr(payloadJson, "salamander.commands.execute") != NULL)
    {
        if (state != NULL)
            ++state->CommandCalls;
        response = "{\"ok\":true,\"result\":\"ok\"}";
    }
    else if (strstr(payloadJson, "salamander.fileSystem.addItems") != NULL)
    {
        if (state != NULL)
            ++state->FileSystemCalls;
        response = "{\"ok\":true,\"added\":true,\"addedCount\":2}";
    }
    else if (strstr(payloadJson, "salamander.fileSystem.addItem") != NULL)
    {
        if (state != NULL)
            ++state->FileSystemCalls;
        response = "{\"ok\":true,\"added\":true}";
    }
    else if (strstr(payloadJson, "salamander.ui.notify") != NULL)
    {
        if (state != NULL)
            ++state->NotificationCalls;
        response = "{\"ok\":true,\"shown\":true}";
    }
    else if (strstr(payloadJson, "salamander.ui.controls") != NULL)
    {
        if (state != NULL)
            ++state->ControlsCalls;
        response = "{\"ok\":true,\"shown\":true}";
    }
    else if (strstr(payloadJson, "salamander.ui.fileProperties") != NULL)
    {
        if (state != NULL)
            ++state->FilePropertiesCalls;
        response = "{\"ok\":true,\"shown\":true,\"error\":0}";
    }
    else if (strstr(payloadJson, "salamander.ui.messageBox") != NULL)
    {
        if (state != NULL)
        {
            ++state->MessageBoxCalls;
            if (strstr(payloadJson, "\"buttons\":\"YesNo\"") != NULL &&
                strstr(payloadJson, "\"icon\":\"Question\"") != NULL)
                state->MessageBoxOptionsSeen = true;
        }
        response = "{\"ok\":true,\"result\":1}";
    }
    else if (strstr(payloadJson, "salamander.host.language") != NULL)
    {
        if (state != NULL)
            ++state->LanguageCalls;
        response = "{\"ok\":true,\"language\":\"en\"}";
    }
    else if (strstr(payloadJson, "salamander.host.appearance") != NULL)
    {
        if (state != NULL)
            ++state->AppearanceCalls;
        response = "{\"ok\":true,\"darkMode\":true}";
    }
    else if (strstr(payloadJson, "salamander.commands.register") != NULL)
    {
        if (state != NULL)
        {
            ++state->CommandRegistrationCalls;
            if (strstr(payloadJson, "handler=\"first_handler\"") != NULL ||
                strstr(payloadJson, "\"handler\":\"first_handler\"") != NULL)
                state->HandlerRegistrationSeen = true;
        }
        response = "{\"ok\":true,\"registered\":true}";
    }
    else if (strstr(payloadJson, "salamander.commands.unregister") != NULL)
    {
        if (state != NULL)
            ++state->CommandRegistrationCalls;
        response = "{\"ok\":true,\"unregistered\":true}";
    }
    else if (strstr(payloadJson, "salamander.commands.setState") != NULL)
    {
        if (state != NULL)
            ++state->CommandStateCalls;
        response = "{\"ok\":true,\"updated\":true}";
    }
    else if (strstr(payloadJson, "salamander.storage.schema") != NULL)
    {
        if (state != NULL)
            ++state->SchemaCalls;
        response =
            "{\"ok\":true,\"settings\":["
            "{\"key\":\"autoRefresh\",\"type\":\"boolean\",\"hasDefault\":true,\"default\":true},"
            "{\"key\":\"maxItems\",\"type\":\"integer\",\"hasDefault\":true,\"default\":42}]}";
    }
    else if (strstr(payloadJson, "salamander.storage.keys") != NULL)
    {
        if (state != NULL)
            ++state->StorageKeysCalls;
        response =
            "{\"ok\":true,\"keys\":["
            "{\"key\":\"bootstrap\",\"type\":\"string\"},"
            "{\"key\":\"autoRefresh\",\"type\":\"boolean\"},"
            "{\"key\":\"maxItems\",\"type\":\"integer\"}]}";
    }
    else if (strstr(payloadJson, "salamander.storage.set") != NULL)
    {
        if (state != NULL)
        {
            ++state->StorageCalls;
            if (strstr(payloadJson, "\"value\":true") != NULL)
                state->BooleanStorageSeen = true;
            if (strstr(payloadJson, "\"value\":42") != NULL)
                state->IntegerStorageSeen = true;
        }
        response = "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.storage.get") != NULL)
    {
        if (state != NULL)
            ++state->StorageCalls;
        if (strstr(payloadJson, "\"key\":\"autoRefresh\"") != NULL)
            response = "{\"ok\":true,\"type\":\"boolean\",\"value\":true}";
        else if (strstr(payloadJson, "\"key\":\"maxItems\"") != NULL)
            response = "{\"ok\":true,\"type\":\"integer\",\"value\":42}";
        else
            response = "{\"ok\":true,\"type\":\"string\",\"value\":\"ok\"}";
    }
    else if (strstr(payloadJson, "salamander.events.subscribe") != NULL)
    {
        if (state != NULL)
            ++state->SubscribeCalls;
        response = "{\"ok\":true,\"subscriptionId\":\"41\"}";
    }
    else if (strstr(payloadJson, "salamander.fileOperations.") != NULL)
    {
        if (state != NULL)
            ++state->FileOperationCalls;
        response = "{\"ok\":true,\"result\":\"ok\"}";
    }
    else if (strstr(payloadJson, "salamander.sides.createTab") != NULL)
    {
        if (state != NULL)
            ++state->CreateTabCalls;
        response = "{\"created\":true,\"tabId\":\"1234567890123456789\"}";
    }
    else if (strstr(payloadJson, "salamander.sides.closeTab") != NULL)
    {
        if (state != NULL)
            ++state->CloseTabCalls;
        response = "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.sides.reorderTab") != NULL)
    {
        if (state != NULL)
            ++state->ReorderTabCalls;
        response = "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.sides.moveTab") != NULL)
    {
        if (state != NULL)
            ++state->MoveTabCalls;
        response = "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.sides.setDetached") != NULL)
    {
        if (state != NULL)
            ++state->SetDetachedCalls;
        response = "{\"ok\":true,\"detached\":true}";
    }
    else if (strstr(payloadJson, "salamander.sides.context") != NULL)
    {
        if (state != NULL)
            ++state->SideContextCalls;
        response =
            "{\"ok\":true,\"path\":\"C:\\\\Temp\",\"pathType\":0,"
            "\"selectedCount\":1,\"selectedItems\":[{\"name\":\"seed.txt\","
            "\"path\":\"C:\\\\Temp\\\\seed.txt\",\"size\":\"4\","
            "\"attributes\":32,\"isDirectory\":false}],"
            "\"focusedItem\":{\"name\":\"seed.txt\","
            "\"path\":\"C:\\\\Temp\\\\seed.txt\",\"size\":\"4\","
            "\"attributes\":32,\"isDirectory\":false}}";
    }
    else if (strstr(payloadJson, "salamander.clipboard.copyText") != NULL)
    {
        if (state != NULL)
            ++state->ClipboardCalls;
        response = "{\"ok\":true,\"copied\":true}";
    }
    else if (strstr(payloadJson, "salamander.ui.pickFile") != NULL)
    {
        if (state != NULL)
            ++state->PickerCalls;
        response =
            "{\"ok\":true,\"selected\":true,\"path\":\"C:\\\\Temp\\\\chosen.txt\"}";
    }
    else if (strstr(payloadJson, "salamander.ui.pickFolder") != NULL)
    {
        if (state != NULL)
            ++state->FolderPickerCalls;
        response =
            "{\"ok\":true,\"selected\":true,\"path\":\"C:\\\\Temp\\\\chosen-folder\"}";
    }
    else if (strstr(payloadJson, "salamander.runtimes.list") != NULL)
    {
        if (state != NULL)
            ++state->RuntimeListCalls;
        response =
            "{\"ok\":true,\"runtimes\":[{\"id\":\"Python.CPython\",\"name\":\"CPython\",\"language\":\"python\",\"extensions\":\".py\",\"version\":65536,\"available\":true}]}";
    }
    else if (strstr(payloadJson, "salamander.ai.preview") != NULL)
    {
        response =
            "{\"ok\":true,\"status\":\"succeeded\",\"preview\":true,"
            "\"canRun\":true,\"response\":{\"title\":\"Preview\","
            "\"description\":\"Generated\",\"capabilities\":[],"
            "\"estimatedEffects\":{},\"script\":\"print(1)\"}}";
    }
    else if (strstr(payloadJson, "salamander.ui.dialog.create") != NULL)
    {
        if (state != NULL)
            ++state->DialogCalls;
        response = "{\"ok\":true,\"dialogId\":\"7\"}";
    }
    else if (strstr(payloadJson, "salamander.ui.dialog.column") != NULL)
    {
        if (state != NULL)
            ++state->DialogCalls;
        response = "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.ui.dialog.selection") != NULL)
    {
        if (state != NULL)
            ++state->DialogCalls;
        response = "{\"ok\":true,\"selectedIndex\":0}";
    }
    else if (strstr(payloadJson, "salamander.ui.dialog.add") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.item") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.clearItems") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.validation") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.events") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.show") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.destroy") != NULL)
    {
        if (state != NULL)
        {
            ++state->DialogCalls;
            if (strstr(payloadJson, "folderpicker") != NULL)
                ++state->FolderPickerControlCalls;
            if (strstr(payloadJson, "filepicker") != NULL)
            {
                ++state->FilePickerControlCalls;
                std::string kind;
                if (Salamatrix::Runtime::Protocol::Json::FindStringMember(
                        payloadJson, "kind", &kind) != FALSE &&
                    kind == "filepicker")
                {
                    ++state->FilePickerOptionsPayloadCalls;
                    std::string filter;
                    BOOL save = FALSE;
                    if (Salamatrix::Runtime::Protocol::Json::FindStringMember(
                            payloadJson, "filter", &filter) != FALSE &&
                        filter == "Text files|*.txt|All files|*.*")
                        ++state->FilePickerOptionsPayloadFilterCalls;
                    if (Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                            payloadJson, "save", &save) != FALSE &&
                        save != FALSE)
                        ++state->FilePickerOptionsPayloadSaveTrueCalls;
                }
            }
        }
        response = strstr(payloadJson, "salamander.ui.dialog.show") != NULL
                       ? "{\"ok\":true,\"result\":1}"
                       : strstr(payloadJson, "salamander.ui.dialog.item") != NULL
                             ? "{\"ok\":true,\"itemCount\":1}"
                             : "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.ui.dialog.get") != NULL)
    {
        if (state != NULL)
            ++state->DialogCalls;
        response = "{\"ok\":true,\"text\":\"seed\",\"checked\":false}";
    }
    DWORD responseSize = static_cast<DWORD>(strlen(response));
    if (resultCapacity <= responseSize)
        return FALSE;
    memcpy(resultJson, response, responseSize);
    *resultLength = responseSize;
    return TRUE;
}

bool FindProgram(const wchar_t* name, wchar_t* path, int pathCount)
{
    return SearchPathW(NULL, name, NULL, pathCount, path, NULL) != 0;
}

bool WriteScript(const wchar_t* path, const char* text)
{
    HANDLE file = CreateFileW(
        path,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    bool result = WriteFile(
        file,
        text,
        static_cast<DWORD>(strlen(text)),
        &written,
        NULL) != FALSE;
    CloseHandle(file);
    return result && written == strlen(text);
}

void MakePath(const wchar_t* extension, wchar_t* path, int pathCount)
{
    std::vector<wchar_t> tempPath(SAL_MAX_PATH);
    DWORD length = GetTempPathW(static_cast<DWORD>(tempPath.size()), &tempPath[0]);
    if (length == 0 || length >= tempPath.size())
    {
        path[0] = L'\0';
        return;
    }
    StringCchPrintfW(
        path,
        pathCount,
        L"%ssalamatrix-runtime-%lu%s",
        &tempPath[0],
        GetCurrentProcessId(),
        extension);
}

void RunPythonTests()
{
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(L"python.exe", &interpreter[0], static_cast<int>(interpreter.size())))
    {
        std::fprintf(stderr, "SKIPPED: python.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", &interpreter[0]);

    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L".py", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(&script[0], "print('salamatrix-python-ok')\n"), "write python script");

    CAutomationProcessRuntimeAdapter adapter(
        "Python.CPython",
        "CPython",
        "python",
        ".py",
        L"SALAMATRIX_PYTHON",
        L"python.exe",
        L"python3.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPython);
    Check(adapter.IsAvailable() != FALSE, "python adapter available");
    Check(adapter.SupportsEntryPoint("sample.py") != FALSE, "python extension support");

    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "python execution succeeds");
    Check(result.Status == Salamatrix::Runtime::RuntimeExecutionStatusSucceeded,
          "python result status");
    Check(wcsstr(result.Output, L"salamatrix-python-ok") != NULL,
          "python output captured");

    Check(WriteScript(&script[0], "import time\ntime.sleep(2)\n"), "write timeout script");
    request.TimeoutMs = 100;
    result = Salamatrix::Runtime::RuntimeExecutionResult();
    Check(adapter.Execute(&request, &result) == FALSE, "python timeout returns false");
    Check(result.Status == Salamatrix::Runtime::RuntimeExecutionStatusCancelled,
          "python timeout status");

    Check(WriteScript(
              &script[0],
              "import sys\n"
              "for line in sys.stdin:\n"
              "    sys.stdout.write(line)\n"
              "    sys.stdout.flush()\n"),
          "write persistent python worker");
    request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker;
    request.HostDispatch = WorkerHostDispatch;
    request.HostDispatchContext = NULL;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start persistent python worker");
    if (session != NULL)
    {
        std::string frame;
        Check(
            Salamatrix::Runtime::Protocol::LineCodec::Encode(
                Salamatrix::Runtime::Protocol::MessageCall,
                7,
                "{\"method\":\"host.call\"}",
                &frame) != FALSE,
            "encode persistent worker frame");
        Check(session->SendFrame(frame.c_str(), static_cast<DWORD>(frame.size())) != FALSE,
              "send persistent worker frame");
        Check(session->Pump(5000) != FALSE, "pump host dispatch frame");
        char received[4096];
        DWORD receivedLength = 0;
        Check(session->ReceiveFrame(received, _countof(received), 5000, &receivedLength) != FALSE,
              "receive persistent worker frame");
        Salamatrix::Runtime::Protocol::LineCodec decoder;
        Salamatrix::Runtime::Protocol::Frame receivedFrame;
        BOOL complete = FALSE;
        Check(
            decoder.Append(received, receivedLength, &receivedFrame, &complete) != FALSE &&
            complete != FALSE && receivedFrame.Type ==
                Salamatrix::Runtime::Protocol::MessageResult &&
                receivedFrame.Id == 7,
            "persistent worker echoes frame");
        Salamatrix::Runtime::RuntimeSessionDiagnostic diagnostic;
        Check(session->GetDiagnostic(&diagnostic) != FALSE,
              "persistent worker running diagnostic is available");
        Check(diagnostic.State == Salamatrix::Runtime::RuntimeSessionStateRunning,
              "persistent worker reports running state");
        Check(diagnostic.ProcessId != 0,
              "persistent worker diagnostic preserves process id");
        session->Stop();
        diagnostic = Salamatrix::Runtime::RuntimeSessionDiagnostic();
        Check(session->GetDiagnostic(&diagnostic) != FALSE,
              "stopped worker diagnostic is available");
        Check(diagnostic.State == Salamatrix::Runtime::RuntimeSessionStateStopped,
              "stopped worker reports stopped state");
        Check(diagnostic.Message[0] != L'\0',
              "stopped worker diagnostic has a message");
        session->Release();
    }
    request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagNone;
    DeleteFileW(&script[0]);
}

void RunPowerShellTest()
{
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(L"pwsh.exe", &interpreter[0], static_cast<int>(interpreter.size())))
    {
        std::fprintf(stderr, "SKIPPED: pwsh.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_POWERSHELL", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L".ps1", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(&script[0], "Write-Output 'salamatrix-powershell-ok'\n"),
          "write powershell script");
    CAutomationProcessRuntimeAdapter adapter(
        "PowerShell",
        "PowerShell",
        "powershell",
        ".ps1",
        L"SALAMATRIX_POWERSHELL",
        L"pwsh.exe",
        L"powershell.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPowerShell);
    Check(adapter.IsAvailable() != FALSE, "powershell adapter available");
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "powershell execution succeeds");
    Check(wcsstr(result.Output, L"salamatrix-powershell-ok") != NULL,
          "powershell output captured");
    DeleteFileW(&script[0]);
}

void RunPythonFailureDiagnosticTest()
{
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(
            L"python.exe", &interpreter[0], static_cast<int>(interpreter.size())))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L"-failure.py", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(&script[0], "raise SystemExit(7)\n"),
          "write failing python worker");

    CAutomationProcessRuntimeAdapter adapter(
        "Python.CPython", "CPython", "python", ".py",
        L"SALAMATRIX_PYTHON", L"python.exe", L"python3.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPython);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start failing python worker");
    if (session != NULL)
    {
        for (int attempt = 0; attempt < 40 && session->IsAlive(); ++attempt)
            Sleep(25);
        Salamatrix::Runtime::RuntimeSessionDiagnostic diagnostic;
        Check(session->GetDiagnostic(&diagnostic) != FALSE,
              "failed worker diagnostic is available");
        Check(diagnostic.State == Salamatrix::Runtime::RuntimeSessionStateFailed,
              "failed worker reports failed state");
        Check(diagnostic.ExitCode == 7,
              "failed worker diagnostic preserves nonzero exit code");
        Check(diagnostic.ErrorCode != S_OK,
              "failed worker diagnostic preserves error code");
        Check(diagnostic.Message[0] != L'\0',
              "failed worker diagnostic has a message");
        session->Stop();
        session->Release();
    }
    DeleteFileW(&script[0]);
}

void RunPythonOneShotBootstrapTest()
{
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(
            L"python.exe", &interpreter[0], static_cast<int>(interpreter.size())))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L"-oneshot.py", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(
              &script[0],
              "if Salamander.command_id != 'bootstrap.second':\n"
              "    raise RuntimeError('command context was not propagated')\n"
              "if Salamander.command_handler != 'run_second':\n"
              "    raise RuntimeError('handler context was not propagated')\n"
              "if Salamander.invocation.get('role') != 'viewer':\n"
              "    raise RuntimeError('invocation context was not propagated')\n"
              "if Salamander.invocation.get('path') != 'C:/test/readme.md':\n"
              "    raise RuntimeError('invocation path was not propagated')\n"
              "if Salamander.commands.execute('Copy') != 'ok':\n"
              "    raise RuntimeError('one-shot host call failed')\n"
              "if Salamander.file_system.add_items([\n"
              "        {'id': 'development', 'name': 'Development VM — Running'},\n"
              "        {'id': 'test', 'name': 'Test VM — Stopped'}]) != 2:\n"
              "    raise RuntimeError('file-system item batch call failed')\n"),
          "write one-shot python worker");

    CAutomationProcessRuntimeAdapter adapter(
        "Python.CPython", "CPython", "python", ".py",
        L"SALAMATRIX_PYTHON", L"python.exe", L"python3.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPython);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.CommandId = "bootstrap.second";
    request.CommandHandler = "run_second";
    request.InvocationJson =
        "{\"role\":\"viewer\",\"path\":\"C:/test/readme.md\"}";
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap |
        Salamatrix::Runtime::RuntimeExecutionFlagOneShotWorker;
    request.TimeoutMs = 5000;
    BootstrapDispatchState state;
    request.HostDispatch = WorkerHostDispatch;
    request.HostDispatchContext = &state;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start one-shot python worker");
    if (session != NULL)
    {
        Salamatrix::Runtime::RuntimeSessionDiagnostic diagnostic;
        Check(session->GetDiagnostic(&diagnostic) != FALSE,
              "one-shot worker lifecycle diagnostic is available");
        Check(diagnostic.State == Salamatrix::Runtime::RuntimeSessionStateRunning,
              "one-shot worker reports running state");
        for (int attempt = 0; attempt < 30 && session->IsAlive(); ++attempt)
            session->Pump(250);
        DWORD exitCode = 1;
        Check(session->GetExitCode(&exitCode) != FALSE && exitCode == 0,
              "one-shot python worker exits successfully");
        diagnostic = Salamatrix::Runtime::RuntimeSessionDiagnostic();
        Check(session->GetDiagnostic(&diagnostic) != FALSE,
              "exited worker lifecycle diagnostic is available");
        Check(diagnostic.State == Salamatrix::Runtime::RuntimeSessionStateExited,
              "exited worker reports exited state");
        Check(diagnostic.ExitCode == 0,
              "exited worker diagnostic preserves exit code");
        Check(state.CommandCalls == 1, "one-shot worker host call reached host");
        Check(state.FileSystemCalls == 1,
              "one-shot Python file-system item batch reached host");
        session->Stop();
        session->Release();
    }
    DeleteFileW(&script[0]);
}

void RunPythonDemoLifecycleTest()
{
    std::vector<wchar_t> workerRoot(SAL_MAX_PATH);
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", &workerRoot[0],
        static_cast<DWORD>(workerRoot.size()));
    if (rootLength == 0 || rootLength >= workerRoot.size())
        return;
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(
            L"python.exe", &interpreter[0],
            static_cast<int>(interpreter.size())))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", &interpreter[0]);

    std::vector<wchar_t> entryPoint(SAL_MAX_PATH);
    DWORD entryLength = GetFullPathNameW(
        L"src\\extensions\\demos\\python\\main.py",
        static_cast<DWORD>(entryPoint.size()), &entryPoint[0], NULL);
    Check(entryLength != 0 && entryLength < entryPoint.size(),
          "resolve Python demo lifecycle entry point");
    if (entryLength == 0 || entryLength >= entryPoint.size())
        return;

    CAutomationProcessRuntimeAdapter adapter(
        "Python.CPython", "CPython", "python", ".py",
        L"SALAMATRIX_PYTHON", L"python.exe", L"python3.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPython);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &entryPoint[0];
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker |
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.TimeoutMs = 5000;
    BootstrapDispatchState state;
    request.HostDispatch = WorkerHostDispatch;
    request.HostDispatchContext = &state;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start Python demo lifecycle worker");
    if (session != NULL)
    {
        Check(session->Pump(1000) != FALSE,
              "Python demo lifecycle worker handshake");
        // Once the entry point has returned, a quiet persistent worker waits
        // for events. The unfixed demo instead sends its Run notification here.
        (void)session->Pump(500);
        Check(session->IsAlive() != FALSE,
              "Python demo lifecycle worker remains available for events");
        Check(state.NotificationCalls == 0,
              "Python demo lifecycle activation has no notification side effect");
        Check(state.StorageCalls == 0,
              "Python demo lifecycle activation has no storage side effect");
        Check(state.DialogCalls == 0 && state.MessageBoxCalls == 0,
              "Python demo lifecycle activation has no modal UI side effect");
        session->Stop();
        session->Release();
    }
}

void RunPythonBootstrapTest()
{
    std::vector<wchar_t> workerRoot(SAL_MAX_PATH);
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", &workerRoot[0], static_cast<DWORD>(workerRoot.size()));
    if (rootLength == 0 || rootLength >= workerRoot.size())
    {
        std::fprintf(stderr, "SKIPPED: SALAMATRIX_WORKER_ROOT was not set.\n");
        return;
    }

    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(L"python.exe", &interpreter[0], static_cast<int>(interpreter.size())))
    {
        std::fprintf(stderr, "SKIPPED: python.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L"-bootstrap.py", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(
              &script[0],
              "if Salamander.commands.execute('Copy') != 'ok':\n"
              "    raise RuntimeError('command call failed')\n"
              "if not Salamander.ui.notify('Build finished', timeout_ms=1000):\n"
              "    raise RuntimeError('notification call failed')\n"
              "if Salamander.ui.message_box('Parity', 'Runtime', buttons='YesNo', icon='Question') != 1:\n"
              "    raise RuntimeError('message box options failed')\n"
              "if not Salamander.ui.file_properties('C:/Test/app.exe').get('shown'):\n"
              "    raise RuntimeError('file properties call failed')\n"
              "if Salamander.application.language().get('language') != 'en':\n"
              "    raise RuntimeError('host language call failed')\n"
              "if Salamander.application.appearance().get('darkMode') is not True:\n"
              "    raise RuntimeError('host appearance call failed')\n"
              "Salamander.storage.set('bootstrap', 'ok')\n"
              "if Salamander.storage.get('bootstrap') != 'ok':\n"
              "    raise RuntimeError('storage call failed')\n"
              "Salamander.storage.set('autoRefresh', True)\n"
              "if Salamander.storage.get('autoRefresh') is not True:\n"
              "    raise RuntimeError('boolean storage call failed')\n"
              "Salamander.storage.set('maxItems', 42)\n"
              "if Salamander.storage.get('maxItems') != 42:\n"
              "    raise RuntimeError('integer storage call failed')\n"
              "if not any(item.get('key') == 'autoRefresh' and item.get('type') == 'boolean' for item in Salamander.storage.schema()):\n"
              "    raise RuntimeError('storage schema call failed')\n"
              "storage_keys = Salamander.storage.keys()\n"
              "if not all(any(item.get('key') == key and item.get('type') == value for item in storage_keys) for key, value in [('bootstrap', 'string'), ('autoRefresh', 'boolean'), ('maxItems', 'integer')]):\n"
              "    raise RuntimeError('storage keys call failed')\n"
              "if not Salamander.commands.register('bootstrap.first', 'First', True, False, 0, False, 'first_handler'):\n"
              "    raise RuntimeError('first command registration failed')\n"
              "if not Salamander.commands.register('bootstrap.second', 'Second', True, True):\n"
              "    raise RuntimeError('second command registration failed')\n"
              "if not Salamander.commands.set_state('bootstrap.second', enabled=False):\n"
              "    raise RuntimeError('command disable failed')\n"
              "if not Salamander.commands.set_state('bootstrap.second', visible=False):\n"
              "    raise RuntimeError('command visibility update failed')\n"
              "if not Salamander.commands.unregister('bootstrap.first'):\n"
              "    raise RuntimeError('first command unregister failed')\n"
              "Salamander.events.subscribe('hostStartup', lambda event: None)\n"
              "side_context = Salamander.source_side.context()\n"
              "if side_context.get('selectedCount') != 1 or side_context.get('focusedItem', {}).get('name') != 'seed.txt':\n"
              "    raise RuntimeError('side context call failed')\n"
              "created = Salamander.source_side.create_tab(index=1)\n"
              "if created.get('created') is not True or not str(created.get('tabId', '')).isdigit() or created.get('tabId') != '1234567890123456789':\n"
              "    raise RuntimeError('create tab response failed')\n"
              "if Salamander.source_side.close_tab(created['tabId']) is not True:\n"
              "    raise RuntimeError('close tab response failed')\n"
              "if Salamander.source_side.reorder_tab(created['tabId'], 1) is not True:\n"
              "    raise RuntimeError('reorder tab response failed')\n"
              "if Salamander.source_side.move_tab(created['tabId'], side='target', index=1) is not True:\n"
              "    raise RuntimeError('move tab response failed')\n"
              "if Salamander.source_side.set_detached(True) is not True:\n"
              "    raise RuntimeError('detach response failed')\n"
              "if not Salamander.clipboard.copy_text('seed.txt'):\n"
              "    raise RuntimeError('clipboard call failed')\n"
              "picked = Salamander.ui.pick_file(save=True, filter='Text files|*.txt|All files|*.*')\n"
              "if not picked.get('selected') or not picked.get('path', '').endswith('chosen.txt'):\n"
              "    raise RuntimeError('file picker call failed')\n"
              "folder = Salamander.ui.pick_folder()\n"
              "if not folder.get('selected') or not folder.get('path', '').endswith('chosen-folder'):\n"
              "    raise RuntimeError('folder picker call failed')\n"
              "if not any(item.get('id') == 'Python.CPython' for item in Salamander.runtimes.list()):\n"
              "    raise RuntimeError('runtime list call failed')\n"
              "if not Salamander.ai.preview('list files', runtime='Python.CPython', existing_script='print(1)', feedback='keep originals').get('canRun'):\n"
              "    raise RuntimeError('ai preview call failed')\n"
              "if Salamander.file_operations.refresh() != 'ok':\n"
              "    raise RuntimeError('file operation call failed')\n"
              "dialog = Salamander.ui.dialog('Bootstrap', 640, 420)\n"
              "dialog.add_control('label', 'label', 'Hello', layout={'x': 12, 'y': 10, 'width': 180, 'height': 16})\n"
              "dialog.add_textbox('value', 'seed', multiline=True)\n"
              "dialog.add_folder_picker('folder', 'C:\\\\Temp')\n"
              "dialog.add_file_picker('file', 'C:\\\\Temp\\\\seed.txt', filter='Text files|*.txt|All files|*.*', save=True)\n"
              "dialog.set_validation('value', True, 'Value is required')\n"
              "dialog.on_change(lambda event: None)\n"
              "dialog.add_radio_button('radio', 'Choice', True)\n"
              "dialog.add_combo_box('combo', 'Option')\n"
              "dialog.add_list_view('list')\n"
              "dialog.add_column('list', 'Name', 220)\n"
              "dialog.add_item('list', 'entry')\n"
              "dialog.set_selected_index('list', 0)\n"
              "dialog.add_tree_view('tree')\n"
              "if dialog.add_item('combo', 'Option 2') != 1:\n"
              "    raise RuntimeError('dialog item call failed')\n"
              "dialog.add_item('list', 'Item 1')\n"
              "dialog.add_item('tree', 'Node 1')\n"
              "dialog.clear_items('list')\n"
              "dialog.add_button('ok', 'OK', 1, keep_open=True)\n"
              "if dialog.show() != 1:\n"
              "    raise RuntimeError('dialog show failed')\n"
              "if dialog.get('value').get('text') != 'seed':\n"
              "    raise RuntimeError('dialog get failed')\n"
              "dialog.close()\n"
              "if Salamander.commands.execute('__processruntime_bootstrap_complete__') != 'ok':\n"
              "    raise RuntimeError('bootstrap completion call failed')\n"),
          "write python bootstrap worker");

    CAutomationProcessRuntimeAdapter adapter(
        "Python.CPython",
        "CPython",
        "python",
        ".py",
        L"SALAMATRIX_PYTHON",
        L"python.exe",
        L"python3.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPython);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker |
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.TimeoutMs = 5000;
    BootstrapDispatchState state;
    request.HostDispatch = WorkerHostDispatch;
    request.HostDispatchContext = &state;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start python bootstrap worker");
    if (session != NULL)
    {
        // Pump until the script has completed the complete host-call sequence.
        // A persistent worker can issue many calls after the subscription call;
        // stopping at SubscribeCalls would race the script and send shutdown
        // while it is still waiting for the next response.
        PumpBootstrapUntilComplete(session, &state);
        Check(state.CommandCalls == 2, "bootstrap command and completion calls reached host");
        Check(state.NotificationCalls == 1, "bootstrap notification call reached host");
        Check(state.MessageBoxCalls == 1, "bootstrap message box call reached host");
        Check(state.FilePropertiesCalls == 1, "bootstrap file properties call reached host");
        Check(state.MessageBoxOptionsSeen, "bootstrap message box options reached host");
        Check(state.LanguageCalls == 1, "bootstrap host language call reached host");
        Check(state.AppearanceCalls == 1, "bootstrap host appearance call reached host");
        Check(state.CommandRegistrationCalls == 3, "bootstrap multiple command registrations reached host");
        Check(state.CommandStateCalls == 2, "bootstrap command state updates reached host");
        Check(state.HandlerRegistrationSeen, "bootstrap command handler reached host");
        Check(state.StorageCalls == 6, "bootstrap storage calls reached host");
        Check(state.BooleanStorageSeen, "bootstrap boolean storage reached host");
        Check(state.IntegerStorageSeen, "bootstrap integer storage reached host");
        Check(state.SchemaCalls == 1, "bootstrap storage schema reached host");
        Check(state.StorageKeysCalls == 1, "bootstrap storage keys reached host");
        Check(state.SubscribeCalls == 1, "bootstrap event subscription reached host");
        Check(state.SideContextCalls == 1, "bootstrap side context reached host");
        Check(state.CreateTabCalls == 1, "bootstrap createTab reached host once");
        Check(state.CloseTabCalls == 1, "bootstrap closeTab reached host once");
        Check(state.ReorderTabCalls == 1, "bootstrap reorderTab reached host once");
        Check(state.MoveTabCalls == 1, "bootstrap moveTab reached host once");
        Check(state.SetDetachedCalls == 1, "bootstrap setDetached reached host once");
        Check(state.ClipboardCalls == 1, "bootstrap clipboard reached host");
        Check(state.PickerCalls == 1, "bootstrap file picker reached host");
        Check(state.FolderPickerCalls == 1, "bootstrap folder picker reached host");
        Check(state.RuntimeListCalls == 1, "bootstrap runtime list reached host");
        Check(state.FileOperationCalls == 1, "bootstrap file operation reached host");
        Check(state.DialogCalls == 22, "bootstrap dialog calls reached host");
        Check(state.FolderPickerControlCalls == 1, "bootstrap folder picker control reached host");
        Check(state.FilePickerControlCalls == 1, "bootstrap editable file picker control reached host");
        Check(state.FilePickerOptionsPayloadCalls == 1,
              "bootstrap filepicker options payload reached host");
        Check(state.FilePickerOptionsPayloadFilterCalls == 1,
              "bootstrap filepicker options payload includes filter");
        Check(state.FilePickerOptionsPayloadSaveTrueCalls == 1,
              "bootstrap filepicker options payload includes save=true");
        std::string shutdown;
        Check(
            Salamatrix::Runtime::Protocol::LineCodec::Encode(
                Salamatrix::Runtime::Protocol::MessageShutdown,
                0,
                "{}",
                &shutdown) != FALSE,
            "encode bootstrap shutdown");
        Check(session->SendFrame(shutdown.c_str(), static_cast<DWORD>(shutdown.size())) != FALSE,
              "send bootstrap shutdown");
        for (int attempt = 0; attempt < 80 && session->IsAlive(); ++attempt)
            Sleep(25);
        Check(session->IsAlive() == FALSE, "bootstrap worker exits after shutdown");
        session->Stop();
        session->Release();
    }
    DeleteFileW(&script[0]);
}

void RunPowerShellBootstrapTest()
{
    std::vector<wchar_t> workerRoot(SAL_MAX_PATH);
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", &workerRoot[0], static_cast<DWORD>(workerRoot.size()));
    if (rootLength == 0 || rootLength >= workerRoot.size())
        return;
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(L"pwsh.exe", &interpreter[0], static_cast<int>(interpreter.size())))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_POWERSHELL", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L"-bootstrap.ps1", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(
              &script[0],
              "if ($Salamander.commands.Execute('Copy') -ne 'ok') { throw 'command call failed' }\n"
              "if ($Salamander.ui.MessageBox('Parity', 'Runtime', 'YesNo', 'Question') -ne 1) { throw 'message box options failed' }\n"
              "if (-not ($Salamander.ui.FileProperties('C:/Test/app.exe')).shown) { throw 'file properties call failed' }\n"
              "if ($Salamander.application.Language().language -ne 'en') { throw 'host language call failed' }\n"
              "if ($Salamander.application.Appearance().darkMode -ne $true) { throw 'host appearance call failed' }\n"
              "$Salamander.storage.Set('bootstrap', 'ok')\n"
              "if ($Salamander.storage.Get('bootstrap') -ne 'ok') { throw 'storage call failed' }\n"
              "$Salamander.storage.Set('autoRefresh', $true)\n"
              "if ($Salamander.storage.Get('autoRefresh') -ne $true) { throw 'boolean storage call failed' }\n"
              "$Salamander.storage.Set('maxItems', 42)\n"
              "if ($Salamander.storage.Get('maxItems') -ne 42) { throw 'integer storage call failed' }\n"
              "if (-not ($Salamander.storage.Schema() | Where-Object { $_.key -eq 'autoRefresh' -and $_.type -eq 'boolean' })) { throw 'storage schema call failed' }\n"
              "$storageKeys = @($Salamander.storage.Keys())\n"
              "foreach ($expected in @(@{ key = 'bootstrap'; type = 'string' }, @{ key = 'autoRefresh'; type = 'boolean' }, @{ key = 'maxItems'; type = 'integer' })) { if (-not ($storageKeys | Where-Object { $_.key -eq $expected.key -and $_.type -eq $expected.type })) { throw 'storage keys call failed' } }\n"
              "if (-not $Salamander.commands.Register('bootstrap.first', 'First', $true, $false)) { throw 'first command registration failed' }\n"
              "if (-not $Salamander.commands.Register('bootstrap.second', 'Second', $true, $true)) { throw 'second command registration failed' }\n"
              "if (-not $Salamander.commands.SetState('bootstrap.second', $false, $null)) { throw 'command disable failed' }\n"
              "if (-not $Salamander.commands.SetState('bootstrap.second', $null, $false)) { throw 'command visibility update failed' }\n"
              "if (-not $Salamander.commands.Unregister('bootstrap.first')) { throw 'first command unregister failed' }\n"
              "$null = $Salamander.events.Subscribe('hostStartup', { param($event) })\n"
              "$sideContext = $Salamander.source_side.Context()\n"
              "if ($sideContext.selectedCount -ne 1 -or $sideContext.focusedItem.name -ne 'seed.txt') { throw 'side context call failed' }\n"
              "if (-not $Salamander.clipboard.CopyText('seed.txt')) { throw 'clipboard call failed' }\n"
              "$picked = $Salamander.ui.PickFile($true, '', 'Text files|*.txt|All files|*.*', '')\n"
              "if (-not $picked.selected -or -not $picked.path.EndsWith('chosen.txt')) { throw 'file picker call failed' }\n"
              "$folder = $Salamander.ui.PickFolder('', '')\n"
              "if (-not $folder.selected -or -not $folder.path.EndsWith('chosen-folder')) { throw 'folder picker call failed' }\n"
              "if (-not ($Salamander.runtimes.List() | Where-Object { $_.id -eq 'Python.CPython' })) { throw 'runtime list call failed' }\n"
              "if (-not $Salamander.ai.Preview('list files', $null, $null, 'PowerShell', 'Write-Output 1', 'keep originals').canRun) { throw 'ai preview call failed' }\n"
              "if ($Salamander.file_operations.Refresh() -ne 'ok') { throw 'file operation call failed' }\n"
              "if ($Salamander.file_system.AddItems(@(@{ id = 'one'; name = 'One' }, @{ id = 'two'; name = 'Two' })) -ne 2) { throw 'file-system item batch failed' }\n"
              "$dialog = $Salamander.ui.Dialog('Bootstrap', 640, 420)\n"
              "$dialog.AddControl('label', 'label', 'Hello', $false, $false, 0, @{ x = 12; y = 10; width = 180; height = 16 })\n"
              "$dialog.AddTextBox('value', 'seed', $false, $true)\n"
              "$dialog.AddFolderPicker('folder', 'C:\\Temp')\n"
              "$dialog.AddFilePicker('file', 'C:\\Temp\\seed.txt', 'Text files|*.txt|All files|*.*', $true)\n"
              "$dialog.SetValidation('value', $true, 'Value is required')\n"
              "$dialog.OnChange({ param($event) })\n"
              "$dialog.AddRadioButton('radio', 'Choice', $true)\n"
              "$dialog.AddComboBox('combo', 'Option')\n"
              "$dialog.AddListView('list')\n"
              "$dialog.AddColumn('list', 'Name', 220)\n"
              "$dialog.AddItem('list', 'entry')\n"
              "$dialog.SetSelectedIndex('list', 0)\n"
              "$dialog.AddTreeView('tree')\n"
              "if ($dialog.AddItem('combo', 'Option 2') -ne 1) { throw 'dialog item call failed' }\n"
              "$null = $dialog.AddItem('list', 'Item 1')\n"
              "$null = $dialog.AddItem('tree', 'Node 1')\n"
              "$dialog.ClearItems('list')\n"
              "$dialog.AddButton('ok', 'OK', 1, $true)\n"
              "if ($dialog.Show() -ne 1) { throw 'dialog show failed' }\n"
              "if ($dialog.Get('value').text -ne 'seed') { throw 'dialog get failed' }\n"
              "$dialog.Close()\n"
              "if ($Salamander.commands.execute('__processruntime_bootstrap_complete__') -ne 'ok') { throw 'bootstrap completion call failed' }\n"),
          "write powershell bootstrap worker");
    CAutomationProcessRuntimeAdapter adapter(
        "PowerShell", "PowerShell", "powershell", ".ps1",
        L"SALAMATRIX_POWERSHELL", L"pwsh.exe", L"powershell.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPowerShell);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker |
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.TimeoutMs = 5000;
    BootstrapDispatchState state;
    request.HostDispatch = WorkerHostDispatch;
    request.HostDispatchContext = &state;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start powershell bootstrap worker");
    if (session != NULL)
    {
        PumpBootstrapUntilComplete(session, &state);
        Check(state.CommandCalls == 2, "powershell bootstrap command and completion calls");
        Check(state.MessageBoxCalls == 1, "powershell message box call");
        Check(state.FilePropertiesCalls == 1, "powershell file properties call");
        Check(state.MessageBoxOptionsSeen, "powershell message box options");
        Check(state.LanguageCalls == 1, "powershell host language call");
        Check(state.AppearanceCalls == 1, "powershell host appearance call");
        Check(state.CommandRegistrationCalls == 3, "powershell multiple command registrations");
        Check(state.CommandStateCalls == 2, "powershell command state updates");
        Check(state.StorageCalls == 6, "powershell typed storage calls");
        Check(state.BooleanStorageSeen, "powershell boolean storage reached host");
        Check(state.IntegerStorageSeen, "powershell integer storage reached host");
        Check(state.SchemaCalls == 1, "powershell storage schema reached host");
        Check(state.StorageKeysCalls == 1, "powershell storage keys reached host");
        Check(state.SubscribeCalls == 1, "powershell bootstrap event subscription");
        Check(state.FileSystemCalls == 1, "powershell file-system item batch reached host");
        Check(state.SideContextCalls == 1, "powershell bootstrap side context");
        Check(state.ClipboardCalls == 1, "powershell bootstrap clipboard");
        Check(state.PickerCalls == 1, "powershell bootstrap file picker");
        Check(state.FolderPickerCalls == 1, "powershell bootstrap folder picker");
        Check(state.RuntimeListCalls == 1, "powershell bootstrap runtime list");
        Check(state.FileOperationCalls == 1, "powershell bootstrap file operation");
        Check(state.DialogCalls == 22, "powershell bootstrap dialog calls");
        Check(state.FolderPickerControlCalls == 1, "powershell folder picker control");
        Check(state.FilePickerControlCalls == 1, "powershell editable file picker control");
        Check(state.FilePickerOptionsPayloadCalls == 1,
              "powershell filepicker options payload reached host");
        Check(state.FilePickerOptionsPayloadFilterCalls == 1,
              "powershell filepicker options payload includes filter");
        Check(state.FilePickerOptionsPayloadSaveTrueCalls == 1,
              "powershell filepicker options payload includes save=true");
        std::string shutdown;
        Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageShutdown, 0, "{}", &shutdown);
        session->SendFrame(shutdown.c_str(), static_cast<DWORD>(shutdown.size()));
        for (int attempt = 0; attempt < 80 && session->IsAlive(); ++attempt)
            Sleep(25);
        Check(session->IsAlive() == FALSE, "powershell bootstrap exits after shutdown");
        session->Stop();
        session->Release();
    }
    DeleteFileW(&script[0]);
}

void RunPhpBootstrapTest()
{
    std::vector<wchar_t> workerRoot(SAL_MAX_PATH);
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", &workerRoot[0], static_cast<DWORD>(workerRoot.size()));
    if (rootLength == 0 || rootLength >= workerRoot.size())
        return;
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(L"php.exe", &interpreter[0], static_cast<int>(interpreter.size())))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_PHP", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L"-bootstrap.php", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(
              &script[0],
              "<?php\n"
              "if ($Salamander->commands->execute('Copy') !== 'ok') throw new Exception('command call failed');\n"
              "if ($Salamander->ui->messageBox('Parity', 'Runtime', 'YesNo', 'Question') !== 1) throw new Exception('message box options failed');\n"
              "if (empty($Salamander->ui->fileProperties('C:/Test/app.exe')['shown'])) throw new Exception('file properties call failed');\n"
              "if (!$Salamander->ui->controls()) throw new Exception('controls showcase call failed');\n"
              "if ($Salamander->application->language()['language'] !== 'en') throw new Exception('host language call failed');\n"
              "if ($Salamander->application->appearance()['darkMode'] !== true) throw new Exception('host appearance call failed');\n"
              "$Salamander->storage->set('bootstrap', 'ok');\n"
              "if ($Salamander->storage->get('bootstrap') !== 'ok') throw new Exception('storage call failed');\n"
              "$Salamander->storage->set('autoRefresh', true);\n"
              "if ($Salamander->storage->get('autoRefresh') !== true) throw new Exception('boolean storage call failed');\n"
              "$Salamander->storage->set('maxItems', 42);\n"
              "if ($Salamander->storage->get('maxItems') !== 42) throw new Exception('integer storage call failed');\n"
              "$schema = $Salamander->storage->schema();\n"
              "if (empty(array_filter($schema, function($item) { return $item['key'] === 'autoRefresh' && $item['type'] === 'boolean'; }))) throw new Exception('storage schema call failed');\n"
              "$storageKeys = $Salamander->storage->keys();\n"
              "foreach (array('bootstrap' => 'string', 'autoRefresh' => 'boolean', 'maxItems' => 'integer') as $key => $type) { if (empty(array_filter($storageKeys, function($item) use ($key, $type) { return $item['key'] === $key && $item['type'] === $type; }))) throw new Exception('storage keys call failed'); }\n"
              "if (!$Salamander->commands->register('bootstrap.first', 'First', true, false)) throw new Exception('first command registration failed');\n"
              "if (!$Salamander->commands->register('bootstrap.second', 'Second', true, true)) throw new Exception('second command registration failed');\n"
              "if (!$Salamander->commands->setState('bootstrap.second', false, null)) throw new Exception('command disable failed');\n"
              "if (!$Salamander->commands->setState('bootstrap.second', null, false)) throw new Exception('command visibility update failed');\n"
              "if (!$Salamander->commands->unregister('bootstrap.first')) throw new Exception('first command unregister failed');\n"
              "$Salamander->events->subscribe('hostStartup', function($event) {});\n"
              "$sideContext = $Salamander->source_side->context();\n"
              "if ($sideContext['selectedCount'] !== 1 || $sideContext['focusedItem']['name'] !== 'seed.txt') throw new Exception('side context call failed');\n"
              "if (!$Salamander->clipboard->copyText('seed.txt')) throw new Exception('clipboard call failed');\n"
              "$picked = $Salamander->ui->pickFile(true, '', 'Text files|*.txt|All files|*.*', '');\n"
              "if (empty($picked['selected']) || substr($picked['path'], -10) !== 'chosen.txt') throw new Exception('file picker call failed');\n"
              "$folder = $Salamander->ui->pickFolder('', '');\n"
              "if (empty($folder['selected']) || substr($folder['path'], -13) !== 'chosen-folder') throw new Exception('folder picker call failed');\n"
              "$runtimes = $Salamander->runtimes->list();\n"
              "if (empty($runtimes) || $runtimes[0]['id'] !== 'Python.CPython') throw new Exception('runtime list call failed');\n"
              "if (!$Salamander->ai->preview('list files', null, null, 'PHP.CLI', '<?php echo 1; ?>', 'keep originals')['canRun']) throw new Exception('ai preview call failed');\n"
              "if ($Salamander->file_operations->refresh() !== 'ok') throw new Exception('file operation call failed');\n"
              "if ($Salamander->file_system->add_items(array(array('id'=>'one', 'name'=>'One'), array('id'=>'two', 'name'=>'Two'))) !== 2) throw new Exception('file-system item batch failed');\n"
              "$dialog = $Salamander->ui->dialog('Bootstrap', 640, 420);\n"
              "$dialog->addControl('label', 'label', 'Hello', false, false, 0, array('x' => 12, 'y' => 10, 'width' => 180, 'height' => 16));\n"
              "$dialog->addTextBox('value', 'seed', false, true);\n"
              "$dialog->addFolderPicker('folder', 'C:\\\\Temp');\n"
              "$dialog->addFilePicker('file', 'C:\\\\Temp\\\\seed.txt', 'Text files|*.txt|All files|*.*', true);\n"
              "$dialog->setValidation('value', true, 'Value is required');\n"
              "$dialog->onChange(function($event) {});\n"
              "$dialog->addRadioButton('radio', 'Choice', true);\n"
              "$dialog->addComboBox('combo', 'Option');\n"
              "$dialog->addListView('list');\n"
              "$dialog->addColumn('list', 'Name', 220);\n"
              "$dialog->addItem('list', 'entry');\n"
              "$dialog->setSelectedIndex('list', 0);\n"
              "$dialog->addTreeView('tree');\n"
              "if ($dialog->addItem('combo', 'Option 2') !== 1) throw new Exception('dialog item call failed');\n"
              "$dialog->addItem('list', 'Item 1');\n"
              "$dialog->addItem('tree', 'Node 1');\n"
              "$dialog->clearItems('list');\n"
              "$dialog->addButton('ok', 'OK', 1, true);\n"
              "if ($dialog->show() !== 1) throw new Exception('dialog show failed');\n"
              "if ($dialog->get('value')['text'] !== 'seed') throw new Exception('dialog get failed');\n"
              "$dialog->close();\n"
              "if ($Salamander->commands->execute('__processruntime_bootstrap_complete__') !== 'ok') throw new Exception('bootstrap completion call failed');\n"
              "?>\n"),
          "write php bootstrap worker");
    CAutomationProcessRuntimeAdapter adapter(
        "PHP.CLI", "PHP", "php", ".php", L"SALAMATRIX_PHP", L"php.exe", NULL,
        CAutomationProcessRuntimeAdapter::ProcessKindPhp);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker |
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.TimeoutMs = 5000;
    BootstrapDispatchState state;
    request.HostDispatch = WorkerHostDispatch;
    request.HostDispatchContext = &state;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    Check(adapter.StartPersistent(&request, &session) != FALSE && session != NULL,
          "start php bootstrap worker");
    if (session != NULL)
    {
        PumpBootstrapUntilComplete(session, &state);
        Check(state.CommandCalls == 2, "php bootstrap command and completion calls");
        Check(state.MessageBoxCalls == 1, "php message box call");
        Check(state.FilePropertiesCalls == 1, "php file properties call");
        Check(state.MessageBoxOptionsSeen, "php message box options");
        Check(state.ControlsCalls == 1, "php controls showcase call");
        Check(state.LanguageCalls == 1, "php host language call");
        Check(state.AppearanceCalls == 1, "php host appearance call");
        Check(state.CommandRegistrationCalls == 3, "php multiple command registrations");
        Check(state.CommandStateCalls == 2, "php command state updates");
        Check(state.StorageCalls == 6, "php typed storage calls");
        Check(state.BooleanStorageSeen, "php boolean storage reached host");
        Check(state.IntegerStorageSeen, "php integer storage reached host");
        Check(state.SchemaCalls == 1, "php storage schema reached host");
        Check(state.StorageKeysCalls == 1, "php storage keys reached host");
        Check(state.FileSystemCalls == 1, "php file-system item batch reached host");
        Check(state.SubscribeCalls == 1, "php bootstrap event subscription");
        Check(state.SideContextCalls == 1, "php bootstrap side context");
        Check(state.ClipboardCalls == 1, "php bootstrap clipboard");
        Check(state.PickerCalls == 1, "php bootstrap file picker");
        Check(state.FolderPickerCalls == 1, "php bootstrap folder picker");
        Check(state.RuntimeListCalls == 1, "php bootstrap runtime list");
        Check(state.FileOperationCalls == 1, "php bootstrap file operation");
        Check(state.DialogCalls == 22, "php bootstrap dialog calls");
        Check(state.FolderPickerControlCalls == 1, "php folder picker control");
        Check(state.FilePickerControlCalls == 1, "php editable file picker control");
        Check(state.FilePickerOptionsPayloadCalls == 1,
              "php filepicker options payload reached host");
        Check(state.FilePickerOptionsPayloadFilterCalls == 1,
              "php filepicker options payload includes filter");
        Check(state.FilePickerOptionsPayloadSaveTrueCalls == 1,
              "php filepicker options payload includes save=true");
        std::string shutdown;
        Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageShutdown, 0, "{}", &shutdown);
        session->SendFrame(shutdown.c_str(), static_cast<DWORD>(shutdown.size()));
        for (int attempt = 0; attempt < 80 && session->IsAlive(); ++attempt)
            Sleep(25);
        Check(session->IsAlive() == FALSE, "php bootstrap exits after shutdown");
        session->Stop();
        session->Release();
    }
    DeleteFileW(&script[0]);
}

void RunPhpTest()
{
    std::vector<wchar_t> interpreter(SAL_MAX_PATH);
    if (!FindProgram(L"php.exe", &interpreter[0], static_cast<int>(interpreter.size())))
    {
        std::fprintf(stderr, "SKIPPED: php.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PHP", &interpreter[0]);
    std::vector<wchar_t> script(SAL_MAX_PATH);
    MakePath(L".php", &script[0], static_cast<int>(script.size()));
    Check(WriteScript(&script[0], "<?php echo 'salamatrix-php-ok\\n'; ?>\n"),
          "write php script");
    CAutomationProcessRuntimeAdapter adapter(
        "PHP.CLI",
        "PHP",
        "php",
        ".php",
        L"SALAMATRIX_PHP",
        L"php.exe",
        NULL,
        CAutomationProcessRuntimeAdapter::ProcessKindPhp);
    Check(adapter.IsAvailable() != FALSE, "php adapter available");
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = &script[0];
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "php execution succeeds");
    Check(wcsstr(result.Output, L"salamatrix-php-ok") != NULL,
          "php output captured");
    DeleteFileW(&script[0]);
}
} // namespace

int main()
{
    RunPythonTests();
    RunPythonFailureDiagnosticTest();
    RunPythonOneShotBootstrapTest();
    RunPythonDemoLifecycleTest();
    RunPythonBootstrapTest();
    RunPowerShellBootstrapTest();
    RunPhpBootstrapTest();
    RunPowerShellTest();
    RunPhpTest();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d process runtime test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All process runtime tests passed.\n");
    return 0;
}
