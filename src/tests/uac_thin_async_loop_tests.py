# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the actual async transfer loop on isolated local NTFS files.

Surrounding worker, dialog, and session APIs are adapted; network buffering is
stubbed and the test is not complete DoCopyFile, elevated broker, or UAC.
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile
from uac_thin_sync_loop_tests import build_source
from safe_file_paths_tests import ROOT, compiler_environment, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
config = parser.parse_args().configuration
worker_path, header_path = ROOT / 'src/worker.cpp', ROOT / 'src/worker.h'
before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in (worker_path, header_path)}
worker = worker_path.read_text(encoding='utf-8-sig')
header = header_path.read_text(encoding='utf-8-sig')
code = build_source(worker, header)
code = code[:code.index('void DoCopyFileLoopOrig(')]

def adapt(old, new):
    global code
    if code.count(old) != 1:
        raise RuntimeError('test adapter no longer matches its template: ' + old)
    code = code.replace(old, new, 1)

adapt('    CQuadWord& operator+=', '    void SetUI64(unsigned __int64 v) { Value = v; }\n    CQuadWord& operator+=')
adapt('static bool operator<(CQuadWord a, CQuadWord b)', 'static bool operator>=(CQuadWord a, CQuadWord b) { return a.Value >= b.Value; }\nstatic bool operator<(CQuadWord a, CQuadWord b)')
adapt('static bool operator<(CQuadWord a, CQuadWord b)', 'static CQuadWord operator-(CQuadWord a, CQuadWord b) { return CQuadWord(a.Value - b.Value); }\nstatic bool operator<=(CQuadWord a, CQuadWord b) { return a.Value <= b.Value; }\nstatic bool operator!=(CQuadWord a, CQuadWord b) { return a.Value != b.Value; }\nstatic bool operator<(CQuadWord a, CQuadWord b)')
adapt('CQuadWord Size; };', 'CQuadWord Size; DWORD OpFlags = 0; };')
adapt('unsigned progressBytes = 0;', 'unsigned progressBytes = 0; void SetTFSandProgressSize(CQuadWord, CQuadWord) {}')
adapt('    bool terminal = false;', '    bool terminal = false, brokerHandleDelivered = true;\n    bool Scoped() const { return true; }\n    DWORD WaitTransferResume() const { return ERROR_SUCCESS; }')
adapt('injectFault && writes == 1 && !didFault', 'injectFault && writes > 0 && !didFault')
adapt('if (result) ++writes; return result;', 'return result;')
code += '\n#define OPFL_SRCPATH_IS_NET 1\n#define OPFL_TGTPATH_IS_NET 2\n#define IDB_IGNORE 1003\n#define IDB_IGNOREALL 1004\n'
code += '\nstatic bool IsLockRecoveryError(DWORD) { return false; }\nstruct CFileLockRecoveryDialogData { int* result; const char* caption; const char* path; const char* error; const wchar_t* wide; };\nstatic void ThinCopyTestMarkWriteCompleted(bool) { ++writes; }\n'
start = worker.index('enum CCopy_BlkState')
end = worker.index('static UacF7::Result ExecuteWithNativeTransferProgress', start)
section = worker[start:end]
begin = section.index('BOOL DisableLocalBuffering(')
finish = section.index('BOOL CCopy_Context::StartReading(', begin)
section = section[:begin] + 'BOOL DisableLocalBuffering(CAsyncCopyParams*, HANDLE, DWORD*) { return TRUE; }\n' + section[finish:]
code += section
code += r'''
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 2;
    const bool retry = argv[2][0] == L'1';
    const std::wstring source = std::wstring(argv[1]) + L"\\source-東.bin";
    const std::wstring target = std::wstring(argv[1]) + L"\\target-日.bin";
    std::vector<BYTE> expected(65539);
    for (size_t i=0; i<expected.size(); ++i) expected[i]=static_cast<BYTE>((i*31+i/251)&255);
    HANDLE seed = CreateFileW(source.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr);
    DWORD count=0;
    if (seed == INVALID_HANDLE_VALUE || !WriteFile(seed, expected.data(), static_cast<DWORD>(expected.size()), &count, nullptr) || count != expected.size()) return 3;
    CloseHandle(seed);
    HANDLE in = CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    HANDLE out = CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED, nullptr);
    if (in == INVALID_HANDLE_VALUE || out == INVALID_HANDLE_VALUE) return 4;
    BOOL cancel=FALSE, error=FALSE, skip=FALSE, again=FALSE;
    HANDLE resumed=CreateEventW(nullptr, TRUE, TRUE, nullptr);
    CProgressDlgData dialog{&cancel, resumed, nullptr};
    char inName[]="source", outName[]="target";
    COperation operation{}; operation.SourceName=inName; operation.TargetName=outName;
    operation.SourceNameW=source; operation.TargetNameW=target; operation.Size=CQuadWord(expected.size());
    COperations script; script.TotalSize=operation.Size;
    CAsyncCopyParams parameters; parameters.Init(TRUE);
    if (parameters.Failed()) return 5;
    CQuadWord done, total, size(expected.size()), previous;
    int limit=1024, allocation=0;
    CThinCopyWorker thin;
    injectFault=retry; didFault=false; writes=0; dialogCount=0; dialogChoice=IDRETRY;
    DoCopyFileLoopAsync(&parameters, in, out, nullptr, limit, &script, dialog, FALSE, &operation,
                        total, error, skip, nullptr, done, size, 1024, allocation, again, previous, &thin);
    if (in && in != INVALID_HANDLE_VALUE) CloseHandle(in);
    if (out && out != INVALID_HANDLE_VALUE) CloseHandle(out);
    CloseHandle(resumed);
    HANDLE result=CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    std::vector<BYTE> actual(expected.size()); count=0; LARGE_INTEGER actualSize{};
    const bool ok = !error && !skip && !again && (!retry || (didFault && dialogCount == 1)) && done.Value == expected.size() && result != INVALID_HANDLE_VALUE &&
        GetFileSizeEx(result, &actualSize) && actualSize.QuadPart == static_cast<LONGLONG>(expected.size()) &&
        ReadFile(result, actual.data(), static_cast<DWORD>(actual.size()), &count, nullptr) && count == actual.size() && actual == expected;
    if (result != INVALID_HANDLE_VALUE) CloseHandle(result);
    std::printf("%s original async loop%s transferred exact content: %llu bytes, fault %d, dialogs %u\n", ok ? "PASS" : "FAIL", retry ? " with Retry" : "", done.Value, didFault, dialogCount);
    if (!DeleteFileW(source.c_str()) || !DeleteFileW(target.c_str())) return 6;
    return ok ? 0 : 1;
}
'''
env = compiler_environment()
compiler = shutil.which('cl', path=env.get('PATH'))
if not compiler:
    raise RuntimeError('MSVC cl.exe unavailable')
flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
with tempfile.TemporaryDirectory(prefix='thin-async-loop-', dir=ROOT.parent / 'builds') as directory:
    root = Path(directory)
    source, exe = root / 'test.cpp', root / 'test.exe'
    source.write_text(code, encoding='utf-8')
    result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', '/wd4100',
                  '/wd4201', '/wd4244', *flags, str(source), '/Fe:' + str(exe)], cwd=str(root), env=env, timeout=60)
    if result.returncode: raise SystemExit(result.returncode)
    for retry in ('0', '1'):
        result = run([str(exe), directory, retry], cwd=str(root), env=env, timeout=25)
        if result.returncode: raise SystemExit(result.returncode)
for path, digest in before.items():
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise RuntimeError(str(path) + ' changed during test')
print('worker.cpp SHA-256:', before[worker_path])
print('Actual extracted async loop; local overlapped NTFS I/O with adapted surrounding APIs.')
print('Not complete DoCopyFile, actual privileged broker, UAC or VM acceptance.')
