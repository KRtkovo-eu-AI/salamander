<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Document the protected P2 diagnostic pair. | Make startup and shutdown checks repeatable in the VM. | | GPT-6.1 Sol (Codex) |
-->

# P2 UAC broker handshake

The solution builds two executables from uacbroker.cpp. The diagnostic host uacbrokerprobe.exe uses the asInvoker manifest. The separate uacbroker.exe uses requireAdministrator. Both use uiAccess=false, a static CRT and system-only dependent DLL lookup. The broker also restricts subsequent default DLL lookup to System32. The solution outputs both executables under output/uac/<configuration>_<platform> or the corresponding configured build root. This pair is staged by an administrator in the VM, separately from the application installer. The diagnostic host stands in for the future Samandarin integration.

Use the default Program Files known folder for the executable architecture. Create the Open Samandarin UAC subdirectory there and place both matching binaries in it. The launch path is fixed. The code rejects reparse points, unexpected ownership and ACL entries that grant mutation rights to untrusted principals on Program Files, the staging directory or the relevant images. A handle without delete sharing is held on both directories throughout the handshake. The host holds the verified broker file without write or delete sharing through launch and handshake. The elevated process verifies the protected host image and checks that the pipe server is the same live process in the same Windows session. The host checks the client PID against the process handle returned by ShellExecuteExW and inspects its elevated identification token. Same-account and alternate-account elevation use the same checks. [Administrator Broker Model](https://learn.microsoft.com/en-us/windows/win32/secauthz/administrator-broker-model) · [Named pipe access](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights)

The host creates one local pipe instance with a fresh name and an explicit DACL. The Administrators ACE has access mask 0x00100083. The host's SID owns the server permissions. Client connections use SecurityIdentification. The three fixed frames carry protocol version 1, capabilities 0 and a handshake phase. There is one handshake per process and bounded, cancelable overlapped I/O. The helper exits after completion, incompatible protocol, loss of the host, or expiration of the handshake deadline. The diagnostic modes do not request privileged work.

| Invocation in an unelevated PowerShell | Expected result |
| --- | --- |
| uacbrokerprobe.exe outside the protected directory | Exit 3, with no UAC prompt. |
| uacbrokerprobe.exe in the protected directory, approve | Exit 0, with authenticated elevated helper exit 0. |
| uacbrokerprobe.exe --drop, approve | Exit 0 for the test, with authenticated helper exit 5 after the host closes the pipe. |
| uacbrokerprobe.exe --bad-version, approve | Exit 0 for the test, with authenticated helper exit 4 after the incompatible response. |
| uacbrokerprobe.exe in the protected directory, cancel | Exit 2 and Windows error 1223. |

The protected-path check assumes an administrator controls the staged package. Production signing, package installation, integration with the normal UI and authority for individual privileged operations are separate acceptance gates. The P2 diagnostic protocol has zero file capabilities.

## Local verification

Run python -B src/tests/uacbroker_transport_tests.py --configuration Debug or Release. The fixture uses real Windows pipe endpoints and two processes. It checks server and client PID mismatches, the client's identification token, peer exit after disconnect and cancellation of a connection deadline. It does not request elevation. The projects were built locally with warnings as errors for x64, Win32 and ARM64. The embedded execution levels, uiAccess=false and dependent load flag 0x0800 were inspected. The protected launch guard rejected the probe in the build directory.

The isolated VM scenarios in the table require the staged pair and a person at the secure desktop. The P0 results used different laboratory executables and are not results for this pair.
