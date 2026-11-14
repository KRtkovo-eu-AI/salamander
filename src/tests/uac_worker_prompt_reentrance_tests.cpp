// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Exercise nested worker prompt completion using real Windows messages. | Check stack lifetime and deferred completion under modal reentrancy. | #731 | GPT-6.1 Sol |
// This is a minimal model of the relevant handler, not CProgressDialog itself.
// It uses real HWNDs, DialogBox, SendMessage, PostMessage and ReplyMessage.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cwchar>

namespace
{
constexpr UINT promptMessage = WM_APP + 91;
constexpr LPARAM foreignCompletion = 0x131;
constexpr LPARAM workerCompletion = 0x132;

struct State
{
    HANDLE modalReady = nullptr, worker = nullptr, injector = nullptr, workerContinue = nullptr, cancel = nullptr;
    HWND parent = nullptr, modal = nullptr;
    volatile LONG active = 0, payloadAlive = 0, workerReturned = 0, guardedForeign = 0;
    volatile LONG payloadWasAlive = 0, workerFinished = 0, foreignTimeout = 0, modalError = 0;
    volatile LONG gotDecision = 0, completed = 0;
    DWORD sendState = 0;
    bool guarded = false, foreignSent = false, requestCancel = false;
};

struct Payload { int decision = 0; };

INT_PTR CALLBACK ModalProc(HWND dialog, UINT message, WPARAM, LPARAM parameter)
{
    if (message == WM_INITDIALOG)
    {
        auto* state = reinterpret_cast<State*>(parameter);
        state->modal = dialog;
        SetEvent(state->modalReady);
        return TRUE;
    }
    return FALSE;
}

LRESULT CALLBACK ParentProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    if (!state) return DefWindowProcW(window, message, wparam, lparam);
    if (message == promptMessage)
    {
        InterlockedExchange(&state->active, 1);
        struct Template { DLGTEMPLATE dialog; WORD menu, className, title; } definition{};
        definition.dialog.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
        definition.dialog.cx = 100; definition.dialog.cy = 50;
        const INT_PTR result = DialogBoxIndirectParamW(GetModuleHandleW(nullptr), &definition.dialog,
                                                        window, ModalProc, reinterpret_cast<LPARAM>(state));
        if (result == -1) InterlockedExchange(&state->modalError, 1);
        const bool alive = InterlockedCompareExchange(&state->payloadAlive, 0, 0) != 0;
        InterlockedExchange(&state->payloadWasAlive, alive ? 1 : 0);
        // Never dereference a potentially expired stack object in the legacy
        // observation; the guarded path checks lifetime before touching it.
        if (state->guarded && alive)
            reinterpret_cast<Payload*>(lparam)->decision =
                WaitForSingleObject(state->cancel, 0) == WAIT_OBJECT_0 || result != IDYES ? IDCANCEL : IDYES;
        InterlockedExchange(&state->active, 0);
        return TRUE;
    }
    if (message == WM_COMMAND && LOWORD(wparam) == IDOK)
    {
        if (lparam == foreignCompletion && InterlockedCompareExchange(&state->active, 0, 0))
        {
            state->sendState = InSendMessageEx(nullptr);
            if (state->modal) EndDialog(state->modal, IDCANCEL);
            if (state->guarded)
            {
                InterlockedExchange(&state->guardedForeign, 1);
                return TRUE; // Do not reply to a worker prompt or finalize its lifetime.
            }
            if (InSendMessage()) ReplyMessage(0); // Observe the production-shaped legacy path.
            SetEvent(state->workerContinue);
            if (WaitForSingleObject(state->worker, 300) == WAIT_TIMEOUT)
                InterlockedExchange(&state->foreignTimeout, 1);
            return TRUE;
        }
        if (lparam == workerCompletion)
        {
            if (InSendMessage()) ReplyMessage(0);
            SetEvent(state->workerContinue);
            if (WaitForSingleObject(state->worker, 3000) == WAIT_OBJECT_0)
                InterlockedExchange(&state->completed, 1);
            PostQuitMessage(0);
            return TRUE;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

DWORD WINAPI Worker(void* value)
{
    auto& state = *static_cast<State*>(value);
    Payload payload;
    InterlockedExchange(&state.payloadAlive, 1);
    SendMessageW(state.parent, promptMessage, 0, reinterpret_cast<LPARAM>(&payload));
    InterlockedExchange(&state.workerReturned, 1);
    InterlockedExchange(&state.gotDecision, payload.decision);
    InterlockedExchange(&state.payloadAlive, 0);
    SendMessageW(state.parent, WM_COMMAND, IDOK, workerCompletion);
    if (WaitForSingleObject(state.workerContinue, 3000) == WAIT_OBJECT_0)
        InterlockedExchange(&state.workerFinished, 1);
    return 0;
}

DWORD WINAPI Injector(void* value)
{
    auto& state = *static_cast<State*>(value);
    if (WaitForSingleObject(state.modalReady, 3000) != WAIT_OBJECT_0) return 1;
    if (state.requestCancel) SetEvent(state.cancel);
    if (state.foreignSent)
        SendMessageW(state.parent, WM_COMMAND, IDOK, foreignCompletion);
    else if (!PostMessageW(state.parent, WM_COMMAND, IDOK, foreignCompletion))
        return 2;
    return 0;
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 4) return 2;
    State state;
    state.guarded = wcscmp(argv[1], L"guard") == 0;
    state.foreignSent = wcscmp(argv[2], L"sent") == 0;
    state.requestCancel = wcscmp(argv[3], L"cancel") == 0;
    state.modalReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    state.workerContinue = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    state.cancel = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!state.modalReady || !state.workerContinue || !state.cancel) return 3;
    WNDCLASSW wc{}; wc.lpfnWndProc = ParentProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"SamandarinUacWorkerPromptTest";
    if (!RegisterClassW(&wc)) return 4;
    state.parent = CreateWindowExW(0, wc.lpszClassName, L"test", WS_OVERLAPPED,
                                   0, 0, 100, 100, nullptr, nullptr, wc.hInstance, &state);
    if (!state.parent) return 5;
    state.worker = CreateThread(nullptr, 0, Worker, &state, CREATE_SUSPENDED, nullptr);
    state.injector = CreateThread(nullptr, 0, Injector, &state, 0, nullptr);
    if (!state.worker || !state.injector || ResumeThread(state.worker) == static_cast<DWORD>(-1)) return 6;
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
    const DWORD injectorWait = WaitForSingleObject(state.injector, 3000);
    const bool pass = state.guarded && state.guardedForeign && state.payloadWasAlive && state.workerReturned &&
                      state.workerFinished && state.completed && !state.modalError && injectorWait == WAIT_OBJECT_0 &&
                      state.gotDecision == IDCANCEL;
    std::printf("mode=%s foreign=%s cancel=%d sendState=%lu guarded=%ld alive=%ld workerReturned=%ld "
                "workerFinished=%ld completed=%ld foreignTimeout=%ld decision=%ld modalError=%ld result=%s\n",
                state.guarded ? "guard" : "legacy-observe", state.foreignSent ? "sent" : "posted", state.requestCancel,
                state.sendState, state.guardedForeign, state.payloadWasAlive, state.workerReturned, state.workerFinished,
                state.completed, state.foreignTimeout, state.gotDecision, state.modalError, pass ? "PASS" : "OBSERVE");
    DestroyWindow(state.parent);
    CloseHandle(state.injector); CloseHandle(state.worker); CloseHandle(state.cancel);
    CloseHandle(state.workerContinue); CloseHandle(state.modalReady);
    return state.guarded && !pass ? 1 : 0;
}
