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
parser.add_argument('--emit-linked', type=Path, help=argparse.SUPPRESS)
args = parser.parse_args()
config = args.configuration
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
adapt('    HANDLE (*sessionRetryTarget)(void*) = nullptr;', '    HANDLE (*sessionRetryTarget)(void*) = nullptr;\n    HANDLE (*sessionRetrySource)(void*) = nullptr;')
adapt('#define THIN_OPEN_SOURCE_RETRY(ctx, local, overlapped) (local)',
      '#define THIN_OPEN_SOURCE_RETRY(ctx, local, overlapped) ((ctx) && (ctx)->sessionRetrySource ? (ctx)->sessionRetrySource((ctx)->sessionContext) : (local))')
adapt('#define THIN_COPY_READ ReadFile', r'''
static bool injectReadFault=false, didReadFault=false;
static unsigned readAttempts=0;
static BOOL TestRead(HANDLE h, LPVOID buffer, DWORD bytes, LPDWORD read, LPOVERLAPPED overlapped) {
    ++readAttempts;
    if (injectReadFault && readAttempts == 2 && !didReadFault) {
        didReadFault=true; SetLastError(ERROR_READ_FAULT); return FALSE;
    }
    return ReadFile(h, buffer, bytes, read, overlapped);
}
#define THIN_COPY_READ TestRead''')
code += '\n#define OPFL_SRCPATH_IS_NET 1\n#define OPFL_TGTPATH_IS_NET 2\n#define IDB_IGNORE 1003\n#define IDB_IGNOREALL 1004\n'
code += '\nstatic bool IsLockRecoveryError(DWORD) { return false; }\nstruct CFileLockRecoveryDialogData { int* result; const char* caption; const char* path; const char* error; const wchar_t* wide; };\nstatic void ThinCopyTestMarkWriteCompleted(bool) { ++writes; }\n'
start = worker.index('enum CCopy_BlkState')
end = worker.index('static UacF7::Result ExecuteWithNativeTransferProgress', start)
section = worker[start:end]
begin = section.index('BOOL DisableLocalBuffering(')
finish = section.index('BOOL CCopy_Context::StartReading(', begin)
section = section[:begin] + 'BOOL DisableLocalBuffering(CAsyncCopyParams*, HANDLE, DWORD*) { return TRUE; }\n' + section[finish:]
code += section
if args.emit_linked:
    code += r'''
extern "C" BOOL RunLinkedNativeCopyLoop(HANDLE* input, HANDLE* output, const wchar_t* source, const wchar_t* target,
                                        ULONGLONG fileBytes, ULONGLONG expectedBytes, void* context, DWORD (*check)(void*),
                                        HANDLE (*retry)(void*), bool (*discard)(void*), BOOL expectRestart)
{
    BOOL cancel=FALSE, error=FALSE, skipped=FALSE, again=FALSE;
    HANDLE resumed=CreateEventW(nullptr, TRUE, TRUE, nullptr);
    if (!resumed) return FALSE;
    CProgressDlgData dialog{&cancel, resumed, nullptr};
    char inName[]="source", outName[]="target";
    COperation op{}; op.SourceName=inName; op.TargetName=outName; op.SourceNameW=source; op.TargetNameW=target;
    op.Size=CQuadWord(fileBytes);
    COperations script; script.TotalSize=op.Size;
    CAsyncCopyParams parameters; parameters.Init(TRUE);
    if (parameters.Failed()) { CloseHandle(resumed); return FALSE; }
    CQuadWord done, total, size(fileBytes), previous;
    int limit=1024, allocation=0;
    CThinCopyWorker thin; thin.sessionContext=context; thin.sessionCheck=check;
    thin.sessionRetryTarget=retry; thin.sessionDiscardTarget=discard;
    injectFault=true; didFault=false; writes=dialogCount=0; dialogChoice=IDRETRY;
    DoCopyFileLoopAsync(&parameters, *input, *output, nullptr, limit, &script, dialog, FALSE, &op,
                        total, error, skipped, nullptr, done, size, 1024, allocation, again, previous, &thin);
    CloseHandle(resumed);
    return !error && !skipped && (again != FALSE) == (expectRestart != FALSE) && !thin.reports &&
           didFault && dialogCount == 1 && (expectRestart || done.Value == expectedBytes);
}

extern "C" BOOL RunLinkedNativeReadRetryLoop(HANDLE* input, HANDLE* output, const wchar_t* source, const wchar_t* target,
                                             ULONGLONG fileBytes, void* context, DWORD (*check)(void*), HANDLE (*retrySource)(void*))
{
    BOOL cancel=FALSE, error=FALSE, skipped=FALSE, again=FALSE;
    HANDLE resumed=CreateEventW(nullptr, TRUE, TRUE, nullptr);
    if (!resumed) return FALSE;
    CProgressDlgData dialog{&cancel, resumed, nullptr};
    char inName[]="source", outName[]="target";
    COperation op{}; op.SourceName=inName; op.TargetName=outName; op.SourceNameW=source; op.TargetNameW=target;
    op.Size=CQuadWord(fileBytes);
    COperations script; script.TotalSize=op.Size;
    CAsyncCopyParams parameters; parameters.Init(TRUE);
    if (parameters.Failed()) { CloseHandle(resumed); return FALSE; }
    CQuadWord done, total, size(fileBytes), previous;
    int limit=1024, allocation=0;
    CThinCopyWorker thin; thin.sessionContext=context; thin.sessionCheck=check; thin.sessionRetrySource=retrySource;
    injectFault=false; didFault=false; writes=dialogCount=readAttempts=0;
    injectReadFault=true; didReadFault=false; dialogChoice=IDRETRY;
    DoCopyFileLoopAsync(&parameters, *input, *output, nullptr, limit, &script, dialog, FALSE, &op,
                        total, error, skipped, nullptr, done, size, 1024, allocation, again, previous, &thin);
    CloseHandle(resumed);
    return !error && !skipped && !again && !thin.reports && didReadFault && dialogCount == 1 && done.Value == fileBytes;
}
'''
    args.emit_linked.write_text(code, encoding='utf-8')
    raise SystemExit(0)
code += r'''
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 2;
    const wchar_t mode = argv[2][0];
    if (mode < L'0' || mode > L'8' || argv[2][1] != L'\0') return 2;
    const bool readFault = mode >= L'5';
    const bool inject = mode != L'0';
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
    injectFault=inject && !readFault; didFault=false; writes=0; dialogCount=0;
    injectReadFault=readFault; didReadFault=false; readAttempts=0;
    dialogChoice=(mode == L'2' || mode == L'6') ? IDB_SKIP :
                 (mode == L'3' || mode == L'7') ? IDB_SKIPALL :
                 (mode == L'4' || mode == L'8') ? IDCANCEL : IDRETRY;
    DoCopyFileLoopAsync(&parameters, in, out, nullptr, limit, &script, dialog, FALSE, &operation,
                        total, error, skip, nullptr, done, size, 1024, allocation, again, previous, &thin);
    if (in && in != INVALID_HANDLE_VALUE) CloseHandle(in);
    if (out && out != INVALID_HANDLE_VALUE) CloseHandle(out);
    CloseHandle(resumed);
    HANDLE result=CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    std::vector<BYTE> actual(expected.size()); count=0; LARGE_INTEGER actualSize{};
    bool ok = result != INVALID_HANDLE_VALUE && GetFileSizeEx(result, &actualSize) &&
        actualSize.QuadPart >= 0 && actualSize.QuadPart <= static_cast<LONGLONG>(expected.size());
    if (mode <= L'1' || mode == L'5') {
        ok = ok && !error && !skip && !again && done.Value == expected.size() &&
            actualSize.QuadPart == static_cast<LONGLONG>(expected.size()) &&
            ReadFile(result, actual.data(), static_cast<DWORD>(actual.size()), &count, nullptr) && count == actual.size() && actual == expected;
    } else {
        ok = ok && !again && ((mode == L'4' || mode == L'8') ? error && !skip : !error && skip) &&
            (dialog.SkipAllFileWrite != FALSE) == (mode == L'3') &&
            (dialog.SkipAllFileRead != FALSE) == (mode == L'7') &&
            (readFault || actualSize.QuadPart > 0) &&
            ReadFile(result, actual.data(), static_cast<DWORD>(actualSize.QuadPart), &count, nullptr) &&
            count == static_cast<DWORD>(actualSize.QuadPart) &&
            std::equal(actual.begin(), actual.begin() + count, expected.begin());
    }
    ok = ok && (inject ? (readFault ? didReadFault : didFault) && dialogCount == 1 : !didFault && !didReadFault && dialogCount == 0);
    const bool firstDidFault=readFault ? didReadFault : didFault;
    const unsigned firstDialogCount=dialogCount;
    if (result != INVALID_HANDLE_VALUE) CloseHandle(result);
    HANDLE sourceCheck=CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    LARGE_INTEGER sourceSize{}; count=0; std::vector<BYTE> sourceBytes(expected.size());
    ok = ok && sourceCheck != INVALID_HANDLE_VALUE && GetFileSizeEx(sourceCheck, &sourceSize) &&
        sourceSize.QuadPart == static_cast<LONGLONG>(expected.size()) &&
        ReadFile(sourceCheck, sourceBytes.data(), static_cast<DWORD>(sourceBytes.size()), &count, nullptr) &&
        count == sourceBytes.size() && sourceBytes == expected;
    if (sourceCheck != INVALID_HANDLE_VALUE) CloseHandle(sourceCheck);
    if ((mode == L'3' || mode == L'7') && ok) {
        const std::wstring secondTarget = std::wstring(argv[1]) + L"\\target-second-東.bin";
        in=CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        out=CreateFileW(secondTarget.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED, nullptr);
        resumed=CreateEventW(nullptr, TRUE, TRUE, nullptr);
        ok = in != INVALID_HANDLE_VALUE && out != INVALID_HANDLE_VALUE && resumed != nullptr;
        if (ok) {
            CProgressDlgData nextDialog{&cancel, resumed, nullptr};
            nextDialog.SkipAllFileWrite=dialog.SkipAllFileWrite;
            nextDialog.SkipAllFileRead=dialog.SkipAllFileRead;
            COperation nextOperation=operation; nextOperation.TargetNameW=secondTarget.c_str();
            CAsyncCopyParams nextParameters; nextParameters.Init(TRUE);
            error=skip=again=FALSE; CQuadWord nextDone, nextTotal, nextSize(expected.size()), nextPrevious;
            writes=dialogCount=readAttempts=0; injectFault=mode == L'3'; didFault=false;
            injectReadFault=mode == L'7'; didReadFault=false; dialogChoice=IDCANCEL;
            if (nextParameters.Failed()) ok=false;
            else {
                DoCopyFileLoopAsync(&nextParameters, in, out, nullptr, limit, &script, nextDialog, FALSE, &nextOperation,
                                    nextTotal, error, skip, nullptr, nextDone, nextSize, 1024, allocation, again, nextPrevious, &thin);
                ok = !error && skip && !again && dialogCount == 0 &&
                    (mode == L'3' ? didFault && nextDialog.SkipAllFileWrite : didReadFault && nextDialog.SkipAllFileRead);
            }
        }
        if (in && in != INVALID_HANDLE_VALUE) CloseHandle(in);
        if (out && out != INVALID_HANDLE_VALUE) CloseHandle(out);
        if (resumed) CloseHandle(resumed);
        if (!DeleteFileW(secondTarget.c_str())) return 6;
        std::printf("%s persisted Skip All skips a later %s failure without a new dialog\n", ok ? "PASS" : "FAIL", mode == L'3' ? "write" : "read");
    }
    std::printf("%s original async loop mode %lc: done %llu bytes, target %lld bytes, fault %d, dialogs %u, error %d, skip %d, skip-all-read %d, skip-all-write %d\n", ok ? "PASS" : "FAIL", mode, done.Value, actualSize.QuadPart, firstDidFault, firstDialogCount, error, skip, dialog.SkipAllFileRead, dialog.SkipAllFileWrite);
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
    for mode in ('0', '1', '2', '3', '4', '5', '6', '7', '8'):
        result = run([str(exe), directory, mode], cwd=str(root), env=env, timeout=25)
        if result.returncode: raise SystemExit(result.returncode)
for path, digest in before.items():
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise RuntimeError(str(path) + ' changed during test')
print('worker.cpp SHA-256:', before[worker_path])
print('Actual extracted async loop; local overlapped NTFS I/O with adapted surrounding APIs.')
print('Not complete DoCopyFile, actual privileged broker, UAC or VM acceptance.')
