# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise protected Find scan incompleteness. | Keep existing limits and errors distinct from completed results. | #731 | GPT-6.1 Terra |
"""Compile the production protected Find scan and finish methods with controlled seams."""
import argparse
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import compiler_environment, block, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    source, header = read('src/find.cpp'), read('src/find.h')
    fixture = read('src/tests/find_protected_scan_incomplete_tests.cpp.in')
    for name, content in {
        'REASONS': block(header, 'enum FindIncompleteReason', ';'),
        'IGNORE_TYPE': block(header, 'enum CFindIgnoreItemType', ';'),
        'IGNORE_PREPARE': block(source, 'BOOL CFindIgnore::Prepare('),
        'IGNORE_SKIP_ROOT': block(source, 'const char* SkipRoot('),
        'IGNORE_CONTAINS': block(source, 'BOOL CFindIgnore::Contains('),
        'IGNORE': block(source, 'bool IsIgnored(const std::wstring& directory)'),
        'CONTINUE': block(source, 'bool CanContinue(const std::wstring& directory)'),
        'SCAN': block(source, 'void Scan(const std::wstring& directory, unsigned depth'),
        'FINISH': block(source, 'UacF7::ListingSession* Finish(CDuplicateCandidates* candidates)'),
        'MATCH_FILE': block(source, 'bool MatchFile(const std::wstring& directory, const WIN32_FIND_DATAW& file'),
    }.items():
        fixture = fixture.replace('@@' + name + '@@', content)
    assert '@@' not in fixture
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='find-protected-scan-') as temp:
        folder = Path(temp)
        generated = folder / 'test.cpp'
        generated.write_text(fixture, encoding='utf-8')
        for config in (('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)):
            flags = ['/MTd', '/Od', '/D_DEBUG'] if config == 'Debug' else ['/MT', '/O2', '/DNDEBUG']
            exe = folder / (config + '.exe')
            print('Actual protected Find scan:', config, flush=True)
            result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                          str(generated), '/Fe:' + str(exe)], cwd=folder, env=env, timeout=40)
            if result.returncode:
                return result.returncode
            result = run([str(exe)], cwd=folder, env=env, timeout=20)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
