/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-09 | Exercise the broker's image protocol check without elevation. | Compare known matching and stale PE resources directly. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Test canonical local pipe names and reject other path inputs. | Guard the elevated pipe path validation. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Exercise broker peer checks against a real local pipe. | Reject wrong process IDs and an unelevated identification token. | | GPT-6.1 Sol (Codex) |
*/

#define wmain InstalledProbeMain
#include "uacbroker.cpp"
#undef wmain

int TestClient(const wchar_t* name, DWORD parentPid)
{
    const auto parent = Adopt(OpenProcess(processAccess, FALSE, parentPid));
    const auto pipe = Adopt(CreateFileW(name, pipeAccess, 0, nullptr, OPEN_EXISTING,
                                        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                                        nullptr));
    if (!parent || !pipe ||
        !ExpectedPipePeer(pipe.get(), parent.get(), parentPid, true) ||
        ExpectedPipePeer(pipe.get(), GetCurrentProcess(), GetCurrentProcessId(), true))
        return 1;

    const auto deadline = GetTickCount64() + timeoutMs;
    Frame frame{protocolVersion, capabilities, Phase::Hello};
    if (Exchange(pipe.get(), frame, true, deadline))
        return 2;
    // The host closes the pipe after checking the identification token.
    return Exchange(pipe.get(), frame, false, deadline) == ERROR_BROKEN_PIPE ? 0 : 3;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 4 && std::wstring_view(argv[1]) == L"--protocol-images")
    {
        if (!MatchesProtocolImage(argv[2]) || MatchesProtocolImage(argv[3]))
            return 12;
        std::puts("PASS accept the matching PE protocol resource and reject a stale image");
        return 0;
    }
    if (argc == 3)
        return TestClient(argv[1], wcstoul(argv[2], nullptr, 10));

    const ComApartment com;
    if (FAILED(com.result))
        return 4;
    {
        std::wstring unusedName;
        const auto unused = CreateEndpoint(unusedName);
        if (!unused || Connect(unused.get(), GetTickCount64() + 100) != ERROR_TIMEOUT)
            return 10;
    }

    std::wstring name;
    auto pipe = CreateEndpoint(name);
    if (name.empty() || ParsePipeName(name) != name ||
        !ParsePipeName(LR"(\\server\pipe\OpenSamandarin-UAC-{00000000-0000-0000-0000-000000000000})").empty() ||
        !ParsePipeName(std::wstring(pipePrefix) + L"{00000000-0000-0000-0000-00000000000Z}").empty() ||
        !ParsePipeName(name + L"\\other").empty())
        return 11;
    std::wstring exe(32768, L'\0');
    const DWORD chars = GetModuleFileNameW(nullptr, exe.data(), static_cast<DWORD>(exe.size()));
    if (!pipe || !chars || chars >= exe.size())
        return 5;
    exe.resize(chars);
    std::wstring args = L"\"" + exe + L"\" " + name + L" " + std::to_wstring(GetCurrentProcessId());
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), args.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process))
        return 6;
    const auto child = Adopt(process.hProcess);
    CloseHandle(process.hThread);

    const auto deadline = GetTickCount64() + timeoutMs;
    if (Connect(pipe.get(), deadline) ||
        !ExpectedPipePeer(pipe.get(), child.get(), process.dwProcessId, false) ||
        ExpectedPipePeer(pipe.get(), GetCurrentProcess(), GetCurrentProcessId(), false))
        return 7;
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken))
        return 8;
    const auto token = Adopt(rawToken);
    const bool expectedElevation = Elevated(token.get());
    Frame frame{};
    bool actualElevation = false;
    if (Exchange(pipe.get(), frame, false, deadline) || !Matches(frame, Phase::Hello) ||
        !InspectPipeClient(pipe.get(), actualElevation) || actualElevation != expectedElevation)
        return 8;

    pipe.reset();
    DWORD exitCode = 0;
    if (WaitForSingleObject(child.get(), timeoutMs) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child.get(), &exitCode) || exitCode != 0)
        return 9;
    std::printf("Real pipe: both mismatched PIDs rejected, identification token %s, peer exited on disconnect, connection deadline expired as expected.\n",
                expectedElevation ? "elevation confirmed" : "correctly rejected as unelevated");
    return 0;
}
