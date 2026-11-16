# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise listing-derived archive preparation. | Verify scope, wire data and NTFS identity checks. | #731 | GPT-6.1 Sol |
"""Exercise actual ZIP preparation and framing with a local NTFS-backed peer.

The peer emulates the listing protocol, and RunProbe is a recording seam; this
does not start an elevated broker or exercise a panel/plugin transition.
"""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    production = read('src/uacbroker/uacbroker.cpp')
    template = read('src/tests/uac_zip_listing_prepare_tests.cpp.in')
    global_methods = ('bool IsStopSignaled(', 'DWORD FinishIo(', 'DWORD TransferRequest(')
    members = ('PreparedLeaf::~PreparedLeaf(', 'ListingSession::~ListingSession(',
               'void ListingSession::Close(',
               'bool ListingSession::Contains(', 'DWORD ListingSession::Transfer(',
               'DWORD ListingSession::SendPath(', 'DWORD ListingSession::PrepareLeaf(',
               'ArchiveReadSession::~ArchiveReadSession(', 'void ArchiveReadSession::Close(',
               'DWORD ArchiveReadSession::PrepareFromListing(', 'DWORD ArchiveReadSession::Start(')
    template = template.replace('@@GLOBAL_METHODS@@', '\n\n'.join(block(production, s) for s in global_methods))
    template = template.replace('@@REQUEST_HEADER@@', block(production, 'struct RequestHeader', ';'))
    template = template.replace('@@MEMBER_METHODS@@', block(production, 'class ListingForegroundLock', ';') +
                                '\n\n' + '\n\n'.join(block(production, s) for s in members))
    template = template.replace('@@BROKER_DIR@@', (ROOT / 'src/uacbroker').as_posix())
    if '@@' in template:
        raise RuntimeError('unexpanded test placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='uac-zip-listed-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(template, encoding='utf-8')
        for config in ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,):
            flags = ['/MTd', '/Od', '/D_DEBUG'] if config == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
            exe = folder / (config + '.exe')
            print('ZIP listed identity:', config, flush=True)
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(generated), '/Fe:' + str(exe), '/link', 'ntdll.lib'], cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            result = run([str(exe), str(folder)], cwd=folder, env=env, timeout=25)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
