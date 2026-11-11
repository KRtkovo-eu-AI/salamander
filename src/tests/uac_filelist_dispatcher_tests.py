# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Test the generated source on a real broker job pipe. | Verify scoped parent opens, publication and collision. | #731 | GPT-6.1 Sol |
"""Unelevated broker dispatcher test; does not test installed-image handshake or UAC."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run


MAIN = r'''
int wmain()
{
    Directory source, target;
    UacFileList::Workspace workspace;
    if (workspace.Create() || !target.Open(L"target")) return 2;
    DWORD written = 0;
    const BYTE value = 47;
    if (!WriteFile(workspace.Writer(), &value, 1, &written, nullptr) || written != 1 || workspace.Seal()) return 3;
    std::wstring sourcePath = workspace.Path();
    sourcePath.resize(sourcePath.find_last_of(L'\\'));
    source.handle = Adopt(CreateFileW(sourcePath.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!source.handle || UacDirectoryLeaf::ReadParentIdentity(source.handle.get(), source.identity)) return 4;
    source.path = sourcePath;
    constexpr DWORD flags = VOLUME_NAME_GUID | FILE_NAME_OPENED;
    const DWORD count = GetFinalPathNameByHandleW(source.handle.get(), nullptr, 0, flags);
    if (!count) return 5;
    std::wstring canonical(count, L'\0');
    const DWORD length = GetFinalPathNameByHandleW(source.handle.get(), canonical.data(), count, flags);
    if (!length || length >= count || length <= source.identity.volume.size()) return 6;
    canonical.resize(length);
    source.relative = canonical.substr(source.identity.volume.size() + 1);
    JobPipe job;
    DWORD result = ERROR_GEN_FAILURE;
    Check(job.Open() && job.Request(Operation::CopyIgnoreAds, source, L"list.tmp", result, &target,
          nullptr, nullptr, false, nullptr, nullptr, 1) && result == ERROR_SUCCESS && HasByte(target.File(L"list.tmp"), value),
          "copy the pinned generated source through the real broker dispatcher");
    Check(job.Request(Operation::CopyIgnoreAds, source, L"list.tmp", result, &target,
          nullptr, nullptr, false, nullptr, nullptr, 1) &&
          (result == ERROR_FILE_EXISTS || result == ERROR_ALREADY_EXISTS) && HasByte(target.File(L"list.tmp"), value),
          "broker rejects a destination collision without replacing it");
    Check(DeleteFileW(target.File(L"list.tmp").c_str()) != FALSE, "remove only the successful test output before identity tests");
    const auto noPublication = [&] {
        if (GetFileAttributesW(target.File(L"list.tmp").c_str()) != INVALID_FILE_ATTRIBUTES ||
            GetLastError() != ERROR_FILE_NOT_FOUND) return false;
        WIN32_FIND_DATAW data{};
        HANDLE find = FindFirstFileW(target.File(L".samandarin-copy-*.tmp").c_str(), &data);
        if (find != INVALID_HANDLE_VALUE) { FindClose(find); return false; }
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    };
    FILE_ID_128 wrong = workspace.Identity().FileId;
    wrong.Identifier[15] ^= 1;
    Check(job.Request(Operation::CopyIgnoreAds, source, L"list.tmp", result, &target,
          nullptr, nullptr, false, nullptr, nullptr, 1, {}, &wrong) && result == ERROR_FILE_INVALID && noPublication(),
          "broker rejects a mismatched full 128-bit source identity");
    const auto correctParent = source.identity.file.FileId;
    source.identity.file.FileId.Identifier[15] ^= 1;
    const bool parentReply = job.Request(Operation::CopyIgnoreAds, source, L"list.tmp", result, &target,
          nullptr, nullptr, false, nullptr, nullptr, 1);
    const DWORD dispatchError = job.Finish();
    std::printf("Changed parent existing-job dispatcher result: %lu\n", dispatchError);
    Check(!parentReply && dispatchError == ERROR_ACCESS_DENIED && noPublication(),
          "existing broker job rejects a changed source-parent scope before dispatch");
    JobPipe fresh;
    result = ERROR_GEN_FAILURE;
    const bool freshReply = fresh.Open() && fresh.Request(Operation::CopyIgnoreAds, source, L"list.tmp", result, &target,
          nullptr, nullptr, false, nullptr, nullptr, 1);
    std::printf("Changed parent fresh-job terminal result: %lu\n", result);
    Check(freshReply && result == ERROR_FILE_INVALID && noPublication(),
          "fresh broker job rejects a mismatched full 128-bit source parent identity");
    fresh.Finish();
    source.identity.file.FileId = correctParent;
    source.handle.reset();
    target.handle.reset();
    RemoveDirectoryW(L"target");
    return failures ? 1 : 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ['/MTd', '/Od', '/D_DEBUG'] if args.configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
    broker = ROOT / 'src/uacbroker/uacbroker_job_tests.cpp'
    source = broker.read_text(encoding='utf-8-sig')
    marker = 'int wmain()'
    if source.count(marker) != 1:
        raise RuntimeError('broker fixture entry point is ambiguous')
    source = source.replace(marker, 'int ExistingBrokerFixtureMain()', 1)
    source += '\n#include "' + str(ROOT / 'src/uacfilelist.h').replace('\\', '/') + '"\n' + MAIN
    with tempfile.TemporaryDirectory(prefix='uac-filelist-dispatcher-') as folder:
        directory = Path(folder)
        cpp = directory / 'dispatcher.cpp'
        exe = directory / 'test.exe'
        cpp.write_text(source, encoding='utf-8')
        command = [compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                   '/I' + str(ROOT / 'src/uacbroker'), str(cpp), '/Fe:' + str(exe), '/link',
                   'ntdll.lib', 'ole32.lib', 'advapi32.lib', 'shell32.lib', 'uuid.lib', 'bcrypt.lib']
        result = run(command, cwd=directory, env=env, timeout=45)
        if result.returncode:
            return result.returncode
        return run([str(exe)], cwd=directory, env=env, timeout=15).returncode


if __name__ == '__main__':
    raise SystemExit(main())
