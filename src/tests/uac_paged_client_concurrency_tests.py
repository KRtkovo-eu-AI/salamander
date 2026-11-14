# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise Stop during a content read with active cursors. | Verify production pipe cancellation releases all leases. | #731 | GPT-6.1 Terra |
# | 2026-11-12 | Exercise concurrent paged listing client requests. | Verify leases, serialized pipe frames and cancellation. | #731 | GPT-6 (Codex) |
"""Exercise extracted production ListingSession methods with a controlled local pipe peer."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    production = read('src/uacbroker/uacbroker.cpp')
    print('Production source SHA-256:', hashlib.sha256(production.encode('utf-8')).hexdigest(), flush=True)
    source = read('src/tests/uac_paged_client_concurrency_tests.cpp.in')
    globals_ = ('bool IsStopSignaled(', 'DWORD FinishIo(', 'DWORD TransferRequest(')
    members = ('PreparedLeaf::~PreparedLeaf(', 'ListingSession::~ListingSession(',
               'static DWORD LockListingForCall(', 'void ListingSession::Close(',
               'bool ListingSession::Contains(', 'DWORD ListingSession::Transfer(',
               'DWORD ListingSession::SendPath(', 'DWORD ListingSession::List(',
               'DWORD ListingSession::OpenContent(', 'DWORD ListingSession::ReadContent(', 'DWORD ListingSession::CloseContent(',
               'DWORD ListingSession::OpenDirectoryCursor(', 'DWORD ListingSession::NextDirectoryPage(',
               'DWORD ListingSession::CloseDirectoryCursor(')
    source = source.replace('@@GLOBAL_METHODS@@', '\n\n'.join(block(production, marker) for marker in globals_))
    foreground = block(production, 'class ListingForegroundLock', ';')
    source = source.replace('@@MEMBER_METHODS@@', foreground + '\n\n' + '\n\n'.join(block(production, marker) for marker in members))
    source = source.replace('@@BROKER_DIR@@', (ROOT / 'src/uacbroker').as_posix())
    if '@@' in source:
        raise AssertionError('unexpanded fixture placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='uac-paged-client-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(source, encoding='utf-8')
        configurations = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
        for configuration in configurations:
            flags = ['/MTd', '/Od', '/D_DEBUG'] if configuration == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
            exe = folder / (configuration + '.exe')
            print('Paged listing concurrency:', configuration, flush=True)
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(generated), '/Fe:' + str(exe), '/link', 'ntdll.lib'], cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            result = run([str(exe)], cwd=folder, env=env, timeout=25)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
