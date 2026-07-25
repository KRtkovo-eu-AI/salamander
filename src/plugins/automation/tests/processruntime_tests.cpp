// SPDX-License-Identifier: GPL-2.0-or-later

#include "../precomp.h"
#include <strsafe.h>
#include <cstdio>

#include "../salamatrixbridge.h"

namespace
{
int Failures = 0;

struct BootstrapDispatchState
{
    int CommandCalls;
    int StorageCalls;
    int SubscribeCalls;
    int FileOperationCalls;
    int DialogCalls;
    int SideContextCalls;

    BootstrapDispatchState()
        : CommandCalls(0),
          StorageCalls(0),
          SubscribeCalls(0),
          FileOperationCalls(0),
          DialogCalls(0),
          SideContextCalls(0)
    {
    }
};

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
    else if (strstr(payloadJson, "salamander.storage.set") != NULL)
    {
        if (state != NULL)
            ++state->StorageCalls;
        response = "{\"ok\":true}";
    }
    else if (strstr(payloadJson, "salamander.storage.get") != NULL)
    {
        if (state != NULL)
            ++state->StorageCalls;
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
    else if (strstr(payloadJson, "salamander.ui.dialog.create") != NULL)
    {
        if (state != NULL)
            ++state->DialogCalls;
        response = "{\"ok\":true,\"dialogId\":\"7\"}";
    }
    else if (strstr(payloadJson, "salamander.ui.dialog.add") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.show") != NULL ||
             strstr(payloadJson, "salamander.ui.dialog.destroy") != NULL)
    {
        if (state != NULL)
            ++state->DialogCalls;
        response = strstr(payloadJson, "salamander.ui.dialog.show") != NULL
                       ? "{\"ok\":true,\"result\":1}"
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
    wchar_t tempPath[MAX_PATH];
    DWORD length = GetTempPathW(_countof(tempPath), tempPath);
    if (length == 0 || length >= _countof(tempPath))
    {
        path[0] = L'\0';
        return;
    }
    StringCchPrintfW(
        path,
        pathCount,
        L"%ssalamatrix-runtime-%lu%s",
        tempPath,
        GetCurrentProcessId(),
        extension);
}

void RunPythonTests()
{
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"python.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: python.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", interpreter);

    wchar_t script[MAX_PATH];
    MakePath(L".py", script, _countof(script));
    Check(WriteScript(script, "print('salamatrix-python-ok')\n"), "write python script");

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
    request.EntryPoint = script;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "python execution succeeds");
    Check(result.Status == Salamatrix::Runtime::RuntimeExecutionStatusSucceeded,
          "python result status");
    Check(wcsstr(result.Output, L"salamatrix-python-ok") != NULL,
          "python output captured");

    Check(WriteScript(script, "import time\ntime.sleep(2)\n"), "write timeout script");
    request.TimeoutMs = 100;
    result = Salamatrix::Runtime::RuntimeExecutionResult();
    Check(adapter.Execute(&request, &result) == FALSE, "python timeout returns false");
    Check(result.Status == Salamatrix::Runtime::RuntimeExecutionStatusCancelled,
          "python timeout status");

    Check(WriteScript(
              script,
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
        session->Stop();
        session->Release();
    }
    request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagNone;
    DeleteFileW(script);
}

void RunPowerShellTest()
{
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"pwsh.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: pwsh.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_POWERSHELL", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L".ps1", script, _countof(script));
    Check(WriteScript(script, "Write-Output 'salamatrix-powershell-ok'\n"),
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
    request.EntryPoint = script;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "powershell execution succeeds");
    Check(wcsstr(result.Output, L"salamatrix-powershell-ok") != NULL,
          "powershell output captured");
    DeleteFileW(script);
}

void RunPythonBootstrapTest()
{
    wchar_t workerRoot[MAX_PATH * 4];
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", workerRoot, _countof(workerRoot));
    if (rootLength == 0 || rootLength >= _countof(workerRoot))
    {
        std::fprintf(stderr, "SKIPPED: SALAMATRIX_WORKER_ROOT was not set.\n");
        return;
    }

    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"python.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: python.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L"-bootstrap.py", script, _countof(script));
    Check(WriteScript(
              script,
              "if Salamander.commands.execute('Copy') != 'ok':\n"
              "    raise RuntimeError('command call failed')\n"
              "Salamander.storage.set('bootstrap', 'ok')\n"
              "if Salamander.storage.get('bootstrap') != 'ok':\n"
              "    raise RuntimeError('storage call failed')\n"
              "Salamander.events.subscribe('hostStartup', lambda event: None)\n"
              "side_context = Salamander.source_side.context()\n"
              "if side_context.get('selectedCount') != 1 or side_context.get('focusedItem', {}).get('name') != 'seed.txt':\n"
              "    raise RuntimeError('side context call failed')\n"
              "if Salamander.file_operations.refresh() != 'ok':\n"
              "    raise RuntimeError('file operation call failed')\n"
              "dialog = Salamander.ui.dialog('Bootstrap')\n"
              "dialog.add_label('label', 'Hello')\n"
              "dialog.add_textbox('value', 'seed')\n"
              "dialog.add_radio_button('radio', 'Choice', True)\n"
              "dialog.add_combo_box('combo', 'Option')\n"
              "dialog.add_list_view('list')\n"
              "dialog.add_tree_view('tree')\n"
              "dialog.add_button('ok', 'OK', 1)\n"
              "if dialog.show() != 1:\n"
              "    raise RuntimeError('dialog show failed')\n"
              "if dialog.get('value').get('text') != 'seed':\n"
              "    raise RuntimeError('dialog get failed')\n"
              "dialog.close()\n"),
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
    request.EntryPoint = script;
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
        for (int attempt = 0; attempt < 12 && state.SubscribeCalls == 0; ++attempt)
            Check(session->Pump(1000) != FALSE, "pump python bootstrap call");
        Check(state.CommandCalls == 1, "bootstrap command call reached host");
        Check(state.StorageCalls == 2, "bootstrap storage calls reached host");
        Check(state.SubscribeCalls == 1, "bootstrap event subscription reached host");
        Check(state.SideContextCalls == 1, "bootstrap side context reached host");
        Check(state.FileOperationCalls == 1, "bootstrap file operation reached host");
        Check(state.DialogCalls == 11, "bootstrap dialog calls reached host");
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
        for (int attempt = 0; attempt < 20 && session->IsAlive(); ++attempt)
            Sleep(25);
        Check(session->IsAlive() == FALSE, "bootstrap worker exits after shutdown");
        session->Stop();
        session->Release();
    }
    DeleteFileW(script);
}

void RunPowerShellBootstrapTest()
{
    wchar_t workerRoot[MAX_PATH * 4];
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", workerRoot, _countof(workerRoot));
    if (rootLength == 0 || rootLength >= _countof(workerRoot))
        return;
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"pwsh.exe", interpreter, _countof(interpreter)))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_POWERSHELL", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L"-bootstrap.ps1", script, _countof(script));
    Check(WriteScript(
              script,
              "if ($Salamander.commands.Execute('Copy') -ne 'ok') { throw 'command call failed' }\n"
              "$Salamander.storage.Set('bootstrap', 'ok')\n"
              "if ($Salamander.storage.Get('bootstrap') -ne 'ok') { throw 'storage call failed' }\n"
              "$null = $Salamander.events.Subscribe('hostStartup', { param($event) })\n"
              "$sideContext = $Salamander.SourceSide.Context()\n"
              "if ($sideContext.selectedCount -ne 1 -or $sideContext.focusedItem.name -ne 'seed.txt') { throw 'side context call failed' }\n"
              "if ($Salamander.file_operations.Refresh() -ne 'ok') { throw 'file operation call failed' }\n"
              "$dialog = $Salamander.ui.Dialog('Bootstrap')\n"
              "$dialog.AddLabel('label', 'Hello')\n"
              "$dialog.AddTextBox('value', 'seed')\n"
              "$dialog.AddRadioButton('radio', 'Choice', $true)\n"
              "$dialog.AddComboBox('combo', 'Option')\n"
              "$dialog.AddListView('list')\n"
              "$dialog.AddTreeView('tree')\n"
              "$dialog.AddButton('ok', 'OK', 1)\n"
              "if ($dialog.Show() -ne 1) { throw 'dialog show failed' }\n"
              "if ($dialog.Get('value').text -ne 'seed') { throw 'dialog get failed' }\n"
              "$dialog.Close()\n"),
          "write powershell bootstrap worker");
    CAutomationProcessRuntimeAdapter adapter(
        "PowerShell", "PowerShell", "powershell", ".ps1",
        L"SALAMATRIX_POWERSHELL", L"pwsh.exe", L"powershell.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPowerShell);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = script;
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
        for (int attempt = 0; attempt < 15 && state.SubscribeCalls == 0; ++attempt)
            Check(session->Pump(1000) != FALSE, "pump powershell bootstrap call");
        Check(state.CommandCalls == 1, "powershell bootstrap command call");
        Check(state.StorageCalls == 2, "powershell bootstrap storage calls");
        Check(state.SubscribeCalls == 1, "powershell bootstrap event subscription");
        Check(state.SideContextCalls == 1, "powershell bootstrap side context");
        Check(state.FileOperationCalls == 1, "powershell bootstrap file operation");
        Check(state.DialogCalls == 11, "powershell bootstrap dialog calls");
        std::string shutdown;
        Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageShutdown, 0, "{}", &shutdown);
        session->SendFrame(shutdown.c_str(), static_cast<DWORD>(shutdown.size()));
        for (int attempt = 0; attempt < 20 && session->IsAlive(); ++attempt)
            Sleep(25);
        Check(session->IsAlive() == FALSE, "powershell bootstrap exits after shutdown");
        session->Stop();
        session->Release();
    }
    DeleteFileW(script);
}

void RunPhpBootstrapTest()
{
    wchar_t workerRoot[MAX_PATH * 4];
    DWORD rootLength = GetEnvironmentVariableW(
        L"SALAMATRIX_WORKER_ROOT", workerRoot, _countof(workerRoot));
    if (rootLength == 0 || rootLength >= _countof(workerRoot))
        return;
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"php.exe", interpreter, _countof(interpreter)))
        return;
    SetEnvironmentVariableW(L"SALAMATRIX_PHP", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L"-bootstrap.php", script, _countof(script));
    Check(WriteScript(
              script,
              "<?php\n"
              "if ($Salamander->commands->execute('Copy') !== 'ok') throw new Exception('command call failed');\n"
              "$Salamander->storage->set('bootstrap', 'ok');\n"
              "if ($Salamander->storage->get('bootstrap') !== 'ok') throw new Exception('storage call failed');\n"
              "$Salamander->events->subscribe('hostStartup', function($event) {});\n"
              "$sideContext = $Salamander->source_side->context();\n"
              "if ($sideContext['selectedCount'] !== 1 || $sideContext['focusedItem']['name'] !== 'seed.txt') throw new Exception('side context call failed');\n"
              "if ($Salamander->file_operations->refresh() !== 'ok') throw new Exception('file operation call failed');\n"
              "$dialog = $Salamander->ui->dialog('Bootstrap');\n"
              "$dialog->addLabel('label', 'Hello');\n"
              "$dialog->addTextBox('value', 'seed');\n"
              "$dialog->addRadioButton('radio', 'Choice', true);\n"
              "$dialog->addComboBox('combo', 'Option');\n"
              "$dialog->addListView('list');\n"
              "$dialog->addTreeView('tree');\n"
              "$dialog->addButton('ok', 'OK', 1);\n"
              "if ($dialog->show() !== 1) throw new Exception('dialog show failed');\n"
              "if ($dialog->get('value')['text'] !== 'seed') throw new Exception('dialog get failed');\n"
              "$dialog->close();\n"
              "?>\n"),
          "write php bootstrap worker");
    CAutomationProcessRuntimeAdapter adapter(
        "PHP.CLI", "PHP", "php", ".php", L"SALAMATRIX_PHP", L"php.exe", NULL,
        CAutomationProcessRuntimeAdapter::ProcessKindPhp);
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = script;
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
        for (int attempt = 0; attempt < 15 && state.SubscribeCalls == 0; ++attempt)
            Check(session->Pump(1000) != FALSE, "pump php bootstrap call");
        Check(state.CommandCalls == 1, "php bootstrap command call");
        Check(state.StorageCalls == 2, "php bootstrap storage calls");
        Check(state.SubscribeCalls == 1, "php bootstrap event subscription");
        Check(state.SideContextCalls == 1, "php bootstrap side context");
        Check(state.FileOperationCalls == 1, "php bootstrap file operation");
        Check(state.DialogCalls == 11, "php bootstrap dialog calls");
        std::string shutdown;
        Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageShutdown, 0, "{}", &shutdown);
        session->SendFrame(shutdown.c_str(), static_cast<DWORD>(shutdown.size()));
        for (int attempt = 0; attempt < 20 && session->IsAlive(); ++attempt)
            Sleep(25);
        Check(session->IsAlive() == FALSE, "php bootstrap exits after shutdown");
        session->Stop();
        session->Release();
    }
    DeleteFileW(script);
}

void RunPhpTest()
{
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"php.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: php.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PHP", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L".php", script, _countof(script));
    Check(WriteScript(script, "<?php echo 'salamatrix-php-ok\\n'; ?>\n"),
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
    request.EntryPoint = script;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "php execution succeeds");
    Check(wcsstr(result.Output, L"salamatrix-php-ok") != NULL,
          "php output captured");
    DeleteFileW(script);
}
} // namespace

int main()
{
    RunPythonTests();
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
