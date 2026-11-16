# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Share scoped Find authorization. | Reuse approved reads without widening content rights. | #731 | GPT-6 |
# | 2026-11-12 | Run identity-bearing paged dispatcher tests. | Verify real NTFS and pipe frames beyond one listing budget. | #731 | GPT-6 (Codex) |
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-paged-dispatch-') as temp:
        job = (ROOT / 'src/uacbroker/uacbroker_job_tests.cpp').read_text(encoding='utf-8-sig')
        if job.count('int wmain(int argc, wchar_t** argv)\n{') != 1 or job.count('#include "uacbroker.cpp"') != 1:
            raise AssertionError('unexpected dispatcher fixture layout')
        job = job.replace('int wmain(int argc, wchar_t** argv)\n{', 'int ExistingJobFixtureMain(int argc, wchar_t** argv)\n{')
        job = job.replace('#include "uacbroker.cpp"',
                          '#include "' + (ROOT / 'src/uacbroker/uacbroker.cpp').as_posix() + '"')
        job = job.replace('class JobPipe\n{\npublic:\n',
                          'class JobPipe\n{\npublic:\n    HANDLE DetachClientForPanel() { return server.release(); }\n', 1)
        job = ('#include <windows.h>\n#include <cstring>\n#include <atomic>\n#include <array>\n'
               '#include <mutex>\n#include <memory>\n#include <string>\n#include <string_view>\n#include <vector>\n'
               '#define private public\n#include "' + (ROOT / 'src/uacbroker/f7pilot.h').as_posix() + '"\n#undef private\n') + job
        production = read('src/uacbroker/uacbroker.cpp')
        members = ('PreparedLeaf::~PreparedLeaf(', 'ListingSession::~ListingSession(',
                   'static DWORD LockListingForCall(', 'void ListingSession::Close(',
                   'size_t ListingSession::FindReadScope(', 'bool ListingSession::Contains(',
                   'DWORD ListingSession::Transfer(', 'DWORD ListingSession::SendPath(',
                   'DWORD ListingSession::PrepareLeaf(', 'DWORD ListingSession::CaptureFindRoot(',
                   'DWORD ListingSession::Inventory(',
                   'DWORD ListingSession::List(', 'DWORD ListingSession::ListPanel(',
                   'DWORD ListingSession::OpenDirectoryCursor(', 'DWORD ListingSession::NextDirectoryPage(',
                   'DWORD ListingSession::CloseDirectoryCursor(')
        client = 'namespace UacF7 {\n' + block(production, 'class ListingForegroundLock', ';') + '\n' + \
                 '\n'.join(block(production, marker) for marker in members) + '\n}\n'
        fixture = (ROOT / 'src/tests/uac_paged_dispatch_tests.cpp.in').read_text(encoding='utf-8-sig')
        if fixture.count('@@JOB_FIXTURE@@') != 1:
            raise AssertionError('missing dispatcher fixture placeholder')
        source = Path(temp) / 'test.cpp'
        source.write_text(fixture.replace('@@JOB_FIXTURE@@', job + '\n' + client), encoding='utf-8')
        exe = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                     '/I' + str(ROOT / 'src/uacbroker'), str(source), '/Fe:' + str(exe),
                     '/link', '/MANIFEST:EMBED', 'advapi32.lib', 'ole32.lib', 'shell32.lib', 'uuid.lib', 'ntdll.lib'],
                    cwd=temp, env=env, timeout=35)
        if built.returncode:
            return built.returncode
        return run([str(exe)], cwd=temp, env=env, timeout=60).returncode


if __name__ == '__main__':
    raise SystemExit(main())
